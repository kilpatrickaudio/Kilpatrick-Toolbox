/*
 * Kilpatrick Audio MIDI Repeater Handler for CC Message
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2022: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "MidiRepeater.h"
#include "MidiHelper.h"

// constructor
MidiRepeater::MidiRepeater() {
    hist.resize(128);
    mode = RepeaterMode::MODE_OFF;
    setSendInterval(REPEAT_SEND_INTERVAL);
    setHistTimeout(REPEAT_HIST_TIMEOUT);
    setCheckInterval(REPEAT_CHECK_INTERVAL);
    sender = NULL;
    index = 0;
    reset();
}

// register a sender for the repeater
void MidiRepeater::registerSender(MidiRepeaterSender *sender, int index) {
    this->sender = sender;
    this->index = index;
}

// reset the internal state
void MidiRepeater::reset(void) {
    int i;
    repeatCheck = 0;
    for(i = 0; i < 128; i ++) {
        hist[i].timeout = 0;
    }
    setMode(MODE_OFF);
}

// get the mode
int MidiRepeater::getMode(void) {
    return mode;
}

// set the mode
void MidiRepeater::setMode(int mode) {
    if(mode < RepeaterMode::MODE_OFF || mode > RepeaterMode::MODE_GEN) {
        return;
    }
    this->mode = mode;
}

// set the send interval in task runs
void MidiRepeater::setSendInterval(int interval) {
    sendInterval = interval;
}

// set the hist timeout in task runs
void MidiRepeater::setHistTimeout(int timeout) {
    histTimeout = timeout;
}

// set the check interval in task runs
void MidiRepeater::setCheckInterval(int interval) {
    checkInterval = interval;
}

// process an incoming message and send if necessary
// only CCs are processed - other messages are dropped
void MidiRepeater::handleMessage(const midi::Message& msg) {
    midi::Message outMsg;
    if(!MidiHelper::isControlChangeMessage(msg)) {
        return;
    }
    switch(mode) {
        case RepeaterMode::MODE_OFF:
            // if we have a message in timeout skip the echo
            if(hist[msg.bytes[1]].msg.bytes[0] == msg.bytes[0] &&
                    hist[msg.bytes[1]].msg.bytes[2] == msg.bytes[2] &&
                    hist[msg.bytes[1]].timeout > 0) {
                hist[msg.bytes[1]].timeout = histTimeout;
                return;
            }
            else {
                hist[msg.bytes[1]].msg = msg;  // store message
                hist[msg.bytes[1]].timeout = histTimeout;
                MidiHelper::copyMessage(&outMsg, msg);
                if(sender != NULL) {
                    sender->sendMessage(outMsg, index);
                }
            }
            break;
        case RepeaterMode::MODE_GEN:  // pass through and remember
            hist[msg.bytes[1]].msg = msg;  // store message
            hist[msg.bytes[1]].timeout = sendInterval;  // reset send interval
            MidiHelper::copyMessage(&outMsg, msg);
            if(sender != NULL) {
                sender->sendMessage(outMsg, index);
            }
            break;
        case RepeaterMode::MODE_ON:  // pass any repeats through unchanged
        default:
            MidiHelper::copyMessage(&outMsg, msg);
            if(sender != NULL) {
                sender->sendMessage(outMsg, index);
            }
            break;
    }
}

// run the task timer and send if necessary
void MidiRepeater::taskTimer(void) {
    int cc;
    if(repeatCheck == checkInterval) {
        for(cc = 0; cc < 128; cc ++) {
            if(hist[cc].timeout == 0) {
                continue;
            }
            switch(mode) {
                case RepeaterMode::MODE_OFF:  // time out repeat blocking
                    hist[cc].timeout -= checkInterval;
                    if(hist[cc].timeout <= 0) {
                        hist[cc].timeout = 0;
                    }
                    break;
                case RepeaterMode::MODE_GEN:
                    hist[cc].timeout -= checkInterval;
                    if(hist[cc].timeout <= 0) {
                        if(sender != NULL) {
                            sender->sendMessage(hist[cc].msg, index);
                        }
                        hist[cc].timeout = sendInterval;
                    }
                    break;
                case RepeaterMode::MODE_ON:
                default:
                    // no action
                    break;
            }
        }
        repeatCheck = 0;
    }
    repeatCheck ++;
}
