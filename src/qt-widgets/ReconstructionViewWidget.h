/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H
#define GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H

#include <memory>
#include <QByteArray>
#include <QGraphicsScene>
#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QWidget>
#include <boost/optional.hpp>

#include "GMenuButton.h"
#include "ReconstructionViewWidgetUi.h"
#include "ZoomSliderWidget.h"
#include "gui/ViewportZoom.h"
#include "maths/LatLonPoint.h"

namespace GPlatesMaths
{
	class PointOnSphere;
	class Rotation;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class AnimationController;
	class ViewportProjection;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;
	class GlobeCanvas;
	class AnimateControlWidget;
	class ZoomControlWidget;
	class TimeControlWidget;
	class MapCanvas;
	class MapView;
	class ProjectionControlWidget;
	class SceneView;
	class TaskPanel;
	class ViewportWindow;

	class ReconstructionViewWidget:
			public QWidget, 
			protected Ui_ReconstructionViewWidget
	{
		Q_OBJECT
		
	public:

		ReconstructionViewWidget(
				ViewportWindow &viewport_window,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		enum WidgetIndex
		{
			CANVAS_WIDGET_INDEX = 0,
			TASK_PANEL_INDEX
		};

		/**
		 * The Task Panel is created, and initialised by ViewportWindow.
		 * ViewportWindow uses this method to insert the panel into the
		 * ReconstructionViewWidget's layout. This is done mainly to reduce
		 * dependencies in ViewportWindow's initialiser list.
		 */
		void
		insert_task_panel(
				GPlatesQtWidgets::TaskPanel *task_panel);

		GlobeCanvas &
		globe_canvas();

		const GlobeCanvas &
		globe_canvas() const;

		MapView &
		map_view();

		const MapView &
		map_view() const;

		SceneView &
		active_view();

		const SceneView &
		active_view() const;

		GlobeAndMapWidget &
		globe_and_map_widget();

		const GlobeAndMapWidget &
		globe_and_map_widget() const;

		bool
		globe_is_active();

		bool
		map_is_active();

		boost::optional<GPlatesMaths::LatLonPoint>
		camera_llp();

	public slots:

		void
		activate_time_spinbox();

		void
		recalc_camera_position();

		void
		update_mouse_pointer_position(
				const GPlatesMaths::PointOnSphere &new_virtual_pos,
				bool is_on_globe);

		void
		update_mouse_pointer_position(
				const boost::optional<GPlatesMaths::LatLonPoint> &new_lat_lon_pos,
				bool is_on_map);
		
		void
		activate_zoom_spinbox();

		void
		handle_update_tools_and_status_message();

	signals:

		void
		update_tools_and_status_message();

		void
		send_camera_pos_to_stdout(
			double,
			double);

		void
		send_orientation_to_stdout(
			GPlatesMaths::Rotation &);
	
	private slots:

		void
		handle_globe_and_map_widget_resized(
				int new_width, int new_height);

		void
		handle_projection_type_changed(
				const GPlatesGui::ViewportProjection &viewport_projection);

	private:
		
		std::auto_ptr<QWidget>
		construct_awesomebar_one(
				GPlatesGui::AnimationController &animation_controller,
				GPlatesQtWidgets::ViewportWindow &main_window);

		std::auto_ptr<QWidget>
		construct_awesomebar_two(
				GPlatesGui::ViewportZoom &vzoom,
				GPlatesGui::ViewportProjection &vprojection);

		std::auto_ptr<QWidget>
		construct_viewbar(
				GPlatesGui::ViewportZoom &vzoom);

		// Experiment with adding the proj combo-box to the lower toolbar. 
		std::auto_ptr<QWidget>
		construct_viewbar_with_projections(
				GPlatesGui::ViewportZoom &vzoom,
				GPlatesGui::ViewportProjection &vprojection);
		
		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * The QSplitter responsible for dividing the interface between canvas
		 * and TaskPanel.
		 */
		QSplitter *d_splitter_widget;

		/**
		 * The camera coordinates label.
		 */
		QLabel *d_label_camera_coords;

		/**
		 * The mouse coordinates label.
		 */
		QLabel *d_label_mouse_coords;

		/**
		 * Holds the globe and the map.
		 */
		GlobeAndMapWidget *d_globe_and_map_widget_ptr;

		AnimateControlWidget *d_animate_control_widget_ptr;
		ZoomControlWidget *d_zoom_control_widget_ptr;
		TimeControlWidget *d_time_control_widget_ptr;
		ZoomSliderWidget *d_zoom_slider_widget;
		ProjectionControlWidget *d_projection_control_widget_ptr;
		GMenuButton *d_gmenu_button_ptr;

	};
}

#endif  // GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H
