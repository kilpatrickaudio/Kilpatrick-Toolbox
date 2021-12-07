/*
 * Kilpatrick Audio GUI Components
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef KACOMPONENTS_H
#define KACOMPONENTS_H

#include "../plugin.hpp"  // used for settings
#include "componentlibrary.hpp"
#include "PLog.h"
#include "PUtils.h"
#ifdef BGFX
#warning BGFX defined - implementing platform code
#include "../pLib/BGfx.h"  // callbacks for framebuffer
#endif

using namespace rack;

// SPDT slide/toggle switch
struct KilpatrickToggle2P : SvgSwitch {
    KilpatrickToggle2P() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/toggle-down.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/toggle-up.svg")));
    }
};

// SP3T slide/toggle switch
struct KilpatrickToggle3P : SvgSwitch {
    KilpatrickToggle3P() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/toggle-down.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/toggle-middle.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/toggle-up.svg")));
    }
};

// 2 pos rotary switch
struct KilpatrickRotary2P : SvgSwitch {
    KilpatrickRotary2P() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-m45.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-p45.svg")));
    }
};

// 3 pos rotary switch
struct KilpatrickRotary3P : SvgSwitch {
    KilpatrickRotary3P() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-m45.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-p45.svg")));
    }
};

// 5 pos rotary switch
struct KilpatrickRotary5P : SvgSwitch {
    KilpatrickRotary5P() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-m90.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-m45.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-p45.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/knob-black_red-sw-p90.svg")));
    }
};

struct KilpatrickButton;

// callback handler for buttons
struct KilpatrickButtonHandler {
    // handle a button being deselected - used for mapping
    virtual void buttonDeselect(int id, int armed) { }

    // handle latch mode toggle
    virtual void buttonLatchToggle(int id) { }

    // check if we should be in latch mode
    virtual bool getLatchMode(int id) { return false; }
};

// momentary pushbutton
struct KilpatrickButton : SvgSwitch {
    int id;
    int armed;
    int armable;
    int mapKey;
    int latchKey;
    KilpatrickButtonHandler *handler;
    float armDotSize;
    NVGcolor armDotColor;
    NVGcolor latchColor;

    KilpatrickButton(std::string svgFilenameReleased, std::string svgFilenamePressed) {
        momentary = true;
        addFrame(APP->window->loadSvg(svgFilenameReleased));
        addFrame(APP->window->loadSvg(svgFilenamePressed));
        handler = NULL;
        armed = 0;
        armable = 0;  // set to 1 to allow double-click arming
        mapKey = 0;
        latchKey = 0;
        armDotSize = 6.0;
        armDotColor = nvgRGB(0x00, 0xff, 0xff);
        latchColor = nvgRGB(0xff, 0x00, 0x00);
    }

    void draw(const DrawArgs& args) override {
        SvgSwitch::draw(args);
        if(handler) {
            if(handler->getLatchMode(id)) {
                momentary = false;
            }
            else {
                momentary = true;
            }
        }
        if(armed) {
            nvgSave(args.vg);
            nvgFillColor(args.vg, armDotColor);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, box.size.x * 0.5, box.size.y * 0.5, armDotSize);
            nvgClosePath(args.vg);
            nvgFill(args.vg);
            nvgRestore(args.vg);
        }
        if(momentary == false) {
            nvgSave(args.vg);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, box.size.x * 0.5, box.size.y * 0.5, (box.size.x * 0.5) + 1.0f);
            nvgClosePath(args.vg);
            nvgStrokeColor(args.vg, latchColor);
            nvgStrokeWidth(args.vg, 2.0f);
            nvgStroke(args.vg);
            nvgRestore(args.vg);
        }
    }

    void onDeselect(const event::Deselect& e) override {
        if(handler) {
            handler->buttonDeselect(id, armed);
        }
        armed = 0;
        SvgSwitch::onDeselect(e);
    }

    void onButton(const event::Button& e) override {
        if(e.action == GLFW_PRESS) {
            if(armable && mapKey) {
                armed = 1;
            }
            if(handler && latchKey) {
                handler->buttonLatchToggle(id);
            }
        }
        SvgSwitch::onButton(e);
    }

    void onHoverKey(const event::HoverKey& e) override {
        if(e.key == GLFW_KEY_M) {
            mapKey = 1;
        }
        else if(e.key == GLFW_KEY_L) {
            latchKey = 1;
        }
        SvgSwitch::onHoverKey(e);
    }

    // must do this so we get leave events
    void onHover(const HoverEvent& e) override {
        e.consume(this);
        SvgSwitch::onHover(e);
    }

    void onLeave(const event::Leave& e) override {
        mapKey = 0;
        latchKey = 0;
        SvgSwitch::onLeave(e);
    }
};

// momentary pushbutton
struct KilpatrickD6RRedButton : KilpatrickButton {
    KilpatrickD6RRedButton() : KilpatrickButton(asset::plugin(pluginInstance, "res/components/button-D6R-red-released.svg"),
        asset::plugin(pluginInstance, "res/components/button-D6R-red-pressed.svg")) {
    }
};

// momentary pushbutton
struct KilpatrickD6RYellowButton : KilpatrickButton {
    KilpatrickD6RYellowButton() : KilpatrickButton(asset::plugin(pluginInstance, "res/components/button-D6R-yellow-released.svg"),
        asset::plugin(pluginInstance, "res/components/button-D6R-yellow-pressed.svg")) {
    }
};

// momentary pushbutton
struct KilpatrickD6RGreenButton : KilpatrickButton {
    KilpatrickD6RGreenButton() : KilpatrickButton(asset::plugin(pluginInstance, "res/components/button-D6R-green-released.svg"),
        asset::plugin(pluginInstance, "res/components/button-D6R-green-pressed.svg")) {
    }
};

// momentary pushbutton
struct KilpatrickD6RGreyButton : KilpatrickButton {
    KilpatrickD6RGreyButton() : KilpatrickButton(asset::plugin(pluginInstance, "res/components/button-D6R-grey-released.svg"),
        asset::plugin(pluginInstance, "res/components/button-D6R-grey-pressed.svg")) {
    }
};

// momentary pushbutton
struct KilpatrickD6RWhiteButton : KilpatrickButton {
    KilpatrickD6RWhiteButton() : KilpatrickButton(asset::plugin(pluginInstance, "res/components/button-D6R-white-released.svg"),
        asset::plugin(pluginInstance, "res/components/button-D6R-white-pressed.svg")) {
    }
};

// momentary pushbutton
struct KilpatrickSmallRedButton : KilpatrickButton {
    KilpatrickSmallRedButton() : KilpatrickButton(asset::plugin(pluginInstance, "res/components/button-small-red-released.svg"),
        asset::plugin(pluginInstance, "res/components/button-small-red-pressed.svg")) {
    }
};

// callback handler for knobs
struct KilpatrickKnobHandler {
    // handle a knob being deselected
    virtual void knobDeselect(int id, int armed) { }
};

// knob
struct KilpatrickKnob : SvgKnob {
    int id;
    int armed;
    int armable;
    int mapKey;
    KilpatrickKnobHandler *handler;
    float armDotSize;
    NVGcolor armDotColor;

    KilpatrickKnob(std::string svgFilename) {
        minAngle = -0.8334 * M_PI;
        maxAngle = 0.8334 * M_PI;
        shadow->opacity = 0;
        setSvg(APP->window->loadSvg(svgFilename));
        handler = NULL;
        armed = 0;
        armable = 0;  // set to 1 to allow double-click arming
        mapKey = 0;
        armDotSize = 6.0;
        armDotColor = nvgRGB(0x00, 0xff, 0xff);
    }

    void draw(const DrawArgs& args) override {
        SvgKnob::draw(args);
        if(armed) {
            nvgSave(args.vg);
            nvgFillColor(args.vg, armDotColor);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, box.size.x * 0.5, box.size.y * 0.5, armDotSize);
            nvgClosePath(args.vg);
    		nvgFill(args.vg);
            nvgRestore(args.vg);
        }
    }

    void onDeselect(const event::Deselect& e) override {
        if(handler) {
            handler->knobDeselect(id, armed);
        }
        armed = 0;
        SvgKnob::onDeselect(e);
    }

    void onButton(const event::Button& e) override {
        if(e.action == GLFW_PRESS) {
            if(armable && mapKey) {
                armed = 1;
            }
        }
        SvgKnob::onButton(e);
    }

    void onHoverKey(const event::HoverKey& e) override {
        if(e.key == GLFW_KEY_M) {
            mapKey = 1;
        }
        SvgKnob::onHoverKey(e);
    }

    // must do this so we get leave events
    void onHover(const HoverEvent& e) override {
        e.consume(this);
        SvgKnob::onHover(e);
    }

    void onLeave(const event::Leave& e) override {
        mapKey = 0;
        SvgKnob::onLeave(e);
    }
};

// grey knob with grey
struct KilpatrickKnobGreyGrey : KilpatrickKnob {
    KilpatrickKnobGreyGrey() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-grey_grey.svg")) {
    }
};

// black knob with red
struct KilpatrickKnobBlackRed : KilpatrickKnob {
    KilpatrickKnobBlackRed() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-black_red.svg")) {
    }
};

// black knob with red - mini
struct KilpatrickKnobBlackRedMini : KilpatrickKnob {
    KilpatrickKnobBlackRedMini() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-black_red-mini.svg")) {
    }
};

// black aluminum knob with pointer
struct KilpatrickKnobBlackAlumPointer : KilpatrickKnob {
    KilpatrickKnobBlackAlumPointer() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-black-alum-pointer.svg")) {
    }
};

// clear aluminum knob with pointer
struct KilpatrickKnobClearAlumPointer : KilpatrickKnob {
    KilpatrickKnobClearAlumPointer() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-clear-alum-pointer.svg")) {
    }
};

// clear aluminum knob
struct KilpatrickKnobClearAlum : KilpatrickKnob {
    KilpatrickKnobClearAlum() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-clear-alum.svg")) {
    }
};

// large clear aluminum knob with pointer
struct KilpatrickKnobClearLargeAlumPointer : KilpatrickKnob {
    KilpatrickKnobClearLargeAlumPointer() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-clear-large-alum-pointer.svg")) {
    }
};

// large clear aluminum knob
struct KilpatrickKnobClearLargeAlum : KilpatrickKnob {
    KilpatrickKnobClearLargeAlum() : KilpatrickKnob(asset::plugin(pluginInstance, "res/components/knob-clear-large-alum.svg")) {
    }
};

// callback handler for sliders
struct KilpatrickSliderHandler {
    // handle a slider being deselected
    virtual void sliderDeselect(int id, int armed) { }
};

// slide pot black knob with red
struct KilpatrickSlidePot : app::SvgSlider {
    int id;
    int armed;
    int armable;
    int mapKey;
    KilpatrickSliderHandler *handler;
    float armDotSize;
    NVGcolor armDotColor;

	KilpatrickSlidePot() {
        setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/slider-45mm-bg.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/slider-knob-black-red.svg")));
        setTravel(45.0);
        handler = NULL;
        armed = 0;
        armable = 0;  // set to 1 to allow double-click arming
        mapKey = 0;
        armDotSize = 6.0;
        armDotColor = nvgRGB(0x00, 0xff, 0xff);
    }

    void draw(const DrawArgs& args) override {
        SvgSlider::draw(args);
        if(armed) {
            nvgSave(args.vg);
            nvgFillColor(args.vg, armDotColor);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, box.size.x * 0.5, box.size.y * 0.5, armDotSize);
            nvgClosePath(args.vg);
    		nvgFill(args.vg);
            nvgRestore(args.vg);
        }
    }

    // set the travel and position all the components
    void setTravel(float travel) {
        travel = mm2px(travel);
        math::Vec margin = math::Vec(handle->box.size).mult(0.5);
        float handleX = -margin.x + (background->box.size.x * 0.5);
        float handleY = margin.y;
        float trackOffset = (background->box.size.y - travel) * 0.5;
        maxHandlePos = math::Vec(handleX, trackOffset - handleY);
        minHandlePos = math::Vec(handleX, trackOffset - handleY + travel);
    }

    void onDeselect(const event::Deselect& e) override {
        if(handler) {
            handler->sliderDeselect(id, armed);
        }
        armed = 0;
        SvgSlider::onDeselect(e);
    }

    void onButton(const event::Button& e) override {
        if(e.action == GLFW_PRESS) {
            if(armable && mapKey) {
                armed = 1;
            }
        }
        SvgSlider::onButton(e);
    }

    void onHoverKey(const event::HoverKey& e) override {
        if(e.key == GLFW_KEY_M) {
            mapKey = 1;
        }
        SvgSlider::onHoverKey(e);
    }

    // must do this so we get leave events
    void onHover(const HoverEvent& e) override {
        e.consume(this);
        SvgSlider::onHover(e);
    }

    void onLeave(const event::Leave& e) override {
        mapKey = 0;
        SvgSlider::onLeave(e);
    }
};

// slide pot black knob with red
struct KilpatrickSlidePotBlackWhite : KilpatrickSlidePot {
    KilpatrickSlidePotBlackWhite() : KilpatrickSlidePot() {
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/slider-knob-black-white.svg")));
    }
};

struct KilpatrickLabel;

// a handler to deal with updating a label
struct KilpatrickLabelHandler {
    // update the label
    virtual std::string updateLabel(int id) { return ""; }

    // handle button on the label - return 1 if consumed the event, 0 otherwise
    virtual int onLabelButton(int id, const event::Button& e) { return 0; }

    // handle scroll on the label - return 1 if consumed the event, 0 otherwise
    virtual int onLabelHoverScroll(int id, const event::HoverScroll& e) { return 0; }
};

// a custom label
struct KilpatrickLabel : widget::TransparentWidget {
    int id;
    KilpatrickLabelHandler *handler;
	std::string text;
    std::string fontFilename;
	float fontSize;
    float rad;
    int hAlign;
    int vAlign;
	NVGcolor fgColor;
    NVGcolor bgColor;

    // create a new label
    KilpatrickLabel(int id, math::Vec pos, math::Vec size) {
        this->id = id;
        this->handler = NULL;
        text = "";
        fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        fontSize = 8;
        rad = mm2px(3.0);
        hAlign = NVG_ALIGN_CENTER;
        vAlign = NVG_ALIGN_MIDDLE;
        fgColor = nvgRGB(0xe0, 0xe0, 0xe0);
        bgColor = nvgRGB(0x30, 0x40, 0x90);
        box.pos = pos.minus(size.div(2));
        box.size = size;
    }

    void draw(const DrawArgs& args) override {
        float xPos, yPos;
        float pos[4];
        Widget::draw(args);
    	nvgBeginPath(args.vg);
    	nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, rad);
    	nvgFillColor(args.vg, bgColor);
    	nvgFill(args.vg);
        std::shared_ptr<Font> font = APP->window->loadFont(fontFilename);
    	if(font->handle >= 0) {
            if(handler) {
                text = handler->updateLabel(id);
            }
            nvgFontFaceId(args.vg, font->handle);
        	nvgFontSize(args.vg, fontSize);
        	nvgFillColor(args.vg, fgColor);
            nvgTextAlign(args.vg, hAlign | vAlign);
            nvgTextBoxBounds(args.vg, 0.0, 0.0, box.size.x, text.c_str(), NULL, pos);
            xPos = (box.size.x - pos[2] - pos[0]) * 0.5f;
            yPos = (box.size.y - pos[3] - pos[1]) * 0.5f;
            nvgTextBox(args.vg, xPos, yPos, box.size.x, text.c_str(), NULL);
        }
    }

	void onButton(const event::Button& e) override {
        if(handler) {
            if(handler->onLabelButton(id, e)) {
                e.consume(NULL);
            }
        }
        TransparentWidget::onButton(e);
    }

    void onHoverScroll(const event::HoverScroll& e) override {
        if(handler) {
            if(handler->onLabelHoverScroll(id, e)) {
                e.consume(NULL);
            }
        }
        TransparentWidget::onHoverScroll(e);
    }
};

// create a framebuffer - normal 24 bit mode
struct KilpatrickFramebufferRGB888 : widget::TransparentWidget {
    float rad = 3.0f;
    NVGcolor bgColor;
    int32_t *fb;
    int fbW, fbH;
    float imgScale = 1.0f;
    float drawScale = 1.0f;
    int dirty = 0;
    // drawing settings
    int drawX1, drawY1, drawX2, drawY2;
    NVGpaint paint;

    // create a new framebuffer
    KilpatrickFramebufferRGB888(int id, math::Vec pos, math::Vec size, int w, int h) {
        bgColor = nvgRGB(0x30, 0x40, 0x90);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        this->fbW = w;
        this->fbH = h;
        fb = new int32_t[fbW * fbH];
        clear(0x000000);
        setXY(0, 0, fbW, fbH);
    }

    // destructor
    ~KilpatrickFramebufferRGB888() {
        delete fb;
    }

    void draw(const DrawArgs& args) override {
        // create image - don't save it
        int img = nvgCreateImageRGBA(args.vg, fbW, fbH, 0, (unsigned char *)fb);
        drawScale = mm2px(box.size.x / (float)fbW);
        imgScale = 1.0f / drawScale;
        nvgUpdateImage(args.vg, img, (unsigned char *)fb);
        paint = nvgImagePattern(args.vg, 0, 0, fbW, fbH, 0, img, 1.0f);

        Widget::draw(args);
        nvgScale(args.vg, imgScale, imgScale);
    	nvgBeginPath(args.vg);
    	nvgRoundedRect(args.vg, 0, 0, box.size.x * drawScale, box.size.y * drawScale, rad);
        nvgFillPaint(args.vg, paint);  // fb texture
    	nvgFill(args.vg);
    }

    // set the XY drawing position for the screen
    void setXY(int x, int y, int w, int h) {
        if(x < 0 || (x + w) > fbW) return;
        if(y < 0 || (y + h) > fbH) return;
        drawX1 = x;
        drawY1 = y;
        drawX2 = drawX1 + w;
        drawY2 = drawY1 + h;
    }

    // send pixels in RRGGBB format
    void sendPixels(int32_t *buf, int len) {
        int32_t *bufp;
        int i, xPos, yPos;
        xPos = drawX1;
        yPos = drawY1;
        bufp = buf;
        for(i = 0; i < len; i ++) {
            // reverse byte order to AABBGGRR
            *(fb + (yPos * fbW) + xPos) = 0xff000000 |
                ((*bufp & 0xff) << 16) |  // B
                (*bufp & 0xff00) |  // G
                ((*bufp >> 16) & 0xff);  // R
            bufp ++;
            xPos ++;
            if(xPos == drawX2) {
                xPos = drawX1;
                yPos ++;
            }
        }
        dirty = 1;
    }

    // clear the screen with a color - color = RRGGBB
    void clear(int32_t color) {
        int32_t *p = fb;
        int i;
        for(i = 0; i < (fbW * fbH); i ++) {
            *p = 0xff000000 |
                ((color & 0xff) << 16) |  // B
                (color & 0xff00) |  // G
                ((color >> 16) & 0xff);  // R
            p ++;
        }
        dirty = 1;
    }
};

#ifdef BGFX
// create a framebuffer - RGB565 mode (for embedded)
struct KilpatrickFramebufferRGB565 : widget::TransparentWidget, BGfxScreen {
    float rad = 3.0f;
    NVGcolor bgColor;
    int32_t *fb;
    int fbW, fbH;
    float imgScale = 1.0f;
    float drawScale = 1.0f;
    int dirty = 0;
    // drawing settings
    int drawX1, drawY1, drawX2, drawY2;
    NVGpaint paint;

    // create a new framebuffer
    KilpatrickFramebufferRGB565(int id, math::Vec pos, math::Vec size, int w, int h) {
        bgColor = nvgRGB(0x30, 0x40, 0x90);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        this->fbW = w;
        this->fbH = h;
        fb = new int32_t[fbW * fbH];
        clear(0x000000);
        setXY(0, 0, fbW, fbH);
    }

    // destructor
    ~KilpatrickFramebufferRGB565() {
        delete fb;
    }

    void draw(const DrawArgs& args) override {
        // create image - don't save it
        int img = nvgCreateImageRGBA(args.vg, fbW, fbH, 0, (unsigned char *)fb);
        drawScale = mm2px(box.size.x / (float)fbW);
        imgScale = 1.0f / drawScale;
        nvgUpdateImage(args.vg, img, (unsigned char *)fb);
        paint = nvgImagePattern(args.vg, 0, 0, fbW, fbH, 0, img, 1.0f);
        dirty = 0;

        Widget::draw(args);
        nvgScale(args.vg, imgScale, imgScale);
    	nvgBeginPath(args.vg);
    	nvgRoundedRect(args.vg, 0, 0, box.size.x * drawScale, box.size.y * drawScale, rad);
        nvgFillPaint(args.vg, paint);  // fb texture
    	nvgFill(args.vg);
    }

    // set the XY drawing position for the screen
    void setXY(int x, int y, int w, int h) override {
        if(x < 0 || (x + w) > fbW) return;
        if(y < 0 || (y + h) > fbH) return;
        drawX1 = x;
        drawY1 = y;
        drawX2 = drawX1 + w;
        drawY2 = drawY1 + h;
    }

    // send pixels in 16 bit RGB565 format
    void sendPixels(int16_t *buf, int len) override {
        int16_t *bufp;
        int i, xPos, yPos;
        xPos = drawX1;
        yPos = drawY1;
        bufp = buf;
        for(i = 0; i < len; i ++) {
            *(fb + (yPos * fbW) + xPos) = 0xff000000 |
                ((*bufp & 0x001f) << 19) |  // B
                ((*bufp & 0x07e0) << 5) |  // G
                ((*bufp & 0xf800) >> 8);  // R
            bufp ++;
            xPos ++;
            if(xPos == drawX2) {
                xPos = drawX1;
                yPos ++;
            }
        }
        dirty = 1;
    }

    // clear the screen with a color - color = 16 bit RGB565 format
    void clear(int16_t color) override {
        int32_t *p = fb;
        int i;
        for(i = 0; i < (fbW * fbH); i ++) {
            *p = 0xff000000 |
                ((color & 0x001f) << 19) |  // B
                ((color & 0x07e0) << 5) |  // G
                ((color & 0xf800) >> 8);  // R
            p ++;
        }
        dirty = 1;
    }
};
#endif

// bipolar signal red/green LED mapper
inline void KARedGreenLEDMap(Light& lightR, Light& lightG, float level) {
    lightR.setBrightness(putils::clampf(-level, 0.0f, 1.0f));  // red
    lightG.setBrightness(putils::clampf(level, 0.0f, 1.0f));  // green
}

// levelmeter
struct KALevelmeter {
    NVGcolor textColor;
    NVGcolor textRefColor;
    NVGcolor bgColor;
    NVGcolor barColor;
    NVGcolor peakColor;
    math::Vec pos;
    math::Vec size;
    float peakHeight = 2.0f;
    float minLevel;  // the min level in dB to display
    float dbScale;  // how to scale the value to the vertical height
    std::string fontFilename;
    float fontSizeReadout;  // size of the dB readout
    float readoutH;  // height of readout box
    int drawReadout;  // 1 = draw the level readout under the bar
    float refLevel;  // reference adjustment level
    int refLevelChangeTime = 10;  // timeout for showing ref level when changed - divided by textSlowdownCount
    // running state
    float level;  // dB level -96.0f to 0.0f
    float peak;  // dB level -96.0f to 0.0f
    int textSlowCount = 9999;
    int textSlowdown = 0;  // slowdown level (0 = disable)
    int textDrawDecimal = 1;  // default on
    char peakStr[64];
    int refLevelChangeTimeout = 0;

    // constructor
    KALevelmeter() {
        textColor = nvgRGBA(0xff, 0xff, 0xff, 0xff);
        textRefColor = nvgRGBA(0xff, 0x00, 0x00, 0xff);
        bgColor = nvgRGBA(0x33, 0x73, 0x33, 0xff);
        barColor = nvgRGBA(0x00, 0xe0, 0x00, 0xff);
        peakColor = nvgRGBA(0xe0, 0x00, 0x00, 0xff);
        level = -96.0f;
        peak = -96.0f;
        pos.x = 0.0f;
        pos.y = 0.0f;
        size.x = 5.0f;
        size.y = 100.0f;
        fontFilename = asset::plugin(pluginInstance, "res/components/fixedsys.ttf");
        fontSizeReadout = 9.5f;
        drawReadout = 1;
        readoutH = 12.0f;
        refLevel = 0.0f;
        sprintf(peakStr, " ");
    }

    // set the levels
    void setLevels(float level, float peak) {
        this->level = putils::clampf(level, -96.0f, 0.0f);
        this->peak = putils::clampf(peak, -96.0f, 0.0f);
    }

    // set the min level
    void setMinLevel(float minLevel) {
        this->minLevel = putils::clampf(minLevel, -96.0f, 0.0f);
        dbScale = size.y / -this->minLevel;
    }

    // set the reference level
    void setRefLevel(float level) {
        if(level != refLevel) {
            refLevelChangeTimeout = refLevelChangeTime;
        }
        refLevel = level;
    }

    // draw - the background must be cleared
    void draw(const widget::Widget::DrawArgs& args) {
        float tempf;

		nvgSave(args.vg);
        nvgTranslate(args.vg, pos.x, pos.y);

        std::shared_ptr<Font> font = APP->window->loadFont(fontFilename);

        // readout
        if(drawReadout) {
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, bgColor);
            nvgRect(args.vg, 0.0f, 0.0f, size.x, readoutH);
            nvgFill(args.vg);

            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFontFaceId(args.vg, font->handle);
            if(refLevel != 0) {
                nvgFillColor(args.vg, textRefColor);
            }
            else {
                nvgFillColor(args.vg, textColor);
            }
            nvgFontSize(args.vg, fontSizeReadout);

            textSlowCount ++;
            if(textSlowCount > textSlowdown) {
                if(refLevelChangeTimeout) {
                    refLevelChangeTimeout --;
                    sprintf(peakStr, "%3.1f", refLevel);
                }
                else if(textDrawDecimal) {
                    sprintf(peakStr, "%3.1f", level + refLevel);
                }
                else {
                    sprintf(peakStr, "%3.0f", level + refLevel);
                }
                textSlowCount = 0;
            }
            nvgText(args.vg, size.x * 0.5f, (readoutH * 0.5f), peakStr, NULL);
            nvgTranslate(args.vg, 0.0f, readoutH + 2.0f);
        }

        nvgBeginPath(args.vg);
        nvgFillColor(args.vg, bgColor);
        nvgRect(args.vg, 0.0f, 0.0f, size.x, size.y);
        nvgFill(args.vg);

        // level
        if(level > minLevel) {
            tempf = putils::clampf(level, minLevel, 0.0f);
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, barColor);
            nvgRect(args.vg, 0.0f, -tempf * dbScale, size.x,
                (-minLevel + tempf) * dbScale);
            nvgFill(args.vg);
        }

        // peak
        if(peak > minLevel) {
            tempf = putils::clampf(peak, minLevel, 0.0f);
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, peakColor);
            nvgRect(args.vg, 0.0f, -tempf * dbScale, size.x, peakHeight);
            nvgFill(args.vg);
        }

		nvgRestore(args.vg);
    }
};

// handler for joystick movement
struct KilpatrickJoystickHandler {
    // handle the joystick position
    virtual void updateJoystick(int id, float xPos, float yPos) { }

    // check if the reset button has been pressed
    virtual int resetJoystick(void) { return 0; }
};

// a custom label
struct KilpatrickJoystick : widget::OpaqueWidget {
    int id;
    KilpatrickJoystickHandler *handler = NULL;
    NVGcolor bgColor;
    NVGcolor knobColor;
    float xPos = 0.0f;
    float yPos = 0.0f;
    float startX, startY;
    float moveScale;
    float controlAreaScale = 1.0f;
    static constexpr float borderSize = 0.5f;  // size for detecting edge buttons
    int snap = 0;

    // create a new label
    KilpatrickJoystick(int id, math::Vec pos, math::Vec size) {
        this->id = id;
        bgColor = nvgRGBA(0x33, 0x33, 0x90, 0xff);
        knobColor = nvgRGBA(0xff, 0x00, 0x00, 0xff);
        box.pos = pos.minus(size.div(2));
        box.size = size;
        moveScale = 1.0 / box.size.x;
    }

    void draw(const DrawArgs& args) override {
        Widget::draw(args);

        if(handler && handler->resetJoystick()) {
            xPos = 0.0f;
            yPos = 0.0f;
            handler->updateJoystick(id, xPos, -yPos);
        }

    	nvgBeginPath(args.vg);
        nvgEllipse(args.vg, (box.size.x * 0.5) + (xPos * controlAreaScale * box.size.x * 0.5),
            (box.size.y * 0.5) + (-yPos * controlAreaScale * box.size.y * 0.5), 10.0, 10.0);
    	nvgFillColor(args.vg, knobColor);
    	nvgFill(args.vg);
    }

    // handle edge snapping
	void onButton(const event::Button& e) override {
        float posX = ((e.pos.x / box.size.x) * 2.0f) - 1.0f;
        float posY = -(((e.pos.y / box.size.y) * 2.0f) - 1.0f);

        e.consume(this);  // prevent entire module from dragging
        startX = posX;
        startY = posY;

        if(e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }

        if(!snap) {
            return;
        }

        // top
        if(posY > 1.0f - borderSize) {
            // left
            if(posX < -1.0f + borderSize) {
                xPos = -1.0f;
                yPos = 1.0f;
            }
            // centre
            else if(posX > -0.25f && posX < 0.25f) {
                xPos = 0.0f;
                yPos = 1.0f;
            }
            // right
            else if(posX > 1.0f - borderSize) {
                xPos = 1.0f;
                yPos = 1.0f;
            }
        }
        // front/back centre
        else if(posY > -0.25f && posY < 0.25f) {
            // left
            if(posX < -1.0f + borderSize) {
                xPos = -1.0f;
                yPos = 0.0f;
            }
            // right
            else if(posX > 1.0f - borderSize) {
                xPos = 1.0f;
                yPos = 0.0f;
            }
        }
        // bottom
        else if(posY > -1.0f - borderSize) {
            // left
            if(posX < -1.0f + borderSize) {
                xPos = -1.0f;
                yPos = -1.0f;
            }
            // centre
            else if(posX > -0.25f && posX < 0.25f) {
                xPos = 0.0f;
                yPos = -1.0f;
            }
            // right
            else if(posX > 1.0f - borderSize) {
                xPos = 1.0f;
                yPos = -1.0f;
            }
        }

        if(handler) {
            handler->updateJoystick(id, xPos, yPos);
        }
    }

    // dragging
	void onDragHover(const event::DragHover& e) override {
        float newX, newY;
        newX = ((e.pos.x / box.size.x) * 2.0f) - 1.0f;
        newY = -(((e.pos.y / box.size.y) * 2.0f) - 1.0f);

        xPos = putils::clampf(xPos + (newX - startX), -1.0f, 1.0f);
        yPos = putils::clampf(yPos + (newY - startY), -1.0f, 1.0f);
        startX = newX;
        startY = newY;

        if(handler) {
            handler->updateJoystick(id, xPos, yPos);
        }
    }

    // key
    void onHoverKey(const event::HoverKey& e) override {
        if(e.key == GLFW_KEY_P) {
            PDEBUG("snap");
            snap = 1;
        }
    }

    // must do this so we get leave events
    void onHover(const HoverEvent& e) override {
        e.consume(this);
        OpaqueWidget::onHover(e);
    }

    void onLeave(const event::Leave& e) override {
        snap = 0;
    }

    void onDragLeave(const event::DragLeave& e) override {
        snap = 0;
    }
};

#endif
