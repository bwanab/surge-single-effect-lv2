#include "SingleEffectProcessor.h"
#include <algorithm>
#include <cstring>

SingleEffectProcessor::SingleEffectProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , gs(44100.0)
    , effect(std::make_unique<SurgeFXType>(&gs, &es, nullptr))
{
    effect->initialize();

    numFxParams = SurgeFXType::numParams;

    for (int i = 0; i < numFxParams; ++i)
    {
        auto pmd = effect->paramAt(i);

        // Use natural (display) min/max/default directly from ParamMetaData.
        // The label string drives units:unit in the LV2 TTL via the JUCE patch.
        auto* p = new juce::AudioParameterFloat(
            juce::ParameterID(pmd.name, 1),
            pmd.name,
            juce::NormalisableRange<float>(pmd.minVal, pmd.maxVal),
            pmd.defaultVal,
            juce::AudioParameterFloatAttributes().withLabel(pmd.unit)
        );
        addParameter(p);
        fxParams[i] = p;
        effect->paramStorage[i] = pmd.naturalToNormalized01(pmd.defaultVal);
    }
}

void SingleEffectProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    gs.sampleRate = sampleRate;
    effect->onSampleRateChanged();
}

bool SingleEffectProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    auto in  = layouts.getMainInputChannelSet();
    return out == juce::AudioChannelSet::stereo() &&
           (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void SingleEffectProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Push current JUCE param values (natural units) into ConcreteConfig paramStorage (normalized).
    for (int i = 0; i < numFxParams; ++i)
    {
        auto pmd = effect->paramAt(i);
        effect->paramStorage[i] = pmd.naturalToNormalized01(*fxParams[i]);
    }

    const int totalSamples = buffer.getNumSamples();
    const int numChannels  = buffer.getNumChannels();

    auto* L = buffer.getWritePointer(0);
    auto* R = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

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

void SingleEffectProcessor::getStateInformation(juce::MemoryBlock& data)
{
    juce::XmlElement xml("State");
    for (int i = 0; i < numFxParams; ++i)
        xml.setAttribute("p" + juce::String(i), (double)*fxParams[i]);
    copyXmlToBinary(xml, data);
}

void SingleEffectProcessor::setStateInformation(const void* data, int sizeInBytes)
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
            float val = (float)xml->getDoubleAttribute(key, pmd.defaultVal);
            *fxParams[i] = std::clamp(val, pmd.minVal, pmd.maxVal);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SingleEffectProcessor();
}
