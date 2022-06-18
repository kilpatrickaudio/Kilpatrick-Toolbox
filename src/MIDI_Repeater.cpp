/*
 * MIDI Repeat Processor - vMIDI Repeater
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
#include "utils/MidiRepeater.h"
#include "utils/PUtils.h"

struct MIDI_Repeater : Module, MidiRepeaterSender {
	enum ParamIds {
		MODE_SW,
		NUM_PARAMS
	};
	enum InputIds {
		MIDI_IN1,
		MIDI_IN2,
		MIDI_IN3,
		NUM_INPUTS
	};
	enum OutputIds {
		MIDI_OUT1,
		MIDI_OUT2,
		MIDI_OUT3,
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_IN1_LED,
        MIDI_IN2_LED,
        MIDI_IN3_LED,
        MIDI_OUT1_LED,
        MIDI_OUT2_LED,
        MIDI_OUT3_LED,
		NUM_LIGHTS
	};

    #define NUM_PORTS 3
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIns[NUM_PORTS];
    CVMidi *cvMidiOuts[NUM_PORTS];
    MidiRepeater repeaterHist[NUM_PORTS];

    // constructor
	MIDI_Repeater() {
        int port;
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MODE_SW, 0.0f, 2.0f, 0.0f, "MODE");
        configInput(MIDI_IN1, "MIDI IN1");
        configInput(MIDI_IN2, "MIDI IN2");
        configInput(MIDI_IN3, "MIDI IN3");
        configOutput(MIDI_OUT1, "MIDI OUT1");
        configOutput(MIDI_OUT2, "MIDI OUT2");
        configOutput(MIDI_OUT3, "MIDI OUT3");

        for(port = 0; port < NUM_PORTS; port ++) {
            cvMidiIns[port] = new CVMidi(&inputs[MIDI_IN1 + port], 1);
            cvMidiOuts[port] = new CVMidi(&outputs[MIDI_OUT1 + port], 0);
            repeaterHist[port].registerSender(this, port);
        }
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Repeater() {
        int port;
        for(port = 0; port < NUM_PORTS; port ++) {
            delete cvMidiIns[port];
            delete cvMidiOuts[port];
        }
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        midi::Message msg;
        int port, temp;

        // handle CV MIDI
        for(port = 0; port < NUM_PORTS; port ++) {
            cvMidiIns[port]->process();
            cvMidiOuts[port]->process();
        }

        // run tasks
        if(taskTimer.process()) {
            // check channels
            for(port = 0; port < NUM_PORTS; port ++) {
                // input message
                while(cvMidiIns[port]->getInputMessage(&msg)) {
//                    MidiHelper::printMessage(&msg);
                    repeaterHist[port].handleMessage(msg);  // let the repeater handle all incoming MIDI
                }
                // MIDI LEDs
                lights[MIDI_IN1_LED + port].setBrightness(cvMidiIns[port]->getLedState());
                lights[MIDI_OUT1_LED + port].setBrightness(cvMidiOuts[port]->getLedState());
            }

            // check mode
            if(params[MODE_SW].getValue() > 1.5f) {
                temp = MidiRepeater::RepeaterMode::MODE_GEN;
            }
            else if(params[MODE_SW].getValue() > 0.5f) {
                temp = MidiRepeater::RepeaterMode::MODE_ON;
            }
            else {
                temp = MidiRepeater::RepeaterMode::MODE_OFF;
            }
            if(temp != repeaterHist[0].getMode()) {
                for(port = 0; port < NUM_PORTS; port ++) {
                    repeaterHist[port].setMode(temp);
                }
            }

            // run tasks for each port
            for(port = 0; port < NUM_PORTS; port ++) {
                repeaterHist[port].taskTimer();
            }
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
        for(i = 0; i < NUM_PORTS; i ++) {
            repeaterHist[i].reset();
        }
    }

    //
    // callbacks
    //
    // send an output message to a port - callback from repeater
    // returns -1 on error
    void sendMessage(const midi::Message& msg, int index) override {
        if(index < 0 || index >= NUM_PORTS) {
            return;
        }
        cvMidiOuts[index]->sendOutputMessage(msg);
    }
};

struct MIDI_RepeaterWidget : ModuleWidget {
	MIDI_RepeaterWidget(MIDI_Repeater* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Repeater.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 24.5)), module, MIDI_Repeater::MIDI_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 36.5)), module, MIDI_Repeater::MIDI_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 48.5)), module, MIDI_Repeater::MIDI_IN3));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 84.5)), module, MIDI_Repeater::MIDI_OUT1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 96.5)), module, MIDI_Repeater::MIDI_OUT2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_Repeater::MIDI_OUT3));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 18.15)), module, MIDI_Repeater::MIDI_IN1_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 30.15)), module, MIDI_Repeater::MIDI_IN2_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 42.15)), module, MIDI_Repeater::MIDI_IN3_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 78.15)), module, MIDI_Repeater::MIDI_OUT1_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 90.15)), module, MIDI_Repeater::MIDI_OUT2_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_Repeater::MIDI_OUT3_LED));

        addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(10.16, 66.5)), module, MIDI_Repeater::MODE_SW));
	}
};

Model* modelMIDI_Repeater = createModel<MIDI_Repeater, MIDI_RepeaterWidget>("MIDI_Repeater");
