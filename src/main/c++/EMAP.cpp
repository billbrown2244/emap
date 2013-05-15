//============================================================================
// Name        : EMAP.cpp
// Author      : Bill Brown
// Version     :
// Copyright   : Apache 2
// Description : Easy Midi Audio Production
//============================================================================

#include "container.h"
#include <gtkmm/application.h>
#include <fluidsynth.h>
#include <stdio.h>
#include <iostream>
#include "fsynth.h"

int main(int argc, char *argv[]) {
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv,
			"org.gtkmm.examples.base");

	//try {

		//Connect front end and synth
		FSynth fsynth;

		EmapContainer emap(fsynth.get_synth());

		//Shows the window and returns when it is closed.
		return app->run(emap);

	//} catch (std::exception& e) {
		//std::cout << "Standard exception: " << e.what() << std::endl;
	//}
}

