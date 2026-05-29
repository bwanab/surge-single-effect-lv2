# MODEP Skin (modgui) Howto

This guide covers adding a MODEP GUI skin to any effect in this repo. The chorus
skin is the worked example; all other effects follow the same pattern.

---

## Background

Every LV2 plugin shown in MODEP gets either a custom skin or a default "sardine
can" box. A custom skin requires three things inside the LV2 bundle:

- `modgui.ttl` — declares the plugin's GUI resources
- `modgui/icon.html` — Mustache template for the pedal icon
- `modgui/stylesheet.css` — CSS for the pedal (references bundled images)
- Bundled images — pedal body, knob, footswitch PNGs served via `{{{ns}}}`
- `manifest.ttl` must contain `rdfs:seeAlso <modgui.ttl>`

**Important:** All JUCE-built LV2 effects in this repo (and Surge XT effects
generally) expose their controls as `patch:writable` atom parameters, not
traditional `lv2:ControlPort` ports. MODEP's template engine populates
`{{#controls}}` from `modgui:port` entries in `modgui.ttl`, not from the LV2
port list. The two systems are independent.

---

## Step 1: Find the Plugin URI and Parameters

After building, inspect the effect's `dsp.ttl`:

```
build/src/surge-fx-<name>_artefacts/Release/LV2/Surge XT <Name>.lv2/dsp.ttl
```

The plugin URI is declared near the bottom:

```turtle
<https://surge-synth-team.org//plugins/Surge_XT_Flanger>
    a lv2:Plugin ;
```

Parameters appear at the top with their symbols, labels, ranges, and units:

```turtle
plug:fx_parm_0
    a lv2:Parameter ;
    rdfs:label "Rate" ;
    lv2:minimum 0.0078125 ;
    lv2:maximum 512 ;
    units:unit units:hz .
```

Choose which parameters to expose as knobs in the skin. Three knobs (the most
important controls) makes for the clearest pedal. Use `rdfs:label` for the name
and the `fx_parm_N` local name for the symbol.

---

## Step 2: Create the Directory Structure

```
modgui/surge-xt-<name>/
├── modgui.ttl
└── modgui/
    ├── icon.html
    ├── stylesheet.css
    ├── screenshot.png        ← generated in Step 6
    ├── thumbnail.png         ← generated in Step 6
    ├── pedals/
    │   ├── footswitch.png
    │   └── boxy/
    │       └── <color>.png
    └── knobs/
        └── boxy/
            └── <style>.png
```

The `<name>` must match the short name used in `src/CMakeLists.txt`
(e.g. `flanger`, `phaser`). The build system detects this directory
automatically and folds it into the bundle.

---

## Step 3: Write `modgui.ttl`

```turtle
@prefix lv2:    <http://lv2plug.in/ns/lv2core#> .
@prefix modgui: <http://moddevices.com/ns/modgui#> .

<PLUGIN_URI>
    modgui:gui [
        modgui:resourcesDirectory <modgui> ;
        modgui:iconTemplate  <modgui/icon.html> ;
        modgui:stylesheet    <modgui/stylesheet.css> ;
        modgui:screenshot    <modgui/screenshot.png> ;
        modgui:thumbnail     <modgui/thumbnail.png> ;
        modgui:brand  "Surge Synth Team" ;
        modgui:label  "<Effect Name>" ;
        modgui:model  "boxy" ;
        modgui:color  "blue" ;
        modgui:knob   "aluminium" ;
        modgui:port [
            lv2:index 0 ;
            lv2:symbol "fx_parm_N" ;
            lv2:name "Knob Label" ;
        ] , [
            lv2:index 1 ;
            lv2:symbol "fx_parm_N" ;
            lv2:name "Knob Label" ;
        ] , [
            lv2:index 2 ;
            lv2:symbol "fx_parm_N" ;
            lv2:name "Knob Label" ;
        ] ;
    ] .
```

- `lv2:index` is a sequential display order (0, 1, 2…), not a port index.
- `lv2:symbol` must match the `fx_parm_N` local name from `dsp.ttl`.
- `modgui:color` and `modgui:knob` must have corresponding bundled images (see Step 5).

---

## Step 4: Write `modgui/icon.html`

Use the standard boxy template. Change `mod-three-knobs` to match the number
of `modgui:port` entries declared.

```html
<div class="mod-pedal mod-pedal-boxy{{{cns}}} mod-three-knobs mod-{{color}} {{color}}">
    <div mod-role="drag-handle" class="mod-drag-handle"></div>
    <div class="mod-plugin-brand"><h1>{{brand}}</h1></div>
    <div class="mod-plugin-name"><h1>{{label}}</h1></div>
    <div class="mod-light on" mod-role="bypass-light"></div>
    <div class="mod-control-group mod-{{knob}} clearfix">
        {{#controls}}
        <div class="mod-knob">
            <div class="mod-knob-image" mod-role="input-control-port" mod-port-symbol="{{symbol}}"></div>
            <span class="mod-knob-title">{{name}}</span>
        </div>
        {{/controls}}
    </div>
    <div class="mod-footswitch" mod-role="bypass"></div>
    <div class="mod-pedal-input">
        {{#effect.ports.audio.input}}
        <div class="mod-input mod-input-disconnected" title="{{name}}" mod-role="input-audio-port" mod-port-symbol="{{symbol}}">
            <div class="mod-pedal-input-image"></div>
        </div>
        {{/effect.ports.audio.input}}
    </div>
    <div class="mod-pedal-output">
        {{#effect.ports.audio.output}}
        <div class="mod-output mod-output-disconnected" title="{{name}}" mod-role="output-audio-port" mod-port-symbol="{{symbol}}">
            <div class="mod-pedal-output-image"></div>
        </div>
        {{/effect.ports.audio.output}}
    </div>
</div>
```

For a different knob count replace `mod-three-knobs` with `mod-four-knobs`,
`mod-five-knobs`, etc.

---

## Step 5: Write `modgui/stylesheet.css` and Bundle Images

Copy `stylesheet.css` from `modgui/surge-xt-chorus/modgui/stylesheet.css` and
update the two color-specific rules to match your chosen `modgui:color` and
`modgui:knob`. The rules to change are at the bottom:

```css
/* Replace "blue" and "aluminium" with your chosen color/knob */
.mod-pedal-boxy{{{cns}}}.mod-blue {
    background-image: url(/resources/pedals/boxy/blue.png{{{ns}}});
}
.mod-pedal-boxy{{{cns}}}.mod-blue h1,
.mod-pedal-boxy{{{cns}}}.mod-blue .mod-control-group .mod-knob > span.mod-knob-title {
    color: black;   /* use white for dark pedal colors */
}
.mod-pedal-boxy{{{cns}}}.mod-blue h1 {
    border-color: black;   /* use white for dark pedal colors */
}
.mod-pedal-boxy{{{cns}}} .mod-control-group.mod-aluminium .mod-knob .mod-knob-image {
    background-image: url(/resources/knobs/boxy/aluminium.png{{{ns}}});
}
```

**Images to bundle** — copy from `gx_chorus.lv2/modgui/` on the Pi:

```bash
SRC=/var/modep/lv2/gx_chorus.lv2/modgui
DST=modgui/surge-xt-<name>/modgui

mkdir -p $DST/pedals/boxy $DST/knobs/boxy
cp $SRC/pedals/boxy/<color>.png  $DST/pedals/boxy/
cp $SRC/pedals/footswitch.png    $DST/pedals/
cp $SRC/knobs/boxy/<style>.png   $DST/knobs/boxy/
```

**Available colors** (in `gx_chorus.lv2/modgui/pedals/boxy/`):
`blue`, `petrol`, `tribal1`

**Available knob styles** (in `gx_chorus.lv2/modgui/knobs/boxy/`):
`aluminium`, `blue`, `bronze`, `petrol`

For dark pedal colors (petrol, tribal1) set text/border to `white` in the CSS.

---

## Step 6: Build, Deploy, and Generate Screenshots

### Build

The build system automatically detects `modgui/surge-xt-<name>/` and folds it
into the LV2 bundle, including patching `manifest.ttl`.

```bash
cmake --build build --config Release -j2
```

### Deploy

**Do not** use `cp -r SRC DST` when the destination directory already exists —
it nests the source inside rather than replacing it. Instead copy files
explicitly:

```bash
SRC="build/src/surge-fx-<name>_artefacts/Release/LV2/Surge XT <Name>.lv2"
DST="/var/modep/lv2/surge-xt-<name>.lv2"

sudo cp "$SRC/dsp.ttl" "$SRC/manifest.ttl" "$SRC/modgui.ttl" "$DST/"
sudo cp -r "$SRC/modgui/." "$DST/modgui/"
sudo chown -R modep:modep "$DST"
sudo systemctl restart modep-mod-ui.service
```

### Generate Screenshots

MODEP renders the skin headlessly and produces the exact right dimensions.
Run these after confirming the skin renders correctly on a pedalboard.

```bash
URI="https%3A%2F%2Fsurge-synth-team.org%2F%2Fplugins%2FSurge_XT_<Name>"
DST=modgui/surge-xt-<name>/modgui

curl -s -o $DST/screenshot.png "http://localhost/effect/image/screenshot.png?uri=$URI"
curl -s -o $DST/thumbnail.png  "http://localhost/effect/image/thumbnail.png?uri=$URI"
```

Then redeploy:

```bash
sudo cp $DST/screenshot.png $DST/thumbnail.png \
    /var/modep/lv2/surge-xt-<name>.lv2/modgui/
```

Rebuild and redeploy once more so the screenshots are baked into the bundle for
future installs.

---

## Optional: Patching dsp.ttl

The build system automatically runs `modgui/surge-xt-<name>/patch-dsp.py` if
that file exists. Use it to correct metadata that can't be changed upstream
(JUCE generates `dsp.ttl` from plugin source code):

- **Category** — add `lv2:ChorusPlugin`, `lv2:FlangerPlugin`, etc. so MODEP
  sorts the effect into the right bucket (Modulators, Delays, …)
- **Description** — replace the generic `doap:description` with real copy
- **Parameter ranges** — tighten musically extreme min/max values
- **Unused parameters** — remove `_unused_N` entries from `patch:writable`,
  `patch:readable`, and the parameter definition blocks

Copy `modgui/surge-xt-chorus/patch-dsp.py` as a starting template. The script
receives the path to the deployed `dsp.ttl` as its first argument.

---

## Chorus: Special Case

The chorus DSP (`sst-effects/Chorus.h`) is an empty stub. Its DSP comes from
`../surge/build/surge_xt_products/Surge XT Chorus.lv2`. The CMake target
`surge-fx-chorus` copies that pre-built bundle and then overlays the modgui
files from `modgui/surge-xt-chorus/`. The process otherwise follows the same
steps above.

---

## Quick Reference: Chorus Parameters

For reference, the full chorus parameter list (from `dsp.ttl`):

| Symbol      | Label    | Range              | Units |
|-------------|----------|--------------------|-------|
| `fx_parm_0` | Rate     | 0.0078125 – 20     | Hz    |
| `fx_parm_1` | Depth    | 0 – 100            | %     |
| `fx_parm_2` | Time     | 0.488 – 125        | ms    |
| `fx_parm_3` | Feedback | 0 – 100            | %     |
| `fx_parm_4` | Low Cut  | 13.75 – 25087.7    | Hz    |
| `fx_parm_5` | High Cut | 13.75 – 25087.7    | Hz    |
| `fx_parm_6` | Width    | −24 – 24           | dB    |
| `fx_parm_7` | Mix      | 0 – 100            | %     |
