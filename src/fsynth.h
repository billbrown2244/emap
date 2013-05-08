/*
 * FSynth.h
 *
 *  Created on: Apr 28, 2013
 *      Author: bill
 */

#ifndef FLUIDSYNTH_H_
#define FLUIDSYNTH_H_

#include <fluidsynth.h>

class FSynth {

public:

	FSynth();
	~FSynth();

	fluid_synth_t* get_synth();

protected:

	//set up the midi driver
	fluid_settings_t* settings;

	fluid_midi_driver_t* mdriver;

	fluid_synth_t* synth;

	fluid_audio_driver_t* adriver;

};

int handle_midi_event(void* data, fluid_midi_event_t* event);

#endif /* FLUIDSYNTH_H_ */
