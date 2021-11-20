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
#include "utils/MenuHelper.h"
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

    // get whether the source is synced
    virtual int midiClockDisplayIsSourceSynced(void) { return 1; }

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

    // toggle int/ext source
    virtual void midiClockToggleSource(void) { }
};

struct MIDI_Clock : Module, MidiClockPllHandler, MidiClockDisplaySource {
	enum ParamId {
		RESET_SW,
		RUNSTOP_SW,
        TEMPO,  // 30.0 to 300.0 = BPM
        OUTPUT_DIV,  // 1.0-24.0 = divide ratio
        AUTOSTART_EN,  // 0 = disable, 1.0 = enable
        CLOCK_SOURCE,  // 0 = ext, 1 = int
        RUN_IN_MODE,  // 0 = momentary, 1 = run, 2 = toggle
		PARAMS_LEN
	};
	enum InputId {
        RUN_IN,
		STOP_IN,
		CLOCK_IN,
		RESET_IN,
        MIDI_IN,
		INPUTS_LEN
	};
	enum OutputId {
		MIDI_OUT,
		CLOCK_OUT,
		RESET_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
        RUN_IN_LED,
        STOP_IN_LED,
        CLOCK_IN_LED,
        RESET_IN_LED,
		MIDI_IN_LED,
		MIDI_OUT_LED,
		CLOCK_OUT_LED,
		RESET_OUT_LED,
		LIGHTS_LEN
	};
    static constexpr int OUTPUT_DIV_MIN = 1;
    static constexpr int OUTPUT_DIV_MAX = 24;
    static constexpr int OUT_PULSE_LEN = 4;
    static constexpr int LED_PULSE_LEN = 50;
    static constexpr int AUTOSTART_TIMEOUT = 50;
    static constexpr int ANALOG_CLOCK_TIMEOUT = 2000;
    static constexpr int RUN_IN_IGNORE_TIMEOUT = 50;
    dsp::ClockDivider taskTimer;
    CVMidi *cvMidiIn;
    CVMidi *cvMidiOut;
    putils::PosEdgeDetect resetSwEdge;
    putils::PosEdgeDetect runstopSwEdge;
    putils::PosEdgeDetect runInEdge;
    putils::PosEdgeDetect stopInEdge;
    putils::PosEdgeDetect clockInEdge;
    putils::PosEdgeDetect resetInEdge;
    putils::Pulser autostartTimeout;
    putils::Pulser runInIgnoreTimeout;
    putils::Pulser runInLedPulse;
    putils::Pulser stopInLedPulse;
    putils::Pulser clockInLedPulse;
    putils::Pulser resetInLedPulse;
    putils::Pulser clockOutPulse;
    putils::Pulser resetOutPulse;
    putils::Pulser clockOutLedPulse;
    putils::Pulser resetOutLedPulse;
    putils::Pulser analogClockTimeout;
    MidiClockPll midiClock;
    int outputDiv;  // current running output divider ratio
    int outputDivCount;
    enum RunInMode {
        RUNSTOP_MOMENTARY = 0,
        RUNSTOP_RUN,
        RUNSTOP_TOGGLE,
    };

    // constructor
	MIDI_Clock() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RESET_SW, 0.f, 1.f, 0.f, "RESET");
		configParam(RUNSTOP_SW, 0.f, 1.f, 0.f, "RUN/STOP");
        configParam(TEMPO, 30.0f, 300.0f, 120.0f, "TEMPO");
        configParam(OUTPUT_DIV, 1.0f, 24.0f, 1.0f, "OUTPUT DIV");
        configParam(AUTOSTART_EN, 0.0f, 1.0f, 0.0f, "AUTOSTART");
        configParam(CLOCK_SOURCE, 0.0f, 1.0f, 1.0f, "SOURCE");
        configParam(RUN_IN_MODE, 0.0f, 2.0f, 0.0f, "RUN IN MODE");
		configInput(CLOCK_IN, "CLOCK IN");
		configInput(MIDI_IN, "MIDI IN");
        configInput(RUN_IN, "RUN IN");
		configInput(STOP_IN, "STOP IN");
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
        float tempf;
        // handle CV MIDI
        cvMidiIn->process();
        cvMidiOut->process();

        // run tasks
        if(taskTimer.process()) {
            // delayed autostart
            if(autostartTimeout.timeout) {
                if(!autostartTimeout.update()) {
                    midiClock.resetRequest();
                    midiClock.continueRequest();
                }
            }

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

            // handle run in
            if(inputs[RUN_IN].isConnected() && !runInIgnoreTimeout.update()) {
                tempf = inputs[RUN_IN].getVoltage();
                lights[RUN_IN_LED].setBrightness(tempf * 0.2f);
                switch((int)params[RUN_IN_MODE].getValue()) {
                    case RUNSTOP_MOMENTARY:
                        // run
                        if(tempf > 1.0f && !midiClock.getRunState()) {
                            midiClock.continueRequest();
                        }
                        // stop
                        else if(tempf < 1.0f && midiClock.getRunState()) {
                            midiClock.stopRequest();
                        }
                        break;
                    case RUNSTOP_RUN:
                        // run
                        if(tempf > 1.0f && !midiClock.getRunState()) {
                            midiClock.continueRequest();
                        }
                        break;
                    case RUNSTOP_TOGGLE:
                        if(runInEdge.update(tempf > 1.0f)) {
                            if(midiClock.getRunState()) {
                                midiClock.stopRequest();
                            }
                            else {
                                midiClock.continueRequest();
                            }
                        }
                        break;
                }
                runInIgnoreTimeout.timeout = RUN_IN_IGNORE_TIMEOUT;
            }

            // handle stop in
            if(stopInEdge.update(inputs[STOP_IN].getVoltage() > 1.0f)) {
                midiClock.stopRequest();
                stopInLedPulse.timeout = LED_PULSE_LEN;
            }

            // clock and reset inputs - takes precedence over MIDI input
            if(clockInEdge.update(inputs[CLOCK_IN].getVoltage() > 1.0f)) {
                analogClockTimeout.timeout = ANALOG_CLOCK_TIMEOUT;
                midiClock.handleMidiTick();
                clockInLedPulse.timeout = LED_PULSE_LEN;
            }
            if(resetInEdge.update(inputs[RESET_IN].getVoltage() > 1.0f)) {
                analogClockTimeout.timeout = ANALOG_CLOCK_TIMEOUT;
                midiClock.resetRequest();
                resetInLedPulse.timeout = LED_PULSE_LEN;
            }
            analogClockTimeout.update();  // time out the analog clock
            handleMidiInput();

            // run the MIDI clock
            midiClock.timerTask();

            // outputs
            outputs[CLOCK_OUT].setVoltage((clockOutPulse.update() != 0) * 10.0f);
            outputs[RESET_OUT].setVoltage((resetOutPulse.update() != 0) * 10.0f);

            // LEDs
            lights[CLOCK_IN_LED].setBrightness(clockInLedPulse.update() != 0);
            lights[RESET_IN_LED].setBrightness(resetInLedPulse.update() != 0);
            lights[CLOCK_OUT_LED].setBrightness(clockOutLedPulse.update() != 0);
            lights[RESET_OUT_LED].setBrightness(resetOutLedPulse.update() != 0);
            lights[MIDI_IN_LED].setBrightness(cvMidiIn->getLedState());
            lights[MIDI_OUT_LED].setBrightness(cvMidiOut->getLedState());

            // update source param
            if((int)params[CLOCK_SOURCE].getValue() != midiClock.getSource()) {
                params[CLOCK_SOURCE].setValue(midiClock.getSource());
            }
        }
	}

    // samplerate changed
    void onSampleRateChange(void) override {
        taskTimer.setDivision((int)(APP->engine->getSampleRate() / MIDI_RT_TASK_RATE));
    }

    // module initialize
    void onReset(void) override {
        midiClock.setSource((int)params[CLOCK_SOURCE].getValue());
        midiClock.setTempo(params[TEMPO].getValue());
        outputDiv = 1;
        outputDivCount = 0;
    }

    // module added (post initialize)
    void onAdd(void) override {
        if((int)params[AUTOSTART_EN].getValue()) {
            autostartTimeout.timeout = AUTOSTART_TIMEOUT;
        }
        onReset();
    }

    // handle MIDI input
    void handleMidiInput(void) {
        midi::Message msg;
        if(cvMidiIn->getInputMessage(&msg)) {
            if(analogClockTimeout.timeout) {
                return;
            }
            if(msg.getSize() != 1) {
                return;
            }
            switch(msg.bytes[0]) {
                case MIDI_TIMING_TICK:
                    midiClock.handleMidiTick();
                    break;
                case MIDI_CLOCK_START:
                    midiClock.handleMidiStart();
                    break;
                case MIDI_CLOCK_CONTINUE:
                    midiClock.handleMidiContinue();
                    break;
                case MIDI_CLOCK_STOP:
                    midiClock.handleMidiStop();
                    break;
            }
        }
    }

    // update stored tempo
    void updateTempoParam(void) {
        if(midiClock.getSource() == MidiClockPll::SOURCE_INTERNAL) {
            params[TEMPO].setValue(midiClock.getTempo());
        }
    }

    //
    // MIDI clock callbacks
    //
    // a beat was crossed
    void midiClockBeatCrossed(void) override {
        int temp;
        temp = (int)params[OUTPUT_DIV].getValue();
        if(outputDiv != temp) {
            outputDiv = temp;
            outputDivCount = 0;
        }
    }

    // run state changed
    void midiClockRunStateChanged(int running, int reset) override {
        midi::Message msg;
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

    // tap tempo locked
    void midiClockTapTempoLocked(void) override {
        updateTempoParam();
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
                clockOutLedPulse.timeout = LED_PULSE_LEN;
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
        resetOutLedPulse.timeout = LED_PULSE_LEN;
        outputDivCount = 0;

        // MIDI out
        msg.setSize(1);
        msg.bytes[0] = MIDI_CLOCK_START;
        cvMidiOut->sendOutputMessage(msg);
    }

    // external sync state changed
    void midiClockExtSyncChanged(int synced) override {
        if((int)params[AUTOSTART_EN].getValue()) {
            midiClock.continueRequest();
        }
    }

    // get the run in mode
    int getRunInMode(void) {
        return (int)params[RUN_IN_MODE].getValue();
    }

    // set the run in mode
    void setRunInMode(int mode) {
        params[RUN_IN_MODE].setValue(mode);
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

    // get whether the source is synced
    int midiClockDisplayIsSourceSynced(void) override {
        return midiClock.isExtSynced();
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
        updateTempoParam();
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

    // toggle int/ext source
    void midiClockToggleSource(void) override {
        if(midiClock.getSource() == MidiClockPll::SOURCE_INTERNAL) {
            midiClock.setSource(MidiClockPll::SOURCE_EXTERNAL);
        }
        else {
            midiClock.setSource(MidiClockPll::SOURCE_INTERNAL);
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
    NVGcolor extSyncColor;
    NVGcolor extLossColor;
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
        rad = mm2px(1.0);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        textColor = nvgRGB(0xff, 0xff, 0xff);
        runColor = nvgRGB(0x00, 0xff, 0x00);
        stopColor = nvgRGB(0xcc, 0xcc, 0xcc);
        extLossColor = nvgRGB(0xff, 0x00, 0x00);
        extSyncColor = nvgRGB(0x00, 0xff, 0xff);
        bgColor = nvgRGBA(0x00, 0x00, 0x00, 0xff);
        fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        fontSizeSmall = 11.0f;
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
        float tempo;
        int internal, synced, div, autostart, running;
        if(source == NULL) {
            tempo = 120.0f;
            internal = 1;
            synced = 0;
            div = 1;
            autostart = 0;
            running = 0;
        }
        else {
            tempo = source->midiClockDisplayGetTempo();
            internal = source->midiClockDisplayIsSourceInternal();
            synced = source->midiClockDisplayIsSourceSynced();
            div = source->midiClockDisplayGetOutputDiv();
            autostart = source->midiClockDisplayIsAutostartEnabled();
            running = source->midiClockDisplayIsRunning();
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
            putils::format("%3.1f", tempo).c_str(), NULL);

        nvgFontSize(args.vg, fontSizeSmall);

        // int/ext state
        if(internal) {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.15f, "INT", NULL);
        }
        else {
            if(synced) {
                nvgFillColor(args.vg, extSyncColor);
            }
            else {
                nvgFillColor(args.vg, extLossColor);
            }
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.15f, "EXT", NULL);
        }

        // output divider
        nvgFillColor(args.vg, textColor);
        nvgText(args.vg, box.size.x * 0.25f, box.size.y * 0.85f,
            putils::format("d:1/%d", div).c_str(), NULL);

        // autostart enable
        if(autostart) {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.85f, "AUTO", NULL);
        }
        else {
            nvgText(args.vg, box.size.x * 0.75f, box.size.y * 0.85f, "MAN", NULL);
        }

        // run/stop state
        if(running) {
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
                case ZONE_INTEXT:
                    source->midiClockToggleSource();
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

// handle choosing the run in mode
struct MIDIClockRunModeMenuItem : MenuItem {
    MIDI_Clock *module;
    int mode;

    MIDIClockRunModeMenuItem(Module *module, int mode, std::string name) {
        this->module = dynamic_cast<MIDI_Clock*>(module);
        this->mode = mode;
        this->text = name;
        this->rightText = CHECKMARK(this->module->getRunInMode() == mode);
    }

    // the menu item was selected
    void onAction(const event::Action &e) override {
        this->module->setRunInMode(mode);
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

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 60.5)), module, MIDI_Clock::RUN_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.32, 60.5)), module, MIDI_Clock::MIDI_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 76.5)), module, MIDI_Clock::STOP_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 92.5)), module, MIDI_Clock::CLOCK_IN));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.32, 108.5)), module, MIDI_Clock::RESET_IN));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.32, 76.516)), module, MIDI_Clock::MIDI_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.32, 92.5)), module, MIDI_Clock::CLOCK_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.32, 108.5)), module, MIDI_Clock::RESET_OUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 60.5)), module, MIDI_Clock::RUN_IN_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 60.5)), module, MIDI_Clock::MIDI_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 76.5)), module, MIDI_Clock::STOP_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 76.5)), module, MIDI_Clock::MIDI_OUT_LED));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 92.5)), module, MIDI_Clock::CLOCK_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 92.5)), module, MIDI_Clock::CLOCK_OUT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(18.728, 108.5)), module, MIDI_Clock::RESET_IN_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.728, 108.5)), module, MIDI_Clock::RESET_OUT_LED));
	}

    // add menu items
    void appendContextMenu(Menu *menu) override {
        MIDI_Clock *module = dynamic_cast<MIDI_Clock*>(this->module);
        if(!module) {
            return;
        }

        // mode
        menuHelperAddSpacer(menu);
        menuHelperAddLabel(menu, "Run In Mode");
        menuHelperAddItem(menu, new MIDIClockRunModeMenuItem(module, MIDI_Clock::RUNSTOP_MOMENTARY, "Momentary"));
        menuHelperAddItem(menu, new MIDIClockRunModeMenuItem(module, MIDI_Clock::RUNSTOP_RUN, "Run"));
        menuHelperAddItem(menu, new MIDIClockRunModeMenuItem(module, MIDI_Clock::RUNSTOP_TOGGLE, "Toggle"));
    }
};

Model* modelMIDI_Clock = createModel<MIDI_Clock, MIDI_ClockWidget>("MIDI_Clock");
