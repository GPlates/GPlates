/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_ACTIONBUTTONBOX_H
#define GPLATES_QTWIDGETS_ACTIONBUTTONBOX_H

#include <QWidget>
#include <QGridLayout>


namespace GPlatesQtWidgets
{
	/**
	 * A lightweight reusable box of QActions, each triggered by a
	 * QToolButton and automatically laid out in a grid arrangement.
	 */
	class ActionButtonBox:
			public QWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ActionButtonBox(
				int num_columns,
				int default_icon_size,
				QWidget *parent_ = NULL);
		
		/**
		 * Adds a new QToolButton linked to the given @a action_ptr .
		 *
		 * Note that neither the ActionButtonBox nor the QToolButton
		 * will take ownership of the QAction.
		 */
		void
		add_action(
				QAction *action_ptr);
	
	private:
		
		/**
		 * Adjusts @a d_next_row and @a d_next_col to point to the next
		 * grid cell in the sequence.
		 */
		void
		next_cell();
		
		/**
		 * How many columns should be used for the grid layout.
		 */
		int d_num_columns;
		
		/**
		 * The default width and height of icons for the QToolButtons,
		 * in pixels.
		 */
		int d_default_icon_size;
		
		/**
		 * The layout for this ActionButtonBox.
		 * Memory is managed by Qt.
		 */
		QGridLayout *d_layout_ptr;
		
		/**
		 * The next empty grid cell's row number.
		 */
		int d_next_row;

		/**
		 * The next empty grid cell's column number.
		 */
		int d_next_col;
		
	};
}

#endif  // GPLATES_QTWIDGETS_ACTIONBUTTONBOX_H
