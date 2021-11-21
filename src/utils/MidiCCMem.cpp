/*
 * Kilpatrick Audio MIDI CC Memory
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "MidiCCMem.h"

// constructor
MidiCCMem::MidiCCMem() {
    timeout = TIMEOUT_DEFAULT;
    reset();
}

// set the timeout in process ticks
void MidiCCMem::setTimeout(int timeout) {
    this->timeout = timeout;
}

// process timeouts
void MidiCCMem::process() {
    std::vector<MidiCCHist>::iterator iter;
    int i;
    // check each channel list
    for(i = 0; i < MIDI_NUM_CHANNELS; i ++) {
        // time out messages for channel
        iter = repeatHist[i].begin();
        while(iter != repeatHist[i].end()) {
            if(iter->timeout > 0) {
                iter->timeout --;
                // timed out
                if(iter->timeout == 0) {
                    iter = repeatHist[i].erase(iter);
                    continue;
                }
            }
            ++iter;
        }
    }
}

// handle a CC message
// returns:
//  - 1 = CC already known with this value
//  - 0 = CC not known or different value / message type
int MidiCCMem::handleCC(const midi::Message& msg) {
    MidiCCHist hist;
    int chan;
    std::vector<MidiCCHist>::iterator iter;
    if((msg.bytes[0] & 0xf0) != MIDI_CONTROL_CHANGE) {
        return 0;
    }
    chan = msg.bytes[0] & 0x0f;
    // see if we already have this CC
    for(iter = repeatHist[chan].begin(); iter != repeatHist[chan].end(); iter ++) {
        // CC matches matches for this channel
        if(iter->msg.bytes[1] == msg.bytes[1]) {
            // value matches
            if(iter->msg.bytes[2] == msg.bytes[2]) {
                iter->timeout = timeout;  // reset timeout
                return 1;  // already know this value
            }
            // value doesn't match
            iter->msg = msg;  // update stored message
            iter->timeout = timeout;  // reset timeout
            return 0;
        }
    }
    // CC not found in list
    hist.msg = msg;
    hist.timeout = timeout;
    repeatHist[chan].push_back(hist);
    return 0;
}

// reset all history
void MidiCCMem::reset() {
    int i;
    for(i = 0; i < MIDI_NUM_CHANNELS; i ++) {
        repeatHist[i].clear();
    }
}
