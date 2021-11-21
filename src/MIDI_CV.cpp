/*
 * MIDI to CV - vMIDI CV Converter
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
#include "Midi2Note/Midi2Note.h"
#include "utils/CVMidi.h"
#include "utils/KAComponents.h"
#include "utils/MenuHelper.h"
#include "utils/MidiHelper.h"
#include "utils/MidiProtocol.h"
#include "utils/MidiCCMem.h"
#include "utils/PUtils.h"

struct MIDI_CV : Module {
	enum ParamIds {
		LEARN_SW,
		POLY_SW,
		MODE_SW,
        MAP_CC1,  // control for output 1 (CC mode)
        MAP_CC2,  // control for output 2 (CC mode)
        MAP_CC3,  // control for output 3 (CC mode)
        MAP_CHAN1,  // channel for output 1 (CC / note mode)
        MAP_CHAN2,  // channel for output 2 (CC mode)
        MAP_CHAN3,  // channel for output 3 (CC mode)
        BEND_RANGE,
		NUM_PARAMS
	};
	enum InputIds {
		MIDI_IN,
		NUM_INPUTS
	};
	enum OutputIds {
		P1_OUT,
		G2_OUT,
		V3_OUT,
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_IN_LED,
        P1_OUT_LED,
        G2_OUT_LED,
        V3_OUT_LED,
		NUM_LIGHTS
	};
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    MidiHelper *midi;
    MidiCCMem ccMem;
    Midi2Note midi2note;
    putils::PosEdgeDetect learnEdge;
    putils::ParamChangeDetect cvModeChange;
    putils::ParamChangeDetect polyChange;
    enum {
        LEARN_P1,
        LEARN_G2,
        LEARN_V3,
        LEARN_DISABLE
    };
    int learnMode;
    int learnTimeout;
    #define LEARN_TIMEOUT (MIDI_RT_TASK_RATE * 4)  // 2 seconds
    enum {
        CV_MODE_CC,
        CV_MODE_POLY,
        CV_MODE_MONO
    };
    enum {
        POLY_VOICE3,
        POLY_VOICE2,
        POLY_VOICE1
    };
    int timerDiv;
    dsp::ExponentialFilter valueFilters[NUM_OUTPUTS];
    float outputVals[NUM_OUTPUTS];
    putils::Pulser outputPulsers[NUM_OUTPUTS];
    putils::ParamChangeDetect outputChangeDetect[NUM_OUTPUTS];
    #define OUTPUT_LED_PULSE (MIDI_RT_TASK_RATE / 5)  // 200ms

    // constructor
	MIDI_CV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LEARN_SW, 0.0f, 1.0f, 0.0f, "LEARN");
		configParam(POLY_SW, 0.0f, 2.0f, 0.0f, "POLY");
		configParam(MODE_SW, 0.0f, 2.0f, 0.0f, "MODE");
        configParam(MAP_CC1, 0.0f, 127.0f, 0.0f, "CC1");
        configParam(MAP_CC2, 0.0f, 127.0f, 0.0f, "CC2");
        configParam(MAP_CC3, 0.0f, 127.0f, 0.0f, "CC3");
        configParam(MAP_CHAN1, 0.0f, 127.0f, 0.0f, "CHAN1");
        configParam(MAP_CHAN2, 0.0f, 127.0f, 0.0f, "CHAN2");
        configParam(MAP_CHAN3, 0.0f, 127.0f, 0.0f, "CHAN3");
        configInput(MIDI_IN, "MIDI IN");
        configOutput(P1_OUT, "P1 OUT");
        configOutput(G2_OUT, "G2 OUT");
        configOutput(V3_OUT, "V3 OUT");
        cvMidiIn = new CVMidi(&inputs[MIDI_IN], 1);
        ccMem.setTimeout(MIDI_RT_TASK_RATE * 2);  // 2 seconds
        timerDiv = 0;
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_CV() {
        delete cvMidiIn;
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        int i, temp;
        midi::Message msg;

        // handle CV MIDI
        cvMidiIn->process();

        // run tasks
        if(taskTimer.process()) {
            ccMem.process();

            // process MIDI
            while(cvMidiIn->getInputMessage(&msg)) {
                // handle CC messages - filter repeats
                if(MidiHelper::isControlChangeMessage(msg)) {
                    // ignore repeated messages
                    if(ccMem.handleCC(msg) != 0) {
                        continue;
                    }
                    // learn CC input when in learn mode
                    if(learnMode != LEARN_DISABLE) {
                        learn(msg);
                        continue;
                    }
                }
                // learn other messages
                else if(learnMode != LEARN_DISABLE) {
                    learn(msg);
                    continue;
                }

                // handle output modes
                switch((int)params[MODE_SW].getValue()) {
                    case CV_MODE_CC:
                        if(MidiHelper::isControlChangeMessage(msg)) {
                            // check each CC output
                            for(i = 0; i < 3; i ++) {
                                if(msg.bytes[1] == (int)params[MAP_CC1 + i].getValue() &&
                                        MidiHelper::getChannelMsgChannel(msg) == (int)params[MAP_CHAN1 + i].getValue()) {
                                    outputVals[i] = (putils::midi2float(msg.bytes[2]) * 10.0f) + -5.0f;
                                }
                            }
                        }
                        break;
                    case CV_MODE_MONO:
                        midi2note.handleMessage(msg);
                        outputVals[P1_OUT] = midi2note.getPitchVoltage(0);
                        outputVals[G2_OUT] = midi2note.getGateVoltage(0);
                        outputVals[V3_OUT] = midi2note.getVelocityVoltage(0);
                        break;
                    case CV_MODE_POLY:
                    default:
                        midi2note.handleMessage(msg);
                        temp = 2 - (int)params[POLY_SW].getValue();  // flip around poly switch values
                        outputVals[P1_OUT] = midi2note.getPitchVoltage(temp);
                        outputVals[G2_OUT] = midi2note.getGateVoltage(temp);
                        outputVals[V3_OUT] = midi2note.getVelocityVoltage(temp);
                        break;
                }
            }

            // handle learn button
            if(learnEdge.update((int)params[LEARN_SW].getValue())) {
                setLearnMode();
            }
            // handle change to CV mode
            if(cvModeChange.update(params[MODE_SW].getValue())) {
                setCVMode(params[MODE_SW].getValue());
            }
            // handle change to poly voices
            if(polyChange.update(params[POLY_SW].getValue())) {
                setPolyVoices(params[POLY_SW].getValue());
            }

            // learn mode handler
            if(learnMode != LEARN_DISABLE) {
                // blink learn LEDs
                if(timerDiv & 0x200) {
                    switch(learnMode) {
                        case LEARN_P1:
                            lights[P1_OUT_LED].setBrightness(1.0f);
                            break;
                        case LEARN_G2:
                            lights[G2_OUT_LED].setBrightness(1.0f);
                            break;
                        case LEARN_V3:
                            lights[V3_OUT_LED].setBrightness(1.0f);
                            break;
                    }
                }
                else {
                    lights[P1_OUT_LED].setBrightness(0.0f);
                    lights[G2_OUT_LED].setBrightness(0.0f);
                    lights[V3_OUT_LED].setBrightness(0.0f);
                }
            }
            // normal LED
            else {
                lights[MIDI_IN_LED].setBrightness(cvMidiIn->getLedState());
                // check if output changed
                for(i = 0; i < NUM_OUTPUTS; i ++) {
                    // special case for gate mode
                    if(i == G2_OUT && (int)params[MODE_SW].getValue() != CV_MODE_CC) {
                        if(outputVals[i] > 0.0f) {
                            lights[G2_OUT_LED].setBrightness(1.0f);
                        }
                        else {
                            lights[G2_OUT_LED].setBrightness(0.0f);
                        }
                        continue;
                    }
                    // output changed
                    if(outputChangeDetect[i].update(outputVals[i])) {
                        outputPulsers[i].timeout = OUTPUT_LED_PULSE;
                    }
                    // generate output pulse and timeout
                    lights[P1_OUT_LED + i].setBrightness(outputPulsers[i].update());
                }
            }
            // timeout learn mode
            if(learnTimeout) {
                learnTimeout --;
                if(learnTimeout == 0) {
                    learnMode = LEARN_DISABLE;
                }
            }
            timerDiv ++;
        }

        // CV outputs
        for(i = 0; i < NUM_OUTPUTS; i ++) {
            valueFilters[i].process(args.sampleTime, outputVals[i]);
            outputs[i].setVoltage(valueFilters[i].out);
        }
	}

    // set the learn mode
    void setLearnMode() {
        switch((int)params[MODE_SW].getValue()) {
            case CV_MODE_CC:
                learnMode ++;
                if(learnMode >= LEARN_DISABLE) {
                    learnMode = 0;
                }
                break;
            case CV_MODE_MONO:
            case CV_MODE_POLY:
            default:
                learnMode = LEARN_P1;  // force P1 always
                break;
        }
        learnTimeout = LEARN_TIMEOUT;
    }

    // set the CV mode
    void setCVMode(int mode) {
        int i;
        float filterCoeff;
        switch(mode) {
            case CV_MODE_MONO:
                midi2note.setPolyMode(0);
                midi2note.setChannel((int)params[MAP_CHAN1].getValue());
                filterCoeff = PITCH_GATE_SMOOTHING;
                break;
            case CV_MODE_POLY:
                midi2note.setPolyMode(1);
                midi2note.setChannel((int)params[MAP_CHAN1].getValue());
                filterCoeff = PITCH_GATE_SMOOTHING;
                break;
            default:  // CC mode
                midi2note.reset();
                filterCoeff = CC_CV_SMOOTHING;
                break;
        }
        // set up output filters
        for(i = 0; i < NUM_OUTPUTS; i ++) {
            valueFilters[i].setTau(filterCoeff);
            outputVals[i] = 0.0f;
        }
        params[MODE_SW].setValue(mode);
        learnMode = LEARN_DISABLE;
    }

    // set the number of poly voices
    void setPolyVoices(int voice) {
        params[POLY_SW].setValue(voice);
        learnMode = LEARN_DISABLE;
    }

    // learn the current input message
    void learn(const midi::Message& msg) {
        if(learnMode == LEARN_DISABLE) {
            return;
        }
        switch((int)params[MODE_SW].getValue()) {
            case CV_MODE_CC:  // map CC to CV
                if(MidiHelper::isControlChangeMessage(msg)) {
                    params[MAP_CC1 + learnMode].setValue(msg.bytes[1]);
                    params[MAP_CHAN1 + learnMode].setValue(MidiHelper::getChannelMsgChannel(msg));
                }
                break;
            case CV_MODE_MONO:
            case CV_MODE_POLY:
            default:
                if(MidiHelper::isChannelMessage(msg)) {
                    // store param so we can recall it later
                    params[MAP_CHAN1].setValue(MidiHelper::getChannelMsgChannel(msg));
                    midi2note.setChannel((int)params[MAP_CHAN1].getValue());
                }
                break;
        }
        learnMode = LEARN_DISABLE;
        learnTimeout = 0;
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
        ccMem.reset();
        learnMode = LEARN_DISABLE;
        learnTimeout = 0;
        setCVMode(CV_MODE_MONO);
        setPolyVoices(POLY_VOICE1);
    }

    // runs after param values have been restored
	void onAdd() override {
        // init internal code
        setCVMode(params[MODE_SW].getValue());
        setPolyVoices(params[POLY_SW].getValue());
        midi2note.setBendRange(params[BEND_RANGE].getValue());
    }

    // set the bend range
    void setBendRange(int range) {
        params[BEND_RANGE].setValue(range);
        midi2note.setBendRange(range);
    }
};

// handle choosing a pitch bend range
struct MIDI_CVBendRangeMenuItem : MenuItem {
    MIDI_CV *module;
    int range;

    MIDI_CVBendRangeMenuItem(Module *module, int range) {
        this->module = dynamic_cast<MIDI_CV*>(module);
        this->text = std::to_string(range) + " semitones";
        this->rightText = CHECKMARK(range == this->module->midi2note.getBendRange());
        this->range = range;
    }

    // the menu item was selected
    void onAction(const event::Action &e) override {
        this->module->setBendRange(range);
    }
};

struct MIDI_CVWidget : ModuleWidget {
	MIDI_CVWidget(MIDI_CV* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_CV.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<KilpatrickD6RRedButton>(mm2px(Vec(10.16, 36.5)), module, MIDI_CV::LEARN_SW));
		addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(10.16, 52)), module, MIDI_CV::POLY_SW));
		addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(10.16, 68)), module, MIDI_CV::MODE_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 22.5)), module, MIDI_CV::MIDI_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 84.5)), module, MIDI_CV::P1_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 96.5)), module, MIDI_CV::G2_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 108.5)), module, MIDI_CV::V3_OUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 18.15)), module, MIDI_CV::MIDI_IN_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 78.15)), module, MIDI_CV::P1_OUT_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 90.15)), module, MIDI_CV::G2_OUT_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 102.15)), module, MIDI_CV::V3_OUT_LED));
	}

    // add menu items
    void appendContextMenu(Menu *menu) override {
        MIDI_CV *module = dynamic_cast<MIDI_CV*>(this->module);
        if(!module) {
            return;
        }

        // module settings
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Pitch Bend Range");
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 1));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 2));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 3));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 4));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 5));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 6));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 7));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 8));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 9));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 10));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 11));
        menuHelperAddItem(menu, new MIDI_CVBendRangeMenuItem(module, 12));
    }
};

Model* modelMIDI_CV = createModel<MIDI_CV, MIDI_CVWidget>("MIDI_CV");
