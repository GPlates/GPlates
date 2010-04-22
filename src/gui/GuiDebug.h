/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_GUIDEBUG_H
#define GPLATES_GUI_GUIDEBUG_H

#include <QObject>
#include <QAction>


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesAppLogic
{
	class ApplicationState;
}


namespace GPlatesGui
{
	/**
	 * This GUI class creates a 'Debug' menu that developers can use to
	 * assist them in debugging GUI problems and testing code that does not
	 * yet have a working UI.
	 *
	 * It is instantiated from ViewportWindow::install_gui_debug_menu(),
	 * in response to the commandline switch '--debug-gui'.
	 */
	class GuiDebug: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		GuiDebug(
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesAppLogic::ApplicationState &app_state_,
				QObject *parent_);

		virtual
		~GuiDebug()
		{  }

	public slots:


	private slots:

		/**
		 * Respond to the all-purpose 'Debug Action' hotkey, Ctrl-Alt-/
		 */
		void
		handle_gui_debug_action();

		/**
		 * For testing Unsaved Changes functionality.
		 */
		void
		debug_set_all_files_clean();

	private:

		/**
		 * Adds menus and connects to actions, etc.
		 */
		void
		create_menu();

		/**
		 * Finds child of ViewportWindow with given objectName dynamically,
		 * by traversing the widget hierarchy. Relies on everything being
		 * properly parented to everything else.
		 *
		 * Returns NULL pointer on failure.
		 */
		QObject *
		find_child_qobject(
				QString name);


		/**
		 * Pointer to the ViewportWindow so we can access all manner of things.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * Pointer to the ApplicationState so we can access all manner of things.
		 */
		GPlatesAppLogic::ApplicationState *d_app_state_ptr;
	};
}


#endif	// GPLATES_GUI_GUIDEBUG_H
