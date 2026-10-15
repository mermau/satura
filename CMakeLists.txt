cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) su macOS
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(SATURA VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: usa una copia locale se indicata (-DJUCE_DIR=...), altrimenti la scarica
set(JUCE_DIR "" CACHE PATH "Cartella di JUCE (opzionale)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# preset di fabbrica (stesso formato JSON della versione Max for Live)
juce_add_binary_data(SaturaAssets SOURCES
    resources/factory_presets.json
    resources/section_presets.json)

juce_add_plugin(Satura
    COMPANY_NAME "Collateral Effects"
    BUNDLE_ID com.collateraleffects.satura
    PLUGIN_MANUFACTURER_CODE Clfx
    PLUGIN_CODE Str2
    PRODUCT_NAME "SATURA"
    ICON_BIG resources/icon.png
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE                    # sorgenti MIDI, MIDI Learn, Program Change
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicEffect  # come Saturn 2: "AU MIDI-controlled Effects" in Logic
    VST3_CATEGORIES Fx Distortion
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "SATURA usa l'ingresso della scheda audio per elaborare il segnale."
    COPY_PLUGIN_AFTER_BUILD TRUE)

set(SATURA_SOURCES
    Source/dsp/Engine.cpp
    Source/Model.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/View.cpp
    Source/ViewDraw.cpp)

target_sources(Satura PRIVATE ${SATURA_SOURCES})
target_include_directories(Satura PRIVATE Source)
target_compile_definitions(Satura PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Satura
    PRIVATE SaturaAssets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Test del motore DSP (senza JUCE)
option(SATURA_BUILD_TESTS "Compila i test del motore" ON)
if(SATURA_BUILD_TESTS)
    foreach(t engine_test os_test lp_test)
        add_executable(${t} tests/${t}.cpp $<$<STREQUAL:${t},engine_test>:Source/dsp/Engine.cpp>)
        target_include_directories(${t} PRIVATE Source)
    endforeach()
endif()

# Strumenti di sviluppo: screenshot dell'interfaccia e test del processor (Linux/macOS)
option(SATURA_BUILD_DEVTOOLS "Compila gli strumenti di sviluppo" OFF)
if(SATURA_BUILD_DEVTOOLS)
    foreach(t gui_snapshot processor_test)
        juce_add_console_app(${t} PRODUCT_NAME "${t}")
        target_sources(${t} PRIVATE tests/${t}.cpp ${SATURA_SOURCES})
        target_include_directories(${t} PRIVATE Source)
        target_compile_definitions(${t} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
            JucePlugin_Name="SATURA" JUCE_STANDALONE_APPLICATION=1)
        target_link_libraries(${t} PRIVATE SaturaAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
    endforeach()
endif()
