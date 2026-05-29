#!/usr/bin/env python3
"""Post-build patcher for Surge XT Rotary Speaker dsp.ttl."""
import sys, re

path = sys.argv[1]
text = open(path).read()

# 1. Category
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:ModulatorPlugin ;\n',
)

# 2. Description (fill in before shipping)
# text = text.replace(
#     'doap:description "Surge XT Rotary Speaker"',
#     'rdfs:comment """...""" ;\n\tdoap:description "Surge XT Rotary Speaker"',
# )

open(path, 'w').write(text)
print(f"Patched {path}")
