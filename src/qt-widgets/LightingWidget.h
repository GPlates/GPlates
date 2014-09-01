/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_LIGHTINGWIDGET_H
#define GPLATES_QT_WIDGETS_LIGHTINGWIDGET_H

#include <QWidget>

#include "LightingWidgetUi.h"
#include "TaskPanelWidget.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;
	class ViewportWindow;

	class LightingWidget :
			public TaskPanelWidget, 
			protected Ui_LightingWidget
	{
		Q_OBJECT

	public:

		explicit
		LightingWidget(
				ViewportWindow &viewport_window_,
				QWidget *parent_ = NULL);

		~LightingWidget();

		virtual
		void
		handle_activation();

	private Q_SLOTS:

		void
		react_enable_lighting_geometry_on_sphere_check_box_changed();

		void
		react_enable_lighting_filled_geometry_on_sphere_check_box_changed();

		void
		react_enable_lighting_arrow_check_box_changed();

		void
		react_enable_lighting_raster_check_box_changed();

		void
		react_enable_lighting_scalar_field_check_box_changed();

		void
		react_ambient_lighting_spinbox_changed(
				double value);

		void
		react_light_direction_attached_to_view_frame_check_box_changed();

	private:

		GPlatesPresentation::ViewState &d_view_state;

		GlobeAndMapWidget &d_globe_and_map_widget;

		void
		apply_lighting();

		void
		make_signal_slot_connections();
	};
}

#endif // GPLATES_QT_WIDGETS_LIGHTINGWIDGET_H
