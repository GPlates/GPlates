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
 
#ifndef GPLATES_OPENGL_GLPROJECTION_H
#define GPLATES_OPENGL_GLPROJECTION_H

#include <utility>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "GLIntersectPrimitives.h"
#include "GLMatrix.h"
#include "GLViewport.h"


namespace GPlatesOpenGL
{
	/**
	 * Utilities involving projection of 3D geometry to screen-space using OpenGL.
	 *
	 * This typically involves the OpenGL model-view and projection transforms and the viewport.
	 */
	class GLProjection
	{
	public:

		GLProjection(
				const GLViewport &viewport,
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform);


		/**
		 * Convenience function performs same as similarly named OpenGL function.
		 */
		int
		glu_project(
				double objx,
				double objy,
				double objz,
				GLdouble *winx,
				GLdouble *winy,
				GLdouble *winz) const;


		/**
		 * Convenience function performs same as similarly named OpenGL function.
		 */
		int
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
		 * Returns none is unable to invert model-view-projection transform.
		 */
		boost::optional<GLIntersect::Ray>
		project_window_coords_into_ray(
				const double &window_x,
				const double &window_y) const;


		/**
		 * Projects a window coordinate onto the unit sphere in model space using the specified
		 * model-view and projection transforms and the specified viewport.
		 *
		 * The returned vector is the intersection of the window coordinate (screen pixel)
		 * projected onto the unit sphere.
		 *
		 * Returns false if misses the globe (or if unable to invert model-view-projection transform).
		 *
		 * The screen pixel ray is intersected with the unit sphere (centered on global origin).
		 * The first intersection with sphere is the returned position on sphere.
		 */
		boost::optional<GPlatesMaths::UnitVector3D>
		project_window_coords_onto_unit_sphere(
				const double &window_x,
				const double &window_y) const;


		/**
		 * Returns an estimate of the minimum and maximum sizes of one viewport pixel,
		 * at the specified position on the unit sphere.
		 *
		 * Currently this is done by sampling 8 screen points in a circle (of radius one pixel) around the window
		 * coordinate (that @a position_on_sphere projects onto) and projecting them onto the unit sphere.
		 * Then minimum and maximum distances of these unit-sphere samples to @a position_on_sphere are returned.
		 *
		 * Since these sampled points are projected onto the visible front side of the unit sphere, it is
		 * assumed that @a position_on_sphere is also on the visible front side of the unit sphere.
		 *
		 * Returned results are in the range (0, Pi] where Pi is the distance between North and South poles.
		 *
		 * Returns none if none of the offset pixels intersect the unit sphere.
		 */
		boost::optional< std::pair<double/*min*/, double/*max*/> >
		get_min_max_pixel_size_on_unit_sphere(
				const GPlatesMaths::UnitVector3D &position_on_sphere) const;


		/**
		 * Returns an estimate of the minimum and maximum sizes of viewport pixels projected onto the unit sphere.
		 *
		 * This assumes the globe is a sphere of radius one centred at the origin in model space.
		 *
		 * Currently this is done by sampling the corners of the view frustum and the middle
		 * of each of the four sides of the view frustum and the centre.
		 *
		 * This method is reasonably expensive but should be fine since it's only
		 * called once per raster per render scene.
		 *
		 * Returned result is in the range (0, Pi] where Pi is the distance between north and
		 * south poles on the unit sphere.
		 */
		std::pair<double/*min*/, double/*max*/>
		get_min_max_pixel_size_on_unit_sphere() const;


		/**
		 * Returns the minimum value of @a get_min_max_pixel_size_on_unit_sphere.
		 *
		 * See @a get_min_max_pixel_size_on_unit_sphere for more details.
		 */
		inline
		double
		get_min_pixel_size_on_unit_sphere() const
		{
			return get_min_max_pixel_size_on_unit_sphere().first/*min*/;
		}


		/**
		 * Returns the maximum value of @a get_min_max_pixel_size_on_unit_sphere.
		 *
		 * See @a get_min_max_pixel_size_on_unit_sphere for more details.
		 */
		inline
		double
		get_max_pixel_size_on_unit_sphere() const
		{
			return get_min_max_pixel_size_on_unit_sphere().second/*max*/;
		}

	private:
		GLViewport d_viewport;
		GLMatrix d_model_view_transform;
		GLMatrix d_projection_transform;

		mutable boost::optional<GLMatrix> d_inverse_model_view_projection;
	};
}

#endif // GPLATES_OPENGL_GLPROJECTION_H
