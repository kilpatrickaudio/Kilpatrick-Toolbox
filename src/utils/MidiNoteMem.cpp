/*
 * Kilpatrick Audio MIDI Note Memory
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "MidiNoteMem.h"

// constructor
MidiNoteMem::MidiNoteMem() {
}

// add a note
void MidiNoteMem::addNote(const midi::Message& msg) {
    int noteOff = 0;
    std::vector<midi::Message>::iterator iter;

    switch(msg.bytes[0] & 0xf0) {
        case MIDI_NOTE_ON:
            if(msg.bytes[2] == 0) {
                noteOff = 1;
            }
            break;
        case MIDI_NOTE_OFF:
            noteOff = 1;
            break;
        default:
            return;
    }

    iter = activeNotes.begin();
    while(iter != activeNotes.end()) {
        // found note with same number
        if(iter->bytes[1] == msg.bytes[1]) {
            // remove note
            if(noteOff) {
                activeNotes.erase(iter);
                continue;
            }
            // update note
            else {
                iter->bytes[2] = msg.bytes[2];
                return;
            }
        }
        ++iter;
    }
    if(noteOff) {
        return;
    }
    // add note
    activeNotes.push_back(msg);
}

// get the number of currently active notes
int MidiNoteMem::getNumNotes(void) {
    return activeNotes.size();
}

// get a note - returns a note of size 0 on error
midi::Message MidiNoteMem::getNote(int index) {
    midi::Message msg;
    if(index < 0 || index >= (int)activeNotes.size()) {
        msg.setSize(0);
        return msg;
    }
    return activeNotes[index];
}

// clear the note list
void MidiNoteMem::clear(void) {
    activeNotes.clear();
}
