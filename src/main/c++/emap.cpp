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
 */
//#include <gtkmm/application.h>
#include <stdio.h>
#include <iostream>
#include "fsynth.h"
#include "container.h"

int main(int argc, char *argv[]) {
//	Glib::RefPtr < Gtk::Application > app = Gtk::Application::create(argc, argv,
//			"org.gtkmm.examples.base");

	//Connect the UI and the fluidsynth
	FSynth fsynth(false, 0);
	EmapContainer emap(fsynth.get_synth(), false);

	gtk_main();
	return 0;

	std::cout << "Started EMAP: " << std::endl;
	//return app->run(emap);

}

