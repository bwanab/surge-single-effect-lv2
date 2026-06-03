#!/usr/bin/env python3
"""Post-build patcher for Surge XT Phaser dsp.ttl."""
import sys, re

path = sys.argv[1]
text = open(path).read()

NOTE = (
    'Note: This plugin uses LV2 patch:writable parameters (an atom-based protocol). '
    "MODEP's controls table only shows traditional lv2:ControlPort parameters, "
    'so it will appear empty — all parameters are accessible through the GUI and Settings panel.'
)
COMMENT = (
    'A solid phaser effect.\n\n'
    'Features: Modelled by Surge Synth Team (https://surge-synthesizer.github.io/)\n\n'
    + NOTE
)

def set_param(text, name, default=None, minimum=None, maximum=None):
    def replacer(m):
        b = m.group(0)
        if default is not None:
            b = re.sub(r'(lv2:default )[\d.]+', rf'\g<1>{default}', b)
        if minimum is not None:
            b = re.sub(r'(lv2:minimum )[\d.]+', rf'\g<1>{minimum}', b)
        if maximum is not None:
            b = re.sub(r'(lv2:maximum )[\d.]+', rf'\g<1>{maximum}', b)
        return b
    return re.sub(rf'plug:{re.escape(name)}\n(?:\t.*\n)+', replacer, text)

# 1. Category
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:PhaserPlugin ;\n',
)

# 2. Author / homepage
text = text.replace('foaf:name "Surge Synth Team"', 'foaf:name "Bill Allen"')
text = text.replace(
    'foaf:homepage <https://surge-synthesizer.github.io/>',
    'foaf:homepage <https://github.com/bwanab/surge-single-effect-lv2>',
)

# 3. Description
text = text.replace(
    'doap:description "Surge XT Phaser"',
    f'rdfs:comment """{COMMENT}""" ;\n\tdoap:description "Surge XT Phaser"',
)

# 4. Rate: cap to musically useful range (full range 0.0078–512 Hz is unusable on a knob)
text = set_param(text, 'Rate', default=2, minimum=0.01, maximum=10)

# 5. Depth: sensible default (0% is inaudible)
text = set_param(text, 'Depth', default=50)

open(path, 'w').write(text)
print(f"Patched {path}")
