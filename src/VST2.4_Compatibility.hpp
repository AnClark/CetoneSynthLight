#pragma once

#include <cstdint>

// VST 2.4 compatibility stuff


typedef int32_t VstInt32;

constexpr auto kVstMaxEffectNameLen = 32;
constexpr auto kVstMaxVendorStrLen = 64;
constexpr auto kVstMaxProductStrLen = 32;
constexpr auto kVstMaxProgNameLen = 24;
constexpr auto kVstMaxParamStrLen = 24;

// define the midi stuff ourselves
typedef struct _VstMidiEvent {
    int32_t type;
    int32_t byteSize;
    int32_t deltaFrames;
    int32_t _ignore1[3];
    char midiData[4];
    char _ignore2[4];
} VstMidiEvent;

typedef union _VstEvent {
    int32_t type;
    VstMidiEvent midi; // type 1
} VstEvents;

#define vst_strncpy(x, y, z) std::strncpy(x, y, z)
#define vst_strncat(x, y, z) std::strncat(x, y, z)