/* $Id$ */

/**
 * \file
 * Contains the definition of the GlobeAndMapWidget class.
 *
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
 
#ifndef GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H
#define GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H

#include <QWidget>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "maths/LatLonPoint.h"

namespace GPlatesGui
{
	class ColourScheme;
	class ViewportProjection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class MapCanvas;
	class MapView;
	class SceneView;

	/**
	 * This class is responsible for creating and holding the globe and the map,
	 * and for switching between them as appropriate.
	 */
	class GlobeAndMapWidget :
		public QWidget
	{

		Q_OBJECT

	public:

		//! Use this constructor if you're constructing a fresh GlobeAndMapWidget from scratch.
		GlobeAndMapWidget(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		//! Use this constructor if you want to make a clone of an existing GlobeAndMapWidget.
		GlobeAndMapWidget(
				GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
				boost::shared_ptr<GPlatesGui::ColourScheme> colour_scheme,
				QWidget *parent_ = NULL);

		~GlobeAndMapWidget();

		GlobeCanvas &
		get_globe_canvas() const;

		MapCanvas &
		get_map_canvas() const;
		
		MapView &
		get_map_view() const;

		SceneView &
		get_active_view() const;

		boost::optional<GPlatesMaths::LatLonPoint> &
		get_camera_llp();

		const boost::optional<GPlatesMaths::LatLonPoint> &
		get_camera_llp() const;

		void
		set_mouse_wheel_enabled(
				bool enabled = true);

		virtual
		QSize
		sizeHint() const;

		virtual
		void
		resizeEvent(
				QResizeEvent *resize_event);

	signals:

		void
		update_tools_and_status_message();

		void
		resized(
				int new_width, int new_height);

	private slots:

		void
		init();

		void
		handle_zoom_change();

		void
		change_projection(
			const GPlatesGui::ViewportProjection &view_projection);

	private:

		void
		make_signal_slot_connections();

		GPlatesPresentation::ViewState &d_view_state;

		GlobeCanvas *d_globe_canvas_ptr;
		MapCanvas *d_map_canvas_ptr;
		MapView *d_map_view_ptr;

		//! Which of globe and map is currently active
		SceneView *d_active_view_ptr;

		boost::optional<GPlatesMaths::LatLonPoint> d_camera_llp;

	};
}

#endif  // GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H
