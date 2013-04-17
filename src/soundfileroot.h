/*
 * SoundFileRoot.h
 *
 *  Created on: Apr 16, 2013
 *      Author: bill
 */

#ifndef SoundFileRoot_H_
#define SoundFileRoot_H_

#include <gtkmm.h>


class SoundFileRoot : public Gtk::ButtonBox
{

public:
  SoundFileRoot();
  virtual ~SoundFileRoot();

protected:
  //Signal handlers:
  void on_button_clicked();
  void on_button_quit();
  void listFiles( const char* path );


  Gtk::Button m_button, m_Button_Quit;
};


#endif /* SoundFileRoot_H_ */
