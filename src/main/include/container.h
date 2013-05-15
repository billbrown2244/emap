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

class EmapContainer: public Gtk::Window {
public:
	EmapContainer(fluid_synth_t* synth);
	~EmapContainer();



protected:

	class ModelColumns: public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(name);
			add(path);
			add(bank);
			add(program);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> path;
		Gtk::TreeModelColumn<int> bank;
		Gtk::TreeModelColumn<int> program;
	};

	Gtk::Table *container, *path_container;
	Gtk::ScrolledWindow *scrolled;
	Gtk::Label *path_label;
	Gtk::Entry *path_entry;
	Gtk::Button *path_enumerate;
	Gtk::TreeView *treeview;
	Glib::RefPtr<Gtk::TreeStore> model;
	ModelColumns columns;
	Gtk::Button *set_root_folder_button, *quit_button;
	std::string root_folder, home_dir, config_file;
	fluid_synth_t* synth; //the fluid synth instance
	fluid_sfont_t* soundfont; //the loaded soundfont
	std::map<Glib::ustring,int> presets; //map for holding build out presets.

	//Signal handlers:
	void on_button_clicked();
	void on_button_quit();
	bool on_key_press_or_release_event(GdkEventKey* event);
	void on_selection_changed();
	bool is_soundfont(const char * filename);
	void loadTree(const char*, const char* root_folder,
			const Gtk::TreeModel::Row row);
	void set_root_folder(const char* root_folder);





};

#endif /* CONTAINER_H_ */
