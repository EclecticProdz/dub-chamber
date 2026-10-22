cmake_minimum_required(VERSION 3.22)
project(DubChamber VERSION 0.3.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DubChamber

    COMPANY_NAME "Roots Audio Lab"
    BUNDLE_ID "com.rootsaudiolab.dubchamber"
    PLUGIN_MANUFACTURER_CODE RtAL
    PLUGIN_CODE DbCh
    FORMATS VST3 AU
    PRODUCT_NAME "Dub Chamber"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(DubChamber PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)
target_compile_features(DubChamber PUBLIC cxx_std_17)
target_compile_definitions(DubChamber PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(DubChamber PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
juce_generate_juce_header(DubChamber)
