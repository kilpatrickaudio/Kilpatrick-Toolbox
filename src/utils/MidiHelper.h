/*
 * Kilpatrick Audio MIDI Helper
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_HELPER_H
#define MIDI_HELPER_H

#include "../plugin.hpp"
#include "MenuHelper.h"

// a callback handler for choosing MIDI driver
struct MidiHelperDriverHandler {
    // get the current driver that is active
    virtual int driverGetSelected(void) { return 0; }

    // handle the driver being chosen
    virtual void driverSetSelected(int driverId) { }
};

// MIDI driver chooser menu item
struct MidiHelperDriverMenuItem : MenuItem {
    int driverId;
    MidiHelperDriverHandler *handler;

    // create a driver chooser menu item
    MidiHelperDriverMenuItem(std::string name, int driverId,
        MidiHelperDriverHandler *handler);

    // the menu item was selected
    void onAction(const event::Action &e) override;
};

// a callback handler for choosing a MIDI device
struct MidiHelperDeviceHandler {
    // check if the current device is open
    virtual int deviceIsOpen(int isInput, int slot, int deviceId) { return 0; }

    // handle the device being chosen
    virtual void deviceSetSelected(int isInput, int deviceId, int slot) { }
};

// a MIDI device chooser menu item
struct MidiHelperDeviceMenuItem : MenuItem {
    int isInput;
    int slot;
    int deviceId;
    MidiHelperDeviceHandler *handler;

    // create a device chooser menu item
    MidiHelperDeviceMenuItem(std::string name, int isInput,
        int deviceId, int slot, MidiHelperDeviceHandler *handler);

    // the menu item was selected
    void onAction(const event::Action &e) override;
};

class MidiHelper : MidiHelperDriverHandler, MidiHelperDeviceHandler {
private:
    int combinedMode;
    std::vector<midi::InputQueue> inputs;
    std::vector<midi::Output> outputs;
    std::vector<std::string> inputNames;  // expected device names
    std::vector<std::string> outputNames;  // expected device names
    dsp::ClockDivider taskTimer;
    #define MIDI_TASK_RATE 2  // 2Hz
    #define ONLINE_TIMEOUT 4  // 2 seconds
    std::vector<int> onlineTimeouts;
    int deviceNameMatchLen;
    int autoKeepalive;  // 1 = reconnect in/out pairs if input times out

    // private methods
    void openInput(int slot, int deviceId);
    void openOutput(int slot, int deviceId);
    void openInputByName(int slot, std::string name);
    void openOutputByName(int slot, std::string name);
    std::string getInputDeviceName(int slot, int deviceId);
    std::string getOutputDeviceName(int slot, int deviceId);

public:
    // create a new MIDI helper (use 1 per module)
    MidiHelper(int numInputSlots, int numOutputSlots, int autoKeepalive);

    // destructor
    virtual ~MidiHelper() = default;

    // handle task for reconnect, etc.
    void process(void);

    // save MIDI settings to JSON in the patch
    void dataToJson(json_t *rootJ);

    // load MIDI settings from JSON in the patch
    void dataFromJson(json_t *rootJ);

    // set combined in/out mode to open pairs at once
    void setCombinedInOutMode(int enable);

    // set the length to match device names
    void setDeviceNameMatchLen(int len);

    // get the device name of the current device
    std::string getDeviceName(int slot, int isInput);

    //
    // menu helpers
    //
    // populate the menu with driver options
    void populateDriverMenu(Menu *menu, std::string portName);

    // populate the menu with input choices
    void populateInputMenu(Menu *menu, std::string prefixFilter, int slot);

    // populate the menu with output choices
    void populateOutputMenu(Menu *menu, std::string prefixFilter, int slot);

    //
    // MIDI port access
    //
    // check if a port is assigned to an device/port
    // returns 1 if connected, 0 otherwise
    int isAssigned(int isInput, int slot);

    // check if a port is detected (active sensing recently)
    int isDetected(int slot);

    // get an input message from a port
    // returns -1 on error, 0 for no message, 1 if message received
    int getInputMessage(int slot, midi::Message *msg);

    // send an output message to a port
    // returns -1 on error
    int sendOutputMessage(int slot, const midi::Message& msg);

    // reset ports - returns -1 on error
    int resetPorts(void);

    //
    // MIDI send helpers
    //
    // send a note on - returns -1 on error
    int sendNoteOn(int slot, int chan, int note, int vel);

    // send a note off - returns -1 on error
    int sendNoteOff(int slot, int chan, int note);

    // send a CC - returns -1 on error
    int sendCC(int slot, int chan, int cc, int val);

    //
    // callbacks
    //
    // get the current driver
    int driverGetSelected(void) override;

    // handle the driver being chosen
    void driverSetSelected(int devId) override;

    // check if the current device is open
    int deviceIsOpen(int isInput, int slot, int deviceId) override;

    // handle the device being chosen
    void deviceSetSelected(int slot, int isInput, int deviceId) override;

    //
    // helpers
    //
    // print a message
    static void printMessage(const midi::Message& msg);

    // check if the message is a note message
    static int isNoteMessage(const midi::Message& msg);

    // check if the message is a CC message
    static int isControlChangeMessage(const midi::Message& msg);

    // check if the message is a channel message
    static int isChannelMessage(const midi::Message& msg);

    // check if the message is a system common message (not including sysex)
    static int isSystemCommonMessage(const midi::Message& msg);

    // check if the message is a system realtime message
    static int isSystemRealtimeMessage(const midi::Message& msg);

    // get the channel for a channel mode message
    // returns channel or -1 on error or not a channel message
    static int getChannelMsgChannel(const midi::Message& msg);

    // get the pitch bend value
    // returns the value or -1 if the message is not a pitch bend
    static int getPitchBendVal(const midi::Message& msg);

    //
    // message encoders
    //
    // encode a CC message
    // returns the encoded message
    static midi::Message encodeCCMessage(int channel, int cc, int value);

    // check if the message is a SYSEX message
    // returns:
    //  0 - not a SYSEX message
    //  1 - SYSEX start
    //  2 - SYSEX continuation
    //  3 - SYSEX end
    static int isSysexMessage(const midi::Message& msg);
};

#endif
