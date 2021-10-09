/*
 * Kilpatrick Audio MIDI CC Memory
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_CC_MEM_H
#define MIDI_CC_MEM_H

#include "../plugin.hpp"
#include "MidiProtocol.h"

struct MidiCCHist {
    midi::Message msg;
    int timeout;

    MidiCCHist() {
        msg.setSize(0);
        timeout = 0;
    }
};

class MidiCCMem {
private:
    std::vector<MidiCCHist> repeatHist[MIDI_NUM_CHANNELS];
    int timeout;  // the timeout to use for messages
    #define TIMEOUT_DEFAULT 1000

public:
    // constructor
    MidiCCMem();

    // set the timeout in process ticks
    void setTimeout(int timeout);

    // process timeouts
    void process();

    // handle a CC message
    // returns:
    //  - 1 = CC already known with this value
    //  - 0 = CC not known or different value / message type
    int handleCC(midi::Message msg);

    // reset all history
    void reset();
};

#endif
