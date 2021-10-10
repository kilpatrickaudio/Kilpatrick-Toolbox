/*
 * Kilpatrick Audio MIDI to Note
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * This file is part of Kilpatrick-Toolbox.
 *
 * Kilpatrick-Toolbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Kilpatrick-Toolbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kilpatrick-Toolbox.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "Midi2Note.h"
#include "../utils/MidiHelper.h"
#include "../utils/MidiProtocol.h"
#include "../utils/PUtils.h"

// constructor
Midi2Note::Midi2Note() {
    setBendRange(2);
    setPolyMode(0);
    reset();
}

// reset everything
void Midi2Note::reset() {
    int i;
    damper = 0;
    monoPrio.clear();
    for(i = 0; i < POLY_MAX_VOICES; i ++) {
        heldNotes[i] = -1;
        currentNotes[i] = -1;
        pitchOut[i] = 0.0f;
        gateOut[i] = 0;
        velOut[i] = 0.0f;
    }
    currentBend = 0.0f;
    channel = -1;
}

// handle an input message
void Midi2Note::handleMessage(const midi::Message& msg) {
    if(!MidiHelper::isChannelMessage(msg)) {
        return;
    }
    if(MidiHelper::getChannelMsgChannel(msg) != channel) {
        return;
    }

    switch(msg.bytes[0] & 0xf0) {
        case MIDI_NOTE_ON:
            if(msg.bytes[2] == 0) {
                handleNoteOff(msg);
            }
            else {
                handleNoteOn(msg);
            }
            break;
        case MIDI_NOTE_OFF:
            handleNoteOff(msg);
            break;
        case MIDI_CONTROL_CHANGE:
            handleCC(msg);
            break;
        case MIDI_PITCH_BEND:
            handleBend(msg);
            break;
        default:
            return;
    }

}

// get the poly mode state
int Midi2Note::getPolyMode(void) {
    return polyMode;
}

// set the poly mode on or off
void Midi2Note::setPolyMode(int enable) {
    polyMode = enable;
    reset();
}

// get the receive channel
int Midi2Note::getChannel(void) {
    return channel;
}

// set the receive channel
void Midi2Note::setChannel(int chan) {
    if(chan < -1 || chan >= MIDI_NUM_CHANNELS) {
        return;
    }
    channel = chan;
}

// get the bend range
int Midi2Note::getBendRange(void) {
    return bendRange;
}

// set the bend range
void Midi2Note::setBendRange(int semis) {
    if(semis < 1 || semis > 12) {
        return;
    }
    bendRange = semis;
}

// get the current voltage for a voice - returns 0.0 on error
float Midi2Note::getPitchVoltage(int voice) {
    if(voice < 0 || voice >= POLY_MAX_VOICES) {
        return 0.0f;
    }
    return pitchOut[voice];
}

// get the current gate voltage for a voice - returns 0.0 on error
float Midi2Note::getGateVoltage(int voice) {
    if(voice < 0 || voice >= POLY_MAX_VOICES) {
        return 0.0f;
    }
    return (float)gateOut[voice] * 10.0f;
}

// get the current velocity voltage for a voice - returns 0.0 on error
float Midi2Note::getVelocityVoltage(int voice) {
    if(voice < 0 || voice >= POLY_MAX_VOICES) {
        return 0.0f;
    }
    return velOut[voice];
}

//
// private methods
//
// handle a note off
void Midi2Note::handleNoteOff(midi::Message msg) {
    int i;
    std::vector<midi::Message>::iterator iter;
    if(msg.bytes[1] < NOTE_MIN || msg.bytes[1] > NOTE_MAX) {
        return;
    }
    // poly mode
    if(polyMode) {
        // free the slots that contain this note
        for(i = 0; i < POLY_MAX_VOICES; i ++) {
            if(heldNotes[i] == msg.bytes[1]) {
                heldNotes[i] = -1;
                setVoiceNote(i, -1, -1);
            }
        }
    }
    // mono mode
    else {
        // check that note is not already in the list
        iter = monoPrio.begin();
        while(iter != monoPrio.end()) {
            if(iter->bytes[1] == msg.bytes[1]) {
                monoPrio.erase(iter);
                continue;
            }
            ++iter;
        }
        // no more notes
        if(monoPrio.size() == 0) {
            heldNotes[0] = -1;
            setVoiceNote(0, -1, -1);  // turn off note
        }
        // set the latest note - don't update velocity
        else {
            heldNotes[0] = monoPrio.back().bytes[1];
            setVoiceNote(0, heldNotes[0], -1);
        }
    }
}

// handle a note on
void Midi2Note::handleNoteOn(midi::Message msg) {
    int i, newStart = 0;
    std::vector<midi::Message>::iterator iter;
    if(msg.bytes[1] < NOTE_MIN || msg.bytes[1] > NOTE_MAX) {
        return;
    }
    // poly mode
    if(polyMode) {
        // find a free voice slot
        for(i = 0; i < POLY_MAX_VOICES; i ++) {
            // need to check currentNotes[] because of damper
            if(currentNotes[i] == -1) {
                heldNotes[i] = msg.bytes[1];
                setVoiceNote(i, msg.bytes[1], msg.bytes[2]);
                break;
            }
        }
    }
    // mono mode
    else {
        // no notes are down - keep track for starting with new velocity
        if(monoPrio.size() == 0) {
            newStart = 1;
        }
        // check that note is not already in the list
        iter = monoPrio.begin();
        while(iter != monoPrio.end()) {
            if(iter->bytes[1] == msg.bytes[1]) {
                monoPrio.erase(iter);
                continue;
            }
            ++iter;
        }
        // add note to the end of the list
        monoPrio.push_back(msg);
        heldNotes[0] = monoPrio.back().bytes[1];
        // new note - send velocity
        if(newStart) {
            heldNotes[0] = monoPrio.back().bytes[1];
            setVoiceNote(0, heldNotes[0], monoPrio.back().bytes[2]);
        }
        // note is already on - don't update velocity
        else {
            setVoiceNote(0, monoPrio.back().bytes[1], -1);
        }
    }
}

// handle a CC
void Midi2Note::handleCC(midi::Message msg) {
    int i;
    switch(msg.bytes[1]) {
        case MIDI_CONTROLLER_DAMPER_PEDAL:
            if(msg.bytes[2] & 0x40) {
                damper = 1;
            }
            else {
                damper = 0;
                // if notes are playing we need to release them
                // poly mode
                if(polyMode) {
                    for(i = 0; i < POLY_MAX_VOICES; i ++) {
                        if(heldNotes[i] == -1) {
                            setVoiceNote(i, -1, -1);  // turn off note
                        }
                    }
                }
                // mono mode
                else {
                    if(heldNotes[0] == -1) {
                        setVoiceNote(0, -1, -1);  // turn off note
                    }
                }
            }
            break;
        default:
            return;
    }
}

// handle a pitch bend
void Midi2Note::handleBend(midi::Message msg) {
    int i;
    int bend = MidiHelper::getPitchBendVal(msg);
    currentBend = ((float)bend * (float)bendRange) * 0.000010173;
    // poly mode
    if(polyMode) {
        for(i = 0; i < POLY_MAX_VOICES; i ++) {
            setVoiceNote(i, currentNotes[i], -1);  // update bend
        }
    }
    // mono mode
    else {
        setVoiceNote(0, currentNotes[0], -1);
    }
}

// set a voice note
void Midi2Note::setVoiceNote(int voice, int note, int vel) {
    // turn on note
    if(note >= 0) {
        pitchOut[voice] = ((float)note * 0.083333333f) + currentBend - 5.0f;
        gateOut[voice] = 1;
        if(vel != -1) {
            velOut[voice] = (putils::midi2float(vel) * 10.0f) - 5.0f;
        }
    }
    // turn off note
    else {
        if(!damper) {
            gateOut[voice] = 0;
        }
    }
    currentNotes[voice] = note;
}
