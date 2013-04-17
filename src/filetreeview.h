/*
 * filetreeview.h
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#ifndef FILETREEVIEW_H_
#define FILETREEVIEW_H_

#include <gtkmm.h>

class FileTreeView : public Gtk::Box
{
public:
  FileTreeView();
  virtual ~FileTreeView();

protected:

  void on_selection_changed();
  Glib::RefPtr<Gtk::TreeStore> listFiles(const char* path, Glib::RefPtr<Gtk::TreeStore> tree_model);
  bool on_key_press_event(GdkEventKey* event);

  //Tree model columns:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:

      ModelColumns()
      { add(m_col_id); add(m_col_name); }

      Gtk::TreeModelColumn<int> m_col_id;
      Gtk::TreeModelColumn<Glib::ustring> m_col_name;
    };

    ModelColumns m_Columns;


    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::TreeView m_TreeView;
    Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;

};


#endif /* FILETREEVIEW_H_ */
