# surge-single-effect-lv2

Standalone LV2 (and VST3) plugins for individual Surge XT effects, built directly on
[sst-effects](https://github.com/surge-synthesizer/sst-effects) without pulling in all of Surge.
Designed for [MODEP](https://blokas.io/modep/) on Raspberry Pi, developed on macOS.

Included effects:
| Plugin | LV2 URI suffix |
|---|---|
| Flanger | `surge-xt-flanger` |
| Phaser | `surge-xt-phaser` |
| Reverb 1 | `surge-xt-reverb1` |
| Delay | `surge-xt-delay` |
| Rotary Speaker | `surge-xt-rotary` |

---

## Prerequisites

### macOS (development + testing)

```sh
brew install cmake
```

Ninja is optional — the build works with Make (the default generator).

For LV2 testing install [Carla](https://kx.studio/Applications:Carla) from kx.studio or
`brew install carla` if available. VST3 builds work in any compatible DAW.

#### macOS 15 (Sequoia) Compatibility

This project requires updated library versions for macOS 15 support. The submodules have been
updated to versions compatible with both macOS 15 and the Surge project:

- **JUCE**: Updated to `surge_8.0.12` branch (fixes deprecated `CGWindowListCreateImage` API)
- **sst-basic-blocks/effects/filters**: Updated to versions matching Surge
- **sst-waveshapers**: Added as new submodule (required by RotarySpeaker effect)
- **simde**: Fetched automatically for ARM/Apple Silicon SIMD support
- **fmt**: Fetched automatically (required by sst-basic-blocks)

### Linux / Raspberry Pi (aarch64, MODEP target)

```sh
sudo apt install cmake ninja-build build-essential pkg-config \
                 libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev \
                 libxinerama-dev libxcursor-dev libgl1-mesa-dev lv2-dev \
                 libcurl4-openssl-dev libwebkit2gtk-4.1-dev
```

### macOS → Linux cross-build via Docker (M1/Apple Silicon)

On Apple Silicon the Docker Linux aarch64 container runs at native speed and produces
exact MODEP-ready binaries:

```sh
docker run --rm -v $(pwd):/work -w /work ubuntu:24.04 bash -c "
  apt-get update -q && \
  apt-get install -y cmake ninja-build build-essential pkg-config git \
    libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libgl1-mesa-dev lv2-dev \
    libcurl4-openssl-dev libwebkit2gtk-4.1-dev && \
  git config --global --add safe.directory /work && \
  git submodule update --init --recursive && \
  cmake -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release && \
  cmake --build build-linux -- -j 2
"
```

The `-j 2` limits parallelism to avoid running out of memory in Docker.

The LV2 bundles will be in `build-linux/src/surge-fx-<name>_artefacts/Release/LV2/`.

---

## Clone

```sh
git clone https://github.com/surge-synthesizer/surge-single-effect-lv2
cd surge-single-effect-lv2
git submodule update --init --recursive
```

---

## Build

### macOS (VST3 + LV2)

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or with Ninja (if installed): `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`

Built plugins appear in `build/src/surge-fx-<name>_artefacts/`.

### Raspberry Pi / Linux (LV2 only)

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Install to MODEP

```sh
# Copy each .lv2 bundle to MODEP's plugin directory
for bundle in build/src/*_artefacts/LV2/*.lv2; do
  sudo cp -r "$bundle" /var/modep/lv2/
done

# Trigger MODEP plugin rescan
sudo systemctl restart modep-mod-ui
```

### Test on macOS with Carla

Open Carla → Add Plugin → scan `build/src/`. The LV2 bundles live in
`build/src/surge-fx-<name>_artefacts/LV2/`. VST3 builds are in the `VST3/` folder next to it.

---

## Adding a new effect

1. **Check availability** — see which effects are in
   `libs/sst/sst-effects/include/sst/effects/`. `Chorus.h` is a stub; Ensemble is not yet
   in sst-effects (still Surge-only).

2. **Add the `#elif` branch** in `src/SingleEffectProcessor.h`:
   ```cpp
   #elif defined(SURGE_FX_IS_REVERB2)
   #include <sst/effects/Reverb2.h>
   using SurgeFXType = sst::effects::reverb2::Reverb2<sst::effects::core::ConcreteConfig>;
   ```

3. **Add one line** in `src/CMakeLists.txt`:
   ```cmake
   surge_add_effect(reverb2  REVERB2  "Reverb 2"  "surge-xt-reverb2"  "S2R2")
   ```

4. Re-run CMake and build.

### Available sst-effects (as of this writing)

| Header | Namespace | Notes |
|---|---|---|
| `Flanger.h` | `sst::effects::flanger` | ✓ included |
| `Phaser.h` | `sst::effects::phaser` | ✓ included |
| `Reverb1.h` | `sst::effects::reverb1` | ✓ included |
| `Delay.h` | `sst::effects::delay` | ✓ included |
| `RotarySpeaker.h` | `sst::effects::rotaryspeaker` | ✓ included |
| `Reverb2.h` | `sst::effects::reverb2` | ready to add |
| `Bonsai.h` | `sst::effects::bonsai` | ready to add |
| `FloatyDelay.h` | `sst::effects::floatydelay` | ready to add |
| `Nimbus.h` | `sst::effects::nimbus` | needs Eurorack/clouds |
| `TreeMonster.h` | `sst::effects::treemonster` | ready to add |
| `Chorus.h` | — | stub, not implemented yet |

---

## How parameters work

### DSP side

`sst-effects` exposes each effect's parameters via `effect->paramAt(int i)` which returns
a `sst::basic_blocks::params::ParamMetaData` struct with `minVal`, `maxVal`, `defaultVal`,
`name`, and `unit` already in display units (Hz, ms, %, dB, etc.).

`SingleEffectProcessor` creates one `AudioParameterFloat(min, max, default)` per parameter
directly from that metadata. JUCE emits the full parameter table into each plugin's
`dsp.ttl`, including `units:unit` annotations so MODEP displays "250 ms" instead of "0.07".

### lv2:ControlPort vs. patch:writable — why it matters for MODEP skins

LV2 has two distinct ways to expose plugin parameters, and they are **not
interchangeable** in MODEP skins.

**The old way — `lv2:ControlPort`**

When LV2 was designed (circa 2007) it modeled plugins after hardware rack
units. Every knob was a dedicated "ControlPort" wire: a named socket that
carries exactly one float per audio block. The host enumerates them at load
time, displays them in its UI, and writes a new float value whenever the user
moves a knob. Simple, fast, universal. Nearly all pre-2018 LV2 plugins work
this way.

**The new way — `patch:writable` / atoms**

As plugins grew more complex (MIDI instruments, sample loaders, spectral
processors), a single float per socket was not enough. LV2 introduced "atoms":
a typed, self-describing message format that can carry floats, ints, strings,
arrays, MIDI events, or anything else. Instead of dozens of dedicated wires,
plugins get one bidirectional message bus (an `atom:Sequence` port). Parameters
are set by sending a `patch:Set` message — essentially "set parameter `<uri>`
to value `3.5`". The parameter list is declared in the TTL via `patch:writable`
rather than as individual ports.

**Why Surge XT uses `patch:writable`**

JUCE's LV2 export (introduced in the surge-synthesizer JUCE fork) implements
the atom/patch protocol for all plugin parameters. This is the more modern,
extensible approach — it supports arbitrary data types, rich preset recall, and
future-proofing for features that don't fit in a single float. Every JUCE-built
LV2 plugin works this way, including all the Surge XT effects in this repo.

**Why this matters for MODEP modgui skins**

MODEP's modgui skin engine has two separate knob-binding mechanisms:

| Parameter type | HTML `mod-role` | binding attribute | lookup table |
|---|---|---|---|
| `lv2:ControlPort` | `input-control-port` | `mod-port-symbol="rate"` | `effect.ports.control.input` |
| `patch:writable` | `input-parameter` | `mod-parameter-uri="https://…:fx_parm_0"` | `effect.parameters` |

Using `input-control-port` on a Surge XT plugin makes knobs render correctly
(they get label text from `modgui:port`) but they do nothing when dragged —
the lookup returns nothing because the ControlPort list is empty. You must use
`input-parameter` with the full parameter URI.

The full URI for each parameter is the plugin base URI plus `:fx_parm_N`. For
example, for Surge XT Chorus Rate:
```
https://surge-synth-team.org//plugins/Surge_XT_Chorus:fx_parm_0
```

Retrieve all parameter URIs for a deployed plugin with:

```bash
curl -s "http://localhost/effect/get?uri=<ENCODED_URI>" \
  | python3 -c "import sys,json; [print(p['uri'], p.get('name','')) for p in json.load(sys.stdin)['parameters']]"
```

Because `mod-parameter-uri` requires the full URI (not just a short symbol),
and the Mustache `{{#controls}}` loop only exposes `{{symbol}}`, **knobs must
be written out individually** in each effect's `icon.html` rather than using
the generic loop. See `docs/modgui-skin-howto.md` for the complete template.
