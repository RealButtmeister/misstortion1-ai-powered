cmake_minimum_required(VERSION 3.22)

project(MisstortionAIPowered VERSION 1.3.1)

set(JUCE_DIR "$ENV{USERPROFILE}/Desktop/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

juce_add_plugin(Misstortion
    COMPANY_NAME "Nimble Tools"
    BUNDLE_ID "tools.nimble.MisstortionAIPowered"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Orqg
    FORMATS VST3 Standalone
    PRODUCT_NAME "Misstortion AI Powered")

juce_generate_juce_header(Misstortion)

juce_add_binary_data(MisstortionAssets
    SOURCES
        Misstortion.png)

target_sources(Misstortion
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginLookAndFeel.cpp
        Source/PluginProcessor.cpp)

target_compile_features(Misstortion PRIVATE cxx_std_17)

target_compile_definitions(Misstortion
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Misstortion
    PRIVATE
        MisstortionAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
