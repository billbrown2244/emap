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
 * container.cc
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#include "container.h"
#include "fsynth.h"
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/grid.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <giomm/file.h>

#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <fluidsynth.h>
#include <misc.h>
#include <iostream>
#include "lv2.h"
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

EmapContainer::EmapContainer(fluid_synth_t* synth_new) {

		emap = (new Gtk::Window())->gobj();

		std::cout << "made top emap window:" << std::endl;

		set_title("EMAP - Easy Midi Audio Production");

		std::cout << "set title:" << std::endl;

		synth = synth_new;

		scrolled = new Gtk::ScrolledWindow();
		path_container = new Gtk::Grid();

		std::cout << "made container and path_container:" << std::endl;

		set_root_folder_button = new Gtk::Button("Set Root Folder");
		quit_button = new Gtk::Button("Quit");
		expand_all_button = new Gtk::Button("Expand All");
		collapse_all_button = new Gtk::Button("Collapse All");

		std::cout << "buttons:" << std::endl;

		//setup the TreeView
		model = Gtk::TreeStore::create(columns);
		treeview = new Gtk::TreeView(model);//GTK_TREE_VIEW(gtk_tree_view_new())
		std::cout << "created model:" << std::endl;

		treeview->set_model(model);

		treeview->append_column("", columns.name);

		treeview->signal_key_press_event().connect(
				sigc::mem_fun(this,
						&EmapContainer::on_key_press_or_release_event));
		treeview->signal_key_release_event().connect(
				sigc::mem_fun(this,
						&EmapContainer::on_key_press_or_release_event));
		treeview->add_events(
				Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);

		std::cout << "connected tree signals:" << std::endl;

		//connect the signals
		set_root_folder_button->signal_clicked().connect(
				sigc::bind<EmapContainer*>(
						sigc::mem_fun(*this, &EmapContainer::on_button_clicked),
						this));

		quit_button->signal_clicked().connect(
				sigc::mem_fun(*this, &EmapContainer::on_button_quit), this);

		expand_all_button->signal_clicked().connect(
				sigc::bind<GtkTreeView*>(
						sigc::mem_fun(*this, &EmapContainer::on_button_expand),
						treeview->gobj()));

		collapse_all_button->signal_clicked().connect(
				sigc::bind<GtkTreeView*>(
						sigc::mem_fun(*this,
								&EmapContainer::on_button_collapse), treeview->gobj()));

		//Connect signal:signal_row_activated
		Glib::RefPtr < Gtk::TreeSelection > refTreeSelection =
				treeview->get_selection();

		refTreeSelection->signal_changed().connect(
				sigc::bind<Gtk::TreeView*>(
						sigc::mem_fun(*this,
								&EmapContainer::on_selection_changed),
						treeview));

		//add the path_* widgets to the path_container


		path_container->attach(*set_root_folder_button, 0, 0, 1, 1);
		path_container->attach(*expand_all_button, 1, 0, 1, 1);
		path_container->attach(*collapse_all_button, 2, 0, 1, 1);
		path_container->attach(*quit_button, 3, 0, 1, 1);

		//Setup the ScrolledWindow and add the TreeView in it
		scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		scrolled->add(*(treeview));

		scrolled->set_min_content_width(400);
		scrolled->set_min_content_height(400);

		path_container->attach(*scrolled, 0, 1, 4, 1);

		//finally add the container to the main window and show all children
		add(*path_container);

		//set a default size
		//set_default_size(400, 400);
		//gtk_window_set_default_size(GTK_WINDOW(emap), 400, 400);

		//set up the root folder
		passwd *pw = getpwuid(getuid());
		home_dir = pw->pw_dir;
		root_folder = home_dir + "/emap";

		config_file = home_dir + "/.config/emap/emapconfig.json";
		std::cout << "root foler: '" << root_folder
			<< "'.  Change this in ~/.config/emap/emapconfig.json if you experience and issue."
			<< std::endl;

		JsonParser *parser = json_parser_new ();
		bool file_exists = json_parser_load_from_file (parser,
                            config_file.c_str(),
                            NULL);
    
		if(file_exists){
			JsonReader *reader = json_reader_new (json_parser_get_root (parser));
			json_reader_read_member (reader, "rootdir");
			root_folder = json_reader_get_string_value (reader);
			json_reader_end_member (reader);
			g_object_unref (reader);
			std::cout << "config file exists.  set root_folder to: " << root_folder
				<< std::endl;
		} else {
			std::cout
				<< "config file doesn't exist. create config file with default root folder: "
				<< root_folder << std::endl;
			set_root_folder(root_folder.c_str());
		}
		g_object_unref (parser);

		Gtk::TreeModel::Row row;

		//populate the tree
		std::cout << "tree root folder: " << root_folder << std::endl;

		loadTree(root_folder.c_str(), root_folder.c_str(), row);

		show_all_children();

		this->hide();

}

EmapContainer::~EmapContainer() {
	delete treeview;
	delete path_container;
	delete scrolled;
}

void EmapContainer::on_button_clicked(EmapContainer* emap) {
	std::cout << "Set Root Folder" << std::endl;
	Gtk::FileChooserDialog dialog("Please choose a folder",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dialog.set_transient_for((Gtk::Window&) (*this->get_toplevel()));

	std::cout << "root_folder before:  " << emap->root_folder << std::endl;

	dialog.set_current_folder(emap->root_folder);

	//Add response buttons the the dialog:
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result = dialog.run();

	//Handle the response:
	switch (result) {
	case (Gtk::RESPONSE_OK): {
		std::cout << "Select clicked." << std::endl;
		std::cout << "Folder selected: " << dialog.get_filename() << std::endl;

		model->clear();

		emap->root_folder = dialog.get_filename();

		set_root_folder(emap->root_folder.c_str());

		Gtk::TreeModel::Row row;	//pass an empty row.

		loadTree(emap->root_folder.c_str(), emap->root_folder.c_str(), row);

		break;
	}
	case (Gtk::RESPONSE_CANCEL): {
		std::cout << "Cancel clicked." << std::endl;
		break;
	}
	default: {
		std::cout << "Unexpected button clicked." << std::endl;
		break;
	}
	}
}



void EmapContainer::on_button_quit() {
	std::cout << "Quitting" << std::endl;
	exit(0);
}

void EmapContainer::on_button_expand(GtkTreeView* treeview) {
	std::cout << "expand all" << std::endl;
	gtk_tree_view_expand_all(treeview);
}

void EmapContainer::on_button_collapse(GtkTreeView* treeview) {
	std::cout << "collapse all" << std::endl;
	gtk_tree_view_collapse_all(treeview);
}

void EmapContainer::loadTree(const char* orig_path, const char* path,
		const Gtk::TreeModel::Row row) {
	//std::cout << "orig_path: " << path << std::endl;
	//std::cout << "path again: " << path << std::endl;

	//try {

	Glib::RefPtr < Gio::File > file = Gio::File::create_for_path(path);
	Glib::RefPtr < Gio::FileEnumerator > child_enumeration =
			file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);

	//sort the file name since they aren't sorted by default
	//child_enumeration = sortnames(child_enumeration);
	std::vector < Glib::ustring > file_names;
	std::vector < Glib::RefPtr<Gio::FileInfo> > sorted;
	Glib::RefPtr < Gio::FileInfo > file_info;
	while ((file_info = child_enumeration->next_file()) != 0) {
		sorted.push_back(file_info);
	}
	EmapContainer::sortstruct s(this);
	std::sort(sorted.begin(), sorted.end(), s);

	//std::cout << "sorted files." << std::endl;

	//loop throught the results and add them to the tree.
	for (std::vector<int>::size_type i = 0; i != sorted.size(); i++) {
		file_info = sorted[i];

		//get the file name from the file_info object.
		std::string fileName = file_info->get_name();

		//std::cout << "fileName: " << fileName << std::endl;

		if (file_info->get_file_type() == Gio::FILE_TYPE_DIRECTORY) {

			//std::cout << "found directory." << std::endl;

			//std::cout << "found directory: " << fileName << std::endl;
			//std::cout << "in loadTree" << std::endl;
			file_names.push_back(fileName);

			//std::cout << "in fileName: " << fileName << std::endl;

			//now populate the TreeView
			//Gtk::TreeModel::Row row = *(model->append());

			char result[strlen(path) + strlen(fileName.c_str())]; // array to hold the result.

			strcpy(result, path);
			strcat(result, "/");
			strcat(result, fileName.c_str());

			//std::cout << "result: " << result << std::endl;

			//dont display the first folder in the result;
			if (strcmp(orig_path, path) == 0) {
				//model.clear();
				//std::cout << "orig path, reset model: " << model
				//	<< std::endl;

				//std::cout << "paths are the same: " << std::endl;

				Gtk::TreeModel::Row rowb = *(model->append());
				//std::cout << "new row: " << rowb << std::endl;
				rowb[columns.name] = fileName;
				rowb[columns.path] = result;
				//std::cout << "result: " << result << std::endl;
				rowb[columns.bank] = -1;
				rowb[columns.program] = -1;
				//std::cout << "rowb: " << rowb << std::endl;
				loadTree(orig_path, result, rowb);
			} else {

				//std::cout << "paths are different: " << std::endl;

				Gtk::TreeModel::Row rowb = *(model->append(row.children()));
				rowb[columns.name] = fileName;
				rowb[columns.path] = result;
				//std::cout << "result: " << result << std::endl;
				rowb[columns.bank] = -1;
				rowb[columns.program] = -1;
				//std::cout << "rowb: " << rowb << std::endl;

				//std::cout << "not root path" << std::endl;

				loadTree(orig_path, result, rowb);
			}

		} else {
			char result[strlen(path) + strlen(fileName.c_str())]; // array to hold the result.
			strcpy(result, path);
			strcat(result, "/");
			strcat(result, fileName.c_str());

			file_names.push_back(fileName);

			if (is_soundfont(result)) {

				Gtk::TreeModel::Row rownew;
				if (strcmp(orig_path, path) == 0) {

					rownew = *(model->append());

				} else {

					rownew = *(model->append(row.children()));

				}
				rownew[columns.name] = fileName;
				rownew[columns.path] = result;
				//std::cout << "result: " << result << std::endl;
				rownew[columns.bank] = -1;
				rownew[columns.program] = -1;

			}
		}
	}
}

bool EmapContainer::is_soundfont(const char * filename) {
	//read these bytes to find
	/*
	 * Identifying characters
	 * Hex: 52 49 46 46 ,
	 * ASCII: RIFF , and ,
	 * Hex (position 6): 00 73 66 62 6B 4C 49 53 54 ,
	 * ASCII: .sfbkLIST
	 */

	/* Read field of n bytes */

	// there's no need for new;
	// in fact, new may lead to a memory leak if you forget to delete
	std::vector<char> buf(260);

	std::ifstream soundfont(filename);

	unsigned char riff[4] = { 'R', 'I', 'F', 'F' };
	unsigned char sfbk[4] = { 's', 'f', 'b', 'k' };
	unsigned char list[4] = { 'L', 'I', 'S', 'T' };
	char buff[4];

	soundfont.read(buff, 4);

	for (int i = 0; i < 4; i++) {
		if (buff[i] != riff[i]) {
			//std::cout << "not a soundfont riff" << std::endl;
			return false;
		}
	}

	//read ignored dword
	soundfont.read(buff, 4);

	soundfont.read(buff, 4);
	for (int i = 0; i < 4; i++) {
		if (buff[i] != sfbk[i]) {
			//std::cout << "not a soundfont sfbk" << std::endl;
			return false;
		}
	}

	soundfont.read(buff, 4);
	for (int i = 0; i < 4; i++) {
		if (buff[i] != list[i]) {
			//std::cout << "not a soundfont list" << std::endl;
			return false;
		}
	}

	return true;
}

void EmapContainer::set_root_folder(const char* root_folder) {

	std::cout << "set_root_folder" << std::endl;
	//delete the existing file
	std::remove(config_file.c_str());

	std::cout << "removed existing config file." << std::endl;

	//write the new path to the file.
	std::string path_contents = std::string(root_folder);
	std::cout << "root_folder:" << path_contents << std::endl;
	struct stat st = {0};
	if (stat(path_contents.c_str(), &st) == -1) {
		mkdir(path_contents.c_str(), 0755);
	}
	
	save_state(root_folder,"","",-1,-1);
	
}

bool EmapContainer::on_key_press_or_release_event(GdkEventKey* event) {

	std::cout << "wrote config file." << std::endl;

	//right arrow == expand
	if (event->keyval == 65363) {
		Gtk::TreeModel::iterator iter =
				treeview->get_selection()->get_selected();
		Gtk::TreePath path = model->get_path(iter);

		treeview->expand_row(path, false);

	}

	//left arrow == collapse
	if (event->keyval == 65361) {
		Gtk::TreeModel::iterator iter =
				treeview->get_selection()->get_selected();
		Gtk::TreePath path = model->get_path(iter);
		treeview->collapse_row(path);

	}

	return false;
}

void EmapContainer::on_selection_changed(Gtk::TreeView* treeview) {

	std::cout << "on_selection_changed: " << std::endl;

	Gtk::TreeModel::iterator iter =
			treeview->get_selection()->get_selected();
	if (iter) //If anything is selected
	{
		Gtk::TreeModel::Row row = *iter;
		const Glib::ustring filename = row[columns.name];
		const Glib::ustring path = row[columns.path];
		const int bank = row[columns.bank];
		const int program = row[columns.program];

		std::cout << "filename " << filename << std::endl;
		std::cout << "path " << path << std::endl;
		std::cout << "bank " << bank << std::endl;
		std::cout << "program " << program << std::endl;

		if (is_soundfont(path.c_str())) {
			
			const Glib::ustring filepath = path.c_str();
			this->filepath = filepath;
			
			std::cout << "is_soundfont " << std::endl;

			fluid_synth_sfload(synth, path.c_str(), 1);

			soundfont = fluid_synth_get_sfont(synth, 0);

			std::cout << "soundfont name: " << path << std::endl;

			soundfont->iteration_start(soundfont);

			std::cout << "presets size before: " << presets.size() << std::endl;

			std::cout << "presets count of "<< path << ": " << presets.count(path) << std::endl;

			std::map<const Glib::ustring, int>::iterator it = presets.begin();

			//conditionally add the presets to the tree
			if (presets.count(path) == 0) {

				fluid_preset_t preset;
				while (soundfont->iteration_next(soundfont, &preset)) {
					int iBank = preset.get_banknum(&preset);
					int iProg = preset.get_num(&preset);
					char* preset_name = preset.get_name(&preset);

					Gtk::TreeModel::Row rownew = *iter;
					Gtk::TreeModel::Row rowp = *(model->append(
							rownew.children()));
					rowp[columns.name] = Glib::ustring(preset_name);
					rowp[columns.path] = Glib::ustring(preset_name);
					rowp[columns.bank] = iBank;
					rowp[columns.program] = iProg;

				}
				presets[path] = 1;

			}
			//save the soundfont name to disk.
			save_state(root_folder.c_str(), filepath.c_str(), "", -1, -1);
		}
		if (bank != -1 && program != -1) {
			const Glib::ustring filepath = this->filepath;
			fluid_synth_bank_select(synth, 0, bank);
			fluid_synth_program_change(synth, 0, program);
			fluid_synth_program_reset(synth);
			std::cout << "selection: " << filename << ", bank: " << bank
					<< ", program: " << program << std::endl;
			
			save_state(root_folder.c_str(), filepath.c_str(), filename.c_str(), bank, program);
		
		}

	}

}

void EmapContainer::save_state(const char* root_folder, const char* path, const char* preset, int bank, int program){
	
	std::cout << "save state" << std::endl;
	//delete the existing file
	std::remove(config_file.c_str());

	std::cout << "removed existing config file." << std::endl;

	//write the new path to the file.
	std::string path_contents = std::string(root_folder);
	std::string sfpath = std::string(path);
	std::string sfpreset = std::string(preset);
	std::string sfbank = std::to_string(bank);
	std::string sfprogram = std::to_string(program);
	
	JsonBuilder *builder = json_builder_new ();
	json_builder_begin_object (builder);
	json_builder_set_member_name (builder, "rootdir");
	json_builder_add_string_value (builder, path_contents.c_str());
	json_builder_set_member_name (builder, "sfpath");
	json_builder_add_string_value (builder, sfpath.c_str());
	json_builder_set_member_name (builder, "sfpreset");
	json_builder_add_string_value (builder, sfpreset.c_str());
	json_builder_set_member_name (builder, "sfbank");
	json_builder_add_string_value (builder, sfbank.c_str());
	json_builder_set_member_name (builder, "sfprogram");
	json_builder_add_string_value (builder, sfprogram.c_str());
	json_builder_end_object (builder);

	std::cout << "save state to json with root_folder: " << path_contents << std::endl;
	std::cout << "sfpath: " << sfpath << std::endl;
	std::cout << "sfpreset: " << sfpreset << std::endl;
	std::cout << "sfbank: " << sfbank << std::endl;
	std::cout << "sfprogram: " << sfprogram << std::endl;

	JsonGenerator *gen = json_generator_new ();
	JsonNode * root = json_builder_get_root (builder);
	json_generator_set_root (gen, root);
	json_generator_to_file (gen, config_file.c_str(), NULL);
	json_node_free (root);
	g_object_unref (gen);
	g_object_unref (builder);
}

