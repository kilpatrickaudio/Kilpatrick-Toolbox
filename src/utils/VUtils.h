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

// check of an object is an instance of
using namespace std;
template<typename Base, typename T>
inline bool instanceof(const T*) {
   return is_base_of<Base, T>::value;
}

};

#endif
