/*
 * MIDI Input - Hardware MIDI Input to vMIDI Adapter
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
#include "utils/PUtils.h"
#include "utils/VUtils.h"

struct MIDI_Input : Module, KilpatrickLabelHandler {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
        MIDI_OUT1,
        MIDI_OUT2,
        MIDI_OUT3,
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_OUT1_LED,
        MIDI_OUT2_LED,
        MIDI_OUT3_LED,
		NUM_LIGHTS
	};

    #define NUM_OUTPUTS 3
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiOuts[NUM_OUTPUTS];
    MidiHelper *midi;

    // constructor
	MIDI_Input() {
        int port;
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configOutput(MIDI_OUT1, "CHN OUT");
        configOutput(MIDI_OUT2, "SYS OUT");
        configOutput(MIDI_OUT3, "ALL OUT");
        for(port = 0; port < NUM_OUTPUTS; port ++) {
            cvMidiOuts[port] = new CVMidi(&outputs[MIDI_OUT1 + port], 0);
        }
        midi = new MidiHelper(1, 0, 1);
        midi->setCombinedInOutMode(0);
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Input() {
        int port;
        delete midi;
        for(port = 0; port < NUM_OUTPUTS; port ++) {
            delete cvMidiOuts[port];
        }
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        midi::Message msg;
        int port;

        // handle CV MIDI
        for(port = 0; port < NUM_OUTPUTS; port ++) {
            cvMidiOuts[port]->process();
        }

        // run tasks
        if(taskTimer.process()) {
            // get incoming MIDI
            if(midi->isAssigned(1, 0)) {
                while(midi->getInputMessage(0, &msg)) {
                    if(MidiHelper::isChannelMessage(msg)) {
                        cvMidiOuts[MIDI_OUT1]->sendOutputMessage(msg);
                        cvMidiOuts[MIDI_OUT3]->sendOutputMessage(msg);
                    }
                    if(MidiHelper::isSystemCommonMessage(msg) ||
                            MidiHelper::isSystemRealtimeMessage(msg)) {
                        cvMidiOuts[MIDI_OUT2]->sendOutputMessage(msg);
                        cvMidiOuts[MIDI_OUT3]->sendOutputMessage(msg);
                    }
                }
            }
            // handle outputs
            for(port = 0; port < NUM_OUTPUTS; port ++) {
                lights[MIDI_OUT1_LED + port].setBrightness(cvMidiOuts[port]->getLedState());
            }
        }

        midi->process();
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
    }

    // save custom JSON in patch
    json_t* dataToJson() override {
		json_t* rootJ = json_object();
        midi->dataToJson(rootJ);  // add MIDI settings
		return rootJ;
	}

    // load custom JSON from patch
	void dataFromJson(json_t* rootJ) override {
        midi->dataFromJson(rootJ);  // get MIDI settings
	}

    //
    // GUI stuff
    //
    // update the label
    std::string updateLabel(int id) override {
        return midi->getDeviceName(0, 1);
    }
};

struct MIDI_InputWidget : ModuleWidget {
	MIDI_InputWidget(MIDI_Input* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Input.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 84.5)), module, MIDI_Input::MIDI_OUT1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 96.5)), module, MIDI_Input::MIDI_OUT2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_Input::MIDI_OUT3));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 78.15)), module, MIDI_Input::MIDI_OUT1_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 90.15)), module, MIDI_Input::MIDI_OUT2_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_Input::MIDI_OUT3_LED));

        // labels
        float labelW = 0.700;
        float labelH = 0.800;
        float labelSpacing = 0.400;
        KilpatrickLabel *l = new KilpatrickLabel(0,
            vutils::panelin2px(0.4, 4.25 - labelSpacing),
            vutils::in2px(labelW, labelH));
        l->handler = (KilpatrickLabelHandler *)module;
        l->text = "Input";
        l->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        l->fontSize = 10;
        l->fgColor = MIDI_LABEL_FG_COLOR;
        l->bgColor = MIDI_LABEL_BG_COLOR;
        addChild(l);
	}

    // add menu items
    void appendContextMenu(Menu *menu) override {
        MIDI_Input *module = dynamic_cast<MIDI_Input*>(this->module);
        if(!module) {
            return;
        }

        // MIDI settings
        module->midi->populateDriverMenu(menu, "MIDI Input Device");
        module->midi->populateInputMenu(menu, "", 0);
    }
};

Model* modelMIDI_Input = createModel<MIDI_Input, MIDI_InputWidget>("MIDI_Input");
