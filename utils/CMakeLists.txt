# ---------------------------------------------------------------------------
# Utility: factory_patch_converter
#   Reads .clight preset files from a directory and emits a C++ header with
#   a static const SynthProgram array of factory presets.
#
# Build:  cmake --build build --target factory_patch_converter
# Usage:  factory_patch_converter <preset_dir> <output_header>
# ---------------------------------------------------------------------------
add_executable (factory_patch_converter
    ./factory_patch_converter.cpp
)

target_include_directories (factory_patch_converter PRIVATE
    ${PROJECT_SOURCE_DIR}/src/
    ${PROJECT_SOURCE_DIR}/src/3rdparty/
)

set_target_properties (factory_patch_converter PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Statically link on Windows and Win32 cross-build
# This prevents "libgcc/libstdc++ not found" error on most conditions
if (WIN32 AND NOT MSVC)
    target_link_options (factory_patch_converter PRIVATE -static)
endif ()

if (CETONE_ENABLE_POLYPHONY)
    target_compile_definitions (factory_patch_converter PRIVATE ENABLE_POLYPHONY)
endif ()
