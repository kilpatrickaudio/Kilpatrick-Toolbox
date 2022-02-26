# Kilpatrick-Toolbox

This repo contains Kilpatrick Toolbox Modules for
[VCV Rack Virtual Eurorack DAW](https://vcvrack.com). Kilpatrick Audio has been
designing and building music and audio electronics since 2008. Please visit the
links below for information about Kilpatrick Audio and its founder Andrew Kilpatrick.

**Useful links:**

* [VCV Rack Virtual Eurorack DAW](https://vcvrack.com) - VCV Rack is what this code runs on
* [Kilpatrick Audio](https://www.kilpatrickaudio.com) - Commercial music products from Kilpatrick Audio
* [Andrew Kilpatrick](https://www.andrewkilpatrick.org) - Creator of these modules

This plugin is provided open source and free of charge. If you like what we do please consider
buying one of our hardware products or commercial VCV Rack modules such as the K4815 Pattern Generator.

## Licence

All code in this repository is licensed under GPL 3.0 or later. All panel designs, graphics, logos
and other media files are copyright Kilpatrick Audio and/or others and may not be used, forked or
redistributed without permission.

---
### vMIDI
**Patchable MIDI in VCV Rack**

Most Virtue expander modules support the **vMIDI&trade;** protocol developed by Kilpatrick Audio. This allows
MIDI to be patched between modules as easily as patching CV. Expanders will output the raw MIDI data from modules.
Additional utility modules can be used to merge, process and display the MIDI data. You can even interface with other
MIDI devices to create complex setups.

You can split signals by stacking cables and routing the MIDI to multiple destinations. To merge signals together
the MIDI Merge module must be used. This will interleave messages from multiple inputs and
generate a single output. If you wish to remap different controller numbers or channels to use with specific hardware
or generate a composite signal from multiple controllers use the MIDI Map module.

For more information see the full spec: [vMIDI Implementation](vMIDI.md)


## Modules

----
### MIDI CC Note
**MIDI CC to Note Converter**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_CC_Note.png" />

The MIDI CC Note module allows conversion of MIDI CC messages to note messages. A common use case is a MIDI controller with
buttons that you wish to use to generate note events. The velocity of the notes generated can be set by the VELOCITY pot.

It is assumed that the CC buttons are arranged in order from low to high. You can remap CC messages into other orders using the
MIDI Mapper module. Set the CC base using the right click menu. This will shift all events to start at note 60. The CC message must
be >= 64 for press, and <64 for release. Use the **OCT UP**, **OCT NORM** and **OCT DOWN** buttons to shift the octave.

**Features:**

- CC input
- Note output
- Input and output jacks uses the **vMIDI&trade;** patchable MIDI protocol

<br clear="right"/>

----
### MIDI Channel
**MIDI Channel Filter / Splitter / Transposer**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Channel.png" />

The MIDI Channel module allows the channel of MIDI events to be remapped. A key split mode allows notes to be sent to two
different outputs depending on the note played. This can be used to split a regular MIDI keyboard into two different parts.
Finally a transpose function allows the outputs to be transposed up or down.

**Features:**

- MIDI channel filter allows omni or single channel input remapped to a single output channel
- MIDI key split function filters note messages onto two different outputs with configurable split point
- MIDI transpose will adjust notes up or down before they are output
- Key split function can be turned on and off without resetting the split setting
- All jacks uses the **vMIDI&trade;** patchable MIDI protocol

**Input and Output Jacks**

MIDI is input on the MIDI IN jack. This can accept any kind of MIDI message and route it to one of the two output jacks. The MIDI
OUT jacks are labeled **L** and **R** indicating the hand they will output when using key split mode. By default all messages
come out of the **R** hand jack.

**Setting the IN and OUT Channels**

To set the IN channel hover your mouse over the IN CHAN display and scroll up and down to set the channel. A setting of **ALL** will
accept all channels as input, whereas as setting of **CH 01** through **CH 16** will only accept that specific channel.

To set the OUT channel, scroll over the OUT CHAN display. Only a single channel can be output at a time.

**Using Key Split Mode**

To use key split, hover and scroll over the KEY SPLIT display to change the split point. To enable and disable key split on the fly
simply double click the display to toggle it on and off. When key split is enabled, low notes will come out of the **L** jack and
high notes will come out of the **R** jack.

**Using the Transpose Function**

To transpose the outputs up or down simply hover and scroll over the TRANS display. Transpose is processed after the key split.

<br clear="right"/>

----
### MIDI Clock
**MIDI Clock**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Clock.png" />

The MIDI Clock module implements a MIDI clock that can generate stable clocks internally or can sync to either analog or MIDI
clocks. The analog clock output can be divided down to quarter notes. The clock internally runs at the standard MIDI rate of
24 pulses per quarter. (PPQ)

When inputting an analog clock it must be at the 24PPQ rate. Use an LFO or VCO to generate clocks that vary if that's what you
want, otherwise us this module as the master clock.

An auto-start function can be used so that the clock will start automatically when VCV Rack is launched. This can be useful
for development or testing.

When using external MIDI clock the analog clock inputs take precedence over the MIDI clock input. Using two kinds of clocks
at the same time or very slow clocks might result in undesired behaviour.

As per the MIDI spec, the MIDI OUT jack sends MIDI timing ticks all the time even when the clock is stopped. This is the
correct way as it lets other devices stay synchronized to the clock so they all start together smoothly.

**Editing:**

- To adjust the tempo middle scroll over the tempo display. To adjust in 0.1 BPM hold shift while scrolling.
- To tap the tempo click on the tempo display.
- To adjust the INT/EXT or AUTOSTART functions click on them
- To adjust the CLOCK OUT divider middle scroll over the display

**Features:**

- Controls for manual RESET and RUN/STOP control
- **RUN IN** jack supports three different behaviours which can be selected by the right click menu.
- **STOP IN** jack always stops the sequencer and can be used along with the RUN IN modes
- **CLOCK IN** accepts 24PPQ clock pulses
- **RESET IN** resets the count and produces a pulse on the **RESET OUT** jack
- **CLOCK OUT** produces an analog clock pulse output that can be divded from 1/1 (24PPQ) down to 1/24 (1PPQ)
- **RESET OUT** produces a pulse when a MIDI start message is received or the clock is reset
- **MIDI IN** and **MIDI OUT** jacks uses the **vMIDI&trade;** patchable MIDI protocol


<br clear="right"/>

----
### MIDI CV
**MIDI to CV Converter**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_CV.png" />

The MIDI CV module allows either note or CC to CV conversion. In note mode either mono or polyphonic
notes can be converted. In the case of mono mode, last-note priority is used. In poly mode up to three
MIDI CV modules can be used with the same MIDI IN signal for up to three voice polyphony.

CC mode allows up to three different CC messages to be mapped to the three output jacks.

**Features:**

Three MIDI to CV modes:

- **MNO** - Mono mode converts notes to pitch, gate and velocity voltages with last-note priority.
- **PLY** - Poly mode allows polyphonic conversion of notes to pitch, gate and velocity voltages by using up to three copies of the MIDI CV module with the same input signal. The voice to output on each module is selected with the VOICE switch.
- **CC** - CC mode allows conversion of CC messages to voltages. Up to 3 CCs can be mapped at once.

Three CV outputs:

- **P1** - Outputs pitch CV (note modes) or a mapped CC (CC mode)
- **G2** - Outputs gate CV (note modes) or a mapped CC (CC mode)
- **V3** - Outputs velocity CV (note modes) or a mapped CC (CC mode)

Notes:

- Velocity output in note modes.
- Support for pitch bend in note modes.
- Support for damper (hold) pedal in note modes.
- MIDI IN jacks uses the **vMIDI&trade;** patchable MIDI protocol

**Using LEARN Mode**

To map a note or CC message press the LEARN button. In note mode the P1 output will flash. Hit a key on your keyboard to map.
To map a CC press the LEARN button to select which output to map. Turn or press the control to generate the CC.

**Setting the Pitch Bend Range**

The pitch bend range in note mode can be set from 1 to 12 semitones. Right click on the module to select the range. The setting
will be saved as part of your patch.

<br clear="right"/>

----
### MIDI Input
**Hardare MIDI Input**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Input.png" />

The MIDI Input module allows you to bring in MIDI from any hardware device to use within
VCV Rack by way of Kilpatrick Audio's **vMIDI&trade;** protocol for patchable
MIDI. Bring in a keyboard or other controller to use with other MIDI modules in this plugin.

<br clear="right"/>

----
### MIDI Mapper
**MIDI CC Mapper**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Mapper.png" />

The MIDI Mapper allows CC messages to be remapped. If you have a control on a keyboard or Virtue controller
and wish to transform it into another CC message needed by a synthesizer, simply pass the signal through the CC mapper
and map the CC to a different number. Up to six mappings can be performed at once. Daisy-chain multiple mappers together
if you require additional mappings. All map settings are stored within the VCV Rack patch.

To enable mapping simply click on the display corresponding to the mapper you wish to use. Turn or press the
control to learn the CC input number. Then use the scroll wheel on your mouse to adjust the output CC number up or down.
The MIDI Mapper does not care about MIDI channels and will map CCs found on any channel.

**Features:**

- MIDI CC mapper with auto-learning function
- Six mappers can be used at the same time
- All jacks use the **vMIDI&trade;** patchable MIDI protocol

<br clear="right"/>

----
### MIDI Merger
**MIDI Stream Merger**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Merger.png" />

If you have multiple MIDI streams and wish to create a single stream combining them together simply use the MIDI Merger
to merge up to three streams together. The output is also filtered to allow convenient access to channel mode messages,
system messages and all messages on three dedicated jacks. SYSEX messages are currently not processed.

**Features:**

- Three input MIDI merger can merge three streams of MIDI together
- Filtered outputs send Channel mode messages, System messages and All messages
- All jacks use the **vMIDI&trade;** patchable MIDI protocol

<br clear="right"/>

----
### MIDI Monitor
**MIDI Monitor Display**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Monitor.png" />

The MIDI Monitor module allows you to check the data on a MIDI stream. It displays the raw MIDI data in
three-byte messages just as they appear on the **vMIDI&trade;** cables within VCV Rack. Up to four inputs
can be monitored at the same time and the input number will be shown on the display. Inputs can be switched
on and off allowing quick checking of different streams of data.

**Features:**

- MIDI Monitor shows raw MIDI message data
- Four input channels with individual on/off controls
- All jacks use the **vMIDI&trade;** patchable MIDI protocol

<br clear="right"/>

----
### MIDI Output
**Hardware MIDI Output**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Output.png" />

The MIDI Output module allows you to send MIDI from VCV Rack to any hardware device by way of
Kilpatrick Audio's **vMIDI&trade;** protocol for patchable MIDI. Control an external synth
or sequencer with MIDI that you generate from within VCV Rack.

**Features:**

- Single Hardware MIDI Output
- MIDI input jack accepts **vMIDI&trade;** patchable MIDI for use with included modules

<br clear="right"/>

----
### MIDI Repeater
**MIDI Repeat Processor**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/MIDI_Repeater.png" />

The MIDI Repeater is a unique module used to manage CC messages from some types of controls such
as knobs and sliders. By default Virtue control modules send knobs and sliders repeatedly. This enables
receivers to instantly know the front panel setting without having to touch all the controls. However in some situations
this might not be what you want. You can use the MIDI Repeater to filter out repeated CC messages with the same
value as the last time. Additionally you can generate repeated CC messages from devices that do not support sending
the same values over and over. This can be helpful when patching MIDI on the fly as the receiving module or device
will be updated with the current values right away. The MIDI Repeater ignores channel numbers and treats all CC
messages globally.

There are three modes you can use with the MIDI Repeater:

- **OFF** - Filter out any repeated CC messages with the same CC and value
- **ON** - Allow repeated CC messages with the same CC and value
- **OFF** - Generate repeated CC messages from a non-repeating source

**Features:**

- Processor for managing CC messages from knobs and sliders
- All jacks use the **vMIDI&trade;** patchable MIDI protocol

<br clear="right"/>

----
### Multi Meter
**Multi-channel and X/Y Meter**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Multi_Meter.png" />

The Multi Meter is useful for viewing a lot of audio signal levels at the same time. It also supports X/Y mode for
viewing phase differences between stereo channels. All meters are peak reading with peak hold.

**Features:**

- **IN L** and **IN R** jacks input the first two channels.
- **MULTI IN** jack accepts up to 16 channels for X/Y or multi-input mode
- **X/Y MODE** displays levels and X/Y plot of the first two input channels
- **MULTI MODE** displays 4, 8 or 16 channel levels

<br clear="right"/>

----
### Quad Decoder
**Quad 4-2-4 Matrix Decoder**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Quad_Decoder.png" />

The Quad Decoder can perform matrix decoding of 4-2-4 matrix quad signals.
Several popular / classic quad formats are supported: QS (Regular Matrix)
and SQ. The QS format seems identical to the encoding performed by the
QUARK quadraphonic DAW plugin. Quad speakers are normally arranged in square
around the room with two speakers in front and two behind the listener.

This plugin can be used to decode 2 channels into 4 channels for playback.
Please see notes about quadraphonic encoding / decoding in the Quad Encoder
section below.

To change the matrix mode right click and select the mode in the menu. Both
QS and SQ formats are supported based on the type of source material you are
playing back.

**Matrix vs. Logic**

Decoding quad signals requires two steps: matrix decoding, and logic control.
Matrix decoding involves phase shifting and mixing to extract and then subtract
signals from each other. Depending on the format this can work fairly well (QS)
or barely at all (SQ) offering between none and 3dB of separation between adjacent
channels.

Logic adds additional *steering* processing that detects the relative volume levels
of channels and uses dyanmic gain control (like a compressor) to adjust the balance
of the various channels to achieve better channel separation. This module contains
experimental logic decoding.

**Subwoofer Output**

As a convenience there is a mono subwoofer output which sums the post-output level
and post-fs/balance channels and then optionally passes them through a 24dB/octave
lowpass filter to produce a mono subwoofer signal. Right click on the module to set
the subwoofer cutoff frequency.

The connections and controls are as follows:

- **OUTPUT** - output level control
- **F/S BALANCE** - front / surround level balance
- **LT IN** - left matrix input
- **RT IN** - right matrix input
- **FL** - surround left output
- **FR** - surround right output
- **SL** - surround left output
- **SR** - surround right output
- **MULTI OUT** - polyphonic cable output carrying five channels:
  - **1** = FL output
  - **2** = FR output
  - **3** = SL output
  - **4** = SR output
  - **5** = SUB output

<br clear="right"/>

----
### Quad Encoder
**Quad 4-2-4 Matrix Encoder**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Quad_Encoder.png" />

The Quad Encoder can perform matrix encoding of 4-2-4 matrix quad signals.
Several popular / classic quad formats are supported: QS (Regular Matrix)
and SQ. The QS format seems identical to the encoding performed by the
QUARK quadraphonic DAW plugin. Quad speakers are normally arranged in square
around the room with two speakers in front and two behind the listener.

This plugin can be used to encode 4 channels into a 2 channel stereo stream
for distribution or recording which can then be decoded back to 4 channels
for playback. Please note that 4-2-4 matrix systems are *lossy* in that there
is some crosstalk between channels after decoding. The amount of separation
after decoding is almost completely dependent on the decoder. An encoded stereo
stream can be played back with normal stereo equipment and sound pretty much
fine. You should definitely monitor your mix with stereo and quad playback
equipment before releasing it.

To change the matrix mode right click and select the mode in the menu. You can
choose QS and SQ mode. Unless you have a reason, you should always use QS mode
as it is technically superior to SQ mode and offers better stereo compatibility
and better separation on decoding.

The connections and controls are as follows:

- **OUTPUT** - matrix output level control
- **FL** - front left input
- **FR** - front right input
- **SL** - surround left input
- **SR** - surround right input
- **MULTI INS** - polyphonic cable inputs carrying four channels:
  - **1** = FL input
  - **2** = FR input
  - **3** = SL input
  - **4** = SR input
- **MATRIX OUT** - LT (Left Total) and RT (Right Total) matrix outputs

<br clear="right"/>

----
### Quad Panner
**Quad Panner with CV Control**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Quad_Panner.png" />

The Quad Panner is a mono input quadraphonic panner that uses a fake on-screen joystick (not that great) or a pair
of X and Y CV inputs. The panning uses a quasi-constant power law whereby the middle position will be 3dB down in each
direction. For instance, full front and centre will be 3dB down in front left and front right outputs. The centre stick
position will be 6dB down in all four channels. This is unlike the basic pan pot law of 6dB down.

You can snap to preset positions such as top left, top centre, top right, etc. (indicated with blue arrows) by holding down
P and clicking around the edge of the joystick circuit. This is particularly useful for alignment to make sure that you are
fully deflected in one direction or another.

The connections and controls are as follows:

- **FL** - front left output
- **FR** - front right output
- **SL** - surround left output
- **SR** - surround right output
- **IN** - signal input
- **X** - X panning CV input - -5V to +5V deflects fully when the stick is in the middle position
- **Y** - Y panning CV input - -5V to +5V deflects fully when the stick is in the middle position
- **MULTI** - polyphonic cable output carrying four channels:
  - **1** = FL
  - **2** = FR
  - **3** = SL
  - **4** = SR
- **RESET** - resets the stick to the centre position

<br clear="right"/>

----
### Stereo Meter
**Stereo Audio Levelmeter**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Stereo_Meter.png" />

The Stereo Meter is a useful tool especially for development. I often want to test signals and read out
exact dB values. The module is a peak reading meter with 0.0dBFS normalized to 10Vpk. (20Vpp) There is a two-pole
high-pass filter set at 10Hz to strip out DC which can ruin the measurement of small signals.

You can adjust the displayed value and set it to a reference level of your choice. Scroll the mouse wheel over
the left or right meter to adjust the reference level up or down. The adjustment will show for a moment before reverting
back to the actual level display. The text will show in red when the reference level has been set to a non-0dB value.

<br clear="right"/>

----
### Test Osc
**Test Oscillator with Sweep**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Test_Osc.png" />

The Test Osc can generate useful test tones for development or alignment purposes. The output can sweep
a sine wave across the audio band and this can be triggered by an external CV trigger signal.

The functions and controls are as follows:

- **LEVEL** - Sets the absolute output level where 0dB equals an output of 10Vpk. (20Vpp)
- **FREQ** - Sets the frequency when in sine wave mode. (not sweeping)
- **SPEED** - Sets the sweep speed for *LOG* or *LIN* mode. The set speed in seconds is shown on the display. A percentage of sweep completion will also be displayed when sweeping.
- **TONE** - Set the output tone to *SINE*, *WH* (white noise), or *PK*. (pink noise) The pink noise source is approximated with a series of low-pass filter. Please check the spectra before using it for high precision tasks.
- **SWEEP** - Sets the sweep mode either *OFF*, *LOG* (log sweep) or *LIN*. (linear sweep)
- **STEP** - Sets the frequency step size for the sine output when using manual frequency setting. Choices are *3RD*, (3rd octave steps), *OCT* (octave steps) and *LOG* which goes by semitone.
- **TRIG** - Button and CV input jack. Starts a sweep or momentarily produces output.
- **ON** - Sets the unit into either *MOM* (momentary) or continuous *ON* mode.

Note that the *REF* level displayed is a convenience feature. You can scroll your middle mouse button over the display to adjust it relative to the ABS value.

<br clear="right"/>
