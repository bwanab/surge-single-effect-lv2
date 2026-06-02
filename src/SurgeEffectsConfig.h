#pragma once

#include <cmath>
#include <sst/basic-blocks/tables/TwoToTheXProvider.h>
#include <sst/basic-blocks/tables/DbToLinearProvider.h>

// Project-specific FX configuration, replacing ConcreteConfig for production use.
//
// Improvements over ConcreteConfig:
//   - envelopeRateLinear uses TwoToTheXProvider lookup table (matches Surge accuracy)
//   - dbToLinear uses DbToLinearProvider lookup table (matches Surge accuracy)
//   - isDeactivated reads per-parameter state from ES::deactivated[] instead of
//     returning a blanket value — allows callers to link/bypass specific parameters
struct SurgeEffectsConfig
{
    static constexpr uint16_t maxParamCount{20};

    struct BC
    {
        float paramStorage[maxParamCount]{};
        template <typename... Types> BC(Types...) {}
    };

    struct GS
    {
        double sampleRate;
        sst::basic_blocks::tables::TwoToTheXProvider twoToX;
        sst::basic_blocks::tables::DbToLinearProvider dbTable;

        GS(double sr) : sampleRate(sr)
        {
            twoToX.init();
            dbTable.init();
        }
    };

    struct ES
    {
        bool deactivated[maxParamCount]{};
    };

    using BaseClass = BC;
    using GlobalStorage = GS;
    using EffectStorage = ES;
    using ValueStorage = float *;
    using BiquadAdapter = SurgeEffectsConfig;

    static constexpr int blockSize{16};

    static inline float floatValueAt(const BC *const e, const ValueStorage *const, int idx)
    {
        return e->paramStorage[idx];
    }
    static inline int intValueAt(const BC *const e, const ValueStorage *const, int idx)
    {
        return (int)std::round(e->paramStorage[idx]);
    }

    static inline float envelopeRateLinear(GS *s, float f)
    {
        return (float)blockSize / (float)s->sampleRate * s->twoToX.twoToThe(f);
    }

    static inline float temposyncRatio(GS *, ES *, int) { return 1.f; }

    static inline bool isDeactivated(ES *e, int idx) { return e->deactivated[idx]; }
    static inline bool isTemposynced(ES *, int) { return false; }
    static inline bool isExtended(ES *, int) { return false; }

    static inline float rand01(GS *) { return (float)rand() / (float)RAND_MAX; }

    static inline double sampleRate(GS *s) { return s->sampleRate; }
    static inline double sampleRateInv(GS *s) { return 1.0 / s->sampleRate; }

    static inline float noteToPitch(GS *, float p) { return powf(2.f, p / 12.f); }
    static inline float noteToPitchIgnoringTuning(GS *s, float p) { return noteToPitch(s, p); }
    static inline float noteToPitchInv(GS *s, float p) { return 1.f / noteToPitch(s, p); }

    static inline float dbToLinear(GS *s, float f) { return s->dbTable.dbToLinear(f); }
};
