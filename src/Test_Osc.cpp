/*
 * Test Oscillator
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
#include "utils/PUtils.h"

// test osc display source
struct TestOscDisplaySource {
    // get the reference level in dB
    virtual float dispGetRefLevel(void) { return 0.0f; }

    // get the absolute level in dB
    virtual float dispGetAbsLevel(void) { return 0.0f; }

    // get the frequency in Hz
    virtual float dispGetFrequency(void) { return 0.0f; }

    // get the sweep time in s
    virtual float dispGetSweepTime(void) { return 0.0f; }

    // get the sweep progress - 0.0 to 1.0 = 0-100%
    virtual float dispGetSweepProgress(void) { return 0.0f; }

    // handle scroll on the label
    virtual void dispOnHoverScroll(int id, const event::HoverScroll& e) {}
};

float Test_Osc_freqs[] = {
    20.0, 25.0, 31.5, 40.0, 50.0, 63.0, 80.0, 100.0,
    125.0, 160.0, 200.0, 250.0, 315.0, 400.0, 500.0, 630.0,
    800.0, 1000.0, 1250.0, 1600.0, 2000.0, 2500.0, 3150.0, 4000.0,
    5000.0, 6300.0, 8000.0, 10000.0, 12500.0, 16000.0, 20000.0
};

struct Test_Osc : Module, TestOscDisplaySource {
	enum ParamIds {
		LEVEL_ENC,
        FREQ_ENC,
		SPEED_ENC,
		TONE_SW,
		SWEEP_SW,
		STEP_SW,
		TRIG_SW,
		ON_SW,
        // settings (not controls)
        FREQ_INDEX,  // fixed freq index
        ABS_LEVEL,  // output level
        REF_LEVEL,  // reference level offset
        SPEED,  // sweep speed in seconds
		NUM_PARAMS
	};
	enum InputIds {
		TRIG_IN,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT,
		NUM_OUTPUTS
	};
	enum LightIds {
        TRIG_IN_LED,
		NUM_LIGHTS
	};
    enum {
        TONE_PINK = 0,
        TONE_WHITE,
        TONE_SINE
    };
    enum {
        SWEEP_OFF = 0,
        SWEEP_LOG,
        SWEEP_LIN
    };
    enum {
        STEP_LOG = 0,
        STEP_OCT,
        STEP_THIRD
    };
    enum {
        ON_MOM = 0,
        ON_ON
    };
    static constexpr int NUM_FREQS = 31;
    static constexpr int RT_TASK_RATE = 1000;
    static constexpr float AUDIO_OUT_GAIN = 10.0f;
    static constexpr float SWEEP_START_FREQ = 20.0f;
    static constexpr float SWEEP_END_FREQ = 20000.0f;
    static constexpr float LOG_START_FREQ = 20.6f;
    static constexpr float LOG_END_FREQ = 19912.0f;
    dsp::ClockDivider taskTimer;
    putils::ParamChangeDetect freqChange;
    putils::ParamChangeDetect levelChange;
    putils::ParamChangeDetect speedChange;
    putils::ParamChangeDetect toneChange;
    putils::ParamChangeDetect sweepChange;
    putils::ParamChangeDetect stepChange;
    putils::ParamChangeDetect onChange;
    putils::PosEdgeDetect trigSwChange;
    putils::PosEdgeDetect trigInChange;
    // running state
    float sampleTime;  // length of 1 sample in seconds
    int trig;  // current trig state of button OR with CV in
    // sweep freq
    float sweepPos;  // the current sweep position (0.0 to 1.0)
    float sweepPosInc;  // amount to increment the sweep position each sample
    float sweepFreq;  // the current sweep frequency
    float sweepExp;  // the sweep exponent
    int sweeping;  // 0 = stopped, 1 = sweeping
    // fixed freq
    float fixedFreq;  // fixed freq in Hz
    float fixedPhaseInc;  // amount to increment the phase for each sample
    // tone gen state
    float sinePhase;  // current sine phase
    float pink_state[3];

    // constructor
	Test_Osc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LEVEL_ENC, -INFINITY, INFINITY, 0.0f, "LEVEL");
        configParam(FREQ_ENC, -INFINITY, INFINITY, 0.0f, "FREQ");
		configParam(SPEED_ENC, -INFINITY, INFINITY, 0.0f, "SPEED");
		configParam(TONE_SW, 0.0f, 2.0f, 2.0f, "TONE");
		configParam(SWEEP_SW, 0.0f, 2.0f, 0.0f, "SWEEP");
		configParam(STEP_SW, 0.0f, 2.0f, 0.0f, "STEP");
		configParam(TRIG_SW, 0.0f, 1.0f, 0.0f, "TRIG");
		configParam(ON_SW, 0.0f, 1.0f, 0.0f, "ON");
        // settings (not controls)
        configParam(FREQ_INDEX, 0.0f, 30.0f, 17.0f, "FREQ INDEX");
        configParam(ABS_LEVEL, 0.000015848931924611107f, 1.0f, 0.1f, "ABS LEVEL");
        configParam(REF_LEVEL, 0.001f, 1000.0f, 1.0f, "REF LEVEL");
        configParam(SPEED, 0.1f, 60.0f, 1.0f, "SPEED");
        onReset();
        onSampleRateChange();
        freqChange.changeAmount = 0.1f;
        levelChange.changeAmount = 0.025f;
        speedChange.changeAmount = 0.025f;
        pink_state[0] = 0.0f;
        pink_state[1] = 0.0f;
        pink_state[2] = 0.0f;
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        int temp, onTrig;
        float wn, pn;

        // run tasks
        if(taskTimer.process()) {
            // frequency encoder
            if((temp = freqChange.updateDir(params[FREQ_ENC].getValue()))) {
                // set the freq based on the step type
                switch((int)params[STEP_SW].getValue()) {
                    case STEP_OCT:
                        params[FREQ_INDEX].setValue(
                            putils::clamp((int)params[FREQ_INDEX].getValue() + (temp * 3), 2, NUM_FREQS - 2));
                        fixedFreq = Test_Osc_freqs[(int)params[FREQ_INDEX].getValue()];
                        break;
                    case STEP_THIRD:
                        params[FREQ_INDEX].setValue(
                            putils::clamp((int)params[FREQ_INDEX].getValue() + temp, 0, NUM_FREQS - 1));
                        fixedFreq = Test_Osc_freqs[(int)params[FREQ_INDEX].getValue()];
                        break;
                    case STEP_LOG:
                    default:
                        float change = 1.059463094f;
                        if(temp < 0) {
                            change = 1.0f / change;
                        }
                        fixedFreq = putils::clampf(fixedFreq * change, LOG_START_FREQ, LOG_END_FREQ);
                        break;
                }
            }
            // level encoder
            if((temp = levelChange.updateDir(params[LEVEL_ENC].getValue()))) {
                params[ABS_LEVEL].setValue(
                    putils::clampf(dsp2::adjustFactorByDb(params[ABS_LEVEL].getValue(), (float)temp * 0.1f),
                    0.000015848931924611107f, 1.0f));
            }
            // speed encoder
            if((temp = speedChange.updateDir(params[SPEED_ENC].getValue()))) {
                params[SPEED].setValue(
                    putils::clampf(dsp2::adjustTimeFactorCoarse(params[SPEED].getValue(), (float)temp),
                    0.1f, 60.0f));
            }
            // tone switch
            if((temp = toneChange.update(params[TONE_SW].getValue()))) {
                settingChange();
            }
            onTrig = 0;
            // sweep switch
            if((temp = sweepChange.update(params[SWEEP_SW].getValue()))) {
                settingChange();
                // if we're changing to a sweep mode in on mode we need to trigger it
                if((int)params[SWEEP_SW].getValue() != SWEEP_OFF &&
                        (int)params[ON_SW].getValue() == ON_ON) {
                    onTrig = 1;
                }
            }
            // step switch
            if((temp = stepChange.update(params[STEP_SW].getValue()))) {
                settingChange();
            }
            // on switch
            if((temp = onChange.update(params[ON_SW].getValue()))) {
                settingChange();
                // if we're turning on in sweep mode we need to trigger it
                if((int)params[ON_SW].getValue() == ON_ON &&
                        (int)params[SWEEP_SW].getValue() != SWEEP_OFF) {
                    onTrig = 1;
                }
            }
            // trigger switch / CV input
            if(trigSwChange.update((int)params[TRIG_SW].getValue()) ||
                    trigInChange.update((int)inputs[TRIG_IN].getVoltage()) ||
                    onTrig) {
                switch((int)params[SWEEP_SW].getValue()) {
                    case SWEEP_LOG:
                        sweepPos = 0.0f;
                        sweeping = 1;
                        sweepExp = log2f(SWEEP_END_FREQ / SWEEP_START_FREQ);
                        break;
                    case SWEEP_LIN:
                        sweepPos = 0.0f;
                        sweeping = 1;
                        sweepExp = SWEEP_END_FREQ - SWEEP_START_FREQ;  // span
                        break;
                    case SWEEP_OFF:
                    default:
                        break;
                }
            }
            if(params[TRIG_SW].getValue() > 0.5f || inputs[TRIG_IN].getVoltage() > 1.0f) {
                trig = 1;
            }
            else {
                trig = 0;
            }
            lights[TRIG_IN_LED].setBrightness((int)dsp2::clampPos(inputs[TRIG_IN].getVoltage()));
            // calculate stuff
            // fixed freq
            fixedPhaseInc = fixedFreq * M_PI * 2.0f * sampleTime;
            // sweeping
            sweepPosInc = (1.0f / params[SPEED].getValue()) * sampleTime;
        }

        //
        // generate tone
        //
        switch((int)params[SWEEP_SW].getValue()) {
            case SWEEP_LIN:
                if(sweeping) {
                    sweepFreq = (sweepExp * sweepPos) + SWEEP_START_FREQ;  // sweepExp = span
                    float phaseInc = sweepFreq * M_PI * 2.0f * sampleTime;
                    outputs[OUT].setVoltage(simd::sin(sinePhase) * params[ABS_LEVEL].getValue() * AUDIO_OUT_GAIN);
                    sinePhase += phaseInc;
                    if(sinePhase > (M_PI * 2.0f)) {
                        sinePhase -= (M_PI * 2.0f);
                    }
                    sweepPos += sweepPosInc;
                    // cancel or restart sweep - try to minimize clicks
                    if((sweepPos >= 1.0f && sinePhase < 0.01f) || sweepPos >= 1.1f) {
                        sweepPos = 0.0f;
                        // one-shot if we are in momentary mode
                        if((int)params[ON_SW].getValue() == ON_MOM) {
                            sweeping = 0;
                        }
                    }
                }
                else {
                    outputs[OUT].setVoltage(0.0f);
                }
                break;
            case SWEEP_LOG:
                if(sweeping) {
                    sweepFreq = powf(2.0f, sweepPos * sweepExp) * SWEEP_START_FREQ;
                    float phaseInc = sweepFreq * M_PI * 2.0f * sampleTime;
                    outputs[OUT].setVoltage(simd::sin(sinePhase) * params[ABS_LEVEL].getValue() * AUDIO_OUT_GAIN);
                    sinePhase += phaseInc;
                    if(sinePhase > (M_PI * 2.0f)) {
                        sinePhase -= (M_PI * 2.0f);
                    }
                    sweepPos += sweepPosInc;
                    // cancel or restart sweep - try to minimize clicks
                    if((sweepPos >= 1.0f && sinePhase < 0.01f) || sweepPos >= 1.1f) {
                        sweepPos = 0.0f;
                        // one-shot if we are in momentary mode
                        if((int)params[ON_SW].getValue() == ON_MOM) {
                            sweeping = 0;
                        }
                    }
                }
                else {
                    outputs[OUT].setVoltage(0.0f);
                }
                break;
            case SWEEP_OFF:
            default:
                // we are on or momentary with a trigger
                if((int)params[ON_SW].getValue() == ON_ON || ((int)params[ON_SW].getValue() == ON_MOM && trig)) {
                    switch((int)params[TONE_SW].getValue()) {
                        case TONE_WHITE:
                            wn = random::uniform() - 0.5f;
                            outputs[OUT].setVoltage(wn * params[ABS_LEVEL].getValue() * AUDIO_OUT_GAIN * 2.0f);
                            break;
                        case TONE_PINK:
                            wn = random::uniform() - 0.5f;
                            pink_state[0] = 0.99765 * pink_state[0] + wn * 0.0990460;
                            pink_state[1] = 0.96300 * pink_state[1] + wn * 0.2965164;
                            pink_state[2] = 0.57000 * pink_state[2] + wn * 1.0526913;
                            pn = pink_state[0] + pink_state[1] + pink_state[2] + wn * 0.1848f;
                            pn *= 0.25f;
                            outputs[OUT].setVoltage(pn * params[ABS_LEVEL].getValue() * AUDIO_OUT_GAIN);
                            break;
                        case TONE_SINE:
                        default:
                            outputs[OUT].setVoltage(simd::sin(sinePhase) *
                                params[ABS_LEVEL].getValue() * AUDIO_OUT_GAIN);
                            sinePhase += fixedPhaseInc;
                            if(sinePhase > (M_PI * 2.0f)) {
                                sinePhase -= (M_PI * 2.0f);
                            }
                            break;
                    }
                }
                else {
                    outputs[OUT].setVoltage(0.0f);
                }
                break;
        }
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
        sampleTime = 1.0f / APP->engine->getSampleRate();
    }

    // module initialize
    void onReset(void) override {
        params[FREQ_INDEX].setValue(17.0f);
        params[ABS_LEVEL].setValue(1.0f);
        params[REF_LEVEL].setValue(1.0f);
        params[SPEED].setValue(1.0f);
        sweepPos = 0.0f;
        sweepFreq = SWEEP_START_FREQ;
        sweeping = 0;
        sinePhase = 0.0f;
        trig = 0;
    }

    // if a setting is changed we need to reset a bunch of stuff
    void settingChange(void) {
        params[FREQ_INDEX].setValue(17.0f);  // reset fixed freq
        fixedFreq = Test_Osc_freqs[(int)params[FREQ_INDEX].getValue()];
        if((int)params[STEP_SW].getValue() == STEP_LOG) {
            fixedFreq = 1046.5f;
        }
        sweeping = 0;
    }

    //
    // callbacks
    //
    // get the reference level factor
    float dispGetRefLevel(void) override {
        return params[ABS_LEVEL].getValue() * params[REF_LEVEL].getValue();
    }

    // get the absolute level factor
    float dispGetAbsLevel(void) override {
        return params[ABS_LEVEL].getValue();
    }

    // get the frequency in Hz
    float dispGetFrequency(void) override {
        if(sweeping) {
            return sweepFreq;
        }
        return fixedFreq;
    }

    // get the sweep time in s
    float dispGetSweepTime(void) override {
        return params[SPEED].getValue();
    }

    // get the sweep progress - 0.0 to 1.0 = 0-100%
    float dispGetSweepProgress(void) override {
        return sweepPos;
    }

    // handle scroll on the label
    void dispOnHoverScroll(int id, const event::HoverScroll& e) override {
        float change = 0.1f;
        if(e.scrollDelta.y < 0.0f) {
            change *= -1.0f;
        }
        params[REF_LEVEL].setValue(dsp2::adjustFactorByDb(params[REF_LEVEL].getValue(), change));
    }
};

// test osc display
struct TestOscDisplay : widget::TransparentWidget {
    int id;
    TestOscDisplaySource *source;
    float rad;
    NVGcolor textColor;
    NVGcolor bgColor;
    std::string fontFilename;
    float fontSizeSmall;
    float fontSizeLarge;

    // create a display
    TestOscDisplay(int id, math::Vec pos, math::Vec size) {
        this->id = id;
        this->source = NULL;
        rad = mm2px(2.0);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        textColor = nvgRGB(0xe0, 0xe0, 0xe0);
        bgColor = nvgRGBA(0x00, 0x00, 0x00, 0xff);
        fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        fontSizeSmall = 10.0f;
        fontSizeLarge = 13.0f;
    }

    // draw
    void draw(const DrawArgs& args) override {
        if(source == NULL) {
            return;
        }

        std::shared_ptr<Font> font = APP->window->loadFont(fontFilename);

        // background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, rad);
        nvgFillColor(args.vg, bgColor);
        nvgFill(args.vg);

        // text
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFontFaceId(args.vg, font->handle);
        nvgFillColor(args.vg, textColor);

        nvgFontSize(args.vg, fontSizeSmall);

        // abs level
        nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.15f,
            ("ABS: " + putils::factorToDbStr1(source->dispGetAbsLevel())).c_str(), NULL);

        // ref level
        nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.3f,
            ("REF: " + putils::factorToDbStr1(source->dispGetRefLevel())).c_str(), NULL);

        // sweep time
        // sweep progress
        nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.85f,
            ("S: " + putils::floatToStr1(source->dispGetSweepTime()) + "s " +
            putils::factorToPercentStr(source->dispGetSweepProgress())).c_str(), NULL);

        // frequency
//        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFontSize(args.vg, fontSizeLarge);
        nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.5f,
            putils::freqToStr(source->dispGetFrequency()).c_str(), NULL);
    }

    void onHoverScroll(const event::HoverScroll& e) override {
        if(source) {
            source->dispOnHoverScroll(id, e);
            e.consume(NULL);
            return;
        }
        TransparentWidget::onHoverScroll(e);
    }
};

struct Test_OscWidget : ModuleWidget {
	Test_OscWidget(Test_Osc* module) {
		setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Test_Osc.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
//		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
//		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        TestOscDisplay *disp = new TestOscDisplay(0, mm2px(Vec(20.32, 20.5)), mm2px(Vec(32.0, 20.0)));
        disp->source = module;
        addChild(disp);

		addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(12.32, 43.5)), module, Test_Osc::LEVEL_ENC));
		addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(28.32, 43.5)), module, Test_Osc::FREQ_ENC));
		addParam(createParamCentered<KilpatrickKnobBlackRed>(mm2px(Vec(12.32, 62.5)), module, Test_Osc::SPEED_ENC));
		addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(28.32, 62.5)), module, Test_Osc::TONE_SW));
		addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(12.32, 80.5)), module, Test_Osc::SWEEP_SW));
		addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(28.32, 80.5)), module, Test_Osc::STEP_SW));
		addParam(createParamCentered<KilpatrickD6RRedButton>(mm2px(Vec(12.32, 96.5)), module, Test_Osc::TRIG_SW));
		addParam(createParamCentered<KilpatrickToggle2P>(mm2px(Vec(28.32, 96.5)), module, Test_Osc::ON_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.32, 112.5)), module, Test_Osc::TRIG_IN));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(28.32, 112.5)), module, Test_Osc::OUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.32, 106.15)), module, Test_Osc::TRIG_IN_LED));
	}
};

Model* modelTest_Osc = createModel<Test_Osc, Test_OscWidget>("Test_Osc");
