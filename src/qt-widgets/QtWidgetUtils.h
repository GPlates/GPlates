/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_QTWIDGETUTILS_H
#define GPLATES_QTWIDGETS_QTWIDGETUTILS_H

#include <boost/optional.hpp>
#include <QWidget>
#include <QDialog>
#include <QColorDialog>
#include <QKeyEvent>
#include <QPixmap>

#include "gui/Colour.h"


namespace GPlatesQtWidgets
{
	namespace QtWidgetUtils
	{
		/**
		 * Inserts @a widget into @a placeholder such that @a widget fills up the
		 * entirety of @a placeholder.
		 */
		void
		add_widget_to_placeholder(
				QWidget *widget,
				QWidget *placeholder);

		/**
		 * Repositions @a dialog to the side of its parent.
		 */
		void
		reposition_to_side_of_parent(
				QDialog *dialog);

		/**
		 * Shows @a dialog if currently hidden, ensures that it is active and also
		 * ensures that it is on top of its parent.
		 */
		void
		pop_up_dialog(
				QWidget *dialog);

		/**
		 * Sets the height of @a dialog to that of its @a sizeHint(), and ensures
		 * that the width of @a dialog is at least that of its @a sizeHint().
		 *
		 * This is useful for making sure that dialogs with fixed height contents
		 * (i.e. dialogs without a vertically-expanding widget in the middle) have a
		 * reasonable height regardless of platform. Note that for this function to
		 * work correctly, any vertical spacers must have a sizeHint() height of 0
		 * (see for example, @a SetProjectionDialog). The width is also changed
		 * if the dialog's set width is not wide enough to accommodate its
		 * contents on a particular platform.
		 */
		void
		resize_based_on_size_hint(
				QDialog *dialog);

		/**
		 * Retrieves a colour using a standard dialog box. Returns boost::none if the
		 * user clicked cancel.
		 *
		 * This method exists because Qt 4.5 revamped QColorDialog. The implementation
		 * of this function calls the correct methods on QColorDialog depending on the
		 * version of Qt.
		 *
		 * In addition, note that this method uses GPlatesGui::Colour.
		 */
		boost::optional<GPlatesGui::Colour>
		get_colour_with_alpha(
				const GPlatesGui::Colour &initial,
				QWidget *parent);

		/**
		 * Returns true if the @a key_event represents Ctrl+C on Windows and
		 * Linux, and control+C (not command+C) on the Mac.
		 */
		bool
		is_control_c(
				QKeyEvent *key_event);


		/**
		 * Returns a checkboard typically used as the background of a semi-transparent
		 * image, with the given @a width, @a height and @a grid_size.
		 */
		QPixmap
		create_transparent_checkerboard(
				int width,
				int height,
				int grid_size);
	}
}

#endif	// GPLATES_QTWIDGETS_QTWIDGETUTILS_H
