#!/usr/bin/env python3
"""Post-build patcher for Surge XT Flanger dsp.ttl."""
import sys, re

path = sys.argv[1]
text = open(path).read()

# 1. Category
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:FlangerPlugin ;\n',
)

# 2. Description (fill in before shipping)
# text = text.replace(
#     'doap:description "Surge XT Flanger"',
#     'rdfs:comment """...""" ;\n\tdoap:description "Surge XT Flanger"',
# )

open(path, 'w').write(text)
print(f"Patched {path}")
