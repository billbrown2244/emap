/*
 * FSynth.cc
 *
 *  Created on: Apr 28, 2013
 *      Author: bill
 */
#include "fsynth.h"
#include <fluidsynth.h>
#include <stdio.h>
#include <iostream>
#include <exception>
#include <types.h>
#include <stdlib.h>


FSynth::FSynth() {

	settings = new_fluid_settings();

	fluid_settings_setstr(settings,"audio.jack.id","EMAP");

	fluid_settings_setstr(settings,"midi.jack.id","EMAP");

	fluid_settings_setstr(settings,"midi.alsa_seq.id","EMAP");

	synth = new_fluid_synth(settings);

	mdriver = new_fluid_midi_driver(settings, handle_midi_event, (void*) synth);

	adriver = new_fluid_audio_driver(settings, synth);

}

FSynth::~FSynth() {

}

int handle_midi_event(void* data, fluid_midi_event_t* event) {

	try {

		fluid_synth_t* synth = (fluid_synth_t*) data;

		int event_type = fluid_midi_event_get_type(event);

		//std::cout << "event type int: " << event_type << std::endl;

		//std::cout << "event type hex: " << std::hex << event_type << std::endl;

		//std::cout << "handle_midi_event synth: " << synth << std::endl;

		//std::cout << "event type: " << fluid_midi_event_get_type(event)
		//		<< std::endl;

		/*
		 * enum fluid_midi_event_type {
		 // channel messages

		 KEY_PRESSURE = 0xa0,
		 CONTROL_CHANGE = 0xb0,
		 PROGRAM_CHANGE = 0xc0,
		 CHANNEL_PRESSURE = 0xd0,
		 PITCH_BEND = 0xe0,
		 // system exclusive
		 MIDI_SYSEX = 0xf0,
		 // system common - never in midi files
		 MIDI_TIME_CODE = 0xf1,
		 MIDI_SONG_POSITION = 0xf2,
		 MIDI_SONG_SELECT = 0xf3,
		 MIDI_TUNE_REQUEST = 0xf6,
		 MIDI_EOX = 0xf7,
		 // system real-time - never in midi files
		 MIDI_SYNC = 0xf8,
		 MIDI_TICK = 0xf9,
		 MIDI_START = 0xfa,
		 MIDI_CONTINUE = 0xfb,
		 MIDI_STOP = 0xfc,
		 MIDI_ACTIVE_SENSING = 0xfe,
		 MIDI_SYSTEM_RESET = 0xff,
		 // meta event - for midi files only
		 MIDI_META_EVENT = 0xff
		 };
		 */

		//int type = fluid_midi_event_get_type(event);
		int channel = fluid_midi_event_get_channel(event);
		int key = fluid_midi_event_get_key(event);
		int velocity = fluid_midi_event_get_velocity(event);
		int control = fluid_midi_event_get_control(event);
		int value = fluid_midi_event_get_value(event);
		int program = fluid_midi_event_get_program(event);
		int pitch = fluid_midi_event_get_pitch(event);
		/* Play a note */

		switch (event_type) {
		case 0x80: //NOTE_OFF:
			//std::cout << "NOTE_OFF: " << std::endl;
			fluid_synth_noteoff(synth, channel, key);
			break;
		case 0x90: //NOTE_ON
			//std::cout << "NOTE_ON: " << std::endl;
			fluid_synth_noteon(synth, channel, key, velocity);
			break;
		case 0xa0: //KEY_PRESSURE
			//std::cout << "KEY_PRESSURE / Aftertouch" << std::endl;
			break;
		case 0xb0: //CONTROL_CHANGE
			//std::cout << "CONTROL_CHANGE" << std::endl;
			fluid_synth_cc(synth,channel,control,value);
			break;
		case 0xc0: //PROGRAM_CHANGE
			//std::cout << "PROGRAM_CHANGE" << std::endl;
			fluid_synth_program_change(synth,channel,program);
			break;
		case 0xd0: //CHANNEL_PRESSURE
			//std::cout << "CHANNEL_PRESSURE" << std::endl;
			//fluid_synth_set_gain(synth, velocity);
			fluid_synth_channel_pressure(synth,channel,value);
			break;
		case 0xe0: //PITCH_BEND
			//std::cout << "PITCH_BEND" << std::endl;
			fluid_synth_pitch_bend(synth,channel,pitch);
			break;

		default:
			std::cout << "Undetected event" << std::endl;
			break;
		};
		//fluid_synth_noteon((fluid_synth_t*)data, , 60, 100);
		// return fluid_synth_handle_midi_event(data, event);

	} catch (std::exception& e) {
		std::cout << "Standard exception: " << e.what() << std::endl;
	}

	return 0;
}

fluid_synth_t* FSynth::get_synth() {
	return synth;
}

