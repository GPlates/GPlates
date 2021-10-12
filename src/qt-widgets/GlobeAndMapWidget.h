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
#include <boost/scoped_ptr.hpp>

#include "gui/ColourScheme.h"
#include "maths/LatLonPoint.h"

namespace GPlatesGui
{
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
				const GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
				GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
				QWidget *parent_ = NULL);

		~GlobeAndMapWidget();

		GlobeCanvas &
		get_globe_canvas();

		const GlobeCanvas &
		get_globe_canvas() const;
		
		MapView &
		get_map_view();

		const MapView &
		get_map_view() const;

		SceneView &
		get_active_view();

		const SceneView &
		get_active_view() const;

		bool
		is_globe_active() const;

		bool
		is_map_active() const;

		boost::optional<GPlatesMaths::LatLonPoint>
		get_camera_llp() const;

		void
		set_mouse_wheel_enabled(
				bool enabled = true);

		virtual
		QSize
		sizeHint() const;

		//! Gets the framebuffer for the active view.
		QImage
		grab_frame_buffer();

		void
		update_canvas();

	signals:

		void
		update_tools_and_status_message();

		void
		resized(
				int new_width, int new_height);

		void
		repainted(bool mouse_down);

	protected:

		virtual
		void
		resizeEvent(
				QResizeEvent *resize_event);

	private slots:

		void
		init();

		void
		handle_zoom_change();

		void
		change_projection(
			const GPlatesGui::ViewportProjection &view_projection);

		void
		handle_globe_or_map_repainted(
				bool mouse_down);

	private:

		void
		make_signal_slot_connections();

		GPlatesPresentation::ViewState &d_view_state;

		boost::scoped_ptr<GlobeCanvas> d_globe_canvas_ptr;
		boost::scoped_ptr<MapView> d_map_view_ptr;

		//! Which of globe and map is currently active.
		SceneView *d_active_view_ptr;

	};
}

#endif  // GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H
