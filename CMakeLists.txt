cmake_minimum_required (VERSION 3.20)

project (cetone_synth_light
    VERSION 1.0.0
    LANGUAGES C CXX
)

add_subdirectory (dpf)

include_directories (   
    src/
)

set (SRC_BACKEND
    src/Filter303.cpp
    src/Filter8580.cpp
    src/FilterBiquad.cpp
    src/FilterCh12db.cpp
    src/FilterDirty.cpp
    src/FilterMoog.cpp
    src/FilterMoog2.cpp
    src/GlobalFunctions.cpp
    src/MidiStack.cpp
    src/SynthEnvelope.cpp
    src/SynthLfo.cpp
    src/SynthOscillator.cpp
)

dpf_add_plugin (${PROJECT_NAME}
    TARGETS vst2 vst3 lv2 clap jack
    FILES_DSP
        src/CetoneSynth.cpp
        src/CetoneSynthDPF.cpp
        src/CetoneSynthMain.cpp
        src/CetoneSynthPlugin.cpp
        ${SRC_BACKEND}
    FILES_UI
        src/CetoneUI.cpp
        src/CetoneUIHelper.cpp
        src/Images/CetoneArtwork.cpp
        src/Fonts/CetoneFonts.cpp
)

