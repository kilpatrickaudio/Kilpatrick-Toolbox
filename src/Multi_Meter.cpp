/*
 * Multi Meter
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

// stereo meter display source
struct Multi_MeterDisplaySource {
    // get the peak meter levels
    virtual void getPeakDbLevels(int chan, float *level, float *peak) { }

    // handle hover scroll
    virtual void onHoverScroll(int id, const event::HoverScroll& e) { }

    // get the reference level for a meter
    virtual float getRefLevel(int chan) { return 0.0f; }

    // set the reference level for a meter
    virtual void setRefLevel(int chan, float level) { }

    // get the MeterMode
    virtual int getMeterMode(void) { return 0; }

    // full a buffer of points from the XY ringbuffer up to maxSize
    // returns the number of points
    virtual int getXyPoints(Vec *buf, int maxSize) { return 0; }

    // clear the XY point buf
    virtual void clearXyPoints(void) { }
};

struct Multi_Meter : Module, Multi_MeterDisplaySource {
	enum ParamId {
		MODE_SW,
		CHAN_SW,
        REF_LEVEL_1,  // must be sequential
        REF_LEVEL_2,
        REF_LEVEL_3,
        REF_LEVEL_4,
        REF_LEVEL_5,
        REF_LEVEL_6,
        REF_LEVEL_7,
        REF_LEVEL_8,
        REF_LEVEL_9,
        REF_LEVEL_10,
        REF_LEVEL_11,
        REF_LEVEL_12,
        REF_LEVEL_13,
        REF_LEVEL_14,
        REF_LEVEL_15,
        REF_LEVEL_16,
		PARAMS_LEN
	};
	enum InputId {
		IN_L,
		IN_R,
		MULTI_IN,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
    static constexpr int XY_BUFLEN = 4096;
    static constexpr int MAX_CHANNELS = 16;
    static constexpr float AUDIO_IN_GAIN = 0.1f;
    enum ModeSwMode {
        MODE_MULTI,
        MODE_XY
    };
    enum ChannelSwMode {
        CHANNELS_16,
        CHANNELS_8,
        CHANNELS_4
    };
    // combined meter mode
    enum MeterMode {
        METER_XY,
        METER_4CH,
        METER_8CH,
        METER_16CH
    };
    dsp2::Levelmeter meterProc[MAX_CHANNELS];
    dsp::RingBuffer<Vec, XY_BUFLEN> xyBuf;

    // constructor
	Multi_Meter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MODE_SW, 0.f, 1.f, 0.f, "MODE");
		configParam(CHAN_SW, 0.f, 2.f, 0.f, "CHANNELS");
        for(int i = 0; i < MAX_CHANNELS; i ++) {
            configParam(REF_LEVEL_1 + i, -60.0f, 24.0f, 0.0f, putils::format("REF LEVEL %d", (i + 1)));
        }
		configInput(IN_L, "IN L");
		configInput(IN_R, "IN R");
		configInput(MULTI_IN, "MULTI IN");
        onReset();
        onSampleRateChange();
	}

    // process a sample
	void process(const ProcessArgs& args) override {
        float in1, in2;
        // get channel 1-2 (we need the for other functions than just metering)
        in1 = dsp2::clamp((inputs[MULTI_IN].getPolyVoltage(0) + inputs[IN_L].getVoltage()) * AUDIO_IN_GAIN);
        in2 = dsp2::clamp((inputs[MULTI_IN].getPolyVoltage(1) + inputs[IN_R].getVoltage()) * AUDIO_IN_GAIN);
        meterProc[0].update(in1);
        meterProc[1].update(in2);
        xyBuf.push(Vec(in1, in2));

        // get channel 3-16
        for(int i = 2; i < inputs[MULTI_IN].getChannels(); i ++) {
            meterProc[i].update(dsp2::clamp(inputs[MULTI_IN].getPolyVoltage(i) * AUDIO_IN_GAIN));
        }
        for(int i = inputs[MULTI_IN].getChannels(); i < MAX_CHANNELS; i ++) {
            meterProc[i].update(0.0f);
        }

	}

    // samplerate changed
    void onSampleRateChange(void) override {
        for(int i = 0; i < MAX_CHANNELS; i ++) {
            meterProc[i].useHighpass = 1;
            meterProc[i].onSampleRateChange();
        }
    }

    // module initialize
    void onReset(void) override {
    }

    //
    // callbacks
    //
    // get the peak meter levels
    void getPeakDbLevels(int chan, float *level, float *peak) override {
        if(chan < 0 || chan >= MAX_CHANNELS) {
            return;
        }
        *level = meterProc[chan].getDbLevel();
        *peak = meterProc[chan].getPeakDbLevel();
    }

    // get the reference level for a meter
    float getRefLevel(int chan) override {
        if(chan < 0 || chan >= MAX_CHANNELS) {
            return 0.0f;
        }
        return params[REF_LEVEL_1 + chan].getValue();
    }

    // set the reference level for a meter
    void setRefLevel(int chan, float level) override {
        if(chan < 0 || chan >= MAX_CHANNELS) {
            return;
        }
        params[REF_LEVEL_1 + chan].setValue(level);
    }

    // get the MeterMode
    int getMeterMode(void) override {
        switch((int)params[MODE_SW].getValue()) {
            case ModeSwMode::MODE_XY:
                return MeterMode::METER_XY;
            default:
                switch((int)params[CHAN_SW].getValue()) {
                    case CHANNELS_4:
                        return MeterMode::METER_4CH;
                    case CHANNELS_8:
                        return MeterMode::METER_8CH;
                    case CHANNELS_16:
                    default:
                        return MeterMode::METER_16CH;
                }
                return MeterMode::METER_16CH;
        }
    }

    // full a buffer of points from the XY ringbuffer up to maxSize
    // returns the number of points
    int getXyPoints(Vec *buf, int maxSize) override {
        int len = xyBuf.size();
        if(len > maxSize) len = maxSize;
        if(len > 0) {
            xyBuf.shiftBuffer(buf, len);
        }
        return len;
    }

    // clear the XY point buf
    void clearXyPoints(void) override {
        xyBuf.clear();
    }
};

// levelmeter / scope display
struct Multi_MeterDisplay : widget::TransparentWidget {
    int id;
    Multi_MeterDisplaySource *source;
    float rad;
    NVGcolor bgColor;
    NVGcolor scopeGridColor;
    NVGcolor scopeColor;
    KALevelmeter multimeters[Multi_Meter::MAX_CHANNELS];
    KALevelmeter meters[Multi_Meter::MAX_CHANNELS];
    KALevelmeter meterR;
    int meterMode = -1;  // force reload
    static constexpr int XY_BUFLEN = 4096;
    Vec xyBuf[XY_BUFLEN];
    Vec xyOld;

    // create a display
    Multi_MeterDisplay(int id, math::Vec pos, math::Vec size) {
        this->id = id;
        this->source = NULL;
        rad = mm2px(1);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        bgColor = nvgRGBA(0x00, 0x00, 0x00, 0xff);
        scopeGridColor = nvgRGBA(0x00, 0x99, 0x99, 0xff);
        scopeColor = nvgRGBA(0x00, 0xff, 0xff, 0xff);
        xyOld.x = 0.0f;
        xyOld.y = 0.0f;

        for(int i = 0; i < Multi_Meter::MAX_CHANNELS; i ++) {
            meters[i].fontSizeReadout = 8.0;
            meters[i].textDrawDecimal = 0;
            meters[i].textSlowdown = 8;
            meters[i].textColor = nvgRGBA(0xe0, 0xe0, 0xe0, 0xff);
            meters[i].bgColor = nvgRGBA(0x30, 0x30, 0x30, 0xff);
            meters[i].barColor = nvgRGBA(0x00, 0xe0, 0x00, 0xff);
            meters[i].peakColor = nvgRGBA(0xe0, 0x00, 0x00, 0xff);
        }
    }

    // draw
    void draw(const DrawArgs& args) override {
        float level[16], peak[16], ref[16];
        int reflowMeters = 0;
        if(source == NULL) {
            meterMode = Multi_Meter::METER_16CH;
            reflowMeters = 1;
            for(int i = 0; i < getNumMeters(); i ++) {
                level[i] = -10.0f;
                peak[i] = -10.0f;
                ref[i] = 0.0f;
            }
        }
        else {
            if(meterMode != source->getMeterMode()) {
                meterMode = source->getMeterMode();
                reflowMeters = 1;
            }

            // render meters
            for(int i = 0; i < getNumMeters(); i ++) {
                source->getPeakDbLevels(i, &level[i], &peak[i]);
                ref[i] = source->getRefLevel(i);
            }
        }

        // background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, rad);
        nvgFillColor(args.vg, bgColor);
        nvgFill(args.vg);

        // re-layout the meters
        if(reflowMeters) {
            float xPos = box.size.x * 0.025f;
            float w = box.size.x * 0.05;
            float space = box.size.x * 0.01;

            // scale the width
            switch(meterMode) {
                case Multi_Meter::METER_4CH:
                    w *= 4.0f;
                    w += (space * 3.0f);
                    break;
                case Multi_Meter::METER_8CH:
                    w *= 2.0f;
                    w += space;
                    break;
            }

            // lay out the meters
            for(int i = 0; i < getNumMeters(); i ++) {
                meters[i].size.x = w;
                meters[i].size.y = box.size.y * 0.87f;
                meters[i].pos.x = xPos;
                meters[i].pos.y = box.size.y * 0.03f;
                meters[i].setMinLevel(-96.0f);  // this is needed after we set the height
                xPos += w + space;
            }
            reflowMeters = 0;
        }

        // render meters
        for(int i = 0; i < getNumMeters(); i ++) {
            meters[i].setLevels(level[i], peak[i]);
            meters[i].setRefLevel(ref[i]);
            meters[i].draw(args);
        }

        // for X/Y mode render the scope
        if(meterMode == Multi_Meter::METER_XY) {
            float scopeHalfSize = box.size.x * 0.35f;
            float scopeXPos = (box.size.x * 0.5f) + (box.size.x * 0.05f);
            float scopeYPos = box.size.y * 0.5f;

            // draw graticule
            nvgBeginPath(args.vg);
            nvgRect(args.vg, scopeXPos - scopeHalfSize, scopeYPos - scopeHalfSize,
                scopeHalfSize * 2.0f, scopeHalfSize * 2.0f);
            nvgMoveTo(args.vg, scopeXPos, scopeYPos - scopeHalfSize);
            nvgLineTo(args.vg, scopeXPos, scopeYPos + scopeHalfSize);
            nvgMoveTo(args.vg, scopeXPos - scopeHalfSize, scopeYPos);
            nvgLineTo(args.vg, scopeXPos + scopeHalfSize, scopeYPos);
            nvgMoveTo(args.vg, scopeXPos - scopeHalfSize, scopeYPos - scopeHalfSize);
            nvgLineTo(args.vg, scopeXPos + scopeHalfSize, scopeYPos + scopeHalfSize);
            nvgMoveTo(args.vg, scopeXPos - scopeHalfSize, scopeYPos + scopeHalfSize);
            nvgLineTo(args.vg, scopeXPos + scopeHalfSize, scopeYPos - scopeHalfSize);
            nvgStrokeColor(args.vg, scopeGridColor);
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);

            // draw X/Y data
            int len = source->getXyPoints(xyBuf, XY_BUFLEN);
            if(len == 0) {
                return;
            }
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, (xyOld.x * scopeHalfSize) + scopeXPos,
                (-xyOld.y * scopeHalfSize) + scopeYPos);
            for(int i = 0; i < len; i ++) {
                nvgLineTo(args.vg, (xyBuf[i].x * scopeHalfSize) + scopeXPos,
                    (-xyBuf[i].y * scopeHalfSize) + scopeYPos);
            }
            xyOld = xyBuf[len-1];
            nvgStrokeColor(args.vg, scopeColor);
            nvgStrokeWidth(args.vg, 1.5f);
            nvgStroke(args.vg);
        }
        else if(source != NULL) {
            source->clearXyPoints();
        }
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
/*
        // select meter based on mouse hover pos
        if(e.pos.x > box.size.x / 2) {
            source->setRefLevel(1, source->getRefLevel(1) + change);
        }
        else {
            source->setRefLevel(0, source->getRefLevel(0) + change);
        }
        e.consume(NULL);
*/
        TransparentWidget::onHoverScroll(e);
    }

    // convert the meter mode into the number of meters
    int getNumMeters(void) {
        switch(meterMode) {
            case Multi_Meter::METER_XY:
                return 2;
            case Multi_Meter::METER_4CH:
                return 4;
            case Multi_Meter::METER_8CH:
                return 8;
        }
        return 16;
    }
};

struct Multi_MeterWidget : ModuleWidget {
	Multi_MeterWidget(Multi_Meter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Multi_Meter.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        Multi_MeterDisplay *disp = new Multi_MeterDisplay(0, mm2px(Vec(55.88, 55.5)), mm2px(Vec(92.0, 84.0)));
        disp->source = module;
        addChild(disp);

		addParam(createParamCentered<KilpatrickToggle2P>(mm2px(Vec(64.88, 108.5)), module, Multi_Meter::MODE_SW));
		addParam(createParamCentered<KilpatrickToggle3P>(mm2px(Vec(84.88, 108.5)), module, Multi_Meter::CHAN_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.88, 108.5)), module, Multi_Meter::IN_L));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.88, 108.5)), module, Multi_Meter::IN_R));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(45.88, 108.5)), module, Multi_Meter::MULTI_IN));
	}
};

Model* modelMulti_Meter = createModel<Multi_Meter, Multi_MeterWidget>("Multi_Meter");
