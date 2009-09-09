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

#ifndef GPLATES_QTWIDGETS_PROJECTIONCONTROLWIDGET_H
#define GPLATES_QTWIDGETS_PROJECTIONCONTROLWIDGET_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QWidget>
#include "ProjectionControlWidgetUi.h"

#include "gui/MapProjection.h"


namespace GPlatesViewOperations
{
	class ViewportProjection;
}

namespace GPlatesQtWidgets
{
	/**
	 * Small widget with combobox, to let the user switch projections.
	 */
	class ProjectionControlWidget:
			public QWidget,
			protected Ui_ProjectionControlWidget
	{
		Q_OBJECT

	public:
		explicit
		ProjectionControlWidget(
				GPlatesViewOperations::ViewportProjection &viewport_projection,
				QWidget *parent_);

	private slots:
		
		void
		handle_combobox_changed(
				int idx);
		
	public slots:
		void
		handle_projection_type_changed(
				const GPlatesViewOperations::ViewportProjection &);
				
		void
		show_label(
			bool show_);				

	private:

		GPlatesViewOperations::ViewportProjection &d_viewport_projection;
		
	};
}

#endif  // GPLATES_QTWIDGETS_PROJECTIONCONTROLWIDGET_H
