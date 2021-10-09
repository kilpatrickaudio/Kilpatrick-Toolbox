/*
 * VCV MIDI Interface for CV Cables
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef CV_MIDI_H
#define CV_MIDI_H

#include <rack.hpp>

// CV MIDI adapter
struct CVMidi {
private:
    Port *port;  // port for sending or receiving
    int isInput;  // 1 = port is input, 0 = port is output
    midi::InputQueue msgQueue;  // a queue to handle messages
    int MIDI_LED_TIMEOUT = 1920;  // sample periods
    int ledTimeout;

public:
    // constructor
    CVMidi(Port *port, int isInput) {
        this->port = port;
        this->isInput = isInput;
        ledTimeout = 0;
    }

    // get an input message from a port
    // 0 for no message, 1 if message received
    int getInputMessage(midi::Message *msg) {
        if(msgQueue.tryPop(msg, INT64_MAX) == true) {
            return 1;
        }
        return 0;
    }

    // send an output message to a port
    // returns -1 on error
    int sendOutputMessage(midi::Message msg) {
        msgQueue.onMessage(msg);
        return 0;
    }

    // get the state of the activity LED
    int getLedState(void) {
        if(ledTimeout) {
            return 1;
        }
        return 0;
    }

    // process input and output messages - run at samplerate
    void process(void) {
        int msgWord;
        midi::Message msg;

        //
        // float can hold a 24 bit int with no rounding error
        // bit representation of int converted from float
        //  - bits: 23-16: message byte 0 - 8 bits
        //  - bits: 15-8: message byte 1 - 8 bits
        //  - bits: 7-0: message byte 2 - 8 bits
        //
        // input messages and send them to MIDI lib
        if(isInput) {
            // port is connected and value is not negative
            if(port->getVoltage() < 0.0f) {
                msgWord = roundf(-port->getVoltage());
                msg.bytes[0] = (msgWord >> 16) & 0xff;
                msg.bytes[1] = (msgWord >> 8) & 0xff;
                msg.bytes[2] = msgWord & 0xff;
                // figure out the length
                switch(msg.bytes[0] & 0xf0) {
                    case 0x80:  // note off
                    case 0x90:  // note on
                    case 0xa0:  // poly pressure
                    case 0xb0:  // control change
                    case 0xe0:  // pitch bend
                        msg.setSize(3);
                        break;
                    case 0xc0:  // program change
                    case 0xd0:  // channel pressure
                        msg.setSize(2);
                        break;
                    default:  // system messages or SYSEX
                        switch(msg.bytes[0]) {
                            case 0xf0:  // SYSEX start
                                msg.setSize(3);  // almost certainly >=3 bytes
                                break;
                            case 0xf1:  // MTC Qframe
                            case 0xf3:  // song select
                                msg.setSize(2);
                                break;
                            case 0xf2:  // song position
                                msg.setSize(3);
                                break;
                            case 0xf4:  // undefined
                            case 0xf5:  // undefined
                            case 0xf6:  // tune request
                            case 0xf7:  // end of exclusive
                            case 0xf8:  // timing clock
                            case 0xf9:  // undefined
                            case 0xfa:  // clock start
                            case 0xfb:  // clock continue
                            case 0xfc:  // clock stop
                            case 0xfd:  // undefined
                            case 0xfe:  // active sensing
                            case 0xff:  // system reset
                                msg.setSize(1);
                                break;
                            default:  // SYSEX continuation / end
                                // SYSEX end on 2nd byte
                                if(msg.bytes[1] == 0xf7) {
                                    msg.setSize(2);
                                }
                                // sysex end on 3rd byte or other data
                                else {
                                    msg.setSize(3);
                                }
                                break;
                        }
                        break;
                }
                sendOutputMessage(msg);
                ledTimeout = MIDI_LED_TIMEOUT;
            }
        }
        // output messages from MIDI lib to port
        else {
            // a message is ready
            if(getInputMessage(&msg)) {
                msgWord = (msg.bytes[0] & 0xff) << 16;
                msgWord |= (msg.bytes[1] & 0xff) << 8;
                msgWord |= msg.bytes[2] & 0xff;
                port->setVoltage(-(float)msgWord);
                ledTimeout = MIDI_LED_TIMEOUT;
            }
            else {
                port->setVoltage(0.0f);
            }
        }

        // time out LED
        if(ledTimeout) {
            ledTimeout --;
        }
    }
};

#endif
