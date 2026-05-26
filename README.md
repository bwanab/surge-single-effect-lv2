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
brew install cmake ninja
```

For LV2 testing install [Carla](https://kx.studio/Applications:Carla) from kx.studio or
`brew install carla` if available. VST3 builds work in any compatible DAW.

### Linux / Raspberry Pi (aarch64, MODEP target)

```sh
sudo apt install cmake ninja-build build-essential pkg-config \
                 libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev \
                 libxinerama-dev libxcursor-dev libgl1-mesa-dev lv2-dev
```

### macOS → Linux cross-build via Docker (M1/Apple Silicon)

On Apple Silicon the Docker Linux aarch64 container runs at native speed and produces
exact MODEP-ready binaries:

```sh
# One-time: pull a builder image
docker pull ghcr.io/surge-synthesizer/surge-build-environment:latest  # or use ubuntu:24.04

# Build inside container (mount the repo)
docker run --rm -v $(pwd):/work -w /work ubuntu:24.04 bash -c "
  apt-get update -q && \
  apt-get install -y cmake ninja-build build-essential pkg-config \
    libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libgl1-mesa-dev lv2-dev && \
  cmake -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release && \
  cmake --build build-linux
"
```

The LV2 bundles will be in `build-linux/src/surge-fx-<name>_artefacts/`.

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
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

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

`sst-effects` exposes each effect's parameters via `effect->paramAt(int i)` which returns
a `sst::basic_blocks::params::ParamMetaData` struct with `minVal`, `maxVal`, `defaultVal`,
`name`, and `unit` already in display units (Hz, ms, %, dB, etc.).

`SingleEffectProcessor` creates one `AudioParameterFloat(min, max, default)` per parameter
directly from that metadata. The parameter label (unit string) is picked up by a small patch
in JUCE's LV2 TTL writer (`patches/juce-lv2-units.patch`) to emit `units:unit` annotations
— so MODEP shows "250 ms" instead of "0.07".

The patch is applied automatically during `cmake` configuration.
