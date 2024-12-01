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

#include "Colour.h"
#include "ColourScheme.h"
#include "GlobeRenderedGeometryCollectionPainter.h"
#include "OpaqueSphere.h"
#include "SphericalGrid.h"
#include "SimpleGlobeOrientation.h"
#include "Stars.h"

#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLTexture.h"
#include "opengl/GLVisualLayers.h"

#include "presentation/VisualLayers.h"

#include "utils/VirtualProxy.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
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
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme,
				int device_pixel_ratio);

		//! To clone a Globe
		Globe(
				Globe &existing_globe,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme,
				int device_pixel_ratio);

		~Globe()
		{  }

		/**
		 * Initialise any OpenGL state.
		 *
		 * This method is called when the OpenGL context is first bound (and hence we can make OpenGL calls).
		 */
		void
		initialiseGL(
				GPlatesOpenGL::GLRenderer &renderer);

		void
		set_new_handle_pos(
				const GPlatesMaths::PointOnSphere &pos);

		/**
		 * @a in_mouse_drag should be set to true when the mouse button (left) is pressed (down)
		 * and the mouse is moving and if it is set to true then it should subsequently be set back
		 * to false when the mouse button (left) is released (up).
		 */
		void
		update_handle_pos(
				const GPlatesMaths::PointOnSphere &pos,
				bool in_mouse_drag = false);

		const GPlatesMaths::PointOnSphere
		orient(
				const GPlatesMaths::PointOnSphere &pos) const;

		const SimpleGlobeOrientation &
		orientation() const
		{
			return *d_globe_orientation_ptr;
		}

		SimpleGlobeOrientation &
		orientation()
		{
			return *d_globe_orientation_ptr;
		}

		/**
		 * Paint the globe and all the visible features and rasters on it.
		 *
		 * The three projection transforms differ only in their far clip plane distance.
		 * One includes only the front half of the globe, another includes the full globe and
		 * another is long enough to allow rendering of the stars.
		 *
		 * @param viewport_zoom_factor The magnification of the globe in the viewport window.
		 *        Value should be one when earth fills viewport and proportionately greater
		 *        than one when viewport shows only part of the globe.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor,
				const double &device_independent_pixel_to_world_space_ratio,
				float scale,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_stars);

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
		 * The solid earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<OpaqueSphere> d_sphere;

		/**
		 * Lines of lat and lon on surface of earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<SphericalGrid> d_grid;

		/**
		 * The accumulated orientation of the globe.
		 */
		boost::shared_ptr<SimpleGlobeOrientation> d_globe_orientation_ptr;

		/**
		 * Is true when the mouse button (left) is pressed (down) and mouse is moving.
		 *
		 * This is currently used to temporarily reduce the sampling rate for 3D scalar field iso-surfaces.
		 */
		bool d_globe_orientation_changing_during_mouse_drag;

		/**
		 * Painter used to draw @a RenderedGeometry objects on the globe.
		 */
		GlobeRenderedGeometryCollectionPainter d_rendered_geom_collection_painter;

		//! Multiplier for point sizes and line widths (due to a device *independent* pixel containing multiple device pixels).
		int d_device_pixel_ratio;


		/**
		 * Calculate tranform to ransform the view according to the current globe orientation.
		 */
		void
		get_globe_orientation_transform(
				GPlatesOpenGL::GLMatrix &transform) const;

		void
		set_scene_lighting(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLMatrix &view_orientation);

		void
		render_stars(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_stars);

		void
		render_sphere_background(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe);

		void
		render_globe_hemisphere_surface(
				GPlatesOpenGL::GLRenderer &renderer,
				std::vector<cache_handle_type> &cache_handle,
				const double &viewport_zoom_factor,
				const double &device_independent_pixel_to_world_space_ratio,
				const GPlatesOpenGL::GLMatrix &projection_transform,
				bool is_front_half_globe);

		void
		render_front_globe_hemisphere_surface_texture(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesOpenGL::GLTexture::shared_ptr_to_const_type &front_globe_surface_texture);

		void
		render_globe_sub_surface(
				GPlatesOpenGL::GLRenderer &renderer,
				std::vector<cache_handle_type> &cache_handle,
				const double &viewport_zoom_factor,
				const double &device_independent_pixel_to_world_space_ratio,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
				boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture = boost::none);
	};
}

#endif  // GPLATES_GUI_GLOBE_H
