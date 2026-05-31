# ConcreteConfig Bugs — Analysis and Upstream Defense

## Background

`ConcreteConfig` is a self-described "WIP" convenience class in sst-effects for
using bus effects standalone, without full Surge state. The production equivalent
is `SurgeSSTFXAdapter`, which delegates all three of these functions to
`SurgeStorage` (table lookups backed by real math). The bugs were latent in
`ConcreteConfig` because it was never seriously exercised in a running audio
context.

This document covers the three fixes made in commit `71f489c` and evaluates
alternatives that would avoid touching the sst-effects submodule.

---

## Fix 1: `envelopeRateLinear` — `pow(-2, f)` → `powf(2.f, f)`

### What it does

Returns the fractional phase advance per processing block for a modulation LFO,
converting a log₂-Hz rate `f` into a per-block step size. Call sites in our
effects:

| File | Line | Effect |
|------|------|--------|
| `Flanger.h` | 262 | comb-filter sweep rate |
| `Delay.h` | 271 | chorus-LFO modulation rate |
| `Phaser.h` | 176 | all-pass stage sweep rate |

### Why the old code was wrong

`pow(-2, f)` where `f` is a floating-point non-integer invokes a domain error
under IEEE 754 / C99: a negative real base raised to a non-integer exponent
requires complex arithmetic, and `pow()` returns NaN. For odd integer exponents
it returns a negative value. Either way the result is wrong:

- **NaN**: multiplied into the LFO phase accumulator, the phase becomes NaN and
  all comb/all-pass outputs become NaN → silence. This is exactly why Flanger
  produced no sound at all.
- **Negative**: phase advances backward at incorrect magnitude.

Verification in Python (same semantics as C `pow()`):
```python
>>> import math
>>> math.pow(-2, 2.5)   # non-integer exponent
nan
>>> math.pow(-2, 3)     # odd integer
-8.0
```

### Why `powf(2.f, f)` is correct

Surge's `table_envrate_linear` is initialized as:

```cpp
double k = dsamplerate_os * pow(2.0, (i - 256.0) / 16.0) / BLOCK_SIZE_OS;
table_envrate_linear[i] = (float)(1.f / k);
```

After substituting the index mapping this is `blockSize / (sampleRate * pow(2, f))`.
`ConcreteConfig` approximates the same function without a lookup table as
`blockSize / sampleRate * pow(2, f)`. The base must be +2. The minus sign in the
original was simply a typo.

**This is an unambiguous C-language bug, easy to defend upstream.**

---

## Fix 2: `dbToLinear` — `return 1` → `powf(10.f, f / 20.f)`

### What it does

Converts a decibel value to a linear amplitude ratio. Used transitively by
`WidthProvider::setWidthTarget` (called by Reverb1 for its Width parameter) and
available to all effects via `EffectCore::dbToLinear`.

### Why the old code was wrong

`return 1` is a stub. Every dB value evaluates to unity gain regardless of
magnitude, so the Width parameter on Reverb1 was always pegged at 0 dBFS
regardless of the knob.

### Why `powf(10.f, f / 20.f)` is correct

This is the definition of dBFS → linear amplitude conversion. Surge's production
implementation (`SurgeStorage::db_to_linear`) uses a lookup table that computes
the same function. The formula is textbook.

**`return 1` is obviously an unfinished placeholder. This fix is easy to defend
upstream.**

---

## Fix 3: `isDeactivated` — `false` → `true`

This fix is more nuanced than the other two.

### What it does

Tells an effect whether a given parameter index is "deactivated" — meaning its
value should be ignored or its sub-feature bypassed. The semantics differ per
call site:

| Effect | Parameter index | `isDeactivated == true` means |
|--------|----------------|-------------------------------|
| `Delay.h:293` | `dly_time_right` | right channel follows left (linked) |
| `Delay.h:415,420` | `dly_highcut`, `dly_lowcut` | filter is bypassed |
| `Reverb1.h:318,325` | `rev1_lowcut`, `rev1_highcut` | filter is bypassed |
| `RotarySpeaker.h:319,340` | `rot_drive` | drive stage is bypassed |
| `Phaser.h:185` | `ph_mod_rate` | LFO frozen (rate used as phase offset) |
| `Phaser.h:289` | `ph_tone` | tone filter is bypassed |

### The specific problem that motivated the fix

```cpp
// Delay.h:293
auto isLinked = this->isDeactivated(dly_time_right) ? dly_time_left : dly_time_right;
```

With old `return false`: `isLinked = dly_time_right`. In Surge, `dly_time_right`
has its own UI control. In our single-effect plugin, only `Left` is exposed —
`dly_time_right` sits at `pmd.defaultVal` (−2 = 0.25 s) and never changes. The
result: left channel echoes at user-set time, right channel always at 0.25 s.
The Time knob audibly affects only one ear.

With `return true`: `isLinked = dly_time_left`. Both channels follow the same
Time knob — correct single-knob behavior.

### Unintended side effects of always returning `true`

- **`Delay.h:415,420`**: high/low-cut filters always bypassed. We don't expose
  those knobs in our 3-knob UI, so the behavior is indistinguishable from
  "not implemented."
- **`Reverb1.h:318,325`**: same — reverb hi/lo shelf filters always bypassed.
- **`RotarySpeaker.h:319,340`**: drive stage always bypassed, sound is always
  clean.
- **`Phaser.h:185`**: **this one matters.** When `isDeactivated(ph_mod_rate)`
  is `true`, Phaser interprets Rate as a static phase offset (frozen sweep
  position) rather than an LFO frequency. The knob still changes the sound
  (different tonal coloration) but the characteristic sweep does not oscillate.
  This is a real functional regression in the Phaser, even if it wasn't caught
  in initial testing.

### Why neither `true` nor `false` is universally correct

`dly_time_right` needs `true` (linked). `ph_mod_rate` needs `false` (active LFO).
These are contradictory requirements that a blanket return value cannot satisfy.

In Surge's production config, `isDeactivated` reads `e->p[idx].deactivated` — a
per-parameter boolean set by the UI. `ConcreteConfig::BC` has no such field.

---

## Recommended Upstream Fix for `isDeactivated`

Add a `deactivated[]` array to `ConcreteConfig::BC`, defaulting to `false`
(everything active), and adjust the signature to accept the base class:

```cpp
struct BC {
    static constexpr uint16_t maxParamCount{20};
    float paramStorage[maxParamCount];
    bool deactivated[maxParamCount]{};   // default: all active
    template <typename... Types> BC(Types...) {}
};

static inline bool isDeactivated(EffectStorage *e, const BaseClass *const b, int idx) {
    return b->deactivated[idx];
}
```

Our plugin would then explicitly set the deactivated flag for `dly_time_right`
after constructing the effect, matching how Surge uses the concept.

---

## PR Strategy

| Fix | Where it belongs | Rationale |
|-----|-----------------|-----------|
| `envelopeRateLinear` | upstream sst-effects PR | unambiguous bug, one correct answer |
| `dbToLinear` | upstream sst-effects PR | unambiguous stub, one correct answer |
| `isDeactivated` | upstream sst-effects PR (with `deactivated[]` array) + plugin update | blanket `true` is a workaround; proper fix needs per-parameter state |

Items 1 and 2 should be straightforward to merge. Item 3 requires a small API
addition to `ConcreteConfig::BC` and a corresponding update in our plugin to set
`effect->deactivated[dly_time_right_index] = true` at init time — at which point
our blanket `return true` can be reverted.

Until the upstream PR lands, the current `return true` is a workable interim
fix for our specific 3-knob pedal UI: all the parameters affected by the side
effects (hi/lo cut filters, drive) are not exposed as user controls anyway.
The Phaser sweep regression is the one known quality issue introduced by it.
