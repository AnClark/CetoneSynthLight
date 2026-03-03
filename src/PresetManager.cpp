#include "PresetManager.h"

#include "3rdparty/json.hpp"
#include "CetoneUI.hpp"   // For class CCetoneUI
#include "Defines.h"      // For constants like FTYPE_MAX, OWAVE_MAX, etc.

#ifdef DISTRHO_OS_WINDOWS
#include <shlobj.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef INCLUDE_FACTORY_PRESETS
#include "factory_presets.h"   // Auto-generated header containing factory presets
#endif

using json = nlohmann::json;

// Internal conversion utilities (UI-side, no dependency on CCetoneSynth)
namespace {

// Convert bool to parameter value [0.0, 1.0]
inline float bool2val(bool value) { return value ? 1.0f : 0.0f; }

// Convert coarse pitch [-50, 50] to parameter value [0.0, 1.0]  (same range as CCetoneSynth::c_coarse2val)
inline float coarse2val(int value) {
    return static_cast<float>(value + 50) / 100.0f;
}

// Convert fine pitch [-100, 100] to parameter value [0.0, 1.0]
inline float fine2val(int value) {
    return static_cast<float>(value + 100) / 200.0f;
}

// Convert integer enum [0, max] to parameter value [0.0, 1.0]
inline float int2val(int value, int max) {
    return static_cast<float>(value) / static_cast<float>(max + 1);
}

// Clamp float to range [min, max]
inline float clampf(float value, float min, float max) {
    return (value < min) ? min : ((value > max) ? max : value);
}

// Clamp int to range [min, max]
inline int clampi(int value, int min, int max) {
    return (value < min) ? min : ((value > max) ? max : value);
}

} // anonymous namespace

// Default program matching InitSynthParameters() defaults
static const SynthProgram DefaultProgram = {
    DEFAULT_PRESET_NAME, // Name

    // Global
    1.0f,   // Volume  (DSP internal: 1.0;  param = Volume/5 = 0.2)
    0.5f,   // Panning (direct: 0.5)

    0,      // Coarse (main)
    0,      // Fine (main)

    // Filter
    1.0f,       // Cutoff   (direct: 1.0)
    0.0f,       // Resonance
    FTYPE_NONE, // FilterType
    FMODE_LOW,  // FilterMode

    // Arpeggio
    -1,     // ArpMode (off)
    20,     // ArpSpeed (ms)
#ifdef ENABLE_POLYPHONY
    false,  // ArpPoly
#endif

    // Portamento
    false,  // PortaMode
    0.1f,   // PortaSpeed (DSP internal: 0.1;  param = PortaSpeed/5 = 0.02)

    // Envelopes — stored as DSP internal values; loadProgram divides by 10 (except Sustain)
    {0.01f,  0.0f},    // Attack  [0]: DSP=0.01
    {0.02f,  0.0f},    // Hold    [0]: DSP=0.02
    {0.23f,  0.0f},    // Decay   [0]: DSP=0.23
    {0.75f,  0.0f},    // Sustain [0]: direct=0.75
    {0.5f,   0.0f},    // Release [0]: DSP=0.5

    // LFO — LfoSpeed stored as DSP internal; loadProgram divides by 50
    0.05f,      // LfoSpeed (DSP internal: 0.05;  param = LfoSpeed/50 = 0.001)
    WAVE_SINE,  // LfoWave
    32768,      // LfoPw
    false,      // LfoTrigger

    // Voices — Volume stored as DSP internal; loadProgram divides by 5
    {
        {1.0f, 0,   0, OWAVE_SAW, 32768, false, false}, // Voice[0]: Vol DSP=1.0, C=0
        {1.0f, 12,  0, OWAVE_SAW, 32768, false, false}, // Voice[1]: C=+12
        {1.0f, -12, 0, OWAVE_SAW, 32768, false, false}, // Voice[2]: C=-12
        {0.0f,  0,  0, 0,          0,    false, false}, // Voice[3]: unused
    },

    // Modulations (all disabled) — Amount in DSP range -100..100; Multiplicator DSP 0..100
    {
        {MOD_SRC_NONE, MOD_DEST_MAINVOL, 0.0f, 1.0f},
        {MOD_SRC_NONE, MOD_DEST_MAINVOL, 0.0f, 1.0f},
        {MOD_SRC_NONE, MOD_DEST_MAINVOL, 0.0f, 1.0f},
        {MOD_SRC_NONE, MOD_DEST_MAINVOL, 0.0f, 1.0f},
    },

    0.0f, // EnvMod

#ifdef ENABLE_POLYPHONY
    16,      // MaxPolyphony (default: 16 = full polyphonic)
#endif
};

// ============================================================================
// Load Program
// ============================================================================

void CPresetManager::loadProgram(const SynthProgram& program) {
    // Global
    _triggerParamUpdate(pVolume,   clampf(program.Volume  / 5.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pPanning,  clampf(program.Panning, 0.0f, 1.0f));
    _triggerParamUpdate(pCoarse,   coarse2val(clampi(program.Coarse, -50, 50)));
    _triggerParamUpdate(pFine,     fine2val(clampi(program.Fine, -100, 100)));

    // Filter
    _triggerParamUpdate(pFilterType,
                        int2val(clampi(program.FilterType, 0, FTYPE_MAX), FTYPE_MAX));
    _triggerParamUpdate(pFilterMode,
                        int2val(clampi(program.FilterMode, 0, FMODE_MAX), FMODE_MAX));
    _triggerParamUpdate(pCutoff,    clampf(program.Cutoff, 0.0f, 1.0f));
    _triggerParamUpdate(pResonance, clampf(program.Resonance, 0.0f, 1.0f));

    // Portamento
    _triggerParamUpdate(pPortaMode,  bool2val(program.PortaMode));
    _triggerParamUpdate(pPortaSpeed, clampf(program.PortaSpeed / 5.0f, 0.0f, 1.0f));

    // Arpeggio
    // ArpMode is stored as -1..ARP_MAX; the parameter encodes (ArpMode+1) / (ARP_MAX+2)
    _triggerParamUpdate(pArpMode,
                        int2val(clampi(program.ArpMode + 1, 0, ARP_MAX + 1), ARP_MAX + 1));
    // ArpSpeed is stored in ms (0..500); parameter = 1 - ArpSpeed/500
    _triggerParamUpdate(pArpSpeed,
                        clampf(1.0f - static_cast<float>(program.ArpSpeed) / 500.0f, 0.0f, 1.0f));
#ifdef ENABLE_POLYPHONY
    _triggerParamUpdate(pArpPoly, bool2val(program.ArpPoly));
#endif

    // Oscillators
    const int oscParams[3][7] = {
        {pOsc1Coarse, pOsc1Fine, pOsc1Wave, pOsc1Pw, pOsc1Volume, pOsc1Ring, pOsc1Sync},
        {pOsc2Coarse, pOsc2Fine, pOsc2Wave, pOsc2Pw, pOsc2Volume, pOsc2Ring, pOsc2Sync},
        {pOsc3Coarse, pOsc3Fine, pOsc3Wave, pOsc3Pw, pOsc3Volume, pOsc3Ring, pOsc3Sync},
    };
    for (int i = 0; i < 3; i++) {
        _triggerParamUpdate(oscParams[i][0], coarse2val(clampi(program.Voice[i].Coarse, -50, 50)));
        _triggerParamUpdate(oscParams[i][1], fine2val(clampi(program.Voice[i].Fine, -100, 100)));
        _triggerParamUpdate(oscParams[i][2],
                            int2val(clampi(program.Voice[i].Wave, 0, OWAVE_MAX), OWAVE_MAX));
        _triggerParamUpdate(oscParams[i][3],
                            clampf(static_cast<float>(program.Voice[i].Pw) / 65536.0f, 0.0f, 1.0f));
        _triggerParamUpdate(oscParams[i][4],
                            clampf(program.Voice[i].Volume / 5.0f, 0.0f, 1.0f));
        _triggerParamUpdate(oscParams[i][5], bool2val(program.Voice[i].Ring));
        _triggerParamUpdate(oscParams[i][6], bool2val(program.Voice[i].Sync));
    }

    // Envelopes (parameters are stored as internal * 10, except Sustain which is direct)
    _triggerParamUpdate(pEnv1A, clampf(program.Attack[0]  / 10.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pEnv1H, clampf(program.Hold[0]    / 10.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pEnv1D, clampf(program.Decay[0]   / 10.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pEnv1S, clampf(program.Sustain[0], 0.0f, 1.0f));
    _triggerParamUpdate(pEnv1R, clampf(program.Release[0] / 10.0f, 0.0f, 1.0f));

    _triggerParamUpdate(pEnv2A, clampf(program.Attack[1]  / 10.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pEnv2H, clampf(program.Hold[1]    / 10.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pEnv2D, clampf(program.Decay[1]   / 10.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pEnv2S, clampf(program.Sustain[1], 0.0f, 1.0f));
    _triggerParamUpdate(pEnv2R, clampf(program.Release[1] / 10.0f, 0.0f, 1.0f));

    // LFO
    _triggerParamUpdate(pLfo1Speed, clampf(program.LfoSpeed / 50.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pLfo1Wave,
                        int2val(clampi(program.LfoWave, 0, WAVE_MAX), WAVE_MAX));
    _triggerParamUpdate(pLfo1Pw,
                        clampf(static_cast<float>(program.LfoPw) / 65536.0f, 0.0f, 1.0f));
    _triggerParamUpdate(pLfo1Trig, bool2val(program.LfoTrigger));

    // Modulations
    const int modSrcParams[4]    = {pMod1Src,    pMod2Src,    pMod3Src,    pMod4Src};
    const int modDestParams[4]   = {pMod1Dest,   pMod2Dest,   pMod3Dest,   pMod4Dest};
    const int modAmountParams[4] = {pMod1Amount, pMod2Amount, pMod3Amount, pMod4Amount};
    const int modMulParams[4]    = {pMod1Mul,    pMod2Mul,    pMod3Mul,    pMod4Mul};
    for (int i = 0; i < 4; i++) {
        _triggerParamUpdate(modSrcParams[i],
                            int2val(clampi(program.Modulations[i].Source, 0, MOD_SRC_MAX), MOD_SRC_MAX));
        _triggerParamUpdate(modDestParams[i],
                            int2val(clampi(program.Modulations[i].Destination, 0, MOD_DEST_MAX), MOD_DEST_MAX));
        // Amount: -100..100 stored as float; param = (Amount + 100) / 200
        _triggerParamUpdate(modAmountParams[i],
                            clampf((program.Modulations[i].Amount + 100.0f) / 200.0f, 0.0f, 1.0f));
        // Multiplicator: 0..100; param = Multiplicator / 100
        _triggerParamUpdate(modMulParams[i],
                            clampf(program.Modulations[i].Multiplicator / 100.0f, 0.0f, 1.0f));
    }

    // Filter Modulation (EnvMod): stored as -1..+1; param = (EnvMod + 1) / 2
    _triggerParamUpdate(pFilterMod,
                        clampf((program.EnvMod + 1.0f) / 2.0f, 0.0f, 1.0f));

#ifdef ENABLE_POLYPHONY
    _triggerParamUpdate(pMaxPolyphony, static_cast<float>(program.MaxPolyphony));
#endif
}

void CPresetManager::loadDefaultProgram() {
    loadProgram(DefaultProgram);
}

void CPresetManager::initFactoryPrograms() {
    // Add the built-in DefaultProgram into FactoryPrograms as the first factory preset ("Init Patch").
    // Additional factory presets can be added here in the future.
    FactoryPrograms.push_back(DefaultProgram);

#ifdef INCLUDE_FACTORY_PRESETS
    // Append presets from factory_presets.h (auto-generated by factory_patch_converter)
    for (int i = 0; i < kFactoryPresetCount; i++) {
        FactoryPrograms.push_back(kFactoryPresets[i]);
    }
#endif
}

String CPresetManager::getFactoryProgramName(uint32_t index) const {
    DISTRHO_SAFE_ASSERT_RETURN(index < FactoryPrograms.size(), String())
    return String(FactoryPrograms[index].Name);
}

void CPresetManager::loadFactoryProgram(uint32_t index) {
    DISTRHO_SAFE_ASSERT_RETURN(index < FactoryPrograms.size(), )
    loadProgram(FactoryPrograms[index]);
}

void CPresetManager::_triggerParamUpdate(uint32_t paramId, float newValue) {
    ui->setParameterValue(paramId, newValue);   // Tell the DSP to update parameter value
    ui->parameterChanged(paramId, newValue);    // Request UI refresh
}

// ============================================================================
// Parameter Snapshot
// ============================================================================

SynthProgram CPresetManager::captureCurrentParameters() const {
    SynthProgram snapshot;
    std::memset(&snapshot, 0, sizeof(SynthProgram));

    // Global
    snapshot.Volume  = ui->fKnobVolume->getValue() * 5.0f;
    snapshot.Panning = ui->fKnobPanning->getValue();
    snapshot.Coarse  = ui->_c_val2coarse(ui->fKnobCoarse->getValue());
    snapshot.Fine    = ui->_c_val2fine(ui->fKnobFine->getValue());

    // Filter
    snapshot.FilterType = ui->_pf2i(ui->fKnobFilterType->getValue(), FTYPE_MAX);
    snapshot.FilterMode = ui->_pf2i(ui->fKnobFilterMode->getValue(), FMODE_MAX);
    snapshot.Cutoff     = ui->fKnobCutoff->getValue();
    snapshot.Resonance  = ui->fKnobResonance->getValue();

    // Portamento
    snapshot.PortaMode  = ui->fBtnGlideState->isDown();
    snapshot.PortaSpeed = ui->fKnobGlideSpeed->getValue() * 5.0f;

    // Arpeggio
    snapshot.ArpMode  = ui->_pf2i(ui->fArpMode->getValue(), ARP_MAX + 1) - 1;
    snapshot.ArpSpeed = (int)((1.0f - ui->fArpSpeed->getValue()) * 500.0f + 0.5f);
#ifdef ENABLE_POLYPHONY
    snapshot.ArpPoly  = ui->fArpPoly;
#endif

    // Oscillators
    snapshot.Voice[0].Coarse = ui->_c_val2coarse(ui->fKnobOsc1Coarse->getValue());
    snapshot.Voice[0].Fine   = ui->_c_val2fine(ui->fKnobOsc1Fine->getValue());
    snapshot.Voice[0].Wave   = ui->_pf2i(ui->fKnobOsc1Waveform->getValue(), OWAVE_MAX);
    snapshot.Voice[0].Pw     = ui->_c_val2pw(ui->fKnobOsc1PulseWidth->getValue());
    snapshot.Voice[0].Volume = ui->fKnobOsc1Volume->getValue() * 5.0f;
    snapshot.Voice[0].Ring   = ui->fBtnOsc1Ring->isDown();
    snapshot.Voice[0].Sync   = ui->fBtnOsc1Sync->isDown();

    snapshot.Voice[1].Coarse = ui->_c_val2coarse(ui->fKnobOsc2Coarse->getValue());
    snapshot.Voice[1].Fine   = ui->_c_val2fine(ui->fKnobOsc2Fine->getValue());
    snapshot.Voice[1].Wave   = ui->_pf2i(ui->fKnobOsc2Waveform->getValue(), OWAVE_MAX);
    snapshot.Voice[1].Pw     = ui->_c_val2pw(ui->fKnobOsc2PulseWidth->getValue());
    snapshot.Voice[1].Volume = ui->fKnobOsc2Volume->getValue() * 5.0f;
    snapshot.Voice[1].Ring   = ui->fBtnOsc2Ring->isDown();
    snapshot.Voice[1].Sync   = ui->fBtnOsc2Sync->isDown();

    snapshot.Voice[2].Coarse = ui->_c_val2coarse(ui->fKnobOsc3Coarse->getValue());
    snapshot.Voice[2].Fine   = ui->_c_val2fine(ui->fKnobOsc3Fine->getValue());
    snapshot.Voice[2].Wave   = ui->_pf2i(ui->fKnobOsc3Waveform->getValue(), OWAVE_MAX);
    snapshot.Voice[2].Pw     = ui->_c_val2pw(ui->fKnobOsc3PulseWidth->getValue());
    snapshot.Voice[2].Volume = ui->fKnobOsc3Volume->getValue() * 5.0f;
    snapshot.Voice[2].Ring   = ui->fBtnOsc3Ring->isDown();
    snapshot.Voice[2].Sync   = ui->fBtnOsc3Sync->isDown();

    // Envelopes
    snapshot.Attack[0]  = ui->fAmpAttack->getValue()  * 10.0f;
    snapshot.Hold[0]    = ui->fAmpHold->getValue()    * 10.0f;
    snapshot.Decay[0]   = ui->fAmpDecay->getValue()   * 10.0f;
    snapshot.Sustain[0] = ui->fAmpSustain->getValue();
    snapshot.Release[0] = ui->fAmpRelease->getValue() * 10.0f;

    snapshot.Attack[1]  = ui->fModAttack->getValue()  * 10.0f;
    snapshot.Hold[1]    = ui->fModHold->getValue()    * 10.0f;
    snapshot.Decay[1]   = ui->fModDecay->getValue()   * 10.0f;
    snapshot.Sustain[1] = ui->fModSustain->getValue();
    snapshot.Release[1] = ui->fModRelease->getValue() * 10.0f;

    // LFO
    snapshot.LfoSpeed   = ui->fLfoSpeed->getValue() * 50.0f;
    snapshot.LfoWave    = ui->_pf2i(ui->fLfoWaveform->getValue(), WAVE_MAX);
    snapshot.LfoPw      = ui->_c_val2pw(ui->fLfoPulseWidth->getValue());
    snapshot.LfoTrigger = ui->fBtnLFOTrigger->isDown();

    // Modulations
    snapshot.Modulations[0].Source        = ui->_pf2i(ui->fMod1Source->getValue(), MOD_SRC_MAX);
    snapshot.Modulations[0].Destination   = ui->_pf2i(ui->fMod1Destination->getValue(), MOD_DEST_MAX);
    snapshot.Modulations[0].Amount        = (float)ui->_c_val2modAmount(ui->fMod1Amount->getValue());
    snapshot.Modulations[0].Multiplicator = (float)ui->_c_val2modMul(ui->fMod1Multiply->getValue());

    snapshot.Modulations[1].Source        = ui->_pf2i(ui->fMod2Source->getValue(), MOD_SRC_MAX);
    snapshot.Modulations[1].Destination   = ui->_pf2i(ui->fMod2Destination->getValue(), MOD_DEST_MAX);
    snapshot.Modulations[1].Amount        = (float)ui->_c_val2modAmount(ui->fMod2Amount->getValue());
    snapshot.Modulations[1].Multiplicator = (float)ui->_c_val2modMul(ui->fMod2Multiply->getValue());

    snapshot.Modulations[2].Source        = ui->_pf2i(ui->fMod3Source->getValue(), MOD_SRC_MAX);
    snapshot.Modulations[2].Destination   = ui->_pf2i(ui->fMod3Destination->getValue(), MOD_DEST_MAX);
    snapshot.Modulations[2].Amount        = (float)ui->_c_val2modAmount(ui->fMod3Amount->getValue());
    snapshot.Modulations[2].Multiplicator = (float)ui->_c_val2modMul(ui->fMod3Multiply->getValue());

    snapshot.Modulations[3].Source        = ui->_pf2i(ui->fMod4Source->getValue(), MOD_SRC_MAX);
    snapshot.Modulations[3].Destination   = ui->_pf2i(ui->fMod4Destination->getValue(), MOD_DEST_MAX);
    snapshot.Modulations[3].Amount        = (float)ui->_c_val2modAmount(ui->fMod4Amount->getValue());
    snapshot.Modulations[3].Multiplicator = (float)ui->_c_val2modMul(ui->fMod4Multiply->getValue());

    // Filter Mod (EnvMod)
    snapshot.EnvMod = (ui->fKnobFilterParameter->getValue() - 0.5f) * 2.0f;

#ifdef ENABLE_POLYPHONY
    snapshot.MaxPolyphony = ui->fMaxPolyphony;
#endif

    return snapshot;
}

// ============================================================================
// JSON Serialization
// ============================================================================

String CPresetManager::serializeBankToJSON(const PresetBank& bank) const {
    try {
        json j;
        j["formatVersion"] = "1.0.0";
        j["bankName"]      = bank.Name.buffer();
        j["presetCount"]   = bank.Presets.size();
        j["presets"]       = json::array();

        for (const auto& preset : bank.Presets) {
            json p;
            p["name"] = preset.Name;

            // Global
            p["volume"]  = preset.Volume;
            p["panning"] = preset.Panning;
            p["coarse"]  = preset.Coarse;
            p["fine"]    = preset.Fine;

            // Filter
            p["filterType"] = preset.FilterType;
            p["filterMode"] = preset.FilterMode;
            p["cutoff"]     = preset.Cutoff;
            p["resonance"]  = preset.Resonance;

            // Portamento
            p["portaMode"]  = preset.PortaMode;
            p["portaSpeed"] = preset.PortaSpeed;

            // Arpeggio
            p["arpMode"]  = preset.ArpMode;
            p["arpSpeed"] = preset.ArpSpeed;
#ifdef ENABLE_POLYPHONY
            p["arpPoly"]       = preset.ArpPoly;
            p["maxPolyphony"] = preset.MaxPolyphony;
#endif

            // Oscillators
            for (int i = 0; i < 3; i++) {
                char keyBuf[32];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dCoarse", i + 1); p[keyBuf] = preset.Voice[i].Coarse;
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dFine",   i + 1); p[keyBuf] = preset.Voice[i].Fine;
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dWave",   i + 1); p[keyBuf] = preset.Voice[i].Wave;
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dPw",     i + 1); p[keyBuf] = preset.Voice[i].Pw;
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dVolume", i + 1); p[keyBuf] = preset.Voice[i].Volume;
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dRing",   i + 1); p[keyBuf] = preset.Voice[i].Ring;
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dSync",   i + 1); p[keyBuf] = preset.Voice[i].Sync;
            }

            // Envelopes
            p["env1Attack"]  = preset.Attack[0];
            p["env1Hold"]    = preset.Hold[0];
            p["env1Decay"]   = preset.Decay[0];
            p["env1Sustain"] = preset.Sustain[0];
            p["env1Release"] = preset.Release[0];

            p["env2Attack"]  = preset.Attack[1];
            p["env2Hold"]    = preset.Hold[1];
            p["env2Decay"]   = preset.Decay[1];
            p["env2Sustain"] = preset.Sustain[1];
            p["env2Release"] = preset.Release[1];

            // LFO
            p["lfoSpeed"]   = preset.LfoSpeed;
            p["lfoWave"]    = preset.LfoWave;
            p["lfoPw"]      = preset.LfoPw;
            p["lfoTrigger"] = preset.LfoTrigger;

            // Modulations
            for (int i = 0; i < 4; i++) {
                char keyBuf[32];
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dSrc",    i + 1); p[keyBuf] = preset.Modulations[i].Source;
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dDest",   i + 1); p[keyBuf] = preset.Modulations[i].Destination;
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dAmount", i + 1); p[keyBuf] = preset.Modulations[i].Amount;
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dMul",    i + 1); p[keyBuf] = preset.Modulations[i].Multiplicator;
            }

            // Filter Mod (EnvMod)
            p["envMod"] = preset.EnvMod;

            j["presets"].push_back(p);
        }

        return String(j.dump(2).c_str());
    } catch (const std::exception& e) {
        d_stderr("serializeBankToJSON: Exception - %s", e.what());
        return String();
    }
}

bool CPresetManager::deserializeBankFromJSON(const String& jsonString,
                                              PresetBank& outBank) const {
    if (jsonString.isEmpty()) {
        d_stderr("deserializeBankFromJSON: Empty JSON string");
        return false;
    }
    try {
        json j = json::parse(jsonString.buffer());

        if (!j.contains("formatVersion")) {
            d_stderr("deserializeBankFromJSON: Missing formatVersion");
            return false;
        }

        if (!j.contains("bankName")) {
            d_stderr("deserializeBankFromJSON: Missing bankName");
            return false;
        }

        outBank.Name = String(j["bankName"].get<std::string>().c_str());
        outBank.Presets.clear();

        if (!j.contains("presets") || !j["presets"].is_array()) {
            d_stderr("deserializeBankFromJSON: Missing or invalid presets array");
            return false;
        }

        for (const auto& pj : j["presets"]) {
            SynthProgram preset(DefaultProgram); // Start with defaults in case some fields are missing

            if (pj.contains("name")) {
                std::string name = pj["name"];
                strncpy(preset.Name, name.c_str(), 63);
                preset.Name[63] = '\0';
            }

            // Global
            if (pj.contains("volume"))  preset.Volume  = pj["volume"];
            if (pj.contains("panning")) preset.Panning = pj["panning"];
            if (pj.contains("coarse"))  preset.Coarse  = pj["coarse"];
            if (pj.contains("fine"))    preset.Fine    = pj["fine"];

            // Filter
            if (pj.contains("filterType")) preset.FilterType = pj["filterType"];
            if (pj.contains("filterMode")) preset.FilterMode = pj["filterMode"];
            if (pj.contains("cutoff"))     preset.Cutoff     = pj["cutoff"];
            if (pj.contains("resonance"))  preset.Resonance  = pj["resonance"];

            // Portamento
            if (pj.contains("portaMode"))  preset.PortaMode  = pj["portaMode"];
            if (pj.contains("portaSpeed")) preset.PortaSpeed = pj["portaSpeed"];

            // Arpeggio
            if (pj.contains("arpMode"))  preset.ArpMode  = pj["arpMode"];
            if (pj.contains("arpSpeed")) preset.ArpSpeed = pj["arpSpeed"];
#ifdef ENABLE_POLYPHONY
            if (pj.contains("arpPoly"))      preset.ArpPoly      = pj["arpPoly"];
            if (pj.contains("maxPolyphony")) preset.MaxPolyphony = pj["maxPolyphony"];
#endif

            // Oscillators
            for (int i = 0; i < 3; i++) {
                char keyBuf[32];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dCoarse", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Coarse = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dFine", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Fine   = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dWave", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Wave   = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dPw", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Pw     = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dVolume", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Volume = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dRing", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Ring   = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "osc%dSync", i + 1);
                if (pj.contains(keyBuf)) preset.Voice[i].Sync   = pj[keyBuf];
            }

            // Envelopes
            if (pj.contains("env1Attack"))  preset.Attack[0]  = pj["env1Attack"];
            if (pj.contains("env1Hold"))    preset.Hold[0]    = pj["env1Hold"];
            if (pj.contains("env1Decay"))   preset.Decay[0]   = pj["env1Decay"];
            if (pj.contains("env1Sustain")) preset.Sustain[0] = pj["env1Sustain"];
            if (pj.contains("env1Release")) preset.Release[0] = pj["env1Release"];
            if (pj.contains("env2Attack"))  preset.Attack[1]  = pj["env2Attack"];
            if (pj.contains("env2Hold"))    preset.Hold[1]    = pj["env2Hold"];
            if (pj.contains("env2Decay"))   preset.Decay[1]   = pj["env2Decay"];
            if (pj.contains("env2Sustain")) preset.Sustain[1] = pj["env2Sustain"];
            if (pj.contains("env2Release")) preset.Release[1] = pj["env2Release"];

            // LFO
            if (pj.contains("lfoSpeed"))   preset.LfoSpeed   = pj["lfoSpeed"];
            if (pj.contains("lfoWave"))    preset.LfoWave    = pj["lfoWave"];
            if (pj.contains("lfoPw"))      preset.LfoPw      = pj["lfoPw"];
            if (pj.contains("lfoTrigger")) preset.LfoTrigger = pj["lfoTrigger"];

            // Modulations
            for (int i = 0; i < 4; i++) {
                char keyBuf[32];
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dSrc",    i + 1);
                if (pj.contains(keyBuf)) preset.Modulations[i].Source        = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dDest",   i + 1);
                if (pj.contains(keyBuf)) preset.Modulations[i].Destination   = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dAmount", i + 1);
                if (pj.contains(keyBuf)) preset.Modulations[i].Amount        = pj[keyBuf];
                std::snprintf(keyBuf, sizeof(keyBuf), "mod%dMul",    i + 1);
                if (pj.contains(keyBuf)) preset.Modulations[i].Multiplicator = pj[keyBuf];
            }

            // Filter Mod (EnvMod)
            if (pj.contains("envMod")) preset.EnvMod = pj["envMod"];

            outBank.Presets.push_back(preset);
        }

        return true;
    } catch (const std::exception& e) {
        d_stderr("deserializeBankFromJSON: Exception - %s", e.what());
        return false;
    }
}

String CPresetManager::serializePresetToJSON(const SynthProgram& preset) const {
    // Wrap single preset in a minimal "bank" and serialize
    PresetBank tempBank;
    tempBank.Name = String(preset.Name);
    tempBank.Presets.push_back(preset);
    return serializeBankToJSON(tempBank);
}

bool CPresetManager::deserializePresetFromJSON(const String& jsonString,
                                               SynthProgram& outPreset) const {
    PresetBank tempBank;
    if (!deserializeBankFromJSON(jsonString, tempBank))
        return false;
    if (tempBank.Presets.empty())
        return false;
    outPreset = tempBank.Presets[0];
    return true;
}

bool CPresetManager::exportCurrentPresetToFile(const char* filePath) {
    if (!filePath || filePath[0] == '\0') {
        d_stderr("exportCurrentPresetToFile: Invalid file path");
        return false;
    }

    SynthProgram preset = captureCurrentParameters();
    strncpy(preset.Name, ui->fCurrentPresetName.buffer(), 63);
    preset.Name[63] = '\0';

    String jsonContent = serializePresetToJSON(preset);
    if (jsonContent.isEmpty()) {
        d_stderr("exportCurrentPresetToFile: Failed to serialize preset");
        return false;
    }

    if (!_writeFileContent(String(filePath), jsonContent)) {
        d_stderr("exportCurrentPresetToFile: Failed to write file '%s'", filePath);
        return false;
    }

    d_stderr("Successfully exported preset to '%s'", filePath);
    return true;
}

bool CPresetManager::importPresetFromFile(const char* filePath, String* outPresetName) {
    if (!filePath || filePath[0] == '\0') {
        d_stderr("importPresetFromFile: Invalid file path");
        return false;
    }

    String jsonContent = _readFileContent(String(filePath));
    if (jsonContent.isEmpty()) {
        d_stderr("importPresetFromFile: Failed to read file '%s'", filePath);
        return false;
    }

    SynthProgram preset;
    if (!deserializePresetFromJSON(jsonContent, preset)) {
        d_stderr("importPresetFromFile: Failed to deserialize preset");
        return false;
    }

    loadProgram(preset);

    if (outPresetName)
        *outPresetName = String(preset.Name);

    d_stderr("Successfully imported preset '%s' from file", preset.Name);
    return true;
}

// ============================================================================
// File I/O Helper
// ============================================================================

String CPresetManager::_getUserPresetsDirectory() const {
#ifdef DISTRHO_OS_WINDOWS
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        String path(appData);
        path += "\\" + String(DISTRHO_PLUGIN_NAME);
        return path;
    }
#elif defined(DISTRHO_OS_MAC)
    const char* home = std::getenv("HOME");
    if (home) {
        String path(home);
        path += "/Library/Application Support/" + String(DISTRHO_PLUGIN_NAME);
        return path;
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        String path(home);
        path += "/.config/" + String(DISTRHO_PLUGIN_NAME);
        return path;
    }
#endif
    return String();
}

// ============================================================================
// Default User Bank Management
// ============================================================================

bool CPresetManager::loadDefaultBank() {
    String dirPath = _getUserPresetsDirectory();
    if (!_createDirectoryIfNeeded(dirPath)) {
        d_stderr("Failed to create user presets directory");
    }

    String path = _getDefaultBankPath();

    if (_fileExists(path)) {
        String jsonContent = _readFileContent(path);
        if (jsonContent.isNotEmpty()) {
            if (deserializeBankFromJSON(jsonContent, fDefaultUserBank)) {
                d_stderr("Loaded default user bank from: %s", path.buffer());
                return true;
            }
        }
    }

    d_stderr("Initializing empty default user bank");
    fDefaultUserBank.Name = String(DEFAULT_USER_BANK_NAME);
    fDefaultUserBank.Presets.clear();
    return saveDefaultBank();
}

bool CPresetManager::saveDefaultBank() {
    String path = _getDefaultBankPath();
    if (path.isEmpty()) {
        d_stderr("Failed to get user presets path");
        return false;
    }

    String dirPath = _getUserPresetsDirectory();
    if (!_createDirectoryIfNeeded(dirPath)) {
        d_stderr("Failed to create user presets directory");
        return false;
    }

    String jsonContent = serializeBankToJSON(fDefaultUserBank);
    if (jsonContent.isEmpty()) {
        d_stderr("Failed to serialize user bank to JSON");
        return false;
    }

    if (_writeFileContent(path, jsonContent)) {
        d_stderr("Saved default user bank to: %s", path.buffer());
        return true;
    }

    return false;
}

void CPresetManager::savePresetToDefaultBank(const char* presetName,
                                             const SynthProgram& preset) {
    bool found = false;
    for (auto& p : fDefaultUserBank.Presets) {
        if (std::strcmp(p.Name, presetName) == 0) {
            p = preset;
            std::strncpy(p.Name, presetName, 63);
            p.Name[63] = '\0';
            found = true;
            d_stderr("Updated preset '%s' in default user bank", presetName);
            break;
        }
    }

    if (!found) {
        SynthProgram newPreset = preset;
        std::strncpy(newPreset.Name, presetName, 63);
        newPreset.Name[63] = '\0';
        fDefaultUserBank.Presets.push_back(newPreset);
        d_stderr("Added preset '%s' to default user bank", presetName);
    }

    saveDefaultBank();
}

bool CPresetManager::loadPresetFromDefaultBank(const char* presetName,
                                               SynthProgram& outPreset) {
    for (const auto& p : fDefaultUserBank.Presets) {
        if (std::strcmp(p.Name, presetName) == 0) {
            outPreset = p;
            return true;
        }
    }
    d_stderr("Preset '%s' not found in default user bank", presetName);
    return false;
}

bool CPresetManager::deletePresetFromDefaultBank(const char* presetName) {
    for (auto it = fDefaultUserBank.Presets.begin();
         it != fDefaultUserBank.Presets.end(); ++it) {
        if (std::strcmp(it->Name, presetName) == 0) {
            fDefaultUserBank.Presets.erase(it);
            d_stderr("Deleted preset '%s' from default user bank", presetName);
            saveDefaultBank();
            return true;
        }
    }
    d_stderr("Preset '%s' not found in default user bank", presetName);
    return false;
}

bool CPresetManager::renamePresetInDefaultBank(const char* oldName, const char* newName) {
    for (const auto& p : fDefaultUserBank.Presets) {
        if (std::strcmp(p.Name, newName) == 0) {
            d_stderr("Preset with name '%s' already exists", newName);
            return false;
        }
    }
    for (auto& p : fDefaultUserBank.Presets) {
        if (std::strcmp(p.Name, oldName) == 0) {
            std::strncpy(p.Name, newName, 63);
            p.Name[63] = '\0';
            d_stderr("Renamed preset '%s' to '%s'", oldName, newName);
            saveDefaultBank();
            return true;
        }
    }
    d_stderr("Preset '%s' not found in default user bank", oldName);
    return false;
}

String CPresetManager::getDefaultBankName() const {
    return fDefaultUserBank.Name;
}

size_t CPresetManager::getDefaultBankPresetCount() const {
    return fDefaultUserBank.Presets.size();
}

String CPresetManager::getDefaultBankPresetName(size_t index) const {
    if (index < fDefaultUserBank.Presets.size())
        return String(fDefaultUserBank.Presets[index].Name);
    return String();
}

bool CPresetManager::loadPresetFromDefaultBank(const char* presetName) {
    SynthProgram preset;
    if (!loadPresetFromDefaultBank(presetName, preset))
        return false;
    loadProgram(preset);
    return true;
}

// ============================================================================
// Bank Management
// ============================================================================

std::vector<String> CPresetManager::getImportedBankNames() {
    std::vector<String> bankNames;

    String banksDir = _getBanksDirectory();
    if (banksDir.isEmpty())
        return bankNames;

#ifdef DISTRHO_OS_WINDOWS
    WIN32_FIND_DATAA findData;
    String searchPattern = banksDir + "\\*" USER_PRESET_BANK_EXTENSION;
    HANDLE hFind = FindFirstFileA(searchPattern.buffer(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                String fileName(findData.cFileName);
                if (fileName.endsWith(USER_PRESET_BANK_EXTENSION)) {
                    size_t nameLen = fileName.length() - strlen(USER_PRESET_BANK_EXTENSION);
                    char* buf = (char*)std::malloc(nameLen + 1);
                    std::strncpy(buf, fileName.buffer(), nameLen);
                    buf[nameLen] = '\0';
                    String bankName(buf);
                    std::free(buf);
                    if (!_isDefaultUserBank(bankName.buffer()) && !_isFactoryBank(bankName.buffer()))
                        bankNames.push_back(bankName);
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(banksDir.buffer());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            String fileName(entry->d_name);
            if (fileName == "." || fileName == "..")
                continue;
            if (fileName.endsWith(USER_PRESET_BANK_EXTENSION)) {
                size_t nameLen = fileName.length() - strlen(USER_PRESET_BANK_EXTENSION);
                char* buf = (char*)std::malloc(nameLen + 1);
                std::strncpy(buf, fileName.buffer(), nameLen);
                buf[nameLen] = '\0';
                String bankName(buf);
                std::free(buf);
                if (!_isDefaultUserBank(bankName.buffer()) && !_isFactoryBank(bankName.buffer()))
                    bankNames.push_back(bankName);
            }
        }
        closedir(dir);
    }
#endif

    return bankNames;
}

bool CPresetManager::importBankFromFile(const char* filePath) {
    if (!filePath || filePath[0] == '\0') {
        d_stderr("importBankFromFile: Invalid file path");
        return false;
    }

    String jsonContent = _readFileContent(String(filePath));
    if (jsonContent.isEmpty()) {
        d_stderr("importBankFromFile: Failed to read file '%s'", filePath);
        return false;
    }

    PresetBank importedBank;
    if (!deserializeBankFromJSON(jsonContent, importedBank)) {
        d_stderr("importBankFromFile: Failed to deserialize bank");
        return false;
    }

    if (importedBank.Name.isEmpty()) {
        d_stderr("importBankFromFile: Bank name is empty");
        return false;
    }

    _sanitizeBankName(importedBank.Name);

    // Reject reserved bank names
    if (_isDefaultUserBank(importedBank.Name.buffer()) || _isFactoryBank(importedBank.Name.buffer())) {
        d_stderr("importBankFromFile: Cannot import with reserved name '%s'", importedBank.Name.buffer());
        return false;
    }

    String banksDir = _getBanksDirectory();
    if (!_createDirectoryIfNeeded(banksDir))
        return false;

    String destPath = _getBankFilePath(importedBank.Name.buffer());
    return _saveBankToFile(destPath, importedBank);
}

bool CPresetManager::exportBankToFile(const char* bankName, const char* filePath) {
    if (!bankName || bankName[0] == '\0' || !filePath || filePath[0] == '\0') {
        d_stderr("exportBankToFile: Invalid parameters");
        return false;
    }

    String bankFilePath = _getBankFilePath(bankName);
    PresetBank bank;

    if (_isDefaultUserBank(bankName)) {
        bank = fDefaultUserBank;
    } else if (_isFactoryBank(bankName)) {
        d_stderr("exportBankToFile: Cannot export the Factory bank");
        return false;
    } else if (!_loadBankFromFile(bankFilePath, bank)) {
        d_stderr("exportBankToFile: Failed to load bank '%s'", bankName);
        return false;
    }

    String jsonContent = serializeBankToJSON(bank);
    if (jsonContent.isEmpty())
        return false;

    return _writeFileContent(String(filePath), jsonContent);
}

bool CPresetManager::deleteBankByName(const char* bankName) {
    if (!bankName || bankName[0] == '\0') {
        d_stderr("deleteBankByName: Invalid bank name");
        return false;
    }
    if (_isDefaultUserBank(bankName)) {
        d_stderr("deleteBankByName: Cannot delete the Default User Bank");
        return false;
    }
    if (_isFactoryBank(bankName)) {
        d_stderr("deleteBankByName: Cannot delete the Factory bank");
        return false;
    }

    String filePath = _getBankFilePath(bankName);
    if (!_fileExists(filePath)) {
        d_stderr("deleteBankByName: Bank file not found: %s", filePath.buffer());
        return false;
    }

#ifdef DISTRHO_OS_WINDOWS
    if (!DeleteFileA(filePath.buffer())) {
        d_stderr("deleteBankByName: Failed to delete file: %s", filePath.buffer());
        return false;
    }
#else
    if (remove(filePath.buffer()) != 0) {
        d_stderr("deleteBankByName: Failed to delete file: %s", filePath.buffer());
        return false;
    }
#endif

    d_stderr("Deleted bank '%s'", bankName);
    return true;
}

bool CPresetManager::renameBankByName(const char* oldName, const char* newName) {
    if (!oldName || oldName[0] == '\0' || !newName || newName[0] == '\0') {
        d_stderr("renameBankByName: Invalid parameters");
        return false;
    }
    if (_isDefaultUserBank(oldName)) {
        d_stderr("renameBankByName: Cannot rename the Default User Bank");
        return false;
    }
    if (_isFactoryBank(oldName)) {
        d_stderr("renameBankByName: Cannot rename the Factory bank");
        return false;
    }

    String oldFilePath = _getBankFilePath(oldName);
    if (!_fileExists(oldFilePath)) {
        d_stderr("renameBankByName: Source bank not found");
        return false;
    }

    String newBankName(newName);
    _sanitizeBankName(newBankName);

    String newFilePath = _getBankFilePath(newBankName.buffer());
    if (_fileExists(newFilePath)) {
        d_stderr("renameBankByName: Target bank already exists");
        return false;
    }

    // Load, rename the Name field, save to new path, delete old
    PresetBank bank;
    if (!_loadBankFromFile(oldFilePath, bank))
        return false;
    bank.Name = newBankName;

    if (!_saveBankToFile(newFilePath, bank))
        return false;

#ifdef DISTRHO_OS_WINDOWS
    DeleteFileA(oldFilePath.buffer());
#else
    remove(oldFilePath.buffer());
#endif

    d_stderr("Renamed bank '%s' to '%s'", oldName, newBankName.buffer());
    return true;
}

bool CPresetManager::loadPresetFromBank(const char* bankName, const char* presetName) {
    if (!bankName || bankName[0] == '\0' || !presetName || presetName[0] == '\0')
        return false;

    if (_isFactoryBank(bankName)) {
        for (uint32_t i = 0; i < FactoryPrograms.size(); i++) {
            if (std::strcmp(FactoryPrograms[i].Name, presetName) == 0) {
                loadFactoryProgram(i);
                return true;
            }
        }
        d_stderr("loadPresetFromBank: Factory preset '%s' not found", presetName);
        return false;
    }

    if (_isDefaultUserBank(bankName))
        return loadPresetFromDefaultBank(presetName);

    String filePath = _getBankFilePath(bankName);
    PresetBank bank;
    if (!_loadBankFromFile(filePath, bank))
        return false;

    for (const auto& p : bank.Presets) {
        if (std::strcmp(p.Name, presetName) == 0) {
            loadProgram(p);
            return true;
        }
    }

    d_stderr("loadPresetFromBank: Preset '%s' not found in bank '%s'", presetName, bankName);
    return false;
}

std::vector<String> CPresetManager::getPresetsInBank(const char* bankName) {
    std::vector<String> presetNames;

    if (!bankName || bankName[0] == '\0')
        return presetNames;

    if (_isFactoryBank(bankName)) {
        for (uint32_t i = 0; i < FactoryPrograms.size(); i++) {
            presetNames.push_back(String(FactoryPrograms[i].Name));
        }
        return presetNames;
    }

    if (_isDefaultUserBank(bankName)) {
        for (size_t i = 0; i < fDefaultUserBank.Presets.size(); i++)
            presetNames.push_back(String(fDefaultUserBank.Presets[i].Name));
        return presetNames;
    }

    String filePath = _getBankFilePath(bankName);
    PresetBank bank;
    if (_loadBankFromFile(filePath, bank)) {
        for (const auto& p : bank.Presets)
            presetNames.push_back(String(p.Name));
    }
    return presetNames;
}

bool CPresetManager::createNewBank(const char* bankName) {
    if (!bankName || bankName[0] == '\0') {
        d_stderr("createNewBank: Invalid bank name");
        return false;
    }
    if (_isDefaultUserBank(bankName)) {
        d_stderr("createNewBank: Cannot create a bank with the Default Bank name");
        return false;
    }
    if (_isFactoryBank(bankName)) {
        d_stderr("createNewBank: Cannot use the Factory bank name");
        return false;
    }

    String newBankName(bankName);
    _sanitizeBankName(newBankName);

    String banksDir = _getBanksDirectory();
    if (!_createDirectoryIfNeeded(banksDir))
        return false;

    String filePath = _getBankFilePath(newBankName.buffer());
    if (_fileExists(filePath)) {
        d_stderr("createNewBank: Bank '%s' already exists", newBankName.buffer());
        return false;
    }

    PresetBank newBank;
    newBank.Name = newBankName;
    return _saveBankToFile(filePath, newBank);
}

bool CPresetManager::savePresetToBank(const char* bankName, const char* presetName,
                                       const SynthProgram& preset) {
    if (!bankName || bankName[0] == '\0' || !presetName || presetName[0] == '\0')
        return false;

    if (_isDefaultUserBank(bankName)) {
        savePresetToDefaultBank(presetName, preset);
        return true;
    }

    if (_isFactoryBank(bankName)) {
        d_stderr("savePresetToBank: Cannot modify the Factory bank");
        return false;
    }

    String filePath = _getBankFilePath(bankName);
    PresetBank bank;

    if (_fileExists(filePath)) {
        if (!_loadBankFromFile(filePath, bank))
            return false;
    } else {
        bank.Name = String(bankName);
    }

    // Ensure bank name matches
    if (bank.Name != bankName)
        bank.Name = String(bankName);

    // Update or insert
    bool found = false;
    for (auto& p : bank.Presets) {
        if (std::strcmp(p.Name, presetName) == 0) {
            p = preset;
            std::strncpy(p.Name, presetName, 63);
            p.Name[63] = '\0';
            found = true;
            break;
        }
    }
    if (!found) {
        SynthProgram newPreset = preset;
        std::strncpy(newPreset.Name, presetName, 63);
        newPreset.Name[63] = '\0';
        bank.Presets.push_back(newPreset);
    }

    return _saveBankToFile(filePath, bank);
}

bool CPresetManager::deletePresetFromBank(const char* bankName, const char* presetName) {
    if (!bankName || bankName[0] == '\0' || !presetName || presetName[0] == '\0')
        return false;

    if (_isDefaultUserBank(bankName))
        return deletePresetFromDefaultBank(presetName);

    if (_isFactoryBank(bankName)) {
        d_stderr("deletePresetFromBank: Cannot modify the Factory bank");
        return false;
    }

    String filePath = _getBankFilePath(bankName);
    PresetBank bank;
    if (!_loadBankFromFile(filePath, bank))
        return false;

    if (bank.Name != bankName)
        bank.Name = String(bankName);

    bool found = false;
    for (auto it = bank.Presets.begin(); it != bank.Presets.end(); ++it) {

        if (std::strcmp(it->Name, presetName) == 0) {
            bank.Presets.erase(it);
            found = true;
            break;
        }
    }

    if (!found) {
        d_stderr("deletePresetFromBank: Preset '%s' not found in bank '%s'", presetName, bankName);
        return false;
    }

    return _saveBankToFile(filePath, bank);
}

bool CPresetManager::renamePresetInBank(const char* bankName, const char* oldName,
                                         const char* newName) {
    if (!bankName || bankName[0] == '\0' || !oldName || oldName[0] == '\0' || !newName || newName[0] == '\0')
        return false;

    if (_isDefaultUserBank(bankName))
        return renamePresetInDefaultBank(oldName, newName);

    if (_isFactoryBank(bankName)) {
        d_stderr("renamePresetInBank: Cannot modify the Factory bank");
        return false;
    }

    String filePath = _getBankFilePath(bankName);
    PresetBank bank;
    if (!_loadBankFromFile(filePath, bank))
        return false;

    if (bank.Name != bankName)
        bank.Name = String(bankName);

    for (const auto& p : bank.Presets) {
        if (std::strcmp(p.Name, newName) == 0) {
            d_stderr("renamePresetInBank: Preset '%s' already exists in bank", newName);
            return false;
        }
    }

    bool found = false;
    for (auto& p : bank.Presets) {
        if (std::strcmp(p.Name, oldName) == 0) {
            std::strncpy(p.Name, newName, 63);
            p.Name[63] = '\0';
            found = true;
            break;
        }
    }

    if (!found) {
        d_stderr("renamePresetInBank: Preset '%s' not found in bank '%s'", oldName, bankName);
        return false;
    }

    return _saveBankToFile(filePath, bank);
}
