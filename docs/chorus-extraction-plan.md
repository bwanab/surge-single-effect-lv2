# Plan: Extract Chorus Effect to sst-effects

## Motivation

The `surge-single-effect-lv2` project builds standalone LV2 plugins from effects in `sst-effects`. Currently Chorus exists only in Surge proper (`src/common/dsp/effects/ChorusEffect.h`) and cannot be used independently. Extracting it to `sst-effects` would enable:

- Standalone LV2/VST3 Chorus plugin for MODEP and other hosts
- Reuse in other sst-based projects
- Consistent architecture with already-extracted effects (Flanger, Phaser, etc.)

## Current State

**Surge implementation:**
- `src/common/dsp/effects/ChorusEffect.h` - class declaration (templated on voice count)
- `src/common/dsp/effects/ChorusEffectImpl.h` - implementation

**sst-effects:**
- `include/sst/effects/Chorus.h` - empty stub (header guards only)

## Dependencies Analysis

| Dependency | Location | sst-effects equivalent | Action needed |
|------------|----------|------------------------|---------------|
| `Effect` base class | Surge | `EffectTemplateBase<FXConfig>` | Refactor to template pattern |
| `BiquadFilter` | Surge | `sst::filters::Biquad` | Use sst-filters |
| `vembertech/lipol` | Surge | `sst::basic_blocks::dsp::lipol` | Already in sst-basic-blocks |
| `SurgeStorage` | Surge | `FXConfig::GlobalStorage` | Template parameter |
| `FxStorage` | Surge | `FXConfig::EffectStorage` | Template parameter |
| `pdata` | Surge | `FXConfig::ValueStorage` | Template parameter |
| `globals.h` (BLOCK_SIZE, etc.) | Surge | `FXConfig::blockSize` | Template constant |
| `sst/basic-blocks/mechanics/*` | sst-basic-blocks | Same | Already available |
| `sst/basic-blocks/dsp/Clippers.h` | sst-basic-blocks | Same | Already available |

## Extraction Steps

### 1. Create header structure
```cpp
// include/sst/effects/Chorus.h
namespace sst::effects::chorus
{
template <typename FXConfig> 
struct Chorus : core::EffectTemplateBase<FXConfig>
{
    // ...
};
}
```

### 2. Convert parameters
Current Surge params → sst-effects ParamMetaData:
```cpp
enum chorus_params
{
    ch_time,      // delay time
    ch_rate,      // LFO rate  
    ch_depth,     // modulation depth
    ch_feedback,  // feedback amount
    ch_lowcut,    // HP filter
    ch_highcut,   // LP filter
    ch_mix,       // wet/dry
    ch_width,     // stereo width
};
```

Each needs a `paramAt(int)` implementation returning `ParamMetaData` with proper ranges, defaults, and units.

### 3. Replace Surge-specific types

| Surge type | Replacement |
|------------|-------------|
| `lipol_ps_blocksz` | `sst::basic_blocks::dsp::lipol_sse<FXConfig::blockSize, true>` |
| `BiquadFilter` | `sst::filters::Biquad<...>` or keep simple IIR inline |
| `SIMD_M128` | Direct `__m128` or simde equivalent |
| `lag<float, true>` | `sst::basic_blocks::dsp::SurgeLag<float>` |

### 4. Refactor constructor/initialization

From:
```cpp
ChorusEffect(SurgeStorage *storage, FxStorage *fxdata, pdata *pd)
    : Effect(storage, fxdata, pd), lp(storage), hp(storage)
```

To:
```cpp
Chorus(typename FXConfig::GlobalStorage *s, 
       typename FXConfig::EffectStorage *e,
       typename FXConfig::ValueStorage *p)
    : core::EffectTemplateBase<FXConfig>(s, e, p)
```

### 5. Port the DSP

The core algorithm is straightforward:
1. Write input to circular delay buffer
2. Read from buffer at LFO-modulated positions (one per voice)
3. Apply feedback through LP/HP filters
4. Mix wet/dry with stereo panning

Key functions to port:
- `init()` → `initialize()`
- `process(float*, float*)` → `processBlock(float*, float*)`
- `setvars(bool init)` → inline or helper method

### 6. Add to CMake

In `sst-effects/CMakeLists.txt`, ensure Chorus.h is included in the header list (likely automatic via glob).

## Testing Strategy

1. **Unit test in sst-effects**: Create test using `ConcreteConfig` similar to existing effect tests
2. **A/B comparison**: Run same audio through Surge Chorus and extracted Chorus, compare output
3. **Integration test**: Build as LV2 in surge-single-effect-lv2, test in Carla/MODEP

## Integration with surge-single-effect-lv2

Once extracted, add to this project:

**src/SingleEffectProcessor.h:**
```cpp
#elif defined(SURGE_FX_IS_CHORUS)
#include <sst/effects/Chorus.h>
using SurgeFXType = sst::effects::chorus::Chorus<sst::effects::core::ConcreteConfig>;
```

**src/CMakeLists.txt:**
```cmake
surge_add_effect(chorus  CHORUS  "Chorus"  "surge-xt-chorus"  "S2CH")
```

## Estimated Effort

- **Extraction & refactoring**: 4-6 hours
- **Testing & debugging**: 2-4 hours
- **Documentation**: 1 hour
- **Total**: ~1-2 days

## Open Questions for Surge Team

1. Is there interest in having Chorus extracted to sst-effects?
2. Should it support both 2-voice and 4-voice variants, or just one?
3. Any planned changes to Chorus that should be incorporated first?
4. Should Ensemble (BBD) extraction be considered as a follow-on project?

## References

- Existing extracted effect for reference: `sst-effects/include/sst/effects/Flanger.h`
- Surge Chorus: `surge/src/common/dsp/effects/ChorusEffect.h`
- sst-effects template pattern: `sst-effects/include/sst/effects/EffectCore.h`
