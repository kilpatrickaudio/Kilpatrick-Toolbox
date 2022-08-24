/*
 * General Portable Utils
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef PUTILS_H
#define PUTILS_H

#include "../plugin.hpp"
#include <string>
#include <vector>

namespace putils {

static constexpr float PUTILS_VSN = (1.0 / 4294967295.0);

// convert a float to int by rounding and clamp
inline int clampftoi(float val, int min, int max) {
    int i = roundf(val);
    if(i < min) return min;
    if(i > max) return max;
    return i;
}

// clamp an int
inline int clamp(int val, int min, int max) {
    if(val < min) return min;
    if(val > max) return max;
    return val;
}

// clamp a float
inline float clampf(float val, float min, float max) {
    if(val < min) return min;
    if(val > max) return max;
    return val;
}

// convert a MIDI value (0x00 to 0x7f) into a float (0.0 to 1.0)
inline float midi2float(int val) {
    return (float)val * 0.007874016f;
}

// convert a float value (0.0 to 1.0) to a MIDI value (0x00 to 0x7f)
inline int float2midi(float val){
    return (int)roundf(val * 127.0f);
}

// digital positive edge detector
struct PosEdgeDetect {
    int oldVal;

    PosEdgeDetect() {
        oldVal = 0;
    }

    // update the value - returns 1 if posedge detected
    int update(int newVal) {
        if(newVal && !oldVal) {
            oldVal = newVal;
            return 1;
        }
        oldVal = newVal;
        return 0;
    }
};

// detect held switch
struct HoldSwDetect {
    int HOLD_TIME = 2000;  // 2000ms at 1ms task rate
    int holdTimeout;

    HoldSwDetect() {
        holdTimeout = 0;
    }

    // update the old - returns 1 if hold expired
    int update(int newVal) {
        if(newVal) {
            holdTimeout ++;
            if(holdTimeout == HOLD_TIME) {
                return 1;
            }
        }
        else {
            holdTimeout = 0;
        }
        return 0;
    }
};

// parameter value change detector
struct ParamChangeDetect {
    float changeAmount;
    float oldVal;
    int force;

    ParamChangeDetect() {
        changeAmount = 0.0001;
        oldVal = 0.0f;
        force = 0;
    }

    // update the value - returns 1 if change detected
    int update(float newVal) {
        int ret = 0;
        if(abs(newVal - oldVal) > changeAmount || force) {
            ret = 1;
            oldVal = newVal;
        }
        force = 0;
        return ret;
    }

    // get the latest changed value
    float getValue(void) {
        return oldVal;
    }

    // update the value and get the direction the parameter changed
    // returns 1 if param moved up or was forced
    // returns -1 if down
    // returns 0 otherwise
    int updateDir(float newVal) {
        int ret = 0;
        if(abs(newVal - oldVal) > changeAmount || force) {
            if(force || newVal > oldVal) {
                ret = 1;
            }
            else {
                ret = -1;
            }
            oldVal = newVal;
        }
        force = 0;
        return ret;
    }

    // force a change - used when resetting to force an update
    void forceChange(void) {
        force = 1;
    }
};

// one-shot pulse generator
struct Pulser {
    int timeout;

    Pulser() {
        timeout = 0;
    }

    // decrement the timeout and return 0 if time elapsed
    int update() {
        if(timeout) {
            timeout --;
        }
        return timeout;
    }
};

//
// string formatting
//
// convert an into an on/off string
std::string onOffToStr(int on);

// convert a float to a string with 1 decimal digit
std::string floatToStr1(float val);

// convert an int to a string
std::string intToStr(int val);

// convert factor to a dB string with no decimal point
// format: "13dB" or "-42dB"
std::string factorToDbStr(float val);

// convert factor to a dB string with 1 decimal point
// format: "13.4dB" or "-42.0dB"
std::string factorToDbStr1(float val);

// convert a value from 0.0 to 1.0 to a percent: "50%"
std::string factorToPercentStr(float val);

// convert a frequency to a string
// format:
// - <1kHz = xxx.yHz
// - >=1kHz - xxxxx.ykHz
std::string freqToStr(float freq);

// possibly truncate the prefix of a string to make it maxlen
std::string truncateStrPrefix(std::string str, int maxlen);

// possibly truncate the suffix of a string to make it maxlen
std::string truncateStrSuffix(std::string str, int maxlen);

// convert a channel count to a string
// return format:
//  - <1 = "n/a"
//  - 1 = "MONO"
//  - 2 = "STEREO"
//  - 3-n = "nCH"
std::string channelCountToStr(int channels);

// convert an audio samplerate to a string
// detects common rates
std::string samplerateToStr(int samplerate);

// format a time position used for a recording playback display
// posFrames - the current position in the playback
// lenFrames - the total recording length in frames
// samplerate - the recording samplerate
// returns a string formatted like: "0:00:00 / 0:04:33"
std::string playbackTimeStr(int posFrames,
    int lenFrames, int samplerate);

// format a string the right way
inline std::string format(std::string format, ...) {
    va_list args;
    va_start (args, format);
    size_t len = std::vsnprintf(NULL, 0, format.c_str(), args);
    va_end (args);
    std::vector<char> vec(len + 1);
    va_start (args, format);
    std::vsnprintf(&vec[0], len + 1, format.c_str(), args);
    va_end (args);
    return &vec[0];
}

};

#endif
