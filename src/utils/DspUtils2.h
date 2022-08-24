/*
 * Kilpatrick Audio DSP Utils 2 - C++ Functions
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Kilpatrick Audio
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef DSP_UTILS2_H
#define DSP_UTILS2_H

#include <math.h>
#include <string>
#include <vector>
#include "PLog.h"

// portable PC-centric C++ here - no VCV functions
namespace dsp2 {

// very small number
static constexpr float DSP_VSN = (1.0 / 4294967295.0);

// absolute value
inline float abs(float x) {
    if(x < 0.0f) return -x;
    return x;
}

// max value
inline float max(float a, float b) {
    if(a > b) return a;
    return b;
}

// convert a factor to a dB value - 1.0 = 0dB (field size)
inline float fieldToDb(float val) {
    return 20.0 * log10f(val + DSP_VSN);
}

// convert a pitch in Hz into a CV
// 0V = middle C
inline float pitchToVoltage(float pitch) {
    return log2(pitch / 261.63f);  // convert Hz to voltage
}

// clamp a value to between -1.0 and +1.0
inline float clamp(float val) {
    if(val > 1.0f) return 1.0f;
    if(val < -1.0f) return -1.0f;
    return val;
}

// clamp a value to between 0.0 and +1.0
inline float clampPos(float val) {
    if(val > 1.0f) return 1.0f;
    if(val < 0.0f) return 0.0f;
    return val;
}

// clamp a value to a specific range
inline float clampRange(float val, float min, float max) {
    if(val > max) return max;
    if(val < min) return min;
    return val;
}

// scale a value between 0.0 and 1.0 to within a range
inline float scaleRange(float val, float min, float max) {
    return (val * (max - min)) + min;
}

// convert a factor (0.0 to 1.0) to a clamped percent
inline float factorToPercent(float factor) {
    return clampPos(factor) * 100.0f;
}

// convert a factor (0.0 to 1.0) to a clamped percent as int
inline int factorToPercentInt(float factor) {
    return (int)roundf(clampPos(factor) * 100.0f);
}

// convert a factor to a dB - 1.0 = 0dB field size
inline float factorToDb(float val) {
    return 20.0f * log10f(val + DSP_VSN);
}

// convert a dB to a factor - 0.0dB = 1.0
inline float dbToFactor(float val) {
    return powf(10.0f, val / 20.0f);
}

// map a value of 0.0 to 1.0 to a frequency from 20Hz to 20480Hz
inline float freqRange(float val) {
    if(val < 0.0) {
        return 20.0;
    }
    if(val > 1.0) {
        return 20480.0;
    }
    // power of 10.0 is a special number only for this range
    return powf(2, val * 10.0) * 20.0;
}

// map a value of 0.0 to 1.0 to a frequency range
inline float freqRangeRange(float val, float min, float max) {
    if(val <= 0.0) {
        return min;
    }
    if(val >= 1.0) {
        return max;
    }
    return powf(2, val * log2(max / min)) * min;
}

// adjust an EQ value in quantized steps - set change
// to -1.0 or +1.0 * number of steps to take
inline float adjustEqValue(float currentVal, float change) {
    int val;
    // <100Hz - 1Hz step
    if(currentVal < 100.0f) {
        val = (int)currentVal + (int)change;
        return (float)val;
    }
    // 100-999.9Hz - 10Hs step
    else if(currentVal < 1000.0f) {
        val = (int)currentVal + (int)(change * 10.0f);
        return (float)val;
    }
    // <10kHz - 100Hz step
    else if(currentVal < 10000.0f) {
        val = (int)currentVal + (int)(change * 100.0f);
        return (float)val;
    }
    // >= 10kHz - 1kHz step
    val = (int)currentVal + (int)(change * 1000.0f);
    return (float)val;
}

// adjust a factor by a dB change value
// change: the + or - dB change value
inline float adjustFactorByDb(float currentVal, float change) {
    return dbToFactor(factorToDb(currentVal) + change);
}

// adjust a time factor
// change: -1.0 or +1.0 * number of steps to take
inline float adjustTimeFactor(float currentVal, float change) {
    int ms;
    // <100ms - 1ms step
    if(currentVal < 0.1f) {
        ms = (int)(currentVal * 1000.0f) + (int)change;
        return (float)ms * 0.001f;
    }
    // <1s - 10ms step
    else if(currentVal < 1.0f) {
        ms = (int)(currentVal * 1000.0f) + (int)(change * 10.0f);
        return (float)ms * 0.001f;
    }
    // <10s - 100ms step
    else if(currentVal < 10.0f) {
        ms = (int)(currentVal * 1000.0f) + (int)(change * 100.0f);
        return (float)ms * 0.001f;
    }
    // >= 10s - 1s step
    ms = (int)(currentVal * 1000.0f) + (int)(change * 1000.0f);
    return (float)ms * 0.001f;
}

// adjust a time factor
// change: -1.0 or +1.0 * number of steps to take
inline float adjustTimeFactorCoarse(float currentVal, float change) {
    int ms;
    // <10s - 100ms step
    if(currentVal < 10.0f) {
        ms = (int)(currentVal * 1000.0f) + (int)(change * 100.0f);
        return (float)ms * 0.001f;
    }
    // >= 10s - 1s step
    ms = (int)(currentVal * 1000.0f) + (int)(change * 1000.0f);
    return (float)ms * 0.001f;
}

// a level sense structured like a 1 pole LPF
struct LevelSense {
    float a0Attack = 0.0f;
    float a0Release = 0.0f;
    float z1 = 0.0f;

    // set the attack speed in seconds
    void setAttack(float speed, float fs);

    // set the release speed in seconds
    void setRelease(float speed, float fs);

    // run 1-pole lowpass
    float process(float in);
};

// single pole filter
struct Filter1Pole {
    float a0 = 0.0f;
    float z1 = 0.0f;

    // set the cutoff frequency of a 1 pole filter
    void setCutoff(float freq, float fs);

    // run 1-pole lowpass
    float lowpass(float in);

    // run 1-pole highpass
    float highpass(float in);

    // get the most recently computied out
    float getOutput(void);
};

// two pole filter
struct Filter2Pole {
    float a0 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
    // values for string
    float freq = 0.0f;
    float gain = 0.0f;
    float q = 0.0f;
    // filter type
    enum {
        TYPE_LPF,
        TYPE_BPF,
        TYPE_HPF,
        TYPE_NOTCH,
        TYPE_PEAK,
        TYPE_LOWSHELF,
        TYPE_HIGHSHELF
    };

    // set the filter cutoff
    // type: filter type
    // freq: frequency in Hz
    // q: Q factor
    // gain: gain factor
    // fs: audio samplerate in Hz
    void setCutoff(int type, float freq, float q,
        float gain, float fs);

    // process a sample
    float process(float in);

    // get the frequency as a string
    std::string getFreqStr(void);

    // get the gain as a string in dB
    std::string getGainStr(void);

    // get the Q as a string
    std::string getQStr(void);
};

// levelmeter with peak hold
struct Levelmeter {
    float hist;
    float peak;
    int peakHoldTime;
    float smoothingSetting;  // user setting
    float peakTimeoutSetting;  // user setting
    float smoothing;  // coeff
    int peakTimeout;  // coeff
    Filter2Pole hpf;  // optional high pass filter
    int useHighpass;  // set to 1 to pass signal through 10Hz highpass filter
    static constexpr float PEAK_METER_SMOOTHING = 1.0f;
    static constexpr float PEAK_METER_PEAK_HOLD_TIME = 1.0f;

    // constructor
    Levelmeter();

    // update the meter
    void update(float val);

    // call this if the samplerate changes
    void onSampleRateChange(void);

    // set the smoothing freq cutoff
    void setSmoothingFreq(float freq, float fs);

    // set the peak hold time in seconds
    void setPeakHoldTime(float time, float fs);

    // get the current level as field size
    // returns a value from 0.0 to 1.0
    float getLevel(void);

    // get the level as a dB value
    // returns a value from -96.0 to 0.0
    float getDbLevel(void);

    // get the current peak level as a field size
    // returns a value from 0.0 to 1.0 or 0.0 if no peak is found
    float getPeakLevel(void);

    // get the current peak level as a dB value
    // returns a vlaue fro -96.0 to 0.0 or -96.0 if no peak is found
    float getPeakDbLevel(void);
};

// a form of levelmeter for LEDs
struct LevelLed {
    Levelmeter meter;

    // constructor
    LevelLed();

    // call this if the samplerate changes
    void onSampleRateChange(void);

    // update the meter with a cable signal (-10V to +10V) signal
    void update(float level);

    // update the meter with a normalized (-1.0V to +1.0v) signal
    void updateNormalized(float level);

    // get the brightness
    float getBrightness(void);
};

// a simple LFO with several modes
struct SimpleLFO {
    float freq;
    float pa;

    // constructor
    SimpleLFO();

    // process a sample - output: -1.0 to +1.0
    float process(void);

    // set the frequency in Hz
    void setFrequency(float rate, float fs);

    // set the phase offset - phase offset: -1.0 to +1.0
    void setPhase(float phase);

    // phase - a phase shift from 0.0 to 1.0
    //   -1.0 = -180 degrees
    //    0.0 = 0 degrees
    //   +1.0 = +180 degrees
    float getPhaseShiftedOutput(float phase);
};

// delay memory with rotating and interpolation
struct DelayMem {
    int dlen;  // delay memory length (must be a power of 2)
    int dp;  // delay memory pointer
    int preallocated;  // 1 = a preallocated buffer was passed in

    // constructor - pass a pre-allocated buffer and length
    DelayMem() {
        dlen = 0;
        dp = 0;
        preallocated = 0;
    }

    // destructor
    virtual ~DelayMem() { }

    // clear the memory
    virtual void clear(void) { }

    // rotate the memory
    virtual void rotate(void) { }

    // read a sample by address no interpolation
    // addr: address to read from
    virtual float read(int addr) { return 0.0f; }

    // read a sample by address interpolated
    // addr - the address to read from as a float - real = addr, fract = interp
    virtual float readFract(float addr) { return 0.0f; }

    // write into the delay line
    // addr - the address to write to
    // in - the input var
    virtual void write(int addr, float in) { }

    // inaddr - the address to write to
    // outaddr - the address to read from
    // feeback - AP feedback coeff - + = alternating sign, - = same sign
    // inout - used for input and output
    virtual void allpass(int inaddr, int outaddr,
        float feedback, float *inout) { }

    // inaddr - the address to write to
    // outaddr - the address to read from as a float - real = addr, fract = interp
    // feedback - AP feedback coeff
    // acc - used for input and output
    virtual void allpassFract(int inaddr, float outaddr,
        float feedback, float *inout) { }
};

// delay memory with rotating and interpolation
struct DelayMemFloat : DelayMem {
    float *delay;  // delay memory
    int dlen;  // delay memory length (must be a power of 2)
    int dp;  // delay memory pointer
    int preallocated;  // 1 = a preallocated buffer was passed in

    // constructor - pass a pre-allocated buffer and length
    // the length is the number of samples and must be a power of 2
    DelayMemFloat(float *buf, int len);

    // constructor - the min len is rounded up to the next
    // power of 2 and the memory is allocated
    DelayMemFloat(int minLen);

    // destructor
    ~DelayMemFloat();

    // clear the memory
    void clear(void) override;

    // rotate the memory
    void rotate(void) override;

    // read a sample by address no interpolation
    // addr: address to read from
    float read(int addr) override;

    // read a sample by address interpolated
    // addr - the address to read from as a float - real = addr, fract = interp
    float readFract(float addr) override;

    // write into the delay line
    // addr - the address to write to
    // in - the input var
    void write(int addr, float in) override;

    // inaddr - the address to write to
    // outaddr - the address to read from
    // feeback - AP feedback coeff - + = alternating sign, - = same sign
    // inout - used for input and output
    void allpass(int inaddr, int outaddr,
        float feedback, float *inout) override;

    // inaddr - the address to write to
    // outaddr - the address to read from as a float - real = addr, fract = interp
    // feedback - AP feedback coeff
    // acc - used for input and output
    void allpassFract(int inaddr, float outaddr,
        float feedback, float *inout) override;
};

// delay memory with rotating and interpolation - 16 bit storage
struct DelayMem16 : DelayMem {
    int16_t *delay;  // delay memory

    // constructor - pass a pre-allocated buffer and length
    // the length is the number of samples and must be a power of 2
    DelayMem16(int16_t *buf, int len);

    // constructor - the min len is rounded up to the next
    // power of 2 and the memory is allocated
    DelayMem16(int minLen);

    // destructor
    ~DelayMem16();

    // clear the memory
    void clear(void) override;

    // rotate the memory
    void rotate(void) override;

    // read a sample by address no interpolation
    // addr: address to read from
    // returns the value in float - range: -1.0f to +1.0f
    float read(int addr) override;

    // read a sample by address interpolated
    // addr - the address to read from as a float - real = addr, fract = interp
    // returns the value in float - range: -1.0f to +1.0f
    float readFract(float addr) override;

    // write into the delay line
    // addr - the address to write to
    // in - the input var as a float - range: -1.0f to +1.0f
    void write(int addr, float in) override;

    // inaddr - the address to write to
    // outaddr - the address to read from
    // feeback - AP feedback coeff - + = alternating sign, - = same sign
    // inout - used for input and output
    void allpass(int inaddr, int outaddr,
        float feedback, float *inout) override;

    // inaddr - the address to write to
    // outaddr - the address to read from as a float - real = addr, fract = interp
    // feedback - AP feedback coeff
    // acc - used for input and output
    void allpassFract(int inaddr, float outaddr,
        float feedback, float *inout) override;
};

// audio bufferer - can be used for input or output
// for input: add samples one at a time and then read the buf directly
// for output: write to buf director, then read sample by sample
// convert to/from single samples to buffer blocks
struct AudioBufferer {
    float *buf = NULL;
    int bufCount;
    int bufSizeFrames;
    int bufSizeSamps;

    // constructor
    AudioBufferer(int bufsize, int chans);

    // destructor
    ~AudioBufferer();

    // add an input sample - for single write, bulk read
    void addInSample(float val);

    // get an output sample - for bulk write, single read
    float getOutSample(void);

    // get the buffer size in frames
    int getBufSizeFrames(void);

    // get the buffer size in samples
    int getBufSizeSamps(void);

    // check if the inbuf is full and reset if it is
    // this must be called even if the return value is not needed
    // returns 1 if full, 0 otherwise
    int isFull(void);

    // get a pointer to the buffer
    float *getBuf(void);
};

// mono FIR filter
struct FIRFilter {
    float *hist;
    float *coeffs;
    int histpos;
    int numtaps;

    // constructor
    // taps - the number of FIR taps
    // coeffs - an array of coefficients (copied internally)
    FIRFilter(int taps, float *coeffs);

    // destructor
    ~FIRFilter();

    // process a sample and returns next output sample
    float process(float in);
};

// allpass section
struct AllpassSection {
    float out_t2;
    float out_t1;
    float in_t2;
    float in_t1;
    float a2;

    AllpassSection();

    void setCoeff(float a);

    float process(float in);
};

// mono allpass phase shifter with +90 degree phase shift
struct AllpassPhaseShifter {
    struct AllpassSection pr0, pr1, pr2, pr3;
    struct AllpassSection sh0, sh1, sh2, sh3;
    float pr_del;

    // constructor
    AllpassPhaseShifter();

    // process a sample and returns next output sample
    // del = delayed, in phase
    // shift = delayed, +90deg. phase shift (early)
    void process(float in, float *del, float *shift);
};

// fast sine wave generator based on Z-transform
// https://www.musicdsp.org/en/latest/Synthesis/9-fast-sine-wave-calculation.html
// not very high-quality frequency or phase coherence
struct FastSineGen {
    float y0, y1, y2, b1;

    // constructor
    FastSineGen();

    // set the frequency
    void setFreq(float freq, float fs);

    // get the next sample
    float process(void);
};

// NCO-style generator with precise frequency steps
// high-quality drift-free sines and ramps
struct NCOGen {
    static constexpr uint32_t MAXVAL = 2147483647;
    uint32_t pa;
    uint32_t freq;

    // constructor
    NCOGen();

    // set the frequency
    void setFreq(float freq, float fs);

    // get the next ramp sample and increment
    // output range is 0.0f to 1.0f
    float processRamp(void);

    // get the next sample as a sine and increment
    // output range is -1.0f to 1.0f
    float processSine(void);
};

// Goertzel tone detection
struct GoertzelToneDetect {
    int n;
    float coeff;
    float sine, cosine;
    float q1, q2;
    int sampCount;
    int detect;
    float detectLevel;  // normalized detect level
    float thresh;  // thresh level from 0.0 to 1.0

    // constructor
    GoertzelToneDetect();

    // set frequency
    void setFreq(float freq, float blockTime, float fs);

    // set the detection threshold
    void setThresh(float thresh);

    // process a sample and return 1 if tone is detected
    int process(float sample);

    // get detection state
    int getDetect(void);

    // get the detection magnitude
    float getDetectLevel(void);
};

};  // namespace dsp2

#endif
