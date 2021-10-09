/*
 * Kilpatrick Audio MIDI to Note
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 */
#ifndef MIDI2NOTE_H
#define MIDI2NOTE_H

#include "../plugin.hpp"

class Midi2Note {
private:
    // settings
    #define POLY_MAX_VOICES 3
    #define NOTE_MIN 12
    #define NOTE_MAX (127-12)
    // settings
    int bendRange;  // pitch bend range
    int polyMode;  // 1 = poly mode, 0 = mono mode
    int channel;  // receive channel
    // state
    int damper;  // damper pedal state - 1 = pressed, 0 = released
    std::vector<midi::Message> monoPrio;  // mono note list
    int heldNotes[POLY_MAX_VOICES];  // held notes for each voice - voice 0 = mono
    int currentNotes[POLY_MAX_VOICES];  // note for each voice - voice 0 = mono
    float currentBend;  // current pitch bend amount in volts
    // outputs
    float pitchOut[POLY_MAX_VOICES];  // pitch output voltage for each voice
    int gateOut[POLY_MAX_VOICES];  // gate output state for each voice
    float velOut[POLY_MAX_VOICES];  // velocity output voltage for each voice

    // private methods
    void handleNoteOff(midi::Message msg);
    void handleNoteOn(midi::Message msg);
    void handleCC(midi::Message msg);
    void handleBend(midi::Message msg);
    void setVoiceNote(int voice, int note, int vel);

public:
    // constructor
    Midi2Note();

    // reset everything
    void reset();

    // handle an input message
    void handleMessage(const midi::Message& msg);

    // get the poly mode state
    int getPolyMode(void);

    // set the poly mode on or off
    void setPolyMode(int enable);

    // get the receive channel
    int getChannel(void);

    // set the receive channel
    void setChannel(int chan);

    // get the bend range
    int getBendRange(void);

    // set the bend range
    void setBendRange(int semis);

    // get the current voltage for a voice - returns 0.0 on error
    float getPitchVoltage(int voice);

    // get the current gate voltage for a voice - returns 0.0 on error
    float getGateVoltage(int voice);

    // get the current velocity voltage for a voice - returns 0.0 on error
    float getVelocityVoltage(int voice);
};

#endif
