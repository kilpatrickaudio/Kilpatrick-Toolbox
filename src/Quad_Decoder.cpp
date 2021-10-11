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
        ENUMS(MULTI_OUT_LED, 3),
		LIGHTS_LEN
	};
    static constexpr int RT_TASK_RATE = 100;
    dsp::ClockDivider taskTimer;
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    static constexpr float AUDIO_OUT_GAIN = 10.0f;  // save mixing headroom
    static constexpr float PEAK_METER_SMOOTHING = 10.0f;
    static constexpr float PEAK_METER_PEAK_HOLD_TIME = 0.1f;
    dsp2::Levelmeter peakMeterLtIn;
    dsp2::Levelmeter peakMeterRtIn;
    dsp2::Levelmeter peakMeterFlOut;
    dsp2::Levelmeter peakMeterFrOut;
    dsp2::Levelmeter peakMeterSlOut;
    dsp2::Levelmeter peakMeterSrOut;
    dsp2::Levelmeter peakMeterMultOut;
    // matrix mixing
    enum Encoders {
        QS_ENCODE,
        SQ_BASIC_ENCODE,
        SQ_MOD_ENCODE,
        NUM_ENCODERS
    };
    int matrixMode;

    // constructor
	Quad_Decoder() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OUTPUT_POT, 0.f, 1.f, 0.5f, "OUTPUT LEVEL");
        configParam(MODE, 0.f, NUM_ENCODERS - 1, 0.0f, "MODE");
		configInput(LT_IN, "LT IN");
		configInput(RT_IN, "RT IN");
		configOutput(FL_OUT, "FL OUT");
		configOutput(FR_OUT, "FR OUT");
		configOutput(SL_OUT, "SL OUT");
		configOutput(SR_OUT, "SR OUT");
		configOutput(MULTI_OUT, "MULTI OUT");
        onReset();
        onSampleRateChange();
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        float lt, rt;

        // run tasks
        if(taskTimer.process()) {
            lights[LT_IN_LED].setBrightness(peakMeterLtIn.getLevel());
            lights[RT_IN_LED].setBrightness(peakMeterRtIn.getLevel());
            lights[FL_OUT_LED].setBrightness(peakMeterFlOut.getLevel());
            lights[FR_OUT_LED].setBrightness(peakMeterFlOut.getLevel());
            lights[SL_OUT_LED].setBrightness(peakMeterFlOut.getLevel());
            lights[SR_OUT_LED].setBrightness(peakMeterFlOut.getLevel());
            lights[MULTI_OUT_LED + 2].setBrightness(peakMeterMultOut.getLevel());

        }

        // get inputs
        lt = inputs[LT_IN].getVoltage() * AUDIO_IN_GAIN;
        peakMeterLtIn.update(lt);

        rt = inputs[RT_IN].getVoltage() * AUDIO_IN_GAIN;
        peakMeterRtIn.update(rt);
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
        peakMeterLtIn.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterRtIn.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterFlOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterFrOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterSlOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterSrOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterMultOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
    }

    // module initialize
    void onReset(void) override {
        lights[MULTI_OUT_LED + 0].setBrightness(0.0f);
        lights[MULTI_OUT_LED + 1].setBrightness(0.0f);
        matrixMode = -1;  // force refresh
        params[MODE].setValue(QS_ENCODE);
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

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.24, 44.5)), module, Quad_Decoder::LT_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.24, 44.5)), module, Quad_Decoder::RT_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.24, 78.5)), module, Quad_Decoder::FL_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.24, 78.5)), module, Quad_Decoder::FR_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.24, 94.5)), module, Quad_Decoder::SL_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.24, 94.5)), module, Quad_Decoder::SR_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 110.5)), module, Quad_Decoder::MULTI_OUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 39.208)), module, Quad_Decoder::LT_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 39.208)), module, Quad_Decoder::RT_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 73.208)), module, Quad_Decoder::FL_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 73.208)), module, Quad_Decoder::FR_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.74, 89.208)), module, Quad_Decoder::SL_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.74, 89.208)), module, Quad_Decoder::SR_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(24.236, 110.5)), module, Quad_Decoder::MULTI_OUT_LED));
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
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::QS_ENCODE, "QS / Quark Encode"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::SQ_BASIC_ENCODE, "SQ Basic Encode"));
        menuHelperAddItem(menu, new QuadDecoderModeMenuItem(module, Quad_Decoder::SQ_MOD_ENCODE, "SQ Modified Encode"));
    }
};

Model* modelQuad_Decoder = createModel<Quad_Decoder, Quad_DecoderWidget>("Quad_Decoder");
