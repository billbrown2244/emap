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
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/table.h>
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

//http://hammered999.wordpress.com/
//gave info for how to redraw the treeView

EmapContainer::EmapContainer(fluid_synth_t* synth_new) {

	set_title("EMAP - Easy Midi Audio Production");

	synth = synth_new;

	container = new Gtk::Table(2);
	scrolled = new Gtk::ScrolledWindow();
	path_container = new Gtk::Table(1, 2); //2 elements
	//path_label = new Gtk::Label("Path: ");
	//path_entry = new Gtk::Entry();
	//path_enumerate = new Gtk::Button("Enumerate");
	treeview = new Gtk::TreeView;
	set_root_folder_button = new Gtk::Button("Set Root Folder");
	quit_button = new Gtk::Button("Quit");

	//setup the TreeView
	model = Gtk::TreeStore::create(columns);
	//treeview->set_headers_visible(false);
	treeview->set_model(model);
	treeview->append_column("", columns.name);

	//setup the path_enumerate as default widget
	//setup the path_entry
	//path_enumerate->set_can_default(true);
	//set_default(*path_enumerate);
	//path_entry->set_activates_default(true);

	treeview->signal_key_press_event().connect(
			sigc::mem_fun(this, &EmapContainer::on_key_press_or_release_event));
	treeview->signal_key_release_event().connect(
			sigc::mem_fun(this, &EmapContainer::on_key_press_or_release_event));
	treeview->add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);

	//connect the signals
	//path_enumerate->signal_clicked().connect(
	//		sigc::mem_fun(this, &EmapContainer::on_enumerate_clicked));

	set_root_folder_button->signal_clicked().connect(
			sigc::mem_fun(this, &EmapContainer::on_button_clicked));

	quit_button->signal_clicked().connect(
			sigc::mem_fun(this, &EmapContainer::on_button_quit));

	//Connect signal:signal_row_activated
	Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
			treeview->get_selection();

	refTreeSelection->signal_changed().connect(
			sigc::mem_fun(*this, &EmapContainer::on_selection_changed));

	//add the path_* widgets to the path_container
	/*
	 path_container->attach(*path_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL,
	 Gtk::SHRINK);
	 path_container->attach(*path_entry, 1, 2, 0, 1, Gtk::FILL | Gtk::EXPAND,
	 Gtk::SHRINK);
	 path_container->attach(*path_enumerate, 2, 3, 0, 1, Gtk::SHRINK | Gtk::FILL,
	 Gtk::SHRINK);
	 */
	path_container->attach(*set_root_folder_button, 0, 1, 0, 1,
			Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
	path_container->attach(*quit_button, 1, 2, 0, 1, Gtk::SHRINK | Gtk::FILL,
			Gtk::SHRINK);

	//Setup the ScrolledWindow and add the TreeView in it
	scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrolled->add(*treeview);

	//add the path_container and the ScrolledWindow to the main container
	container->attach(*path_container, 0, 1, 0, 1, Gtk::FILL | Gtk::EXPAND,
			Gtk::SHRINK);
	container->attach(*scrolled, 0, 1, 1, 2);

	//finally add the container to the main window and show all children
	add(*container);

	//set a default size
	set_default_size(400, 400);

	//set up the root folder
	struct passwd *pw = getpwuid(getuid());
	home_dir = pw->pw_dir;
	root_folder = home_dir;

	std::fstream fbuf;
	config_file = home_dir + "/.config/emap/rootdir.txt";
	std::cout << "config file:" << config_file << std::endl;

	fbuf.open(config_file.c_str(),
			std::ios::in | std::ios::out | std::ios::binary);

	//set the root folder
	if (!fbuf.is_open()) {
		std::cout
				<< "config file doesn't exist. create config file with default root folder: "
				<< home_dir << std::endl;
		set_root_folder(home_dir.c_str());
	} else {
		std::string line;
		std::getline(fbuf, line);
		root_folder = line;
		std::cout << "config file exists.  set root_folder to: " << root_folder
				<< std::endl;
	}

	Gtk::TreeModel::Row row;

	//populate the tree
	std::cout << "tree root folder: " << root_folder << std::endl;

	loadTree(root_folder.c_str(), root_folder.c_str(), row);

	show_all_children();
}

EmapContainer::~EmapContainer() {
	delete treeview;
	delete path_enumerate;
	delete path_entry;
	delete path_label;
	delete path_container;
	delete scrolled;
	delete container;
}

void EmapContainer::on_button_clicked() {
	std::cout << "Set Root Folder" << std::endl;
	Gtk::FileChooserDialog dialog("Please choose a folder",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dialog.set_transient_for((Gtk::Window&) (*this->get_toplevel()));

	std::cout << "root_folder before:  " << root_folder << std::endl;

	dialog.set_current_folder(root_folder);

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

		root_folder = dialog.get_filename();

		set_root_folder(root_folder.c_str());

		Gtk::TreeModel::Row row;	//pass an empty row.

		loadTree(root_folder.c_str(), root_folder.c_str(), row);

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

void EmapContainer::loadTree(const char* orig_path, const char* path,
		const Gtk::TreeModel::Row row) {
	//std::cout << "orig_path: " << path << std::endl;
	//std::cout << "path again: " << path << std::endl;

	//try {

		Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
		Glib::RefPtr<Gio::FileEnumerator> child_enumeration =
				file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);

		std::vector<Glib::ustring> file_names;
		Glib::RefPtr<Gio::FileInfo> file_info;
		while ((file_info = child_enumeration->next_file()) != 0) {

			std::string fileName = file_info->get_name();
			if (file_info->get_file_type() == Gio::FILE_TYPE_DIRECTORY) {

				//model->clear();
//

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
				if (orig_path == path) {
					//model.clear();
					//std::cout << "orig path, reset model: " << model
					//	<< std::endl;
					Gtk::TreeModel::Row rowb = *(model->append());
					//std::cout << "new row: " << rowb << std::endl;
					rowb[columns.name] = fileName;
					rowb[columns.path] = result;
					rowb[columns.bank] = -1;
					rowb[columns.program] = -1;
					//std::cout << "rowb: " << rowb << std::endl;
					loadTree(orig_path, result, rowb);
				} else {
					Gtk::TreeModel::Row rowb = *(model->append(row.children()));
					rowb[columns.name] = fileName;
					rowb[columns.path] = result;
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
					if (orig_path == path) {

						rownew = *(model->append());

					} else {

						rownew = *(model->append(row.children()));

					}
					rownew[columns.name] = fileName;
					rownew[columns.path] = result;
					rownew[columns.bank] = -1;
					rownew[columns.program] = -1;

				}
			}

		}

		std::sort(file_names.begin(), file_names.end());
	//} catch (int e) {
	//	std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
	//}
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

	//delete the existing file
	std::remove(config_file.c_str());

	//std::cout << "removed existing config file." << std::endl;

	//create dir path if it doesn't exist
	std::string dirs[] = { "/.config", "/emap" };

	for (int i = 0; i <= 1; i++) {
		const char* dir_path = (std::string(home_dir) + dirs[i]).c_str();
		mkdir(dir_path, 0755);
		//std::cout << "created directory path: " << dir_path << " " << mkdir_result
		//		<< std::endl;
		home_dir = dir_path;
	}
	//std::cout << "created directory path: " << home_dir << std::endl;

	//write the new path to the file.
	std::string path_contents = std::string(root_folder);
	//std::cout << "root_folder:" << path_contents << std::endl;

	std::ofstream set_root_folder;
	set_root_folder.open(config_file.c_str());
	set_root_folder << path_contents;
	set_root_folder.close();

	//std::cout << "wrote config file." << std::endl;

}

bool EmapContainer::on_key_press_or_release_event(GdkEventKey* event) {

	//std::cout << "on_key_press_or_release_event: type: " << event->type
	//		<< ", state: " << event->state << ", keyval: " << event->keyval
	//		<< std::endl;

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

	//std::cout << "event->type " << event->type << std::endl;

	//handle_alt_1_press(); // GDK_MOD1_MASK is normally the Alt key

	return false;
}

void EmapContainer::on_selection_changed() {

	//std::cout << "on_selection_changed: " << std::endl;

	Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
	if (iter) //If anything is selected
	{
		Gtk::TreeModel::Row row = *iter;
		const Glib::ustring filename = row[columns.name];
		const Glib::ustring path = row[columns.path];
		const int bank = row[columns.bank];
		const int program = row[columns.program];

		if (is_soundfont(path.c_str())) {

			fluid_synth_sfload(synth, path.c_str(), 1);

			soundfont = fluid_synth_get_sfont(synth, 0);

			if (bank == -1 && program == -1) {
				std::cout << "selection: " << filename
						<< ", load default bank and program." << std::endl;
			}

			//std::cout << "soundfont: " << soundfont << std::endl;

			Glib::ustring sfname = Glib::ustring(
					soundfont->get_name(soundfont));

			//std::cout << "soundfont name: " << sfname << std::endl;

			soundfont->iteration_start(soundfont);

			std::map<Glib::ustring, int>::iterator it = presets.begin();

			//for (it = presets.begin(); it != presets.end(); ++it) {
			//	std::cout << "presets before: " << it->first << " => "
			//			<< it->second << std::endl;
			//}

			//conditionally add the presets to the tree

			//std::cout << "it->second: " << it->second << std::endl;
			if (presets.find(sfname) == presets.end()) {
				//if(it != NULL) {
				//	std::cout << "build out presets for this soundfont"
				//			<< std::endl;
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
				presets[sfname] = 1;
				//presets.insert(std::pair<Glib::ustring, int>(sfname, 1));
				//for (it = presets.begin(); it != presets.end(); ++it) {
				//	std::cout << "presets after: " << it->first << " => "
				//			<< it->second << std::endl;
				//}
			}

		}
		if (bank != -1 && program != -1) {
			fluid_synth_bank_select(synth, 0, bank);
			fluid_synth_program_change(synth, 0, program);
			fluid_synth_program_reset(synth);
			std::cout << "selection: " << filename << ", bank: " << bank
					<< ", program: " << program << std::endl;
		}

	}

}

