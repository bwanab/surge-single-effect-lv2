#!/usr/bin/env python3
"""Post-build patcher for Surge XT Delay dsp.ttl."""
import sys, re

path = sys.argv[1]
text = open(path).read()

NOTE = (
    'Note: This plugin uses LV2 patch:writable parameters (an atom-based protocol). '
    "MODEP's controls table only shows traditional lv2:ControlPort parameters, "
    'so it will appear empty — all parameters are accessible through the GUI and Settings panel.'
)
COMMENT = (
    'A nice sounding delay. The GUI exposes BPM and ratio settings from which the delay '
    'duration is computed. These values can be overridden in the Settings panel along with '
    'other parameters.\n\n'
    'Features: Modelled by Surge Synth Team (https://surge-synthesizer.github.io/)\n\n'
    + NOTE
)

# 1. Category
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:DelayPlugin ;\n',
)

# 2. Author / homepage
text = text.replace('foaf:name "Surge Synth Team"', 'foaf:name "Bill Allen"')
text = text.replace(
    'foaf:homepage <https://surge-synthesizer.github.io/>',
    'foaf:homepage <https://github.com/bwanab/surge-single-effect-lv2>',
)

# 3. Description
text = text.replace(
    'doap:description "Surge XT Delay"',
    f'rdfs:comment """{COMMENT}""" ;\n\tdoap:description "Surge XT Delay"',
)

open(path, 'w').write(text)
print(f"Patched {path}")
