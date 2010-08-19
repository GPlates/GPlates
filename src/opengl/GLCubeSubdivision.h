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
 
#ifndef GPLATES_OPENGL_GLCUBESUBDIVISION_H
#define GPLATES_OPENGL_GLCUBESUBDIVISION_H

#include <cstdlib> // For std::size_t
#include <opengl/OpenGL.h>

#include "GLTransform.h"

#include "maths/UnitVector3D.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Defines a quad-tree subdivision of each face of a cube with optional overlapping extents.
	 */
	class GLCubeSubdivision :
			public GPlatesUtils::ReferenceCount<GLCubeSubdivision>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLCubeSubdivision.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLCubeSubdivision> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCubeSubdivision.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLCubeSubdivision> non_null_ptr_to_const_type;


		/**
		 * Identifies a face of the cube.
		 *
		 * These can be used as indices in your own arrays.
		 */
		enum CubeFaceType
		{
			POSITIVE_X = 0,
			NEGATIVE_X,
			POSITIVE_Y,
			NEGATIVE_Y,
			POSITIVE_Z,
			NEGATIVE_Z,

			NUM_FACES
		};


		/**
		 * The default tile dimension is 256.
		 *
		 * This size gives us a small enough tile region on the globe to make good use
		 * of view frustum culling of tiles.
		 */
		static const std::size_t DEFAULT_TILE_TEXEL_DIMENSION = 256;


		/**
		 * Creates a @a GLCubeSubdivision object.
		 *
		 * @a zNear and @a zFar specify the near and far distance (positive) of
		 * the view frustums generated for each subdivision.
		 */
		static
		non_null_ptr_type
		create(
				std::size_t tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION,
				GLdouble zNear = 0.5,
				GLdouble zFar = 2.0)
		{
			return non_null_ptr_type(new GLCubeSubdivision(tile_texel_dimension, zNear, zFar));
		}


		/**
		 * Returns the projection matrix used to render a scene into a subdivision tile.
		 *
		 * @param level_of_detail represents the level of subdivision (0 means the whole cube face).
		 * @param tile_u_offset represents the offset along the texture 'u' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 * @param tile_v_offset represents the offset along the texture 'v' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 */
		GLTransform::non_null_ptr_to_const_type
		get_projection_transform(
				CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const;


		/**
		 * Returns the view matrix used to render a scene into a subdivision tile.
		 *
		 * @param level_of_detail represents the level of subdivision (0 means the whole cube face).
		 * @param tile_u_offset represents the offset along the texture 'u' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 * @param tile_v_offset represents the offset along the texture 'v' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 */
		GLTransform::non_null_ptr_to_const_type
		get_view_transform(
				CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const;


		/**
		 * Returns the texel dimension of any tile at any subdivision level and uv offset.
		 */
		std::size_t
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}

	private:
		/**
		 * Standard directions used by 3D graphics APIs for cube map textures so we'll adopt
		 * the same convention.
		 */
		static const GPlatesMaths::UnitVector3D UV_FACE_DIRECTIONS[6][2];

		/**
		 * The normal vectors pointing outwards from each face.
		 */
		static const GPlatesMaths::UnitVector3D FACE_NORMALS[6];

		/**
		 * The dimensions of a subdivision tile in texels.
		 */
		std::size_t d_tile_texel_dimension;

		//! Frustum near plane distance.
		GLdouble d_near;
		//! Frustum far plane distance.
		GLdouble d_far;

		//! Constructor.
		GLCubeSubdivision(
				std::size_t tile_texel_dimension,
				GLdouble zNear,
				GLdouble zFar) :
			d_tile_texel_dimension(tile_texel_dimension),
			d_near(zNear),
			d_far(zFar)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLCUBESUBDIVISION_H
