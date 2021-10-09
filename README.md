# Kilpatrick-Toolbox

This repo contains Dintree Virtual Rack Modules for
[VCV Rack Virtual Eurorack DAW](https://vcvrack.com). Dintree is
the name of DIY open-source modules created by Andrew Kilpatrick of
Kilpatrick Audio. Please visit the links below for information about Andrew
and his DIY and commercial music and audio projects.

**Useful links:**

* [VCV Rack Virtual Eurorack DAW](https://vcvrack.com) - VCV Rack is what this code runs on
* [Kilpatrick Audio](https://www.kilpatrickaudio.com) - Commercial music products from Kilpatrick Audio
* [Andrew Kilpatrick](https://www.andrewkilpatrick.org) - Creator of these modules

## Licence

All code in this repository is licensed under GPL 3.0 or later. All panel designs, graphics and
logos are copyright Kilpatrick Audio and may not be redistributed without permission.

## Modules

### Stereo Meter
**Stereo Audio Levelmeter**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Stereo_Meter.png" />

The Stereo Meter is a useful tool especially for development. I often wanted to test signals and read out
exact dB values. The module is a peak reading meter with 0.0dBFS normalized to 10Vpk. (20Vpp) There is a two-pole
high-pass filter set at 10Hz to strip out DC which can ruin the measurement of small signals.

<pre>




















</pre>

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

----
### Quad Panner
**Quad Panner with CV Control**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Quad_Panner.png" />

The Quad Panner is a mono input quadraphonic panner that uses a fake on-screen joystick (not that great) or a pair
of X and Y CV inputs. The panning uses a quasi-constant power law whereby the middle position will be 3dB down in each
direction. For instance, full front and centre will be 3dB down in front left and front right outputs. The centre stick
position will be 6dB down in all four channels. This is unlike the basic pan pot law of 6dB down.

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

----
### Quad Encoder
**Quad 4-2-4 Matrix Encoder**

<img align="right" src="https://github.com/kilpatrickaudio/Kilpatrick-Toolbox/raw/master/res/images/Quad_Encoder.png" />
