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

#include "GLCompiledDrawState.h"
#include "GLTexture.h"

#include "gui/Colour.h"

#include "utils/CallStackTracker.h"


namespace GPlatesOpenGL
{
	class GLMatrix;
	class GLRenderer;
	class GLVertexArray;

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
		 * Creates a full-screen quad vertex array with 2D texture coordinates (in [0,1] range).
		 *
		 * The full-screen quad's vertices are stored in an internally created vertex array.
		 *
		 * The returned compiled draw state can be used to draw a full-screen quad in order to apply
		 * a texture to the screen-space of a render target.
		 *
		 * NOTE: This creates a new vertex buffer, vertex element buffer and vertex array so ideally
		 * you should reuse the returned compiled draw state rather than recreating it where possible.
		 */
		GLCompiledDrawState::non_null_ptr_type
		create_full_screen_2D_textured_quad(
				GLRenderer &renderer);


		/**
		 * Creates a full-screen quad vertex array of the specified vertex colour.
		 *
		 * The full-screen quad's vertices are stored in an internally created vertex array.
		 *
		 * The returned compiled draw state can be used to draw a full-screen quad in order to
		 * render a filled colour quad to the screen-space of a render target.
		 *
		 * NOTE: This creates a new vertex buffer, vertex element buffer and vertex array so ideally
		 * you should reuse the returned compiled draw state rather than recreating it where possible.
		 */
		GLCompiledDrawState::non_null_ptr_type
		create_full_screen_2D_coloured_quad(
				GLRenderer &renderer,
				const GPlatesGui::rgba8_t &colour);


		/**
		 * Creates a full-screen quad vertex array that is a combination of
		 * @a create_full_screen_2D_coloured_quad and @a create_full_screen_2D_textured_quad
		 * in that the quad contains texture coordinates and vertex colours.
		 */
		GLCompiledDrawState::non_null_ptr_type
		create_full_screen_2D_coloured_textured_quad(
				GLRenderer &renderer,
				const GPlatesGui::rgba8_t &colour);


		/**
		 * Returns a matrix that converts (x,y) clip-space coordinates in the range [-1,1] to
		 * texture coordinates in the range [0,1].
		 */
		const GLMatrix &
		get_clip_space_to_texture_space_transform();


		/**
		 * Sets renderer state to translate/scale texture coordinates and then look up a texture.
		 *
		 * Does the following:
		 * (1) binds @a texture to texture unit @a texture_unit,
		 * (2) sets the texture environment mode (gl_tex_env) to @a tex_env_mode on texture unit @a texture_unit,
		 * (3) optionally sets the texture matrix to translate/scale texture coordinates.
		 *
		 * The translate/scales parameters are used to adjust texture coordinates.
		 * Note that the scale is multiplied in first followed by the translate.
		 * The default is no translate/scale.
		 */
		void
		set_full_screen_quad_texture_state(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &texture,
				const unsigned int texture_unit = 0,
				const GLint tex_env_mode = GL_REPLACE,
				const boost::optional<const GLMatrix &> &texture_transform_matrix = boost::none,
				const GLenum texture_target = GL_TEXTURE_2D);


		/**
		 * Sets renderer state to map (x,y,z) positions into texture coordinates
		 * using the specified frustum and then look up a texture.
		 *
		 * Does the following:
		 * (1) binds @a texture to texture unit @a texture_unit,
		 * (2) sets the texture environment mode (gl_tex_env) to @a tex_env_mode on texture unit @a texture_unit,
		 * (3) enables, and sets up, object linear texture coordinate generation using @a set_object_linear_tex_gen_state,
		 * (4) optionally sets the texture matrix to map object positions to texture coordinates in the
		 *     range [0,1] according to the frustum defined by @a projection_transform and @a view_transform.
		 *
		 * The translate/scales parameters are used to adjust the post-transform texture coordinates.
		 * Note that the scale is multiplied in first followed by the translate.
		 * The default values convert the clip-space range (-1,1) to texture coord range (0,1)
		 * in order to map to entire texture.
		 */
		void
		set_frustum_texture_state(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &texture,
				const GLMatrix &projection_transform,
				const GLMatrix &view_transform,
				const unsigned int texture_unit = 0,
				const GLint tex_env_mode = GL_REPLACE,
				const boost::optional<const GLMatrix &> &texture_transform_matrix =
						get_clip_space_to_texture_space_transform(),
				const GLenum texture_target = GL_TEXTURE_2D);


		/**
		 * Enables texture coordinate generation and sets the texture transform state on @a renderer to
		 * generate 4D texture coordinates (s,t,r,q) directly (object linear) from vertex (x,y,z) positions.
		 */
		void
		set_object_linear_tex_gen_state(
				GLRenderer &renderer,
				const unsigned int texture_unit = 0);


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
