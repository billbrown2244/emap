#ifndef LV2_H_
#define LV2_H_

#include "/usr/lib/lv2/atom.lv2/atom.h"
#include "/usr/lib/lv2/atom.lv2/util.h"
#include "/usr/lib/lv2/atom.lv2/forge.h"
#include "/usr/lib/lv2/midi.lv2/midi.h"
#include "/usr/lib/lv2/urid.lv2/urid.h"
#include "/usr/lib/lv2/parameters.lv2/parameters.h"
#include "/usr/include/lv2-c++-tools/lv2.h"
#include "/usr/include/lv2-c++-tools/lv2gui.hpp"

#define EMAP_URI "http://www.colorfulsoftware.com/emap/lv2"
#define EMAP_UI_URI "http://www.colorfulsoftware.com/emap/lv2ui"

typedef struct {
	// URIs defined in LV2 specifications
	LV2_URID midi_MidiEvent;
	LV2_URID atom_Blank;
	LV2_URID atom_Vector;
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_eventTransfer;
	LV2_URID param_sampleRate;
	//custom to communicate back and forth the soundfont state.
	LV2_URID ui_State;
	LV2_URID ui_name;
	LV2_URID ui_path;
	LV2_URID ui_bank;
	LV2_URID ui_program;
	LV2_URID ui_Off;
	LV2_URID ui_Soundfont;

} EMAPUris;

static inline void
map_emap_uris(LV2_URID_Map* map, EMAPUris* uris)
{
	uris->atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Vector = map->map(map->handle, LV2_ATOM__Vector);
	uris->atom_Float = map->map(map->handle, LV2_ATOM__Float);
	uris->atom_Int = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->param_sampleRate = map->map(map->handle, LV2_PARAMETERS__sampleRate);
	uris->midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

	uris->ui_State  = map->map(map->handle, EMAP_URI "#UIState");
	uris->ui_name  = map->map(map->handle, EMAP_URI "#UIState");
	uris->ui_path  = map->map(map->handle, EMAP_URI "#UIState");
	uris->ui_bank  = map->map(map->handle, EMAP_URI "#UIState");
	uris->ui_program  = map->map(map->handle, EMAP_URI "#UIState");
}
#endif /* LV2_H_ */
