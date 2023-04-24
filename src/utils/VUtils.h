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

// convert HSV to RGB color
inline void hsv2rgb(float h, float s, float v, float *r, float *g, float *b) {
    float hh, p, q, t, ff;
    int i;
    if(h < 0.0f) h = 0.0f;
    else if(h > 1.0f) h = 1.0f;
    hh = h * 5.99999;
    i = (int)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));
    switch(i) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
        default:
            *r = v;
            *g = p;
            *b = q;
            break;
    }
}

// convert RGB to HSV color
inline void rgb2hsv(float r, float g, float b, float *h, float *s, float *v) {
    float min, max, delta;
    min = r;
    if(g < min) {
        min = g;
    }
    if(b < min) {
        min = b;
    }
    max = r;
    if(g > max) {
        max = g;
    }
    if(b > max) {
        max = b;
    }

    *v = max;  // v
    delta = max - min;
    if(delta < 0.00001) {
        *s = 0;
        *h = 0;
        return;
    }
    if(max > 0.0) {
        *s = delta / max;  // s
    }
    else {
        *s = 0.0;  // s
        *h = 0.0;  // actually NAN but we won't do it like that
        return;
    }
    if(r == max) {
        *h = (g - b) / delta;  // yellow to magenta
    }
    else if(g == max) {
        *h = 2.0 + ((b - r) / delta);  // cyan to yellow
    }
    else {
        *h = 4.0 + ((r - g) / delta);  // magenta and cyan
    }
    *h /= 6.0;
    if(*h < 0.0) {
        *h += 1.0;
    }
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

    // convert a touch to a position within the zone
    // if a zone is offset from the corner of the box
    // this will return 0.0, 0.0 at the top-left corner
    // of the zone and 1.0, 1.0 and the bottom right corner
    // returns -1 on not in a touch
    int getTouchPos(math::Vec pos, float *x, float *y) {
        int zn = findTouch(pos.x, pos.y);
        if(zn < 0) {
            return -1;
        }
        TouchZone zone = zones[zn];
        *x = (pos.x - zone.rect.pos.x) / zone.rect.size.x;
        *y = (pos.y - zone.rect.pos.y) / zone.rect.size.y;
        return 0;
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
