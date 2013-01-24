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
 
#ifndef GPLATES_GUI_TRINKETAREA_H
#define GPLATES_GUI_TRINKETAREA_H

#include <QObject>
#include <QWidget>
#include <QStatusBar>

#include "qt-widgets/TrinketIcon.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}


namespace GPlatesGui
{
	class Dialogs;

	/**
	 * This GUI class manages the icons displayed in the QStatusBar of the ViewportWindow.
	 */
	class TrinketArea: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		TrinketArea(
				Dialogs &dialogs,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				QObject *parent_ = NULL);

		virtual
		~TrinketArea()
		{  }


		/**
		 * Connects buttons, adds menus, etc. This step must be done @em after
		 * ViewportWindow::setupUi() has been called, because it relies on UI
		 * elements that do not exist until that time, and therefore cannot
		 * be done in TrinketArea's constructor.
		 */
		void
		init();


		/**
		 * Accessor to enable outside control of the "You Have Unsaved Changes" TrinketIcon.
		 */
		GPlatesQtWidgets::TrinketIcon &
		unsaved_changes_trinket()
		{
			return *d_trinket_unsaved;
		}

		/**
		 * Accessor to enable outside control of the "You Have Read Errors" TrinketIcon.
		 */
		GPlatesQtWidgets::TrinketIcon &
		read_errors_trinket()
		{
			return *d_trinket_read_errors;
		}

	public Q_SLOTS:


	private Q_SLOTS:

		void
		react_icon_clicked(
				GPlatesQtWidgets::TrinketIcon *icon,
				QMouseEvent *ev);

	private:

		/**
		 * Quick way to access the ViewportWindow's status bar.
		 * Remember, this won't be valid until after ViewportWindow::setupUi() has
		 * been called.
		 */
		QStatusBar &
		status_bar();


		/**
		 * Pointer to the ViewportWindow so we can access the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * Pointer to the "You have unsaved changes" TrinketIcon.
		 * The icon is parented to the status bar as soon as it is made, and the
		 * memory is managed by Qt.
		 */
		GPlatesQtWidgets::TrinketIcon *d_trinket_unsaved;

		/**
		 * Pointer to the "You loaded some files with read errors" TrinketIcon.
		 * The icon is parented to the status bar as soon as it is made, and the
		 * memory is managed by Qt.
		 */
		GPlatesQtWidgets::TrinketIcon *d_trinket_read_errors;

	};
}


#endif	// GPLATES_GUI_TRINKETAREA_H
