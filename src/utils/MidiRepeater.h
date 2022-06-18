/*
 * Kilpatrick Audio MIDI Repeater Handler for CC Messages
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2022: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_REPEATER_H
#define MIDI_REPEATER_H

#include "../plugin.hpp"

// handler  a port that has to send
struct MidiRepeaterSender {
    // send an output message to a port
    // returns -1 on error
    virtual void sendMessage(const midi::Message& msg, int index) { }
};

// repeater event history
struct MidiRepeaterHist {
    midi::Message msg;
    int timeout;

    MidiRepeaterHist() {
        msg.setSize(0);
        timeout = 0;
    }
};

// MIDI repeater handler
class MidiRepeater {
private:
    std::vector<MidiRepeaterHist> hist;
    int mode;
    int repeatCheck;
    int sendInterval;
    int histTimeout;
    int checkInterval;
    MidiRepeaterSender *sender;
    int index;  // index to use to identify sender
    // default timing
    #define REPEAT_SEND_INTERVAL (RT_TASK_RATE * 0.5f)  // 0.5s
    #define REPEAT_HIST_TIMEOUT (RT_TASK_RATE * 2.0f)  // 2.0s
    #define REPEAT_CHECK_INTERVAL (RT_TASK_RATE * 0.1f)  // 0.1s

public:
    // repeater mode
    enum RepeaterMode {
        MODE_OFF = 0,  // no repeat
        MODE_ON,  // repeat pass-through
        MODE_GEN,  // repeat generate
        MODE_NUM_MODES
    };

    // constructor
    MidiRepeater();

    // register a sender for the repeater
    void registerSender(MidiRepeaterSender *sender, int index);

    // reset the internal state
    void reset(void);

    // get the mode
    int getMode(void);

    // set the mode
    void setMode(int mode);

    // set the send interval in task runs
    void setSendInterval(int interval);

    // set the hist timeout in task runs
    void setHistTimeout(int timeout);

    // set the check interval in task runs
    void setCheckInterval(int interval);

    // process an incoming message and send if necessary
    // only CCs are processed - other messages are dropped
    void handleMessage(const midi::Message& msg);

    // run the task timer and send if necessary
    void taskTimer(void);
};

#endif
