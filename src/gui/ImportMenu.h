/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_IMPORTMENU_H
#define GPLATES_GUI_IMPORTMENU_H

#include <boost/function.hpp>
#include <QObject>
#include <QMenu>
#include <QActionGroup>
#include <QString>


namespace GPlatesGui
{
	/**
	 * This class encapsulates operations on the Import submenu on the File menu.
	 * This submenu is hidden by default and is only visible upon the registration
	 * of one or more import types.
	 */
	class ImportMenu :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * An enumeration of different sections of the Import menu.
		 * Sections are visually separated by a separator.
		 * New sections can be added by simply adding new entries here.
		 */
		enum Section
		{
			BUILT_IN,
			RASTER,
			SCALAR_FIELD_3D,

			NUM_SECTIONS // Must be the last entry.
		};

		/**
		 * Constructs an @a ImportMenu, given pointers to the @a import_menu and
		 * its @a parent_menu as set up in the @a ViewportWindow designer.
		 */
		explicit
		ImportMenu(
				QMenu *import_menu,
				QMenu *parent_menu,
				QObject *parent_);

		/**
		 * Adds an item to the given @a section of the Import menu, with the given
		 * @a text. When the menu item is triggered, the @a callback is activated.
		 */
		void
		add_import(
				Section section,
				const QString &text,
				const boost::function< void () > &callback);

	private Q_SLOTS:

		void
		handle_action_triggered(
				QAction *action);

	private:

		QMenu *d_import_menu;
		QMenu *d_parent_menu;

		/**
		 * Stores the action following the Import menu in its parent menu, so that we
		 * know where to reinsert it after we remove it from its parent menu.
		 * This variable is set to NULL after it has been reinserted.
		 */
		QAction *d_next_action_in_parent_menu;

		QActionGroup *d_action_group;

		/**
		 * An array of actions that are one past the end of the actions for a given
		 * section. These actions are all separators, with the exception of the last
		 * entry in the array, which is NULL.
		 */
		QAction *d_section_end_actions[NUM_SECTIONS];
	};
}


#endif	// GPLATES_GUI_IMPORTMENU_H
