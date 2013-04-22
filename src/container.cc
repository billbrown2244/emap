/*
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
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <algorithm>

//http://hammered999.wordpress.com/
//gave info for how to redraw the treeView

EmapContainer::EmapContainer() {
	container = new Gtk::Table(2);
	scrolled = new Gtk::ScrolledWindow();
	path_container = new Gtk::Table(1, 3);
	path_label = new Gtk::Label("Path: ");
	path_entry = new Gtk::Entry();
	path_enumerate = new Gtk::Button("Enumerate");
	treeview = new Gtk::TreeView;
	set_root_folder_button = new Gtk::Button("Sound File Root");
	quit_button = new Gtk::Button("Quit");
	//start in homefolder if config file is not set.
	struct passwd *pw = getpwuid(getuid());

	homedir = pw->pw_dir;

	//setup the TreeView
	model = Gtk::TreeStore::create(columns);
	treeview->set_headers_visible(false);
	treeview->set_model(model);
	treeview->append_column("", columns.name);

	//setup the path_enumerate as default widget
	//setup the path_entry
	path_enumerate->set_can_default(true);
	set_default(*path_enumerate);
	path_entry->set_activates_default(true);

	//connect the signals
	path_enumerate->signal_clicked().connect(
			sigc::mem_fun(this, &EmapContainer::on_enumerate_clicked));
	set_root_folder_button->signal_clicked().connect(
			sigc::mem_fun(this, &EmapContainer::on_button_clicked));

	quit_button->signal_clicked().connect(
			sigc::mem_fun(this, &EmapContainer::on_button_quit));

	//add the path_* widgets to the path_container
	path_container->attach(*path_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL,
			Gtk::SHRINK);
	path_container->attach(*path_entry, 1, 2, 0, 1, Gtk::FILL | Gtk::EXPAND,
			Gtk::SHRINK);
	path_container->attach(*path_enumerate, 2, 3, 0, 1, Gtk::SHRINK | Gtk::FILL,
			Gtk::SHRINK);

	path_container->attach(*set_root_folder_button, 3, 4, 0, 1,
			Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
	path_container->attach(*quit_button, 4, 5, 0, 1, Gtk::SHRINK | Gtk::FILL,
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

	loadTree(homedir);

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

void EmapContainer::on_enumerate_clicked() {
	Glib::ustring path = path_entry->get_text();
	Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
	Glib::RefPtr<Gio::FileEnumerator> child_enumeration =
			file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);

	std::vector<Glib::ustring> file_names;
	Glib::RefPtr<Gio::FileInfo> file_info;
	while ((file_info = child_enumeration->next_file()) != NULL) {
		file_names.push_back(file_info->get_name());
	}

	//clear the TreeView
	model->clear();
	//now populate the TreeView
	Gtk::TreeModel::Row row = *(model->append());

	for (unsigned int i = 0; i < file_names.size(); i++) {
		row[columns.name] = file_names[i];
		//avoid appending a last empty row
		if (i != file_names.size() - 1)
			row = *(model->append());
	}

}

void EmapContainer::on_button_clicked() {
	std::cout << "Set Root Folder" << std::endl;
	Gtk::FileChooserDialog dialog("Please choose a folder",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dialog.set_transient_for((Gtk::Window&) (*this->get_toplevel()));

	std::cout << "homedir: " << homedir << std::endl;

	dialog.set_current_folder(homedir);

	//Add response buttons the the dialog:
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	//file_tree_view.unset_model();
	//std::cout << "unset model" << std::endl;
	///	file_tree_view.set_model();
	//	std::cout << "set model" << std::endl;
	int result = dialog.run();

	//Handle the response:
	switch (result) {
	case (Gtk::RESPONSE_OK): {
		std::cout << "Select clicked." << std::endl;
		std::cout << "Folder selected: " << dialog.get_filename() << std::endl;

		homedir = dialog.get_filename().c_str();

		loadTree(homedir);

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

void EmapContainer::loadTree(const char* path) {
	model->clear();

	DIR* dirFile = opendir(path);

	if (dirFile) {
		struct dirent* hFile;
		bool gIgnoreHidden = true;

		while ((hFile = readdir(dirFile)) != NULL) {
			std::string filename = hFile->d_name;

			std::cout << "filename: " << filename << std::endl;

			//ignore current directory and parent directory names.
			if (!strcmp(hFile->d_name, "."))
				continue;
			if (!strcmp(hFile->d_name, ".."))
				continue;

			// in linux hidden files all start with '.'
			if (gIgnoreHidden && (hFile->d_name[0] == '.'))
				continue;

			//compare the data as lowercase
			std::transform(filename.begin(), filename.end(), filename.begin(),
					::tolower);

			// dirFile.name is the name of the file. Do whatever string comparison
			// you want here. Something like:
			if (strstr(filename.c_str(), ".sf2")) {
				std::cout << "found a sound font file: " << hFile->d_name
						<< std::endl;
			}

			//now populate the TreeView
			Gtk::TreeModel::Row row = *(model->append());

			//row[m_Columns.m_col_id] = 1;
			row[columns.name] = filename;

			std::cout << "appended: " << filename << std::endl;

		}

		closedir(dirFile);
		//set_model();
		//std::cout << "this: " << this->get_window() << std::endl;
	}

}
