/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#ifndef GPLATES_GUI_FULLSCREENMODE_H
#define GPLATES_GUI_FULLSCREENMODE_H

#include <QObject>
#include <QWidget>
#include <QAction>


namespace GPlatesQtWidgets
{
	// Forward declaration of ViewportWindow to avoid spaghetti.
	// Yes, this is ViewportWindow, not the "View State"; we need
	// this to make it full screen, and to look up various child
	// widgets.
	class ViewportWindow;
}

namespace GPlatesGui
{
	/**
	 * This GUI class encapsulates the ability for GPlates to make the main window
	 * into a full-screen window without decorations, suitable for presentations
	 * and the like.
	 */
	class FullScreenMode: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		FullScreenMode(
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				QObject *parent_ = NULL);

		virtual
		~FullScreenMode()
		{  }


		/**
		 * Connects buttons, adds menus, etc. This step must be done @em after
		 * ViewportWindow::setupUi() has been called, and therefore cannot
		 * be done in FullScreenMode's constructor.
		 */
		void
		init();

	
	public Q_SLOTS:

		void
		leave_full_screen();
		
		void
		toggle_full_screen(
				bool wants_full_screen);


	private:

		/**
		 * Quick method to get at the ViewportWindow from inside this class.
		 * 
		 * You'll see why I've done it this way when you see the other GUI
		 * element accessors below; they use findChild() to locate child
		 * widgets dynamically.
		 */
		GPlatesQtWidgets::ViewportWindow &
		viewport_window()
		{
			return *d_viewport_window_ptr;
		}
		
		/**
		 * Quick method to get at the GMenuButton from inside this class.
		 * 
		 * Saves us passing pointers around, and saves us the (admittedly,
		 * trivial) cost of looking up the GMenu by Qt objectName each time.
		 * We can also keep the null-pointer check in here. Not having
		 * access to this widget is a pretty serious error for Full Screen
		 * Mode. findChild() will return null if setupUi() hasn't been called
		 * yet or there is some other UI disaster.
		 */
		QWidget &
		gmenu_button();

		/**
		 * Quick method to get at the LeaveFullScreenButton from inside this class.
		 * 
		 * Saves us passing pointers around, and saves us the (admittedly,
		 * trivial) cost of looking up the button by Qt objectName each time.
		 * We can also keep the null-pointer check in here. Not having
		 * access to this widget is a pretty serious error for Full Screen
		 * Mode. findChild() will return null if setupUi() hasn't been called
		 * yet or there is some other UI disaster.
		 */
		QWidget &
		leave_full_screen_button();

		/**
		 * Quick method to get at the ReconstructionViewWidget from inside this class.
		 * 
		 * Saves us passing pointers around, and saves us the (admittedly,
		 * trivial) cost of looking up the widget by Qt objectName each time.
		 * We can also keep the null-pointer check in here. Not having
		 * access to this widget is a pretty serious error for Full Screen
		 * Mode. findChild() will return null if setupUi() hasn't been called
		 * yet or there is some other UI disaster.
		 */
		QWidget &
		reconstruction_view_widget();

		/**
		 * Quick method to get at the Full Screen QAction from inside this class.
		 * 
		 * Saves us passing pointers around, and saves us the (admittedly,
		 * trivial) cost of looking up the action by Qt objectName each time.
		 * We can also keep the null-pointer check in here. Not having
		 * access to this action is a pretty serious error for Full Screen
		 * Mode. findChild() will return null if setupUi() hasn't been called
		 * yet or there is some other UI disaster.
		 */
		QAction &
		full_screen_action();


		/**
		 * Pointer to the window we should be full-screening.
		 * This is also used to locate sub-widgets by Qt objectName.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * Main window's state, serialised by Qt's saveState() method.
		 * This should hopefully aid the return to windowed mode on some platforms
		 * (e.g. remembering Maximised state on win32/OSX, Dock/Toolbar state).
		 */
		QByteArray d_viewport_state;
	};
}


#endif	// GPLATES_GUI_FULLSCREENMODE_H
