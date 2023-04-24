/*
 * Kilpatrick Audio DSP Utils 2 - C++ Functions
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Kilpatrick Audio
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "DspUtils2.h"

using namespace dsp2;

//
// LevelSense
//
// set the attack speed in seconds
void LevelSense::setAttack(float speed, float fs) {
    a0Attack = 1.0 - expf(-2.0 * M_PI * (1.0 / speed / fs));
}

// set the release speed in seconds
void LevelSense::setRelease(float speed, float fs) {
    a0Release = 1.0 - expf(-2.0 * M_PI * (1.0 / speed / fs));
}

// run 1-pole lowpass
float LevelSense::process(float in) {
    if(in > z1) {
        return z1 = ((in - z1) * a0Attack) + z1;
    }
    return z1 = ((in - z1) * a0Release) + z1;
}

//
// Filter1Pole
//
// set the cutoff frequency of a 1 pole filter
void Filter1Pole::setCutoff(float freq, float fs) {
    a0 = 1.0 - expf(-2.0 * M_PI * (freq / fs));
}

// run 1-pole lowpass
float Filter1Pole::lowpass(float in) {
    return z1 = ((in - z1) * a0) + z1;
}

// run 1-pole highpass
float Filter1Pole::highpass(float in) {
    z1 = ((in - z1) * a0) + z1;
    return in - z1;
}

// get the most recently computied out
float Filter1Pole::getOutput(void) {
    return z1;
}

//
// Filter2Pole
//
// set the filter cutoff
// type: filter type
// freq: frequency in Hz
// q: Q factor
// gain: gain factor
// fs: audio samplerate in Hz
void Filter2Pole::setCutoff(int type, float freq, float q, float gain, float fs) {
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
float Filter2Pole::process(float in) {
    float out = (in * a0) + z1;
    z1 = (in * a1) + z2 - (out * b1);
    z2 = (in * a2) - (out * b2);
    return out;
}

// get the frequency as a string
std::string Filter2Pole::getFreqStr(void) {
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
std::string Filter2Pole::getGainStr(void) {
    char tempstr[16];
    sprintf(tempstr, "%2.1fdB", factorToDb(gain));
    return tempstr;
}

// get the Q as a string
std::string Filter2Pole::getQStr(void) {
    char tempstr[16];
    sprintf(tempstr, "%2.1f", q);
    return tempstr;
}

//
// Levelmeter
//
// constructor
Levelmeter::Levelmeter() {
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
void Levelmeter::update(float val) {
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
void Levelmeter::onSampleRateChange(void) {
    // XXX fix
    hpf.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
    setSmoothingFreq(smoothingSetting, APP->engine->getSampleRate());
    setPeakHoldTime(peakTimeoutSetting, APP->engine->getSampleRate());
}

// set the smoothing freq cutoff
void Levelmeter::setSmoothingFreq(float freq, float fs) {
    smoothingSetting = freq;
    smoothing = expf(-2.0 * M_PI * (smoothingSetting / fs));
}

// set the peak hold time in seconds
void Levelmeter::setPeakHoldTime(float time, float fs) {
    peakTimeoutSetting = time;
    peakTimeout = (int)roundf(peakTimeoutSetting * fs);
}

// get the current level as field size
// returns a value from 0.0 to 1.0
float Levelmeter::getLevel(void) {
    return hist;
}

// get the level as a dB value
// returns a value from -96.0 to 0.0
float Levelmeter::getDbLevel(void) {
    return clampRange(fieldToDb(hist), -96.0f, 0.0f);
}

// get the current peak level as a field size
// returns a value from 0.0 to 1.0 or 0.0 if no peak is found
float Levelmeter::getPeakLevel(void) {
    if(peakTimeout == 0) {
        return 0.0;
    }
    return peak;
}

// get the current peak level as a dB value
// returns a vlaue fro -96.0 to 0.0 or -96.0 if no peak is found
float Levelmeter::getPeakDbLevel(void) {
    if(peakTimeout == 0) {
        return -96.0;
    }
    return clampRange(fieldToDb(peak), -96.0f, 0.0f);
}

//
// LevelLed
//
// constructor
LevelLed::LevelLed() {
    // XXX fix
//    meter.setSmoothingFreq(10.f, APP->engine->getSampleRate());
}

// call this if the samplerate changes
void LevelLed::onSampleRateChange(void) {
    meter.onSampleRateChange();
}

// update the meter with a cable signal (-10V to +10V) signal
void LevelLed::update(float level) {
    meter.update(level * 0.1);
}

// update the meter with a normalized (-1.0V to +1.0v) signal
void LevelLed::updateNormalized(float level) {
    meter.update(level);
}

// get the brightness
float LevelLed::getBrightness(void) {
    return meter.getLevel();
}

//
// SimpleLFO
//
// constructor
SimpleLFO::SimpleLFO() {
    pa = 0.0f;
    setFrequency(1.0f, 48000.0f);
}

// process a sample - output: -1.0 to +1.0
float SimpleLFO::process(void) {
    pa += freq;
    if(pa > 2.0) pa -= 4.0;
    if(pa > 0.0) {
        return pa * (2.0 - pa);
    }
    return pa * (2.0 + pa);
}

// set the frequency in Hz
void SimpleLFO::setFrequency(float rate, float fs) {
    freq = rate * 4.0f / fs;
}

// set the phase offset - phase offset: -1.0 to +1.0
void SimpleLFO::setPhase(float phase) {
    pa += phase;
    if(pa > 1.0) pa -= 1.0;
    else if(pa < -1.0) pa += 1.0;
}

// phase - a phase shift from 0.0 to 1.0
//   -1.0 = -180 degrees
//    0.0 = 0 degrees
//   +1.0 = +180 degrees
float SimpleLFO::getPhaseShiftedOutput(float phase) {
    float out = pa + (phase * 2.0);
    if(out < -2.0) out += 4.0;
    else if(out > 2.0) out -= 4.0;
    if(out > 0.0) out = out * (2.0 - out);
    else out = out * (2.0 + out);
    return out;
}

//
// DelayMemFloat
//
// constructor - pass a pre-allocated buffer and length
// the length is the number of samples and must be a power of 2
DelayMemFloat::DelayMemFloat(float *buf, int len) {
    delay = buf;
    dlen = len;
    dp = 0;
    preallocated = 1;
}

// constructor - the min len is rounded up to the next
// power of 2 and the memory is allocated
DelayMemFloat::DelayMemFloat(int minLen) {
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
DelayMemFloat::~DelayMemFloat() {
    if(preallocated == 0) {
        free(delay);
    }
}

// clear the memory
void DelayMemFloat::clear(void) {
    int i;
    for(i = 0; i < dlen; i ++) {
        delay[i] = 0.0f;
    }
}

// rotate the memory
void DelayMemFloat::rotate(void) {
    dp = (dp - 1) & (dlen - 1);
}

// read a sample by address no interpolation
// addr: address to read from
float DelayMemFloat::read(int addr) {
    return delay[(dp + addr) & (dlen - 1)];
}

// read a sample by address interpolated
// addr - the address to read from as a float - real = addr, fract = interp
float DelayMemFloat::readFract(float addr) {
    float it1 = addr - (int)addr;
    float out = delay[(dp + (int)addr) & (dlen - 1)] * (1.0 - it1);
    out += delay[(dp + ((int)addr + 1)) & (dlen - 1)] * it1;
    return out;
}

// write into the delay line
// addr - the address to write to
// in - the input var
void DelayMemFloat::write(int addr, float in) {
    delay[(dp + addr) & (dlen - 1)] = in;
}

// inaddr - the address to write to
// outaddr - the address to read from
// feeback - AP feedback coeff - + = alternating sign, - = same sign
// inout - used for input and output
void DelayMemFloat::allpass(int inaddr, int outaddr, float feedback, float *inout) {
    float it1 = delay[(dp + outaddr) & (dlen - 1)];
    *inout += it1 * -feedback;
    delay[(dp + inaddr) & (dlen - 1)] = *inout;
    *inout = (*inout * feedback) + it1;
}

// inaddr - the address to write to
// outaddr - the address to read from as a float - real = addr, fract = interp
// feedback - AP feedback coeff
// acc - used for input and output
void DelayMemFloat::allpassFract(int inaddr, float outaddr, float feedback, float *inout) {
    float it2 = outaddr - (int)outaddr;
    float it1 = delay[(dp + (int)outaddr) & (dlen - 1)] * (1.0 - it2);
    it1 += delay[(dp + ((int)outaddr + 1)) & (dlen - 1)] * it2;
    *inout += (it1 * -feedback);
    delay[(dp + inaddr) & (dlen - 1)] = *inout;
    *inout = (*inout * feedback) + it1;
}

//
// DelayMem16
//
// constructor - pass a pre-allocated buffer and length
// the length is the number of samples and must be a power of 2
DelayMem16::DelayMem16(int16_t *buf, int len) {
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
DelayMem16::DelayMem16(int minLen) {
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
DelayMem16::~DelayMem16() {
    if(preallocated == 0) {
        free(delay);
    }
}

// clear the memory
void DelayMem16::clear(void) {
    int i;
    for(i = 0; i < dlen; i ++) {
        delay[i] = 0;
    }
}

// rotate the memory
void DelayMem16::rotate(void) {
    dp = (dp - 1) & (dlen - 1);
}

// read a sample by address no interpolation
// addr: address to read from
// returns the value in float - range: -1.0f to +1.0f
float DelayMem16::read(int addr) {
    return (float)delay[(dp + addr) & (dlen - 1)] * 0.000030518f;
}
// read a sample by address interpolated
// addr - the address to read from as a float - real = addr, fract = interp
// returns the value in float - range: -1.0f to +1.0f
float DelayMem16::readFract(float addr) {
    float it1 = addr - (int)addr;
    float out = ((float)delay[(dp + (int)addr) & (dlen - 1)] * 0.000030518f) * (1.0 - it1);
    out += ((float)delay[(dp + ((int)addr + 1)) & (dlen - 1)] * 0.000030518f) * it1;
    return out;
}

// write into the delay line
// addr - the address to write to
// in - the input var as a float - range: -1.0f to +1.0f
void DelayMem16::write(int addr, float in) {
    delay[(dp + addr) & (dlen - 1)] = (int16_t)(in * 32768.0f);
}

// inaddr - the address to write to
// outaddr - the address to read from
// feeback - AP feedback coeff - + = alternating sign, - = same sign
// inout - used for input and output
void DelayMem16::allpass(int inaddr, int outaddr, float feedback, float *inout) {
    float it1 = (float)delay[(dp + outaddr) & (dlen - 1)] * 0.000030518f;
    *inout += it1 * -feedback;
    delay[(dp + inaddr) & (dlen - 1)] = (int16_t)(*inout * 32768.0f);
    *inout = (*inout * feedback) + it1;
}

// inaddr - the address to write to
// outaddr - the address to read from as a float - real = addr, fract = interp
// feedback - AP feedback coeff
// acc - used for input and output
void DelayMem16::allpassFract(int inaddr, float outaddr, float feedback, float *inout) {
    float it2 = outaddr - (int)outaddr;
    float it1 = ((float)delay[(dp + (int)outaddr) & (dlen - 1)] * 0.000030518f) * (1.0 - it2);
    it1 += ((float)delay[(dp + ((int)outaddr + 1)) & (dlen - 1)] * 0.000030518f) * it2;
    *inout += (it1 * -feedback);
    delay[(dp + inaddr) & (dlen - 1)] = (int16_t)(*inout * 32768.0f);
    *inout = (*inout * feedback) + it1;
}

//
// AudioBufferer
//
// constructor
AudioBufferer::AudioBufferer(int bufsize, int chans) {
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
AudioBufferer::~AudioBufferer() {
    free(buf);
}

// add an input sample - for single write, bulk read
void AudioBufferer::addInSample(float val) {
    buf[bufCount++] = val;
}

// get an output sample - for bulk write, single read
float AudioBufferer::getOutSample(void) {
    return buf[bufCount++];
}

// get the buffer size in frames
int AudioBufferer::getBufSizeFrames(void) {
    return bufSizeFrames;
}

// get the buffer size in samples
int AudioBufferer::getBufSizeSamps(void) {
    return bufSizeSamps;
}

// get a pointer to the buffer
float *AudioBufferer::getBuf(void) {
    return buf;
}

// check if the inbuf is full and reset if it is
// this must be called even if the return value is not needed
// returns 1 if full, 0 otherwise
int AudioBufferer::isFull(void) {
    if(bufCount >= bufSizeSamps) {
        bufCount = 0;
        return 1;
    }
    return 0;
}

//
// FIRFilter
//
// constructor
// taps - the number of FIR taps
// coeffs - an array of coefficients (copied internally)
FIRFilter::FIRFilter(int taps, float *coeffs) {
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
FIRFilter::~FIRFilter() {
    free(hist);
    free(coeffs);
}

// process a sample and returns next output sample
float FIRFilter::process(float in) {
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

//
// AllpassSection
//
AllpassSection::AllpassSection() {
    out_t2 = 0.0f;
    out_t1 = 0.0f;
    in_t2 = 0.0f;
    in_t1 = 0.0f;
    a2 = 0.0f;
}

void AllpassSection::setCoeff(float a) {
    a2 = a * a;
}

float AllpassSection::process(float in) {
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

//
// AllpassPhaseShifter
//
// constructor
AllpassPhaseShifter::AllpassPhaseShifter() {
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
void AllpassPhaseShifter::process(float in, float *del, float *shift) {
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

//
// FastSineGen
//
// constructor
FastSineGen::FastSineGen() {
    setFreq(1000.0f, 48000.0f);
}

// set the frequency
void FastSineGen::setFreq(float freq, float fs) {
    float w = freq * 2.0f * M_PI / fs;
    b1 = 2.0f * cosf(w);
    y1 = sinf(-w);
    y2 = sinf(-2.0f * w);
}

// get the next sample
float FastSineGen::process(void) {
    y0 = b1 * y1 - y2;
    y2 = y1;
    y1 = y0;
    return y0;
}

//
// NCOGen
//
// constructor
NCOGen::NCOGen() {
    pa = 0;
    setFreq(1000.0f, 48000.0f);
}

// set the frequency
void NCOGen::setFreq(float freq, float fs) {
    this->freq = (int32_t)((freq / fs) * (float)MAXVAL);
}

// get the next ramp sample and increment
// output range is 0.0f to 1.0f
float NCOGen::processRamp(void) {
    pa += freq;
    return (float)(pa & 0x7fffffff) / (float)MAXVAL;
}

// get the next sample as a sine and increment
// output range is -1.0f to 1.0f
float NCOGen::processSine(void) {
    return sinf(processRamp() * M_PI * 2.0f);
}

//
// GoertzelToneDetect
//
// constructor
GoertzelToneDetect::GoertzelToneDetect() {
    setFreq(1000.0f, 0.025f, 48000.0f);
    setThresh(0.25f);
}

// set frequency
void GoertzelToneDetect::setFreq(float freq, float blockTime, float fs) {
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
void GoertzelToneDetect::setThresh(float thresh) {
    this->thresh = thresh;
}

// process a sample and return 1 if tone is detected
int GoertzelToneDetect::process(float sample) {
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
int GoertzelToneDetect::getDetect(void) {
    return detect;
}

// get the detection magnitude
float GoertzelToneDetect::getDetectLevel(void) {
    return detectLevel;
}
