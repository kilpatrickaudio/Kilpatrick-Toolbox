/*
 * Kilpatrick Audio non-PLL MIDI Clock
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "MidiClock.h"
#include "MidiProtocol.h"

// constructor
MidiClock::MidiClock() {
    handler = NULL;
    runTickCount = 0;
    runState = 0;
    resetf = 0;
    beatf = 0;
}

// register callback handler
void MidiClock::registerHandler(MidiClockHandler *handler) {
    this->handler = handler;
}

// send message to the MIDI clock (from a port)
// this may cause callbacks to be called
void MidiClock::sendMessage(const midi::Message& msg) {
    if(msg.getSize() != 1) {
        return;
    }
    switch(msg.bytes[0]) {
        case MIDI_TIMING_TICK:
            if(runState) {
                beatf = ((runTickCount % MIDI_NATIVE_PPQ) == 0);
                if(handler != NULL) {
                    handler->midiClockTicked(runTickCount, ((runTickCount % MIDI_NATIVE_PPQ) == 0));
                }
                runTickCount ++;
            }
            break;
        case MIDI_CLOCK_START:
            if(!runState) {
                if(handler != NULL) {
                    handler->midiClockRunStateChanged(1, 1);
                }
            }
            else {
                if(handler != NULL) {
                    handler->midiClockPositionReset();
                }
            }
            resetf = 1;
            runState = 1;
            runTickCount = 0;
            break;
        case MIDI_CLOCK_CONTINUE:
            if(!runState) {
                resetf = (runTickCount == 0);
                if(handler != NULL) {
                    handler->midiClockRunStateChanged(1, (runTickCount == 0));
                }
            }
            runState = 1;
            break;
        case MIDI_CLOCK_STOP:
            if(runState) {
                resetf = (runTickCount == 0);
                if(handler != NULL) {
                    handler->midiClockRunStateChanged(0, (runTickCount == 0));
                }
            }
            runState = 0;
            break;
    }
}

// get the run state
int MidiClock::getRunState(void) {
    return runState;
}


// check if we got a beat - clears after reading
int MidiClock::getBeat(void) {
    if(beatf) {
        beatf = 0;
        return 1;
    }
    return 0;
}

// check if we were reset - clears after reading
int MidiClock::getReset(void) {
    if(resetf) {
        resetf = 0;
        return 1;
    }
    return 0;
}

// get the current tick position
int MidiClock::getTickCount(void) {
    return runTickCount;
}
