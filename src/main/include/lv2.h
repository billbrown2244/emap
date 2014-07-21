#ifndef LV2_H_
#define LV2_H_

#include "lv2-c++-tools/lv2.h"
#include "/usr/lib/lv2/atom.lv2/atom.h"
#include "/usr/lib/lv2/atom.lv2/util.h"
#include "/usr/lib/lv2/atom.lv2/forge.h"
#include "/usr/lib/lv2/midi.lv2/midi.h"
#include "/usr/lib/lv2/urid.lv2/urid.h"
#include <lv2-c++-tools/lv2gui.hpp>

#define EMAP_URI "http://www.colorfulsoftware.com/emap/lv2"
#define EMAP_UI_URI "http://www.colorfulsoftware.com/emap/lv2ui"
//#define LV2_MIDI__MidiEvent "http://lv2plug.in/ns/ext/midi#MidiEvent"

// MIDI opcodes
#define NOTE_OFF                0x80
#define NOTE_ON                 0x90
#define KEY_PRESSURE            0xA0
#define CONTROL_CHANGE          0xB0
#define PROGRAM_CHANGE          0xC0
#define CHANNEL_PRESSURE        0xD0
#define PITCH_BEND              0xE0
#define MIDI_SYSTEM_RESET       0xFF


#endif
