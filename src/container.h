/*
 * container.h
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include <gtkmm.h>
class EmapContainer: public Gtk::Window {
public:
	EmapContainer();
	~EmapContainer();

protected:

	class ModelColumns: public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(name);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	Gtk::Table *container, *path_container;
	Gtk::ScrolledWindow *scrolled;
	Gtk::Label *path_label;
	Gtk::Entry *path_entry;
	Gtk::Button *path_enumerate;
	Gtk::TreeView *treeview;
	Glib::RefPtr<Gtk::TreeStore> model;
	ModelColumns columns;
	Gtk::Button *set_root_folder_button, *quit_button;
	const char *homedir;

	void on_enumerate_clicked();
	//Signal handlers:
	void on_button_clicked();
	void on_button_quit();

	void loadTree(const char* path);

};



#endif /* CONTAINER_H_ */
