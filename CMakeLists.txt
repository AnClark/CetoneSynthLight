cmake_minimum_required (VERSION 3.20)

project (cetone_synth_light
    VERSION 1.0.0
    LANGUAGES C CXX
)

# Options for Cetone build
option (CETONE_ENABLE_POLYPHONY "Enable polyphonic features of Cetone synthesizer" ON)
option (CETONE_INCLUDE_FACTORY_PRESETS "Include factory presets in the plugin binary (may increase binary size)" OFF)

# Explicitily specify C++ standard to prevent build error on Github CI - macOS build
set (CMAKE_CXX_STANDARD 17)

add_subdirectory (dpf)

include_directories (   
    src/
    src/3rdparty/
    dpf-widgets/opengl/
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

if (CETONE_ENABLE_POLYPHONY)
    message (STATUS "Cetone build: Polyphony mode")
    set (SRC_POLYPHONY
        src/CetoneSynthVoice.cpp
        src/CetoneSynthPolyphony.cpp
    )
else ()
    message (STATUS "Cetone build: Monophonic mode (original Cetone behavior)")
endif ()

dpf_add_plugin (${PROJECT_NAME}
    TARGETS vst2 vst3 lv2 clap jack
    FILES_DSP
        src/CetoneSynth.cpp
        src/CetoneSynthDPF.cpp
        src/CetoneSynthMain.cpp
        src/CetoneSynthPlugin.cpp
        ${SRC_POLYPHONY}
        ${SRC_BACKEND}
    FILES_UI
        src/CetoneUI.cpp
        src/CetoneUIHelper.cpp
        src/PresetManager.cpp
        src/PresetManagerHelper.cpp
        src/Images/CetoneArtwork.cpp
        src/Fonts/CetoneFonts.cpp
        dpf-widgets/opengl/DearImGui.cpp
        src/Widgets/ImGui_UI.cpp
        src/Widgets/ImGui_UI_MessageBox.cpp
        src/Widgets/ImGui_UI_FileBrowser.cpp
        src/Widgets/ImGui_UI_PresetManager.cpp
)

# NOTE: Do not set ENABLE_POLYPHONY globally, as it only affects plugin source files.
#       This can prevent unexpected rebuild of non-Cetone sources (especially DPF sources).
if (CETONE_ENABLE_POLYPHONY)
    target_compile_definitions (${PROJECT_NAME}-dsp PRIVATE ENABLE_POLYPHONY)
    target_compile_definitions (${PROJECT_NAME}-ui PRIVATE ENABLE_POLYPHONY)
endif ()

# Include factory presets converter utility if enabled
if (CETONE_INCLUDE_FACTORY_PRESETS)
    set (FACTORY_PRESET_DIR ${PROJECT_SOURCE_DIR}/presets/factory)
    set (FACTORY_PRESET_HEADER_FILE ${PROJECT_BINARY_DIR}/factory_presets.h)

    target_compile_definitions (${PROJECT_NAME}-ui PRIVATE INCLUDE_FACTORY_PRESETS)
    target_include_directories (${PROJECT_NAME}-ui PRIVATE ${PROJECT_BINARY_DIR})  # Include directory for generated header

    # Build utility to convert .clight preset files to C++ header with factory presets
    add_subdirectory (utils)

    # Generate factory presets header at build time
    add_custom_command (
        OUTPUT ${FACTORY_PRESET_HEADER_FILE}
        COMMAND factory_patch_converter ${FACTORY_PRESET_DIR} ${FACTORY_PRESET_HEADER_FILE}
        DEPENDS factory_patch_converter
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating factory presets header from .clight files..."
    )
    add_custom_target (generate_factory_presets ALL
        DEPENDS ${FACTORY_PRESET_HEADER_FILE}
    )
    add_dependencies (${PROJECT_NAME}-ui generate_factory_presets)
endif ()
