/*
 * MIDI Mapper - vMIDI CC to Note Converter
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

struct MIDI_CC_Note : Module, KilpatrickLabelHandler {
	enum ParamId {
		VEL_POT,
		OCT_UP_SW,
		OCT_NORM_SW,
		OCT_DOWN_SW,
		PARAMS_LEN
	};
	enum InputId {
		MIDI_IN,
		INPUTS_LEN
	};
	enum OutputId {
		MIDI_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
        MIDI_IN_LED,
        MIDI_OUT_LED,
		LIGHTS_LEN
	};

	MIDI_CC_Note() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(VEL_POT, 0.f, 1.f, 0.8f, "VELOCITY");
		configParam(OCT_UP_SW, 0.f, 1.f, 0.f, "OCT UP");
		configParam(OCT_NORM_SW, 0.f, 1.f, 0.f, "OCT NORMAL");
		configParam(OCT_DOWN_SW, 0.f, 1.f, 0.f, "OCT DOWN");
		configInput(MIDI_IN, "MIDI IN");
		configOutput(MIDI_OUT, "MIDI OUT");
	}

	void process(const ProcessArgs& args) override {
	}


    //
    // callbacks
    //
    // set the text for a label
    std::string updateLabel(int id) override {
        return putils::format("---");
    }
};

struct MIDI_CC_NoteWidget : ModuleWidget {
	MIDI_CC_NoteWidget(MIDI_CC_Note* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MIDI_CC_Note.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        KilpatrickLabel *textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 20.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 0;
        textField->rad = 4.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 14.0;
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

//		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 20.5)), module, MIDI_CC_Note::PATH881_PARAM));
		addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(10.16, 36.5)), module, MIDI_CC_Note::VEL_POT));
		addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(10.16, 52.5)), module, MIDI_CC_Note::OCT_UP_SW));
		addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(10.16, 66.5)), module, MIDI_CC_Note::OCT_NORM_SW));
		addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(10.16, 80.5)), module, MIDI_CC_Note::OCT_DOWN_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 94.5)), module, MIDI_CC_Note::MIDI_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_CC_Note::MIDI_OUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 90.15)), module, MIDI_CC_Note::MIDI_IN_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_CC_Note::MIDI_OUT_LED));
	}
};

Model* modelMIDI_CC_Note = createModel<MIDI_CC_Note, MIDI_CC_NoteWidget>("MIDI_CC_Note");
