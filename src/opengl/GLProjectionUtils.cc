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

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLProjectionUtils.h"

#include "GLIntersect.h"
#include "GLIntersectPrimitives.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/Vector3D.h"
#include "maths/types.h"


namespace GPlatesOpenGL
{
	namespace GLProjectionUtils
	{
		namespace
		{
			/**
			 * Projects a windows coordinate onto the unit sphere in model space
			 * using the specified model-view and projection transforms and the specified viewport.
			 *
			 * The returned vector is the intersection of the window coordinate (screen pixel)
			 * projected onto the unit sphere, or returns false if misses the globe.
			 */
			boost::optional<GPlatesMaths::Vector3D>
			project_window_coords_onto_unit_sphere(
					const GLViewport &viewport,
					const GLMatrix &model_view_transform,
					const GLMatrix &projection_transform,
					const double &window_x,
					const double &window_y)
			{
				// Get point on near clipping plane.
				double near_objx, near_objy, near_objz;
				if (glu_un_project(
					viewport,
					model_view_transform,
					projection_transform,
					window_x, window_y, 0,
					&near_objx, &near_objy, &near_objz) == GL_FALSE)
				{
					return boost::none;
				}

				// Get point on far clipping plane.
				double far_objx, far_objy, far_objz;
				if (glu_un_project(
					viewport,
					model_view_transform,
					projection_transform,
					window_x, window_y, 1,
					&far_objx, &far_objy, &far_objz) == GL_FALSE)
				{
					return boost::none;
				}

				// Near and far point in 3D model space.
				const GPlatesMaths::Vector3D near_point(near_objx, near_objy, near_objz);
				const GPlatesMaths::Vector3D far_point(far_objx, far_objy, far_objz);

				// Use the near and far 3D model-space points to form a ray with a ray origin
				// at the near point and ray direction pointing to the far point.
				const GLIntersect::Ray ray(
						near_point,
						(far_point - near_point).get_normalisation());

				// Create a unit sphere in model space representing the globe.
				const GLIntersect::Sphere sphere(GPlatesMaths::Vector3D(0,0,0), 1);

				// Intersect the ray with the globe.
				const boost::optional<GPlatesMaths::real_t> ray_distance =
						intersect_ray_sphere(ray, sphere);

				// Did the ray intersect the globe ?
				if (!ray_distance)
				{
					return boost::none;
				}

				// Return the point on the sphere where the ray first intersects.
				return ray.get_point_on_ray(ray_distance.get());
			}
		}
	}
}

int
GPlatesOpenGL::GLProjectionUtils::glu_project(
		const GLViewport &viewport,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		double objx,
		double objy,
		double objz,
		GLdouble *winx,
		GLdouble *winy,
		GLdouble *winz)
{
	return gluProject(
			objx, objy, objz,
			model_view_transform.get_matrix(),
			projection_transform.get_matrix(),
			viewport.get_viewport(),
			winx, winy, winz);
}


int
GPlatesOpenGL::GLProjectionUtils::glu_un_project(
		const GLViewport &viewport,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		double winx,
		double winy,
		double winz,
		GLdouble *objx,
		GLdouble *objy,
		GLdouble *objz)
{
	return gluUnProject(
			winx, winy, winz,
			model_view_transform.get_matrix(),
			projection_transform.get_matrix(),
			viewport.get_viewport(),
			objx, objy, objz);
}


double
GPlatesOpenGL::GLProjectionUtils::get_min_pixel_size_on_unit_sphere(
		const GLViewport &viewport,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform)
{
	//
	// Divide the near face of the normalised device coordinates (NDC) box into 9 points and
	// un-project them from window coordinates (see glViewport()) to model-space (x,y,z) positions.
	//
	// The NDC box is the rectangular clip box after the homogenous divide where the
	// clip coordinates (after the model-view-projection transformation) gets converted
	// from (x, y, z, w) to (x/w, y/w, z/w).
	// The NDC box is (-1 <= x <= 1), (-1 <= y <= 1) and (-1 <= z <= 1).
	// Since we are using glu_un_project() there's also the viewport transformation which maps
	// the NDC box to:
	// (viewport_x <= x <= viewport_x + viewport_width),
	// (viewport_y <= y <= viewport_y + viewport_height),
	// (0 <= z <= 1). /* well, glDepthRange does affect the z coordinate actually */
	//

	const double window_xy_coords[9][2] =
	{
		{viewport.x(),                          viewport.y()                           },
		{viewport.x() + 0.5 * viewport.width(), viewport.y()                           },
		{viewport.x() + viewport.width(),       viewport.y()                           },
		{viewport.x(),                          viewport.y() + 0.5 * viewport.height() },
		{viewport.x() + 0.5 * viewport.width(), viewport.y() + 0.5 * viewport.height() },
		{viewport.x() + viewport.width(),       viewport.y() + 0.5 * viewport.height() },
		{viewport.x(),                          viewport.y() + viewport.height()       },
		{viewport.x() + 0.5 * viewport.width(), viewport.y() + viewport.height()       },
		{viewport.x() + viewport.width(),       viewport.y() + viewport.height()       }
	};

	// Iterate over all sample points and project onto the unit sphere in model space.
	// Some might miss the sphere (for example, the corner points of the orthographic
	// view frustum when fully zoomed out most likely will miss the unit sphere)
	// but the centre point will always hit (only because the way GPlates currently
	// sets up its projections - we can't rely on this always being the case in which
	// case we'll return the distance from north pole to south pole if nothing hits.
	GPlatesMaths::real_t max_dot_product_pixel_size(-1);
	for (int n = 0; n < 9; ++n)
	{
		// Project the same point on the unit sphere.
		const boost::optional<GPlatesMaths::Vector3D> projected_pixel =
				project_window_coords_onto_unit_sphere(
						viewport,
						model_view_transform,
						projection_transform,
						window_xy_coords[n][0],
						window_xy_coords[n][1]);
		if (!projected_pixel)
		{
			continue;
		}

		// Project the sample point plus one pixel (in the x direction) onto the unit sphere.
		// It doesn't matter that the window coordinate might go outside the viewport
		// because there's no clipping happening here.
		const boost::optional<GPlatesMaths::Vector3D> projected_pixel_plus_one_x =
				project_window_coords_onto_unit_sphere(
						viewport,
						model_view_transform,
						projection_transform,
						window_xy_coords[n][0] + 1,
						window_xy_coords[n][1]);
		if (!projected_pixel_plus_one_x)
		{
			continue;
		}

		// The dot product can be converted to arc distance but we can delay that
		// expensive operation until we've compared all samples.
		const GPlatesMaths::real_t dot_product_pixel_size_x =
				dot(projected_pixel_plus_one_x.get(), projected_pixel.get());
		// We want the minimum projected pixel size which means maximum dot product.
		if (dot_product_pixel_size_x > max_dot_product_pixel_size)
		{
			max_dot_product_pixel_size = dot_product_pixel_size_x;
		}

		// Project the sample point plus one pixel (in the y direction) onto the unit sphere.
		// It doesn't matter that the window coordinate might go outside the viewport
		// because there's no clipping happening here.
		const boost::optional<GPlatesMaths::Vector3D> projected_pixel_plus_one_y =
				project_window_coords_onto_unit_sphere(
						viewport,
						model_view_transform,
						projection_transform,
						window_xy_coords[n][0],
						window_xy_coords[n][1] + 1);
		if (!projected_pixel_plus_one_y)
		{
			continue;
		}

		// The dot product can be converted to arc distance but we can delay that
		// expensive operation until we've compared all samples.
		const GPlatesMaths::real_t dot_product_pixel_size_y =
				dot(projected_pixel_plus_one_y.get(), projected_pixel.get());
		// We want the minimum projected pixel size which means maximum dot product.
		if (dot_product_pixel_size_y > max_dot_product_pixel_size)
		{
			max_dot_product_pixel_size = dot_product_pixel_size_y;
		}
	}

	// Convert from dot product to arc distance on the unit sphere.
	return acos(max_dot_product_pixel_size).dval();
}
