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
 
#ifndef GPLATES_OPENGL_GLUTILS_H
#define GPLATES_OPENGL_GLUTILS_H

#include <opengl/OpenGL.h>
#include <boost/optional.hpp>

#include "GLTexture.h"
#include "GLVertexArrayDrawable.h"

#include "utils/CallStackTracker.h"


namespace GPlatesOpenGL
{
	class GLCompositeStateSet;
	class GLMatrix;
	class GLTextureTransformState;

	namespace GLUtils
	{
		/**
		 * Asserts that no OpenGL errors (see glGetError) have been recorded.
		 *
		 * If an error is detected then this function aborts for debug builds and
		 * throws @a OpenGLException (with the specific OpenGL error message) for release builds.
		 */
		void
		assert_no_gl_errors(
				const GPlatesUtils::CallStack::Trace &assert_location);


		/**
		 * Creates a full-screen quad drawable with 2D texture coordinates (in [0,1] range).
		 *
		 * 
		 */
		GLVertexArrayDrawable::non_null_ptr_type
		create_full_screen_2D_textured_quad();


		/**
		 * Returns a matrix that converts (x,y) clip-space coordinates in the range [-1,1] to
		 * texture coordinates in the range [0,1].
		 */
		const GLMatrix &
		get_clip_space_to_texture_space_transform();


		/**
		 * Creates state sets that translate/scale texture coordinates and then looks up a texture.
		 *
		 * The created state sets are then added to @a state_set.
		 *
		 * The state sets created are:
		 * (1) GLBindTextureState - binds @a texture to texture unit @a texture_unit,
		 * (2) GLTextureEnvironmentState - sets gl_tex_env_mode to @a tex_env_mode on
		 *     texture unit @a texture_unit,
		 * (3) GLTextureTransformState - sets the texture matrix to translate/scale texture coordinates.
		 *
		 * The translate/scales parameters are used to adjust texture coordinates.
		 * Note that the scale is multiplied in first followed by the translate.
		 * The default is no translate/scale.
		 */
		void
		set_full_screen_quad_texture_state(
				GLCompositeStateSet &state_set,
				const GLTexture::shared_ptr_to_const_type &texture,
				const unsigned int texture_unit = 0,
				const GLint tex_env_mode = GL_REPLACE,
				const boost::optional<const GLMatrix &> &texture_transform_matrix = boost::none);


		/**
		 * Creates state sets that map (x,y,z) positions into texture coordinates
		 * using the specified frustum and then looks up a texture.
		 *
		 * The created state sets are then added to @a state_set.
		 *
		 * The state sets created are:
		 * (1) GLBindTextureState - binds @a texture to texture unit @a texture_unit,
		 * (2) GLTextureEnvironmentState - sets gl_tex_env_mode to @a tex_env_mode on
		 *     texture unit @a texture_unit,
		 * (3) GLTextureTransformState - sets texture transform state using
		 *     @a set_object_linear_tex_gen_state (above) and sets the texture matrix
		 *     to map object positions to texture coordinates in the range [0,1] according
		 *     to the frustum defined by @a projection_transform and @a view_transform.
		 *
		 * The translate/scales parameters are used to adjust the post-transform texture coordinates.
		 * Note that the scale is multiplied in first followed by the translate.
		 * The default values convert the clip-space range (-1,1) to texture coord range (0,1)
		 * in order to map to entire texture.
		 */
		void
		set_frustum_texture_state(
				GLCompositeStateSet &state_set,
				const GLTexture::shared_ptr_to_const_type &texture,
				const GLMatrix &projection_transform,
				const GLMatrix &view_transform,
				const unsigned int texture_unit = 0,
				const GLint tex_env_mode = GL_REPLACE,
				const boost::optional<const GLMatrix &> &texture_transform_matrix =
						get_clip_space_to_texture_space_transform());


		/**
		 * Sets the texture transform state on @a texture_transform_state to
		 * automatically generate 4D texture coordinates (s,t,r,q) directly (object linear)
		 * from vertex (x,y,z) positions.
		 *
		 * NOTE: This does not affect the active texture unit or
		 * texture matrix of @a texture_transform_state.
		 */
		void
		set_object_linear_tex_gen_state(
				GLTextureTransformState &texture_transform_state);


		/**
		 * Used to scale/translate the clip space [-1, 1] coordinates of a quad tree node
		 * relative to its ancestor node.
		 *
		 * This is for situations when a tile is part of an ancestor tile and the ancestor tile
		 * clip space range of [-1, 1] needs to be adjusted such that the descendant tile
		 * now has a clip space range of [-1, 1].
		 *
		 * NOTE: This is the reverse of what @a QuadTreeUVTransform does with the addition
		 * that here we work in the [-1, 1] range whereas u/v works in the [0, 1] range.
		 */
		class QuadTreeClipSpaceTransform
		{
		public:
			//! Identity transformation.
			QuadTreeClipSpaceTransform();

			//! Scale/translate this quad tree child node relative to the identity transformation.
			QuadTreeClipSpaceTransform(
					unsigned int x_offset,
					unsigned int y_offset);

			//! Scale/translate this quad tree child node relative to its parent.
			QuadTreeClipSpaceTransform(
					const QuadTreeClipSpaceTransform &parent_clip_space_transform,
					unsigned int x_offset,
					unsigned int y_offset);

			/**
			 * Post-multiplies @a matrix with the appropriate scale and translation.
			 */
			void
			transform(
					GLMatrix &matrix) const;

		private:
			double d_scale;
			double d_inverse_scale; // Avoids costly division.
			double d_translate_x;
			double d_translate_y;
		};


		/**
		 * Used to scale/translate texture coordinates of a quad tree node relative
		 * to its ancestor node.
		 *
		 * This is for situations when a tile is partially covered by an ancestor tile and hence
		 * needs texture coordinate adjustments to locate the correct sub-part of the ancestor tile.
		 *
		 * NOTE: This is the reverse of what @a QuadTreeClipSpaceTransform does with the addition
		 * that here we work in the [0, 1] range whereas clip space works in the [-1, 1] range.
		 */
		class QuadTreeUVTransform
		{
		public:
			//! Identity transformation.
			QuadTreeUVTransform();

			//! Scale/translate this quad tree child node relative to the identity transformation.
			QuadTreeUVTransform(
					unsigned int u_offset,
					unsigned int v_offset);

			//! Scale/translate this quad tree child node relative to its parent.
			QuadTreeUVTransform(
					const QuadTreeUVTransform &parent_uv_transform,
					unsigned int u_offset,
					unsigned int v_offset);

			/**
			 * Post-multiplies @a matrix with the appropriate scale and translation.
			 */
			void
			transform(
					GLMatrix &matrix) const;

		private:
			double d_scale;
			double d_translate_u;
			double d_translate_v;
		};
	}
}

#endif // GPLATES_OPENGL_GLUTILS_H
