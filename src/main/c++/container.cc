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

EmapContainer::EmapContainer(fluid_synth_t* synth_new, bool is_full_app) {

	if (is_full_app) {

		std::cout << "before top emap window:" << std::endl;

		emap = (new Gtk::Window())->gobj();
		//emap = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

		std::cout << "made top emap window:" << std::endl;

		set_title("EMAP - Easy Midi Audio Production");
		// gtk_window_set_title(GTK_WINDOW(emap),
		//				"EMAP - Easy Midi Audio Production");

		std::cout << "set title:" << std::endl;

		synth = synth_new;

		container = (new Gtk::Table(2))->gobj();
		scrolled = new Gtk::ScrolledWindow();
		path_container = new Gtk::Table(1, 4); //2 elements

		std::cout << "made container and path_container:" << std::endl;

		treeview = GTK_TREE_VIEW(gtk_tree_view_new()); //(new Gtk::TreeView())->gobj();
		set_root_folder_button = new Gtk::Button("Set Root Folder");
		quit_button = new Gtk::Button("Quit");
		expand_all_button = new Gtk::Button("Expand All");
		collapse_all_button = new Gtk::Button("Collapse All");

		std::cout << "made treeview and buttons:" << std::endl;

		//setup the TreeView
		model = Gtk::TreeStore::create(columns);

		std::cout << "created model:" << std::endl;

		Glib::wrap(treeview)->set_model(model);

		Glib::wrap(treeview)->append_column("", columns.name);

		Glib::wrap(treeview)->signal_key_press_event().connect(
				sigc::mem_fun(this,
						&EmapContainer::on_key_press_or_release_event));
		Glib::wrap(treeview)->signal_key_release_event().connect(
				sigc::mem_fun(this,
						&EmapContainer::on_key_press_or_release_event));
		Glib::wrap(treeview)->add_events(
				Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);

		std::cout << "connected tree signals:" << std::endl;

		//connect the signals
		set_root_folder_button->signal_clicked().connect(
				sigc::bind<EmapContainer*>(
						sigc::mem_fun(*this, &EmapContainer::on_button_clicked),
						this));

		quit_button->signal_clicked().connect(
				sigc::mem_fun(*this, &EmapContainer::on_button_quit), this);

		//expand_all_button->signal_clicked().connect(
		//		sigc::mem_fun(this, &EmapContainer::on_button_expand));
		expand_all_button->signal_clicked().connect(
				sigc::bind<GtkTreeView*>(
						sigc::mem_fun(*this, &EmapContainer::on_button_expand),
						treeview));

		//collapse_all_button->signal_clicked().connect(
		//		sigc::mem_fun(this, &EmapContainer::on_button_collapse));
		collapse_all_button->signal_clicked().connect(
				sigc::bind<GtkTreeView*>(
						sigc::mem_fun(*this,
								&EmapContainer::on_button_collapse), treeview));

		//Connect signal:signal_row_activated
		Glib::RefPtr < Gtk::TreeSelection > refTreeSelection = Glib::wrap(
				treeview)->get_selection();

		refTreeSelection->signal_changed().connect(
				sigc::bind<GtkTreeView*>(
						sigc::mem_fun(*this,
								&EmapContainer::on_selection_changed),
						treeview));

		//add the path_* widgets to the path_container

		path_container->attach(*set_root_folder_button, 0, 1, 0, 1,
				Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
		path_container->attach(*expand_all_button, 1, 2, 0, 1,
				Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
		path_container->attach(*collapse_all_button, 2, 3, 0, 1,
				Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
		path_container->attach(*quit_button, 3, 4, 0, 1,
				Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);

		//Setup the ScrolledWindow and add the TreeView in it
		scrolled->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		scrolled->add(*(Glib::wrap(treeview)));

		//add the path_container and the ScrolledWindow to the main container
		(Glib::wrap(container))->attach(*path_container, 0, 1, 0, 1,
				Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK);
		(Glib::wrap(container))->attach(*scrolled, 0, 1, 1, 2);

		//finally add the container to the main window and show all children
		add(*(Glib::wrap(container)));
		//gtk_container_add(GTK_CONTAINER(emap), GTK_WIDGET(container));

		//set a default size
		set_default_size(400, 400);
		//gtk_window_set_default_size(GTK_WINDOW(emap), 400, 400);

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
			std::cout << "config file exists.  set root_folder to: "
					<< root_folder << std::endl;
		}

		Gtk::TreeModel::Row row;

		//populate the tree
		std::cout << "tree root folder: " << root_folder << std::endl;

		loadTree(root_folder.c_str(), root_folder.c_str(), row);

		show_all_children();
		//gtk_widget_show_all(GTK_WIDGET(emap));

		this->hide();

	} else {
		gtk_init(0, NULL);

		GtkWindow* emap2 = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
		emap = emap2;

		std::cout << "making EMAP container (fluidsynth UI)" << std::endl;

		gtk_window_set_title(GTK_WINDOW(emap2),
				"EMAP - Easy Midi Audio Production");

		std::cout << "set title" << std::endl;

		gtk_window_set_default_size(GTK_WINDOW(emap2), 400, 400);

		synth = synth_new;

		GtkWidget* container2 = gtk_table_new(2, 0, false);
		std::cout << "made container2" << std::endl;
		container = GTK_TABLE(container2);

		GtkWidget* scrolled2 = gtk_scrolled_window_new(NULL, NULL); //hadj, vadj);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled2),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		std::cout << "made scrolled2" << std::endl;

		//path_container = gtk_grid_new();
		GtkWidget* path_container2 = gtk_table_new(1, 4, true);
		std::cout << "made path_container2" << std::endl;

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
		model = Glib::wrap(modelc);

		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("",
				renderer, "text", TEXT_COLUMN, NULL);
		//gtk_tree_view_column_pack_start(column, renderer, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

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

		//g_signal_connect(treeview2, "cursor-changed",
		//		G_CALLBACK(&EmapContainer::on_key_press_or_release_event2),
		//		GTK_TREE_VIEW(treeview2));

		GtkTreeSelection* selection = gtk_tree_view_get_selection(
				GTK_TREE_VIEW(treeview2));

		g_signal_connect(selection, "changed",
				G_CALLBACK(&EmapContainer::on_selection_changedLv2), selection);

		std::cout << "connected tree and button signals." << std::endl;

		gtk_table_attach(GTK_TABLE(path_container2), set_root_folder_button2, 0,
				1, 0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK,
				0, 0);
		gtk_table_attach(GTK_TABLE(path_container2), expand_all_button2, 1, 2,
				0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0,
				0);
		gtk_table_attach(GTK_TABLE(path_container2), collapse_all_button2, 2, 3,
				0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0,
				0);

		//Setup the ScrolledWindow and add the TreeView in it
		gtk_container_add((GtkContainer*) scrolled2, (GtkWidget*) treeview2);

		gtk_table_attach(GTK_TABLE(container2), path_container2, 0, 1, 0, 1,
				(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_SHRINK, 0, 0);

		gtk_table_attach_defaults(GTK_TABLE(container2), scrolled2, 0, 1, 1, 2); //0, 2, 1, 2);

		gtk_container_add(GTK_CONTAINER(emap2), container2);

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
			std::cout << "config file exists.  set root_folder to: "
					<< root_folder << std::endl;
		}

		GtkTreeIter *row;	// = gtk_tree_path_new();

		//populate the tree
		std::cout << "tree root folder: " << root_folder << std::endl;

		loadTreeLv2(root_folder.c_str(), root_folder.c_str(), row, modelc);

		std::cout << "loaded initial tree" << std::endl;

		gtk_widget_show_all(GTK_WIDGET(emap2));
	}
}

/*
 EmapContainer::EmapContainer(fluid_synth_t* synth_new, bool is_full_app) {

 gtk_init(0, NULL);

 emap = Glib::wrap(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL)));

 std::cout << "making EMAP container (fluidsynth UI)" << std::endl;

 gtk_window_set_title(GTK_WINDOW(emap->gobj()), "EMAP - Easy Midi Audio Production");

 std::cout << "set title" << std::endl;

 gtk_window_set_default_size(GTK_WINDOW(emap->gobj()), 400, 400);

 synth = synth_new;

 container = Glib::wrap(gtk_table_new(2, 0, false));
 std::cout << "made container" << std::endl;

 scrolled = Glib::wrap(gtk_scrolled_window_new(NULL, NULL)); //hadj, vadj);
 gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled->gobj()),
 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
 std::cout << "made scrolled " << scrolled << std::endl;

 //path_container = gtk_grid_new();
 path_container = Glib::wrap(gtk_table_new(1, 4, true));
 std::cout << "made path_container" << std::endl;

 treeview = Glib::wrap((GtkTreeView*) gtk_tree_view_new());
 std::cout << "made treeview " << treeview << std::endl;

 set_root_folder_button = Glib::wrap(
 gtk_button_new_with_label("Set Root Folder"));
 std::cout << "made set_root_folder_button" << std::endl;

 if (is_full_app) {
 quit_button = Glib::wrap(gtk_button_new_with_label("Quit"));
 std::cout << "made quit_button" << std::endl;
 }

 expand_all_button = Glib::wrap(gtk_button_new_with_label("Expand All"));
 std::cout << "made expand_all_button" << std::endl;

 collapse_all_button = Glib::wrap(gtk_button_new_with_label("Collapse All"));
 std::cout << "made collapse_all_button" << std::endl;

 std::cout << "created EMAP base components." << std::endl;

 //setup the TreeView
 model = Glib::wrap(
 gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
 G_TYPE_INT, G_TYPE_INT));
 gtk_tree_view_set_model((dynamic_cast<Gtk::TreeView*>(treeview))->gobj(),
 (GtkTreeModel*) model->gobj());

 GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
 GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("",
 renderer, "text", TEXT_COLUMN, NULL);
 //gtk_tree_view_column_pack_start(column, renderer, true);
 gtk_tree_view_append_column(
 (dynamic_cast<Gtk::TreeView*>(treeview))->gobj(), column);

 std::cout << "made base tree model." << std::endl;

 //g_signal_connect(G_OBJECT((dynamic_cast<Gtk::TreeView*>(treeview))),
 //		"move-cursor",
 //		G_CALLBACK(&EmapContainer::on_key_press_or_release_event),
 //		(dynamic_cast<Gtk::TreeView*>(treeview)));

 //g_signal_connect(G_OBJECT((dynamic_cast<Gtk::TreeView*>(treeview))),
 //		"expand-collapse-cursor-row",
 //		G_CALLBACK(&EmapContainer::on_key_press_or_release_event2),
 //		(dynamic_cast<Gtk::TreeView*>(treeview)));

 std::cout << "connected tree cursor toggle signals." << std::endl;

 //treeview->signal_key_press_event().connect(
 //			sigc::mem_fun(this, &EmapContainer::on_key_press_or_release_event));
 /********************
 treeview->connect(sigc::mem_fun(this, &EmapContainer::on_key_press_or_release_event));
 treeview->signal_key_release_event().connect(
 sigc::mem_fun(this, &EmapContainer::on_key_press_or_release_event));
 treeview->add_events(Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
 ********************* /

 //connect the signals
 g_signal_connect(set_root_folder_button->gobj(), "clicked",
 G_CALLBACK(&EmapContainer::on_button_clicked), this);

 if (is_full_app) {
 g_signal_connect(quit_button->gobj(), "clicked",
 G_CALLBACK(&EmapContainer::on_button_quit), NULL);
 }

 g_signal_connect(expand_all_button->gobj(), "clicked",
 G_CALLBACK(&EmapContainer::on_button_expand), treeview->gobj());

 g_signal_connect(collapse_all_button->gobj(), "clicked",
 G_CALLBACK(&EmapContainer::on_button_collapse), treeview->gobj());

 g_signal_connect(treeview->gobj(), "cursor-changed",
 G_CALLBACK(&EmapContainer::on_selection_changed), treeview->gobj());

 //************DO THIS ****************
 //Connect signal:signal_row_activated
 //Glib::RefPtr < Gtk::TreeSelection > refTreeSelection =
 //		treeview->get_selection();

 /********************START HERE AND ABOVE SECTION LIKE ******
 refTreeSelection->signal_changed().connect(
 sigc::mem_fun(*this, &EmapContainer::on_selection_changed));
 */
//add the path_* widgets to the path_container
/*
 gtk_grid_attach(GTK_GRID(path_container), set_root_folder_button, 0, 0, 1, 1);
 gtk_grid_attach(GTK_GRID(path_container), expand_all_button, 1, 0, 1, 1);
 gtk_grid_attach(GTK_GRID(path_container), collapse_all_button, 2, 0, 1, 1);
 gtk_grid_attach(GTK_GRID(path_container), quit_button, 3, 0, 1, 1);
 * /

 gtk_table_attach(GTK_TABLE(path_container->gobj()), set_root_folder_button->gobj(),
 0, 1, 0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK,
 0, 0);
 gtk_table_attach(GTK_TABLE(path_container->gobj()), expand_all_button->gobj(), 1, 2,
 0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0, 0);
 gtk_table_attach(GTK_TABLE(path_container->gobj()), collapse_all_button->gobj(), 2,
 3, 0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0,
 0);
 if (is_full_app) {
 gtk_table_attach(GTK_TABLE(path_container->gobj()), quit_button->gobj(), 3, 4,
 0, 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0,
 0);
 }
 //Setup the ScrolledWindow and add the TreeView in it
 gtk_container_add((GtkContainer*) scrolled->gobj(), (GtkWidget*) treeview->gobj());

 gtk_table_attach(GTK_TABLE(container->gobj()), path_container->gobj(), 0, 1, 0, 1,
 (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), GTK_SHRINK, 0, 0);

 gtk_table_attach_defaults(GTK_TABLE(container->gobj()), scrolled->gobj(), 0, 1, 1,
 2);	//0, 2, 1, 2);

 gtk_container_add(GTK_CONTAINER(emap->gobj()), container->gobj());

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

 gtk_widget_show_all(GTK_WIDGET(emap->gobj()));
 }
 */

EmapContainer::~EmapContainer() {
	delete treeview;
	delete path_container;
	delete scrolled;
	delete container;
}

bool EmapContainer::on_key_press_or_release_event2(GtkTreeView *tree_view,
		gpointer treeview) {
	std::cout << "cursor changed " << std::endl;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(treeview));

	g_signal_connect(selection, "changed",
			G_CALLBACK(&EmapContainer::on_selection_changedLv2), selection);
}

void EmapContainer::on_selection_changedLv2(GtkWidget *widget,
		gpointer selection) {
	std::cout << "selection changed " << std::endl;

	GtkTreeIter iter;
	GtkTreeModel * model;
	char* name;
	char* path;
	int bank, program;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model,
			&iter)) {
		std::cout << "got selected " << std::endl;

		gtk_tree_model_get(model, &iter, NAME, &name, -1);
		std::cout << "name: " << name << std::endl;
		g_free(name);

		gtk_tree_model_get(model, &iter, PATH, &path, -1);
		std::cout << "path: " << path << std::endl;
		g_free(path);

		//gtk_tree_model_get(model, &iter, BANK, bank,-1);
		// std::cout << "bank: " << bank << std::endl;
		//g_free(bank);

		//  gtk_tree_model_get(model, &iter, PROGRAM, program,-1);
		//  std::cout << "program: " << program << std::endl;
		// g_free(program);

		// g_free(iter);
		//  std::cout << "freed iter: " << std::endl;

		// std::cout << "bank: " << bank << std::endl;
		// std::cout << "program: " << program << std::endl;

		//g_free(bank);
		// g_free(program);
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

	std::cout << "loading tree for:" << orig_path << std::endl;

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

	std::cout << "sortstruct" << std::endl;

	//parent and child rows.
	GtkTreeIter child;

	//loop throught the results and add them to the tree.
	for (std::vector<int>::size_type i = 0; i != sorted.size(); i++) {
		file_info = sorted[i];

		//get the file name from the file_info object.
		std::string fileName = file_info->get_name();

		//std::cout << "fileName: " << fileName << std::endl;

		if (file_info->get_file_type() == Gio::FILE_TYPE_DIRECTORY) {

			std::cout << "found directory." << std::endl;

			file_names.push_back(fileName);
			//std::cout << "push back." << std::endl;

			//we just want the last portion of the file path with the name
			char result[strlen(path) + strlen(fileName.c_str())]; // array to hold the result.
			strcpy(result, path);
			strcat(result, "/");
			strcat(result, fileName.c_str());

			std::cout << "result: " << result << std::endl;

			//dont display the first folder in the result;
			if (strcmp(orig_path, path) == 0) {

				std::cout << "same path" << std::endl;

				gtk_tree_store_append(model, &child, NULL);	//new row

				std::cout << "created new row." << std::endl;

				gtk_tree_store_set(model, &child, NAME, fileName.c_str(), PATH,
						result, BANK, 0, PROGRAM, 0, -1);

				loadTreeLv2(orig_path, result, &child, model);

			} else {

				std::cout << "new path" << std::endl;

				gtk_tree_store_append(model, &child, row);	//new
				gtk_tree_store_set(model, &child, NAME, fileName.c_str(), PATH,
						result, BANK, 0, PROGRAM, 0, -1);

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
						result, BANK, 0, PROGRAM, 0, -1);

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
				Glib::wrap(treeview)->get_selection()->get_selected();
		Gtk::TreePath path = model->get_path(iter);

		Glib::wrap(treeview)->expand_row(path, false);

	}

	//left arrow == collapse
	if (event->keyval == 65361) {
		Gtk::TreeModel::iterator iter =
				Glib::wrap(treeview)->get_selection()->get_selected();
		Gtk::TreePath path = model->get_path(iter);
		Glib::wrap(treeview)->collapse_row(path);

	}

	return false;
}

void EmapContainer::on_selection_changed(GtkTreeView* treeview) {

	std::cout << "on_selection_changed: " << std::endl;

	Gtk::TreeModel::iterator iter =
			Glib::wrap(treeview)->get_selection()->get_selected();
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

			//std::cout << "soundfont: " << soundfont << std::endl;

			Glib::ustring sfname = Glib::ustring(
					soundfont->get_name(soundfont));

			//std::cout << "soundfont name: " << sfname << std::endl;

			soundfont->iteration_start(soundfont);

			std::map<Glib::ustring, int>::iterator it = presets.begin();

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
static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor * descriptor,
		const char * plugin_uri, const char * bundle_path,
		LV2UI_Write_Function write_function, LV2UI_Controller controller,
		LV2UI_Widget * widget, const LV2_Feature * const * features) {

	if (strcmp(plugin_uri, EMAP_URI) != 0) {
		fprintf(stderr,
				"SORCER_URI error: this GUI does not support plugin with URI %s\n",
				plugin_uri);
		return NULL;
	}

	FSynth fsynth;
	EmapContainer* emap = new EmapContainer(fsynth.get_synth(), false);

	std::cout << "Allocated SourceGUI!" << std::endl;

	if (emap == NULL)
		return NULL;

	//reparent the container to something the same default size as the standalone app
	GtkWidget* new_parent = gtk_vbox_new(false, 0);
	gtk_widget_set_size_request(new_parent, 400, 400);
	gtk_widget_reparent(GTK_WIDGET(emap->container), new_parent);

	gtk_widget_set_size_request(new_parent, 400, 400);

	std::cout << "Creating UI!" << std::endl;
	*widget = (LV2UI_Widget) new_parent;

	//hide the inner top level window
	gtk_widget_hide(GTK_WIDGET(emap->emap));

	std::cout << "returning UI..." << std::endl;

	return (LV2UI_Handle) emap;
}

static void cleanup(LV2UI_Handle ui) {
	std::cout << "cleanup EMAP UI lv2" << std::endl;
	EmapContainer *pluginGui = (EmapContainer *) ui;
	free(pluginGui);
}

static void port_event(LV2UI_Handle ui, uint32_t port_index,
		uint32_t buffer_size, uint32_t format, const void * buffer) {
	EmapContainer *self = (EmapContainer *) ui;

	std::cout << "Port event on index " << port_index << "  Format is "
			<< format << std::endl;
	return;
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

