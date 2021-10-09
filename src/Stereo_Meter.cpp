/*
 * Stereo Meter
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

// stereo meter display source
struct Stereo_MeterDisplaySource {
    // get the peak meter levels
    virtual void getPeakDbLevels(int chan, float *level, float *peak) { }

    // handle hover scroll
    virtual void onHoverScroll(int id, const event::HoverScroll& e) { }

    // get the reference level for a meter
    virtual float getRefLevel(int meterIndex) { return 0.0f; }

    // set the reference level for a meter
    virtual void setRefLevel(int meterIndex, float level) { }
};

struct Stereo_Meter : Module, Stereo_MeterDisplaySource {
	enum ParamIds {
        REF_LEVEL_L,
        REF_LEVEL_R,
		NUM_PARAMS
	};
	enum InputIds {
		IN_L,
		IN_R,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
    static constexpr int RT_TASK_RATE = 1000;
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    static constexpr float PEAK_METER_SMOOTHING = 1.0f;
    static constexpr float PEAK_METER_PEAK_HOLD_TIME = 1.0f;
    dsp::ClockDivider taskTimer;
    dsp2::Filter2Pole hpfL;
    dsp2::Filter2Pole hpfR;
    dsp2::Levelmeter peakMeterL;
    dsp2::Levelmeter peakMeterR;

    // constructor
	Stereo_Meter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(REF_LEVEL_L, -60.0f, 24.0f, 0.0f, "REF LEVEL L");
        configParam(REF_LEVEL_R, -60.0f, 24.0f, 0.0f, "REF LEVEL R");
        onReset();
        onSampleRateChange();
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        float inL, inR;
        // run tasks
        if(taskTimer.process()) {
        }

        inL = hpfL.process(inputs[IN_L].getVoltage()) * AUDIO_IN_GAIN;
        inR = hpfR.process(inputs[IN_R].getVoltage()) * AUDIO_IN_GAIN;
        peakMeterL.update(inL);
        peakMeterR.update(inR);
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / RT_TASK_RATE));
        hpfL.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        hpfR.setCutoff(dsp2::Filter2Pole::TYPE_HPF, 10.0f, 0.707f, 1.0f, APP->engine->getSampleRate());
        peakMeterL.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterR.setSmoothingFreq(PEAK_METER_SMOOTHING, APP->engine->getSampleRate());
        peakMeterL.setPeakHoldTime(PEAK_METER_PEAK_HOLD_TIME, APP->engine->getSampleRate());
        peakMeterR.setPeakHoldTime(PEAK_METER_PEAK_HOLD_TIME, APP->engine->getSampleRate());
    }

    // module initialize
    void onReset(void) override {
    }

    //
    // callbacks
    //
    // get the peak meter levels
    void getPeakDbLevels(int chan, float *level, float *peak) override {
        if(chan) {
            *level = peakMeterR.getDbLevel();
            *peak = peakMeterR.getPeakDbLevel();
            return;
        }
        *level = peakMeterL.getDbLevel();
        *peak = peakMeterL.getPeakDbLevel();
    }

    // get the reference level for a meter
    float getRefLevel(int meterIndex) override {
        if(meterIndex) {
            return params[REF_LEVEL_R].getValue();
        }
        return params[REF_LEVEL_L].getValue();
    }

    // set the reference level for a meter
    void setRefLevel(int meterIndex, float level) override {
        if(meterIndex) {
            params[REF_LEVEL_R].setValue(level);
        }
        else {
            params[REF_LEVEL_L].setValue(level);
        }
    }
};

// levelmeter display
struct Stereo_MeterDisplay : widget::TransparentWidget {
    int id;
    Stereo_MeterDisplaySource *source;
    float rad;
    NVGcolor bgColor;
    KALevelmeter peakMeterL;
    KALevelmeter peakMeterR;

    // create a display
    Stereo_MeterDisplay(int id, math::Vec pos, math::Vec size) {
        this->id = id;
        this->source = NULL;
        rad = mm2px(2.0);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        bgColor = nvgRGBA(0x00, 0x00, 0x00, 0xff);
        peakMeterL.textSlowdown = 8;
        peakMeterL.textColor = nvgRGBA(0xe0, 0xe0, 0xe0, 0xff);
        peakMeterL.bgColor = nvgRGBA(0x30, 0x30, 0x30, 0xff);
        peakMeterL.barColor = nvgRGBA(0x00, 0xe0, 0x00, 0xff);
        peakMeterL.peakColor = nvgRGBA(0xe0, 0x00, 0x00, 0xff);
        peakMeterL.size.x = box.size.x * 0.4f;
        peakMeterL.size.y = box.size.y * 0.87f;
        peakMeterL.pos.x = (box.size.x * 0.28f) - (peakMeterL.size.x * 0.5f);
        peakMeterL.pos.y = box.size.y * 0.02f;
        peakMeterL.setMinLevel(-96.0f);
        peakMeterR.textSlowdown = 8;
        peakMeterR.textColor = nvgRGBA(0xe0, 0xe0, 0xe0, 0xff);
        peakMeterR.bgColor = nvgRGBA(0x30, 0x30, 0x30, 0xff);
        peakMeterR.barColor = nvgRGBA(0x00, 0xe0, 0x00, 0xff);
        peakMeterR.peakColor = nvgRGBA(0xe0, 0x00, 0x00, 0xff);
        peakMeterR.size.x = box.size.x * 0.4f;
        peakMeterR.size.y = box.size.y * 0.87f;
        peakMeterR.pos.x = (box.size.x * 0.72f) - (peakMeterR.size.x * 0.5f);
        peakMeterR.pos.y = box.size.y * 0.02f;
        peakMeterR.setMinLevel(-96.0f);
    }

    // draw
    void draw(const DrawArgs& args) override {
        float level, peak;
        if(source == NULL) {
            return;
        }

        // background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, rad);
        nvgFillColor(args.vg, bgColor);
        nvgFill(args.vg);

        source->getPeakDbLevels(0, &level, &peak);
        peakMeterL.setLevels(level, peak);
        peakMeterL.setRefLevel(source->getRefLevel(0));

        source->getPeakDbLevels(1, &level, &peak);
        peakMeterR.setLevels(level, peak);
        peakMeterR.setRefLevel(source->getRefLevel(1));

        peakMeterL.draw(args);
        peakMeterR.draw(args);
    }

    void onHoverScroll(const event::HoverScroll& e) override {
        float change = 0.0f;
        // get scroll direction
        if(e.scrollDelta.y < 0.0f) {
            change = -1.0f;
        }
        else if(e.scrollDelta.y > 0.0f) {
            change = 1.0f;
        }
        // select meter based on mouse hover pos
        if(e.pos.x > box.size.x / 2) {
            source->setRefLevel(1, source->getRefLevel(1) + change);
        }
        else {
            source->setRefLevel(0, source->getRefLevel(0) + change);
        }
        e.consume(NULL);
        TransparentWidget::onHoverScroll(e);
    }
};

struct Stereo_MeterWidget : ModuleWidget {
	Stereo_MeterWidget(Stereo_Meter* module) {
		setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Stereo_Meter.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        Stereo_MeterDisplay *disp = new Stereo_MeterDisplay(0, mm2px(Vec(15.24, 47.5)), mm2px(Vec(26.0, 68.0)));
        disp->source = module;
        addChild(disp);

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 94.5)), module, Stereo_Meter::IN_L));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 108.5)), module, Stereo_Meter::IN_R));
	}
};

Model* modelStereo_Meter = createModel<Stereo_Meter, Stereo_MeterWidget>("Stereo_Meter");
