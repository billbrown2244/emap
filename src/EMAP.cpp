//============================================================================
// Name        : EMAP.cpp
// Author      : Bill Brown
// Version     :
// Copyright   : Apache 2
// Description : Easy Midi Audio Production
//============================================================================

#include "container.h"
#include <gtkmm/application.h>

int main(int argc, char *argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv,
			"org.gtkmm.examples.base");

	//SoundFileRoot SoundFileRoot;
	EmapContainer emap;

	//Shows the window and returns when it is closed.
	return app->run(emap);
}
