/*
 *  Copyright 2013 Bill Brown
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * FSynth.cc
 *
 *  Created on: Apr 28, 2013
 *      Author: bill
 */

#include <fluidsynth.h>
#include <stdio.h>
#include <iostream>
#include <exception>
#include <types.h>
#include <stdlib.h>

#include "fsynth.h"
#include "atom.h"
#include "util.h"
#include "midi.h"
#include "urid.h"

FSynth::FSynth(bool is_lv2, double sample_rate) {

	std::cout << "making EMAP synth (fluidsynth wrapper)" << std::endl;

	settings = new_fluid_settings();

	fluid_settings_setstr(settings, "audio.jack.id", "EMAP");

	fluid_settings_setstr(settings, "midi.jack.id", "EMAP");

	fluid_settings_setstr(settings, "midi.alsa_seq.id", "EMAP");

	if (is_lv2) {

		fluid_settings_setnum(settings, "synth.sample-rate", sample_rate);

	}

	synth = new_fluid_synth(settings);

	if (!is_lv2) {
		mdriver = new_fluid_midi_driver(settings, handle_midi_event,
				(void*) synth);

		std::cout << "made FSynth midi driver" << std::endl;

		adriver = new_fluid_audio_driver(settings, synth);

		std::cout << "made FSynth audio driver" << std::endl;
	}
}

FSynth::~FSynth() {

}

int handle_midi_event(void* data, fluid_midi_event_t* event) {

	//try {

	fluid_synth_t* synth = (fluid_synth_t*) data;

	int event_type = fluid_midi_event_get_type(event);

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
		fluid_synth_cc(synth, channel, control, value);
		break;
	case 0xc0: //PROGRAM_CHANGE
		//std::cout << "PROGRAM_CHANGE" << std::endl;
		fluid_synth_program_change(synth, channel, program);
		break;
	case 0xd0: //CHANNEL_PRESSURE
		//std::cout << "CHANNEL_PRESSURE" << std::endl;
		//fluid_synth_set_gain(synth, velocity);
		fluid_synth_channel_pressure(synth, channel, value);
		break;
	case 0xe0: //PITCH_BEND
		//std::cout << "PITCH_BEND" << std::endl;
		fluid_synth_pitch_bend(synth, channel, pitch);
		break;

	default:
		std::cout << "Undetected event" << std::endl;
		break;
	};

	return 0;
}

fluid_synth_t* FSynth::get_synth() {
	return synth;
}

//lv2 stuff
typedef struct {
	FSynth* synth;

	// Port buffers
	const LV2_Atom_Sequence* control;
	const LV2_Atom_Sequence* notify;
	float* left;
	float* right;

	// Features
	LV2_URID_Map* map;

	fluid_synth_t* fsynth;

	struct {
		LV2_URID midi_MidiEvent;
	} uris;

} Synth;

typedef enum {
	AUDIO_OUT_LEFT = 0, AUDIO_OUT_RIGHT = 1, MIDI_IN = 2, UI_NOTIFY = 3
} PortIndex;

static LV2_Handle instantiate(const LV2_Descriptor* descriptor, double rate,
		const char* bundle_path, const LV2_Feature* const * features) {
	std::cout << "instantiate EMAP lv2" << std::endl;

	LV2_URID_Map* map = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*) features[i]->data;
			break;
		}
	}
	if (!map) {
		return NULL;
	}

	Synth* synth = (Synth*) malloc(sizeof(Synth));
	synth->synth = new FSynth(true, rate);
	synth->map = map;
	synth->uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
	return (LV2_Handle) synth;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
//std::cout << "connect_port EMAP lv2" << std::endl;

	Synth* synth = (Synth*) instance;

	switch ((PortIndex) port) {
	case AUDIO_OUT_LEFT:
		synth->left = (float*) data;
		break;
	case AUDIO_OUT_RIGHT:
		synth->right = (float*) data;
		break;
	case MIDI_IN:
		synth->control = (const LV2_Atom_Sequence*) data;
		std::cout << "connect_port EMAP lv2" << std::endl;
		break;
	case UI_NOTIFY:
		synth->notify = (const LV2_Atom_Sequence*) data;
		break;
	}

}

static void activate(LV2_Handle instance) {
	std::cout << "activate EMAP lv2" << std::endl;
	Synth* synth = (Synth*) instance;

}

static void run(LV2_Handle instance, uint32_t n_samples) {

	Synth* synth = (Synth*) instance;

	fluid_synth_t* fsynth = synth->synth->get_synth();

	LV2_ATOM_SEQUENCE_FOREACH(synth->control, ev)
	{
		//std::cout << "cv->body.type: " << ev->body.type << std::endl;
		//std::cout << "synth->uris.midi_MidiEvent: "
		//		<< synth->uris.midi_MidiEvent << std::endl;
		if (ev->body.type == synth->uris.midi_MidiEvent) {
			uint8_t* msg = (uint8_t*) (ev + 1);

			std::cout << "msg: " << msg << std::endl;

			//std::cout << "fsynth: " << fsynth << std::endl;

			int channel = (msg[0] & 0x0F);
			int key = msg[1];
			int velocity = msg[2];
			int control = msg[1];
			int value = msg[2];
			int program = msg[1];
			int pressure_val = msg[1];
			int pitch = msg[1] + (msg[2] << 7);

			std::cout << "channel: " << (msg[0] & 0x0F) << std::endl;
			//std::cout << "key: " << key << std::endl;
			//std::cout << "velocity: " << velocity << std::endl;
			//std::cout << "control: " << control << std::endl;
			//std::cout << "value: " << value << std::endl;
			//std::cout << "program: " << program << std::endl;
			//std::cout << "pressure_val: " << pressure_val << std::endl;
			//std::cout << "pitch: " << pitch << std::endl;

			switch ((msg[0] & 0x0F)) {//lv2_midi_message_type(msg)) {		//event_type) {		//
			case NOTE_ON: {
				std::cout << "NOTE_ON: " << std::endl;
				fluid_synth_noteon(fsynth, channel, key, velocity);
				break;
			}
			case NOTE_OFF: {
				std::cout << "NOTE_OFF: " << std::endl;
				fluid_synth_noteoff(fsynth, channel, key);
				break;
			}
			case KEY_PRESSURE: {
				std::cout << "KEY_PRESSURE: " << std::endl;
				break;
			}
			case CONTROL_CHANGE: {
				std::cout << "CONTROL_CHANGE: " << std::endl;
				fluid_synth_cc(fsynth, channel, control, value);
				break;
			}
			case PROGRAM_CHANGE: {
				std::cout << "PROGRAM_CHANGE: " << std::endl;
				fluid_synth_program_change(fsynth, channel, program);
				break;
			}
			case CHANNEL_PRESSURE: {
				std::cout << "CHANNEL_PRESSURE: " << std::endl;
				fluid_synth_channel_pressure(fsynth, channel, pressure_val);
				break;
			}
			case PITCH_BEND: {
				std::cout << "PITCH_BEND: " << std::endl;
				fluid_synth_pitch_bend(fsynth, channel, pitch);
				break;
			}
			default: {
				std::cout << "no event found: " << std::endl;
				break;
			}
			}
		}

		fluid_synth_write_float(fsynth, n_samples, synth->left, 0, 1,
				synth->right, 0, 1);
	}
}

static void deactivate(LV2_Handle instance) {
	std::cout << "deactivate EMAP lv2" << std::endl;
}

static void cleanup(LV2_Handle instance) {
	std::cout << "cleanup EMAP lv2" << std::endl;
	free(instance);
}

//const void* FSynth::extension_data(const char* uri) {
//  return NULL;
//}

static LV2_Descriptor descriptors[] = { { EMAP_URI, instantiate, connect_port,
		activate, run, deactivate, cleanup } };		//, extension_data } };

const LV2_Descriptor*
lv2_descriptor(uint32_t index) {
	switch (index) {
	case 0:
		return &descriptors[index];
	default:
		return NULL;
	}
}
