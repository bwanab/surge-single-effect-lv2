#include "SingleEffectProcessor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

// ParamMetaData stores internal (DSP) values; the display scale and coefficients
// encode how to convert to/from user-facing units (Hz, ms, %, dB, etc.).
//
// A_TWO_TO_THE_B:  display = svA * 2^(svB * internal + svC) + svD
//                  (used by envelope time, LFO rate, audible frequency)
// LINEAR:          display = svA * internal + svB
//                  (used by percent, decibels — dB types have svA=1, svB=0 so no-op)
// All other scales have display == internal for our purposes.

using PMD = sst::basic_blocks::params::ParamMetaData;

static float internalToDisplay(const PMD &p, float v)
{
    float result;
    switch (p.displayScale)
    {
    case PMD::A_TWO_TO_THE_B:
        result = p.svA * powf(2.f, p.svB * v + p.svC) + p.svD;
        // Envelope-time params report in seconds; convert to ms for MODEP display.
        if (p.unit == "s")
            result *= 1000.f;
        return result;
    case PMD::LINEAR:
        return p.svA * v + p.svB;
    default:
        return v;
    }
}

static float displayToInternal(const PMD &p, float v)
{
    switch (p.displayScale)
    {
    case PMD::A_TWO_TO_THE_B:
        // Undo ms→s scaling before inverting the log2 formula.
        if (p.unit == "s")
            v /= 1000.f;
        // invert: internal = (log2((v - svD) / svA) - svC) / svB
        return (log2f((v - p.svD) / p.svA) - p.svC) / p.svB;
    case PMD::LINEAR:
        return (p.svA != 0.f) ? (v - p.svB) / p.svA : v;
    default:
        return v;
    }
}

// Unit label for JUCE parameter: envelope-time params are exposed in ms, not s.
static juce::String displayUnit(const PMD &p)
{
    if (p.displayScale == PMD::A_TWO_TO_THE_B && p.unit == "s")
        return "ms";
    return juce::String(p.unit);
}

// Surge's init_default_values() often differs from sst-effects paramAt() defaults.
// These tables supply the correct internal (DSP-unit) defaults per effect, sourced
// from the corresponding Surge effect's init_default_values() implementation.
// Any index not listed falls back to paramAt().defaultVal.
static float surgeDefaultFor(int idx, float paramAtDefault)
{
#if defined(SURGE_FX_IS_DELAY)
    // DelayEffect::init_default_values() in surge/src/common/dsp/effects/DelayEffect.cpp
    switch (idx)
    {
    case 0: return -2.f;   // dly_time_left  → 250 ms
    case 1: return -2.f;   // dly_time_right → 250 ms
    case 2: return 0.5f;   // dly_feedback   → 50 %
    case 4: return -24.f;  // dly_lowcut     → 110 Hz
    case 5: return 30.f;   // dly_highcut    → 2489 Hz
    case 6: return -2.f;   // dly_mod_rate   → 0.25 Hz
    default: break;
    }
#elif defined(SURGE_FX_IS_ROTARY)
    // RotarySpeakerEffect::init_default_values() in RotarySpeakerEffect.cpp
    switch (idx)
    {
    case 3: return 0.7f;   // rot_rotor_rate → 70 %
    case 6: return 1.0f;   // rot_width      → 1 dB
    case 7: return 1.0f;   // rot_mix        → 100 %
    default: break;
    }
#endif
    return paramAtDefault;
}

SingleEffectProcessor::SingleEffectProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      gs(44100.0), effect(std::make_unique<SurgeFXType>(&gs, &es, nullptr))
{
    effect->initialize();

    numFxParams = SurgeFXType::numParams;

    for (int i = 0; i < numFxParams; ++i)
    {
        auto pmd = effect->paramAt(i);

        // Expose parameters in user-facing (display) units so that MODEP shows
        // musically meaningful ranges (Hz, ms, %) rather than raw internal values.
        // processBlock converts back to internal units before writing paramStorage.
        float internalDef = surgeDefaultFor(i, pmd.defaultVal);
        float dispMin = internalToDisplay(pmd, pmd.minVal);
        float dispMax = internalToDisplay(pmd, pmd.maxVal);
        float dispDef = internalToDisplay(pmd, internalDef);

        auto *p = new juce::AudioParameterFloat(
            juce::ParameterID(pmd.name, 1), pmd.name,
            juce::NormalisableRange<float>(dispMin, dispMax), dispDef,
            juce::AudioParameterFloatAttributes().withLabel(displayUnit(pmd)));
        addParameter(p);
        fxParams[i] = p;
        effect->paramStorage[i] = internalDef;
    }

#if defined(SURGE_FX_IS_DELAY)
    {
        // "Time" is a master parameter for the GUI knob. It is not a DSP param
        // itself; processBlock mirrors its value to both Left (index 0) and
        // Right (index 1) whenever it changes.
        auto pmd = effect->paramAt(0); // same range/unit as Left
        float dispMin = internalToDisplay(pmd, pmd.minVal);
        float dispMax = internalToDisplay(pmd, pmd.maxVal);
        float dispDef = internalToDisplay(pmd, surgeDefaultFor(0, pmd.defaultVal));
        masterTimeParam = new juce::AudioParameterFloat(
            juce::ParameterID("Time", 1), "Time",
            juce::NormalisableRange<float>(dispMin, dispMax), dispDef,
            juce::AudioParameterFloatAttributes().withLabel(displayUnit(pmd)));
        addParameter(masterTimeParam);
    }
#endif
}

void SingleEffectProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    gs.sampleRate = sampleRate;
    effect->onSampleRateChanged();
}

bool SingleEffectProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    auto in = layouts.getMainInputChannelSet();
    return out == juce::AudioChannelSet::stereo() &&
           (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void SingleEffectProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
#if defined(SURGE_FX_IS_DELAY)
    // When the GUI "Time" master knob changes, mirror its value to both Left
    // and Right so the settings page stays in sync. Left and Right can still
    // be changed independently from settings; the next GUI move resyncs both.
    if (masterTimeParam != nullptr)
    {
        float master = *masterTimeParam;
        if (master != prevMasterTime)
        {
            *fxParams[0] = master;
            *fxParams[1] = master;
            prevMasterTime = master;
        }
    }
#endif

    // Convert from display units back to internal DSP units before each block.
    for (int i = 0; i < numFxParams; ++i)
    {
        auto pmd = effect->paramAt(i);
        effect->paramStorage[i] = displayToInternal(pmd, *fxParams[i]);
    }

    const int totalSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto *L = buffer.getWritePointer(0);
    auto *R = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    // Process in fixed-size blocks of Config::blockSize (16 samples).
    float tmpL[blockSize], tmpR[blockSize];

    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        const int count = std::min(blockSize, totalSamples - pos);

        // Copy input into temp buffers; zero-pad the last (short) chunk.
        std::copy(L + pos, L + pos + count, tmpL);
        if (count < blockSize)
            std::fill(tmpL + count, tmpL + blockSize, 0.f);

        if (R != nullptr)
        {
            std::copy(R + pos, R + pos + count, tmpR);
            if (count < blockSize)
                std::fill(tmpR + count, tmpR + blockSize, 0.f);
        }
        else
        {
            std::copy(tmpL, tmpL + blockSize, tmpR); // mono → dual-mono
        }

        effect->processBlock(tmpL, tmpR);

        std::copy(tmpL, tmpL + count, L + pos);
        if (R != nullptr)
            std::copy(tmpR, tmpR + count, R + pos);
    }
}

void SingleEffectProcessor::getStateInformation(juce::MemoryBlock &data)
{
    juce::XmlElement xml("State");
    for (int i = 0; i < numFxParams; ++i)
        xml.setAttribute("p" + juce::String(i), (double)*fxParams[i]);
    if (masterTimeParam != nullptr)
        xml.setAttribute("pTime", (double)*masterTimeParam);
    copyXmlToBinary(xml, data);
}

void SingleEffectProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml || xml->getTagName() != "State")
        return;
    for (int i = 0; i < numFxParams; ++i)
    {
        auto pmd = effect->paramAt(i);
        auto key = "p" + juce::String(i);
        if (xml->hasAttribute(key))
        {
            float dispMin = internalToDisplay(pmd, pmd.minVal);
            float dispMax = internalToDisplay(pmd, pmd.maxVal);
            float dispDef = internalToDisplay(pmd, pmd.defaultVal);
            float val = (float)xml->getDoubleAttribute(key, dispDef);
            *fxParams[i] = std::clamp(val, dispMin, dispMax);
        }
    }
    if (masterTimeParam != nullptr && xml->hasAttribute("pTime"))
    {
        auto pmd = effect->paramAt(0);
        float dispMin = internalToDisplay(pmd, pmd.minVal);
        float dispMax = internalToDisplay(pmd, pmd.maxVal);
        float dispDef = internalToDisplay(pmd, surgeDefaultFor(0, pmd.defaultVal));
        float val = (float)xml->getDoubleAttribute("pTime", dispDef);
        *masterTimeParam = std::clamp(val, dispMin, dispMax);
    }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SingleEffectProcessor(); }
