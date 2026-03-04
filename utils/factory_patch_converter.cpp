// =============================================================================
// factory_patch_converter.cpp
//
// Scans a directory for CetoneSynthLight preset files (.clight), parses each
// one, and emits a C++ header containing a static const SynthProgram array
// that can be compiled directly into the plugin as factory presets.
//
// Usage:
//   factory_patch_converter <preset_dir> <output_header>
//
// Example:
//   factory_patch_converter ./factory_presets ../src/FactoryPresets.h
// =============================================================================

// Standalone utility: define ENABLE_POLYPHONY to match plugin build when needed.
// By default we do NOT define it here; adjust if the plugin enables polyphony.

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#else
#  include <dirent.h>
#  include <sys/stat.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Pull in nlohmann/json (single-header) from the project's 3rdparty folder.
#include "../src/3rdparty/json.hpp"

// Pull in the shared data structures.
#include "../src/Structures.h"
#include "../src/Defines.h"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// File helpers
// ---------------------------------------------------------------------------

static std::string readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ---------------------------------------------------------------------------
// Directory scan – returns all .clight files in dir (no recursion)
// ---------------------------------------------------------------------------

static std::vector<std::string> scanPresetFiles(const std::string& dir)
{
    std::vector<std::string> result;

#ifdef _WIN32
    std::string pattern = dir + "\\*.clight";
    WIN32_FIND_DATAA fd{};
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE)
        return result;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            result.push_back(dir + "\\" + fd.cFileName);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir.c_str());
    if (!d) {
        log_err("Cannot open directory: %s", dir.c_str());
        return result;
    }
    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        std::string name = ent->d_name;
        if (name.size() > 7 && name.substr(name.size() - 7) == ".clight") {
            // Check it is a regular file (skip subdirs that somehow end in .clight)
            std::string fullPath = dir + "/" + name;
            struct stat st{};
            if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
                result.push_back(fullPath);
        }
    }
    closedir(d);
#endif

    std::sort(result.begin(), result.end());
    return result;
}

// ---------------------------------------------------------------------------
// JSON → SynthProgram  (mirrors CPresetManager::deserializeBankFromJSON)
// ---------------------------------------------------------------------------

static bool parsePresetFromJSON(const std::string& jsonText, SynthProgram& out)
{
    if (jsonText.empty())
        return false;

    json j;
    try {
        j = json::parse(jsonText);
    } catch (const std::exception&) {
        return false;
    }

    // Validate: must be a singlePreset file (not a bank file)
    if (!j.contains("formatVersion"))
        return false;
    if (j["formatVersion"].get<std::string>() != "1.0.0")
        return false;
    if (j.contains("bankName") || j.contains("presets"))
        return false; // This is a bank file, not a single preset

    memset(&out, 0, sizeof(SynthProgram));

    // Name
    if (j.contains("name")) {
        std::string n = j["name"];
        strncpy(out.Name, n.c_str(), 63);
        out.Name[63] = '\0';
    }

    // Global
    if (j.contains("volume"))  out.Volume  = j["volume"];
    if (j.contains("panning")) out.Panning = j["panning"];
    if (j.contains("coarse"))  out.Coarse  = j["coarse"];
    if (j.contains("fine"))    out.Fine    = j["fine"];

    // Filter
    if (j.contains("cutoff"))     out.Cutoff     = j["cutoff"];
    if (j.contains("resonance"))  out.Resonance  = j["resonance"];
    if (j.contains("filterType")) out.FilterType = j["filterType"];
    if (j.contains("filterMode")) out.FilterMode = j["filterMode"];

    // Portamento
    if (j.contains("portaMode"))  out.PortaMode  = j["portaMode"];
    if (j.contains("portaSpeed")) out.PortaSpeed = j["portaSpeed"];

    // Arpeggio
    if (j.contains("arpMode"))  out.ArpMode  = j["arpMode"];
    if (j.contains("arpSpeed")) out.ArpSpeed = j["arpSpeed"];
#ifdef ENABLE_POLYPHONY
    if (j.contains("arpPoly"))      out.ArpPoly      = j["arpPoly"];
    if (j.contains("maxPolyphony")) out.MaxPolyphony = j["maxPolyphony"];
#endif

    // Envelopes
    for (int i = 0; i < 2; i++) {
        std::string p = "env" + std::to_string(i + 1);
        if (j.contains(p + "Attack"))  out.Attack[i]  = j[p + "Attack"];
        if (j.contains(p + "Hold"))    out.Hold[i]    = j[p + "Hold"];
        if (j.contains(p + "Decay"))   out.Decay[i]   = j[p + "Decay"];
        if (j.contains(p + "Sustain")) out.Sustain[i] = j[p + "Sustain"];
        if (j.contains(p + "Release")) out.Release[i] = j[p + "Release"];
    }

    // LFO
    if (j.contains("lfoSpeed"))   out.LfoSpeed   = j["lfoSpeed"];
    if (j.contains("lfoWave"))    out.LfoWave    = j["lfoWave"];
    if (j.contains("lfoPw"))      out.LfoPw      = j["lfoPw"];
    if (j.contains("lfoTrigger")) out.LfoTrigger = j["lfoTrigger"];

    // Oscillators — stored as a "voices" array: [ {volume,coarse,fine,wave,pw,ring,sync}, ... ]
    if (j.contains("voices") && j["voices"].is_array()) {
        const auto& voices = j["voices"];
        for (size_t i = 0; i < voices.size() && i < 3; i++) {
            const auto& v = voices[i];
            if (v.contains("volume")) out.Voice[i].Volume = v["volume"];
            if (v.contains("coarse")) out.Voice[i].Coarse = v["coarse"];
            if (v.contains("fine"))   out.Voice[i].Fine   = v["fine"];
            if (v.contains("wave"))   out.Voice[i].Wave   = v["wave"];
            if (v.contains("pw"))     out.Voice[i].Pw     = v["pw"];
            if (v.contains("ring"))   out.Voice[i].Ring   = v["ring"];
            if (v.contains("sync"))   out.Voice[i].Sync   = v["sync"];
        }
    }

    // Modulation matrix — stored as a "modulations" array: [ {source,destination,amount,multiplicator}, ... ]
    if (j.contains("modulations") && j["modulations"].is_array()) {
        const auto& mods = j["modulations"];
        for (size_t i = 0; i < mods.size() && i < 4; i++) {
            const auto& m = mods[i];
            if (m.contains("source"))        out.Modulations[i].Source        = m["source"];
            if (m.contains("destination"))   out.Modulations[i].Destination   = m["destination"];
            if (m.contains("amount"))        out.Modulations[i].Amount        = m["amount"];
            if (m.contains("multiplicator")) out.Modulations[i].Multiplicator = m["multiplicator"];
        }
    }

    // Filter envelope modulation
    if (j.contains("envMod")) out.EnvMod = j["envMod"];

    return true;
}

// ---------------------------------------------------------------------------
// Escape a C string literal
// ---------------------------------------------------------------------------

static std::string escapeString(const char* s)
{
    std::string out;
    out.reserve(strlen(s) + 2);
    for (const char* p = s; *p; ++p) {
        switch (*p) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(*p) < 0x20)
                    out += "?";
                else
                    out += *p;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Float → C literal  (always valid: has decimal point or exponent, plus 'f')
// ---------------------------------------------------------------------------

static std::string flt(float v)
{
    // %.8g emits the shortest representation without a trailing 'f'.
    // E.g.: 0.0 → "0", 1.0 → "1", 0.01 → "0.01", 1e-5 → "1e-05"
    char base[64];
    std::snprintf(base, sizeof(base), "%.8g", static_cast<double>(v));

    bool hasDot = (strchr(base, '.') != nullptr);
    bool hasExp = (strchr(base, 'e') != nullptr || strchr(base, 'E') != nullptr);

    char buf[72];
    if (!hasDot && !hasExp) {
        // Integer-looking output like "0" or "32768" – append ".0f"
        std::snprintf(buf, sizeof(buf), "%s.0f", base);
    } else {
        // Already has dot or exponent – just append 'f'
        std::snprintf(buf, sizeof(buf), "%sf", base);
    }
    return buf;
}

// ---------------------------------------------------------------------------
// bool → C literal
// ---------------------------------------------------------------------------

static const char* bl(bool v) { return v ? "true" : "false"; }

// ---------------------------------------------------------------------------
// SynthVoice → C initializer  { Volume, Coarse, Fine, Wave, Pw, Ring, Sync }
// ---------------------------------------------------------------------------

static std::string voiceInit(const SynthVoice& v)
{
    std::ostringstream s;
    s << "{ " << flt(v.Volume)
      << ", " << v.Coarse
      << ", " << v.Fine
      << ", " << v.Wave
      << ", " << v.Pw
      << ", " << bl(v.Ring)
      << ", " << bl(v.Sync)
      << " }";
    return s.str();
}

// ---------------------------------------------------------------------------
// SynthModulation → C initializer  { Source, Destination, Amount, Multiplicator }
// ---------------------------------------------------------------------------

static std::string modInit(const SynthModulation& m)
{
    std::ostringstream s;
    s << "{ " << m.Source
      << ", " << m.Destination
      << ", " << flt(m.Amount)
      << ", " << flt(m.Multiplicator)
      << " }";
    return s.str();
}

// ---------------------------------------------------------------------------
// SynthProgram → C designated-initializer block
// ---------------------------------------------------------------------------

static std::string programInit(const SynthProgram& p, const std::string& indent)
{
    const std::string i2 = indent + "    ";
    const std::string i3 = i2    + "    ";

    std::ostringstream s;
    s << indent << "{\n";

    // Name
    s << i2 << "/* Name       */ \"" << escapeString(p.Name) << "\",\n";

    // Global
    s << i2 << "/* Volume     */ " << flt(p.Volume)  << ",\n";
    s << i2 << "/* Panning    */ " << flt(p.Panning) << ",\n";
    s << i2 << "/* Coarse     */ " << p.Coarse       << ",\n";
    s << i2 << "/* Fine       */ " << p.Fine         << ",\n";

    // Filter
    s << i2 << "/* Cutoff     */ " << flt(p.Cutoff)     << ",\n";
    s << i2 << "/* Resonance  */ " << flt(p.Resonance)  << ",\n";
    s << i2 << "/* FilterType */ " << p.FilterType       << ",\n";
    s << i2 << "/* FilterMode */ " << p.FilterMode       << ",\n";

    // Arpeggio
    s << i2 << "/* ArpMode    */ " << p.ArpMode  << ",\n";
    s << i2 << "/* ArpSpeed   */ " << p.ArpSpeed << ",\n";
#ifdef ENABLE_POLYPHONY
    s << i2 << "/* ArpPoly    */ " << bl(p.ArpPoly) << ",\n";
#endif

    // Portamento
    s << i2 << "/* PortaMode  */ " << bl(p.PortaMode)    << ",\n";
    s << i2 << "/* PortaSpeed */ " << flt(p.PortaSpeed)  << ",\n";

    // Envelopes
    s << i2 << "/* Attack[2]  */ { " << flt(p.Attack[0])  << ", " << flt(p.Attack[1])  << " },\n";
    s << i2 << "/* Hold[2]    */ { " << flt(p.Hold[0])    << ", " << flt(p.Hold[1])    << " },\n";
    s << i2 << "/* Decay[2]   */ { " << flt(p.Decay[0])   << ", " << flt(p.Decay[1])   << " },\n";
    s << i2 << "/* Sustain[2] */ { " << flt(p.Sustain[0]) << ", " << flt(p.Sustain[1]) << " },\n";
    s << i2 << "/* Release[2] */ { " << flt(p.Release[0]) << ", " << flt(p.Release[1]) << " },\n";

    // LFO
    s << i2 << "/* LfoSpeed   */ " << flt(p.LfoSpeed) << ",\n";
    s << i2 << "/* LfoWave    */ " << p.LfoWave        << ",\n";
    s << i2 << "/* LfoPw      */ " << p.LfoPw          << ",\n";
    s << i2 << "/* LfoTrigger */ " << bl(p.LfoTrigger) << ",\n";

    // Voices[4]
    s << i2 << "/* Voice[4]   */\n";
    s << i2 << "{\n";
    for (int vi = 0; vi < 4; vi++) {
        s << i3 << voiceInit(p.Voice[vi]);
        if (vi < 3) s << ",";
        s << "  // Voice[" << vi << "]\n";
    }
    s << i2 << "},\n";

    // Modulations[4]
    s << i2 << "/* Mod[4]     */\n";
    s << i2 << "{\n";
    for (int mi = 0; mi < 4; mi++) {
        s << i3 << modInit(p.Modulations[mi]);
        if (mi < 3) s << ",";
        s << "  // Mod[" << mi << "]\n";
    }
    s << i2 << "},\n";

    // EnvMod
#ifdef ENABLE_POLYPHONY
    s << i2 << "/* EnvMod     */ " << flt(p.EnvMod) << ",\n";
    s << i2 << "/* MaxPoly    */ " << p.MaxPolyphony << "\n";
#else
    s << i2 << "/* EnvMod     */ " << flt(p.EnvMod) << "\n";
#endif

    s << indent << "}";
    return s.str();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr,
            "Usage: factory_patch_converter <preset_dir> <output_header>\n"
            "\n"
            "  preset_dir     - directory containing *.clight preset files\n"
            "  output_header  - path to the C++ header file to generate\n");
        return 1;
    }

    const std::string presetDir   = argv[1];
    const std::string outputPath  = argv[2];

    // ---- scan -------------------------------------------------------
    fprintf(stdout, "* Preset directory  : %s\n", presetDir.c_str());
    fprintf(stdout, "* Output header file: %s\n\n", outputPath.c_str());

    std::vector<std::string> files = scanPresetFiles(presetDir);

    if (files.empty()) {
        fprintf(stderr, "ERROR: No .clight preset files found in: %s\n", presetDir.c_str());
        return 1;
    }

    fprintf(stdout, "* Found %zu .clight file(s). Reading presets:\n", files.size());

    // ---- parse -------------------------------------------------------
    std::vector<SynthProgram> presets;
    presets.reserve(files.size());
    int failCount = 0;

    for (const auto& path : files) {
        // Extract just the filename for display
        std::string displayName = path;
        size_t slash = displayName.find_last_of("/\\");
        if (slash != std::string::npos)
            displayName = displayName.substr(slash + 1);

        std::string jsonText = readFile(path);
        if (jsonText.empty()) {
            fprintf(stdout, "    - [FAIL] %-40s  (cannot read file)\n", displayName.c_str());
            ++failCount;
            continue;
        }

        SynthProgram prog{};
        if (!parsePresetFromJSON(jsonText, prog)) {
            fprintf(stdout, "    - [FAIL] %-40s  (invalid preset format)\n", displayName.c_str());
            ++failCount;
            continue;
        }

        fprintf(stdout, "    - [ OK ] %-40s  \"%s\"\n", displayName.c_str(), prog.Name);
        presets.push_back(prog);
    }

    // ---- summary ----------------------------------------------------
    fprintf(stdout, "\n* Result: %zu succeeded, %d failed.\n\n",
            presets.size(), failCount);

    if (presets.empty()) {
        fprintf(stderr, "ERROR: No presets were successfully parsed. Aborting.\n");
        return 1;
    }

    // ---- emit header -------------------------------------------------
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        fprintf(stderr, "ERROR: Cannot open output file for writing: %s\n", outputPath.c_str());
        return 1;
    }

    // Derive a guard symbol from the filename
    std::string guardBase = outputPath;
    {
        size_t pos = guardBase.find_last_of("/\\");
        if (pos != std::string::npos)
            guardBase = guardBase.substr(pos + 1);
        for (char& c : guardBase)
            c = (std::isalnum(static_cast<unsigned char>(c))) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '_';
    }
    const std::string guard = guardBase;

    out << "// =============================================================\n";
    out << "// AUTO-GENERATED by factory_patch_converter\n";
    out << "// DO NOT EDIT MANUALLY\n";
    out << "//\n";
    out << "// Source directory : " << presetDir  << "\n";
    out << "// Preset count     : " << presets.size() << "\n";
    out << "// =============================================================\n";
    out << "\n";
    out << "#pragma once\n";
    out << "\n";
    out << "#include \"Structures.h\"\n";
    out << "\n";
    out << "static const int kFactoryPresetCount = " << presets.size() << ";\n";
    out << "\n";
    out << "static const SynthProgram kFactoryPresets[" << presets.size() << "] =\n";
    out << "{\n";

    for (size_t idx = 0; idx < presets.size(); idx++) {
        out << programInit(presets[idx], "    ");
        if (idx + 1 < presets.size())
            out << ",";
        out << "  // [" << idx << "] " << presets[idx].Name << "\n";
    }

    out << "};\n";
    out.close();

    fprintf(stdout, "* Header written to: %s  (%zu preset(s))\n", outputPath.c_str(), presets.size());
    return 0;
}
