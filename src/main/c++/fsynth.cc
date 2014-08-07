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

fluid_settings_t* FSynth::get_settings() {
	return settings;
}

fluid_sfont_t* FSynth::get_soundfont() {
	return soundfont;
}

void FSynth::set_soundfont(fluid_sfont_t* soundfont) {
	this->soundfont = soundfont;
}

//lv2 stuff
typedef struct {
	FSynth* synth;

	// Port buffers
	const LV2_Atom_Sequence* control;
	const LV2_Atom_Sequence* notify;
	float* left;
	float* right;

	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;

	// Features
	LV2_URID_Map* map;
	EMAPUris uris;

	fluid_synth_t* fsynth;

} Synth;

typedef enum {
	AUDIO_OUT_LEFT = 0, AUDIO_OUT_RIGHT = 1, MIDI_IN = 2, UI_NOTIFY = 3
} PortIndex;

static LV2_Handle instantiate(const LV2_Descriptor* descriptor, double rate,
		const char* bundle_path, const LV2_Feature* const * features) {
	std::cout << "instantiate EMAP lv2" << std::endl;

	Synth* synth = (Synth*) malloc(sizeof(Synth));

	if (!synth) {
		std::cout << "EMAP: out of memory" << std::endl;
		return NULL;
	}

	synth->map = NULL;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			synth->map = (LV2_URID_Map*) features[i]->data;
			break;
		}
	}
	if (!synth->map) {
		return NULL;
	}

	synth->synth = new FSynth(true, rate);

	map_emap_uris(synth->map, &synth->uris);

	std::cout << "instantiated EMAP lv2" << std::endl;

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
		//std::cout << "connect_port EMAP lv2" << std::endl;
		break;
	case UI_NOTIFY:
		synth->notify = (const LV2_Atom_Sequence*) data;
		break;
	}

}

static void activate(LV2_Handle instance) {
	std::cout << "activate EMAP lv2" << std::endl;
	//Synth* synth = (Synth*) instance;

}

static void run(LV2_Handle instance, uint32_t n_samples) {

	Synth* synth = (Synth*) instance;

	fluid_synth_t* fsynth = synth->synth->get_synth();

	// setup message bus
	uint8_t obj_buf[1024];
	lv2_atom_forge_set_buffer(&synth->forge, obj_buf, sizeof(obj_buf));
	lv2_atom_forge_sequence_head(&synth->forge, &synth->frame, 0);

	LV2_ATOM_SEQUENCE_FOREACH(synth->control, ev)
	{
		//std::cout << "ev->body.type: " << ev->body.type << std::endl;
		///std::cout << "synth->uris.ui_State: " << synth->uris.ui_State
		//		<< std::endl;
		//std::cout << "synth->uris.midi_MidiEvent: "
		//		<< synth->uris.midi_MidiEvent << std::endl;

		//read from the controller.
		if (ev->body.type == synth->uris.midi_MidiEvent) {
			//uint8_t* msg = (uint8_t*) (ev + 1);
			const uint8_t* msg = (uint8_t*) LV2_ATOM_BODY_CONST(&ev->body);

			int channel = (msg[0] & 0x0F);
			int key = msg[1];
			int velocity = msg[2];
			int control = msg[1];
			int control_value = msg[2];
			int program = msg[1];
			int pressure_val = msg[1];
			int pitch = msg[1] + (msg[2] << 7);

			//std::cout << "channel: " << (msg[0] & 0x0F) << std::endl;

			switch (lv2_midi_message_type(msg)) {
			case LV2_MIDI_MSG_NOTE_ON: {
				std::cout << "LV2_MIDI_MSG_NOTE_ON: " << std::endl;
				std::cout << "key: " << key << std::endl;
				std::cout << "velocity: " << velocity << std::endl;
				fluid_synth_noteon(fsynth, channel, key, velocity);
				break;
			}
			case LV2_MIDI_MSG_NOTE_OFF: {
				std::cout << "LV2_MIDI_MSG_NOTE_OFF: " << std::endl;
				std::cout << "key: " << key << std::endl;
				fluid_synth_noteoff(fsynth, channel, key);
				break;
			}
			case LV2_MIDI_MSG_NOTE_PRESSURE: {
				std::cout << "LV2_MIDI_MSG_NOTE_PRESSURE: " << std::endl;
				break;
			}
			case LV2_MIDI_MSG_CONTROLLER: {
				std::cout << "LV2_MIDI_MSG_CONTROLLER: " << std::endl;
				std::cout << "control: " << control << std::endl;
				std::cout << "control_value: " << control_value << std::endl;
				fluid_synth_cc(fsynth, channel, control, control_value);
				break;
			}
			case LV2_MIDI_MSG_PGM_CHANGE: {
				std::cout << "LV2_MIDI_MSG_PGM_CHANGE: " << std::endl;
				std::cout << "program: " << program << std::endl;
				fluid_synth_program_change(fsynth, channel, program);
				break;
			}
			case LV2_MIDI_MSG_CHANNEL_PRESSURE: {
				std::cout << "LV2_MIDI_MSG_CHANNEL_PRESSURE: " << std::endl;
				std::cout << "pressure_val: " << pressure_val << std::endl;
				fluid_synth_channel_pressure(fsynth, channel, pressure_val);
				break;
			}
			case LV2_MIDI_MSG_BENDER: {
				std::cout << "LV2_MIDI_MSG_BENDER: " << std::endl;
				std::cout << "pitch: " << pitch << std::endl;
				fluid_synth_pitch_bend(fsynth, channel, pitch);
				break;
			}
			default: {
				//std::cout << "no midi event: " << std::endl;
				break;
			}
			}
		} else if (ev->body.type == synth->uris.atom_Blank) {
			// get the object representing the rest of the data
			const LV2_Atom_Object* obj = (LV2_Atom_Object*) &ev->body;

			if (obj->body.otype == synth->uris.ui_State) {

				const LV2_Atom* name = NULL;
				const LV2_Atom* path = NULL;
				const LV2_Atom* bank = NULL;
				const LV2_Atom* program = NULL;
				lv2_atom_object_get(obj, synth->uris.ui_name, &name,
						synth->uris.ui_path, &path, synth->uris.ui_bank, &bank,
						synth->uris.ui_program, &program, 0);
				char* namestr = (char*) LV2_ATOM_BODY(name);
				char* pathstr = (char*) LV2_ATOM_BODY(path);
				int* sbank = (int*) LV2_ATOM_BODY(bank);
				int* sprog = (int*) LV2_ATOM_BODY(program);
				int s = *sbank;
				int p = *sprog;

				std::cout << "load name: " << namestr << std::endl;
				std::cout << "load path: " << pathstr << std::endl;
				std::cout << "load bank: " << s << std::endl;
				std::cout << "load program: " << p << std::endl;

				fluid_synth_sfload(fsynth, pathstr, 1);
				synth->synth->set_soundfont(fluid_synth_get_sfont(fsynth, 0));
				fluid_synth_bank_select(fsynth, 0, s);
				fluid_synth_program_change(fsynth, 0, p);
				fluid_synth_program_reset(fsynth);

			} else {
				//	std::cout << "unknown otype: " << std::endl;
			}

		}
	}

	fluid_synth_write_float(fsynth, n_samples, synth->left, 0, 1, synth->right,
			0, 1);

// Close off sequence
	lv2_atom_forge_pop(&synth->forge, &synth->frame);
}

static void deactivate(LV2_Handle instance) {
	std::cout << "deactivate EMAP lv2" << std::endl;
	Synth* synth = (Synth*) instance;
	delete_fluid_synth(synth->synth->get_synth());
	delete_fluid_settings(synth->synth->get_settings());
}

static void cleanup(LV2_Handle instance) {
	std::cout << "cleanup EMAP lv2" << std::endl;
	free(instance);
}

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
