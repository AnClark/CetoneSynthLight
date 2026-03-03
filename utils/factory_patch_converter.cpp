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

    // The .clight format wraps the preset in a single-element bank object.
    if (!j.contains("presets") || !j["presets"].is_array() ||
        j["presets"].empty()) {
        return false;
    }

    const json& pj = j["presets"][0];
    memset(&out, 0, sizeof(SynthProgram));

    // Name
    if (pj.contains("name")) {
        std::string n = pj["name"];
        strncpy(out.Name, n.c_str(), 63);
        out.Name[63] = '\0';
    }

    // Global
    if (pj.contains("volume"))  out.Volume  = pj["volume"];
    if (pj.contains("panning")) out.Panning = pj["panning"];
    if (pj.contains("coarse"))  out.Coarse  = pj["coarse"];
    if (pj.contains("fine"))    out.Fine    = pj["fine"];

    // Filter
    if (pj.contains("filterType")) out.FilterType = pj["filterType"];
    if (pj.contains("filterMode")) out.FilterMode = pj["filterMode"];
    if (pj.contains("cutoff"))     out.Cutoff     = pj["cutoff"];
    if (pj.contains("resonance"))  out.Resonance  = pj["resonance"];

    // Portamento
    if (pj.contains("portaMode"))  out.PortaMode  = pj["portaMode"];
    if (pj.contains("portaSpeed")) out.PortaSpeed = pj["portaSpeed"];

    // Arpeggio
    if (pj.contains("arpMode"))  out.ArpMode  = pj["arpMode"];
    if (pj.contains("arpSpeed")) out.ArpSpeed = pj["arpSpeed"];
#ifdef ENABLE_POLYPHONY
    if (pj.contains("arpPoly"))      out.ArpPoly      = pj["arpPoly"];
    if (pj.contains("maxPolyphony")) out.MaxPolyphony = pj["maxPolyphony"];
#endif

    // Oscillators (Voice[0..2]; Voice[3] is unused in the JSON format)
    for (int i = 0; i < 3; i++) {
        char key[32];
        std::snprintf(key, sizeof(key), "osc%dVolume", i + 1);
        if (pj.contains(key)) out.Voice[i].Volume = pj[key];
        std::snprintf(key, sizeof(key), "osc%dCoarse", i + 1);
        if (pj.contains(key)) out.Voice[i].Coarse = pj[key];
        std::snprintf(key, sizeof(key), "osc%dFine", i + 1);
        if (pj.contains(key)) out.Voice[i].Fine   = pj[key];
        std::snprintf(key, sizeof(key), "osc%dWave", i + 1);
        if (pj.contains(key)) out.Voice[i].Wave   = pj[key];
        std::snprintf(key, sizeof(key), "osc%dPw", i + 1);
        if (pj.contains(key)) out.Voice[i].Pw     = pj[key];
        std::snprintf(key, sizeof(key), "osc%dRing", i + 1);
        if (pj.contains(key)) out.Voice[i].Ring   = pj[key];
        std::snprintf(key, sizeof(key), "osc%dSync", i + 1);
        if (pj.contains(key)) out.Voice[i].Sync   = pj[key];
    }

    // Envelopes
    if (pj.contains("env1Attack"))  out.Attack[0]  = pj["env1Attack"];
    if (pj.contains("env1Hold"))    out.Hold[0]    = pj["env1Hold"];
    if (pj.contains("env1Decay"))   out.Decay[0]   = pj["env1Decay"];
    if (pj.contains("env1Sustain")) out.Sustain[0] = pj["env1Sustain"];
    if (pj.contains("env1Release")) out.Release[0] = pj["env1Release"];
    if (pj.contains("env2Attack"))  out.Attack[1]  = pj["env2Attack"];
    if (pj.contains("env2Hold"))    out.Hold[1]    = pj["env2Hold"];
    if (pj.contains("env2Decay"))   out.Decay[1]   = pj["env2Decay"];
    if (pj.contains("env2Sustain")) out.Sustain[1] = pj["env2Sustain"];
    if (pj.contains("env2Release")) out.Release[1] = pj["env2Release"];

    // LFO
    if (pj.contains("lfoSpeed"))   out.LfoSpeed   = pj["lfoSpeed"];
    if (pj.contains("lfoWave"))    out.LfoWave    = pj["lfoWave"];
    if (pj.contains("lfoPw"))      out.LfoPw      = pj["lfoPw"];
    if (pj.contains("lfoTrigger")) out.LfoTrigger = pj["lfoTrigger"];

    // Modulations
    for (int i = 0; i < 4; i++) {
        char key[32];
        std::snprintf(key, sizeof(key), "mod%dSrc",    i + 1);
        if (pj.contains(key)) out.Modulations[i].Source        = pj[key];
        std::snprintf(key, sizeof(key), "mod%dDest",   i + 1);
        if (pj.contains(key)) out.Modulations[i].Destination   = pj[key];
        std::snprintf(key, sizeof(key), "mod%dAmount", i + 1);
        if (pj.contains(key)) out.Modulations[i].Amount        = pj[key];
        std::snprintf(key, sizeof(key), "mod%dMul",    i + 1);
        if (pj.contains(key)) out.Modulations[i].Multiplicator = pj[key];
    }

    // Filter envelope modulation
    if (pj.contains("envMod")) out.EnvMod = pj["envMod"];

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
