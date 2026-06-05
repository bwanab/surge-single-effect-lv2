For all the effects:

Author: Bill Allen
Homepage: https://github.com/bwanab/surge-single-effect-lv2
Features: Modelled by Surge Synth Team (https://surge-synthesizer.github.io/)

--- CAVEAT (include prominently at the top of every patchstorage description) ---

⚠ KNOWN LIMITATION: MIDI CC and OSC parameter binding is not available for these plugins
in MODEP. This is a bug in MODEP's handling of JUCE-built LV2 plugins — all parameters
use the patch:writable protocol rather than traditional lv2:ControlPort ports, and MODEP's
MIDI/OSC binding UI only works with the latter. Parameters are fully adjustable via the
plugin GUI and Settings panel, but cannot be mapped to MIDI controllers or expression
pedals in real time. The Settings panel parameter list will also appear empty.

This is a known upstream issue being tracked at:
https://github.com/mod-audio/mod-ui/issues/161

---------------------------------------------------------------------------------


1) Reverb1 -
Overrides:
	Decay: max: 20000, min: 500, default: 5000


Description:
	A shimmer style reverb.

2) Rotary - 
Overrides:
	Horn: max: 10, min is ok, default: 2


Descriptions:
	An software implementation of the famed cabinets normally used by organs.

3) Delay
Overrides:



Descriptions:
	A nice sounding delay. The GUI exposes bpm and ratio settings from which the delay duration is computed.
	These values can be overridden in the settings along with other settings.

4) Flanger
Overrides:
	Rate: min: 0.01, max: 2, default: 0.5


Descriptions:
	A solid flanger effect.

5) Phaser
Overrides:
	Depth: default: 50%
	Rate: min: 0.01, max: 10, default: 2



Descriptions:
	A solid phaser effect.

