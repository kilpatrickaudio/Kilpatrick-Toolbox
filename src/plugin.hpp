/*
 * Kilpatrick Toolbox Plugin Top
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
#pragma once
#include <rack.hpp>

using namespace rack;

// plugin definitions
extern Plugin* pluginInstance;

// model definitions
extern Model* modelStereo_Meter;
extern Model* modelTest_Osc;
extern Model* modelQuad_Panner;
extern Model* modelQuad_Encoder;
extern Model* modelMIDI_Monitor;
extern Model* modelMIDI_Repeater;
extern Model* modelMIDI_Merger;
extern Model* modelMIDI_Mapper;
extern Model* modelMIDI_Input;
extern Model* modelMIDI_Output;
extern Model* modelMIDI_CV;
extern Model* modelMIDI_Channel;

// settings
extern NVGcolor MIDI_LABEL_FG_COLOR;
extern NVGcolor MIDI_LABEL_BG_COLOR;
#define PITCH_GATE_SMOOTHING (1.0 / 10000.0f);  // smothing for pitch and gate
#define CC_CV_SMOOTHING (1.0 / 100.0f)  // smoothing for CC to CV
#define MIDI_RT_TASK_RATE 4000  // MIDI handler rate

#define PLATFORM_VCV
