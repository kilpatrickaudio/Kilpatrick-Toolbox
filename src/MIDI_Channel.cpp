/*
 * MIDI Channel - vMIDI Channel Splitter
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Kilpatrick Audio
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
#include "plugin.hpp"
#include "utils/CVMidi.h"
#include "utils/KAComponents.h"
#include "utils/MidiHelper.h"
#include "utils/MidiNoteMem.h"
#include "utils/PUtils.h"

struct MIDI_Channel : Module, KilpatrickLabelHandler {
	enum ParamIds {
        IN_CHAN,  // input channel - -1 means all, 0-15 = channel 1-16
        OUT_CHAN, // out channel - 0-15 = channel 1-16
        KEY_SPLIT,  // key split point - 36 to 84 (C2 to C6)
        KEY_SPLIT_ENABLE,  // key split enable mode
        KEY_TRANS,  // key transpose - -24 to +24
		NUM_PARAMS
	};
	enum InputIds {
		MIDI_IN,
		NUM_INPUTS
	};
	enum OutputIds {
		MIDI_OUT_L,
		MIDI_OUT_R,
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_IN_LED,
        MIDI_OUT_L_LED,
        MIDI_OUT_R_LED,
		NUM_LIGHTS
	};
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    CVMidi *cvMidiOut[2];
    #define DOUBLE_CLICK_TIMEOUT (MIDI_RT_TASK_RATE * 0.3)
    putils::Pulser doubleClickPulser;
    MidiNoteMem midiNoteMem[2];
    int resetOutputNotes;
    // defaults
    #define IN_CHAN_DEFAULT -1
    #define OUT_CHAN_DEFAULT 0
    #define KEY_TRANS_DEFAULT 0
    #define KEY_SPLIT_DEFAULT 60
    #define KEY_SPLIT_ENABLE_DEFAULT 0

    // constructor
	MIDI_Channel() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(IN_CHAN, -1.0f, 15.0f, -1.0f, "IN CHAN");
        configParam(OUT_CHAN, 0.0f, 15.0f, 0.0f, "OUT CHAN");
        configParam(KEY_TRANS, -24.0f, 24.0f, 0.0f, "KEY TRANS");
        configParam(KEY_SPLIT, 36.0f, 84.0f, 60.0f, "KEY SPLIT");
        configParam(KEY_SPLIT_ENABLE, 0.0f, 1.0f, 0.0f, "KEY SPLIT ENABLE");
        cvMidiIn = new CVMidi(&inputs[MIDI_IN], 1);
        cvMidiOut[MIDI_OUT_L] = new CVMidi(&outputs[MIDI_OUT_L], 0);
        cvMidiOut[MIDI_OUT_R] = new CVMidi(&outputs[MIDI_OUT_R], 0);
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Channel() {
        delete cvMidiIn;
        delete cvMidiOut[MIDI_OUT_L];
        delete cvMidiOut[MIDI_OUT_R];
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        int i, outSelect;
        midi::Message msg;

        // handle CV MIDI
        cvMidiIn->process();
        cvMidiOut[MIDI_OUT_L]->process();
        cvMidiOut[MIDI_OUT_R]->process();

        // run tasks
        if(taskTimer.process()) {
            // reset notes that we output before
            if(resetOutputNotes) {
                for(outSelect = 0; outSelect < NUM_OUTPUTS; outSelect ++) {
                    for(i = 0; i < midiNoteMem[outSelect].getNumNotes(); i ++) {
                        msg = midiNoteMem[outSelect].getNote(i);
                        // convert note on to off
                        msg.bytes[0] = MIDI_NOTE_OFF | (msg.bytes[0] & 0x0f);
                        cvMidiOut[MIDI_OUT_L + outSelect]->sendOutputMessage(msg);
                    }
                }
                resetOutputNotes = 0;
            }

            // process MIDI
            while(cvMidiIn->getInputMessage(&msg)) {
                outSelect = 1;  // right output
                // filter channel messages
                if(MidiHelper::isChannelMessage(msg)) {
                    // in channel filter
                    if((int)params[IN_CHAN].getValue() != -1 &&
                            (int)params[IN_CHAN].getValue() != MidiHelper::getChannelMsgChannel(msg)) {
                        continue;
                    }
                    // output channel mapping
                    msg.bytes[0] = (msg.bytes[0] & 0xf0) | ((int)params[OUT_CHAN].getValue() & 0x0f);

                    // process note messages
                    if(MidiHelper::isNoteMessage(msg)) {
                        // key split
                        if((int)params[KEY_SPLIT_ENABLE].getValue()) {
                            // left hand
                            if(msg.bytes[1] < (int)params[KEY_SPLIT].getValue()) {
                                outSelect = 0;
                            }
                        }
                        // key transpose
                        msg.bytes[1] = putils::clamp(msg.bytes[1] + (int)params[KEY_TRANS].getValue(), 0, 127);
                        midiNoteMem[outSelect].addNote(msg);  // keep track of notes we sent
                    }
                }
                cvMidiOut[MIDI_OUT_L + outSelect]->sendOutputMessage(msg);
            }

            // MIDI LEDs
            lights[MIDI_IN_LED].setBrightness(cvMidiIn->getLedState());
            lights[MIDI_OUT_L_LED].setBrightness(cvMidiOut[0]->getLedState());
            lights[MIDI_OUT_R_LED].setBrightness(cvMidiOut[1]->getLedState());
            doubleClickPulser.update();
        }
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / MIDI_RT_TASK_RATE));
    }

    // module initialize
    void onReset(void) override {
        int i;
        for(i = 0; i < NUM_LIGHTS; i ++) {
            lights[i].setBrightness(0.0f);
        }
        params[IN_CHAN].setValue(IN_CHAN_DEFAULT);
        params[OUT_CHAN].setValue(OUT_CHAN_DEFAULT);
        params[KEY_SPLIT].setValue(KEY_SPLIT_DEFAULT);
        params[KEY_SPLIT_ENABLE].setValue(KEY_SPLIT_ENABLE_DEFAULT);
        params[KEY_TRANS].setValue(KEY_TRANS_DEFAULT);
        resetOutputNotes = 1;  // force reset
    }

    //
    // callbacks
    //
    // set the text for a label
    std::string updateLabel(int id) override {
        switch(id) {
            case 0:  // in chan
                if((int)params[IN_CHAN].getValue() == -1) {
                    return putils::format("ALL");
                }
                return putils::format("CH %02d", ((int)params[IN_CHAN].getValue() + 1));
            case 1:  // out chan
                return putils::format("CH %02d", ((int)params[OUT_CHAN].getValue() + 1));
            case 2:  // key split
                if((int)params[KEY_SPLIT_ENABLE].getValue()) {
                    return putils::format("%02d", (int)params[KEY_SPLIT].getValue());
                }
                else {
                    return putils::format("OFF");
                }
            case 3:  // key trans
                if((int)params[KEY_TRANS].getValue() == 0) {
                    return putils::format("0");
                }
                return putils::format("%+02d", (int)params[KEY_TRANS].getValue());
        }
        return putils::format("-");
    }

    // handle button on the label
    int onLabelButton(int id, const event::Button& e) override {
        if(e.action != GLFW_PRESS) {
            return 0;
        }
        // toggle key split mode
        if(doubleClickPulser.timeout && id == 2) {
            if((int)params[KEY_SPLIT_ENABLE].getValue()) {
                params[KEY_SPLIT_ENABLE].setValue(0);
            }
            else {
                params[KEY_SPLIT_ENABLE].setValue(1);
            }
            resetOutputNotes = 1;  // force reset
        }
        doubleClickPulser.timeout = DOUBLE_CLICK_TIMEOUT;
        return 1;
    }

    // handle scroll on the label
    int onLabelHoverScroll(int id, const event::HoverScroll& e) override {
        int change = 1;
        if(e.scrollDelta.y < 0.0) {
            change = -1;
        }
        switch(id) {
            case 0:  // in chan
                params[IN_CHAN].setValue(
                    putils::clamp((int)params[IN_CHAN].getValue() + change, -1, 15));
                resetOutputNotes = 1;  // force reset
                break;
            case 1:  // out chan
                params[OUT_CHAN].setValue(
                    putils::clamp((int)params[OUT_CHAN].getValue() + change, 0, 15));
                resetOutputNotes = 1;  // force reset
                break;
            case 2:  // key split
                params[KEY_SPLIT_ENABLE].setValue(1);  // force on so we can see it
                params[KEY_SPLIT].setValue(
                    putils::clamp((int)params[KEY_SPLIT].getValue() + change, 36, 84));
                resetOutputNotes = 1;  // force reset
                break;
            case 3:  // key trans
                params[KEY_TRANS].setValue(
                    putils::clamp((int)params[KEY_TRANS].getValue() + change, -24, 24));
                resetOutputNotes = 1;  // force reset
                break;
        }
        return 1;
    }
};

struct MIDI_ChannelWidget : ModuleWidget {
	MIDI_ChannelWidget(MIDI_Channel* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Channel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        KilpatrickLabel *textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 20.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 0;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 16.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

        textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 32.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 1;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 16.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

        textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 44.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 2;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 16.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

        textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 56.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 3;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 16.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 76.5)), module, MIDI_Channel::MIDI_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 92.5)), module, MIDI_Channel::MIDI_OUT_L));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_Channel::MIDI_OUT_R));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 70.15)), module, MIDI_Channel::MIDI_IN_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 86.15)), module, MIDI_Channel::MIDI_OUT_L_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_Channel::MIDI_OUT_R_LED));
	}
};

Model* modelMIDI_Channel = createModel<MIDI_Channel, MIDI_ChannelWidget>("MIDI_Channel");
