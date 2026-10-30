cmake_minimum_required(VERSION 3.22)

# Laufzeitbibliothek statisch einbinden: das Plugin läuft damit ohne installierte VC++-Redistributables.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(CSSynth1506 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(CSSYNTH_BUILD_TESTS "Offline-DSP-Test (render_test) mitbauen" OFF)
option(CSSYNTH_COPY_PLUGIN "VST3 nach dem Bauen automatisch nach C:/Program Files/Common Files/VST3 kopieren" OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CSSynth1506
    COMPANY_NAME                "CSSynth"
    PRODUCT_NAME                "CSSynth 1506"
    BUNDLE_ID                   "com.cssynth.model1506"
    PLUGIN_MANUFACTURER_CODE    Csyn
    PLUGIN_CODE                 C506
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Instrument Synth
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${CSSYNTH_COPY_PLUGIN})

target_sources(CSSynth1506 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Dsp/Engine.cpp)

target_compile_definitions(CSSynth1506 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(CSSynth1506 PRIVATE /utf-8)
endif()

target_link_libraries(CSSynth1506
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(CSSYNTH_BUILD_TESTS)
    add_executable(render_test tests/render_test.cpp Source/Dsp/Engine.cpp)
    target_include_directories(render_test PRIVATE Source)
    if(MSVC)
        target_compile_options(render_test PRIVATE /utf-8)
    endif()
endif()
