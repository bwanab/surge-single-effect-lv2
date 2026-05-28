# JUCE Feature Request: Expose Parameter Units in LV2 TTL Output

## Summary

JUCE's `AudioProcessorParameter` already carries a `label` field (returned by
`getLabel()`) that typically holds a unit string such as `"%"`, `"Hz"`, `"ms"`,
or `"dB"`. JUCE writes this label into AU/VST3 metadata, but the LV2 TTL writer
in `juce_audio_plugin_client_LV2.cpp` does not emit it as a `units:unit`
annotation. As a result, LV2 hosts (notably MODEP / MOD-UI) display raw
numeric values for every parameter instead of formatted values like
`250 ms` or `-12 dB`.

## Why this matters

- The LV2 `units` extension (`http://lv2plug.in/ns/extensions/units#`) is the
  standard mechanism for hosts to render parameter values with units.
- MOD Devices / MODEP rely on `units:unit` annotations to render pedal UIs
  correctly. Without them, every knob shows a normalized float.
- Plugins built with JUCE that already populate the parameter label correctly
  for AU and VST3 currently have no way to surface that same information to
  LV2 hosts without forking JUCE or post-processing the generated TTL.

## Proposed change

In `juce_audio_plugin_client_LV2.cpp`, inside the parameter visitor (around
the existing block that writes `lv2:minimum`, `lv2:maximum`, and
`lv2:portProperty`), add emission of `units:unit` when the parameter's label
matches a known LV2 unit. The mapping is small and well-defined by the
`lv2:units` ontology:

```cpp
// Emit units:unit annotation when the parameter label matches a known LV2 unit.
{
    const auto label = param.getLabel();
    const char* unitStr = nullptr;
    if      (label == "%")  unitStr = "units:pc";
    else if (label == "Hz") unitStr = "units:hz";
    else if (label == "ms") unitStr = "units:ms";
    else if (label == "dB") unitStr = "units:db";
    if (unitStr != nullptr)
        os << " ;\n\tunits:unit " << unitStr;
}
```

The `units:` namespace is already declared in the prologue of the generated
TTL by JUCE, so no additional `@prefix` is needed.

## Coverage and extension

The four labels above (`%`, `Hz`, `ms`, `dB`) cover the vast majority of
plugin parameters. The LV2 `units` extension defines additional URIs that
could be mapped if JUCE later wishes to expand the table (`units:s`,
`units:khz`, `units:bpm`, `units:semitone12TET`, `units:bar`, `units:beat`,
`units:cent`, `units:db`, `units:degree`, `units:frame`, `units:midiNote`, etc.).

## Customization hook (optional)

For plugins whose parameter labels use non-standard strings (e.g., `"%"` vs
`"percent"`), JUCE could add a virtual hook on `AudioProcessorParameter` —
e.g., `String getLV2UnitURI() const` — that defaults to a label-based lookup
but can be overridden. This would also help unusual units (cents, semitones,
custom formatters). The default implementation would be the table above.

## Workaround in use today

A small post-build tool (~120 lines of C++) introspects the plugin's
parameter metadata, reads JUCE's generated `dsp.ttl`, and inserts
`units:unit` annotations on each matching block. See:

- https://github.com/bwanab/surge-single-effect-lv2/blob/main/tools/patch-lv2-units.cpp

This works but is awkward: every JUCE-based LV2 plugin author either has to
reimplement this fix-up step or accept incorrect host displays.

## Impact

- Backward compatible: parameters whose label is empty or non-matching
  produce identical output to today.
- Affects only LV2 TTL output; AU and VST3 metadata are unchanged.
- Single-file change with no new dependencies.

## Reference

- LV2 units extension: https://lv2plug.in/ns/extensions/units/
- File to modify: `modules/juce_audio_plugin_client/juce_audio_plugin_client_LV2.cpp`
  (around the parameter visitor lambda, near line 1100 as of JUCE 8.0)
