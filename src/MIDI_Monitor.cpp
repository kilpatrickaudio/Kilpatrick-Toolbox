/*
 * MIDI Monitor - vMIDI Monitor
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

struct MIDI_Monitor : Module, KilpatrickLabelHandler {
	enum ParamIds {
		MIDI_IN1_SW,  // must be sequential
		MIDI_IN2_SW,
		MIDI_IN3_SW,
		MIDI_IN4_SW,
		NUM_PARAMS
	};
	enum InputIds {
		MIDI_IN1,  // must be sequential
		MIDI_IN2,
		MIDI_IN3,
		MIDI_IN4,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		MIDI_IN1_LED,  // must be sequential
		MIDI_IN2_LED,
		MIDI_IN3_LED,
		MIDI_IN4_LED,
        MIDI_IN1_SW_LED, // must be sequential
		MIDI_IN2_SW_LED,
		MIDI_IN3_SW_LED,
		MIDI_IN4_SW_LED,
		NUM_LIGHTS
	};

    #define NUM_INPUTS 4
    #define DISPLAY_LINES 7
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidi[NUM_INPUTS];
    int inputEnable[NUM_INPUTS];
    std::list<std::string> displayLines;
    std::string displayText;
    int lineNum;

    // constructor
	MIDI_Monitor() {
        int port;
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MIDI_IN1_SW, 0.f, 1.f, 0.f, "MIDI IN1");
		configParam(MIDI_IN2_SW, 0.f, 1.f, 0.f, "MIDI IN2");
		configParam(MIDI_IN3_SW, 0.f, 1.f, 0.f, "MIDI IN3");
		configParam(MIDI_IN4_SW, 0.f, 1.f, 0.f, "MIDI IN4");
        configInput(MIDI_IN1, "MIDI IN1");
        configInput(MIDI_IN2, "MIDI IN2");
        configInput(MIDI_IN3, "MIDI IN3");
        configInput(MIDI_IN4, "MIDI IN4");
        for(port = 0; port < NUM_INPUTS; port ++) {
            cvMidi[port] = new CVMidi(&inputs[MIDI_IN1 + port], 1);
        }
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Monitor() {
        int port;
        for(port = 0; port < NUM_INPUTS; port ++) {
            delete cvMidi[port];
        }
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        midi::Message msg;
        int port;

        // handle CV MIDI
        for(port = 0; port < NUM_INPUTS; port ++) {
            cvMidi[port]->process();
        }

        // run tasks
        if(taskTimer.process()) {
            // check channels
            for(port = 0; port < NUM_INPUTS; port ++) {
                // input messages
                while(cvMidi[port]->getInputMessage(&msg) && inputEnable[port]) {
//                    MidiHelper::printMessage(&msg);
                    switch(msg.getSize()) {
                        case 1:
                            addDisplayLine(putils::format("%d %2X -- --",
                                (port + 1),
                                msg.bytes[0]));
                            break;
                        case 2:
                            addDisplayLine(putils::format("%d %2X %2X --",
                                (port + 1),
                                msg.bytes[0],
                                msg.bytes[1]));
                            break;
                        case 3:
                        default:
                            addDisplayLine(putils::format("%d %2X %2X %2X",
                                (port + 1),
                                msg.bytes[0],
                                msg.bytes[1],
                                msg.bytes[2]));
                            break;
                    }
                }

                // input switches / LEDs
                if(params[MIDI_IN1_SW + port].getValue() > 0.5f) {
                    lights[MIDI_IN1_SW_LED + port].setBrightness(1.0f);
                    inputEnable[port] = 1;
                }
                else {
                    lights[MIDI_IN1_SW_LED + port].setBrightness(0.0f);
                    inputEnable[port] = 0;
                }
                // MIDI LEDs
                lights[MIDI_IN1_LED + port].setBrightness(cvMidi[port]->getLedState());
            }
        }
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
    }

    // module initialize
    void onReset(void) override {
        int i;
        for(i = 0; i < NUM_INPUTS; i ++) {
            lights[MIDI_IN1_LED + i].setBrightness(0.0f);
            params[MIDI_IN1_SW + i].setValue(1.0f);
            inputEnable[i] = 1;
        }
        for(i = 0; i < DISPLAY_LINES; i ++) {
            addDisplayLine("");
        }
        lineNum = 0;
    }

    // add a line to the display
    void addDisplayLine(std::string line) {
        if(line.length() > 0) {
            displayLines.push_back(putils::format("%04d ", lineNum) + line);
        }
        else {
            displayLines.push_back(line);
        }
        if(displayLines.size() > DISPLAY_LINES) {
            displayLines.pop_front();
        }
        lineNum = (lineNum + 1) & 0x1fff;
        displayText.clear();
        for(std::list<std::string>::iterator it = displayLines.begin();
                it != displayLines.end(); it++) {
            displayText.append(*it + "\n");
        }
    }

    //
    // callbacks
    //
    // set the text for a label
    std::string updateLabel(int id) override {
        return displayText;
    }
};

struct MIDI_MonitorWidget : ModuleWidget {
	MIDI_MonitorWidget(MIDI_Monitor* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Monitor.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        KilpatrickLabel *textField = new KilpatrickLabel(0, mm2px(Vec(20.32, 30.446)), mm2px(Vec(36.0, 32.0)));
        textField->rad = 1.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 11.5;
        textField->text = "0001 1 B0  1 16\n" \
            "0002 1 B0  2 16\n" \
            "0003 1 B0  3 16\n" \
            "0004 1 B0  4 16\n" \
            "0005 1 B0  1 16\n" \
            "0006 1 B0  2 16\n" \
            "0007 1 B0  3 16\n";
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

		ParamWidget *p = createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(28.32, 60.5)), module, MIDI_Monitor::MIDI_IN1_SW);
        ((KilpatrickButton *)p)->momentary = false;
        ((KilpatrickButton *)p)->latchColor = nvgRGBA(0, 0, 0, 0);
        addParam(p);

        p = createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(28.32, 76.5)), module, MIDI_Monitor::MIDI_IN2_SW);
        ((KilpatrickButton *)p)->momentary = false;
        ((KilpatrickButton *)p)->latchColor = nvgRGBA(0, 0, 0, 0);
        addParam(p);

        p = createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(28.32, 92.5)), module, MIDI_Monitor::MIDI_IN3_SW);
        ((KilpatrickButton *)p)->momentary = false;
        ((KilpatrickButton *)p)->latchColor = nvgRGBA(0, 0, 0, 0);
        addParam(p);

		p = createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(28.32, 108.5)), module, MIDI_Monitor::MIDI_IN4_SW);
        ((KilpatrickButton *)p)->momentary = false;
        ((KilpatrickButton *)p)->latchColor = nvgRGBA(0, 0, 0, 0);
        addParam(p);

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.32, 60.5)), module, MIDI_Monitor::MIDI_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.32, 76.5)), module, MIDI_Monitor::MIDI_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.32, 92.5)), module, MIDI_Monitor::MIDI_IN3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.32, 108.5)), module, MIDI_Monitor::MIDI_IN4));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.499, 54.5)), module, MIDI_Monitor::MIDI_IN1_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.499, 54.5)), module, MIDI_Monitor::MIDI_IN1_SW_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.499, 70.5)), module, MIDI_Monitor::MIDI_IN2_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.499, 70.5)), module, MIDI_Monitor::MIDI_IN2_SW_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.499, 86.5)), module, MIDI_Monitor::MIDI_IN3_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.499, 86.5)), module, MIDI_Monitor::MIDI_IN3_SW_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.499, 102.5)), module, MIDI_Monitor::MIDI_IN4_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.499, 102.5)), module, MIDI_Monitor::MIDI_IN4_SW_LED));
	}
};

Model* modelMIDI_Monitor = createModel<MIDI_Monitor, MIDI_MonitorWidget>("MIDI_Monitor");
