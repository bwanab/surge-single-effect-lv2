#!/usr/bin/env python3
"""Post-build patcher for Surge XT Chorus dsp.ttl.

Applies four corrections that cannot be made upstream (dsp.ttl is generated
by JUCE from the Surge build):

  1. Adds lv2:ChorusPlugin so MODEP sorts it under Modulators
  2. Adds rdfs:comment for the info page description (MODEP reads this field,
     not doap:description)
  3. Caps Rate (fx_parm_0) maximum at 20 Hz (upstream value: 512 Hz)
  4. Removes _unused_8 through _unused_11 from parameter declarations
     and from patch:writable / patch:readable lists

Note: MODEP's info page parameter table only shows traditional lv2:ControlPort
inputs. Surge XT uses the patch:writable atom protocol, so parameters do not
appear in that table — this is a MODEP limitation, not something fixable here.
"""
import sys
import re

path = sys.argv[1]
text = open(path).read()

# 1. Category: lv2:Plugin -> lv2:Plugin , lv2:ChorusPlugin
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:ChorusPlugin ;\n',
)

# 2. Description — MODEP reads rdfs:comment, not doap:description.
#    Insert rdfs:comment before the existing doap:description line.
COMMENT = (
    'A beautiful sounding chorus pedal much like that ubiquitous blue pedal.\n\n'
    'Credit: The Surge XT Team'
)
text = text.replace(
    'doap:description "Surge XT Chorus"',
    f'rdfs:comment """{COMMENT}""" ;\n\tdoap:description "Surge XT Chorus"',
)

# 3. Rate maximum: 512 Hz -> 1 Hz (fx_parm_0 is the only param with max 512)
text = text.replace(
    '\tlv2:maximum 512 ;\n',
    '\tlv2:maximum 1 ;\n',
)

# 4a. Remove _unused_N parameter definition blocks
text = re.sub(
    r'plug:fx_parm_\d+\n\ta lv2:Parameter ;\n\trdfs:label "_unused_\d+" ;'
    r'.*?\tlv2:maximum 1 \.\n\n',
    '',
    text,
    flags=re.DOTALL,
)

# 4b. Remove fx_parm_8–11 from patch:writable and patch:readable lists.
#     Both lists end with fx_parm_11 ; — change the last kept entry (fx_parm_7)
#     from "," to ";" and drop the four trailing entries.
text = re.sub(
    r'(plug:fx_parm_7) ,\n'
    r'\t\tplug:fx_parm_8 ,\n'
    r'\t\tplug:fx_parm_9 ,\n'
    r'\t\tplug:fx_parm_10 ,\n'
    r'\t\tplug:fx_parm_11 ;',
    r'\1 ;',
    text,
)

open(path, 'w').write(text)
print(f"Patched {path}")
