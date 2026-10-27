cmake_minimum_required(VERSION 3.22)
project(WavejackersHitGen VERSION 2.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
  if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary" FORCE)
  endif()
endif()
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # statik CRT: kullanıcıda ekstra kurulum gerekmez
endif()

# JUCE: -DHITGEN_JUCE_DIR=<yol> verilirse oradan, yoksa GitHub'dan 8.0.10 indirilir
set(HITGEN_JUCE_DIR "" CACHE PATH "Local JUCE checkout")
if(HITGEN_JUCE_DIR)
  add_subdirectory(${HITGEN_JUCE_DIR} JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

option(HITGEN_BUILD_TESTS "Build test programs" ON)
set(HITGEN_FORMATS VST3 Standalone CACHE STRING "Plugin formats")
if(APPLE)
  set(HITGEN_FORMATS VST3 AU Standalone CACHE STRING "Plugin formats" FORCE)
endif()

juce_add_plugin(WavejackersHitGen
  COMPANY_NAME "Wavejackers"
  BUNDLE_ID "com.wavejackers.hitgen"
  PLUGIN_MANUFACTURER_CODE Wjck
  PLUGIN_CODE Hgen
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  VST3_CATEGORIES Instrument Synth Generator
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  FORMATS ${HITGEN_FORMATS}
  PRODUCT_NAME "Wavejackers HitGen"
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(HitGenData SOURCES
  Resources/hitgen-data.json
  Resources/fonts/NotoSans-Regular.ttf
  Resources/fonts/NotoSans-Bold.ttf
  Resources/fonts/NotoSansArabic-Regular.ttf
  Resources/fonts/NotoSansArabic-Bold.ttf
  Resources/fonts/NotoSansDevanagari-Regular.ttf
  Resources/fonts/NotoSansDevanagari-Bold.ttf)
set_target_properties(HitGenData PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(HITGEN_CORE_SOURCES Source/Data.cpp Source/Generator.cpp)
set(HITGEN_DSP_SOURCES Source/Synth.cpp)

target_sources(WavejackersHitGen PRIVATE
  ${HITGEN_CORE_SOURCES} ${HITGEN_DSP_SOURCES}
  Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Ui.cpp Source/SoundParams.cpp Source/Presets.cpp Source/SoundPage.cpp Source/PresetPage.cpp)

set(HITGEN_DEFS
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0
  JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0
  JUCE_ALSA=0 JUCE_JACK=0)
if(APPLE OR WIN32)
  list(REMOVE_ITEM HITGEN_DEFS JUCE_ALSA=0 JUCE_JACK=0)
endif()
target_compile_definitions(WavejackersHitGen PUBLIC ${HITGEN_DEFS})
target_link_libraries(WavejackersHitGen
  PRIVATE HitGenData juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(HITGEN_BUILD_TESTS)
  # Üretici testi: web motoruyla birebir aynı MIDI
  juce_add_console_app(HitGenGenTest PRODUCT_NAME "HitGenGenTest")
  target_sources(HitGenGenTest PRIVATE tests/GenTest.cpp ${HITGEN_CORE_SOURCES})
  target_compile_definitions(HitGenGenTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(HitGenGenTest PRIVATE HitGenData juce::juce_core PUBLIC juce::juce_recommended_config_flags)

  # Kararlılık testi: işlemciyi doğrudan çalıştırır (rastgele parametre, durum kaydet/yükle, NaN, CPU ölçümü)
  juce_add_console_app(HitGenStressTest PRODUCT_NAME "HitGenStressTest")
  target_sources(HitGenStressTest PRIVATE tests/StressTest.cpp ${HITGEN_CORE_SOURCES} ${HITGEN_DSP_SOURCES}
    Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Ui.cpp Source/SoundParams.cpp Source/Presets.cpp Source/SoundPage.cpp Source/PresetPage.cpp)
  target_compile_definitions(HitGenStressTest PRIVATE ${HITGEN_DEFS} JucePlugin_Build_Standalone=0 HITGEN_TEST_BUILD=1)
  target_link_libraries(HitGenStressTest PRIVATE HitGenData juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
endif()
