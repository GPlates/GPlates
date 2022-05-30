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

#include <QKeySequence>
#include <QString>
#include <QWidget>

#include "ui_ProjectionControlWidgetUi.h"


namespace GPlatesGui
{
	class Projection;
}

namespace GPlatesQtWidgets
{
	/**
	 * Small widget with combobox, to let the user switch projections
	 * (globe/map projection and viewport projection).
	 */
	class ProjectionControlWidget:
			public QWidget,
			protected Ui_ProjectionControlWidget
	{
		Q_OBJECT

	public:
		explicit
		ProjectionControlWidget(
				GPlatesGui::Projection &projection,
				QWidget *parent_);

	private Q_SLOTS:
		
		void
		handle_globe_map_projection_combo_changed(
				int idx);

		void
		handle_globe_map_projection_shortcut_triggered();
		
		void
		handle_viewport_projection_combo_changed(
				int idx);

	public Q_SLOTS:
		void
		handle_globe_map_projection_changed(
				const GPlatesGui::Projection &);

		void
		handle_viewport_projection_changed(
				const GPlatesGui::Projection &);

	private:

		GPlatesGui::Projection &d_projection;


		void
		add_globe_map_projection(
				const QString &projection_text,
				unsigned int projection_index);
	};
}

#endif  // GPLATES_QTWIDGETS_PROJECTIONCONTROLWIDGET_H
