/*
 * MIDI Clock with PLL
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Kilpatrick Audio
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "MidiClockPll.h"
#include "../utils/PLog.h"
#include "../utils/PUtils.h"

// constructor
MidiClockPll::MidiClockPll() {
    handler = NULL;
    taskIntervalUs = 1000;  // default
    setInternalPpq(24);  // default
    // general clock state
    desiredSource = SOURCE_INTERNAL;
    source = SOURCE_INTERNAL;
    desiredRunState = 0;
    runState = 0;
    runstopf = RUNSTOP_IDLE;
    resetf = 0;
    extTickf = 0;
    timeCount = 0;
    nextTickTime = 0;
    // internal clock state
    runTickCount = 0;
    stopTickCount = 0;
    setTempo(DEFAULT_TEMPO);
    // external clock recovery state
    extIntervalCount = 0;
    extSyncTimeout = 0;  // timed out
    extLastTickTime = 0;
    extRunTickCount = 0;
    extSyncTempoAverage = intUsPerTick;  // default
    // tap tempo
    tapBeatf = 0;
    tapClockLastTap = 0;
    tapClockPeriod = 0;
    tapHistCount = 0;
}

// register the handler for callbacks
void MidiClockPll::registerHandler(MidiClockPllHandler *handler) {
    this->handler = handler;
}

// set the task interval in us to calculate tempo
void MidiClockPll::setTaskInterval(int taskIntervalUs) {
    this->taskIntervalUs = taskIntervalUs;
}

// set the internal PPQ of the clock - i.e. 24 or 96
void MidiClockPll::setInternalPpq(int ppq) {
    float tempo = getTempo();
    this->clockInternalPpq = ppq;
    this->midiClockUpsample = this->clockInternalPpq / MIDI_NATIVE_PPQ;
    this->usPerTickMax = ((uint32_t)(60000000.0 / (TEMPO_MIN * (float)clockInternalPpq)));
    this->usPerTickMin = ((uint64_t)(60000000.0 / (TEMPO_MAX * (float)clockInternalPpq)));
    setTempo(tempo);
}

// run the timer task
void MidiClockPll::timerTask(void) {
    int i, error;
    uint32_t tick_count;
    int32_t temp;

    // handle playback state change flags
    switch(runstopf) {
        case RUNSTOP_START:
            desiredRunState = 1;
            resetPos();
            runstopf = RUNSTOP_IDLE;
            break;
        case RUNSTOP_CONTINUE:
            desiredRunState = 1;
            runstopf = RUNSTOP_IDLE;
            break;
        case RUNSTOP_STOP:
            desiredRunState = 0;
            runstopf = RUNSTOP_IDLE;
            break;
    }

    // handle reset flags
    if(resetf) {
        resetPos();
        resetf = 0;
    }

    // handle source change
    if(source != desiredSource) {
        source = desiredSource;
        if(handler != NULL) {
            handler->midiClockSourceChanged(source);
        }
        changeRunState(0);  // force stop
    }

    // run clock timebase
    timeCount += taskIntervalUs;
    // decide if we should issue a clock
    while(timeCount > nextTickTime) {
        // if run state changed
        if(runState != desiredRunState) {
            // stopping
            if(desiredRunState == 0) {
                stopTickCount = runTickCount;
            }
            changeRunState(desiredRunState);
        }
        // get the correct tick count
        if(runState) {
            tick_count = runTickCount;
        }
        else {
            tick_count = stopTickCount;
        }
        // calculate beat cross before processing sequencer stuff
        if((tick_count % clockInternalPpq) == 0) {
            if(handler != NULL) {
                handler->midiClockBeatCrossed();
            }
//            // XXX debug
//            log_debug("us: %d - run_tick: %d - ext_run_tick: %d - diff: %d",
//                intUsPerTick,
//                runTickCount, extRunTickCount,
//                (runTickCount - extRunTickCount));
        }
        if(handler != NULL) {
            handler->midiClockTicked(tick_count);
        }

        tick_count ++;
        nextTickTime += intUsPerTick;
        // write back the tick count
        if(runState) {
            runTickCount = tick_count;
        }
        else {
            stopTickCount = tick_count;
        }
    }

    // recover external clock and drive the internal clock
    if(source == SOURCE_EXTERNAL) {
        // a tick was received
        if(extTickf) {
            extTickf = 0;
            // ext sync started
            if(!extSyncTimeout && handler != NULL) {
                handler->midiClockExtSyncChanged(1);
            }
            extSyncTimeout = EXT_SYNC_TIMEOUT;
            // measure interval - skip the very first time since it will be wrong
            extIntervalHist[(extIntervalCount - 1) & EXT_HIST_MASK] =
                timeCount - extLastTickTime;
            // average the number of samples we have
            temp = 0;
            for(i = 0; i < EXT_HIST_LEN && i < extIntervalCount; i ++) {
                temp += extIntervalHist[i];
            }
            // if we have at least EXT_MIN_HIST samples let's update the internal clock
            if(i >= EXT_MIN_HIST) {
                temp /= i;
                // set the internal clock to this value
                intUsPerTick = temp / midiClockUpsample;  // convert to internal PPQ (might be >24)
//                // XXX debug
//                log_debug("i: %d - avg: %d - time: %lld - last: %lld - diff: %lld", i, temp,
//                    timeCount, extLastTickTime,
//                    (timeCount - extLastTickTime));
                extSyncTempoAverage = ((float)extSyncTempoAverage *
                    EXT_SYNC_TEMPO_FILTER) +
                    ((float)intUsPerTick * (1.0 - EXT_SYNC_TEMPO_FILTER ));
            }
            // count external ticks if running
            if(runState) {
                extRunTickCount += midiClockUpsample;
                // compensate for error
                error = runTickCount - extRunTickCount;
                if(error < 0) {
                    intUsPerTick -= EXT_ERROR_ADJ;
                }
                else if(error > 0) {
                    intUsPerTick += EXT_ERROR_ADJ;
                }
            }
            extLastTickTime = timeCount;
            extIntervalCount ++;
        }
    }

    // timeout external sync
    if(extSyncTimeout) {
        extSyncTimeout -= taskIntervalUs;
        // ext sync lost
        if(extSyncTimeout <= 0) {
            if(handler != NULL) {
                handler->midiClockExtSyncChanged(0);
            }
            extIntervalCount = 0;  // reset
            runstopf = RUNSTOP_STOP;
        }
    }

    // recover tap tempo input
    // we can't do this when we're locked to external clock
    if(tapBeatf && !extSyncTimeout) {
        tapBeatf = 0;
        tapHist[tapHistCount % TAP_HIST_LEN] =
            timeCount - tapClockLastTap;
        tapClockLastTap = timeCount;
        tapHistCount ++;
        // we've received enough taps
        if(tapHistCount > TAP_HIST_LEN) {
            tapClockPeriod = 0;
            for(i = 0; i < TAP_HIST_LEN; i ++) {
                tapClockPeriod += tapHist[i];
            }
            tapClockPeriod /= TAP_HIST_LEN;
            temp = (tapClockPeriod / clockInternalPpq);
            // ensure that tempo fits in the valid range before accepting
            if(temp < usPerTickMin) {
                intUsPerTick = usPerTickMin;
            }
            else if(temp > usPerTickMax) {
                intUsPerTick = usPerTickMax;
            }
            else {
                intUsPerTick = temp;
            }
            intUsPerBeat = intUsPerTick * clockInternalPpq;
            if(handler != NULL) {
                handler->midiClockTapTempoLocked();
            }
        }
    }
    // time out tap tempo history
    if(tapHistCount && (timeCount - tapClockLastTap) > TAP_TIMEOUT) {
        tapHistCount = 0;
    }
}

// get the MIDI clock source
int MidiClockPll::getSource(void) {
    return source;
}

// set the MIDI clock source
void MidiClockPll::setSource(int source) {
    if(source == SOURCE_INTERNAL) {
        this->source = SOURCE_INTERNAL;
    }
    else {
        this->source = SOURCE_EXTERNAL;
    }
}

// check if the external clock is synced
int MidiClockPll::isExtSynced(void) {
    if(source == SOURCE_EXTERNAL && extSyncTimeout) {
        return 1;
    }
    return 0;
}

// get the clock tempo
float MidiClockPll::getTempo(void) {
    if(isExtSynced()) {
        return 60000000.0f / (float)clockInternalPpq / (float)extSyncTempoAverage;
    }
    return 60000000.0f / (float)intUsPerBeat;
}

// set the clock tempo
void MidiClockPll::setTempo(float tempo) {
    intUsPerBeat = (int32_t)(60000000.0f / tempo);
    intUsPerTick = intUsPerBeat / clockInternalPpq;
    PDEBUG("intUsPerTick: %d", intUsPerTick);
}

// handle tap tempo
void MidiClockPll::tapTempo(void) {
    tapBeatf = 1;
}

// handle continue request (from a user control)
void MidiClockPll::continueRequest(void) {
    runstopf = RUNSTOP_CONTINUE;
}

// handle stop request (from a user control)
void MidiClockPll::stopRequest(void) {
    runstopf = RUNSTOP_STOP;
}

// handle reset request
void MidiClockPll::resetRequest(void) {
    resetf = 1;
}

// get current tick position
uint32_t MidiClockPll::getTickPos(void) {
    if(runState) {
        return runTickCount;
    }
    return stopTickCount;
}

// get clock running state
int MidiClockPll::getRunState(void) {
    return runState;
}

//
// external clock inputs
//
// a MIDI tick was received
void MidiClockPll::handleMidiTick(void) {
    extTickf = 1;
}

// a MIDI clock start was received
void MidiClockPll::handleMidiStart(void) {
    runstopf = RUNSTOP_START;
}

// a MIDI clock continue was received
void MidiClockPll::handleMidiContinue(void) {
    runstopf = RUNSTOP_CONTINUE;
}

// a MIDI clock stop was received
void MidiClockPll::handleMidiStop(void) {
    runstopf = RUNSTOP_STOP;
}

//
// private methods
//
// reset the position
void MidiClockPll::resetPos(void) {
    runTickCount = 0;
    stopTickCount = 0;
    extRunTickCount = 0;
    if(handler != NULL) {
        handler->midiClockPositionReset();
    }
}

// change run state
void MidiClockPll::changeRunState(int run) {
    int isReset = 0;
    desiredRunState = run;
    runState = run;
    if(handler != NULL) {
        if(runState && runTickCount == 0) {
            isReset = 1;
        }
        else if(!runState && stopTickCount == 0) {
            isReset = 1;
        }
        handler->midiClockRunStateChanged(runState, isReset);
    }
}
