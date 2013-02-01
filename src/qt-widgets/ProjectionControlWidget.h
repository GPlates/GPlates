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

#include "ProjectionControlWidgetUi.h"

#include "gui/MapProjection.h"


namespace GPlatesGui
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
				GPlatesGui::ViewportProjection &viewport_projection,
				QWidget *parent_);

	private Q_SLOTS:
		
		void
		handle_combobox_changed(
				int idx);

		void
		handle_shortcut_triggered();

	public Q_SLOTS:
		void
		handle_projection_type_changed(
				const GPlatesGui::ViewportProjection &);
				
		void
		show_label(
			bool show_);				

	private:

		GPlatesGui::ViewportProjection &d_viewport_projection;


		void
		add_projection(
				const QString &projection_text,
				GPlatesGui::MapProjection::Type projection_type,
				const QString &shortcut_key_sequence);

	};
}

#endif  // GPLATES_QTWIDGETS_PROJECTIONCONTROLWIDGET_H
