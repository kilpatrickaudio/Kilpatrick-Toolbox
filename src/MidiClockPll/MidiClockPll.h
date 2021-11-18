/*
 * MIDI Clock with PLL
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Kilpatrick Audio
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_CLOCK_PLL_H
#define MIDI_CLOCK_PLL_H

#include "../plugin.hpp"
#include "../utils/MidiProtocol.h"

// callback handlers for MIDI clock events
class MidiClockPllHandler {
public:
    // a beat was crossed
    virtual void midiClockBeatCrossed(void) { }

    // run state changed
    virtual void midiClockRunStateChanged(int running) { }

    // clock source changed
    virtual void midiClockSourceChanged(int source) { }

    // tap tempo locked
    virtual void midiClockTapTempoLocked(void) { }

    // clock ticked
    virtual void midiClockTicked(uint32_t tickCount) { }

    // clock position was reset
    virtual void midiClockPositionReset(void) { }

    // externally locked tempo changed
    virtual void midiClockExtTempoChanged(void) { }

    // external sync state changed
    virtual void midiClockExtSyncChanged(int synced) { }
};

// MIDI clock PLL class
class MidiClockPll {
private:
    // settings
    enum ClockState {
        RUNSTOP_IDLE,  // no action
        RUNSTOP_START,  // start requested
        RUNSTOP_CONTINUE,  // continue requested
        RUNSTOP_STOP  // stop requested
    };
    static constexpr float DEFAULT_TEMPO = 120.0f;  // BPM
    static constexpr float TEMPO_MIN = 30.0f;  // BPM
    static constexpr float TEMPO_MAX = 300.0f;  // BPM
    static constexpr int TAP_TIMEOUT = 2500000;  // us (just longer than 30BPM)
    static constexpr int TAP_HIST_LEN = 2;  // taps in history buffer - required taps will be +1
    static constexpr int EXT_HIST_LEN = 8;  // number of historical intervals to average (must be a power of 2)
    static constexpr int EXT_HIST_MASK = (EXT_HIST_LEN - 1);
    static constexpr int EXT_MIN_HIST = 3;  // number of interval samples needed before changing internal clock
    static constexpr int EXT_SYNC_TIMEOUT = 125000;  // timeout for receiving external sync (us)
    static constexpr int EXT_ERROR_ADJ = 500;  // lock adjust amount
    static constexpr float EXT_SYNC_TEMPO_FILTER = 0.9f;  // tempo average filter coeff
    MidiClockPllHandler *handler;
    int usPerTickMax;  // max allowed us per tick
    int usPerTickMin;  // min allowed us per tick
    int clockInternalPpq;  // the internal PPQ rate of the clock
    int taskIntervalUs;  // task interval in us
    int midiClockUpsample;  // upsample ratio
    // general clock state
    int desiredSource;  // 0 = external, 1 = internal
    int source;  // 0 = external, 1 = internal
    int desiredRunState;  // 0 = stopped, 1 = running
    int runState;  // 0 = stopped, 1 = running
    int runstopf;  // flag that we want to change the playback state
    int resetf;  // flag that we want to reset playback pos
    int extTickf;  // external tick received flag
    int64_t timeCount;  // running time count
    int64_t nextTickTime;  // time for next tick
    // internal clock state
    int32_t runTickCount;  // running tick count
    int32_t stopTickCount;  // stopped tick count
    int32_t intUsPerBeat;  // master tempo setting value
    int32_t intUsPerTick;  // number of us per tick (internal)
    // external clock recovery state
    int32_t extIntervalHist[EXT_HIST_LEN];
    int32_t extIntervalCount;  // number of historical intervals measured
    int extSyncTimeout;  // countdown for invalidating clock
    int64_t extLastTickTime;  // time of the last tick received
    int32_t extRunTickCount;  // count of external ticks
    int extSyncTempoAverage;  // average tempo for display
    // tap tempo state
    int tapBeatf;  // tap tempo beat was received
    int64_t tapClockLastTap;  // last tap time
    int tapClockPeriod;  // tape clock period - averaged
    int tapHistCount;  // number of historical taps
    int64_t tapHist[TAP_HIST_LEN];

    // private methods
    void resetPos(void);
    void changeRunState(int run);

public:
    enum ClockSource {
        SOURCE_EXTERNAL = 0,
        SOURCE_INTERNAL
    };

    // constructor
    MidiClockPll();

    // register the handler for callbacks
    void registerHandler(MidiClockPllHandler *handler);

    // set the task interval in us to calculate tempo
    void setTaskInterval(int taskIntervalUs);

    // set the internal PPQ of the clock - i.e. 24 or 96
    void setInternalPpq(int ppq);

    // run the timer task
    void timerTask(void);

    // get the MIDI clock source
    int getSource(void);

    // set the MIDI clock source
    void setSource(int source);

    // check if the external clock is synced
    int isExtSynced(void);

    // get the clock tempo
    float getTempo(void);

    // set the clock tempo
    void setTempo(float tempo);

    // handle tap tempo
    void tapTempo(void);

    // handle continue request (from a user control)
    void continueRequest(void);

    // handle stop request (from a user control)
    void stopRequest(void);

    // handle reset request
    void resetRequest(void);

    // get current tick position
    uint32_t getTickPos(void);

    // get clock running state
    int getRunState(void);

    //
    // external clock inputs
    //
    // a MIDI tick was received
    void handleMidiTick(void);

    // a MIDI clock start was received
    void handleMidiStart(void);

    // a MIDI clock continue was received
    void handleMidiContinue(void);

    // a MIDI clock stop was received
    void handleMidiStop(void);
};

#endif
