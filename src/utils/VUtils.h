/*
 * Kilpatrick Audio VCV Util Functions
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef VUTILS_H
#define VUTILS_H

#include "plugin.hpp"
#include <cstring>
#include <cmath>

namespace vutils {

// convert a panel position to px
inline math::Vec panelin2px(float x, float y) {
    return mm2px(Vec(x * 25.4f, 128.5f - (y * 25.4f)));
}

// convert inches to px
inline math::Vec in2px(float x, float y) {
    return mm2px(Vec(x * 25.4f, y * 25.4f));
}

// convert an RGB value (0xrrggbb) to LEDs
inline void rgbToLed(std::vector<Light> *lights, int base, uint32_t val) {
    (*lights)[base].setBrightness((float)((val >> 16) & 0xff) * 0.003921569);
    (*lights)[base+1].setBrightness((float)((val >> 8) & 0xff) * 0.003921569);
    (*lights)[base+2].setBrightness((float)(val & 0xff) * 0.003921569);
}

// a zone to keep track of
struct TouchZone {
    Rect rect;
    int id;
};

// handle zone of touch on an ad-hoc display
struct TouchZones {
    std::vector<TouchZone> zones;

    // add a touch zone
    void addZone(int id, float x, float y, float w, float h) {
        TouchZone zone;
        zone.id = id;
        zone.rect.pos.x = x;
        zone.rect.pos.y = y;
        zone.rect.size.x = w;
        zone.rect.size.y = h;
        zones.push_back(zone);
    }

    // add a touch zone with centered X and Y
    void addZoneCentered(int id, float cx, float cy, float w, float h) {
        TouchZone zone;
        zone.id = id;
        zone.rect.pos.x = cx - (w * 0.5f);
        zone.rect.pos.y = cy - (h * 0.5f);
        zone.rect.size.x = w;
        zone.rect.size.y = h;
        zones.push_back(zone);
    }

    // find the first zone which a point is inside - returns -1 if none found
    int findTouch(math::Vec pos) {
        return findTouch(pos.x, pos.y);
    }

    // find the first zone which a point is inside - returns -1 if none found
    int findTouch(float x, float y) {
        std::vector<TouchZone>::iterator iter;
        for(iter = zones.begin(); iter != zones.end(); iter ++) {
            if(iter->rect.contains(Vec(x, y))) {
                return iter->id;
            }
        }
        return -1;
    }
};

// check of an object is an instance of
using namespace std;
template<typename Base, typename T>
inline bool instanceof(const T*) {
   return is_base_of<Base, T>::value;
}

// create a param with ParamWidget arg passed in
template <class TParamWidget>
TParamWidget* createArgParam(math::Vec pos, TParamWidget *o,
        engine::Module* module, int paramId) {
	o->box.pos = pos;
	o->app::ParamWidget::module = module;
	o->app::ParamWidget::paramId = paramId;
	o->initParamQuantity();
	return o;
}

// create a param with ParamWidget arg passed in
template <class TParamWidget>
TParamWidget* createArgParamCentered(math::Vec pos, TParamWidget *o,
        engine::Module* module, int paramId) {
    o->box.pos = pos;
	o->box.pos = o->box.pos.minus(o->box.size.div(2));
    o->app::ParamWidget::module = module;
	o->app::ParamWidget::paramId = paramId;
    o->initParamQuantity();
	return o;
}

};

#endif
