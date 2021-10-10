/*
 * MIDI Mapper - vMIDI CC Mapper
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
#include "utils/MidiCCMem.h"
#include "utils/PUtils.h"

struct MIDI_Mapper : Module, KilpatrickLabelHandler {
	enum ParamIds {
        MAP_CC_IN1,  // must be sequential
        MAP_CC_IN2,
        MAP_CC_IN3,
        MAP_CC_IN4,
        MAP_CC_IN5,
        MAP_CC_IN6,
        MAP_CC_OUT1,  // must be sequential
        MAP_CC_OUT2,
        MAP_CC_OUT3,
        MAP_CC_OUT4,
        MAP_CC_OUT5,
        MAP_CC_OUT6,
		NUM_PARAMS
	};
	enum InputIds {
		MIDI_IN,
		NUM_INPUTS
	};
	enum OutputIds {
		MIDI_OUT,
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_IN_LED,
        MIDI_OUT_LED,
		NUM_LIGHTS
	};

    #define MAP_TIMEOUT (MIDI_RT_TASK_RATE * 4)  // 4 seconds
    #define NUM_MAP_CHANS 6
    #define UNMAP -1.0f
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    CVMidi *cvMidiOut;
    MidiCCMem ccMem;
    enum {
        MAP_CHAN1,
        MAP_CHAN2,
        MAP_CHAN3,
        MAP_CHAN4,
        MAP_CHAN5,
        MAP_CHAN6,
        MAP_DISABLE
    };
    int mapMode;
    int mapTimeout;

    // constructor
	MIDI_Mapper() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MAP_CC_IN1, 0.0f, 255.0f, 0.0f, "CC_IN1");
        configParam(MAP_CC_IN2, 0.0f, 255.0f, 0.0f, "CC_IN2");
        configParam(MAP_CC_IN3, 0.0f, 255.0f, 0.0f, "CC_IN3");
        configParam(MAP_CC_IN4, 0.0f, 255.0f, 0.0f, "CC_IN4");
        configParam(MAP_CC_IN5, 0.0f, 255.0f, 0.0f, "CC_IN5");
        configParam(MAP_CC_IN6, 0.0f, 255.0f, 0.0f, "CC_IN6");
        configParam(MAP_CC_OUT1, 0.0f, 255.0f, 0.0f, "CC_OUT1");
        configParam(MAP_CC_OUT2, 0.0f, 255.0f, 0.0f, "CC_OUT2");
        configParam(MAP_CC_OUT3, 0.0f, 255.0f, 0.0f, "CC_OUT3");
        configParam(MAP_CC_OUT4, 0.0f, 255.0f, 0.0f, "CC_OUT4");
        configParam(MAP_CC_OUT5, 0.0f, 255.0f, 0.0f, "CC_OUT5");
        configParam(MAP_CC_OUT6, 0.0f, 255.0f, 0.0f, "CC_OUT6");
        cvMidiIn = new CVMidi(&inputs[MIDI_IN], 1);
        cvMidiOut = new CVMidi(&outputs[MIDI_OUT], 0);
        ccMem.setTimeout(MIDI_RT_TASK_RATE * 2);  // 2 seconds
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Mapper() {
        delete cvMidiIn;
        delete cvMidiOut;
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        int i;
        midi::Message msg;

        // handle CV MIDI
        cvMidiIn->process();
        cvMidiOut->process();

        // run tasks
        if(taskTimer.process()) {
            while(cvMidiIn->getInputMessage(&msg)) {
                // handle CC messages
                if(MidiHelper::isControlChangeMessage(msg)) {
                    // filter repeated messages from mapping
                    if(ccMem.handleCC(msg) == 0) {
                        // learn maping
                        if(mapMode != MAP_DISABLE) {
                            setMap(mapMode, msg.bytes[1], msg.bytes[1]);
                            mapTimeout = 0;
                            mapMode = MAP_DISABLE;
                        }
                    }
                    // process maps
                    for(i = 0; i < NUM_MAP_CHANS; i ++) {
                        if(msg.bytes[1] == (int)params[MAP_CC_IN1 + i].getValue()) {
                            msg.bytes[1] = (int)params[MAP_CC_OUT1 + i].getValue();
                            break;
                        }
                    }
                }
                cvMidiOut->sendOutputMessage(msg);
            }

            // MIDI LEDs
            lights[MIDI_IN_LED].setBrightness(cvMidiIn->getLedState());
            lights[MIDI_OUT_LED].setBrightness(cvMidiOut->getLedState());

            // timeout mapping
            if(mapTimeout) {
                mapTimeout --;
                if(mapTimeout == 0) {
                    mapMode = MAP_DISABLE;
                }
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
        for(i = 0; i < NUM_MAP_CHANS; i ++) {
            params[MAP_CC_IN1 + i].setValue(UNMAP);
            params[MAP_CC_OUT1 + i].setValue(UNMAP);
        }
        mapMode = MAP_DISABLE;
        mapTimeout = 0;
        ccMem.reset();
    }

    // set a mapping - updates local map and params
    void setMap(int chan, int ccIn, int ccOut) {
        if(chan < 0 || chan >= NUM_MAP_CHANS) {
            return;
        }
        if(ccIn == (int)UNMAP || ccOut == (int)UNMAP) {
            params[MAP_CC_IN1 + chan].setValue(UNMAP);
            params[MAP_CC_OUT1 + chan].setValue(UNMAP);
            return;
        }
        params[MAP_CC_IN1 + chan].setValue(ccIn);
        params[MAP_CC_OUT1 + chan].setValue(ccOut);
    }

    //
    // callbacks
    //
    // set the text for a label
    std::string updateLabel(int id) override {
        if(mapMode == id) {
            return putils::format("MAP>MAP");
        }
        if(params[MAP_CC_IN1 + id].getValue() != UNMAP) {
            return putils::format("%03d>%03d",
                (int)params[MAP_CC_IN1 + id].getValue(),
                (int)params[MAP_CC_OUT1 + id].getValue());
        }
        return putils::format("--- ---");
    }

    // handle button on the label
    int onLabelButton(int id, const event::Button& e) override {
        mapMode = id;
        mapTimeout = MAP_TIMEOUT;
        return 1;
    }

    // handle scroll on the label
    int onLabelHoverScroll(int id, const event::HoverScroll& e) override {
        float change = 1.0f;
        if(e.scrollDelta.y < 0.0f) {
            change = -1.0f;
        }
        setMap(id, params[MAP_CC_IN1 + id].getValue(),
            putils::clampf(params[MAP_CC_OUT1 + id].getValue() + change, UNMAP, 127.0f));
        return 1;
    }
};

struct MIDI_MapperWidget : ModuleWidget {
	MIDI_MapperWidget(MIDI_Mapper* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Mapper.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        KilpatrickLabel *textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 20.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 0;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 12.0;
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
        textField->fontSize = 12.0;
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
        textField->fontSize = 12.0;
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
        textField->fontSize = 12.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

        textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 68.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 4;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 12.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

        textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 80.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 5;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 12.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 96.5)), module, MIDI_Mapper::MIDI_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_Mapper::MIDI_OUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 90.15)), module, MIDI_Mapper::MIDI_IN_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_Mapper::MIDI_OUT_LED));

	}
};

Model* modelMIDI_Mapper = createModel<MIDI_Mapper, MIDI_MapperWidget>("MIDI_Mapper");
