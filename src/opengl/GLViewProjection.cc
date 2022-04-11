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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <cmath>
#include <boost/optional.hpp>

#include "GLViewProjection.h"

#include "GLIntersect.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"
#include "maths/Vector3D.h"
#include "maths/types.h"


GPlatesOpenGL::GLViewProjection::GLViewProjection(
		const GLViewport &viewport,
		const GLMatrix &view_transform,
		const GLMatrix &projection_transform) :
	d_viewport(viewport),
	d_view_transform(view_transform),
	d_projection_transform(projection_transform)
{
}


const boost::optional<GPlatesOpenGL::GLMatrix> &
GPlatesOpenGL::GLViewProjection::get_inverse_view_transform() const
{
	if (!d_inverse_view_transform)
	{
		//
		// Converts view space vector 'v' to world space vector 'w':
		//
		//   w = inverse(V) * v
		//
		GLMatrix inverse_view_transform = get_view_transform();
		if (inverse_view_transform.glu_inverse())
		{
			d_inverse_view_transform = inverse_view_transform;
		}
		// else failed to invert so leave 'd_inverse_view_transform' as boost::none.
	}

	return d_inverse_view_transform;
}


const boost::optional<GPlatesOpenGL::GLMatrix> &
GPlatesOpenGL::GLViewProjection::get_inverse_projection_transform() const
{
	if (!d_inverse_projection_transform)
	{
		//
		// Converts clip space vector 'c' to view space vector 'v':
		//
		//   v = inverse(P) * c
		//
		GLMatrix inverse_projection_transform = get_projection_transform();
		if (inverse_projection_transform.glu_inverse())
		{
			d_inverse_projection_transform = inverse_projection_transform;
		}
		// else failed to invert so leave 'd_inverse_projection_transform' as boost::none.
	}

	return d_inverse_projection_transform;
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLViewProjection::get_view_projection_transform() const
{
	if (!d_view_projection_transform)
	{
		//
		// Converts world space vector 'w' to clip space vector 'c' using 'P * V':
		//
		//   c = P * v
		//     = P * V * w
		//
		d_view_projection_transform = get_projection_transform();
		d_view_projection_transform->gl_mult_matrix(get_view_transform());
	}

	return d_view_projection_transform.get();
}


const boost::optional<GPlatesOpenGL::GLMatrix> &
GPlatesOpenGL::GLViewProjection::get_inverse_view_projection_transform() const
{
	if (!d_inverse_view_projection_transform)
	{
		//
		// Converts clip space vector 'c' to world space vector 'w' using 'inverse(V) * inverse(P)':
		//
		//   w = inverse(V) * v
		//     = inverse(V) * inverse(P) * c
		//
		const boost::optional<GPlatesOpenGL::GLMatrix> &inverse_view_transform = get_inverse_view_transform();
		const boost::optional<GPlatesOpenGL::GLMatrix> &inverse_projection_transform = get_inverse_projection_transform();
		if (inverse_view_transform && inverse_projection_transform)
		{
			GLMatrix inverse_view_projection_transform = inverse_view_transform.get();
			inverse_view_projection_transform.gl_mult_matrix(inverse_projection_transform.get());

			d_inverse_view_projection_transform = inverse_view_projection_transform;
		}
		// else failed to invert so leave 'd_inverse_view_projection_transform' as boost::none.
	}

	return d_inverse_view_projection_transform;
}


bool
GPlatesOpenGL::GLViewProjection::glu_project(
		double objx,
		double objy,
		double objz,
		GLdouble *winx,
		GLdouble *winy,
		GLdouble *winz) const
{
	double in_vec[4] = { objx, objy, objz, 1.0 };
	double out_vec[4];

	// Transform world-space vector to clip-space vector using the view-projection.
	get_view_projection_transform().glu_mult_vec(in_vec, out_vec);

	if (GPlatesMaths::are_almost_exactly_equal(out_vec[3], 0.0))
	{
		return false;
	}

	// Divide xyz by w.
	const double inv_w = 1.0 / out_vec[3];
	out_vec[0] *= inv_w;
	out_vec[1] *= inv_w;
	out_vec[2] *= inv_w;

	// Convert range xyz range [-1, 1] to [0, 1].
	out_vec[0] = 0.5 + 0.5 * out_vec[0];
	out_vec[1] = 0.5 + 0.5 * out_vec[1];
	out_vec[2] = 0.5 + 0.5 * out_vec[2];

	// Convert x range [0, 1] to [viewport_x, viewport_x + viewport_width].
	// Convert y range [0, 1] to [viewport_y, viewport_y + viewport_height].
	out_vec[0] = d_viewport.x() + out_vec[0] * d_viewport.width();
	out_vec[1] = d_viewport.y() + out_vec[1] * d_viewport.height();

	// Return window coordinates.
	*winx = out_vec[0];
	*winy = out_vec[1];
	*winz = out_vec[2];

	return true;
}


bool
GPlatesOpenGL::GLViewProjection::glu_un_project(
		double winx,
		double winy,
		double winz,
		GLdouble *objx,
		GLdouble *objy,
		GLdouble *objz) const
{
	// Get the inverse view-projection transform.
	const boost::optional<GLMatrix> &inverse_view_projection = get_inverse_view_projection_transform();
	if (!inverse_view_projection)
	{
		return false;
	}

	double in_vec[4] = { winx, winy, winz, 1.0 };

	// Convert x range [viewport_x, viewport_x + viewport_width] to [0, 1].
	// Convert y range [viewport_y, viewport_y + viewport_height] to [0, 1].
	in_vec[0] = (in_vec[0] - d_viewport.x()) / d_viewport.width();
	in_vec[1] = (in_vec[1] - d_viewport.y()) / d_viewport.height();

	// Convert range xyz range [0, 1] to [-1, 1].
	in_vec[0] = 2 * in_vec[0] - 1;
	in_vec[1] = 2 * in_vec[1] - 1;
	in_vec[2] = 2 * in_vec[2] - 1;

	// Transform window-space vector using inverse view-projection matrix.
	double out_vec[4];
	inverse_view_projection->glu_mult_vec(in_vec, out_vec);

	if (GPlatesMaths::are_almost_exactly_equal(out_vec[3], 0.0))
	{
		return false;
	}

	// Divide xyz by w.
	const double inv_w = 1.0 / out_vec[3];
	out_vec[0] *= inv_w;
	out_vec[1] *= inv_w;
	out_vec[2] *= inv_w;

	// Return world-space coordinates.
	*objx = out_vec[0];
	*objy = out_vec[1];
	*objz = out_vec[2];

	return true;
}


boost::optional<GPlatesOpenGL::GLIntersect::Ray>
GPlatesOpenGL::GLViewProjection::project_window_coords_into_ray(
		const double &window_x,
		const double &window_y) const
{
	// Get point on near clipping plane.
	double near_objx, near_objy, near_objz;
	if (!glu_un_project(window_x, window_y, 0, &near_objx, &near_objy, &near_objz))
	{
		return boost::none;
	}

	// Get point on far clipping plane.
	double far_objx, far_objy, far_objz;
	if (!glu_un_project(window_x, window_y, 1, &far_objx, &far_objy, &far_objz))
	{
		return boost::none;
	}

	// Near and far point in 3D world space.
	const GPlatesMaths::Vector3D near_point(near_objx, near_objy, near_objz);
	const GPlatesMaths::Vector3D far_point(far_objx, far_objy, far_objz);

	const GPlatesMaths::Vector3D near_to_far = far_point - near_point;
	if (near_to_far.is_zero_magnitude())
	{
		return boost::none;
	}

	// Use the near and far 3D world-space points to form a ray with a ray origin
	// at the near point and ray direction pointing to the far point.
	return GLIntersect::Ray(
			near_point,
			near_to_far.get_normalisation());
}


boost::optional<GPlatesMaths::UnitVector3D>
GPlatesOpenGL::GLViewProjection::project_window_coords_onto_unit_sphere(
		const double &window_x,
		const double &window_y) const
{
	boost::optional<GLIntersect::Ray> ray = project_window_coords_into_ray(window_x, window_y);
	if (!ray)
	{
		return boost::none;
	}

	// Create a unit sphere in world space representing the globe.
	const GLIntersect::Sphere sphere(GPlatesMaths::Vector3D(0,0,0), 1);

	// Intersect the ray with the globe.
	const boost::optional<GPlatesMaths::real_t> ray_distance =
			intersect_ray_sphere(ray.get(), sphere);

	// Did the ray intersect the globe ?
	if (!ray_distance)
	{
		return boost::none;
	}

	// Return the point on the sphere where the ray first intersects.
	// Due to numerical precision the ray may be slightly off the sphere so we'll
	// normalise it (otherwise can provide out-of-range for 'acos' later on).
	// Also note the normalisation shouldn't fail since ray-globe intersection cannot be at the origin.
	return ray->get_point_on_ray(ray_distance.get()).get_normalisation();
}


boost::optional< std::pair<double/*min*/, double/*max*/> >
GPlatesOpenGL::GLViewProjection::get_min_max_pixel_size_on_unit_sphere(
		const GPlatesMaths::UnitVector3D &projected_pixel) const
{
	// Find the window coordinates of the position on the unit sphere.
	GLdouble window_x, window_y, window_z;
	if (!glu_project(
			projected_pixel.x().dval(),
			projected_pixel.y().dval(),
			projected_pixel.z().dval(),
			&window_x,
			&window_y,
			&window_z))
	{
		return boost::none;
	}

	// Calculate 8 sample points in a circle (of radius one pixel) around the window coordinate.
	const double inv_sqrt_two = 1.0 / std::sqrt(2.0);
	// The offset pixel coordinates.
	// It doesn't matter if their window coordinates go outside the viewport
	// because there's no clipping happening here.
	const double window_xy_offset_coords[8][2] =
	{
		{ window_x + 1.0         , window_y },
		{ window_x - 1.0         , window_y },
		{ window_x               , window_y + 1.0 },
		{ window_x               , window_y - 1.0 },
		{ window_x + inv_sqrt_two, window_y + inv_sqrt_two },
		{ window_x + inv_sqrt_two, window_y - inv_sqrt_two },
		{ window_x - inv_sqrt_two, window_y + inv_sqrt_two },
		{ window_x - inv_sqrt_two, window_y - inv_sqrt_two }
	};

	// Iterate over all sample points and project onto the unit sphere.
	// Some might miss the unit sphere if the position on the unit sphere is tangential to the view.
	// If all miss the unit sphere then we return no result.
	bool projected_at_least_one_offset_pixel_onto_unit_sphere = false;
	GPlatesMaths::real_t min_dot_product(1.0);
	GPlatesMaths::real_t max_dot_product(-1.0);
	for (int n = 0; n < 8; ++n)
	{
		// Project the offset pixel onto the unit sphere.
		const boost::optional<GPlatesMaths::UnitVector3D> projected_offset_pixel =
				project_window_coords_onto_unit_sphere(
						window_xy_offset_coords[n][0],
						window_xy_offset_coords[n][1]);
		if (!projected_offset_pixel)
		{
			continue;
		}

		// The dot product can be converted to arc distance but we can delay that
		// expensive operation until we've compared all samples.
		const GPlatesMaths::real_t dot_product =
				dot(projected_offset_pixel.get(), projected_pixel);
		// Here we want the maximum projected pixel size which means minimum dot product.
		if (dot_product < min_dot_product)
		{
			min_dot_product = dot_product;
		}
		// Here we want the minimum projected pixel size which means maximum dot product.
		if (dot_product > max_dot_product)
		{
			max_dot_product = dot_product;
		}

		projected_at_least_one_offset_pixel_onto_unit_sphere = true;
	}

	// If none of our offset pixels intersected the unit sphere then return none.
	if (!projected_at_least_one_offset_pixel_onto_unit_sphere)
	{
		return boost::none;
	}

	// Convert from dot product to arc distance on the unit sphere.
	const double min_distance = acos(max_dot_product).dval();
	const double max_distance = acos(min_dot_product).dval();

	return std::make_pair(min_distance, max_distance);
}


std::pair<double/*min*/, double/*max*/>
GPlatesOpenGL::GLViewProjection::get_min_max_pixel_size_on_unit_sphere() const
{
	//
	// Divide the near face of the normalised device coordinates (NDC) box into 9 points and
	// un-project them from window coordinates (see glViewport()) to world-space (x,y,z) positions.
	//
	// The NDC box is the rectangular clip box after the homogenous divide where the
	// clip coordinates (after the view-projection transformation) gets converted
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
		{double(d_viewport.x()),                            double(d_viewport.y())                             },
		{double(d_viewport.x() + 0.5 * d_viewport.width()), double(d_viewport.y())                             },
		{double(d_viewport.x() + d_viewport.width()),       double(d_viewport.y())                             },
		{double(d_viewport.x()),                            double(d_viewport.y() + 0.5 * d_viewport.height()) },
		{double(d_viewport.x() + 0.5 * d_viewport.width()), double(d_viewport.y() + 0.5 * d_viewport.height()) },
		{double(d_viewport.x() + d_viewport.width()),       double(d_viewport.y() + 0.5 * d_viewport.height()) },
		{double(d_viewport.x()),                            double(d_viewport.y() + d_viewport.height())       },
		{double(d_viewport.x() + 0.5 * d_viewport.width()), double(d_viewport.y() + d_viewport.height())       },
		{double(d_viewport.x() + d_viewport.width()),       double(d_viewport.y() + d_viewport.height())       }
	};

	// Iterate over all sample points and project onto the unit sphere in world space.
	// Some might miss the sphere (for example, the corner points of the orthographic
	// view frustum when fully zoomed out most likely will miss the unit sphere)
	// but the centre point will always hit (only because the way GPlates currently
	// sets up its projections - we can't rely on this always being the case in which
	// case we'll return the distance from north pole to south pole (for minimum distance) and
	// zero distance (for maximum distance) if nothing hits.
	GPlatesMaths::real_t min_dot_product_pixel_size(1);
	GPlatesMaths::real_t max_dot_product_pixel_size(-1);
	for (int n = 0; n < 9; ++n)
	{
		// Project the same point on the unit sphere.
		const boost::optional<GPlatesMaths::UnitVector3D> projected_pixel =
				project_window_coords_onto_unit_sphere(
						window_xy_coords[n][0],
						window_xy_coords[n][1]);
		if (!projected_pixel)
		{
			continue;
		}

		// Project the sample point plus one pixel (in the x direction) onto the unit sphere.
		// It doesn't matter that the window coordinate might go outside the viewport
		// because there's no clipping happening here.
		const boost::optional<GPlatesMaths::UnitVector3D> projected_pixel_plus_one_x =
				project_window_coords_onto_unit_sphere(
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
		// Here we want the maximum projected pixel size which means minimum dot product.
		if (dot_product_pixel_size_x < min_dot_product_pixel_size)
		{
			min_dot_product_pixel_size = dot_product_pixel_size_x;
		}
		// Here we want the minimum projected pixel size which means maximum dot product.
		if (dot_product_pixel_size_x > max_dot_product_pixel_size)
		{
			max_dot_product_pixel_size = dot_product_pixel_size_x;
		}

		// Project the sample point plus one pixel (in the y direction) onto the unit sphere.
		// It doesn't matter that the window coordinate might go outside the viewport
		// because there's no clipping happening here.
		const boost::optional<GPlatesMaths::UnitVector3D> projected_pixel_plus_one_y =
				project_window_coords_onto_unit_sphere(
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
		// Here we want the maximum projected pixel size which means minimum dot product.
		if (dot_product_pixel_size_y < min_dot_product_pixel_size)
		{
			min_dot_product_pixel_size = dot_product_pixel_size_y;
		}
		// Here we want the minimum projected pixel size which means maximum dot product.
		if (dot_product_pixel_size_y > max_dot_product_pixel_size)
		{
			max_dot_product_pixel_size = dot_product_pixel_size_y;
		}
	}

	// Convert from dot product to arc distance on the unit sphere.
	const double min_distance = acos(max_dot_product_pixel_size).dval();
	const double max_distance = acos(min_dot_product_pixel_size).dval();

	return std::make_pair(min_distance, max_distance);
}
