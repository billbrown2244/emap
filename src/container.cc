/*
 * container.cc
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */


#include "container.h"

EmapContainer::EmapContainer()

{
  set_title("EMAP - Easy Midi Audio Production");
  add(m_grid);

  m_grid.add(sound_file_root);

  m_grid.attach_next_to(file_tree_view,sound_file_root , Gtk::POS_BOTTOM, 1, 1);

  sound_file_root.set_hexpand(true);
  file_tree_view.set_hexpand(true);
  file_tree_view.set_vexpand(true);

  add_events(Gdk::KEY_PRESS_MASK);

  show_all_children();
}

EmapContainer::~EmapContainer()
{
}

