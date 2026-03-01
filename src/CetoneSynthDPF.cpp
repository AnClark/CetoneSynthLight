#include "CetoneSynth.h"
#include "Structures.h"  // For STATE_PRESET_NAME etc.
#include "Defines.h"     // For DEFAULT_PRESET_NAME

void CCetoneSynth::initParameter(uint32_t index, Parameter& parameter)
{
    parameter.hints |= kParameterIsAutomatable;

#ifdef ENABLE_POLYPHONY
    // Special handling for pMaxPolyphony parameter (need correct range before getParameter call)
    if (index == pMaxPolyphony) {
        parameter.hints |= kParameterIsInteger;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 16.0f;
        parameter.ranges.def = 16.0f;
        parameter.unit = "voices";
    } else {
        // For Cetone normal parameters, fallback to classic VST 2.4 param range (0.0 ~ 1.0), to fit with Cetone's own param handlers.
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.ranges.def = getParameter(index);
    }
#else
    // For all Cetone parameters:
    // Fallback to classic VST 2.4 param range (0.0 ~ 1.0), to fit with Cetone's own param handlers.
    parameter.ranges.min = 0.0f;
    parameter.ranges.max = 1.0f;
    parameter.ranges.def = getParameter(index);
#endif

    // Must set parameter.symbol, this is the unique ID of each parameter.
    // If not set, you can neither save presets nor reset to factory default, in VST3 and CLAP!
    char buff[256];
    getParameterName(index, buff);
    parameter.symbol = String(buff).replace(' ', '_').replace('.', '_');
    parameter.name = String(buff);

    switch (index) {
    case pPortaMode:

    case pOsc1Sync:
    case pOsc2Sync:
    case pOsc3Sync:

    case pOsc1Ring:
    case pOsc2Ring:
    case pOsc3Ring:

    case pLfo1Trig:

#ifdef ENABLE_POLYPHONY
    case pArpPoly:
#endif
        parameter.hints |= kParameterIsBoolean;
        break;
    }
}

float CCetoneSynth::getParameterValue(uint32_t index) const
{
    return this->getParameter(index);
}

void CCetoneSynth::setParameterValue(uint32_t index, float value)
{
    this->setParameter(index, value);
}

void CCetoneSynth::activate()
{
    this->resume();
}

void CCetoneSynth::run(const float** inputs, float** outputs, uint32_t frames, const DISTRHO::MidiEvent* midiEvents, uint32_t midiEventCount)
{
    this->processEvents(midiEvents, midiEventCount);
    this->processReplacing((float**)inputs, outputs, frames);
}

void CCetoneSynth::sampleRateChanged(double newSampleRate)
{
    this->setSampleRate(newSampleRate);
}

void CCetoneSynth::bufferSizeChanged(int newBufferSize)
{
    this->setBlockSize(newBufferSize);
}

void CCetoneSynth::initState(uint32_t index, State& state)
{
    switch (index)
    {
    case 0:
        state.key = STATE_PRESET_NAME;
        state.defaultValue = DEFAULT_PRESET_NAME;
        this->PresetName = DEFAULT_PRESET_NAME;
        break;
    case 1:
        state.key = STATE_PRESET_MODIFIED;
        state.defaultValue = "false";
        this->PresetModified = false;
        break;
    case 2:
        state.key = STATE_PRESET_BANK;
        state.defaultValue = FACTORY_BANK_NAME;
        this->PresetBank = FACTORY_BANK_NAME;
        break;
    }

    state.hints = kStateIsHostWritable;
}

String CCetoneSynth::getState(const char* key) const
{
    static const String sTrue ("true");
    static const String sFalse("false");

    if (std::strcmp(key, STATE_PRESET_NAME) == 0)
        return PresetName;
    else if (std::strcmp(key, STATE_PRESET_MODIFIED) == 0)
        return PresetModified ? sTrue : sFalse;
    else if (std::strcmp(key, STATE_PRESET_BANK) == 0)
        return PresetBank;

    return String();
}

void CCetoneSynth::setState(const char* key, const char* value)
{
    const bool valueOnOff = (std::strcmp(value, "true") == 0);

    if (std::strcmp(key, STATE_PRESET_NAME) == 0)
        PresetName = value;
    else if (std::strcmp(key, STATE_PRESET_MODIFIED) == 0)
        PresetModified = valueOnOff;
    else if (std::strcmp(key, STATE_PRESET_BANK) == 0)
        PresetBank = value;
}
