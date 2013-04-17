/*
 * container.h
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include "filetreeview.h"
#include "soundfileroot.h"
#include <gtkmm.h>

class EmapContainer : public Gtk::Window
{
public:
	EmapContainer();
  virtual ~EmapContainer();

private:
  // Signal handlers:
  void on_button_quit();
  void on_button_numbered(const Glib::ustring& data);

  // Child widgets:
  Gtk::Grid m_grid;
  SoundFileRoot sound_file_root;
  FileTreeView file_tree_view;

};


#endif /* CONTAINER_H_ */
