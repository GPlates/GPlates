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

#include <QWidget>
#include <QHBoxLayout>

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
	}
}

#endif	// GPLATES_QTWIDGETS_QTWIDGETUTILS_H
