/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "GLFrustum.h"

#include "GLMatrix.h"


namespace GPlatesOpenGL
{
	namespace
	{
		//! Returns the left clip plane - @a mvp is model-view-projection matrix.
		GLIntersect::Plane
		get_left_plane(
				const GLMatrix &mvp)
		{
			return GLIntersect::Plane(
					mvp.get_element(3,0) + mvp.get_element(0,0),
					mvp.get_element(3,1) + mvp.get_element(0,1),
					mvp.get_element(3,2) + mvp.get_element(0,2),
					mvp.get_element(3,3) + mvp.get_element(0,3));
		}

		//! Returns the right clip plane - @a mvp is model-view-projection matrix.
		GLIntersect::Plane
		get_right_plane(
				const GLMatrix &mvp)
		{
			return GLIntersect::Plane(
					mvp.get_element(3,0) - mvp.get_element(0,0),
					mvp.get_element(3,1) - mvp.get_element(0,1),
					mvp.get_element(3,2) - mvp.get_element(0,2),
					mvp.get_element(3,3) - mvp.get_element(0,3));
		}

		//! Returns the bottom clip plane - @a mvp is model-view-projection matrix.
		GLIntersect::Plane
		get_bottom_plane(
				const GLMatrix &mvp)
		{
			return GLIntersect::Plane(
					mvp.get_element(3,0) + mvp.get_element(1,0),
					mvp.get_element(3,1) + mvp.get_element(1,1),
					mvp.get_element(3,2) + mvp.get_element(1,2),
					mvp.get_element(3,3) + mvp.get_element(1,3));
		}

		//! Returns the top clip plane - @a mvp is model-view-projection matrix.
		GLIntersect::Plane
		get_top_plane(
				const GLMatrix &mvp)
		{
			return GLIntersect::Plane(
					mvp.get_element(3,0) - mvp.get_element(1,0),
					mvp.get_element(3,1) - mvp.get_element(1,1),
					mvp.get_element(3,2) - mvp.get_element(1,2),
					mvp.get_element(3,3) - mvp.get_element(1,3));
		}

		//! Returns the near clip plane - @a mvp is model-view-projection matrix.
		GLIntersect::Plane
		get_near_plane(
				const GLMatrix &mvp)
		{
			return GLIntersect::Plane(
					mvp.get_element(3,0) + mvp.get_element(2,0),
					mvp.get_element(3,1) + mvp.get_element(2,1),
					mvp.get_element(3,2) + mvp.get_element(2,2),
					mvp.get_element(3,3) + mvp.get_element(2,3));
		}

		//! Returns the far clip plane - @a mvp is model-view-projection matrix.
		GLIntersect::Plane
		get_far_plane(
				const GLMatrix &mvp)
		{
			return GLIntersect::Plane(
					mvp.get_element(3,0) - mvp.get_element(2,0),
					mvp.get_element(3,1) - mvp.get_element(2,1),
					mvp.get_element(3,2) - mvp.get_element(2,2),
					mvp.get_element(3,3) - mvp.get_element(2,3));
		}
	}
}


// NOTE: These should be in the same order as specified by the 'PlaneType' enum.
const GPlatesOpenGL::GLIntersect::Plane GPlatesOpenGL::GLFrustum::IDENTITY_FRUSTUM_PLANES[6] =
{
	GLIntersect::Plane(1, 0, 0, 1),  // left plane
	GLIntersect::Plane(-1, 0, 0, 1), // right plane
	GLIntersect::Plane(0, 1, 0, 1),  // bottom plane
	GLIntersect::Plane(0, -1, 0, 1), // top plane
	GLIntersect::Plane(0, 0, 1, 1),  // near plane
	GLIntersect::Plane(0, 0, -1, 1)  // far plane
};


GPlatesOpenGL::GLFrustum::GLFrustum()
{
	d_planes.reserve(6);

	for (unsigned int plane_index = 0; plane_index < NUM_PLANES; ++plane_index)
	{
		d_planes.push_back(IDENTITY_FRUSTUM_PLANES[plane_index]);
	}
}


GPlatesOpenGL::GLFrustum::GLFrustum(
		const GLMatrix &model_view_matrix,
		const GLMatrix &projection_matrix)
{
	// Multiply the model-view and projection matrices.
	// When we extract frustum planes from this combined matrix they will be
	// in model-space (also called object-space).
	GLMatrix mvp(projection_matrix);
	mvp.gl_mult_matrix(model_view_matrix);

	//
	// From "Fast extraction of viewing frustum planes from the world-view-projection matrix"
	// by Gil Gribb and Klaus Hartmann.
	//

	// NOTE: The plane normals point towards the *inside* of the view frustum
	// volume and hence the view frustum is defined by the intersection of the
	// positive half-spaces of these planes.

	// NOTE: These planes do not have *unit* vector normals.

	d_planes.reserve(6);

	// Left clipping plane.
	d_planes.push_back(get_left_plane(mvp));

	// Right clipping plane.
	d_planes.push_back(get_right_plane(mvp));

	// Bottom clipping plane.
	d_planes.push_back(get_bottom_plane(mvp));

	// Top clipping plane.
	d_planes.push_back(get_top_plane(mvp));

	// Near clipping plane.
	d_planes.push_back(get_near_plane(mvp));

	// Far clipping plane.
	d_planes.push_back(get_far_plane(mvp));
}


void
GPlatesOpenGL::GLFrustum::set_identity_model_view_projection()
{
	for (unsigned int plane_index = 0; plane_index < NUM_PLANES; ++plane_index)
	{
		d_planes[plane_index] = IDENTITY_FRUSTUM_PLANES[plane_index];
	}
}


void
GPlatesOpenGL::GLFrustum::set_model_view_projection(
		const GLMatrix &model_view_matrix,
		const GLMatrix &projection_matrix)
{
	// Multiply the model-view and projection matrices.
	// When we extract frustum planes from this combined matrix they will be
	// in model-space (also called object-space).
	GLMatrix mvp(projection_matrix);
	mvp.gl_mult_matrix(model_view_matrix);

	//
	// From "Fast extraction of viewing frustum planes from the world-view-projection matrix"
	// by Gil Gribb and Klaus Hartmann.
	//

	// NOTE: The plane normals point towards the *inside* of the view frustum
	// volume and hence the view frustum is defined by the intersection of the
	// positive half-spaces of these planes.

	// NOTE: These planes do not have *unit* vector normals.

	// Left clipping plane.
	d_planes[LEFT_PLANE] = get_left_plane(mvp);

	// Right clipping plane.
	d_planes[RIGHT_PLANE] = get_right_plane(mvp);

	// Bottom clipping plane.
	d_planes[BOTTOM_PLANE] = get_bottom_plane(mvp);

	// Top clipping plane.
	d_planes[TOP_PLANE] = get_top_plane(mvp);

	// Near clipping plane.
	d_planes[NEAR_PLANE] = get_near_plane(mvp);

	// Far clipping plane.
	d_planes[FAR_PLANE] = get_far_plane(mvp);
}
