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
}

// register callback handler
void MidiClock::registerHandler(MidiClockHandler *handler) {
    this->handler = handler;
}

// send message to the MIDI clock (from a port)
// this may cause callbacks to be called
void MidiClock::sendMessage(midi::Message msg) {
    if(msg.getSize() != 1) {
        return;
    }
    if(handler == NULL) {
        return;
    }

    switch(msg.bytes[0]) {
        case MIDI_TIMING_TICK:
            if(runState) {
                handler->midiClockTicked(runTickCount, ((runTickCount % MIDI_NATIVE_PPQ) == 0));
                runTickCount ++;
            }
            break;
        case MIDI_CLOCK_START:
            if(!runState) {
                handler->midiClockRunStateChanged(1, 1);
            }
            else {
                handler->midiClockPositionReset();
            }
            runState = 1;
            runTickCount = 0;
            break;
        case MIDI_CLOCK_CONTINUE:
            if(!runState) {
                handler->midiClockRunStateChanged(1, (runTickCount == 0));
            }
            runState = 1;
            break;
        case MIDI_CLOCK_STOP:
            if(runState) {
                handler->midiClockRunStateChanged(0, (runTickCount == 0));
            }
            runState = 0;
            break;
    }
}
