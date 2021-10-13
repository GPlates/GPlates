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

#ifndef GPLATES_OPENGL_GLFRUSTUM_H
#define GPLATES_OPENGL_GLFRUSTUM_H

#include <vector>
#include <boost/cstdint.hpp>

#include "GLIntersectPrimitives.h"


namespace GPlatesOpenGL
{
	class GLMatrix;

	/**
	 * An array of the six frustum planes that bound a viewing volume.
	 *
	 * The *six* frustum planes represented by the current model-view and projection matrices.
	 *
	 * These frustum planes are in model-space (before any model-view or projection
	 * transformations are applied) also called object-space.
	 *
	 * The planes can be used for frustum culling (culling objects not visible inside the current view frustum).
	 */
	class GLFrustum
	{
	public:
		/**
		 * The specific frustum planes.
		 *
		 * NOTE: Don't change the order of these enums - the code assumes this ordering and will break if changed.
		 */
		enum PlaneType
		{
			LEFT_PLANE = 0,
			RIGHT_PLANE,
			BOTTOM_PLANE,
			TOP_PLANE,
			NEAR_PLANE,
			FAR_PLANE,

			NUM_PLANES
		};


		/**
		 * Bitmask to indicate all frustum planes are active.
		 */
		static const boost::uint32_t ALL_PLANES_ACTIVE_MASK = 0x3f;


		//! Default constructor initialises planes using identity model-view and projection matrices.
		GLFrustum();


		/**
		 * Initialises planes using the specified model-view and projection matrices.
		 *
		 * These planes form the boundary of the frustum of the view volume in model space.
		 */
		GLFrustum(
				const GLMatrix &model_view_matrix,
				const GLMatrix &projection_matrix);


		/**
		 * Initialises planes using the identity model-view and projection matrices.
		 */
		void
		set_identity_model_view_projection();


		/**
		 * Initialises planes using the specified model-view and projection matrices.
		 *
		 * These planes form the boundary of the frustum of the view volume in model space.
		 */
		void
		set_model_view_projection(
				const GLMatrix &model_view_matrix,
				const GLMatrix &projection_matrix);


		//
		// NOTE: The plane normals point towards the *inside* of the view frustum
		// volume and hence the view frustum is defined by the intersection of the
		// positive half-spaces of these planes.
		//
		// NOTE: These planes do *not* have *unit* vector normals.
		//


		/**
		 * Returns the specified frustum plane.
		 */
		const GLIntersect::Plane &
		get_plane(
				PlaneType plane) const
		{
			return d_planes[plane];
		}


		/**
		 * Returns the frustum planes.
		 *
		 * NOTE: @a NUM_PLANES - 1 is the highest index you should use on the returned array.
		 */
		const GLIntersect::Plane *
		get_planes() const
		{
			return &d_planes[0];
		}


	private:
		/**
		 * The frustum planes for the identify model-view-projection.
		 */
		static const GLIntersect::Plane IDENTITY_FRUSTUM_PLANES[6];


		/**
		 * The left, right, bottom, top, near and far frustum planes.
		 */
		std::vector<GLIntersect::Plane> d_planes;
	};
}

#endif // GPLATES_OPENGL_GLFRUSTUM_H
