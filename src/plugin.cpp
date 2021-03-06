/*
 * Kilpatrick Toolbox Plugin Top
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
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

Plugin* pluginInstance;

// settings
NVGcolor MIDI_LABEL_FG_COLOR = nvgRGB(0x33, 0x33, 0x33);
NVGcolor MIDI_LABEL_BG_COLOR = nvgRGB(0xcc, 0xcc, 0xcc);

void init(Plugin* p) {
	pluginInstance = p;
    p->addModel(modelStereo_Meter);
    p->addModel(modelTest_Osc);
    p->addModel(modelQuad_Panner);
    p->addModel(modelQuad_Encoder);
    p->addModel(modelQuad_Decoder);
    p->addModel(modelMIDI_Monitor);
    p->addModel(modelMIDI_Repeater);
    p->addModel(modelMIDI_Merger);
    p->addModel(modelMIDI_Mapper);
    p->addModel(modelMIDI_Input);
    p->addModel(modelMIDI_Output);
    p->addModel(modelMIDI_CV);
    p->addModel(modelMIDI_Channel);
    p->addModel(modelMIDI_Clock);
    p->addModel(modelMIDI_CC_Note);
    p->addModel(modelMulti_Meter);
}
