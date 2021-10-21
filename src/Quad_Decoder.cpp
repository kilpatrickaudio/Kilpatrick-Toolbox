/*
 * Quad Decoder
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
#include "utils/DspUtils2.h"
#include "utils/KAComponents.h"
#include "utils/MenuHelper.h"
#include "utils/PUtils.h"

struct Quad_Decoder : Module {
	enum ParamId {
		OUTPUT_POT,
        FS_POT,
        MODE,  // selected mode index
		PARAMS_LEN
	};
	enum InputId {
		LT_IN,
		RT_IN,
		INPUTS_LEN
	};
	enum OutputId {
		FL_OUT,
		FR_OUT,
		SL_OUT,
		SR_OUT,
        SUB_OUT,
		MULTI_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LT_IN_LED,
		RT_IN_LED,
		FL_OUT_LED,
		FR_OUT_LED,
		SL_OUT_LED,
		SR_OUT_LED,
        SUB_OUT_LED,
        ENUMS(MULTI_OUT_LED, 3),
		LIGHTS_LEN
	};
    static constexpr int AUDIO_BUFLEN = 64;
    static constexpr int RT_TASK_RATE = 100;
    dsp::ClockDivider taskTimer;
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    static constexpr float AUDIO_OUT_GAIN = 10.0f;  // save mixing headroom
    dsp2::LevelLed ltInLed;
    dsp2::LevelLed rtInLed;
    dsp2::LevelLed flOutLed;
    dsp2::LevelLed frOutLed;
    dsp2::LevelLed slOutLed;
    dsp2::LevelLed srOutLed;
    dsp2::LevelLed subOutLed;
    dsp2::LevelLed multiOutLed;
    // matrix mixing
    enum Encoders {
        QS_MATRIX_DECODE,
        QS_LOGIC_DECODE,
        SQ_MATRIX_DECODE,
        SQ_LOGIC_DECODE,
        NUM_ENCODERS
    };
    dsp2::AudioBufferer *inBuf, *outBuf;

    // constructor
	Quad_Decoder() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OUTPUT_POT, 0.f, 1.f, 0.5f, "OUTPUT LEVEL");
        configParam(FS_POT, 0.f, 1.f, 0.5f, "FS BALANCE");
        configParam(MODE, 0.f, NUM_ENCODERS - 1, 0.0f, "MODE");
		configInput(LT_IN, "LT IN");
		configInput(RT_IN, "RT IN");
		configOutput(FL_OUT, "FL OUT");
		configOutput(FR_OUT, "FR OUT");
		configOutput(SL_OUT, "SL OUT");
		configOutput(SR_OUT, "SR OUT");
        configOutput(SUB_OUT, "SUB OUT");
		configOutput(MULTI_OUT, "MULTI OUT");
        onReset();
        onSampleRateChange();
        inBuf = new dsp2::AudioBufferer(AUDIO_BUFLEN, 2);
        outBuf = new dsp2::AudioBufferer(AUDIO_BUFLEN, 4);
	}

    // destructor
    ~Quad_Decoder() {
        delete inBuf;
        delete outBuf;
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        int i;
        float lt, rt, multiSum;
        float multiOut[4];
        float *inp, *outp;

        // run tasks
        if(taskTimer.process()) {
            lights[LT_IN_LED].setBrightness(ltInLed.getBrightness());
            lights[RT_IN_LED].setBrightness(rtInLed.getBrightness());
            lights[FL_OUT_LED].setBrightness(flOutLed.getBrightness());
            lights[FR_OUT_LED].setBrightness(frOutLed.getBrightness());
            lights[SL_OUT_LED].setBrightness(slOutLed.getBrightness());
            lights[SR_OUT_LED].setBrightness(srOutLed.getBrightness());
            lights[SUB_OUT_LED].setBrightness(subOutLed.getBrightness());
            lights[MULTI_OUT_LED + 2].setBrightness(multiOutLed.getBrightness());
        }

        // process
        outBuf->isFull();
        if(inBuf->isFull()) {
            switch((int)params[MODE].getValue()) {
                case QS_MATRIX_DECODE:
                    inp = inBuf->buf;
                    outp = outBuf->buf;
                    for(i = 0; i < AUDIO_BUFLEN; i ++) {
                        // get inputs
                        lt = *inp;
                        inp ++;
                        rt = *inp;
                        inp ++;
                        // output
                        *outp = lt + (rt * 0.414f);  // FL
                        outp ++;
                        *outp = rt + (lt * 0.414f);  // FR
                        outp ++;
                        *outp = 0.0f;
                        outp ++;
                        *outp = 0.0f;
                        outp ++;
                    }
                    break;
                case QS_LOGIC_DECODE:
                    break;
                case SQ_MATRIX_DECODE:
                    break;
                case SQ_LOGIC_DECODE:
                    break;
                default:
                    break;
            }
        }

        // shuffle inputs and outputs
        lt = inputs[LT_IN].getVoltage() * AUDIO_IN_GAIN;
        ltInLed.updateNormalized(lt);
        rt = inputs[RT_IN].getVoltage() * AUDIO_IN_GAIN;
        rtInLed.updateNormalized(rt);
        inBuf->addInSample(lt);
        inBuf->addInSample(rt);
        multiOut[0] = outBuf->getOutSample() * AUDIO_OUT_GAIN;  // FL
        multiOut[1] = outBuf->getOutSample() * AUDIO_OUT_GAIN;  // FR
        multiOut[2] = outBuf->getOutSample() * AUDIO_OUT_GAIN;  // SL
        multiOut[3] = outBuf->getOutSample() * AUDIO_OUT_GAIN;  // SR
        outputs[FL_OUT].setVoltage(multiOut[0]);
        outputs[FR_OUT].setVoltage(multiOut[1]);
        outputs[SL_OUT].setVoltage(multiOut[2]);
        outputs[SR_OUT].setVoltage(multiOut[3]);
        outputs[MULTI_OUT].writeVoltages(multiOut);
        flOutLed.update(multiOut[0]);
        frOutLed.update(multiOut[1]);
        slOutLed.update(multiOut[2]);
        srOutLed.update(multiOut[3]);
        multiSum = multiOut[0];
        multiSum += multiOut[1];
        multiSum += multiOut[2];
        multiSum += multiOut[3];
        multiOutLed.update(multiSum * 0.25f);
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
        ltInLed.onSampleRateChange();
        rtInLed.onSampleRateChange();
        flOutLed.onSampleRateChange();
        frOutLed.onSampleRateChange();
        slOutLed.onSampleRateChange();
        srOutLed.onSampleRateChange();
        subOutLed.onSampleRateChange();
        multiOutLed.onSampleRateChange();
    }

    // module initialize
    void onReset(void) override {
        lights[MULTI_OUT_LED + 0].setBrightness(0.0f);
        lights[MULTI_OUT_LED + 1].setBrightness(0.0f);
        params[MODE].setValue(QS_MATRIX_DECODE);
    }

    // gets the mode
    int getMode(void) {
        return (int)params[MODE].getValue();
    }

    // sets the mode
    void setMode(int mode) {
        params[MODE].setValue(mode);
    }
};

// handle choosing the decoding mode
struct QuadDecoderModeMenuItem : MenuItem {
    Quad_Decoder *module;
    int mode;

    QuadDecoderModeMenuItem(Module *module, int mode, std::string name) {
        this->module = dynamic_cast<Quad_Decoder*>(module);
        this->mode = mode;
        this->text = name;
        this->rightText = CHECKMARK(this->module->getMode() == mode);
    }

    // the menu item was selected
    void onAction(const event::Action &e) override {
        this->module->setMode(mode);
    }
};

struct Quad_DecoderWidget : ModuleWidget {
	Quad_DecoderWidget(Quad_Decoder* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Quad_Decoder.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(15.24, 24.5)), module, Quad_Decoder::OUTPUT_POT));
        addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(15.24, 42.5)), module, Quad_Decoder::FS_POT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.24, 62.5)), module, Quad_Decoder::LT_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.24, 62.5)), module, Quad_Decoder::RT_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.24, 78.5)), module, Quad_Decoder::FL_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.24, 78.5)), module, Quad_Decoder::FR_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.24, 94.5)), module, Quad_Decoder::SL_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.24, 94.5)), module, Quad_Decoder::SR_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.24, 110.5)), module, Quad_Decoder::SUB_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.24, 110.5)), module, Quad_Decoder::MULTI_OUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 57.208)), module, Quad_Decoder::LT_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 57.208)), module, Quad_Decoder::RT_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 73.208)), module, Quad_Decoder::FL_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 73.208)), module, Quad_Decoder::FR_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 89.208)), module, Quad_Decoder::SL_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 89.208)), module, Quad_Decoder::SR_OUT_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 105.208)), module, Quad_Decoder::SUB_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 105.208)), module, Quad_Decoder::MULTI_OUT_LED));
	}

    // add menu items to select which memory slot we use
    void appendContextMenu(Menu *menu) override {
        Quad_Decoder *module = dynamic_cast<Quad_Decoder*>(this->module);
        if(!module) {
            return;
        }

        // mode
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Decoding Mode");
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::QS_MATRIX_DECODE, "QS / Quark Matrix Decode"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::QS_LOGIC_DECODE, "QS / Quark Logic Decode"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::SQ_MATRIX_DECODE, "SQ Matrix Decode"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::SQ_LOGIC_DECODE, "SQ Logic Decode"));
    }
};

Model* modelQuad_Decoder = createModel<Quad_Decoder, Quad_DecoderWidget>("Quad_Decoder");
