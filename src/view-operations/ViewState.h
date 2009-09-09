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

#ifndef GPLATES_VIEW_OPERATIONS_VIEWSTATE_H
#define GPLATES_VIEW_OPERATIONS_VIEWSTATE_H

#include <boost/scoped_ptr.hpp>
#include <QObject>

#include "gui/GeometryFocusHighlight.h"
#include "gui/ViewportZoom.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/ViewportProjection.h"


namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class MapCanvas;
}

namespace GPlatesViewOperations
{
	class ViewState :
			public QObject
	{
		Q_OBJECT
		
	public:
		ViewState(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::FeatureFocus &feature_focus);

		~ViewState();


		GPlatesGui::ViewportZoom &
		get_viewport_zoom()
		{
			return d_viewport_zoom;
		}


		ViewportProjection &
		get_viewport_projection()
		{
			return d_viewport_projection;
		}

	private slots:
		void
		handle_zoom_change();

		void
		handle_projection_type_change(
				const GPlatesViewOperations::ViewportProjection &viewport_projection);
		void
		handle_central_meridian_change(
				const GPlatesViewOperations::ViewportProjection &viewport_projection);

	private:
		//! Used to store rendered geometries for the current view state. FIXME: ultimately it will live here.
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		//! The viewport zoom state.
		GPlatesGui::ViewportZoom d_viewport_zoom;

		//! The viewport projection state.
		GPlatesViewOperations::ViewportProjection d_viewport_projection;

		//! Renders the focused geometry highlighted.
		GPlatesGui::GeometryFocusHighlight d_geometry_focus_highlight;

#if 0
		boost::scoped_ptr<GPlatesQtWidgets::GlobeCanvas> d_globe_canvas_ptr;

		// The QGraphicsScene representing the map canvas.
		boost::scoped_ptr<GPlatesQtWidgets::MapCanvas> d_map_canvas_ptr;

		// The QGraphicsView associated with the map canvas.
		boost::scoped_ptr<GPlatesQtWidgets::MapView> d_map_view_ptr;

		// The active scene view.
		GPlatesQtWidgets::SceneView *d_active_view_ptr;

		boost::optional<GPlatesMaths::LatLonPoint> d_camera_llp;
#endif
	};
}

#endif // GPLATES_VIEW_OPERATIONS_VIEWSTATE_H
