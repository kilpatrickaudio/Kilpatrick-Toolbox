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
#include "utils/MenuHelper.h"
#include "utils/MidiHelper.h"
#include "utils/MidiProtocol.h"
#include "utils/MidiRepeater.h"
#include "utils/PUtils.h"

struct MIDI_CC_Note : Module, KilpatrickLabelHandler, MidiRepeaterSender {
	enum ParamId {
		VEL_POT,
		OCT_UP_SW,
		OCT_NORM_SW,
		OCT_DOWN_SW,
        OCT_OFFSET,  // octave offset
        CC_BASE,  // base CC offset
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
    static constexpr int OCT_OFFSET_MAX = 6;
    static constexpr int OCT_OFFSET_MIN = -6;
    static constexpr int LED_PULSE_LEN = 50;
    static constexpr int LAST_NOTE_TIMEOUT = 500;
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    CVMidi *cvMidiOut;
    MidiRepeater repeatHist;
    putils::PosEdgeDetect octUpEdge;
    putils::PosEdgeDetect octNormEdge;
    putils::PosEdgeDetect octDownEdge;
    putils::Pulser lastNoteTimeout;
    int lastNote;

    // constructor
	MIDI_CC_Note() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(VEL_POT, 0.f, 1.f, 0.8f, "VELOCITY");
		configParam(OCT_UP_SW, 0.f, 1.f, 0.f, "OCT UP");
		configParam(OCT_NORM_SW, 0.f, 1.f, 0.f, "OCT NORMAL");
		configParam(OCT_DOWN_SW, 0.f, 1.f, 0.f, "OCT DOWN");
        configParam(OCT_OFFSET, -6.0f, 6.0f, 0.0f, "OCT_OFFSET");
        configParam(CC_BASE, 0.0f, 120.0f, 0.0f, "CC BASE");
		configInput(MIDI_IN, "MIDI IN");
		configOutput(MIDI_OUT, "MIDI OUT");
        cvMidiIn = new CVMidi(&inputs[MIDI_IN], 1);
        cvMidiOut = new CVMidi(&outputs[MIDI_OUT], 0);
        repeatHist.registerSender(this, 0);
        repeatHist.setMode(MidiRepeater::RepeaterMode::MODE_OFF);
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_CC_Note() {
        delete cvMidiIn;
        delete cvMidiOut;
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        midi::Message msg;

        // handle CV MIDI
        cvMidiIn->process();
        cvMidiOut->process();

        // run tasks
        if(taskTimer.process()) {
            // handle MIDI input
            while(cvMidiIn->getInputMessage(&msg)) {
                repeatHist.handleMessage(msg);
/*
                if(msg.getSize() < 3) continue;
                if((msg.bytes[0] & 0xf0) != MIDI_CONTROL_CHANGE) {
                    continue;
                }
                int note = (int)params[CC_BASE].getValue() + msg.bytes[1] +
                    ((int)params[OCT_OFFSET].getValue() * 12);
                PDEBUG("note: %d - offset: %d", note, (int)params[OCT_OFFSET].getValue());
                if(note < 0 || note > 127) continue;
                int vel = (int)((float)msg.bytes[2] * params[VEL_POT].getValue());
                msg.setSize(3);
                msg.bytes[0] = MIDI_NOTE_ON;
                msg.bytes[1] = note;
                msg.bytes[2] = vel;
                cvMidiOut->sendOutputMessage(msg);
                lastNote = note;
                lastNoteTimeout.timeout = LAST_NOTE_TIMEOUT;
*/
            }
            lastNoteTimeout.update();

            // handle octave switches
            if(octUpEdge.update((int)params[OCT_UP_SW].getValue())) {
                params[OCT_OFFSET].setValue(putils::clamp((int)params[OCT_OFFSET].getValue() + 1,
                    OCT_OFFSET_MIN, OCT_OFFSET_MAX));
            }
            if(octNormEdge.update((int)params[OCT_NORM_SW].getValue())) {
                params[OCT_OFFSET].setValue(0);
            }
            if(octDownEdge.update((int)params[OCT_DOWN_SW].getValue())) {
                params[OCT_OFFSET].setValue(putils::clamp((int)params[OCT_OFFSET].getValue() - 1,
                    OCT_OFFSET_MIN, OCT_OFFSET_MAX));
            }

            // run tasks for the repeater
            repeatHist.taskTimer();
        }
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / MIDI_RT_TASK_RATE));
    }

    // module initialize
    void onReset(void) override {
        lastNote = -1;
    }

    // get the CC base
    int getCCBase(void) {
        return (int)params[CC_BASE].getValue();
    }

    // set the CC base
    void setCCBase(int base) {
        params[CC_BASE].setValue(putils::clamp(base, 0, 120));
    }

    //
    // callbacks
    //
    // set the text for a label
    std::string updateLabel(int id) override {
        if(lastNoteTimeout.timeout) {
            if(lastNote == -1) {
                return "---";
            }
            return putils::format("N:%d", lastNote);
        }
        return putils::format("T:%d", (int)params[OCT_OFFSET].getValue());
    }

    // send an output message to a port - callback from repeater
    // returns -1 on error
    void sendMessage(const midi::Message& msg, int index) {
        midi::Message sendMsg;
        int note = (int)params[CC_BASE].getValue() + msg.bytes[1] +
            ((int)params[OCT_OFFSET].getValue() * 12);
        PDEBUG("note: %d - offset: %d", note, (int)params[OCT_OFFSET].getValue());
        if(note < 0 || note > 127) return;
        int vel = (int)((float)msg.bytes[2] * params[VEL_POT].getValue());
        sendMsg = MidiHelper::encodeNoteOnMessage(0, note, vel);
        cvMidiOut->sendOutputMessage(sendMsg);
        lastNote = note;
        lastNoteTimeout.timeout = LAST_NOTE_TIMEOUT;
    }
};

// handle choosing the CC base
struct MIDI_CC_NoteCCBaseMenuItem : MenuItem {
    MIDI_CC_Note *module;
    int base;

    MIDI_CC_NoteCCBaseMenuItem(Module *module, int base, std::string name) {
        this->module = dynamic_cast<MIDI_CC_Note*>(module);
        this->base = base;
        this->text = name;
        this->rightText = CHECKMARK(this->module->getCCBase() == base);
    }

    // the menu item was selected
    void onAction(const event::Action &e) override {
        this->module->setCCBase(base);
    }
};

struct MIDI_CC_NoteWidget : ModuleWidget {
	MIDI_CC_NoteWidget(MIDI_CC_Note* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MIDI_CC_Note.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        KilpatrickLabel *textField = new KilpatrickLabel(0, mm2px(Vec(10.16, 20.5)), mm2px(Vec(16.0, 8.0)));
        textField->id = 0;
        textField->rad = 1.0;
        textField->fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        textField->fontSize = 14.0;
        textField->text = "T:0";
        textField->hAlign = NVG_ALIGN_LEFT;
        textField->vAlign = NVG_ALIGN_MIDDLE;
        textField->bgColor = nvgRGBA(0x00, 0x00, 0x00, 0x00);
        textField->fgColor = nvgRGBA(0xee, 0xee, 0xee, 0xff);
        textField->handler = (KilpatrickLabelHandler*)module;
		addChild(textField);

		addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(10.16, 36.5)), module, MIDI_CC_Note::VEL_POT));
		addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(10.16, 52.5)), module, MIDI_CC_Note::OCT_UP_SW));
		addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(10.16, 66.5)), module, MIDI_CC_Note::OCT_NORM_SW));
		addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(10.16, 80.5)), module, MIDI_CC_Note::OCT_DOWN_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 94.5)), module, MIDI_CC_Note::MIDI_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_CC_Note::MIDI_OUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 90.15)), module, MIDI_CC_Note::MIDI_IN_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_CC_Note::MIDI_OUT_LED));
	}

    // add menu items
    void appendContextMenu(Menu *menu) override {
        MIDI_CC_Note *module = dynamic_cast<MIDI_CC_Note*>(this->module);
        if(!module) {
            return;
        }

        // CC bsae
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "CC Base");
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 0, "0"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 12, "12"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 24, "24"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 36, "36"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 48, "48"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 60, "60"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 72, "72"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 84, "84"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 96, "96"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 108, "108"));
        menuHelperAddItem(menu, new MIDI_CC_NoteCCBaseMenuItem(module, 120, "120"));
    }
};

Model* modelMIDI_CC_Note = createModel<MIDI_CC_Note, MIDI_CC_NoteWidget>("MIDI_CC_Note");
