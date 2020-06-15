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
 
#ifndef GPLATES_QTWIDGETS_GMENUBUTTON_H
#define GPLATES_QTWIDGETS_GMENUBUTTON_H

#include <QWidget>
#include <QMenu>
#include "ui_GMenuButtonUi.h"


namespace GPlatesQtWidgets
{
	// Forward declaration of ViewportWindow to avoid spaghetti.
	// Yes, this is ViewportWindow, not the "View State"; we need
	// access to the menu bar.
	class ViewportWindow;

	/**
	 * This button appears in the main window during full-screen mode.
	 * It provides the user with an alternative means of accessing the
	 * main menu, and also ensures that menu-based keyboard shortcuts
	 * will still function even if we choose to hide the main menubar.
	 */
	class GMenuButton: 
			public QWidget,
			protected Ui_GMenuButton
	{
		Q_OBJECT
		
	public:
	
		explicit
		GMenuButton(
				GPlatesQtWidgets::ViewportWindow &main_window,
				QWidget *parent_);

		virtual
		~GMenuButton()
		{  }


	private:
		/**
		 * This is the menu that pops up when you click the GMenuButton.
		 * It contains a copy of the top-level menus from the main menu bar.
		 *
		 * As with most Qt things, it is a QObject parented to this widget.
		 */
		QMenu *d_menu_ptr;

	};
}


#endif	// GPLATES_QTWIDGETS_GMENUBUTTON_H
