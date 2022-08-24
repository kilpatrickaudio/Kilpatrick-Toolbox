/*
 * General Portable Utils
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "PUtils.h"
#include "DspUtils2.h"

namespace putils {

//
// string formatting
//
// convert an into an on/off string
std::string onOffToStr(int on) {
    if(on) {
        return "ON";
    }
    return "OFF";
}

// convert a float to a string with 1 decimal digit
std::string floatToStr1(float val) {
    char tempstr[64];
    sprintf(tempstr, "%.1f", val);
    return tempstr;
}

// convert an into a string
std::string intToStr(int val) {
    char tempstr[64];
    sprintf(tempstr, "%d", val);
    return tempstr;
}

// convert factor to a dB string with no decimal point
// format: "13dB" or "-42dB"
std::string factorToDbStr(float val) {
    char tempstr[64];
//    float tempf = roundf(20.0f * log10f(val + PUTILS_VSN));
    float tempf = roundf(dsp2::factorToDb(val));
    if(tempf < -96.0f) {
        return "-inf dB";
    }
    if(fabs(tempf) < 0.5f) {
        tempf = 0.0f;
    }
    sprintf(tempstr, "%2.0fdB", tempf);
    return tempstr;
}

// convert factor to a dB string with 1 decimal point
// format: "13.4dB" or "-42.0dB"
std::string factorToDbStr1(float val) {
    char tempstr[64];
//    float tempf = 20.0f * log10f(val + PUTILS_VSN);
    float tempf = dsp2::factorToDb(val);
    if(fabs(tempf) < 0.05f) {
        tempf = 0.0f;
    }
    sprintf(tempstr, "%2.1fdB", tempf);
    return tempstr;
}

// convert a value from 0.0 to 1.0 to a percent: "50%"
std::string factorToPercentStr(float val) {
    char tempstr[64];
    sprintf(tempstr, "%3.0f%%", (val * 100.0f));
    return tempstr;
}

// convert a frequency to a string
// format:
// - <1kHz =  "xxx.yHz"
// - >=1kHz = " xx.yyykHz"
std::string freqToStr(float freq) {
    char tempstr[64];
    if(freq < 1000.0f) {
        sprintf(tempstr, "%3.1fHz", freq);
    }
    else {
        sprintf(tempstr, " %2.3fkHz", (freq * .001f));
    }
    return tempstr;
}

// possibly truncate the prefix of a string to make it maxlen
std::string truncateStrPrefix(std::string str, int maxlen) {
    if(str.length() > (unsigned int)maxlen) {
        return str.substr(str.length() - maxlen, -1);
    }
    return str;
}

// possibly truncate the suffix of a string to make it maxlen
std::string truncateStrSuffix(std::string str, int maxlen) {
    if(str.length() > (unsigned int)maxlen) {
        return str.substr(0, maxlen);
    }
    return str;
}

// convert a channel count to a string
// return format:
//  - <1 = "n/a"
//  - 1 = "MONO"
//  - 2 = "STEREO"
//  - 3-n = "nCH"
std::string channelCountToStr(int channels) {
    if(channels < 1) {
        return "n/a";
    }
    switch(channels) {
        case 1:
            return "MONO";
        case 2:
            return "STEREO";
    }
    return format("%dCH", channels);
}

// convert an audio samplerate to a string
// detects common rates
std::string samplerateToStr(int samplerate) {
    switch(samplerate) {
        case 8000:
            return "8K";
        case 11025:
            return "11.025K";
        case 22050:
            return "22.050K";
        case 32000:
            return "32K";
        case 44100:
            return "44.1K";
        case 48000:
            return "48K";
        case 88200:
            return "88.2K";
        case 96000:
            return "96K";
        case 176400:
            return "176.4K";
        case 192000:
            return "192K";
    }
    return format("%.3fK", ((float)samplerate / 1000.0f));
}

// format a time position used for a recording playback display
// posFrames - the current position in the playback
// lenFrames - the total recording length in frames
// samplerate - the recording samplerate
// returns a string formatted like: "0:00:00 / 0:04:33"
std::string playbackTimeStr(int posFrames,
        int lenFrames, int samplerate) {
    int pHour, pMin, pSec;
    int lHour, lMin, lSec;

    pSec = posFrames / samplerate;
    pMin = pSec / 60;
    pHour = pMin / 60;
    pSec = pSec % 60;
    pMin = pMin % 60;

    lSec = lenFrames / samplerate;
    lMin = lSec / 60;
    lHour = lMin / 60;
    lSec = lSec % 60;
    lMin = lMin % 60;

    return format("%01d:%02d:%02d / %01d:%02d:%02d",
        pHour, pMin, pSec, lHour, lMin, lSec);
}

};
