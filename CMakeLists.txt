cmake_minimum_required(VERSION 3.15)
project(Lautset VERSION 0.1.0)

option(LAUTSET_ENABLE_LTO       "Build with LTO enabled"                OFF)
option(LAUTSET_ENABLE_WERROR    "Build with warnings as errors enabled" OFF)

# Caches build artifacts for faster builds
find_program(CCACHE ccache)
if (CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif ()

# Set C11 with no extensions enabled. Fail if the
# standard is not supported.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set C++14 with no extensions enabled. Fail if the
# standard is not supported.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(compiler_warnings)

add_subdirectory(3rd_party/JUCE EXCLUDE_FROM_ALL)

juce_add_gui_app(${PROJECT_NAME} PRODUCT_NAME "Lautset")

target_sources(${PROJECT_NAME}
    PRIVATE
        src/Main.cpp
        src/MainComponent.cpp
        src/MainComponent.hpp

        src/dsp/BufferWithSampleRate.cpp
        src/dsp/BufferWithSampleRate.hpp
        src/dsp/Duration.hpp
        src/dsp/LoudnessAnalysis.cpp
        src/dsp/LoudnessAnalysis.hpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src
        3rd_party/JUCE
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_VERSION>"
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_REPORT_APP_USAGE=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISABLE_JUCE_VERSION_PRINTING=1
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        lautset::compiler_warnings
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
        $<$<BOOL:${LAUTSET_ENABLE_LTO}>:juce::juce_recommended_lto_flags>
)
