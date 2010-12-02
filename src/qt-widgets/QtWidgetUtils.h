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
 
#ifndef GPLATES_QTWIDGETS_QTWIDGETUTILS_H
#define GPLATES_QTWIDGETS_QTWIDGETUTILS_H

#include <boost/optional.hpp>
#include <QWidget>
#include <QDialog>
#include <QColorDialog>

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
	}
}

#endif	// GPLATES_QTWIDGETS_QTWIDGETUTILS_H
