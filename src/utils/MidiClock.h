/*
 * Kilpatrick Audio non-PLL MIDI Clock
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_CLOCK_H
#define MIDI_CLOCK_H

#include "../plugin.hpp"

// MIDI clock handler
class MidiClockHandler {
public:
    // run state changed
    virtual void midiClockRunStateChanged(int running, int reset) { }

    // clock ticked
    virtual void midiClockTicked(uint32_t tickCount, int beat) { }

    // clock position was reset - only called when clock is stopped
    virtual void midiClockPositionReset(void) { }
};

// MIDI clock
class MidiClock {
private:
    MidiClockHandler *handler;
    int32_t runTickCount;
    int runState;

public:
    // constructor
    MidiClock();

    // register callback handler
    void registerHandler(MidiClockHandler *handler);

    // send message to the MIDI clock (from a port)
    // this may cause callbacks to be called
    void sendMessage(midi::Message msg);
};

#endif
