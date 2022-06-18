/*
 * MIDI Output - Hardware MIDI Output to vMIDI Adapter
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

struct MIDI_Output : Module, KilpatrickLabelHandler {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
        MIDI_IN,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_IN_LED,
		NUM_LIGHTS
	};

    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    MidiHelper *midi;

    // constructor
	MIDI_Output() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(MIDI_IN, "MIDI IN");
        cvMidiIn = new CVMidi(&inputs[MIDI_IN], 1);
        midi = new MidiHelper(0, 1, 1);
        midi->setCombinedInOutMode(0);
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Output() {
        delete midi;
        delete cvMidiIn;
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        midi::Message msg;

        // handle CV MIDI
        cvMidiIn->process();

        // run tasks
        if(taskTimer.process()) {
            while(cvMidiIn->getInputMessage(&msg)) {
                if(midi->isAssigned(0, 0)) {
                    midi->sendOutputMessage(0, msg);
                }
            }

            // MIDI in LEDs
            lights[MIDI_IN_LED].setBrightness(cvMidiIn->getLedState());
        }

        midi->process();
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
    }

    // module initialize
    void onReset(void) override {
        lights[MIDI_IN_LED].setBrightness(0.0f);
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
        return midi->getDeviceName(0, 0);
    }
};

struct MIDI_OutputWidget : ModuleWidget {
	MIDI_OutputWidget(MIDI_Output* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Output.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_Output::MIDI_IN));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_Output::MIDI_IN_LED));

        // labels
        float labelW = 0.700;
        float labelH = 0.800;
        float labelSpacing = 0.400;
        KilpatrickLabel *l = new KilpatrickLabel(0,
            vutils::panelin2px(0.4, 4.25 - labelSpacing),
            vutils::in2px(labelW, labelH));
        l->handler = (KilpatrickLabelHandler *)module;
        l->text = "No Device";
        l->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        l->fontSize = 10;
        l->fgColor = MIDI_LABEL_FG_COLOR;
        l->bgColor = MIDI_LABEL_BG_COLOR;
        addChild(l);
	}

    // add menu items
    void appendContextMenu(Menu *menu) override {
        MIDI_Output *module = dynamic_cast<MIDI_Output*>(this->module);
        if(!module) {
            return;
        }

        // MIDI settings
        module->midi->populateDriverMenu(menu, "MIDI Output Device");
        module->midi->populateOutputMenu(menu, "", 0);
    }
};

Model* modelMIDI_Output = createModel<MIDI_Output, MIDI_OutputWidget>("MIDI_Output");
