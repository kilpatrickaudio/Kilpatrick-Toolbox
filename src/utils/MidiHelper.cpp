/*
 * Kilpatrick Audio MIDI Helper
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "MidiHelper.h"
#include <string>
#include "MidiProtocol.h"
#include "PLog.h"
#define MIDI_HELPER_HANDLE_RTMIDI  // uncomment to handle RtMidi exceptions

// create a driver chooser menu item
MidiHelperDriverMenuItem::MidiHelperDriverMenuItem(std::string name, int driverId,
        MidiHelperDriverHandler *handler) {
    this->text = name;
    this->rightText = CHECKMARK(driverId == handler->driverGetSelected());
    this->driverId = driverId;
    this->handler = handler;
}

// the menu item was selected
void MidiHelperDriverMenuItem::onAction(const event::Action &e) {
    handler->driverSetSelected(driverId);
}

// create a device chooser menu item
MidiHelperDeviceMenuItem::MidiHelperDeviceMenuItem(std::string name, int isInput,
        int deviceId, int slot, MidiHelperDeviceHandler *handler) {
    this->text = name;
    this->rightText = CHECKMARK(handler->deviceIsOpen(isInput, slot, deviceId));
    this->isInput = isInput;
    this->deviceId = deviceId;
    this->slot = slot;
    this->handler = handler;
}

// the menu item was selected
void MidiHelperDeviceMenuItem::onAction(const event::Action &e) {
    handler->deviceSetSelected(slot, isInput, deviceId);
}

// create a new MIDI helper (use 1 per module)
MidiHelper::MidiHelper(int numInputSlots, int numOutputSlots, int autoKeepalive) {
    int slot;
    combinedMode = 0;
    for(slot = 0; slot < numInputSlots; slot ++) {
        inputs.push_back(*(new midi::InputQueue()));
        inputNames.push_back("");
        onlineTimeouts.push_back(0);
    }
    for(slot = 0; slot < numOutputSlots; slot ++) {
        outputs.push_back(*(new midi::Output()));
        outputNames.push_back("");
    }
    taskTimer.setDivision((int)(APP->engine->getSampleRate() / MIDI_TASK_RATE));
    deviceNameMatchLen = 64;
    this->autoKeepalive = autoKeepalive;
}

// handle task for reconnect, etc.
void MidiHelper::process(void) {
    int slot;
    if(taskTimer.process()) {
        // timeout stuff
        for(slot = 0; slot < (int)inputs.size(); slot ++) {
            if(onlineTimeouts[slot]) {
                onlineTimeouts[slot] --;
                // if devices timed out remove it
                if(autoKeepalive && !onlineTimeouts[slot]) {
//                    DEBUG("clearing devices so they will be reattached");
                    if((int)inputs.size() > slot && inputs[slot].deviceId != -1) {
//                        DEBUG("clearing input slot: %d", slot);
                        inputs[slot].setDeviceId(-1);
                    }
                    if((int)outputs.size() > slot && outputs[slot].deviceId != -1) {
//                        DEBUG("clearing output slot: %d", slot);
                        outputs[slot].setDeviceId(-1);
                    }
                }
            }
        }

        // check if we need to connect devices
        for(slot = 0; slot < (int)inputs.size(); slot ++) {
            if(inputNames[slot].length() > 0 && inputs[slot].deviceId == -1) {
//                DEBUG("trying to restore input - slot: %d - name: %s", slot, inputNames[slot].c_str());
                openInputByName(slot, inputNames[slot]);
            }
        }
        for(slot = 0; slot < (int)outputs.size(); slot ++) {
            if(outputNames[slot].length() > 0 && outputs[slot].deviceId == -1) {
//                DEBUG("trying to restore output - slot: %d - name: %s", slot, outputNames[slot].c_str());
                openOutputByName(slot, outputNames[slot]);
            }
        }
    }
}

// save MIDI settings to JSON in the patch
void MidiHelper::dataToJson(json_t *rootJ) {
    int slot;
    char tempstr[256];
    // driver ID
    json_object_set_new(rootJ, "midiDriver", json_integer(driverGetSelected()));

    // inputs
    for(slot = 0; slot < (int)inputs.size(); slot ++) {
        if(inputNames[slot].length() < 1) {
            continue;
        }
        sprintf(tempstr, "midiIn%d", slot);
        json_object_set_new(rootJ, tempstr, json_string(inputNames[slot].c_str()));
    }

    // outputs
    for(slot = 0; slot < (int)outputs.size(); slot ++) {
        if(outputNames[slot].length() < 1) {
            continue;
        }
        sprintf(tempstr, "midiOut%d", slot);
        json_object_set_new(rootJ, tempstr, json_string(outputNames[slot].c_str()));
    }
}

// load MIDI settings from JSON in the patch
void MidiHelper::dataFromJson(json_t *rootJ) {
    int slot;
    char tempstr[256];
    json_t *midiJ = NULL;
    midi::InputQueue input;
    midi::Output output;

    // driver ID
    midiJ = json_object_get(rootJ, "midiDriver");
    if(midiJ) {
        driverSetSelected(json_integer_value(midiJ));
    }

    // inputs
    for(slot = 0; slot < (int)inputs.size(); slot ++) {
        sprintf(tempstr, "midiIn%d", slot);
        midiJ = json_object_get(rootJ, tempstr);
        if(midiJ) {
            inputNames[slot] = json_string_value(midiJ);
//            DEBUG("recalling input - slot: %d - name: %s", slot, inputNames[slot].c_str());
        }
    }

    // outputs
    for(slot = 0; slot < (int)outputs.size(); slot ++) {
        sprintf(tempstr, "midiOut%d", slot);
        midiJ = json_object_get(rootJ, tempstr);
        if(midiJ) {
            outputNames[slot] = json_string_value(midiJ);
//            DEBUG("recalling output - slot: %d - name: %s", slot, outputNames[slot].c_str());
        }
    }
}

// set combined in/out mode to open pairs at once
void MidiHelper::setCombinedInOutMode(int enable) {
    if(enable) {
        combinedMode = 1;
    }
    else {
        combinedMode = 0;
    }
}

// set the length to match device names
void MidiHelper::setDeviceNameMatchLen(int len) {
    deviceNameMatchLen = len;
}

// get the device name of the current device
std::string MidiHelper::getDeviceName(int slot, int isInput) {
    if(isInput) {
        if(slot < 0 || slot >= (int)inputs.size()) {
            return "No Device";
        }
        if(inputs[slot].deviceId == -1) {
            return "No Device";
        }
        return inputs[slot].getDeviceName(inputs[slot].deviceId);
    }
    if(slot < 0 || slot >= (int)outputs.size()) {
        return "No Device";
    }
    if(outputs[slot].deviceId == -1) {
        return "No Device";
    }
    return outputs[slot].getDeviceName(outputs[slot].deviceId);
}

//
// menu helpers
//
// populate the menu with driver options
void MidiHelper::populateDriverMenu(Menu *menu, std::string portName) {
    menuHelperAddSpacer(menu);
    if(portName.length() > 0) {
        menuHelperAddLabel(menu, "MIDI Driver - " + portName);
    }
    else {
        menuHelperAddLabel(menu, "MIDI Driver");
    }

    // add found drivers
    for(int driverId : midi::getDriverIds()) {
        menuHelperAddItem(menu, new MidiHelperDriverMenuItem(midi::getDriver(driverId)->getName(),
            driverId, (MidiHelperDriverHandler *)this));
    }
}

// populate the menu with input choices
void MidiHelper::populateInputMenu(Menu *menu, std::string prefixFilter, int slot) {
    menuHelperAddSpacer(menu);
    if(combinedMode) {
        menuHelperAddLabel(menu, "MIDI Devices - Slot " + std::to_string(slot + 1));
    }
    else {
        menuHelperAddLabel(menu, "MIDI Inputs - Slot " + std::to_string(slot + 1));
    }

    std::string s = prefixFilter;
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });

    // add unassign option
    menuHelperAddItem(menu, new MidiHelperDeviceMenuItem("None",
        1, -1, slot, (MidiHelperDeviceHandler *)this));

    // add found devices
    for(int deviceId : inputs[slot].getDeviceIds()) {
        std::string devName = getInputDeviceName(slot, deviceId);
        std::string s2 = devName;
        transform(s2.begin(), s2.end(), s2.begin(), [](unsigned char c){ return toupper(c); });
        // exclude device that doesn't match our prefix
        if(prefixFilter.length() > 0 && s2.find(s) == s2.npos) {
            continue;
        }
        menuHelperAddItem(menu, new MidiHelperDeviceMenuItem(devName.c_str(),
            1, deviceId, slot, (MidiHelperDeviceHandler *)this));
    }
}

// populate the menu with output choices
void MidiHelper::populateOutputMenu(Menu *menu, std::string prefixFilter, int slot) {
    menuHelperAddSpacer(menu);
    menuHelperAddLabel(menu, "MIDI Outputs - Slot " + std::to_string(slot + 1));

    std::string s = prefixFilter;
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });

    // add unassign option
    menuHelperAddItem(menu, new MidiHelperDeviceMenuItem("None",
        0, -1, slot, (MidiHelperDeviceHandler *)this));

    // add found devices
    for(int deviceId : outputs[slot].getDeviceIds()) {
        std::string devName = getOutputDeviceName(slot, deviceId);
        std::string s2 = devName;
        transform(s2.begin(), s2.end(), s2.begin(), [](unsigned char c){ return toupper(c); });
        // exclude device that doesn't match our prefix
        if(prefixFilter.length() > 0 && s2.find(s) == s2.npos) {
            continue;
        }
        menuHelperAddItem(menu, new MidiHelperDeviceMenuItem(devName.c_str(),
            0, deviceId, slot, (MidiHelperDeviceHandler *)this));
    }
}

//
// MIDI port access
//
// check if a port is assigned to an device/port
// returns 1 if connected, 0 otherwise
int MidiHelper::isAssigned(int isInput, int slot) {
    if(isInput) {
        if(slot < 0 || slot >= (int)inputs.size()) {
            return 0;
        }
        if(inputs[slot].driverId != -1 && inputs[slot].deviceId != -1) {
            return 1;
        }
        return 0;
    }
    if(slot < 0 || slot >= (int)outputs.size()) {
        return 0;
    }
    if(outputs[slot].driverId != -1 && outputs[slot].deviceId != -1) {
        return 1;
    }
    return 0;
}

// check if a port is detected (active sensing recently)
int MidiHelper::isDetected(int slot) {
    if(slot < 0 || slot >= (int)inputs.size()) {
        return 0;
    }
    if(onlineTimeouts[slot]) {
        return 1;
    }
    return 0;
}

// get an input message from a port
// returns -1 on error, 0 for no message, 1 if message received
int MidiHelper::getInputMessage(int slot, midi::Message *msg) {
    if(slot < 0 || slot >= (int)inputs.size()) {
        return -1;
    }
    // drain out the entire queue
    while(inputs[slot].tryPop(msg, INT64_MAX)) {
        if(msg->getSize() < 1) {
            continue;  // try again
        }
        // inspect the message for active sensing and steal it
        if(msg->bytes[0] == MIDI_ACTIVE_SENSING) {
            onlineTimeouts[slot] = ONLINE_TIMEOUT;
            continue;  // try again
        }
        // return 1 saying the caller has a message they can use
        return 1;
    }
    return 0;
}

// send an output message to a port
// returns -1 on error
int MidiHelper::sendOutputMessage(int slot, const midi::Message& msg) {
    if(slot < 0 || slot >= (int)outputs.size()) {
        return -1;
    }
    if(outputs[slot].deviceId == -1) {
        return -1;
    }
    outputs[slot].sendMessage(msg);
    return 0;
}

// reset ports - returns -1 on error
int MidiHelper::resetPorts(void) {
    int slot;
    for(slot = 0; slot < (int)inputs.size(); slot ++) {
        if(inputs[slot].deviceId != -1) {
            inputs[slot].reset();
        }
    }
    for(slot = 0; slot < (int)outputs.size(); slot ++) {
        if(outputs[slot].deviceId != -1) {
            outputs[slot].reset();
        }
    }
    return 0;
}

//
// MIDI send helpers
//
// send a note on - returns -1 on error
int MidiHelper::sendNoteOn(int slot, int chan, int note, int vel) {
    midi::Message msg;
    if(slot < 0 || slot >= (int)outputs.size()) {
        return -1;
    }
    if(outputs[slot].deviceId == -1) {
        return -1;
    }
	msg.setStatus(MIDI_NOTE_ON >> 4);
    msg.setChannel(chan);
	msg.setNote(note);
	msg.setValue(vel);
    outputs[slot].sendMessage(msg);
    return 0;
}

// send a note off - returns -1 on error
int MidiHelper::sendNoteOff(int slot, int chan, int note) {
    midi::Message msg;
    if(slot < 0 || slot >= (int)outputs.size()) {
        return -1;
    }
    if(outputs[slot].deviceId == -1) {
        return -1;
    }
	msg.setStatus(MIDI_NOTE_OFF >> 4);
    msg.setChannel(chan);
	msg.setNote(note);
	msg.setValue(0);
    outputs[slot].sendMessage(msg);
    return 0;
}

// send a CC - returns -1 on error
int MidiHelper::sendCC(int slot, int chan, int cc, int val) {
    midi::Message msg;
    if(slot < 0 || slot >= (int)outputs.size()) {
        return -1;
    }
    if(outputs[slot].deviceId == -1) {
        return -1;
    }
	msg.setStatus(MIDI_CONTROL_CHANGE >> 4);
    msg.setChannel(chan);
	msg.setNote(cc);
	msg.setValue(val);
    outputs[slot].sendMessage(msg);
    return 0;
}

//
// callbacks
//
// get the current driver that is active
int MidiHelper::driverGetSelected(void) {
    if(inputs.size() > 0) {
        return inputs[0].driverId;
    }
    return outputs[0].driverId;
}

// handle the driver being chosen
void MidiHelper::driverSetSelected(int driverId) {
    int slot;
    // inputs
    for(slot = 0; slot < (int)inputs.size(); slot ++) {
        inputs[slot].setDriverId(driverId);
    }
    // outputs
    for(slot = 0; slot < (int)outputs.size(); slot ++) {
        outputs[slot].setDriverId(driverId);
    }
}

// check if the current device is open
int MidiHelper::deviceIsOpen(int isInput, int slot, int deviceId) {
    if(isInput) {
        if(slot < 0 || slot >= (int)inputs.size()) {
            return 0;
        }
        if(inputs[slot].deviceId == deviceId) {
            return 1;
        }
    }
    else {
        if(slot < 0 || slot >= (int)outputs.size()) {
            return -1;
        }
        if(outputs[slot].deviceId == deviceId) {
            return 1;
        }
    }
    return 0;
}

// handle the device being chosen
void MidiHelper::deviceSetSelected(int slot, int isInput, int deviceId) {
    if(combinedMode) {
        openInput(slot, deviceId);  // uses device ID from menu
        if(deviceId == -1) {
            openOutput(slot, -1);
        }
        else {
            std::string devName = getInputDeviceName(slot, deviceId);
            openOutputByName(slot, devName);  // uses name from inout device
        }
    }
    else if(isInput) {
        openInput(slot, deviceId);
    }
    else {
        openOutput(slot, deviceId);
    }
}

//
// helpers
//
// print a message
void MidiHelper::printMessage(const midi::Message& msg) {
    DEBUG("MIDI RX - len: %d - st: 0x%02x - d0: 0x%02x - d1: 0x%02x",
        msg.getSize(), msg.bytes[0], msg.bytes[1], msg.bytes[2]);
}

// check if the message is a note message
int MidiHelper::isNoteMessage(const midi::Message& msg) {
    if(msg.getSize() < 3) {
        return 0;
    }
    if((msg.bytes[0] & 0xf0) == MIDI_NOTE_OFF ||
            (msg.bytes[0] & 0xf0) == MIDI_NOTE_ON) {
        return 1;
    }
    return 0;
}

// check if the message is a CC message
int MidiHelper::isControlChangeMessage(const midi::Message& msg) {
    if(msg.getSize() < 3) {
        return 0;
    }
    if((msg.bytes[0] & 0xf0) == MIDI_CONTROL_CHANGE) {
        return 1;
    }
    return 0;
}

// check if the message is a channel message
int MidiHelper::isChannelMessage(const midi::Message& msg) {
    if(msg.getSize() < 2) {
        return 0;
    }
    if((msg.bytes[0] & 0xf0) < 0xf0) {
        return 1;
    }
    return 0;
}

// check if the message is a system common message (not including sysex)
int MidiHelper::isSystemCommonMessage(const midi::Message& msg) {
    if(msg.getSize() < 1) {
        return 0;
    }
    if(msg.bytes[0] >= 0xf1 && msg.bytes[0] <= 0xf6) {
        return 1;
    }
    return 0;
}

// check if the message is a system realtime message
int MidiHelper::isSystemRealtimeMessage(const midi::Message& msg) {
    if(msg.getSize() < 1) {
        return 0;
    }
    if(msg.bytes[0] >= 0xf8) {
        return 1;
    }
    return 0;
}

// get the channel for a channel mode message
// returns channel or -1 on error or not a channel message
int MidiHelper::getChannelMsgChannel(const midi::Message& msg) {
    if(msg.getSize() < 2) {
        return 0;
    }
    if(msg.bytes[0] >= 0xf0) {
        return -1;
    }
    return msg.bytes[0] & 0x0f;
}

// get the pitch bend value
// returns the value or -1 if the message is not a pitch bend
int MidiHelper::getPitchBendVal(const midi::Message& msg) {
    if(msg.getSize() < 3) {
        return 0;
    }
    if((msg.bytes[0] & 0xf0) != MIDI_PITCH_BEND) {
        return -1;
    }
    return (msg.bytes[1] | msg.bytes[2] << 7) - 8192;
}

//
// message encoders
//
// encode a CC message
// returns the encoded message
midi::Message MidiHelper::encodeCCMessage(int channel, int cc, int value) {
    midi::Message msg;
    msg.bytes[0] = 0xb0 | (channel & 0x0f);
    msg.bytes[1] = cc;
    msg.bytes[2] = value;
    return msg;
}

// check if the message is a SYSEX message
// returns:
//  0 - not a SYSEX message
//  1 - SYSEX start
//  2 - SYSEX continuation
//  3 - SYSEX end
int MidiHelper::isSysexMessage(const midi::Message& msg) {
    int i, allLow;
    if(msg.getSize() < 1) {
        return 0;
    }
    if(msg.bytes[0] == 0xf0) {
        return 1;
    }
    allLow = 1;
    for(i = 0; i < msg.getSize(); i ++) {
        if(msg.bytes[i] == 0xf7) {
            return 3;
        }
        if(msg.bytes[i] & 0x80) {
            allLow = 0;
        }
    }
    if(allLow) {
        return 2;
    }
    return 0;
}

//
// private methods
//
// open an input
void MidiHelper::openInput(int slot, int deviceId) {
    inputs[slot].setDeviceId(deviceId);
    if(inputs[slot].deviceId == -1) {
        inputNames[slot] = "";
        onlineTimeouts[slot] = 4;  // make it timeout soon
    }
    else {
        inputs[slot].setChannel(-1);
        inputNames[slot] = getInputDeviceName(slot, deviceId);
    }
}

// open an output
void MidiHelper::openOutput(int slot, int deviceId) {
    outputs[slot].setDeviceId(deviceId);
    outputs[slot].setChannel(-1);  // make sure channel doesn't get overriden by MIDI backend
    if(outputs[slot].deviceId == -1) {
        outputNames[slot] = "";
    }
    else {
        outputNames[slot] = getOutputDeviceName(slot, deviceId);
    }
}

// open an input by name
void MidiHelper::openInputByName(int slot, std::string name) {
    int i;
    midi::Input sys;
    std::vector<int> devIds = sys.getDeviceIds();
    name.resize(deviceNameMatchLen);
    for(i = 0; i < (int)devIds.size(); i ++) {
        if(getInputDeviceName(slot, devIds[i]).compare(name) == 0) {
            openInput(slot, devIds[i]);
            return;
        }
    }
}

// open an output by name
void MidiHelper::openOutputByName(int slot, std::string name) {
    int i;
    midi::Output sys;
    std::vector<int> devIds = sys.getDeviceIds();
    name.resize(deviceNameMatchLen);
    for(i = 0; i < (int)devIds.size(); i ++) {
        if(getOutputDeviceName(slot, devIds[i]).compare(name) == 0) {
            openOutput(slot, devIds[i]);
            return;
        }
    }
}

// get the input device name maybe truncated
std::string MidiHelper::getInputDeviceName(int slot, int deviceId) {
    std::string devName = inputs[slot].getDeviceName(deviceId);
    devName.resize(deviceNameMatchLen);
    return devName;
}

// get the output device name maybe truncated
std::string MidiHelper::getOutputDeviceName(int slot, int deviceId) {
    std::string devName = outputs[slot].getDeviceName(deviceId);
    devName.resize(deviceNameMatchLen);
    return devName;
}
