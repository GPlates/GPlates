/* $Id$ */

/**
 * @file 
 * Contains the definition of the Map class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_MAP_H
#define GPLATES_GUI_MAP_H

#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "ColourScheme.h"
#include "MapBackground.h"
#include "MapGrid.h"
#include "MapProjection.h"
#include "MapRenderedGeometryCollectionPainter.h"
#include "ViewportZoom.h"

#include "opengl/GLVisualLayers.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesGui
{
	/**
	 * Holds the state for MapCanvas/MapView (analogous to the Globe class).
	 */
	class Map
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		Map(
				GPlatesPresentation::ViewState &view_state,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				const GPlatesPresentation::VisualLayers &visual_layers,
				ViewportZoom &viewport_zoom,
				const ColourScheme::non_null_ptr_type &colour_scheme,
				int device_pixel_ratio);

		/**
		 * Initialise any OpenGL state.
		 *
		 * This method is called when the OpenGL context is first bound (and hence we can make OpenGL calls).
		 */
		void
		initialiseGL(
				GPlatesOpenGL::GLRenderer &renderer);

		MapProjection &
		projection();

		const MapProjection &
		projection() const;

		MapProjection::Type
		projection_type() const;

		void
		set_projection_type(
				GPlatesGui::MapProjection::Type projection_type_);

		double
		central_meridian();

		void
		set_central_meridian(
				double central_meridian_);

		/**
		 * Paint the map and all the visible features and rasters on it.
		 *
		 * @param viewport_zoom_factor The magnification of the map in the viewport window.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor,
				const double &device_independent_pixel_to_map_space_ratio,
				float scale);

	private:

		//! To do map projections
		MapProjection::non_null_ptr_type d_map_projection;

		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		//! A pointer to the state's RenderedGeometryCollection
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		//! For zoom-dependent rendered objects.                                                                     
		GPlatesGui::ViewportZoom &d_viewport_zoom;		
		
		//! For giving colour to RenderedGeometry
		GPlatesGui::ColourScheme::non_null_ptr_type d_colour_scheme;

		/**
		 * The coloured map background (behind the grid and rendered geometry data).
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<MapBackground> d_background;

		/**
		 * Lines of lat and lon on the map.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<MapGrid> d_grid;

		/**
		 * Painter used to draw rendered geometry layers onto the map.
		 */
		MapRenderedGeometryCollectionPainter d_rendered_geom_collection_painter;
	};
}

#endif  // GPLATES_GUI_MAP_H
