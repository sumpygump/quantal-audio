
# Quantal Audio VCV Rack Plugins

This is a series of plugins for [VCV Rack](https://vcvrack.com/).

![Modules](https://github.com/sumpygump/quantal-audio/raw/master/doc/img/quantal-audio-mods.png)

Modules in this plugin pack:

## Horsehair VCO | 7HP

Horsehair is a promise of adventure. This module is an 3-oscillator VCO. The
main two oscillators (A and B) are interpolated between square and saw waves.
For each of the two main oscillators, you can modulate the octave for each
independently as well as the shape (change smoothly between square and saw) and
the pulse width (only affects the square wave component of the oscillation).

| Parameter | Range | Description |
| ---- | ---- | ---- |
| Input : 1V/Oct | -10v to 10v | 0 volts is middle C (261.63) |
| Param : Pitch | -2 to 2 | The pitch knob will adjust the main pitch input as a fine tune. All the way left (-2) will decrease the pitch by a tritone (184.99 Hz F#). All the way right (+2) will increase the pitch by a tritone (369.99 Hz F#) |
| Param: Oct A | -5 to 4 | The Oct A knob will adjust the main pitch input by an octave for oscillator A. |
| Param: Oct B | -5 to 4 | The Oct B knob will adjust the main pitch input by an octave for oscillator B. |
| Param: Shape A | 0 to 1 | The Shape A knob controls the waveform shape for oscillator A. It interpolates between a square wave (0) and a saw wave (1). This parameter can be controlled by a CV input. |
| Input: Osc A Shape CV Input | -10v to 10v | Voltage input controls the waveform shape for oscillator A. |
| Param: Shape B | 0 to 1 | The Shape B knob controls the waveform shape for oscillator B. It interpolates between a square wave (0) and a saw wave (1). This parameter can be controlled by a CV input. |
| Input: Osc B Shape CV Input | -10v to 10v | Voltage input controls the waveform shape for oscillator B. |
| Param: PW A | 0 to 1 | The PW A knob controls the pulsewidth value for the square wave component of oscillator A. This parameter can be controlled by a CV input. |
| Input: Osc A PW CV Input | -10v to 10v | Voltage input controls the pulsewith value for oscillator A. |
| Param: PW B | 0 to 1 | The PW B knob controls the pulsewidth value for the square wave component of oscillator B. This parameter can be controlled by a CV input. |
| Input: Osc A PW CV Input | -10v to 10v | Voltage input controls the pulsewith value for oscillator B. |
| Param: Mix level | 0 to 100% | The mix knob controls the overall mix level from both oscillator A and B. At extreme left (0%) only oscillator A will be output. At extreme right (100%) only oscillator B will be output. The mix can be controlled by a CV input. |
| Input: Mix CV Input | -10v to 10v | Voltage input controls the mix level. |
| Output: Osc Mix |  | Output signal of mix of oscillator A and B. |
| Output: Sine |  | Separate sine wave oscillator output. The pitch matches the octave and pitch of oscillator A. |

## Daisy Mix Modular Mixer (suite of modules)

Daisy chain is a suite of narrow modules when put together constitute a
flexible, modular mixer. Piece together your mixer with the following building
block modules: DC2 (Stereo channel strip), AUX (Aux send channel), VU (a vu
meter), Blank Separator, and a Master Mix bus.

When they are connected together, they will form a chain comprising a working
connected mixer. The signal flow only works by connecting channels from left
to right, the master bus on the end (as the right-most module).

Below is an image of an example daisy mixer composed from various daisy chain
mixer modules with several channels linked together:

![Daisy Mixer Example](https://github.com/sumpygump/quantal-audio/raw/master/doc/img/quantal-audio-daisy-mixer2.png)

## D-MX2 | Daisy Mix Master STEREO | 3HP

The Daisy Mix Master is the final module in the end of a Daisy chain mixer, is a
master bus for the Daisy-chained channels. It has a master fader knob with CV
control, a mute button, and two output channels (L and R). The master bus will
clamp signals to (+/-) 12 volt output signal.

| Parameter | Range | Description |
| ---- | ---- | ---- |
| Param : Mix Level | -inf dB to +6 dB | The overall mix level for all collected daisy channels to the left. The center default value is 0dB (100% of incoming mix signal). |
| Input : CV mix level input | -10v to 10v | Voltage input controls mix level amount for master bus. |
| Param : Mute/solo button | -1,0,1v (on/off) | When enabled, this button will mute all signals for the master bus. To enable solo, longpress the button (1.5 seconds). It will turn green when in solo mode. Note, if you automate this you should know that the underlying values are: `-1.0`=solo, `0.0`=off and `1.0`=mute.|
| Output : Channel L mix | -10v to 10v | Final output signal for left channel post level knob and CV. |
| Output : Channel R mix | -10v to 10v | Final output signal for right channel post level knob and CV. |

### Context menu

**Smooth level CV.** When enabled, this will add a 6ms slew to the level CV
input. This makes it better for general handling of abrupt changes in signal.
(Enabled by default).

**Create *n* channel(s)...** The context menu of this module provides a few
convenience entries to create channel modules to the left, with the following
options:

 - Create 1 channel
 - Create 1 channel with vu meter
 - Create 2 channels
 - Create 2 channels with vu meters
 - Create 2 channels with vu meters + aux sends
 - Create 4 channels
 - Create 4 channels with vu meters
 - Create 4 channels with vu meters + aux sends

![Daisy mix master context menu](https://github.com/sumpygump/quantal-audio/raw/master/doc/img/daisy-master-context-menu.png)

## DC2 | Daisy Mix Channel STEREO | 2HP

How many channel strips do you need in your modular mixer? Compose your mixer
with only as many as you need by using a stereo daisy mix channel (DC2). Place
down a copy of this module for each stereo input needed as long as they are to
the left of a D-MX2 module (master bus). Daisy Mix supports up to 16
channels. Each channel has a stereo input, a CV input to control the fader, a
mute button, a panning knob and individual stereo channel outs (post fader and
CV).

| Parameter | Range | Description |
| ---- | ---- | ---- |
| Input : Left/mono channel input | -10v to 10v | Audio signal for the left channel input for this channel strip. If there is no cable plugged into the right pair input, the module asssumes mono input. Supports a polyphony cable. |
| Input : Right channel input | -10v to 10v | Audio signal for the right channel input for this channel strip. Supports a polyphony cable. |
| Input : Fader amplitude CV input | -10v to 10v | Voltage input controls the amplitude amount for this channel strip. Lowest value provides 0% of signal from input(s) and highest value provides 100% of signal. |
| Param : Fader | 0 to 1 | The fader controls the volume (amplitude) of input signal. |
| Param : Pan | L to R | Pan knob controls the stereo position of the channel strip's signal between the left and right channels. |
| Param : Mute button | on/off | When enabled, this button will mute the input signals. |
| Output : Channel L output | -10v to 10v | Final output signal for left channel post fader, CV, pan and mute for this channel. Only use if you want to pipe output just from this channel to elsewhere in your patch. |
| Output : Channel R output | -10v to 10v | Final output signal for right channel post fader, CV, pan and mute for this channel. Only use if you want to pipe output just from this channel to elsewhere in your patch. |
| Daisy chain lights | | The daisy chain lights at the bottom of this module in the blue 'daisy' area indicate that this module is connected with other modules in this chain. The left light means it successfully is connected to a daisy mix module on the left side and the right light means it successfully is connected to a supported daisy mix module on the right side. The output signals and aux send signals will be passed down the chain to be processed by other daisy mix modules. |
| Aux send | | To send the signal from this channel strip to an aux group later in the chain, use the context menu: Aux Group 1 Send Amt and Aux Group 2 Send Amt. The amount (0% to 100%) sent can be collected and processed by an AUX module later in the chain. |

### Context menu

**Aux Group *x* Send Amt.** Use these sliders to send the signal (post-fader)
to the appropriate Aux module later in the chain (see AUX module below).

**Direct outs pre-mute.** Enable this to send the signal through the direct
outs of this channel strip before the mute button. Can use this to handle
processing the audio of this channel strip elsewhere or putting an effect on
the signal to be brought back in later in the chain without mixing in the
original dry signal too.

**Smooth level CV.** When enabled, this will add a 6ms slew to the level CV
input. This makes it better for general handling of abrupt changes in signal.
(Enabled by default).

![Daisy mix channel context menu showing the aux group sends](https://github.com/sumpygump/quantal-audio/raw/master/doc/img/daisy-channel-context-menu.png)

## AUX | Daisy Mix Channel Aux Sends | 2HP

The AUX module receives signal from DC2 modules to the left within a daisy mix
chain that have an amount defined to be sent from the DC2 context menu. The AUX
module can act as a group 1 receiver or a group 2 receiver. This module
provides stereo outputs that can be sent to another signal processing flow
outside the chain. Receive that processed audio by creating another DC2 module
acting as a return.

| Parameter | Range | Description |
| ---- | ---- | ---- |
| Param: Group button | Radio switch | Pressing this button will select which group this AUX module is acting for: either group 1 or group 2. The lights below will show which group this AUX is configured for which to receive signal. |
| Output : Channel L output | -10v to 10v | Output signal for collected signals in the left channel from DC2 modules sending an amount to the configured group for this AUX module. |
| Output : Channel R output | -10v to 10v | Output signal for collected signals in the right channel from DC2 modules sending an amount to the configured group for this AUX module. |

## VU | Daisy Mix Channel VU Meter | 1HP

Attach a VU module to the right of a DC2 (Daisy Channel strip) or an AUX (Daisy
Aux Sends) and it will show a VU meter of the voltage signal for that channel
strip.

## Daisy Mix Blank Separator | 2HP

The blank separator provides no special functionality other than it supports
being part of the daisy chain and will send the signals along the chain. It is
only useful as a visual separation module between other channels in the daisy
mix as you deem necessary.

## MULT | Buffered Multiple | 2HP

Two 1x3 voltage copies. With switch in the middle in the down position, it will
turn into a 1x6 copier.

## MIX | Unity Mix | 2HP

Sum and average signals from 3 inputs into 1 output. With switch in middle in
the down position, it will take 6 inputs to 2 outputs.

## MIXER-2 | Master Mixer | 5HP

The Mixer-2 is a two-channel mixer with a mono/stereo switch. In mono mode,
input channel one will be copied to channel two's output. This is useful if you
want to copy a mono signal to route to modules that need stereo inputs.

### Context menu

**Smooth level CV.** When enabled, this will add a 6ms slew to the level CV
input. This makes it better for general handling of abrupt changes in signal.
(Enabled by default).
