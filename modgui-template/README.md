# MODEP GUI Template for Surge XT Effects

This template provides the starting point for creating custom pedal interfaces
for the Surge XT effects in MODEP.

## Files

- `modgui.ttl` - LV2 metadata linking the GUI resources
- `icon.html` - HTML structure for the pedal face
- `stylesheet.css` - CSS styling (Surge-themed)
- `screenshot.png` - instruments view image (instrument mode), 1280x800 recommended
- `thumbnail.png` - pedalboard view image (thumb size), 128x64 recommended

## Installation

For each plugin (e.g., Flanger), create a `modgui/` folder inside the `.lv2` bundle:

```
Surge XT Flanger.lv2/
├── manifest.ttl
├── dsp.ttl
├── libSurge XT Flanger.so
└── modgui/
    ├── modgui.ttl
    ├── icon.html
    ├── stylesheet.css
    ├── screenshot.png
    └── thumbnail.png
```

## Customization Steps

### 1. Update modgui.ttl

Replace placeholders with actual values:

```turtle
<https://surge-synthesizer.github.io/plugins/surge-xt-flanger>
    modgui:gui [
        modgui:label "Flanger" ;
        modgui:port [
            lv2:index 0 ;
            lv2:symbol "mode" ;
            lv2:name "Mode" ;
        ] ;
        # ... more ports
    ] .
```

### 2. Get port symbols

Run `lv2info` on the plugin to see available ports:

```sh
lv2info https://surge-synthesizer.github.io/plugins/surge-xt-flanger
```

Or check the generated `dsp.ttl` file in the build output.

### 3. Update icon.html

Map knobs to port symbols:

```html
<div class="knob" data-port-symbol="rate">
    <span class="knob-name">Rate</span>
</div>
```

### 4. Create images

- **thumbnail.png**: Small image for pedalboard view (128x64)
- **screenshot.png**: Full-size preview (around 1280x800 or match pedal size)

You can screenshot the pedal in a browser or create graphics in your image editor.

## Testing on MODEP

1. Copy the `.lv2` bundle with `modgui/` to `/var/modep/lv2/`
2. Restart MODEP: `sudo systemctl restart modep-mod-ui`
3. Open MODEP web interface and add the plugin
4. Edit CSS/HTML directly on Pi for quick iteration:
   ```sh
   sudo nano /var/modep/lv2/Surge\ XT\ Flanger.lv2/modgui/stylesheet.css
   ```
5. Refresh browser to see changes (no restart needed for CSS/HTML)

## Port Types

### Knobs (continuous parameters)
```html
<div class="knob" data-port-symbol="depth"></div>
```

### Switches (toggle/bypass)
```html
<div class="bypass-switch" data-port-symbol=":bypass"></div>
```

### Select (enumerated values)
```html
<select data-port-symbol="mode">
    <option value="0">Classic</option>
    <option value="1">Doppler</option>
</select>
```

## Reference

- [MOD SDK Documentation](https://wiki.mod.audio/wiki/MOD_SDK)
- [LV2 Specifications](https://lv2plug.in/ns/)
- [MODEP on Blokas](https://blokas.io/modep/)
