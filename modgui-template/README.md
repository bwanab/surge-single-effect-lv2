# MODEP GUI Template for Surge XT Effects

This template provides the starting point for creating custom pedal interfaces
for the Surge XT effects in MODEP.

## Getting Started

### 1. Build the plugins (on Mac with Docker)

```sh
cd surge-single-effect-lv2
git submodule update --init --recursive

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

### 2. Deploy to Raspberry Pi

```sh
# Copy all LV2 bundles to Pi
scp -r build-linux/src/*_artefacts/Release/LV2/*.lv2 pi@patchbox.local:/tmp/

# On the Pi: move to MODEP plugins directory
ssh pi@patchbox.local
sudo mv /tmp/*.lv2 /var/modep/lv2/
sudo systemctl restart modep-mod-ui
```

### 3. Add modgui to each plugin

For each plugin, copy and customize the template:

```sh
# Example for Flanger
sudo mkdir -p "/var/modep/lv2/Surge XT Flanger.lv2/modgui"
sudo cp modgui-template/* "/var/modep/lv2/Surge XT Flanger.lv2/modgui/"
# Then edit the files as shown below
```

---

## Plugin URIs and Parameters

### Flanger
**URI:** `https://surge-synthesizer.github.io/plugins/surge-xt-flanger`

| Symbol | Label | Range | Unit |
|--------|-------|-------|------|
| Mode | Mode | 0-3 | enum: Classic, Doppler, Arp Mix, Arp Solo |
| Waveform | Waveform | 0-5 | enum: Sine, Tri, Saw, S&G, S&H, Square |
| Rate | Rate | -7 to 9 | Hz |
| Depth | Depth | 0-1 | % |
| Count | Count | 1-4 | voices |
| Base_20Pitch | Base Pitch | 0-127 | MIDI note |
| Spacing | Spacing | 0-12 | semitones |
| Feedback | Feedback | 0-1 | % |
| HF_20Damping | HF Damping | 0-1 | % |
| Width | Width | 0-1 | % |
| Mix | Mix | 0-1 | % |

### Phaser
**URI:** `https://surge-synthesizer.github.io/plugins/surge-xt-phaser`

| Symbol | Label |
|--------|-------|
| Center | Center |
| Feedback | Feedback |
| Sharpness | Sharpness |
| Rate | Rate |
| Depth | Depth |
| Stereo | Stereo |
| Mix | Mix |
| Width | Width |
| Count | Count |
| Spread | Spread |
| Waveform | Waveform |
| Tone | Tone |

### Reverb 1
**URI:** `https://surge-synthesizer.github.io/plugins/surge-xt-reverb1`

| Symbol | Label |
|--------|-------|
| Pre-Delay | Pre-Delay |
| Room_20Shape | Room Shape |
| Size | Size |
| Decay_20Time | Decay Time |
| HF_20Damping | HF Damping |
| Low_20Cut | Low Cut |
| Peak_20Freq | Peak Freq |
| Peak_20Gain | Peak Gain |
| High_20Cut | High Cut |
| Mix | Mix |
| Width | Width |

### Delay
**URI:** `https://surge-synthesizer.github.io/plugins/surge-xt-delay`

| Symbol | Label |
|--------|-------|
| Left | Left |
| Right | Right |
| Feedback | Feedback |
| Crossfeed | Crossfeed |
| Low_20Cut | Low Cut |
| High_20Cut | High Cut |
| Rate | Rate |
| Depth | Depth |
| Channel | Channel |
| Mix | Mix |
| Width | Width |

### Rotary Speaker
**URI:** `https://surge-synthesizer.github.io/plugins/surge-xt-rotary`

| Symbol | Label |
|--------|-------|
| Horn_20Rate | Horn Rate |
| Doppler | Doppler |
| Tremolo | Tremolo |
| Rotor_20Rate | Rotor Rate |
| Drive | Drive |
| Model | Model |
| Width | Width |
| Mix | Mix |

---

## Complete Flanger Example

### modgui/modgui.ttl

```turtle
@prefix lv2:    <http://lv2plug.in/ns/lv2core#> .
@prefix modgui: <http://moddevices.com/ns/modgui#> .
@prefix rdfs:   <http://www.w3.org/2000/01/rdf-schema#> .

<https://surge-synthesizer.github.io/plugins/surge-xt-flanger>
    modgui:gui [
        modgui:resourcesDirectory <modgui> ;
        modgui:iconTemplate <modgui/icon.html> ;
        modgui:stylesheet <modgui/stylesheet.css> ;
        modgui:screenshot <modgui/screenshot.png> ;
        modgui:thumbnail <modgui/thumbnail.png> ;
        modgui:brand "Surge Synth Team" ;
        modgui:label "Flanger" ;
        modgui:model "boxy" ;
        modgui:panel [
            modgui:color "#2D5D7B" ;
        ] ;
        modgui:port [
            lv2:index 0 ;
            lv2:symbol "Mode" ;
            lv2:name "Mode" ;
        ] ;
        modgui:port [
            lv2:index 1 ;
            lv2:symbol "Waveform" ;
            lv2:name "Waveform" ;
        ] ;
        modgui:port [
            lv2:index 2 ;
            lv2:symbol "Rate" ;
            lv2:name "Rate" ;
        ] ;
        modgui:port [
            lv2:index 3 ;
            lv2:symbol "Depth" ;
            lv2:name "Depth" ;
        ] ;
        modgui:port [
            lv2:index 4 ;
            lv2:symbol "Count" ;
            lv2:name "Count" ;
        ] ;
        modgui:port [
            lv2:index 5 ;
            lv2:symbol "Base_20Pitch" ;
            lv2:name "Base Pitch" ;
        ] ;
        modgui:port [
            lv2:index 6 ;
            lv2:symbol "Spacing" ;
            lv2:name "Spacing" ;
        ] ;
        modgui:port [
            lv2:index 7 ;
            lv2:symbol "Feedback" ;
            lv2:name "Feedback" ;
        ] ;
        modgui:port [
            lv2:index 8 ;
            lv2:symbol "HF_20Damping" ;
            lv2:name "HF Damping" ;
        ] ;
        modgui:port [
            lv2:index 9 ;
            lv2:symbol "Width" ;
            lv2:name "Width" ;
        ] ;
        modgui:port [
            lv2:index 10 ;
            lv2:symbol "Mix" ;
            lv2:name "Mix" ;
        ] ;
    ] .
```

### modgui/icon.html

```html
<div class="pedal surge-flanger">
    <div class="pedal-header">
        <h1>Flanger</h1>
        <span class="brand">Surge XT</span>
    </div>

    <div class="pedal-controls">
        <!-- Row 1: Mode and Waveform selects -->
        <div class="select-row">
            <div class="select-group">
                <select data-port-symbol="Mode">
                    <option value="0">Classic</option>
                    <option value="1">Doppler</option>
                    <option value="2">Arp Mix</option>
                    <option value="3">Arp Solo</option>
                </select>
                <span class="select-label">Mode</span>
            </div>
            <div class="select-group">
                <select data-port-symbol="Waveform">
                    <option value="0">Sine</option>
                    <option value="1">Triangle</option>
                    <option value="2">Saw</option>
                    <option value="3">S&G</option>
                    <option value="4">S&H</option>
                    <option value="5">Square</option>
                </select>
                <span class="select-label">Wave</span>
            </div>
        </div>

        <!-- Row 2: Rate, Depth, Feedback -->
        <div class="knob-row">
            <div class="knob-group">
                <div class="knob" data-port-symbol="Rate"></div>
                <span class="knob-label">Rate</span>
            </div>
            <div class="knob-group">
                <div class="knob" data-port-symbol="Depth"></div>
                <span class="knob-label">Depth</span>
            </div>
            <div class="knob-group">
                <div class="knob" data-port-symbol="Feedback"></div>
                <span class="knob-label">Feedback</span>
            </div>
        </div>

        <!-- Row 3: Width, Mix -->
        <div class="knob-row">
            <div class="knob-group">
                <div class="knob" data-port-symbol="Width"></div>
                <span class="knob-label">Width</span>
            </div>
            <div class="knob-group">
                <div class="knob large" data-port-symbol="Mix"></div>
                <span class="knob-label">Mix</span>
            </div>
        </div>
    </div>

    <div class="pedal-footer">
        <div class="bypass-switch" data-port-symbol=":bypass"></div>
        <div class="pedal-led"></div>
    </div>
</div>
```

### modgui/stylesheet.css

See `stylesheet.css` in this directory - it's ready to use.

---

## Files

- `modgui.ttl` - LV2 metadata linking the GUI resources
- `icon.html` - HTML structure for the pedal face
- `stylesheet.css` - CSS styling (Surge-themed)
- `screenshot.png` - 1280x800 recommended (create on Pi)
- `thumbnail.png` - 128x64 recommended (create on Pi)

---

## Development Workflow on Pi

### Quick iteration (no restart needed)

```sh
# Edit CSS
sudo nano "/var/modep/lv2/Surge XT Flanger.lv2/modgui/stylesheet.css"

# Edit HTML
sudo nano "/var/modep/lv2/Surge XT Flanger.lv2/modgui/icon.html"

# Refresh browser - changes appear immediately
```

### After changing modgui.ttl

```sh
sudo systemctl restart modep-mod-ui
```

### Create screenshot images

1. Style the pedal in browser
2. Take screenshot
3. Resize to 128x64 for thumbnail, ~300x400 for screenshot
4. Copy to modgui folder

---

## Reference

- [MOD SDK Documentation](https://wiki.mod.audio/wiki/MOD_SDK)
- [LV2 Specifications](https://lv2plug.in/ns/)
- [MODEP on Blokas](https://blokas.io/modep/)
