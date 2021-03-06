/*
 * Kilpatrick Audio MIDI Protocol Defines
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2021: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MIDI_PROTOCOL_H
#define MIDI_PROTOCOL_H

//
// MIDI event types
//
// - this is related to midi_file.c
//
#define MIDI_EVENT_CHANNEL 0x80  // an event containing MIDI channel data
#define MIDI_EVENT_META 0xff  // a MIDI file specified meta-event
#define MIDI_EVENT_SYSEX 0xf0  // a SYSEX event
#define MIDI_EVENT_ESCAPE 0xf7  // SYSEX continue or other arbitrary MIDI data
// meta event types
#define MIDI_META_EVENT_SEQ_TRACK_NUM 0x00  // pattern number (type 2 files)
#define MIDI_META_EVENT_TEXT 0x01  // general track text
#define MIDI_META_EVENT_COPYRIGHT 0x02  // copyright notice
#define MIDI_META_EVENT_SEQ_TRACK_NAME 0x03  // sequence / track name
#define MIDI_META_EVENT_INSTRUMENT_NAME 0x04  // instrument name
#define MIDI_META_EVENT_LYRIC 0x05  // a lyric
#define MIDI_META_EVENT_MARKER 0x06  // marker
#define MIDI_META_EVENT_CUE_POINT 0x07  // cue point
#define MIDI_META_EVENT_CHANNEL_PREFIX 0x20  // channel change
#define MIDI_META_EVENT_PORT_PREFIX 0x21  // port change
#define MIDI_META_EVENT_END_OF_TRACK 0x2f  // end of track
#define MIDI_META_EVENT_SET_TEMPO 0x51  // set tempo
#define MIDI_META_EVENT_SMPTE_OFFSET 0x54  // SMTPE offset for track
#define MIDI_META_EVENT_TIME_SIG 0x58  // time signature
#define MIDI_META_EVENT_KEY_SIG 0x59  // key signature
#define MIDI_META_EVENT_SEQ_SPECIFIC 0x7f  // sequencer specific (MMA required)
// ranges for meta events
#define MIDI_META_EVENT_SMPTE_MIN_HOURS 0  // min hours
#define MIDI_META_EVENT_SMPTE_MAX_HOURS 23  // max hours
#define MIDI_META_EVENT_SMPTE_MIN_MINS 0  // min minutes
#define MIDI_META_EVENT_SMPTE_MAX_MINS 59  // max minutes
#define MIDI_META_EVENT_SMPTE_MIN_SECS 0  // min seconds
#define MIDI_META_EVENT_SMPTE_MAX_SECS 59  // max seconds
#define MIDI_META_EVENT_SMPTE_MIN_FRAMES 0  // min frames
#define MIDI_META_EVENT_SMPTE_MAX_FRAMES 30  // max frames
#define MIDI_META_EVENT_SMPTE_MIN_SUBFRAMES 0  // min subframes
#define MIDI_META_EVENT_SMPTE_MAX_SUBFRAMES 99  // max subframes
#define MIDI_META_EVENT_TIME_SIG_MIN_NUMERATOR 1  // min numerator
#define MIDI_META_EVENT_TIME_SIG_MAX_NUMERATOR 255  // max numerator
#define MIDI_META_EVENT_TIME_SIG_MIN_DENOMINATOR 1  // min denominator
#define MIDI_META_EVENT_TIME_SIG_MAX_DENOMINATOR 255  // max denominator
#define MIDI_META_EVENT_TIME_SIG_MIN_TICKS_CLICK 1  // min ticks/click
#define MIDI_META_EVENT_TIME_SIG_MAX_TICKS_CLICK 255  // min ticks/click
#define MIDI_META_EVENT_TIME_SIG_MIN_32NDS_QUARTER 1  // min 32nds per quarter XXX
#define MIDI_META_EVENT_TIME_SIG_MAX_32NDS_QUARTER 32  // max 32ns per quarter XXX
#define MIDI_META_EVENT_KEY_SIG_MIN_SF -7  // min sf
#define MIDI_META_EVENT_KEY_SIG_MAX_SF 7  // max sf
#define MIDI_META_EVENT_KEY_SIG_MIN_MINOR 0  // min minor
#define MIDI_META_EVENT_KEY_SIG_MAX_MINOR 1  // min minor

// macros
#define MIDI_BEND_DATA_TO_INT(data0, data1) (((data1 << 7) | data0) - 8192)
#define MIDI_BEND_INT_TO_DATA0(bend) ((bend + 8192) & 0x7f)
#define MIDI_BEND_INT_TO_DATA1(bend) (((bend + 8192) >> 7) & 0x7f)

// sizes
#define MIDI_MAX_RAW_LEN 1000
#define MIDI_NUM_CHANNELS 16
#define MIDI_NUM_NOTES 128
#define MIDI_NUM_BANKS 16384
#define MIDI_NUM_PROGRAMS 128
#define MIDI_NUM_CONTROLLERS 128
#define MIDI_NATIVE_PPQ 24  // MIDI clock wire rate

// status bytes
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90
#define MIDI_POLY_KEY_PRESSURE 0xa0
#define MIDI_CONTROL_CHANGE 0xb0
#define MIDI_PROGRAM_CHANGE 0xc0
#define MIDI_CHANNEL_PRESSURE 0xd0
#define MIDI_PITCH_BEND 0xe0
#define MIDI_SYSEX_START 0xf0
#define MIDI_MTC_QFRAME 0xf1
#define MIDI_SONG_POSITION 0xf2
#define MIDI_SONG_SELECT 0xf3
#define MIDI_TUNE_REQUEST 0xf6
#define MIDI_SYSEX_END 0xf7
#define MIDI_TIMING_TICK 0xf8
#define MIDI_CLOCK_START 0xfa
#define MIDI_CLOCK_CONTINUE 0xfb
#define MIDI_CLOCK_STOP 0xfc
#define MIDI_ACTIVE_SENSING 0xfe
#define MIDI_SYSTEM_RESET 0xff

// controller changes
#define MIDI_CONTROLLER_BANK_MSB 0
#define MIDI_CONTROLLER_MOD_WHEEL 1
#define MIDI_CONTROLLER_BREATH 2
#define MIDI_CONTROLLER_FOOT 4
#define MIDI_CONTROLLER_PORTAMENTO_TIME 5
#define MIDI_CONTROLLER_DATA_ENTRY_MSB 6
#define MIDI_CONTROLLER_VOLUME 7
#define MIDI_CONTROLLER_PAN 10
#define MIDI_CONTROLLER_EXPRESSION 11
#define MIDI_CONTROLLER_EFFECT_CTRL1 12
#define MIDI_CONTROLLER_EFFECT_CTRL2 13
#define MIDI_CONTROLLER_GP_CTRL1 16
#define MIDI_CONTROLLER_GP_CTRL2 17
#define MIDI_CONTROLLER_GP_CTRL3 18
#define MIDI_CONTROLLER_GP_CTRL4 19
#define MIDI_CONTROLLER_BANK_LSB 32
#define MIDI_CONTROLLER_DATA_ENTRY_LSB 38
#define MIDI_CONTROLLER_DAMPER_PEDAL 64
#define MIDI_CONTROLLER_PORTAMENTO_ONOFF 65
#define MIDI_CONTROLLER_SOSTENUTO 66
#define MIDI_CONTROLLER_SOFT_PEDAL 67
#define MIDI_CONTROLLER_LEGATO_FOOTSWITCH 68
#define MIDI_CONTROLLER_HOLD2 69
#define MIDI_CONTROLLER_SOUND_CTRL1 70
#define MIDI_CONTROLLER_SOUND_CTRL2 71
#define MIDI_CONTROLLER_SOUND_CTRL3 72
#define MIDI_CONTROLLER_SOUND_CTRL4 73
#define MIDI_CONTROLLER_SOUND_CTRL5 74
#define MIDI_CONTROLLER_SOUND_CTRL6 75
#define MIDI_CONTROLLER_SOUND_CTRL7 76
#define MIDI_CONTROLLER_SOUND_CTRL8 77
#define MIDI_CONTROLLER_SOUND_CTRL9 78
#define MIDI_CONTROLLER_SOUND_CTRL10 79
#define MIDI_CONTROLLER_GP_CTRL5 80
#define MIDI_CONTROLLER_GP_CTRL6 81
#define MIDI_CONTROLLER_GP_CTRL7 82
#define MIDI_CONTROLLER_GP_CTRL8 83
#define MIDI_CONTROLLER_PORTAMENTO_CTRL 84
#define MIDI_CONTROLLER_EFFECT_DEPTH1 91
#define MIDI_CONTROLLER_EFFECT_DEPTH2 92
#define MIDI_CONTROLLER_EFFECT_DEPTH3 93
#define MIDI_CONTROLLER_EFFECT_DEPTH4 94
#define MIDI_CONTROLLER_EFFECT_DEPTH5 95
#define MIDI_CONTROLLER_DATA_INCREMENT 96
#define MIDI_CONTROLLER_DATA_DECREMENT 97
#define MIDI_CONTROLLER_NRPN_PARAM_NUM_LSB 98
#define MIDI_CONTROLLER_NRPN_PARAM_NUM_MSB 99
#define MIDI_CONTROLLER_RPN_PARAM_NUM_LSB 100
#define MIDI_CONTROLLER_RPN_PARAM_NUM_MSB 101
#define MIDI_CONTROLLER_ALL_SOUNDS_OFF 120
#define MIDI_CONTROLLER_RESET_ALL_CONTROLLERS 121
#define MIDI_CONTROLLER_LOCAL_CONTROL 122
#define MIDI_CONTROLLER_ALL_NOTES_OFF 123
#define MIDI_CONTROLLER_OMNI_OFF 124
#define MIDI_CONTROLLER_OMNI_ON 125
#define MIDI_CONTROLLER_MONO_ON 126
#define MIDI_CONTROLLER_POLY_ON 127
#define MIDI_CONTROLLER_CHANNEL_MODE_BASE (MIDI_CONTROLLER_ALL_NOTES_OFF)

#endif
