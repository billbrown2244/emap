/*
 * FileTreeModel.h
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#ifndef FILETREEMODEL_H_
#define FILETREEMODEL_H_

#include <gtkmm/treemodel.h>

class FileTreeModel: public Gtk::TreeModel {
public:
	FileTreeModel();
	virtual ~FileTreeModel();
};

#endif /* FILETREEMODEL_H_ */
