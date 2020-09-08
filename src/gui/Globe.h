/* $Id$ */

/**
 * @file 
 * File specific comments.
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

#ifndef GPLATES_GUI_GLOBE_H
#define GPLATES_GUI_GLOBE_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "BackgroundSphere.h"
#include "Colour.h"
#include "GlobeRenderedGeometryCollectionPainter.h"
#include "SphericalGrid.h"
#include "Stars.h"

#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLVisualLayers.h"

#include "presentation/VisualLayers.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLViewProjection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class GlobeCamera;
	class GlobeVisibilityTester;

	class Globe
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		Globe(
				GPlatesPresentation::ViewState &view_state,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesPresentation::VisualLayers &visual_layers,
				const GlobeVisibilityTester &visibility_tester);


		/**
		 * Initialise any OpenGL state.
		 *
		 * This method is called when the OpenGL context is first bound (and hence we can make OpenGL calls).
		 */
		void
		initialiseGL(
				GPlatesOpenGL::GL &gl);


		/**
		 * Paint the globe and all the visible features and rasters on it.
		 *
		 * @param view_projection The current view-projection transform and the viewport.
		 *
		 * @param viewport_zoom_factor The magnification of the globe in the viewport window.
		 *        Value should be one when earth fills viewport and proportionately greater
		 *        than one when viewport shows only part of the globe.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				const double &viewport_zoom_factor,
				float scale);

	private:

		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;
		
		//! The collection of @a RenderedGeometry objects we need to paint.
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		/**
		 * Stars in the background, behind the Earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<Stars> d_stars;

		/**
		 * The background sphere (can be opaque or translucent depending on the background colour's alpha).
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<BackgroundSphere> d_background_sphere;

		/**
		 * Lines of lat and lon on surface of earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<SphericalGrid> d_grid;

		/**
		 * Painter used to draw @a RenderedGeometry objects on the globe.
		 */
		GlobeRenderedGeometryCollectionPainter d_rendered_geom_collection_painter;


		/**
		 * Calculate transform to transform the view according to the current globe orientation.
		 */
		void
		get_globe_orientation_transform(
				GPlatesOpenGL::GLMatrix &transform) const;

		void
		set_scene_lighting(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLMatrix &view_orientation);

		void
		render_stars(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection);

		void
		render_sphere_background(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection);

		void
		render_globe_hemisphere_surface(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				std::vector<cache_handle_type> &cache_handle,
				const double &viewport_zoom_factor,
				bool is_front_half_globe);

		void
		render_globe_sub_surface(
				GPlatesOpenGL::GL &gl,
				std::vector<cache_handle_type> &cache_handle,
				const double &viewport_zoom_factor);
	};
}

#endif  // GPLATES_GUI_GLOBE_H
