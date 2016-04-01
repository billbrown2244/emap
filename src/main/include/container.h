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
 * container.h
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include <gtkmm.h>
#include <fluidsynth.h>
#include <gdk/gdkkeysyms.h>
#include "uthash.h"

class EmapContainer: public Gtk::Window {
public:
	EmapContainer(fluid_synth_t* synth);
	~EmapContainer();

	GtkWindow *emap;
	int bank = -1, program = -1;

	//full
	Gtk::Grid* path_container;
	Gtk::ScrolledWindow *scrolled;
	Gtk::TreeView *treeview;

	Gtk::Button *set_root_folder_button, *quit_button, *expand_all_button,
			*collapse_all_button;

	Glib::RefPtr<Gtk::TreeStore> model;
	GtkTreeStore* modelc;

	std::string rootdir, home_dir, config_file, path, label, preset;
	fluid_synth_t* synth; //the fluid synth instance
	fluid_sfont_t* soundfont; //the loaded soundfont
	std::map<const Glib::ustring, int> presets; //map for holding build out presets.

	struct cpresets {
	public:
		char* soundfont_key;
		const char* exists;
		UT_hash_handle hh;
	};

	//struct cpresets* cpresets;
	cpresets* m_cpresets;

	struct sortstruct {
		// sortstruct needs to know its containing object
		EmapContainer* m;
		sortstruct(EmapContainer* p) :
				m(p) {
		}
		;

		// this is our sort function, which makes use
		// of some non-static data (sortascending)
		bool operator()(const Glib::RefPtr<Gio::FileInfo>& A,
				const Glib::RefPtr<Gio::FileInfo> & B) {
			return A->get_name() < B->get_name();
		}
	};

	enum {
		ROOTDIR, PATH, LABEL, PRESET, BANK, PROGRAM
	};

	//Signal handlers:
	void on_button_rootdir(EmapContainer* emap);
	void on_button_quit();
	void on_button_expand(GtkTreeView* treeview);
	void on_button_collapse(GtkTreeView* treeview);
	bool on_key_press_or_release_event(GdkEventKey* event);
	bool on_key_press_or_release_event2(GtkTreeView *tree_view,
			gpointer user_data);
	void on_selection_changed(Gtk::TreeView* treeview);
	bool is_soundfont(const char * filename);
	void loadTree(std::string orig_rootdir, std::string rootdir,
			const Gtk::TreeModel::Row row);
	void set_root_folder(std::string rootdir);
	void send_ui_state(EmapContainer* emap);
	void send_ui_disable(EmapContainer* emap);
	void save_state(std::string rootdir, std::string path, std::string label,
			std::string preset, int bank, int program);

protected:

	enum {
		TEXT_COLUMN
	};

	class ModelColumns: public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(rootdir);
			add(path);
			add(label); //Label in UI
			add(preset);
			add(bank);
			add(program);
		}

		Gtk::TreeModelColumn<Glib::ustring> rootdir;
		Gtk::TreeModelColumn<Glib::ustring> path;
		Gtk::TreeModelColumn<Glib::ustring> label;
		Gtk::TreeModelColumn<Glib::ustring> preset;
		Gtk::TreeModelColumn<int> bank;
		Gtk::TreeModelColumn<int> program;
	};
	ModelColumns columns;

};

#endif /* CONTAINER_H_ */
