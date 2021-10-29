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
    virtual float getRefLevel(int chan) { return 0.0f; }

    // set the reference level for a meter
    virtual void setRefLevel(int chan, float level) { }
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
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    dsp2::Levelmeter meterProcL;
    dsp2::Levelmeter meterProcR;

    // constructor
	Stereo_Meter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(REF_LEVEL_L, -60.0f, 24.0f, 0.0f, "REF LEVEL L");
        configParam(REF_LEVEL_R, -60.0f, 24.0f, 0.0f, "REF LEVEL R");
        configInput(IN_L, "IN L");
        configInput(IN_R, "IN R");
        onReset();
        onSampleRateChange();
        meterProcL.useHighpass = 1;
        meterProcR.useHighpass = 1;
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        meterProcL.update(inputs[IN_L].getVoltage() * AUDIO_IN_GAIN);
        meterProcR.update(inputs[IN_R].getVoltage() * AUDIO_IN_GAIN);
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        meterProcL.onSampleRateChange();
        meterProcR.onSampleRateChange();
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
            *level = meterProcR.getDbLevel();
            *peak = meterProcR.getPeakDbLevel();
            return;
        }
        *level = meterProcL.getDbLevel();
        *peak = meterProcL.getPeakDbLevel();
    }

    // get the reference level for a meter
    float getRefLevel(int chan) override {
        if(chan) {
            return params[REF_LEVEL_R].getValue();
        }
        return params[REF_LEVEL_L].getValue();
    }

    // set the reference level for a meter
    void setRefLevel(int chan, float level) override {
        if(chan) {
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
    KALevelmeter meterL;
    KALevelmeter meterR;

    // create a display
    Stereo_MeterDisplay(int id, math::Vec pos, math::Vec size) {
        this->id = id;
        this->source = NULL;
        rad = mm2px(1.625);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        bgColor = nvgRGBA(0x00, 0x00, 0x00, 0xff);
        meterL.textSlowdown = 8;
        meterL.textColor = nvgRGBA(0xe0, 0xe0, 0xe0, 0xff);
        meterL.bgColor = nvgRGBA(0x30, 0x30, 0x30, 0xff);
        meterL.barColor = nvgRGBA(0x00, 0xe0, 0x00, 0xff);
        meterL.peakColor = nvgRGBA(0xe0, 0x00, 0x00, 0xff);
        meterL.size.x = box.size.x * 0.4f;
        meterL.size.y = box.size.y * 0.87f;
        meterL.pos.x = (box.size.x * 0.28f) - (meterL.size.x * 0.5f);
        meterL.pos.y = box.size.y * 0.02f;
        meterL.setMinLevel(-96.0f);
        meterR.textSlowdown = 8;
        meterR.textColor = nvgRGBA(0xe0, 0xe0, 0xe0, 0xff);
        meterR.bgColor = nvgRGBA(0x30, 0x30, 0x30, 0xff);
        meterR.barColor = nvgRGBA(0x00, 0xe0, 0x00, 0xff);
        meterR.peakColor = nvgRGBA(0xe0, 0x00, 0x00, 0xff);
        meterR.size.x = box.size.x * 0.4f;
        meterR.size.y = box.size.y * 0.87f;
        meterR.pos.x = (box.size.x * 0.72f) - (meterR.size.x * 0.5f);
        meterR.pos.y = box.size.y * 0.02f;
        meterR.setMinLevel(-96.0f);
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
        meterL.setLevels(level, peak);
        meterL.setRefLevel(source->getRefLevel(0));

        source->getPeakDbLevels(1, &level, &peak);
        meterR.setLevels(level, peak);
        meterR.setRefLevel(source->getRefLevel(1));

        meterL.draw(args);
        meterR.draw(args);
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
