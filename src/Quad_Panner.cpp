/*
 * Quad Panner
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

struct Quad_Panner : Module, KilpatrickJoystickHandler {
	enum ParamId {
		RESET_SW,
		PARAMS_LEN
	};
	enum InputId {
		SIG_IN,
		X_IN,
		Y_IN,
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
		FL_OUT_LED,
		FR_OUT_LED,
		SL_OUT_LED,
		SR_OUT_LED,
		FL_CV_LED,
		FR_CV_LED,
		SL_CV_LED,
		SR_CV_LED,
		LIGHTS_LEN
	};

    static constexpr int RT_TASK_RATE = 1000;
    static constexpr float CV_IN_SCALE = 0.2;  // -5V to +5V
    static constexpr float PEAK_METER_SMOOTHING = 10.0f;
    static constexpr float PEAK_METER_PEAK_HOLD_TIME = 0.1f;
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    dsp::ClockDivider taskTimer;
    float sumPosX, sumPosY;  // -1.0 to +1.0
    float joyPosX, joyPosY;  // -1.0 to +1.0
    float ctrlL;  // left control - 0.0 to 1.0 = min to max - centre = 0.7 (-3dB)
    float ctrlR;  // right control - 0.0 to 1.0 = min to max - centre = 0.7 (-3dB)
    float ctrlF;  // front control - 0.0 to 1.0 = min to max - centre = 0.7 (-3dB)
    float ctrlS;  // surround control - 0.0 to 1.0 = min to max - centre = 0.7 (-3dB)
    float vcaFL;  // front left VCA level
    float vcaFR;  // front right VCA level
    float vcaSL;  // surround left VCA level
    float vcaSR;  // surround right VCA level
    float multiOut[4];
    dsp2::Levelmeter peakMeterFlOut;
    dsp2::Levelmeter peakMeterFrOut;
    dsp2::Levelmeter peakMeterSlOut;
    dsp2::Levelmeter peakMeterSrOut;

    // constructor
	Quad_Panner() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RESET_SW, 0.f, 1.f, 0.f, "RESET");
		configInput(SIG_IN, "SIG IN");
		configInput(X_IN, "X IN");
		configInput(Y_IN, "Y IN");
		configOutput(FL_OUT, "FL OUT");
		configOutput(FR_OUT, "FR OUT");
		configOutput(SL_OUT, "SL OUT");
		configOutput(SR_OUT, "SR OUT");
		configOutput(MULTI_OUT, "MULTI OUT");
        onReset();
        onSampleRateChange();
        multiOut[0] = 0.0;
        multiOut[1] = 0.0;
        multiOut[2] = 0.0;
        multiOut[3] = 0.0;
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        float tempf;

        // run tasks
        if(taskTimer.process()) {
            // set multi output channels to 4
            if(outputs[MULTI_OUT].isConnected() && outputs[MULTI_OUT].getChannels() != 4) {
                outputs[MULTI_OUT].setChannels(4);
            }

            sumPosX = dsp2::clamp(joyPosX + (inputs[X_IN].getVoltage() * CV_IN_SCALE));
            sumPosY = dsp2::clamp(joyPosY + (inputs[Y_IN].getVoltage() * CV_IN_SCALE));

            // left/right with -3dB pan law in centre
            tempf = (sumPosX * 0.5) + 0.5;
            ctrlR = dsp2::clampPos(atanf(tempf * 1.7f));
            ctrlL = dsp2::clampPos(atanf((1.0 - tempf) * 1.7f));

            // front/surround with -3dB pan law in centre
            tempf = (sumPosY * 0.5) + 0.5;
            ctrlF = dsp2::clampPos(atanf(tempf * 1.7f));
            ctrlS = dsp2::clampPos(atanf((1.0 - tempf) * 1.7f));

            // VCA levels
            vcaFL = ctrlL * ctrlF;
            vcaFR = ctrlR * ctrlF;
            vcaSL = ctrlL * ctrlS;
            vcaSR = ctrlR * ctrlS;

            // CV LEDs
            lights[FL_CV_LED].setBrightness(vcaFL);
            lights[FR_CV_LED].setBrightness(vcaFR);
            lights[SL_CV_LED].setBrightness(vcaSL);
            lights[SR_CV_LED].setBrightness(vcaSR);

            // signal LEDs
            lights[FL_OUT_LED].setBrightness(peakMeterFlOut.getLevel());
            lights[FR_OUT_LED].setBrightness(peakMeterFrOut.getLevel());
            lights[SL_OUT_LED].setBrightness(peakMeterSlOut.getLevel());
            lights[SR_OUT_LED].setBrightness(peakMeterSrOut.getLevel());
        }

        // VCAs
        multiOut[0] = inputs[SIG_IN].getVoltage() * vcaFL;
        multiOut[1] = inputs[SIG_IN].getVoltage() * vcaFR;
        multiOut[2] = inputs[SIG_IN].getVoltage() * vcaSL;
        multiOut[3] = inputs[SIG_IN].getVoltage() * vcaSR;
        outputs[FL_OUT].setVoltage(multiOut[0]);
        outputs[FR_OUT].setVoltage(multiOut[1]);
        outputs[SL_OUT].setVoltage(multiOut[2]);
        outputs[SR_OUT].setVoltage(multiOut[3]);
        outputs[MULTI_OUT].writeVoltages(multiOut);
        peakMeterFlOut.update(multiOut[0] * AUDIO_IN_GAIN);
        peakMeterFrOut.update(multiOut[1] * AUDIO_IN_GAIN);
        peakMeterSlOut.update(multiOut[2] * AUDIO_IN_GAIN);
        peakMeterSrOut.update(multiOut[3] * AUDIO_IN_GAIN);
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
        peakMeterFlOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterFrOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterSlOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterSrOut.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
    }

    // module initialize
    void onReset(void) override {
        sumPosX = 0.0f;
        sumPosY = 0.0f;
        joyPosX = 0.0f;
        joyPosY = 0.0f;
    }

    // handle the joystick position
    void updateJoystick(int id, float xPos, float yPos) override {
        joyPosX = xPos;
        joyPosY = yPos;
    }

    // check if the reset button has been pressed
    int resetJoystick(void) override {
        if(params[RESET_SW].getValue() > 0.5f) {
            return 1;
        }
        return 0;
    }
};

struct Quad_PannerWidget : ModuleWidget {
	Quad_PannerWidget(Quad_Panner* module) {
		setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Quad_Panner.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		KilpatrickJoystick *joy = new KilpatrickJoystick(0, mm2px(Vec(30.48, 82.5)), mm2px(Vec(50.0, 50.0)));
        joy->controlAreaScale = 0.6f;
        joy->handler = module;
        addChild(joy);

        addParam(createParamCentered<KilpatrickD6RRedButton>(mm2px(Vec(50.48, 117.5)), module, Quad_Panner::RESET_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.48, 44.5)), module, Quad_Panner::SIG_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.48, 44.5)), module, Quad_Panner::X_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.48, 44.5)), module, Quad_Panner::Y_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.48, 28.5)), module, Quad_Panner::FL_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.48, 28.5)), module, Quad_Panner::FR_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.48, 28.5)), module, Quad_Panner::SL_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.48, 28.5)), module, Quad_Panner::SR_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.48, 44.5)), module, Quad_Panner::MULTI_OUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(9.48, 17.5)), module, Quad_Panner::FL_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(23.48, 17.5)), module, Quad_Panner::FR_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.48, 17.5)), module, Quad_Panner::SL_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(51.48, 17.5)), module, Quad_Panner::SR_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.48, 60.5)), module, Quad_Panner::FL_CV_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.48, 60.5)), module, Quad_Panner::FR_CV_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.48, 104.5)), module, Quad_Panner::SL_CV_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.48, 104.5)), module, Quad_Panner::SR_CV_LED));
	}

    // add menu items to select which memory slot we use
    void appendContextMenu(Menu *menu) override {
        Quad_Panner *module = dynamic_cast<Quad_Panner*>(this->module);
        if(!module) {
            return;
        }

        // mode
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Quad Panner Shortcuts");
        menuHelperAddLabel(menu, "Hold P and click joy edge to snap.");
    }
};

Model* modelQuad_Panner = createModel<Quad_Panner, Quad_PannerWidget>("Quad_Panner");
