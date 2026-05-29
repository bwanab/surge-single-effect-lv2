#!/usr/bin/env python3
"""Post-build patcher for Surge XT Delay dsp.ttl."""
import sys, re

path = sys.argv[1]
text = open(path).read()

# 1. Category
text = text.replace(
    '\ta lv2:Plugin ;\n',
    '\ta lv2:Plugin , lv2:DelayPlugin ;\n',
)

# 2. Description (fill in before shipping)
# text = text.replace(
#     'doap:description "Surge XT Delay"',
#     'rdfs:comment """...""" ;\n\tdoap:description "Surge XT Delay"',
# )

open(path, 'w').write(text)
print(f"Patched {path}")
