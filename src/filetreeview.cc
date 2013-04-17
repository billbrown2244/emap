#include <iostream>
#include "filetreeview.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <algorithm>


FileTreeView::FileTreeView()
{

	set_orientation(Gtk::ORIENTATION_VERTICAL);

  //Add the TreeView, inside a ScrolledWindow, with the button underneath:
  m_ScrolledWindow.add(m_TreeView);

  //Only show the scrollbars when they are necessary:
  m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);



  pack_start(m_ScrolledWindow);



  //Create the Tree model:
  m_refTreeModel = Gtk::TreeStore::create(m_Columns);
  m_TreeView.set_model(m_refTreeModel);

  //All the items to be reordered with drag-and-drop:
  m_TreeView.set_reorderable();

  //Fill the TreeView's model


struct passwd *pw = getpwuid(getuid());

const char *homedir = pw->pw_dir;


	m_refTreeModel = this->listFiles(homedir,m_refTreeModel);

  Gtk::TreeModel::Row row = *(m_refTreeModel->append());
  row[m_Columns.m_col_id] = 1;
  row[m_Columns.m_col_name] = "Billy Bob";

  Gtk::TreeModel::Row childrow = *(m_refTreeModel->append(row.children()));
  childrow[m_Columns.m_col_id] = 11;
  childrow[m_Columns.m_col_name] = "Billy Bob Junior";

  childrow = *(m_refTreeModel->append(row.children()));
  childrow[m_Columns.m_col_id] = 12;
  childrow[m_Columns.m_col_name] = "Sue Bob";

  row = *(m_refTreeModel->append());
  row[m_Columns.m_col_id] = 2;
  row[m_Columns.m_col_name] = "Joey Jojo";


  row = *(m_refTreeModel->append());
  row[m_Columns.m_col_id] = 3;
  row[m_Columns.m_col_name] = "Rob McRoberts";

  childrow = *(m_refTreeModel->append(row.children()));
  childrow[m_Columns.m_col_id] = 31;
  childrow[m_Columns.m_col_name] = "Xavier McRoberts";

  //Add the TreeView's view columns:
  m_TreeView.append_column("ID", m_Columns.m_col_id);
  m_TreeView.append_column("Name", m_Columns.m_col_name);

  //Connect signal:signal_row_activated
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
      m_TreeView.get_selection();

  refTreeSelection->signal_changed().connect(
		    sigc::mem_fun(*this, &FileTreeView::on_selection_changed)
		);



}

FileTreeView::~FileTreeView()
{
}


void FileTreeView::on_selection_changed()
{


	Gtk::TreeModel::iterator iter = m_TreeView.get_selection()->get_selected();
 if(iter) //If anything is selected
 {
	 Gtk::TreeModel::Row row = *iter;
   const Glib::ustring filename = row[m_Columns.m_col_name];
       const Glib::ustring description = row[m_Columns.m_col_name];

     std::cout  << "Selection Changed to: filename="
         << filename
         << ", description="
         << description
         << std::endl;
 }


}

Glib::RefPtr<Gtk::TreeStore> FileTreeView::listFiles(const char* path, Glib::RefPtr<Gtk::TreeStore> tree_model) {

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

			Gtk::TreeModel::Row row = *(tree_model->append());
			 row[m_Columns.m_col_id] = 1;
			  row[m_Columns.m_col_name] = filename;


		}
		closedir(dirFile);
	}

	return tree_model;
}

bool FileTreeView::on_key_press_event(GdkEventKey* event)
{
  //GDK_MOD1_MASK -> the 'alt' key(mask)
  //GDK_KEY_1 -> the '1' key
  //GDK_KEY_2 -> the '2' key
	std::cout  << "key pressed"
	         << std::endl;


/*
  //select the first radio button, when we press alt + 1
  if((event->keyval == GDK_KEY_1) &&
    (event->state &(GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == GDK_MOD1_MASK)
  {
    m_first.set_active();
    //returning true, cancels the propagation of the event
    return true;
  }
  else if((event->keyval == GDK_KEY_2) &&
    (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) == GDK_MOD1_MASK)
  {
    //and the second radio button, when we press alt + 2
    m_second.set_active();
    return true;
  }
  else if(event->keyval == GDK_KEY_Escape)
  {
    //close the window, when the 'esc' key is pressed
    hide();
    return true;
  }
*/
  //if the event has not been handled, call the base class
  //return Gtk::Window::on_key_press_event(event);
	return true;
}

