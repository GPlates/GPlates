/* $Id$ */


/**
 * \file 
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
 
#ifndef GPLATES_OPENGL_GLVIEWPROJECTION_H
#define GPLATES_OPENGL_GLVIEWPROJECTION_H

#include <utility>
#include <boost/optional.hpp>
#include <opengl/OpenGL1.h>

#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"

#include "GLIntersectPrimitives.h"
#include "GLMatrix.h"
#include "GLViewport.h"


namespace GPlatesGui
{
	class MapProjection;
}

namespace GPlatesOpenGL
{
	/**
	 * Transform world space coordinates to window coordinates (and vice versa).
	 *
	 * Also contains utilities for projecting window coordinates onto unit sphere and into world-space rays.
	 *
	 * This involves the view and projection transforms, and the viewport.
	 */
	class GLViewProjection
	{
	public:

		GLViewProjection(
				const GLViewport &viewport,
				const GLMatrix &view_transform,
				const GLMatrix &projection_transform);

		/**
		 * Returns viewport.
		 *
		 * Converts clip space to window space.
		 */
		const GLViewport &
		get_viewport() const
		{
			return d_viewport;
		}


		/**
		 * Returns view transform.
		 *
		 * Converts world space vector 'w' to view space vector 'v':
		 *
		 *   v = V * w
		 */
		const GLMatrix &
		get_view_transform() const
		{
			return d_view_transform;
		}

		/**
		 * Returns inverse of view transform.
		 *
		 * Converts view space vector 'v' to world space vector 'w':
		 *
		 *   w = inverse(V) * v
		 *
		 * Returns none if failed to invert.
		 */
		const boost::optional<GLMatrix> &
		get_inverse_view_transform() const;


		/**
		 * Returns projection transform.
		 *
		 * Converts view space vector 'v' to clip space vector 'c':
		 *
		 *   c = P * v
		 */
		const GLMatrix &
		get_projection_transform() const
		{
			return d_projection_transform;
		}

		/**
		 * Returns inverse of projection transform.
		 *
		 * Converts clip space vector 'c' to view space vector 'v':
		 *
		 *   v = inverse(P) * c
		 *
		 * Returns none if failed to invert.
		 */
		const boost::optional<GLMatrix> &
		get_inverse_projection_transform() const;


		/**
		 * Returns view-projection transform.
		 *
		 * Converts world space vector 'w' to clip space vector 'c' using 'P * V':
		 *
		 *   c = P * v
		 *     = P * V * w
		 */
		const GLMatrix &
		get_view_projection_transform() const;

		/**
		 * Returns inverse of view-projection transform.
		 *
		 * Converts clip space vector 'c' to world space vector 'w' using 'inverse(V) * inverse(P)':
		 *
		 *   w = inverse(V) * v
		 *     = inverse(V) * inverse(P) * c
		 *
		 * Returns none if failed to invert view or projection.
		 */
		const boost::optional<GLMatrix> &
		get_inverse_view_projection_transform() const;


		/**
		 * Convenience function performs same as similarly named OpenGL function (gluProject).
		 */
		bool
		glu_project(
				double objx,
				double objy,
				double objz,
				GLdouble *winx,
				GLdouble *winy,
				GLdouble *winz) const;


		/**
		 * Convenience function performs same as similarly named OpenGL function (gluUnProject).
		 */
		bool
		glu_un_project(
				double winx,
				double winy,
				double winz,
				GLdouble *objx,
				GLdouble *objy,
				GLdouble *objz) const;


		/**
		 * The screen pixel is converted to a ray where the ray origin is screen pixel projected onto
		 * the near plane (of the projection transform) and the ray direction is towards the screen
		 * pixel projected onto the far plane.
		 *
		 * Returns none if unable to invert view-projection transform.
		 */
		boost::optional<GLIntersect::Ray>
		project_window_coords_into_ray(
				const double &window_x,
				const double &window_y) const;


		/**
		 * Projects a window coordinate onto the globe (unit sphere), or map plane (z=0) if @a map_projection
		 * specified, using the specified view and projection transforms and the specified viewport.
		 *
		 * The returned position is the intersection of the window coordinate screen pixel
		 * (projected from the view into world space) and the globe (unit sphere) or map plane (z=0).
		 *
		 * Returns false if misses the globe or map (or if unable to invert view-projection transform).
		 * When @a map_projection is specified the intersection must not just be with the map plane (z=0)
		 * but also that part of the map plane corresponding to the Earth (otherwise false is returned).
		 *
		 * When intersecting the globe (when @a map_projection is not specified) the first intersection
		 * with the globe (unit sphere) is returned.
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		project_window_coords_onto_globe(
				const double &window_x,
				const double &window_y,
				boost::optional<const GPlatesGui::MapProjection &> map_projection = boost::none) const;


		/**
		 * Returns an estimate of the minimum and maximum sizes of one viewport pixel at the specified
		 * position on the globe (unit sphere) in units of radians (arc distance on unit sphere).
		 *
		 * Currently this is done by sampling 8 screen points in a circle (of radius one pixel) around the window
		 * coordinate (that @a position_on_globe projects onto). If @a map_projection is specified then these 8 screen rays
		 * are intersected with the map (part of map plane, z=0, associated with the Earth; the map projection) and
		 * inverse transformed (using map projection) onto the globe. If @a map_projection is *not* specified then
		 * these 8 screen rays are directly intersected with the globe (unit sphere).
		 *
		 * Then minimum and maximum distances, on the unit sphere, of these globe samples to @a position_on_globe are returned.
		 *
		 * When intersecting the globe (when @a map_projection is not specified) then these sampled points are
		 * projected onto the visible front side of the globe, and it is assumed that @a position_on_globe is
		 * also on the visible front side of the globe.
		 *
		 * Returned results are in the range (0, Pi] where Pi is the distance between North and South poles.
		 *
		 * Returns none if none of the offset pixels intersect the globe/map.
		 */
		boost::optional< std::pair<double/*min*/, double/*max*/> >
		get_min_max_pixel_size_on_globe(
				const GPlatesMaths::PointOnSphere &position_on_globe,
				boost::optional<const GPlatesGui::MapProjection &> map_projection = boost::none) const;


		/**
		 * Returns an estimate of the minimum and maximum sizes of viewport pixels projected onto the globe (unit sphere).
		 *
		 * This assumes the globe is a sphere of radius one centred at the origin in world space.
		 *
		 * Currently this is done by sampling the corners of the view frustum and the middle
		 * of each of the four sides of the view frustum and the centre.
		 *
		 * This method is reasonably expensive but should be fine since it's only
		 * called once per raster per render scene.
		 *
		 * Returned result is in the range (0, Pi] where Pi is the distance between north and
		 * south poles on the unit sphere.
		 *
		 * TODO: Remove this once we test each raster/filled-polygon tile separately rather than determining
		 *       one LOD level for the entire raster/filled-polygon (now that have variable LOD due to view tilt).
		 */
		std::pair<double/*min*/, double/*max*/>
		get_min_max_pixel_size_on_globe() const;

	private:
		GLViewport d_viewport;

		GLMatrix d_view_transform;
		GLMatrix d_projection_transform;
		mutable boost::optional<GLMatrix> d_view_projection_transform;

		mutable boost::optional<GLMatrix> d_inverse_view_transform;
		mutable boost::optional<GLMatrix> d_inverse_projection_transform;
		mutable boost::optional<GLMatrix> d_inverse_view_projection_transform;


		boost::optional<GPlatesMaths::PointOnSphere>
		project_ray_onto_globe(
				const GLIntersect::Ray &ray) const;

		boost::optional<GPlatesMaths::PointOnSphere>
		project_ray_onto_map(
				const GLIntersect::Ray &ray,
				const GPlatesGui::MapProjection &map_projection) const;
	};
}

#endif // GPLATES_OPENGL_GLVIEWPROJECTION_H
