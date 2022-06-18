/*
 * Quad Encoder
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

struct Quad_Encoder : Module {
	enum ParamId {
        OUTPUT_POT,
		MODE,  // selected mode index
		PARAMS_LEN
	};
	enum InputId {
		FL_IN,
		FR_IN,
		SL_IN,
		SR_IN,
		MULTI_A_IN,
		MULTI_B_IN,
		INPUTS_LEN
	};
	enum OutputId {
		LT_OUT,
		RT_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FL_IN_LED,
		FR_IN_LED,
		SL_IN_LED,
		SR_IN_LED,
		LT_OUT_LED,
		RT_OUT_LED,
        ENUMS(MULTI_A_IN_LED, 3),
        ENUMS(MULTI_B_IN_LED, 3),
		LIGHTS_LEN
	};
    static constexpr int AUDIO_BUFLEN = 64;
//    static constexpr int RT_TASK_RATE = 100;
    static constexpr int RT_TASK_DIVIDER = 40;
    dsp::ClockDivider taskTimer;
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    static constexpr float AUDIO_OUT_GAIN = 10.0f;  // save mixing headroom
    static constexpr float PEAK_METER_SMOOTHING = 10.0f;
    static constexpr float PEAK_METER_PEAK_HOLD_TIME = 0.1f;
    dsp2::LevelLed flInLed;
    dsp2::LevelLed frInLed;
    dsp2::LevelLed slInLed;
    dsp2::LevelLed srInLed;
    dsp2::LevelLed ltOutLed;
    dsp2::LevelLed rtOutLed;
    dsp2::LevelLed multiAInLed;
    dsp2::LevelLed multiBInLed;
    // matrix mixing
    enum Encoders {
        QS_ENCODE,
        SQ_ENCODE,
        NUM_ENCODERS
    };
    dsp2::AllpassPhaseShifter flShifter;
    dsp2::AllpassPhaseShifter frShifter;
    dsp2::AllpassPhaseShifter slShifter;
    dsp2::AllpassPhaseShifter srShifter;
    dsp2::Filter2Pole hpfFl;
    dsp2::Filter2Pole hpfFr;
    dsp2::Filter2Pole hpfSl;
    dsp2::Filter2Pole hpfSr;
    dsp2::Filter2Pole hpfMultiA;
    dsp2::Filter2Pole hpfMultiB;
    dsp2::AudioBufferer *inBuf, *outBuf;
    float outLevel;

    // constructor
	Quad_Encoder() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OUTPUT_POT, 0.f, 1.f, 0.5f, "OUTPUT LEVEL");
        configParam(MODE, 0.f, NUM_ENCODERS - 1, 0.0f, "MODE");
		configInput(FL_IN, "FL IN");
		configInput(FR_IN, "FR IN");
		configInput(SL_IN, "SL IN");
		configInput(SR_IN, "SR IN");
		configInput(MULTI_A_IN, "MULTI A IN");
		configInput(MULTI_B_IN, "MULTI B IN");
		configOutput(LT_OUT, "LT OUT");
		configOutput(RT_OUT, "RT OUT");
        onReset();
        onSampleRateChange();
        inBuf = new dsp2::AudioBufferer(AUDIO_BUFLEN, 4);
        outBuf = new dsp2::AudioBufferer(AUDIO_BUFLEN, 2);
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        float fl, fr, sl, sr, multiSumA, multiSumB, tempf;
        float flDel, flShift, frDel, frShift;
        float slDel, slShift, srDel, srShift;
        float *inp, *outp;
        int i;

        // run tasks
        if(taskTimer.process()) {
            lights[FL_IN_LED].setBrightness(flInLed.getBrightness());
            lights[FR_IN_LED].setBrightness(frInLed.getBrightness());
            lights[SL_IN_LED].setBrightness(slInLed.getBrightness());
            lights[SR_IN_LED].setBrightness(srInLed.getBrightness());
            lights[MULTI_A_IN_LED + 2].setBrightness(multiAInLed.getBrightness());
            lights[MULTI_B_IN_LED + 2].setBrightness(multiBInLed.getBrightness());
            lights[LT_OUT_LED].setBrightness(ltOutLed.getBrightness());
            lights[RT_OUT_LED].setBrightness(rtOutLed.getBrightness());
            outLevel = params[OUTPUT_POT].getValue();
        }

        // process
        outBuf->isFull();
        if(inBuf->isFull()) {
            inp = inBuf->buf;
            outp = outBuf->buf;
            switch((int)params[MODE].getValue()) {
                case QS_ENCODE:
                    for(i = 0; i < AUDIO_BUFLEN; i ++) {
                        // QS encode (AES paper)
                        // confirmed identical to Quark output
                        // inputs
                        fl = *inp;
                        inp ++;
                        fr = *inp;
                        inp ++;
                        sl = *inp;
                        inp ++;
                        sr = *inp;
                        inp ++;

                        // phase shifters
                        flShifter.process(fl, &flDel, &flShift);
                        frShifter.process(fr, &frDel, &frShift);
                        slShifter.process(sl, &slDel, &slShift);
                        srShifter.process(sr, &srDel, &srShift);

                        // outputs
                        *outp = flDel + (frDel * 0.414f) + slShift + (srShift * 0.414f); // LT
                        outp ++;
                        *outp = (flDel * 0.414f) + frDel + (slShift * -0.414f) + (srShift * -1.0f);  // RT
                        outp ++;
                    }
                    break;
                case SQ_ENCODE:
                    for(i = 0; i < AUDIO_BUFLEN; i ++) {
                        // SQ basic encode (wikipedia)
                        // seems to work correctly with Sony SQD-2050
                        // inputs
                        fl = *inp;
                        inp ++;
                        fr = *inp;
                        inp ++;
                        sl = *inp;
                        inp ++;
                        sr = *inp;
                        inp ++;

                        // phase shifters
                        flShifter.process(fl, &flDel, &flShift);
                        frShifter.process(fr, &frDel, &frShift);
                        slShifter.process(sl, &slDel, &slShift);
                        srShifter.process(sr, &srDel, &srShift);

                        // outputs
                        *outp = flDel + (-slShift * 0.707f) + (srDel * 0.707f);
                        outp ++;
                        *outp = frDel + (-slDel * 0.707f) + (srShift * 0.707f);
                        outp ++;

                    }
                    break;
            }
        }

        // get inputs
        fl = inputs[FL_IN].getVoltage();
        tempf = inputs[MULTI_A_IN].getPolyVoltage(0);
        multiSumA = tempf;
        fl += tempf;
        tempf = inputs[MULTI_B_IN].getPolyVoltage(0);
        multiSumB = tempf;
        fl += tempf;
        fl *= AUDIO_IN_GAIN;  // normalize level
        fl = hpfFl.process(fl);
        flInLed.updateNormalized(fl);

        fr = inputs[FR_IN].getVoltage();
        tempf = inputs[MULTI_A_IN].getPolyVoltage(1);
        multiSumA += tempf;
        fr += tempf;
        tempf = inputs[MULTI_B_IN].getPolyVoltage(1);
        multiSumB += tempf;
        fr += tempf;
        fr *= AUDIO_IN_GAIN;  // normalize level
        fr = hpfFr.process(fr);
        frInLed.updateNormalized(fr);

        sl = inputs[SL_IN].getVoltage();
        tempf = inputs[MULTI_A_IN].getPolyVoltage(2);
        multiSumA += tempf;
        sl += tempf;
        tempf = inputs[MULTI_B_IN].getPolyVoltage(2);
        multiSumB += tempf;
        sl += tempf;
        sl *= AUDIO_IN_GAIN;  // normalize level
        sl = hpfSl.process(sl);
        slInLed.updateNormalized(sl);

        sr = inputs[SR_IN].getVoltage();
        tempf = inputs[MULTI_A_IN].getPolyVoltage(3);
        multiSumA += tempf;
        sr += tempf;
        tempf = inputs[MULTI_B_IN].getPolyVoltage(3);
        multiSumB += tempf;
        sr += tempf;
        sr *= AUDIO_IN_GAIN;  // normalize level
        sr = hpfSr.process(sr);
        srInLed.updateNormalized(sr);

        multiSumA = hpfMultiA.process(multiSumA * 0.25f);
        multiAInLed.update(multiSumA);
        multiSumB = hpfMultiB.process(multiSumB * 0.25f);
        multiBInLed.update(multiSumB);

        // shuffle inputs and outputs
        inBuf->addInSample(fl);
        inBuf->addInSample(fr);
        inBuf->addInSample(sl);
        inBuf->addInSample(sr);
        tempf = outBuf->getOutSample();
        outputs[LT_OUT].setVoltage(tempf * AUDIO_OUT_GAIN * outLevel);
        ltOutLed.updateNormalized(tempf);
        tempf = outBuf->getOutSample();
        outputs[RT_OUT].setVoltage(tempf * AUDIO_OUT_GAIN * outLevel);
        rtOutLed.updateNormalized(tempf);
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / (RT_TASK_RATE / RT_TASK_DIVIDER)));
        hpfFl.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        hpfFr.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        hpfSl.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        hpfSr.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        hpfMultiA.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        hpfMultiB.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        flInLed.onSampleRateChange();
        frInLed.onSampleRateChange();
        slInLed.onSampleRateChange();
        srInLed.onSampleRateChange();
        ltOutLed.onSampleRateChange();
        rtOutLed.onSampleRateChange();
        multiAInLed.onSampleRateChange();
        multiBInLed.onSampleRateChange();
    }

    // module initialize
    void onReset(void) override {
        lights[MULTI_A_IN_LED + 0].setBrightness(0.0f);
        lights[MULTI_A_IN_LED + 1].setBrightness(0.0f);
        lights[MULTI_B_IN_LED + 0].setBrightness(0.0f);
        lights[MULTI_B_IN_LED + 1].setBrightness(0.0f);
        params[MODE].setValue(QS_ENCODE);
        outLevel = 0.0f;
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

// handle choosing the encoding mode
struct QuadEncoderModeMenuItem : MenuItem {
    Quad_Encoder *module;
    int mode;

    QuadEncoderModeMenuItem(Module *module, int mode, std::string name) {
        this->module = dynamic_cast<Quad_Encoder*>(module);
        this->mode = mode;
        this->text = name;
        this->rightText = CHECKMARK(this->module->getMode() == mode);
    }

    // the menu item was selected
    void onAction(const event::Action &e) override {
        this->module->setMode(mode);
    }
};

struct Quad_EncoderWidget : ModuleWidget {
	Quad_EncoderWidget(Quad_Encoder* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Quad_Encoder.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(15.24, 24.5)), module, Quad_Encoder::OUTPUT_POT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.24, 44.5)), module, Quad_Encoder::FL_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.24, 44.5)), module, Quad_Encoder::FR_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.24, 60.5)), module, Quad_Encoder::SL_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.24, 60.5)), module, Quad_Encoder::SR_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.24, 78.5)), module, Quad_Encoder::MULTI_A_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.24, 78.5)), module, Quad_Encoder::MULTI_B_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 94.5)), module, Quad_Encoder::LT_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 110.5)), module, Quad_Encoder::RT_OUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 39.208)), module, Quad_Encoder::FL_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 39.208)), module, Quad_Encoder::FR_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 55.208)), module, Quad_Encoder::SL_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 55.208)), module, Quad_Encoder::SR_IN_LED));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(12.74, 73.208)), module, Quad_Encoder::MULTI_A_IN_LED));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(17.74, 73.208)), module, Quad_Encoder::MULTI_B_IN_LED));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(24.24, 94.5)), module, Quad_Encoder::LT_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(24.24, 110.5)), module, Quad_Encoder::RT_OUT_LED));
	}

    // add menu items
    void appendContextMenu(Menu *menu) override {
        Quad_Encoder *module = dynamic_cast<Quad_Encoder*>(this->module);
        if(!module) {
            return;
        }

        // mode
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Encoding Mode");
        menuHelperAddItem(menu, new QuadEncoderModeMenuItem(module, Quad_Encoder::QS_ENCODE, "QS / Quark Encode"));
        menuHelperAddItem(menu, new QuadEncoderModeMenuItem(module, Quad_Encoder::SQ_ENCODE, "SQ Encode"));
    }
};

Model* modelQuad_Encoder = createModel<Quad_Encoder, Quad_EncoderWidget>("Quad_Encoder");
