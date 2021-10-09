/*
 * MIDI Merger / Filter - vMIDI Merger
 *
 * Copyright 2021: Kilpatrick Audio
 * Written by: Andrew Kilpatrick
 *
 */
#include "plugin.hpp"
#include "utils/CVMidi.h"
#include "utils/KAComponents.h"
#include "utils/MidiHelper.h"
#include "utils/PUtils.h"

struct MIDI_Merger : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		MIDI_IN1,
		MIDI_IN2,
		MIDI_IN3,
		MIDI_IN4,
		NUM_INPUTS
	};
	enum OutputIds {
		MIDI_OUT1,  // channel messages
		MIDI_OUT2,  // system messages
		MIDI_OUT3,  // all messages
		NUM_OUTPUTS
	};
	enum LightIds {
        MIDI_IN1_LED,
        MIDI_IN2_LED,
        MIDI_IN3_LED,
        MIDI_IN4_LED,
        MIDI_OUT1_LED,
        MIDI_OUT2_LED,
        MIDI_OUT3_LED,
		NUM_LIGHTS
	};

    #define NUM_INPUTS 4
    #define NUM_OUTPUTS 3
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIns[NUM_INPUTS];
    CVMidi *cvMidiOuts[NUM_OUTPUTS];

    // constructor
	MIDI_Merger() {
        int port;
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for(port = 0; port < NUM_INPUTS; port ++) {
            cvMidiIns[port] = new CVMidi(&inputs[MIDI_IN1 + port], 1);
        }
        for(port = 0; port < NUM_OUTPUTS; port ++) {
            cvMidiOuts[port] = new CVMidi(&outputs[MIDI_OUT1 + port], 0);
        }
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Merger() {
        int port;
        for(port = 0; port < NUM_INPUTS; port ++) {
            delete cvMidiIns[port];
        }
        for(port = 0; port < NUM_OUTPUTS; port ++) {
            delete cvMidiOuts[port];
        }
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        midi::Message msg;
        int port;

        // handle CV MIDI
        for(port = 0; port < NUM_INPUTS; port ++) {
            cvMidiIns[port]->process();
        }
        for(port = 0; port < NUM_OUTPUTS; port ++) {
            cvMidiOuts[port]->process();
        }

        // run tasks
        if(taskTimer.process()) {
            // check inputs
            for(port = 0; port < NUM_INPUTS; port ++) {
                // input messages
                while(cvMidiIns[port]->getInputMessage(&msg)) {
                    if(MidiHelper::isChannelMessage(msg)) {
                        cvMidiOuts[MIDI_OUT1]->sendOutputMessage(msg);
                        cvMidiOuts[MIDI_OUT3]->sendOutputMessage(msg);
                    }
                    if(MidiHelper::isSystemCommonMessage(msg) ||
                            MidiHelper::isSystemRealtimeMessage(msg)) {
                        cvMidiOuts[MIDI_OUT2]->sendOutputMessage(msg);
                        cvMidiOuts[MIDI_OUT3]->sendOutputMessage(msg);
                    }
                    // XXX handle SYSEX messages
                }
                // MIDI in LEDs
                lights[MIDI_IN1_LED + port].setBrightness(cvMidiIns[port]->getLedState());
            }

            // handle outputs
            for(port = 0; port < NUM_OUTPUTS; port ++) {
                lights[MIDI_OUT1_LED + port].setBrightness(cvMidiOuts[port]->getLedState());
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
    }
};

struct MIDI_MergerWidget : ModuleWidget {
	MIDI_MergerWidget(MIDI_Merger* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MIDI_Merger.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 24.5)), module, MIDI_Merger::MIDI_IN1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 36.5)), module, MIDI_Merger::MIDI_IN2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 48.5)), module, MIDI_Merger::MIDI_IN3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 60.5)), module, MIDI_Merger::MIDI_IN4));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 90.5)), module, MIDI_Merger::MIDI_OUT1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 102.5)), module, MIDI_Merger::MIDI_OUT2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 114.5)), module, MIDI_Merger::MIDI_OUT3));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 18.15)), module, MIDI_Merger::MIDI_IN1_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 30.15)), module, MIDI_Merger::MIDI_IN2_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 42.15)), module, MIDI_Merger::MIDI_IN3_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 54.15)), module, MIDI_Merger::MIDI_IN4_LED));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 84.15)), module, MIDI_Merger::MIDI_OUT1_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 96.15)), module, MIDI_Merger::MIDI_OUT2_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.81, 108.15)), module, MIDI_Merger::MIDI_OUT3_LED));
	}
};

Model* modelMIDI_Merger = createModel<MIDI_Merger, MIDI_MergerWidget>("MIDI_Merger");
