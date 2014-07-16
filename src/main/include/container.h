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
#include "lv2.h"

class EmapContainer: public Gtk::Window {
public:
	EmapContainer(fluid_synth_t* synth, bool is_lv2);
	~EmapContainer();

	// public lv2 stuff
	LV2UI_Controller controller;

	LV2UI_Write_Function write_function;

	LV2UI_Handle *handle;

	static LV2UI_Descriptor descriptors[];

	GtkWindow *emap;

	//Gtk::Widget *container, *path_container, *scrolled;
	//, *set_root_folder_button,*quit_button, *expand_all_button, *collapse_all_button;//, *treeview;

	GtkTable * container;
	Gtk::Table* path_container;

	Gtk::ScrolledWindow *scrolled;

	GtkTreeView *treeview;

	Gtk::Button *set_root_folder_button, *quit_button, *expand_all_button, *collapse_all_button;

	Glib::RefPtr<Gtk::TreeStore> model;
	GtkTreeStore* modelc;

	//Gtk::Button *set_root_folder_button, *quit_button, *expand_all_button,
	//		*collapse_all_button;
	std::string root_folder, home_dir, config_file;
	fluid_synth_t* synth; //the fluid synth instance
	fluid_sfont_t* soundfont; //the loaded soundfont
	std::map<Glib::ustring, int> presets; //map for holding build out presets.

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
		NAME, PATH, BANK, PROGRAM
	};

	//Signal handlers:
	void on_button_clicked(EmapContainer* emap);
	void on_button_clickedLv2(EmapContainer* emap);
	void on_button_quit();
	void on_button_expand(GtkTreeView* treeview);
	void on_button_collapse(GtkTreeView* treeview);
	bool on_key_press_or_release_event(GdkEventKey* event);
	bool on_key_press_or_release_event2(GtkTreeView *tree_view,
	            gpointer     user_data);
	void on_selection_changed(GtkTreeView* treeview);
	void on_selection_changedLv2(GtkWidget *widget, gpointer treeview);
	bool is_soundfont(const char * filename);
	void loadTree(const char* orig_root_folder, const char* root_folder,
			const Gtk::TreeModel::Row row);
	void loadTreeLv2(const char* orig_path, const char* path, GtkTreeIter *row,
			GtkTreeStore* model);
	void set_root_folder(const char* root_folder);
	void set_root_folderLv2(EmapContainer* emap);



protected:

	enum {
		TEXT_COLUMN
	};

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
	ModelColumns columns;

};

#endif /* CONTAINER_H_ */
