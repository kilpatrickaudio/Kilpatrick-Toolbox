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
    void setAttack(float speed, float fs) {
        a0Attack = 1.0 - expf(-2.0 * M_PI * (1.0 / speed / fs));
    }

    // set the release speed in seconds
    void setRelease(float speed, float fs) {
        a0Release = 1.0 - expf(-2.0 * M_PI * (1.0 / speed / fs));
    }

    // run 1-pole lowpass
    float process(float in) {
        if(in > z1) {
            return z1 = ((in - z1) * a0Attack) + z1;
        }
        return z1 = ((in - z1) * a0Release) + z1;
    }
};


// single pole filter
struct Filter1Pole {
    float a0 = 0.0f;
    float z1 = 0.0f;

    // set the cutoff frequency of a 1 pole filter
    void setCutoff(float freq, float fs) {
        a0 = 1.0 - expf(-2.0 * M_PI * (freq / fs));
    }

    // run 1-pole lowpass
    float lowpass(float in) {
        return z1 = ((in - z1) * a0) + z1;
    }

    // run 1-pole highpass
    float highpass(float in) {
        z1 = ((in - z1) * a0) + z1;
        return in - z1;
    }

    // get the most recently computied out
    float getOutput(void) {
        return z1;
    }
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
    void setCutoff(int type, float freq, float q, float gain, float fs) {
        float norm;
//        float V = pow(10, fabs(gain) / 20.0);
        float V = gain;
        float K = tan(M_PI * freq / fs);
        this->freq = freq;
        this->gain = gain;
        this->q = q;
        switch(type) {
            case TYPE_LPF:
                norm = 1.0 / (1.0 + K / q + K * K);
                a0 = K * K * norm;
                a1 = 2.0 * a0;
                a2 = a0;
                b1 = 2.0 * (K * K - 1.0) * norm;
                b2 = (1.0 - K / q + K * K) * norm;
                break;
            case TYPE_BPF:
                norm = 1.0 / (1.0 + K / q + K * K);
                a0 = K / q * norm;
                a1 = 0.0;
                a2 = -a0;
                b1 = 2.0 * (K * K - 1.0) * norm;
                b2 = (1.0 - K / q + K * K) * norm;
                break;
            case TYPE_HPF:
                norm = 1.0 / (1.0 + K / q + K * K);
                a0 = 1.0 * norm;
                a1 = -2.0 * a0;
                a2 = a0;
                b1 = 2.0 * (K * K - 1.0) * norm;
                b2 = (1.0 - K / q + K * K) * norm;
                break;
            case TYPE_NOTCH:
                norm = 1.0 / (1.0 + K / q + K * K);
                a0 = (1.0 + K * K) * norm;
                a1 = 2.0 * (K * K - 1.0) * norm;
                a2 = a0;
                b1 = a1;
                b2 = (1.0 - K / q + K * K) * norm;
                break;
            case TYPE_PEAK:
                if(gain >= 0.0) {
                    norm = 1.0 / (1.0 + 1.0 / q * K + K * K);
                    a0 = (1.0 + V / q * K + K * K) * norm;
                    a1 = 2.0 * (K * K - 1.0) * norm;
                    a2 = (1.0 - V / q * K + K * K) * norm;
                    b1 = a1;
                    b2 = (1.0 - 1.0 / q * K + K * K) * norm;
                }
                else {
                    norm = 1.0 / (1.0 + V / q * K + K * K);
                    a0 = (1.0 + 1.0 / q * K + K * K) * norm;
                    a1 = 2.0 * (K * K - 1.0) * norm;
                    a2 = (1.0 - 1.0 / q * K + K * K) * norm;
                    b1 = a1;
                    b2 = (1.0 - V / q * K + K * K) * norm;
                }
                break;
            case TYPE_LOWSHELF:
                if(gain >= 0.0) {
                    norm = 1.0 / (1.0 + sqrt(2.0) * K + K * K);
                    a0 = (1.0 + sqrt(2.0 * V) * K + V * K * K) * norm;
                    a1 = 2.0 * (V * K * K - 1.0) * norm;
                    a2 = (1.0 - sqrt(2.0 * V) * K + V * K * K) * norm;
                    b1 = 2.0 * (K * K - 1.0) * norm;
                    b2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
                }
                else {
                    norm = 1.0 / (1.0 + sqrt(2.0 * V) * K + V * K * K);
                    a0 = (1.0 + sqrt(2.0) * K + K * K) * norm;
                    a1 = 2.0 * (K * K - 1.0) * norm;
                    a2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
                    b1 = 2.0 * (V * K * K - 1.0) * norm;
                    b2 = (1.0 - sqrt(2.0 * V) * K + V * K * K) * norm;
                }
                break;
            case TYPE_HIGHSHELF:
                if(gain >= 0.0) {
                    norm = 1.0 / (1.0 + sqrt(2.0) * K + K * K);
                    a0 = (V + sqrt(2.0 * V) * K + K * K) * norm;
                    a1 = 2.0 * (K * K - V) * norm;
                    a2 = (V - sqrt(2.0 * V) * K + K * K) * norm;
                    b1 = 2.0 * (K * K - 1.0) * norm;
                    b2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
                }
                else {
                    norm = 1.0 / (V + sqrt(2.0 * V) * K + K * K);
                    a0 = (1.0 + sqrt(2.0) * K + K * K) * norm;
                    a1 = 2.0 * (K * K - 1.0) * norm;
                    a2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
                    b1 = 2.0 * (K * K - V) * norm;
                    b2 = (V - sqrt(2.0 * V) * K + K * K) * norm;
                }
                break;
        }
        z1 = 0.0f;
        z2 = 0.0f;
    }

    // process a sample
    float process(float in) {
        float out = (in * a0) + z1;
        z1 = (in * a1) + z2 - (out * b1);
        z2 = (in * a2) - (out * b2);
        return out;
    }

    // get the frequency as a string
    std::string getFreqStr(void) {
        char tempstr[16];
        if(freq < 100.0f) {
            sprintf(tempstr, "%2.1fHz", freq);
        }
        else if(freq < 1000.0f) {
            sprintf(tempstr, "%3.0fHz", freq);
        }
        else if(freq < 10000.0f) {
            sprintf(tempstr, "%2.1fkHz", (freq * 0.001f));
        }
        else {
            sprintf(tempstr, "%2.0fkHz", (freq * 0.001f));
        }
        return tempstr;
    }

    // get the gain as a string in dB
    std::string getGainStr(void) {
        char tempstr[16];
        sprintf(tempstr, "%2.1fdB", factorToDb(gain));
        return tempstr;
    }

    // get the Q as a string
    std::string getQStr(void) {
        char tempstr[16];
        sprintf(tempstr, "%2.1f", q);
        return tempstr;
    }
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
    Levelmeter() {
        hist = 0.0f;
        peak = 0.0f;
        smoothingSetting = PEAK_METER_SMOOTHING;
        peakTimeoutSetting = PEAK_METER_PEAK_HOLD_TIME;
        peakHoldTime = 24000;
        peakTimeout = 0;
        useHighpass = 0;  // disable
        onSampleRateChange();
    }

    // update the meter
    void update(float val) {
        if(useHighpass) {
            val = hpf.process(val);
        }
        val = dsp2::abs(val);
        if(val > hist) {
            hist = clamp(val);
            peak = hist;
            peakTimeout = peakHoldTime;
        }
        else {
            hist *= smoothing;
            if(peakTimeout) {
                peakTimeout --;
            }
        }
    }

    // call this if the samplerate changes
    void onSampleRateChange(void) {
        hpf.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        setSmoothingFreq(smoothingSetting, APP->engine->getSampleRate());
        setPeakHoldTime(peakTimeoutSetting, APP->engine->getSampleRate());
    }

    // set the smoothing freq cutoff
    void setSmoothingFreq(float freq, float fs) {
        smoothingSetting = freq;
        smoothing = expf(-2.0 * M_PI * (smoothingSetting / fs));
    }

    // set the peak hold time in seconds
    void setPeakHoldTime(float time, float fs) {
        peakTimeoutSetting = time;
        peakTimeout = (int)roundf(peakTimeoutSetting * fs);
    }

    // get the current level as field size
    // returns a value from 0.0 to 1.0
    float getLevel(void) {
        return hist;
    }

    // get the level as a dB value
    // returns a value from -96.0 to 0.0
    float getDbLevel(void) {
        return clampRange(fieldToDb(hist), -96.0f, 0.0f);
    }

    // get the current peak level as a field size
    // returns a value from 0.0 to 1.0 or 0.0 if no peak is found
    float getPeakLevel(void) {
        if(peakTimeout == 0) {
            return 0.0;
        }
        return peak;
    }

    // get the current peak level as a dB value
    // returns a vlaue fro -96.0 to 0.0 or -96.0 if no peak is found
    float getPeakDbLevel(void) {
        if(peakTimeout == 0) {
            return -96.0;
        }
        return clampRange(fieldToDb(peak), -96.0f, 0.0f);
    }
};

// a form of levelmeter for LEDs
struct LevelLed {
    Levelmeter meter;

    // constructor
    LevelLed() {
        meter.setSmoothingFreq(10.f, APP->engine->getSampleRate());
    }

    // call this if the samplerate changes
    void onSampleRateChange(void) {
        meter.onSampleRateChange();
    }

    // update the meter with a cable signal (-10V to +10V) signal
    void update(float level) {
        meter.update(level * 0.1);
    }

    // update the meter with a normalized (-1.0V to +1.0v) signal
    void updateNormalized(float level) {
        meter.update(level);
    }

    // get the brightness
    float getBrightness(void) {
        return meter.getLevel();
    }
};

// a simple LFO with several modes
struct SimpleLFO {
    float freq;
    float pa;

    // constructor
    SimpleLFO() {
        pa = 0.0f;
        setFrequency(1.0f, 48000.0f);
    }

    // process a sample - output: -1.0 to +1.0
    float process(void) {
        pa += freq;
        if(pa > 2.0) pa -= 4.0;
        if(pa > 0.0) {
            return pa * (2.0 - pa);
        }
        return pa * (2.0 + pa);
    }

    // set the frequency in Hz
    void setFrequency(float rate, float fs) {
        freq = rate * 4.0f / fs;
    }

    // set the phase offset - phase offset: -1.0 to +1.0
    void setPhase(float phase) {
        pa += phase;
        if(pa > 1.0) pa -= 1.0;
        else if(pa < -1.0) pa += 1.0;
    }

    // phase - a phase shift from 0.0 to 1.0
    //   -1.0 = -180 degrees
    //    0.0 = 0 degrees
    //   +1.0 = +180 degrees
    float getPhaseShiftedOutput(float phase) {
        float out = pa + (phase * 2.0);
        if(out < -2.0) out += 4.0;
        else if(out > 2.0) out -= 4.0;
        if(out > 0.0) out = out * (2.0 - out);
        else out = out * (2.0 + out);
        return out;
    }
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
    virtual void allpass(int inaddr, int outaddr, float feedback, float *inout) { }

    // inaddr - the address to write to
    // outaddr - the address to read from as a float - real = addr, fract = interp
    // feedback - AP feedback coeff
    // acc - used for input and output
    virtual void allpassFract(int inaddr, float outaddr, float feedback, float *inout) { }
};

// delay memory with rotating and interpolation
struct DelayMemFloat : DelayMem {
    float *delay;  // delay memory
    int dlen;  // delay memory length (must be a power of 2)
    int dp;  // delay memory pointer
    int preallocated;  // 1 = a preallocated buffer was passed in

    // constructor - pass a pre-allocated buffer and length
    // the length is the number of samples and must be a power of 2
    DelayMemFloat(float *buf, int len) {
        delay = buf;
        dlen = len;
        dp = 0;
        preallocated = 1;
    }

    // constructor - the min len is rounded up to the next
    // power of 2 and the memory is allocated
    DelayMemFloat(int minLen) {
        int i;
        dlen = 1;
        while(dlen < minLen) {
            dlen = dlen << 1;
        }
        delay = (float *)malloc(sizeof(float) * dlen);
        for(i = 0; i < dlen; i ++) {
            delay[i] = 0.0f;
        }
        dp = 0;
        preallocated = 0;
    }

    // destructor
    ~DelayMemFloat() {
        if(preallocated == 0) {
            free(delay);
        }
    }

    // clear the memory
    void clear(void) override {
        int i;
        for(i = 0; i < dlen; i ++) {
            delay[i] = 0.0f;
        }
    }

    // rotate the memory
    void rotate(void) override {
        dp = (dp - 1) & (dlen - 1);
    }

    // read a sample by address no interpolation
    // addr: address to read from
    float read(int addr) override {
        return delay[(dp + addr) & (dlen - 1)];
    }
    // read a sample by address interpolated
    // addr - the address to read from as a float - real = addr, fract = interp
    float readFract(float addr) override {
        float it1 = addr - (int)addr;
        float out = delay[(dp + (int)addr) & (dlen - 1)] * (1.0 - it1);
        out += delay[(dp + ((int)addr + 1)) & (dlen - 1)] * it1;
        return out;
    }

    // write into the delay line
    // addr - the address to write to
    // in - the input var
    void write(int addr, float in) override {
        delay[(dp + addr) & (dlen - 1)] = in;
    }

    // inaddr - the address to write to
    // outaddr - the address to read from
    // feeback - AP feedback coeff - + = alternating sign, - = same sign
    // inout - used for input and output
    void allpass(int inaddr, int outaddr, float feedback, float *inout) override {
        float it1 = delay[(dp + outaddr) & (dlen - 1)];
        *inout += it1 * -feedback;
        delay[(dp + inaddr) & (dlen - 1)] = *inout;
        *inout = (*inout * feedback) + it1;
    }

    // inaddr - the address to write to
    // outaddr - the address to read from as a float - real = addr, fract = interp
    // feedback - AP feedback coeff
    // acc - used for input and output
    void allpassFract(int inaddr, float outaddr, float feedback, float *inout) override {
        float it2 = outaddr - (int)outaddr;
        float it1 = delay[(dp + (int)outaddr) & (dlen - 1)] * (1.0 - it2);
        it1 += delay[(dp + ((int)outaddr + 1)) & (dlen - 1)] * it2;
        *inout += (it1 * -feedback);
        delay[(dp + inaddr) & (dlen - 1)] = *inout;
        *inout = (*inout * feedback) + it1;
    }
};

// delay memory with rotating and interpolation - 16 bit storage
struct DelayMem16 : DelayMem {
    int16_t *delay;  // delay memory

    // constructor - pass a pre-allocated buffer and length
    // the length is the number of samples and must be a power of 2
    DelayMem16(int16_t *buf, int len) {
        int i;
        delay = buf;
        dlen = len;
        for(i = 0; i < dlen; i ++) {
            delay[i] = 0;
        }
        dp = 0;
        preallocated = 1;
    }

    // constructor - the min len is rounded up to the next
    // power of 2 and the memory is allocated
    DelayMem16(int minLen) {
        int i;
        dlen = 1;
        while(dlen < minLen) {
            dlen = dlen << 1;
        }
        delay = (int16_t *)malloc(sizeof(int16_t) * dlen);
        for(i = 0; i < dlen; i ++) {
            delay[i] = 0;
        }
        dp = 0;
        preallocated = 0;
    }

    // destructor
    ~DelayMem16() {
        if(preallocated == 0) {
            free(delay);
        }
    }

    // clear the memory
    void clear(void) override {
        int i;
        for(i = 0; i < dlen; i ++) {
            delay[i] = 0;
        }
    }

    // rotate the memory
    void rotate(void) override {
        dp = (dp - 1) & (dlen - 1);
    }

    // read a sample by address no interpolation
    // addr: address to read from
    // returns the value in float - range: -1.0f to +1.0f
    float read(int addr) override {
        return (float)delay[(dp + addr) & (dlen - 1)] * 0.000030518f;
    }
    // read a sample by address interpolated
    // addr - the address to read from as a float - real = addr, fract = interp
    // returns the value in float - range: -1.0f to +1.0f
    float readFract(float addr) override {
        float it1 = addr - (int)addr;
        float out = ((float)delay[(dp + (int)addr) & (dlen - 1)] * 0.000030518f) * (1.0 - it1);
        out += ((float)delay[(dp + ((int)addr + 1)) & (dlen - 1)] * 0.000030518f) * it1;
        return out;
    }

    // write into the delay line
    // addr - the address to write to
    // in - the input var as a float - range: -1.0f to +1.0f
    void write(int addr, float in) override {
        delay[(dp + addr) & (dlen - 1)] = (int16_t)(in * 32768.0f);
    }

    // inaddr - the address to write to
    // outaddr - the address to read from
    // feeback - AP feedback coeff - + = alternating sign, - = same sign
    // inout - used for input and output
    void allpass(int inaddr, int outaddr, float feedback, float *inout) override {
        float it1 = (float)delay[(dp + outaddr) & (dlen - 1)] * 0.000030518f;
        *inout += it1 * -feedback;
        delay[(dp + inaddr) & (dlen - 1)] = (int16_t)(*inout * 32768.0f);
        *inout = (*inout * feedback) + it1;
    }

    // inaddr - the address to write to
    // outaddr - the address to read from as a float - real = addr, fract = interp
    // feedback - AP feedback coeff
    // acc - used for input and output
    void allpassFract(int inaddr, float outaddr, float feedback, float *inout) override {
        float it2 = outaddr - (int)outaddr;
        float it1 = ((float)delay[(dp + (int)outaddr) & (dlen - 1)] * 0.000030518f) * (1.0 - it2);
        it1 += ((float)delay[(dp + ((int)outaddr + 1)) & (dlen - 1)] * 0.000030518f) * it2;
        *inout += (it1 * -feedback);
        delay[(dp + inaddr) & (dlen - 1)] = (int16_t)(*inout * 32768.0f);
        *inout = (*inout * feedback) + it1;
    }
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
    AudioBufferer(int bufsize, int chans) {
        int i;
        buf = (float *)malloc(sizeof(float) * bufsize * chans);
        for(i = 0; i < bufsize * chans; i ++) {
            buf[i] = 0.0f;
        }
        bufSizeFrames = bufsize;
        bufSizeSamps = bufsize * chans;
        bufCount = 0;
    }

    // destructor
    ~AudioBufferer() {
        free(buf);
    }

    // add an input sample - for single write, bulk read
    void addInSample(float val) {
        buf[bufCount++] = val;
    }

    // get an output sample - for bulk write, single read
    float getOutSample(void) {
        return buf[bufCount++];
    }

    // check if the inbuf is full and reset if it is
    // this must be called even if the return value is not needed
    // returns 1 if full, 0 otherwise
    int isFull(void) {
        if(bufCount >= bufSizeSamps) {
            bufCount = 0;
            return 1;
        }
        return 0;
    }
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
    FIRFilter(int taps, float *coeffs) {
        int i;
        numtaps = taps;
        hist = (float *)malloc(sizeof(float) * numtaps);
        this->coeffs = (float *)malloc(sizeof(float) * numtaps);
        histpos = 0;
        for(i = 0; i < numtaps; i ++) {
            hist[i] = 0.0f;
            this->coeffs[i] = coeffs[i];
        }
    }

    // destructor
    ~FIRFilter() {
        free(hist);
        free(coeffs);
    }

    // process a sample and returns next output sample
    float process(float in) {
        int i, n;
        float sum;

        hist[histpos] = in;
        sum = 0.0;
        n = 0;

        for(i = histpos; i >= 0; i --) {
            sum += coeffs[n++] * hist[i];
        }
        for(i = numtaps - 1; i > histpos; i --) {
            sum += coeffs[n++] * hist[i];
        }
        histpos ++;
        if(histpos == numtaps) histpos = 0;
        return sum;
    }
};

// allpass section
struct AllpassSection {
    float out_t2;
    float out_t1;
    float in_t2;
    float in_t1;
    float a2;

    AllpassSection() {
        out_t2 = 0.0f;
        out_t1 = 0.0f;
        in_t2 = 0.0f;
        in_t1 = 0.0f;
        a2 = 0.0f;
    }

    void setCoeff(float a) {
        a2 = a * a;
    }

    float process(float in) {
        float out;
        // example
        // out(t) = a^2*(in(t) + out(t-2)) - in(t-2)
        out = a2 * (in + out_t2) - in_t2;
        out_t2 = out_t1;
        out_t1 = out;
        in_t2 = in_t1;
        in_t1 = in;
        return out;
    }
};

// mono allpass phase shifter with +90 degree phase shift
struct AllpassPhaseShifter {
    struct AllpassSection pr0, pr1, pr2, pr3;
    struct AllpassSection sh0, sh1, sh2, sh3;
    float pr_del;

    // constructor
    AllpassPhaseShifter() {
        pr_del = 0.0f;

        pr0.setCoeff(0.48660436861367767358);
        pr1.setCoeff(0.88077943527246449484);
        pr2.setCoeff(0.97793125561632343601);
        pr3.setCoeff(0.99767386185073303473);

        sh0.setCoeff(0.16514909355907719801);
        sh1.setCoeff(0.73982901254452670958);
        sh2.setCoeff(0.94794090632917971107);
        sh3.setCoeff(0.99120971270525837227);
    }

    // process a sample and returns next output sample
    // del = delayed, in phase
    // shift = delayed, +90deg. phase shift (early)
    void process(float in, float *del, float *shift) {
        float tempf;

        // phase reference path
        tempf = pr0.process(in);
        tempf = pr1.process(tempf);
        tempf = pr2.process(tempf);
        tempf = pr3.process(tempf);
        *del = pr_del;  // 1 sample delay
        pr_del = tempf;

        // phase shifter +90 path
        tempf = sh0.process(in);
        tempf = sh1.process(tempf);
        tempf = sh2.process(tempf);
        *shift = sh3.process(tempf);
    }
};

// fast sine wave generator based on Z-transform
// https://www.musicdsp.org/en/latest/Synthesis/9-fast-sine-wave-calculation.html
// not very high-quality frequency or phase coherence
struct FastSineGen {
    float y0, y1, y2, b1;

    // constructor
    FastSineGen() {
        setFreq(1000.0f, 48000.0f);
    }

    // set the frequency
    void setFreq(float freq, float fs) {
        float w = freq * 2.0f * M_PI / fs;
        b1 = 2.0f * cosf(w);
        y1 = sinf(-w);
        y2 = sinf(-2.0f * w);
    }

    // get the next sample
    float process(void) {
        y0 = b1 * y1 - y2;
        y2 = y1;
        y1 = y0;
        return y0;
    }
};

// NCO-style generator with precise frequency steps
// high-quality drift-free sines and ramps
struct NCOGen {
    static constexpr uint32_t MAXVAL = 2147483647;
    uint32_t pa;
    uint32_t freq;

    // constructor
    NCOGen() {
        pa = 0;
        setFreq(1000.0f, 48000.0f);
    }

    // set the frequency
    void setFreq(float freq, float fs) {
        this->freq = (int32_t)((freq / fs) * (float)MAXVAL);
    }

    // get the next ramp sample and increment
    // output range is 0.0f to 1.0f
    float processRamp(void) {
        pa += freq;
        return (float)(pa & 0x7fffffff) / (float)MAXVAL;
    }

    // get the next sample as a sine and increment
    // output range is -1.0f to 1.0f
    float processSine(void) {
        return sinf(processRamp() * M_PI * 2.0f);
    }
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
    GoertzelToneDetect() {
        setFreq(1000.0f, 0.025f, 48000.0f);
        setThresh(0.25f);
    }

    // set frequency
    void setFreq(float freq, float blockTime, float fs) {
        n = (int)(fs * blockTime);
        int k = (int)(0.5f + (((float)n * freq) / fs));
        float omega = (2.0f * M_PI * k) / (float)n;
        sine = sinf(omega);
        cosine = cosf(omega);
        coeff = 2.0f * cosine;
        q1 = 0.0f;
        q2 = 0.0f;
        sampCount = 0;
        detect = 0;
    }

    // set the detection threshold
    void setThresh(float thresh) {
        this->thresh = thresh;
    }

    // process a sample and return 1 if tone is detected
    int process(float sample) {
        float q0;
        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;

        sampCount ++;
        if(sampCount == n) {
            float real = (q1 - q2 * cosine);
            float imag = q2 * sine;
            detectLevel = (real * real + imag * imag) / ((float)n * (float)n);
            detectLevel = clampPos(detectLevel * 4.0);  // XXX amplitude fix?
            if(detectLevel > thresh) {
                detect = 1;
            }
            else {
                detect = 0;
            }
            q1 = 0.0f;
            q2 = 0.0f;
            sampCount = 0;
        }
        return detect;
    }

    // get detection state
    int getDetect(void) {
        return detect;
    }

    // get the detection magnitude
    float getDetectLevel(void) {
        return detectLevel;
    }
};

};  // namespace dsp2

#endif
