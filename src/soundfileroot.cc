/*
 * SoundFileRoot.cc
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#include "soundfileroot.h"
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>


SoundFileRoot::SoundFileRoot() :
		m_button("Sound File Root") // creates a new button with label "Hello World".
				, m_Button_Quit("Quit")
	{

	Gtk::ButtonBox* bbox = 0;

	bbox = Gtk::manage(new Gtk::ButtonBox(Gtk::ORIENTATION_HORIZONTAL));
	bbox->set_spacing(0);
	bbox->set_hexpand(true);

	//bbox->set_border_width(5);

	add(*bbox);

	// When the button receives the "clicked" signal, it will call the
	// on_button_clicked() method defined below.
	m_button.signal_clicked().connect(
			sigc::mem_fun(*this, &SoundFileRoot::on_button_clicked));

	m_Button_Quit.signal_clicked().connect(
			sigc::mem_fun(*this, &SoundFileRoot::on_button_quit));

	// This packs the button into the Window (a container).
	bbox->add(m_button);
	bbox->add(m_Button_Quit);

	m_button.set_hexpand(true);
	m_Button_Quit.set_hexpand(true);

}

SoundFileRoot::~SoundFileRoot() {
}

void SoundFileRoot::on_button_clicked() {
	std::cout << "Set Root Folder" << std::endl;
	Gtk::FileChooserDialog dialog("Please choose a folder",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dialog.set_transient_for((Gtk::Window&) (*this->get_toplevel()));

	//Add response buttons the the dialog:
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result = dialog.run();

	//Handle the response:
	switch (result) {
	case (Gtk::RESPONSE_OK): {
		std::cout << "Select clicked." << std::endl;
		std::cout << "Folder selected: " << dialog.get_filename() << std::endl;
		SoundFileRoot::listFiles((dialog.get_filename()).c_str());

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

void SoundFileRoot::on_button_quit() {
	std::cout << "Quitting" << std::endl;
	exit(0);
}

void SoundFileRoot::listFiles(const char* path) {
	DIR* dirFile = opendir(path);

	if (dirFile) {
		struct dirent* hFile;
		bool gIgnoreHidden = true;
		while ((hFile = readdir(dirFile)) != NULL) {
			std::string filename = hFile->d_name;


			//ignore current directory and parent directory names.
			if (!strcmp(hFile->d_name, "."))
				continue;
			if (!strcmp(hFile->d_name, ".."))
				continue;

			// in linux hidden files all start with '.'
			if (gIgnoreHidden && (hFile->d_name[0] == '.'))
				continue;

			//compare the data as lowercase
			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

			// dirFile.name is the name of the file. Do whatever string comparison
			// you want here. Something like:
			if (strstr(filename.c_str(), ".sf2")) {
				std::cout << "found a sound font file: " << hFile->d_name << std::endl;
			}
		}
		closedir(dirFile);
	}
}

