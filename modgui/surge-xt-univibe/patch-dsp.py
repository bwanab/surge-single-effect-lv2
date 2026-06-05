#!/usr/bin/env python3
"""Post-build patcher for Surge XT UniVibe dsp.ttl."""
import sys, re

path = sys.argv[1]
text = open(path).read()

NOTE = (
    '⚠ KNOWN LIMITATION: MIDI CC and OSC parameter binding is not available for this plugin in MODEP. '
    'This affects all JUCE-built LV2 plugins due to a bug in MODEP\'s handling of patch:writable parameters. '
    'Parameters are fully adjustable via the plugin GUI and Settings panel, but cannot be mapped to MIDI controllers or expression pedals. '
    'The Settings panel parameter list will also appear empty for the same reason. '
    'Upstream issue: https://github.com/mod-audio/mod-ui/issues/161'
)
COMMENT = (
    NOTE + '\n\n'
    'A four-stage optical phaser modelled on the Shin-ei Univibe circuit. '
    'The classic "Chorus" mode blends the phase-shifted signal with dry for a '
    'sweeping, warbly vibrato-like effect. The "Vibrato" mode passes only the '
    'phase-shifted signal for pure pitch modulation.\n\n'
    'Features: Modelled by Surge Synth Team (https://surge-synthesizer.github.io/)\n'
    'Maintainer: Bill Allen\n'
)

# 1. Category
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:PhaserPlugin ;\n',
)

# 2. Homepage → this repo (for issue tracking); author name stays "Surge Synth Team"
text = text.replace(
    'foaf:homepage <https://surge-synthesizer.github.io/>',
    'foaf:homepage <https://github.com/bwanab/surge-single-effect-lv2>',
)

# 3. Description
text = text.replace(
    'doap:description "Surge XT UniVibe"',
    f'rdfs:comment """{COMMENT}""" ;\n\tdoap:description "Surge XT UniVibe"',
)

open(path, 'w').write(text)
print(f"Patched {path}")
