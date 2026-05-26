# vi:set sw=2 et:

# surge_add_effect(<short> <DEFINE_SUFFIX> "<Display Name>" "<lv2-uri-suffix>" "<4char-code>")
#
# <DEFINE_SUFFIX> becomes SURGE_FX_IS_<DEFINE_SUFFIX>=1 — must match a branch in
# SingleEffectProcessor.h. Supported values: FLANGER PHASER REVERB1 DELAY ROTARY
#
# To add a new effect: implement the branch in SingleEffectProcessor.h, then add one line here.

function(surge_add_effect short_name define_suffix display_name lv2_suffix plugin_code)
  set(target surge-fx-${short_name})

  juce_add_plugin(${target}
    PRODUCT_NAME        "Surge XT ${display_name}"
    COMPANY_NAME        "Surge Synth Team"
    COMPANY_WEBSITE     "https://surge-synthesizer.github.io/"
    BUNDLE_ID           "org.surge-synth-team.${lv2_suffix}"
    PLUGIN_MANUFACTURER_CODE VmbA
    PLUGIN_CODE         ${plugin_code}

    IS_SYNTH            FALSE
    NEEDS_MIDI_INPUT    FALSE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE

    VST3_CATEGORIES     Fx
    VST3_AUTO_MANIFEST  FALSE

    LV2_URI             https://surge-synthesizer.github.io/plugins/${lv2_suffix}
    LV2_SHARED_LIBRARY_NAME SurgeXT_${display_name}

    FORMATS             ${PLUGIN_FORMATS}
  )

  target_sources(${target} PRIVATE
    SingleEffectProcessor.cpp
    SingleEffectProcessor.h
  )

  target_compile_definitions(${target} PRIVATE
    SURGE_FX_IS_${define_suffix}=1
    SURGE_FX_DISPLAY_NAME="${display_name}"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
  )

  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(${target} PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    sst-effects
    sst-basic-blocks
    sst-filters
  )

  if(UNIX AND NOT APPLE)
    target_link_options(${target}_LV2 PRIVATE -Wl,--exclude-libs,ALL)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Effects — one line each.
# sst-effects has: Flanger, Phaser, Reverb1, Delay, RotarySpeaker
# Not yet available: Ensemble (still in Surge only), Chorus (stub in sst-effects)
# ---------------------------------------------------------------------------

surge_add_effect(flanger  FLANGER  "Flanger"        "surge-xt-flanger"  "S2FL")
surge_add_effect(phaser   PHASER   "Phaser"         "surge-xt-phaser"   "S2PH")
surge_add_effect(reverb1  REVERB1  "Reverb 1"       "surge-xt-reverb1"  "S2R1")
surge_add_effect(delay    DELAY    "Delay"           "surge-xt-delay"    "S2DL")
surge_add_effect(rotary   ROTARY   "Rotary Speaker"  "surge-xt-rotary"   "S2RT")
