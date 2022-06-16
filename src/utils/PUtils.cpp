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
    if(str.length() > maxlen) {
        return str.substr(str.length() - maxlen, -1);
    }
    return str;
}

};
