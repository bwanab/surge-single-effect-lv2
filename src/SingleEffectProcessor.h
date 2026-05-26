#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <sst/effects/ConcreteConfig.h>

// ---------------------------------------------------------------------------
// Compile-time effect selection via SURGE_FX_IS_<NAME>=1
// ---------------------------------------------------------------------------
#if defined(SURGE_FX_IS_FLANGER)
#include <sst/effects/Flanger.h>
using SurgeFXType = sst::effects::flanger::Flanger<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_PHASER)
#include <sst/effects/Phaser.h>
using SurgeFXType = sst::effects::phaser::Phaser<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_REVERB1)
#include <sst/effects/Reverb1.h>
using SurgeFXType = sst::effects::reverb1::Reverb1<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_DELAY)
#include <sst/effects/Delay.h>
using SurgeFXType = sst::effects::delay::Delay<sst::effects::core::ConcreteConfig>;

#elif defined(SURGE_FX_IS_ROTARY)
#include <sst/effects/RotarySpeaker.h>
using SurgeFXType = sst::effects::rotaryspeaker::RotarySpeaker<sst::effects::core::ConcreteConfig>;

#else
#error "No effect selected. Define SURGE_FX_IS_<NAME>=1 (FLANGER, PHASER, REVERB1, DELAY, ROTARY)"
#endif

// ---------------------------------------------------------------------------

class SingleEffectProcessor : public juce::AudioProcessor
{
public:
    SingleEffectProcessor();
    ~SingleEffectProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return SURGE_FX_DISPLAY_NAME; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& data) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

private:
    using Config = sst::effects::core::ConcreteConfig;
    static constexpr int blockSize = Config::blockSize;
    static constexpr int maxParams = Config::BC::maxParamCount;

    Config::GS gs;
    Config::ES es;
    Config::BC bc;
    std::unique_ptr<SurgeFXType> effect;

    int numFxParams{0};
    std::array<juce::AudioParameterFloat*, maxParams> fxParams{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingleEffectProcessor)
};
