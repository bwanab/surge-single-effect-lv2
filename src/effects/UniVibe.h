#pragma once

#include <cmath>
#include <algorithm>
#include "sst/effects/EffectCore.h"
#include "sst/basic-blocks/params/ParamMetadata.h"

// UniVibe — four-stage optical phaser modelled on the Shin-ei Univibe circuit.
//
// The original hardware drives a common incandescent lamp with a sine LFO. Four
// light-dependent resistors (LDRs) respond to the lamp brightness and each sets
// the pole frequency of a first-order allpass filter. Because LDR resistance
// varies exponentially with light, the pole sweep is nonlinear even though the
// lamp drive is sinusoidal — this is the core of the UniVibe character.
//
// Three of the four stages use identical capacitors (0.015 µF in the Shin-ei);
// the fourth uses 0.022 µF, making its frequency 15/22 ≈ 0.68× that of the
// others. This asymmetry creates the characteristic "lopsided" phasing that
// distinguishes a UniVibe from a generic equal-stage phaser.
//
// In Chorus mode the phase-shifted signal is averaged with dry; the swept phase
// cancellations produce the classic undulating, warbly sound. In Vibrato mode
// only the phase-shifted signal passes, giving pure pitch modulation.

namespace sst::effects::univibe
{
template <typename FXConfig>
struct UniVibe : core::EffectTemplateBase<FXConfig>
{
    enum uv_params
    {
        uv_rate = 0, // LFO speed (Hz)
        uv_depth,    // sweep depth
        uv_vibrato,  // 0 % = chorus, 100 % = vibrato
        uv_mix,      // dry / wet
        uv_num_params,
    };

    static constexpr int numParams{uv_num_params};
    static constexpr const char *streamingName{"univibe"};
    static constexpr const char *displayName{"UniVibe"};

    UniVibe(typename FXConfig::GlobalStorage *s, typename FXConfig::EffectStorage *e,
            typename FXConfig::ValueStorage *p)
        : core::EffectTemplateBase<FXConfig>(s, e, p)
    {
        static_assert(core::ValidEffect<UniVibe>);
    }

    void initialize()
    {
        lfoPhase = 0.f;
        for (int ch = 0; ch < 2; ++ch)
            for (int st = 0; st < 4; ++st)
                apState[ch][st] = 0.f;
    }

    void processBlock(float *__restrict L, float *__restrict R);

    void suspendProcessing() { initialize(); }
    int getRingoutDecay() const { return 100; }
    void onSampleRateChanged() { initialize(); }

    basic_blocks::params::ParamMetaData paramAt(int idx) const
    {
        using pmd = basic_blocks::params::ParamMetaData;
        switch ((uv_params)idx)
        {
        case uv_rate:
            return pmd().withName("Rate").withRange(0.1f, 10.f).withDefault(1.f)
                        .withLinearScaleFormatting("Hz");
        case uv_depth:
            return pmd().withName("Depth").withRange(0.f, 1.f).withDefault(0.5f)
                        .withLinearScaleFormatting("%", 100.f);
        case uv_vibrato:
            return pmd().withName("Vibrato").withRange(0.f, 1.f).withDefault(0.f)
                        .withLinearScaleFormatting("%", 100.f);
        case uv_mix:
            return pmd().withName("Mix").withRange(0.f, 1.f).withDefault(0.5f)
                        .withLinearScaleFormatting("%", 100.f);
        default:
            return pmd().withName("Unknown " + std::to_string(idx));
        }
    }

private:
    float lfoPhase{0.f};
    float apState[2][4]{};

    // First-order allpass coefficient from cutoff frequency.
    // H(z) = (a + z^{-1}) / (1 + a*z^{-1}),  a = (tan(πfc/fs) − 1) / (tan(πfc/fs) + 1)
    static float apCoeff(float fc, float sr)
    {
        float t = std::tan(M_PI * fc / sr);
        return (t - 1.f) / (t + 1.f);
    }

    // One-multiply first-order allpass; state is one sample of memory per stage.
    static float apProcess(float x, float &state, float a)
    {
        float y = a * x + state;
        state = x - a * y;
        return y;
    }

    // Optical LFO curve: sinusoidal lamp drive → exponential LDR response.
    // k controls the nonlinearity depth; k=2.5 gives a warm, moderately
    // asymmetric sweep characteristic of the Shin-ei circuit.
    static float opticalCurve(float phase)
    {
        constexpr float k = 2.5f;
        float brightness = 0.5f * (1.f + std::sin(2.f * M_PI * phase));
        return (std::exp(k * brightness) - 1.f) / (std::exp(k) - 1.f);
    }
};

template <typename FXConfig>
void UniVibe<FXConfig>::processBlock(float *__restrict L, float *__restrict R)
{
    const float sr      = (float)this->sampleRate();
    const float rateHz  = std::clamp(this->floatValue(uv_rate), 0.1f, 10.f);
    const float depth   = std::clamp(this->floatValue(uv_depth), 0.f, 1.f);
    const float vibrato = std::clamp(this->floatValue(uv_vibrato), 0.f, 1.f);
    const float mix     = std::clamp(this->floatValue(uv_mix), 0.f, 1.f);

    // Advance LFO one block at a time (16 samples ≈ 0.33 ms at 48 kHz — fine for LFO use).
    lfoPhase += rateHz * FXConfig::blockSize / sr;
    if (lfoPhase >= 1.f) lfoPhase -= 1.f;

    const float sweep = opticalCurve(lfoPhase);

    // Pole frequency sweep range. The Shin-ei sweeps from roughly 200 Hz (lamp dark /
    // LDR high-resistance) to ~1500 Hz (lamp bright / LDR low-resistance). The depth
    // parameter scales how far up that range the sweep travels.
    constexpr float freqMin = 200.f;
    constexpr float freqMax = 1500.f;
    // Stage 4 capacitor is 0.022 µF vs 0.015 µF for stages 1–3: ratio = 15/22.
    constexpr float stageRatio[4] = {1.f, 1.f, 1.f, 15.f / 22.f};

    const float fBase = freqMin * std::pow(freqMax / freqMin, depth * sweep);
    float coeffs[4];
    for (int st = 0; st < 4; ++st)
        coeffs[st] = apCoeff(fBase * stageRatio[st], sr);

    float *ch[2] = {L, R};
    for (int c = 0; c < 2; ++c)
    {
        for (int s = 0; s < FXConfig::blockSize; ++s)
        {
            const float dry = ch[c][s];
            float phased = dry;
            for (int st = 0; st < 4; ++st)
                phased = apProcess(phased, apState[c][st], coeffs[st]);

            // Chorus: average dry + phase-shifted → swept phase-cancellation notches.
            // Vibrato: phase-shifted only → pitch modulation with no dry reference.
            // Blend continuously between the two modes via vibrato parameter.
            const float wet = (1.f - vibrato) * 0.5f * (dry + phased) + vibrato * phased;
            ch[c][s] = (1.f - mix) * dry + mix * wet;
        }
    }
}

} // namespace sst::effects::univibe
