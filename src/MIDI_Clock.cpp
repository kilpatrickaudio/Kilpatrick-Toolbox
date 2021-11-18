/*
 * MIDI Clock - vMIDI and Analog Clock
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
#include "utils/CVMidi.h"
#include "utils/KAComponents.h"
#include "utils/MidiHelper.h"
#include "utils/PLog.h"
#include "utils/PUtils.h"
#include "utils/VUtils.h"
#include "MidiClockPll/MidiClockPll.h"

// MIDI clock display source
struct MidiClockDisplaySource {
    // get the tempo in BPM
    virtual float midiClockDisplayGetTempo(void) { return 100.0f; }

    // get whether internal clock source is used
    virtual int midiClockDisplayIsSourceInternal(void) { return 1; }

    // get whether clock is running
    virtual int midiClockDisplayIsRunning(void) { return 0; }

    // get the output divider ratio - 1 = same as upsample PPQ
    virtual int midiClockDisplayGetOutputDiv(void) { return 1; }

    // get whether autostart mode is enabled
    virtual int midiClockDisplayIsAutostartEnabled(void) { return 0; }

    // tap the tempo
    virtual void midiClockTapTempo(void) { }

    // adjust the tempo
    virtual void midiClockAdjustTempo(float change) { }

    // adjust the output divider
    virtual void midiClockAdjustOutputDiv(float change) { }

    // toggle autostart
    virtual void midiClockToggleAutostart(void) { }

    // toggle run state
    virtual void midiClockToggleRunState(void) { }
};

struct MIDI_Clock : Module, MidiClockPllHandler, MidiClockDisplaySource {
	enum ParamId {
		RESET_SW,
		RUNSTOP_SW,
        OUTPUT_DIV,  // 1.0-24.0 = divide ratio
        AUTOSTART_EN,  // 0 = disable, 1.0 = enable
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_IN,
		MIDI_IN,
		TEMPO_IN,
		RUN_IN,
		RESET_IN,
		INPUTS_LEN
	};
	enum OutputId {
		MIDI_OUT,
		CLOCK_OUT,
		RESET_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		CLOCK_IN_LED,
		MIDI_IN_LED,
		TEMPO_IN_LED,
		MIDI_OUT_LED,
		RUN_IN_LED,
		CLOCK_OUT_LED,
		RESET_IN_LED,
		RESET_OUT_LED,
		LIGHTS_LEN
	};
    static constexpr int OUTPUT_DIV_MIN = 1;
    static constexpr int OUTPUT_DIV_MAX = 24;
    static constexpr int OUT_PULSE_LEN = 4;
    static constexpr int LED_PULSE_LEN = 50;
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    CVMidi *cvMidiOut;
    putils::PosEdgeDetect resetSwEdge;
    putils::PosEdgeDetect runstopSwEdge;
    putils::Pulser clockOutPulse;
    putils::Pulser resetOutPulse;
    putils::Pulser clockLedPulse;
    putils::Pulser resetLedPulse;
    MidiClockPll midiClock;
    int outputDiv;  // current running output divider ratio
    int outputDivCount;

    // constructor
	MIDI_Clock() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RESET_SW, 0.f, 1.f, 0.f, "RESET");
		configParam(RUNSTOP_SW, 0.f, 1.f, 0.f, "RUN/STOP");
        configParam(OUTPUT_DIV, 1.0f, 24.0f, 1.0f, "OUTPUT DIV");
        configParam(AUTOSTART_EN, 0.0f, 1.0f, 0.0f, "AUTOSTART");
		configInput(CLOCK_IN, "CLOCK IN");
		configInput(MIDI_IN, "MIDI IN");
		configInput(TEMPO_IN, "TEMPO IN");
		configInput(RUN_IN, "RUN IN");
		configInput(RESET_IN, "RESET IN");
		configOutput(MIDI_OUT, "MIDI OUT");
		configOutput(CLOCK_OUT, "CLOCK OUT");
		configOutput(RESET_OUT, "RESET OUT");
        cvMidiIn = new CVMidi(&inputs[MIDI_IN], 1);
        cvMidiOut = new CVMidi(&outputs[MIDI_OUT], 0);
        midiClock.setTaskInterval(MIDI_RT_TASK_RATE);
        midiClock.setInternalPpq(24);
        midiClock.registerHandler(this);
        onReset();
        onSampleRateChange();
	}

    // destructor
    ~MIDI_Clock() {
        delete cvMidiIn;
        delete cvMidiOut;
    }

    // process a sample
	void process(const ProcessArgs& args) override {
        // handle CV MIDI
        cvMidiIn->process();
        cvMidiOut->process();

        // run tasks
        if(taskTimer.process()) {
            // handle buttons
            if(resetSwEdge.update((int)params[RESET_SW].getValue())) {
                midiClock.resetRequest();
            }
            if(runstopSwEdge.update((int)params[RUNSTOP_SW].getValue())) {
                if(midiClock.getRunState()) {
                    midiClock.stopRequest();
                }
                else {
                    midiClock.continueRequest();
                }
            }

            // run the MIDI clock
            midiClock.timerTask();

            // outputs
            outputs[CLOCK_OUT].setVoltage((clockOutPulse.update() != 0) * 10.0f);
            outputs[RESET_OUT].setVoltage((resetOutPulse.update() != 0) * 10.0f);

            lights[CLOCK_OUT_LED].setBrightness(clockLedPulse.update() != 0);
            lights[RESET_OUT_LED].setBrightness(resetLedPulse.update() != 0);

            // MIDI LEDs
            lights[MIDI_IN_LED].setBrightness(cvMidiIn->getLedState());
            lights[MIDI_OUT_LED].setBrightness(cvMidiOut->getLedState());
        }
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / MIDI_RT_TASK_RATE));
    }

    // module initialize
    void onReset(void) override {
    }

    // module added (post initialize)
    void onAdd(void) override {
        if((int)params[AUTOSTART_EN].getValue()) {
            midiClock.resetRequest();
            midiClock.continueRequest();
        }
        outputDiv = (int)params[OUTPUT_DIV].getValue();
        outputDivCount = 0;
    }


    //
    // MIDI clock callbacks
    //
    // a beat was crossed
    void midiClockBeatCrossed(void) override {
        int temp;
//        PDEBUG("BEAT!");
        temp = (int)params[OUTPUT_DIV].getValue();
        if(outputDiv != temp) {
            outputDiv = temp;
            outputDivCount = 0;
        }
    }

    // run state changed
    void midiClockRunStateChanged(int running, int reset) override {
        midi::Message msg;
//        PDEBUG("run state: %d - reset: %d", running, reset);
        msg.setSize(1);
        if(running) {
            if(reset) {
                msg.bytes[0] = MIDI_CLOCK_START;
            }
            else {
                msg.bytes[0] = MIDI_CLOCK_CONTINUE;
            }
        }
        else {
            msg.bytes[0] = MIDI_CLOCK_STOP;
        }
        cvMidiOut->sendOutputMessage(msg);
    }

    // clock source changed
    void midiClockSourceChanged(int source) override {
        PDEBUG("clock source: %d", source);
    }

    // tap tempo locked
    void midiClockTapTempoLocked(void) override {
        PDEBUG("tap tempo locked!");
    }

    // clock ticked
    void midiClockTicked(uint32_t tickCount) override {
        midi::Message msg;
        // MIDI out
        msg.setSize(1);
        msg.bytes[0] = MIDI_TIMING_TICK;
        cvMidiOut->sendOutputMessage(msg);

        // clock out
        if(midiClock.getRunState()) {
            if(outputDivCount == 0) {
                clockOutPulse.timeout = OUT_PULSE_LEN;
                clockLedPulse.timeout = LED_PULSE_LEN;
            }
            outputDivCount ++;
            if(outputDivCount == outputDiv) {
                outputDivCount = 0;
            }
        }
    }

    // clock position was reset
    void midiClockPositionReset(void) override {
        midi::Message msg;
        resetOutPulse.timeout = OUT_PULSE_LEN;
        resetLedPulse.timeout = LED_PULSE_LEN;
        outputDivCount = 0;

        // MIDI out
        msg.setSize(1);
        msg.bytes[0] = MIDI_CLOCK_START;
        cvMidiOut->sendOutputMessage(msg);
    }

    // externally locked tempo changed
    void midiClockExtTempoChanged(void) override {
        PDEBUG("ext tempo changed");
    }

    // external sync state changed
    void midiClockExtSyncChanged(int synced) override {
        PDEBUG("ext sync changed: %d", synced);
    }

    //
    // MIDI clock display source
    //
    // get the tempo in BPM
    float midiClockDisplayGetTempo(void) override {
        return midiClock.getTempo();
    }

    // get whether internal clock source is used
    int midiClockDisplayIsSourceInternal(void) override {
        if(midiClock.getSource() == MidiClockPll::SOURCE_INTERNAL) {
            return 1;
        }
        return 0;
    }

    // get whether clock is running
    int midiClockDisplayIsRunning(void) override {
        return midiClock.getRunState();
    }

    // get the output divider ratio - 1 = same as upsample PPQ
    int midiClockDisplayGetOutputDiv(void) override {
        return (int)params[OUTPUT_DIV].getValue();
    }

    // get whether autostart mode is enabled
    int midiClockDisplayIsAutostartEnabled(void) override {
        return (int)params[AUTOSTART_EN].getValue();
    }

    // tap the tempo
    void midiClockTapTempo(void) override {
        midiClock.tapTempo();
    }

    // adjust the tempo
    void midiClockAdjustTempo(float change) override {
        midiClock.setTempo(midiClock.getTempo() + change);
    }

    // adjust the output divider
    void midiClockAdjustOutputDiv(float change) override {
        params[OUTPUT_DIV].setValue(
            putils::clamp((int)(params[OUTPUT_DIV].getValue() + change),
            OUTPUT_DIV_MIN, OUTPUT_DIV_MAX));
    }

    // toggle autostart
    void midiClockToggleAutostart(void) override {
        if((int)params[AUTOSTART_EN].getValue()) {
            params[AUTOSTART_EN].setValue(0.0f);
        }
        else {
            params[AUTOSTART_EN].setValue(1.0f);
        }
    }

    // toggle run state
    void midiClockToggleRunState(void) override {
        if(midiClock.getRunState()) {
            midiClock.stopRequest();
        }
        else {
            midiClock.continueRequest();
        }
    }
};

// MIDI clock display
struct MidiClockDisplay : widget::TransparentWidget {
    MidiClockDisplaySource *source;
    float rad;
    NVGcolor textColor;
    NVGcolor runColor;
    NVGcolor stopColor;
    NVGcolor bgColor;
    std::string fontFilename;
    float fontSizeSmall;
    float fontSizeLarge;
    vutils::TouchZones touchZones;
    int shift;
    enum {
        ZONE_TEMPO,
        ZONE_RUNSTOP,
        ZONE_INTEXT,
        ZONE_DIV,
        ZONE_AUTOSTART
    };

    // create a display
    MidiClockDisplay(math::Vec pos, math::Vec size) {
        this->source = NULL;
        rad = mm2px(2.0);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        textColor = nvgRGB(0xe0, 0xe0, 0xe0);
        runColor = nvgRGB(0x00, 0xff, 0x00);
        stopColor = nvgRGB(0xcc, 0xcc, 0xcc);
        bgColor = nvgRGBA(0x00, 0x00, 0x00, 0xff);
        fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        fontSizeSmall = 10.0f;
        fontSizeLarge = 18.0f;
        shift = 0;
        // add touch zones
        touchZones.addZoneCentered(ZONE_TEMPO, box.size.x * 0.5f, box.size.y * 0.5f,
            box.size.x, box.size.y * 0.5f);
        touchZones.addZoneCentered(ZONE_RUNSTOP, box.size.x * 0.25f, box.size.y * 0.15f,
            box.size.x * 0.5f, box.size.y * 0.25f);
        touchZones.addZoneCentered(ZONE_INTEXT, box.size.x * 0.75f, box.size.y * 0.15f,
            box.size.x * 0.5f, box.size.y * 0.25f);
        touchZones.addZoneCentered(ZONE_DIV, box.size.x * 0.25f, box.size.y * 0.85f,
            box.size.x * 0.5f, box.size.y * 0.25f);
        touchZones.addZoneCentered(ZONE_AUTOSTART, box.size.x * 0.75f, box.size.y * 0.85f,
            box.size.x * 0.5f, box.size.y * 0.25f);
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

        // tempo display
        nvgFontSize(args.vg, fontSizeLarge);
        nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.5f,
            putils::format("%3.1f", source->midiClockDisplayGetTempo()).c_str(), NULL);

        nvgFontSize(args.vg, fontSizeSmall);
        // int/ext state
        if(source->midiClockDisplayIsSourceInternal()) {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.15f, "INT", NULL);
        }
        else {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.15f, "EXT", NULL);
        }

        // output divider
        nvgText(args.vg, box.size.x * 0.25f, box.size.y * 0.85f,
            putils::format("d: 1/%d", source->midiClockDisplayGetOutputDiv()).c_str(), NULL);

        // autostart enable
        if(source->midiClockDisplayIsAutostartEnabled()) {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.85f, "AUTO", NULL);
        }
        else {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.85f, "MAN", NULL);
        }

        // run/stop state
        if(source->midiClockDisplayIsRunning()) {
            nvgFillColor(args.vg, runColor);
            nvgText(args.vg, box.size.x * 0.25f, box.size.y * 0.15f, "RUN", NULL);
        }
        else {
            nvgFillColor(args.vg, stopColor);
            nvgText(args.vg, box.size.x * 0.25f, box.size.y * 0.15f, "STOP", NULL);
        }

    }

    void onHoverScroll(const event::HoverScroll& e) override {
        float change = 1.0f;
        if(source) {
            if(e.scrollDelta.y < 0.0f) {
                change *= -1.0f;
            }
            int id = touchZones.findTouch(e.pos);
            switch(id) {
                case ZONE_TEMPO:
                    if(shift) change *= 0.1f;
                    source->midiClockAdjustTempo(change);
                    break;
                case ZONE_DIV:
                    source->midiClockAdjustOutputDiv(change);
                    break;
            }
            e.consume(NULL);
            return;
        }
        TransparentWidget::onHoverScroll(e);
    }

    void onButton(const event::Button& e) override {
        if(e.action == GLFW_RELEASE) {
            return;
        }
        if(source) {
            int id = touchZones.findTouch(e.pos);
            switch(id) {
                case ZONE_TEMPO:
                    source->midiClockTapTempo();
                    break;
                case ZONE_RUNSTOP:
                    source->midiClockToggleRunState();
                    break;
                case ZONE_AUTOSTART:
                    source->midiClockToggleAutostart();
                    break;
            }
            e.consume(NULL);
            return;
        }
    }

    void onHoverKey(const event::HoverKey& e) override {
        if(e.key == GLFW_KEY_LEFT_SHIFT || e.key == GLFW_KEY_RIGHT_SHIFT) {
            if(e.action == GLFW_PRESS) {
                shift = 1;
            }
            else if(e.action == GLFW_RELEASE) {
                shift = 0;
            }
        }
        TransparentWidget::onHoverKey(e);
    }

    // must do this so we get leave events
    void onHover(const HoverEvent& e) override {
        e.consume(this);
        TransparentWidget::onHover(e);
    }

    void onLeave(const event::Leave& e) override {
        shift = 0;
        TransparentWidget::onLeave(e);
    }
};

struct MIDI_ClockWidget : ModuleWidget {
	MIDI_ClockWidget(MIDI_Clock* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MIDI_Clock.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        MidiClockDisplay *disp = new MidiClockDisplay(mm2px(Vec(20.32, 22.446)), mm2px(Vec(32.0, 16.0)));
        disp->source = module;
        addChild(disp);

        addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(13.32, 40.446)), module, MIDI_Clock::RESET_SW));
        addParam(createParamCentered<KilpatrickD6RWhiteButton>(mm2px(Vec(27.32, 40.446)), module, MIDI_Clock::RUNSTOP_SW));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 60.5)), module, MIDI_Clock::CLOCK_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.32, 60.5)), module, MIDI_Clock::MIDI_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 76.5)), module, MIDI_Clock::TEMPO_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 92.5)), module, MIDI_Clock::RUN_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 108.5)), module, MIDI_Clock::RESET_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.32, 76.516)), module, MIDI_Clock::MIDI_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.32, 92.5)), module, MIDI_Clock::CLOCK_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.32, 108.5)), module, MIDI_Clock::RESET_OUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 60.5)), module, MIDI_Clock::CLOCK_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 60.5)), module, MIDI_Clock::MIDI_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 76.5)), module, MIDI_Clock::TEMPO_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 76.5)), module, MIDI_Clock::MIDI_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 92.5)), module, MIDI_Clock::RUN_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 92.5)), module, MIDI_Clock::CLOCK_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 108.5)), module, MIDI_Clock::RESET_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 108.5)), module, MIDI_Clock::RESET_OUT_LED));
	}
};

Model* modelMIDI_Clock = createModel<MIDI_Clock, MIDI_ClockWidget>("MIDI_Clock");
