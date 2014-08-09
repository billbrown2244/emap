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

EmapContainer::EmapContainer(fluid_synth_t* synth_new, bool is_lv2) {

	if (!is_lv2) { // this is the full version

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
			std::cout << "config file exists.  set root_folder to: "
					<< root_folder << std::endl;
		}

		Gtk::TreeModel::Row row;

		//populate the tree
		std::cout << "tree root folder: " << root_folder << std::endl;

		loadTree(root_folder.c_str(), root_folder.c_str(), row);

		show_all_children();

		this->hide();

	} else {

		std::cout << "making EMAP container (fluidsynth UI)" << std::endl;

		GtkWindow* emap2 = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
		emap = emap2;

		std::cout << "called gtk_window_new." << std::endl;

		gtk_window_set_title(GTK_WINDOW(emap2),
				"EMAP - Easy Midi Audio Production");

		std::cout << "set title" << std::endl;

		gtk_window_set_default_size(GTK_WINDOW(emap2), 400, 400);

		std::cout << "set default window size" << std::endl;

		container2 = gtk_grid_new();
		std::cout << "made container" << std::endl;

		GtkWidget* scrolled2 = gtk_scrolled_window_new(NULL, NULL); //hadj, vadj);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled2),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled2),400);
		gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scrolled2),400);
		std::cout << "made scrolled2" << std::endl;

		GtkWidget* treeview2 = gtk_tree_view_new();
		std::cout << "made treeview2" << std::endl;

		GtkWidget* set_root_folder_button2 = gtk_button_new_with_label(
				"Set Root Folder");
		std::cout << "made set_root_folder_button2" << std::endl;

		GtkWidget* expand_all_button2 = gtk_button_new_with_label("Expand All");
		std::cout << "made expand_all_button2" << std::endl;

		GtkWidget* collapse_all_button2 = gtk_button_new_with_label(
				"Collapse All");
		std::cout << "made collapse_all_button2" << std::endl;

		std::cout << "created EMAP base components." << std::endl;

		//setup the TreeView
		modelc = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
				G_TYPE_INT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview2),
				(GtkTreeModel*) modelc);

		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("",
				renderer, "text", TEXT_COLUMN, NULL);

		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

		//initialize the presets to NULL
		this->m_cpresets = NULL;

		std::cout << "made base tree model2." << std::endl;

		//connect the signals
		g_signal_connect(G_OBJECT(set_root_folder_button2), "clicked",
				G_CALLBACK(&EmapContainer::on_button_clickedLv2), this);

		g_signal_connect(expand_all_button2, "clicked",
				G_CALLBACK(&EmapContainer::on_button_expand),
				GTK_TREE_VIEW(treeview2));

		g_signal_connect(collapse_all_button2, "clicked",
				G_CALLBACK(&EmapContainer::on_button_collapse),
				GTK_TREE_VIEW(treeview2));

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview2));

		g_signal_connect(selection, "changed",
				G_CALLBACK(&EmapContainer::on_selection_changedLv2), this);

		std::cout << "connected tree and button signals." << std::endl;

		gtk_grid_attach(GTK_GRID(container2), set_root_folder_button2, 0, 0, 1,
				1);
		gtk_grid_attach(GTK_GRID(container2), expand_all_button2, 1, 0, 1, 1);

		gtk_grid_attach(GTK_GRID(container2), collapse_all_button2, 2, 0, 1, 1);

		//Setup the ScrolledWindow and add the TreeView to the scrolled window
		gtk_container_add((GtkContainer*) scrolled2, (GtkWidget*) treeview2);

		gtk_grid_attach(GTK_GRID(container2), scrolled2, 0, 1, 3, 1);

		gtk_container_add(GTK_CONTAINER(emap2), container2);

		//set up the root folder
		passwd *pw = getpwuid(getuid());
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
			std::cout << "config file exists.  set root_folder to: "
					<< root_folder << std::endl;
		}

		GtkTreeIter *row = NULL;

		//populate the tree
		std::cout << "tree root folder: " << root_folder << std::endl;

		loadTreeLv2(root_folder.c_str(), root_folder.c_str(), row, modelc);

		std::cout << "loaded initial tree" << std::endl;

		gtk_widget_show_all(GTK_WIDGET(emap2));
	}
}

EmapContainer::~EmapContainer() {
	delete treeview;
	delete path_container;
	delete scrolled;
}

void EmapContainer::on_selection_changedLv2(GtkWidget *widget, gpointer data) {
	std::cout << "selection changed " << std::endl;

	EmapContainer* emap = (EmapContainer*) data;

	//struct cpresets* presets = emap->cpresets;

	GtkTreeSelection* selection = emap->selection;

	GtkTreeIter iter;
	GtkTreeIter* row = NULL;
	GtkTreeModel * model;
	char* name;
	char* path;
	int bank, program;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		std::cout << "got selected " << std::endl;

		gtk_tree_model_get(model, &iter, NAME, &name, PATH, &path, BANK, &bank,
				PROGRAM, &program, -1);

		std::cout << "name: " << name << std::endl;
		std::cout << "path: " << path << std::endl;
		std::cout << "bank: " << bank << std::endl;
		std::cout << "program: " << program << std::endl;

		if (is_soundfont(path) || (bank != -1 && program != -1)) {
			std::cout << "name: " << name << " is a soundfont lets open it."
					<< std::endl;

			emap->name = name;
			emap->path = path;
			emap->bank = bank;
			emap->program = program;

			//if this is an actual patch, send it to the
			//backend to load.
			if (bank != -1 && program != -1) {
				std::cout << "send ui state to backend. " << std::endl;
				send_ui_state(emap);

			} else { //otherwise lets build out the patch leaves

				std::cout
						<< "just open the soundfont but don't load any default patch. "
						<< std::endl;

				fluid_settings_t* settings = new_fluid_settings();
				std::cout << "got fluid_settings_t " << std::endl;

				fluid_synth_t* synth = new_fluid_synth(settings);
				std::cout << "got fluid_synth_t. " << std::endl;

				fluid_synth_sfload(synth, path, 1);
				std::cout << "fluid_synth_sfload. " << std::endl;

				fluid_sfont_t* soundfont = fluid_synth_get_sfont(synth, 0);
				std::cout << "fluid_sfont_t. " << std::endl;

				//char* sfname = soundfont->get_name(soundfont);

				//std::cout << "sfname: " << sfname << std::endl;

				soundfont->iteration_start(soundfont);

				std::cout << "started iteration " << std::endl;

				std::cout << "presets size before: "
				//	<< HASH_COUNT(emap->cpresets) << std::endl;
						<< HASH_COUNT(emap->m_cpresets) << std::endl;

				struct cpresets *existingpreset = 0;
				std::cout << "pexistingpreset before: " << existingpreset
						<< std::endl;

				HASH_FIND_STR(emap->m_cpresets, path, existingpreset);

				if (HASH_COUNT(emap->m_cpresets) > 0
						&& existingpreset != NULL) {
					std::cout << "pexistingpreset after: "
							<< existingpreset->soundfont_key << std::endl;
				}
				//conditionally add the presets to the tree
				if (HASH_COUNT(emap->m_cpresets) == 0
						|| existingpreset == NULL) {

					std::cout << "presets.find(name) == presets.end()"
							<< std::endl;

					fluid_preset_t preset;
					while (soundfont->iteration_next(soundfont, &preset)) {
						std::cout << "soundfont->iteration_next: " << std::endl;
						int iBank = preset.get_banknum(&preset);
						int iProg = preset.get_num(&preset);
						char* preset_name = preset.get_name(&preset);

						std::cout << "iBank: " << iBank << std::endl;

						std::cout << "iProg: " << iProg << std::endl;

						std::cout << "preset_name: " << preset_name
								<< std::endl;

						std::cout << "emap->modelc: " << emap->modelc
								<< std::endl;

						std::cout << "iter: " << &iter << std::endl;

						std::cout << "row: " << row << std::endl;

						GtkTreeIter child2;

						gtk_tree_store_append(emap->modelc, &child2, &iter);
						std::cout << "appended new row." << std::endl;
						gtk_tree_store_set(emap->modelc, &child2, NAME,
								preset_name, PATH, path, BANK, iBank, PROGRAM,
								iProg, -1);
						std::cout << "appended new row2." << std::endl;

					}
					//add the preset
					//add_preset(presets, path, "TRUE");
					//struct cpresets* presetadd = (struct cpresets*) malloc(
					//		sizeof(struct cpresets));
					cpresets* presetadd = (cpresets*) malloc(sizeof(cpresets));

					std::cout << "allocated preset: " << presetadd << std::endl;

					//const char* preset_path = path;
					//strcpy(presetadd->soundfont_key, path);
					presetadd->soundfont_key = path;
					std::cout << "set soundfont_key." << std::endl;

					const char* exists = "TRUE";
					presetadd->exists = exists;

					HASH_ADD_STR(emap->m_cpresets, soundfont_key, presetadd);
					std::cout << "added preset to map." << std::endl;

					//strcpy(presetadd->exists, );
					std::cout << "set exists." << std::endl;

					std::cout << "presets size after: "
							<< HASH_COUNT(emap->m_cpresets) << std::endl;

				} else {
					std::cout << "(presets.find(name) == presets.end()) false"
							<< std::endl;
				}

				//free the memory
				delete_fluid_synth(synth);
				delete_fluid_settings(settings);
			}

		} else {
			std::cout << "name: " << name << " is just a folder.  do nothing. "
					<< std::endl;
		}

	} else {

		std::cout << "didn't get selected " << std::endl;
	}

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

void EmapContainer::on_button_clickedLv2(EmapContainer* emap) {

	std::cout << "Set Root Folder" << std::endl;

	GtkWidget *dialog;

	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

	dialog = gtk_file_chooser_dialog_new("Please choose a folder",
			GTK_WINDOW(emap->emap), action, "Cancel", GTK_RESPONSE_CANCEL,
			"Open", GTK_RESPONSE_ACCEPT, NULL);

	int result = gtk_dialog_run(GTK_DIALOG(dialog));
	std::cout << "result" << result << std::endl;
	//Handle the response:
	switch (result) {
	case (GTK_RESPONSE_OK):
	case (GTK_RESPONSE_ACCEPT): {
		std::cout << "Select clicked." << std::endl;
		std::string filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filename = std::string(gtk_file_chooser_get_current_folder(chooser));

		std::cout << "Folder selected: " << std::endl;

		std::cout << "model: " << emap->modelc << std::endl;

		gtk_tree_store_clear(emap->modelc);

		std::cout << "model cleared: " << std::endl;

		emap->root_folder = filename;

		//std::cout << "filename: " << filename << std::endl;
		//std::cout << "root_folder: " << emap->root_folder.c_str() << std::endl;

		set_root_folderLv2(emap);

		std::cout << "set new root folder: " << emap->root_folder.c_str()
				<< std::endl;

		GtkTreeIter* row;	//pass an empty row.

		loadTreeLv2(emap->root_folder.c_str(), emap->root_folder.c_str(), row,
				emap->modelc);

		//gtk_widget_show_all(GTK_WIDGET(emap->emap));

		std::cout << "loaded tree." << std::endl;

		gtk_widget_destroy(dialog);

		break;
	}
	case (GTK_RESPONSE_CANCEL): {
		std::cout << "Cancel clicked." << std::endl;
		gtk_widget_destroy(dialog);
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

void EmapContainer::loadTreeLv2(const char* orig_path, const char* path,
		GtkTreeIter *row, GtkTreeStore* model) {

	//std::cout << "loading tree for:" << orig_path << std::endl;

	//std::cout << "loading tree for:" << orig_path << std::endl;

	//std::cout << "orig_path: " << orig_path << std::endl;
	//std::cout << "path: " << path << std::endl;

	//get a list of file names for the current file "path"
	Gio::init();
	Glib::RefPtr < Gio::File > file = Gio::File::create_for_path(path);
	Glib::RefPtr < Gio::FileEnumerator > child_enumeration =
			file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);

	//std::cout << "setup enumeration" << std::endl;

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

	//std::cout << "sortstruct" << std::endl;

	//parent and child rows.
	GtkTreeIter child;

	//loop throught the results and add them to the tree.
	for (std::vector<int>::size_type i = 0; i != sorted.size(); i++) {
		file_info = sorted[i];

		//get the file name from the file_info object.
		std::string fileName = file_info->get_name();

		//std::cout << "fileName: " << fileName << std::endl;

		if (file_info->get_file_type() == Gio::FILE_TYPE_DIRECTORY) {

			//std::cout << "found directory." << std::endl;

			file_names.push_back(fileName);
			//std::cout << "push back." << std::endl;

			//we just want the last portion of the file path with the name
			char result[strlen(path) + strlen(fileName.c_str())]; // array to hold the result.
			strcpy(result, path);
			strcat(result, "/");
			strcat(result, fileName.c_str());

			//std::cout << "result: " << result << std::endl;

			//dont display the first folder in the result;
			if (strcmp(orig_path, path) == 0) {

				//std::cout << "same path" << std::endl;

				gtk_tree_store_append(model, &child, NULL);	//new row

				//std::cout << "created new row." << std::endl;

				gtk_tree_store_set(model, &child, NAME, fileName.c_str(), PATH,
						result, BANK, -1, PROGRAM, -1, -1);

				loadTreeLv2(orig_path, result, &child, model);

			} else {

				//std::cout << "new path" << std::endl;

				gtk_tree_store_append(model, &child, row);	//new
				gtk_tree_store_set(model, &child, NAME, fileName.c_str(), PATH,
						result, BANK, -1, PROGRAM, -1, -1);

				loadTreeLv2(orig_path, result, &child, model);
			}

		} else {

			//std::cout << "append file to tree" << std::endl;

			char result[strlen(path) + strlen(fileName.c_str())]; // array to hold the result.
			strcpy(result, path);
			strcat(result, "/");
			strcat(result, fileName.c_str());

			file_names.push_back(fileName);

			if (is_soundfont(result)) {

				if (strcmp(orig_path, path) == 0) {

					gtk_tree_store_append(model, &child, NULL); //new

				} else {

					gtk_tree_store_append(model, &child, row); //new

				}
				gtk_tree_store_set(model, &child, NAME, fileName.c_str(), PATH,
						result, BANK, -1, PROGRAM, -1, -1);

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

void EmapContainer::set_root_folderLv2(EmapContainer* emap) {

	std::cout << "set_root_folder" << std::endl;
	//delete the existing file
	std::remove(emap->config_file.c_str());

	//std::cout << "removed existing config file." << std::endl;

	//create dir path if it doesn't exist
	std::string dirs[] = { "/.config", "/emap" };

	for (int i = 0; i <= 1; i++) {
		const char* dir_path = (std::string(emap->home_dir) + dirs[i]).c_str();
		mkdir(dir_path, 0755);
		//std::cout << "created directory path: " << dir_path << " " << mkdir_result
		//		<< std::endl;
		emap->home_dir = dir_path;
	}
	//std::cout << "created directory path: " << home_dir << std::endl;

	//write the new path to the file.
	std::string path_contents = std::string(emap->root_folder);
	//std::cout << "root_folder:" << path_contents << std::endl;

	std::ofstream set_root_folder;
	set_root_folder.open(emap->config_file.c_str());
	set_root_folder << path_contents;
	set_root_folder.close();

	//std::cout << "wrote config file." << std::endl;

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

			std::cout << "is_soundfont " << std::endl;

			fluid_synth_sfload(synth, path.c_str(), 1);

			soundfont = fluid_synth_get_sfont(synth, 0);

			if (bank == -1 && program == -1) {
				std::cout << "selection: " << filename
						<< ", load default bank and program." << std::endl;
			}

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

//lv2 stuff
static LV2UI_Handle instantiate(const _LV2UI_Descriptor * descriptor,
		const char * plugin_uri, const char * bundle_path,
		LV2UI_Write_Function write_function, LV2UI_Controller controller,
		LV2UI_Widget * widget, const LV2_Feature * const * features) {
	std::cout << "instantiate EMAP lv2 UI" << std::endl;

	if (strcmp(plugin_uri, EMAP_URI) != 0) {
		std::cout
				<< "SORCER_URI error: this GUI does not support plugin with URI "
				<< plugin_uri << std::endl;
		return NULL;
	}

	gtk_init(0, NULL);

	std::cout << "called gtk_init." << std::endl;

	//allocate an emap instance.
	EmapContainer* emap = new EmapContainer(NULL, true);

	if (emap == NULL) {
		std::cout << "EMAP error: could not start EMAP." << std::endl;
		return NULL;
	}

	// Get host features
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			emap->map = (LV2_URID_Map*) features[i]->data;
		}
	}

	if (!emap->map) {
		std::cout << "EMAP error: Host does not support urid:map" << std::endl;
		free(emap);
		return NULL;
	}

	//initialize the communication port
	map_emap_uris(emap->map, &emap->uris);
	lv2_atom_forge_init(&emap->forge, emap->map);
	emap->write = write_function;
	emap->controller = controller;

	std::cout << "Allocated SourceGUI!" << std::endl;

	//reparent the container to something the same default size as the standalone app
	GtkWidget* new_parent = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);//gtk_vbox_new(false, 0);	////
	gtk_widget_set_size_request(new_parent, 400, 400);
	gtk_widget_reparent(GTK_WIDGET(emap->container2), new_parent);
	gtk_widget_set_size_request(new_parent, 400, 400);

	std::cout << "Creating UI!" << std::endl;
	*widget = (LV2UI_Widget) new_parent;

	//hide the inner top level window
	gtk_widget_hide(GTK_WIDGET(emap->emap));

	std::cout << "instantiated EMAP lv2 UI" << std::endl;

	return (LV2UI_Handle) emap;
}

/* tell the back end to load the correct soundfont and patch. */
void EmapContainer::send_ui_state(EmapContainer* emap) {

	std::cout << "send the UI state to the synth." << std::endl;

	// Use local buffer on the stack to build atom
	uint8_t obj_buf[1024];
	lv2_atom_forge_set_buffer(&emap->forge, obj_buf, sizeof(obj_buf));

	std::cout << "allocated buffer." << std::endl;

	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge forge;
	forge = emap->forge;

	//may need later
	LV2_Atom* obj = (LV2_Atom*) lv2_atom_forge_resource(&emap->forge, &frame, 0,
			emap->uris.ui_State);

	std::cout << "allocated resource." << std::endl;

	//send the loaded soundfont state back to the synth

	LV2_Atom* msg = (LV2_Atom*) lv2_atom_forge_blank(&forge, &frame, 1,
			emap->uris.ui_State);

	std::cout << "created message." << std::endl;

	std::cout << "name: " << emap->name << std::endl;

	std::cout << "length: " << strlen(emap->name) << std::endl;

	//name
	lv2_atom_forge_property_head(&emap->forge, emap->uris.ui_name, 0);

	lv2_atom_forge_string(&emap->forge, emap->name, strlen(emap->name));

	std::cout << "name: " << emap->name << std::endl;

	//path
	lv2_atom_forge_property_head(&emap->forge, emap->uris.ui_path, 0);
	lv2_atom_forge_string(&emap->forge, emap->path, strlen(emap->path));

	std::cout << "path: " << emap->path << std::endl;

	//bank
	lv2_atom_forge_property_head(&emap->forge, emap->uris.ui_bank, 0);
	lv2_atom_forge_int(&emap->forge, emap->bank);

	std::cout << "bank: " << emap->bank << std::endl;

	//program
	lv2_atom_forge_property_head(&emap->forge, emap->uris.ui_program, 0);
	lv2_atom_forge_int(&emap->forge, emap->program);

	std::cout << "program: " << emap->program << std::endl;

	// Finish object
	lv2_atom_forge_pop(&forge, &frame);

	std::cout << "lv2_atom_forge_pop" << std::endl;

	std::cout << "controller: " << emap->controller << std::endl;

	std::cout << "lv2_atom_total_size(msg): " << lv2_atom_total_size(msg)
			<< std::endl;

	std::cout << "controller: " << msg << std::endl;

	// send the state back to the synth.
	emap->write(emap->controller, 2, lv2_atom_total_size(msg),
			emap->uris.atom_eventTransfer, msg);

	std::cout << "wrote message" << std::endl;
}

static void cleanup(LV2UI_Handle ui) {
	std::cout << "cleanup EMAP UI lv2" << std::endl;
	EmapContainer *emap = (EmapContainer *) ui;
	std::cout << "emap name on cleanup: " << (emap->name == NULL) << std::endl;
	std::cout << "emap path on cleanup: " << (emap->path == NULL) << std::endl;
	std::cout << "emap bank on cleanup: " << (emap->bank == 0) << std::endl;
	std::cout << "emap program on cleanup: " << emap->program << std::endl;
	if (emap->name != NULL && emap->path != NULL) {
		emap->send_ui_disable(emap);
	}
	free(emap);
}

/** Notify backend that UI is closed. */
void EmapContainer::send_ui_disable(EmapContainer *emap) {

	std::cout << "tell EMAP backend that UI is closing." << std::endl;

	send_ui_state(emap);

	/*

	 std::cout << "sent ui state to the back end." << std::endl;

	 uint8_t obj_buf[64];
	 lv2_atom_forge_set_buffer(&emap->forge, obj_buf, sizeof(obj_buf));

	 std::cout << "lv2_atom_forge_set_buffer" << std::endl;

	 LV2_Atom_Forge_Frame frame;

	 LV2_Atom* msg = (LV2_Atom*) lv2_atom_forge_blank(&emap->forge, &frame, 0,
	 emap->uris.ui_Off);

	 std::cout << "lv2_atom_forge_blank" << std::endl;

	 lv2_atom_forge_pop(&emap->forge, &frame);

	 std::cout << "lv2_atom_forge_pop" << std::endl;

	 emap->write(emap->controller, 0, lv2_atom_total_size(msg),
	 emap->uris.atom_eventTransfer, msg);

	 std::cout << "emap->write" << std::endl;

	 */
}

static void port_event(LV2UI_Handle handle, uint32_t port_index,
		uint32_t buffer_size, uint32_t format, const void* buffer) {
	EmapContainer* emap = (EmapContainer*) handle;
	const LV2_Atom* atom = (const LV2_Atom*) buffer;

	/* Check type of data received
	 *  - format == 0: Control port event (float)
	 *  - format > 0:  Message (atom)
	 */
	if (format == emap->uris.atom_eventTransfer
			&& atom->type == emap->uris.atom_Blank) {
		const LV2_Atom_Object* obj = (const LV2_Atom_Object*) atom;
		if (obj->body.otype == emap->uris.ui_State) {
			std::cout << "recv_ui_state" << std::endl;
			//recv_ui_state(ui, obj);
		}
	}
}

static LV2UI_Descriptor descriptors[] = { { EMAP_UI_URI, instantiate, cleanup,
		port_event, NULL } };

const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
	switch (index) {
	case 0:
		return &descriptors[index];
	default:
		return NULL;
	}
}

