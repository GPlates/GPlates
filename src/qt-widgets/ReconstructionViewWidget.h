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

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <memory>
#include <QGraphicsScene>
#include <QWidget>
#include <QSplitter>
#include <QLabel>

#include <boost/optional.hpp>

#include "gui/ViewportZoom.h"
#include "maths/LatLonPointConversions.h"
#include "ReconstructionViewWidgetUi.h"

#include "ZoomSliderWidget.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class AnimationController;
}

namespace GPlatesQtWidgets
{
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
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::AnimationController &animation_controller,
				ViewportWindow &view_state,
				QWidget *parent_ = NULL);


		GlobeCanvas &
		globe_canvas() const
		{
			return *d_globe_canvas_ptr;
		}

		/**
		 * The Task Panel is created, and initialised by ViewportWindow.
		 * ViewportWindow uses this method to insert the panel into the
		 * ReconstructionViewWidget's layout. This is done mainly to reduce
		 * dependencies in ViewportWindow's initialiser list.
		 */
		void
		insert_task_panel(
				std::auto_ptr<GPlatesQtWidgets::TaskPanel> task_panel);
				

		MapView &
		map_view() const
		{
			return *d_map_view_ptr;
		}

		SceneView &
		active_view() const
		{
			return *d_active_view_ptr;
		}

		MapCanvas &
		map_canvas() const
		{
			return *d_map_canvas_ptr;
		}

		bool
		globe_is_active();

		bool
		map_is_active();

		GPlatesMaths::LatLonPoint &
		camera_llp()
		{
			return *d_camera_llp;
		}
#if 0
		GPlatesGui::ViewportZoom &
		viewport_zoom() 
		{
			return d_viewport_zoom;
		}
#endif


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
		handle_zoom_change();

		void
		change_projection(
			int projection_type);

	signals:

		void
		update_tools_and_status_message();

	private:
		
		std::auto_ptr<QWidget>
		construct_awesomebar_one(
				GPlatesGui::AnimationController &animation_controller);

		std::auto_ptr<QWidget>
		construct_awesomebar_two(
				GPlatesGui::ViewportZoom &vzoom,
				GPlatesQtWidgets::MapCanvas *map_canvas_ptr);

		std::auto_ptr<QWidget>
		construct_viewbar(
				GPlatesGui::ViewportZoom &vzoom);

		// Experiment with adding the proj combo-box to the lower toolbar. 
		std::auto_ptr<QWidget>
		construct_viewbar_with_projections(
				GPlatesGui::ViewportZoom &vzoom,
				GPlatesQtWidgets::MapCanvas *map_canvas_ptr);

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

		GlobeCanvas *d_globe_canvas_ptr;
		AnimateControlWidget *d_animate_control_widget_ptr;
		ZoomControlWidget *d_zoom_control_widget_ptr;
		TimeControlWidget *d_time_control_widget_ptr;
		ZoomSliderWidget *d_zoom_slider_widget;
		ProjectionControlWidget *d_projection_control_widget_ptr;

		// The QGraphicsScene representing the map canvas.
		MapCanvas *d_map_canvas_ptr;

		// The QGraphicsView associated with the map canvas.
		MapView *d_map_view_ptr;

		// The active scene view.
		SceneView *d_active_view_ptr;

		boost::optional<GPlatesMaths::LatLonPoint> d_camera_llp;

	};
}

#endif  // GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H
