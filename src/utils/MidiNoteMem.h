/*
 * Kilpatrick Audio MIDI Note Memory
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_NOTE_MEM_H
#define MIDI_NOTE_MEM_H

#include "../plugin.hpp"
#include "MidiProtocol.h"

class MidiNoteMem {
private:
    std::vector<midi::Message> activeNotes;

public:
    // constructor
    MidiNoteMem();

    // add a note
    void addNote(midi::Message msg);

    // get the number of currently active notes
    int getNumNotes(void);

    // get a note - returns a note of size 0 on error
    midi::Message getNote(int index);

    // clear the note list
    void clear(void);
};

#endif
