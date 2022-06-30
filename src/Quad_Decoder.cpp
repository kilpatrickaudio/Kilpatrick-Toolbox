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
        SUB_CUTOFF,  // sub cutoff index
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
    static constexpr int RT_TASK_DIVIDER = 10;
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
//        SQ_LOGIC_DECODE,
        NUM_ENCODERS
    };
    enum SubCutoffs {
        CUTOFF_BYPASS,
        CUTOFF_60,
        CUTOFF_70,
        CUTOFF_80,
        CUTOFF_90,
        CUTOFF_100,
        CUTOFF_110,
        CUTOFF_120,
        NUM_CUTOFFS
    };
    dsp2::AllpassPhaseShifter flShifter;
    dsp2::AllpassPhaseShifter frShifter;
    dsp2::AllpassPhaseShifter slShifter;
    dsp2::AllpassPhaseShifter srShifter;
    dsp2::AudioBufferer *inBuf, *outBuf;
    static constexpr float LFILT_CUTOFF = 0.5f;
    static constexpr float LOGIC_FADE = 3.5f;
    dsp2::Filter1Pole logicFilt1;
    dsp2::Filter1Pole logicFilt2;
    dsp2::Filter2Pole subFilt1;
    dsp2::Filter2Pole subFilt2;
    float outLevel, frontLevel, surroundLevel;

    // constructor
	Quad_Decoder() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OUTPUT_POT, 0.f, 1.f, 1.0f, "OUTPUT LEVEL");
        configParam(FS_POT, 0.f, 1.f, 0.5f, "FS BALANCE");
        configParam(MODE, 0.f, NUM_ENCODERS - 1, 0.0f, "MODE");
        configParam(SUB_CUTOFF, 0.0f, NUM_CUTOFFS - 1, 0.0f, "SUB CUTOFF");
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
        float multiOut[8];
        float *inp, *outp;
        float fl, fr, sl, sr;
        float flDel, flShift, frDel, frShift;
        float slDel, slShift, srDel, srShift;
        float logA, logB, flSrMix, frSlMix;

        // run tasks
        if(taskTimer.process()) {
            // set multi output channels to 5
            if(outputs[MULTI_OUT].isConnected() && outputs[MULTI_OUT].getChannels() != 5) {
                outputs[MULTI_OUT].setChannels(5);
            }
            lights[LT_IN_LED].setBrightness(ltInLed.getBrightness());
            lights[RT_IN_LED].setBrightness(rtInLed.getBrightness());
            lights[FL_OUT_LED].setBrightness(flOutLed.getBrightness());
            lights[FR_OUT_LED].setBrightness(frOutLed.getBrightness());
            lights[SL_OUT_LED].setBrightness(slOutLed.getBrightness());
            lights[SR_OUT_LED].setBrightness(srOutLed.getBrightness());
            lights[SUB_OUT_LED].setBrightness(subOutLed.getBrightness());
            lights[MULTI_OUT_LED + 2].setBrightness(multiOutLed.getBrightness());

            outLevel = params[OUTPUT_POT].getValue() * 2.0f;
            surroundLevel = params[FS_POT].getValue();
            frontLevel = 1.0f - surroundLevel;
        }

        // process
        outBuf->isFull();
        if(inBuf->isFull()) {
            switch((int)params[MODE].getValue()) {
                case QS_MATRIX_DECODE:
                    inp = inBuf->buf;
                    outp = outBuf->buf;
                    for(i = 0; i < AUDIO_BUFLEN; i ++) {
                        // inputs
                        lt = *inp;
                        inp ++;
                        rt = *inp;
                        inp ++;

                        fl = lt;

                        // matrix
                        fl = lt + (rt * 0.414f);
                        fr = rt + (lt * 0.414f);
                        sl = lt + (rt * -0.414f);
                        sr = -rt + (lt * 0.414f);

                        // phase shifters
                        flShifter.process(fl, &flDel, &flShift);
                        frShifter.process(fr, &frDel, &frShift);
                        slShifter.process(sl, &slDel, &slShift);
                        srShifter.process(sr, &srDel, &srShift);

                        *outp = flDel;  // FL
                        outp ++;
                        *outp = frDel;  // FR
                        outp ++;
                        *outp = -slShift;  // SL
                        outp ++;
                        *outp = -srShift;  // SR
                        outp ++;
                    }
                    break;
                case QS_LOGIC_DECODE:
                    inp = inBuf->buf;
                    outp = outBuf->buf;
                    for(i = 0; i < AUDIO_BUFLEN; i ++) {
                        // inputs
                        lt = *inp;
                        inp ++;
                        rt = *inp;
                        inp ++;

                        fl = lt;

                        // matrix
                        fl = lt + (rt * 0.414f);
                        fr = rt + (lt * 0.414f);
                        sl = lt + (rt * -0.414f);
                        sr = -rt + (lt * 0.414f);

                        //
                        // logic
                        //
                        // FL/SR
                        logA = atanf(dsp2::abs(fl) * 15.0f) * 0.625f;
                        logB = atanf(dsp2::abs(sr) * 15.0f) * -0.625f;
                        flSrMix = dsp2::clamp(logicFilt1.lowpass(logA + logB));
                        fl += fl * (flSrMix * LOGIC_FADE);
                        sr += sr * (-flSrMix * LOGIC_FADE);

                        // FR/SL
                        logA = atanf(dsp2::abs(fr) * 15.0f) * 0.625f;
                        logB = atanf(dsp2::abs(sl) * 15.0f) * -0.625f;
                        frSlMix = dsp2::clamp(logicFilt2.lowpass(logA + logB));
                        fr += fr * (frSlMix * LOGIC_FADE);
                        sl += sl * (-frSlMix * LOGIC_FADE);

                        // phase shifters
                        flShifter.process(fl, &flDel, &flShift);
                        frShifter.process(fr, &frDel, &frShift);
                        slShifter.process(sl, &slDel, &slShift);
                        srShifter.process(sr, &srDel, &srShift);

                        // outputs
                        *outp = flDel;  // FL
                        outp ++;
                        *outp = frDel;  // FR
                        outp ++;
                        *outp = -slShift;  // SL
                        outp ++;
                        *outp = -srShift;  // SR
                        outp ++;
                    }
                    outputs[SUB_OUT].setVoltage(flSrMix);
                    break;
                case SQ_MATRIX_DECODE:
                    inp = inBuf->buf;
                    outp = outBuf->buf;
                    for(i = 0; i < AUDIO_BUFLEN; i ++) {
                        // input / phase shifters
                        flShifter.process(*inp, &flDel, &flShift);
                        inp ++;
                        frShifter.process(*inp, &frDel, &frShift);
                        inp ++;

                        // matrix
                        *outp = flDel;  // FL
                        outp ++;
                        *outp = frDel;  // FR
                        outp ++;
                        *outp = (flDel * -0.707f) + (frShift * -0.707f);  // SL
                        outp ++;
                        *outp = (frDel * 0.707f) + (flShift * 0.707f);  // SR
                        outp ++;
                    }
                    break;
//                case SQ_LOGIC_DECODE:
//                    break;
            }
        }

        // shuffle inputs and outputs
        lt = inputs[LT_IN].getVoltage() * AUDIO_IN_GAIN;
        ltInLed.updateNormalized(lt);
        rt = inputs[RT_IN].getVoltage() * AUDIO_IN_GAIN;
        rtInLed.updateNormalized(rt);
        inBuf->addInSample(lt);
        inBuf->addInSample(rt);
        multiOut[0] = outBuf->getOutSample() * AUDIO_OUT_GAIN * frontLevel * outLevel;  // FL
        multiOut[1] = outBuf->getOutSample() * AUDIO_OUT_GAIN * frontLevel * outLevel;  // FR
        multiOut[2] = outBuf->getOutSample() * AUDIO_OUT_GAIN * surroundLevel * outLevel;  // SL
        multiOut[3] = outBuf->getOutSample() * AUDIO_OUT_GAIN * surroundLevel * outLevel;  // SR
        multiOut[4] = subFilt1.process(subFilt2.process((multiOut[0] + multiOut[1] + multiOut[2] + multiOut[3]) * 0.25f));;
        outputs[FL_OUT].setVoltage(multiOut[0]);
        outputs[FR_OUT].setVoltage(multiOut[1]);
        outputs[SL_OUT].setVoltage(multiOut[2]);
        outputs[SR_OUT].setVoltage(multiOut[3]);
        outputs[SUB_OUT].setVoltage(multiOut[4]);
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

    // module was added
    void onAdd(void) override {
        setSubCutoff((int)params[SUB_CUTOFF].getValue());
    }

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / (RT_TASK_RATE / RT_TASK_DIVIDER)));
        ltInLed.onSampleRateChange();
        rtInLed.onSampleRateChange();
        flOutLed.onSampleRateChange();
        frOutLed.onSampleRateChange();
        slOutLed.onSampleRateChange();
        srOutLed.onSampleRateChange();
        subOutLed.onSampleRateChange();
        multiOutLed.onSampleRateChange();
        logicFilt1.setCutoff(LFILT_CUTOFF, APP->engine->getSampleRate());
        logicFilt2.setCutoff(LFILT_CUTOFF, APP->engine->getSampleRate());
        setSubCutoff((int)params[SUB_CUTOFF].getValue());
    }

    // module initialize
    void onReset(void) override {
        lights[MULTI_OUT_LED + 0].setBrightness(0.0f);
        lights[MULTI_OUT_LED + 1].setBrightness(0.0f);
        params[MODE].setValue(QS_MATRIX_DECODE);
        outLevel = 0.0f;
        frontLevel = 0.0f;
        surroundLevel = 0.0f;
    }

    // get the mode
    int getMode(void) {
        return (int)params[MODE].getValue();
    }

    // set the mode
    void setMode(int mode) {
        params[MODE].setValue(mode);
    }

    // get the sub cutoff
    int getSubCutoff() {
        return (int)params[SUB_CUTOFF].getValue();
    }

    // set the sub cutoff
    void setSubCutoff(int cutoff) {
        float freq;
        params[SUB_CUTOFF].setValue(cutoff);
        switch(cutoff) {
            case CUTOFF_60:
                freq = 60.0f;
                break;
            case CUTOFF_70:
                freq = 70.0f;
                break;
            case CUTOFF_80:
                freq = 80.0f;
                break;
            case CUTOFF_90:
                freq = 90.0f;
                break;
            case CUTOFF_100:
                freq = 100.0f;
                break;
            case CUTOFF_110:
                freq = 110.0f;
                break;
            case CUTOFF_120:
                freq = 120.0f;
                break;
            case CUTOFF_BYPASS:
            default:
                freq = 20000.0f;
                break;
        }
        subFilt1.setCutoff(dsp2::Filter2Pole::TYPE_LPF, freq, 0.707f,
            1.0f, APP->engine->getSampleRate());
        subFilt2.setCutoff(dsp2::Filter2Pole::TYPE_LPF, freq, 0.707f,
            1.0f, APP->engine->getSampleRate());
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

// handle choosing the sub cutoff mode
struct QuadDecoderSubCutoffMenuItem : MenuItem {
    Quad_Decoder *module;
    int cutoff;

    QuadDecoderSubCutoffMenuItem(Module *module, int cutoff, std::string name) {
        this->module = dynamic_cast<Quad_Decoder*>(module);
        this->cutoff = cutoff;
        this->text = name;
        this->rightText = CHECKMARK(this->module->getSubCutoff() == cutoff);
    }

    // the menu item was selected
    void onAction(const event::Action &e) override {
        this->module->setSubCutoff(cutoff);
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

    // add menu items
    void appendContextMenu(Menu *menu) override {
        Quad_Decoder *module = dynamic_cast<Quad_Decoder*>(this->module);
        if(!module) {
            return;
        }

        // mode
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Decoding Mode");
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::QS_MATRIX_DECODE, "QS / Quark Matrix Decode"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::QS_LOGIC_DECODE, "QS / Quark Logic Decode (Experimental)"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::SQ_MATRIX_DECODE, "SQ Matrix Decode (Experimental)"));
//        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::SQ_LOGIC_DECODE, "SQ Logic Decode (Experimental)"));

        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Sub Cutoff");
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_BYPASS, "Bypass"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_60, "60Hz"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_70, "70Hz"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_80, "80Hz"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_90, "90Hz"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_100, "100Hz"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_110, "110Hz"));
        menuHelperAddItem(menu, new QuadDecoderSubCutoffMenuItem(module, Quad_Decoder::CUTOFF_120, "120Hz"));
    }
};

Model* modelQuad_Decoder = createModel<Quad_Decoder, Quad_DecoderWidget>("Quad_Decoder");
