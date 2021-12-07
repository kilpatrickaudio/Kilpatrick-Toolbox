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
    // state we can check
    int resetf;  // clears after reading
    int beatf;  // clears after reading

public:
    // constructor
    MidiClock();

    // register callback handler
    void registerHandler(MidiClockHandler *handler);

    // send message to the MIDI clock (from a port)
    // this may cause callbacks to be called
    void sendMessage(const midi::Message& msg);

    // get the run state
    int getRunState(void);

    // check if we got a beat - clears after reading
    int getBeat(void);

    // check if we were reset - clears after reading
    int getReset(void);

    // get the current tick position
    int getTickCount(void);
};

#endif
