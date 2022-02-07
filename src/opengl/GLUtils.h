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
#include "GLMatrix.h"
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
		 * Creates a full-screen quad vertex array with 2D texture coordinates (in [0,1] range) and
		 * with all vertices containing the colour white - RGBA(1.0, 1.0, 1.0, 1.0).
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
		 * Creates a full-screen quad vertex array with 2D texture coordinates (in [0,1] range) and
		 * with all vertices containing the specified vertex colour.
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
		 * relative to its ancestor node (or vice versa).
		 *
		 * This is for situations when a tile is part of an ancestor tile. There are two cases:
		 *   Regular - The ancestor tile has a clip space range of [-1, 1] and the descendant tile
		 *             needs a clip space range of [-1, 1]. In this case a transformation
		 *             needs to be applied to the ancestor tile as a post-projection (ie, applied
		 *             after the ancestor tile's regular projection transform) in order to get the
		 *             descendant tile's projection transform.
		 *   Inverse - The descendant tile has a clip space range of [-1, 1] and the ancestor tile
		 *             needs a clip space range of [-1, 1]. In this case a transformation
		 *             needs to be applied to the descendant tile as a post-projection (ie, applied
		 *             after the descendant tile's regular projection transform) in order to get the
		 *             ancestor tile's projection transform.
		 *
		 * The math behind the scales/translates can be explained by the following matrix multiplication:
		 *    M = inverse(Sr) * S * T * Sr
		 * where S is scale due to quad tree, T is translate due to quad tree and Sr is
		 * scale due to the expand tile ratio (note that it also includes a factor of 2 or 0.5 for
		 * the 'loose' versions). For a description of T see 'GLCubeSubdivision::create_projection_transform()'.
		 * In this class this is condensed down to:
		 *    M = T' * S'
		 * where T' and S' are combinations of Sr, S and T.
		 * This amounts to a nice multiply-accumulate operation:
		 *    vector2 = M * vector1
		 *            = S' * vector1 + T'
		 * It turns out that inverse(Sr) and Sr cancel out in the scale S' but still apply in the
		 * translate T' such that the components satisfy:
		 *    Ti' = (Si/Sri) * Ti
		 * So:
		 *    vector2 = S' * vector1 + T'
		 *            = S * vector1 + (S/Sr) * T
		 *
		 * The 'inverse' case is:
		 *    inverse(M) = inverse(Sr) * inverse(T) * inverse(S) * Sr
		 * In the interface this is condensed down to the form:
		 *    inverse(M) = T" * S"
		 * where T" and S" are combinations of 'Sr', 'S' and 'T'.
		 *
		 * Also includes regular and inverse 'loose' tiles (see @a GLCubeSubdivision for more details).
		 */
		class QuadTreeClipSpaceTransform
		{
		public:
			/**
			 * Calculates a tile expand ratio for a tile of the specified texel dimension and
			 * the desired overlap in units of texels.
			 *
			 * For example to ensure the centres of border texels lie exactly on the tile
			 * boundaries set @a tile_border_overlap_in_texels to '0.5'.
			 *
			 * NOTE: This is the equivalent of 'GLCubeSubdivision::get_expand_frustum_ratio()'.
			 */
			static
			double
			get_expand_tile_ratio(
					unsigned int tile_texel_dimension,
					const double &tile_border_overlap_in_texels)
			{
				return tile_texel_dimension / (tile_texel_dimension - 2.0 * tile_border_overlap_in_texels);
			}


			/**
			 * Identity transformation.
			 *
			 * The child transforms are always relative to the root node which is always the identity transform.
			 */
			QuadTreeClipSpaceTransform(
					const double &expand_tile_ratio = 1.0);

			/**
			 * Scale/translate this quad tree child node relative to its parent.
			 */
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

			/**
			 * Same as @a transform but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			void
			loose_transform(
					GLMatrix &matrix) const;

			/**
			 * Post-multiplies @a matrix with the inverse (of the appropriate scale and translation).
			 */
			void
			inverse_transform(
					GLMatrix &matrix) const;

			/**
			 * Same as @a inverse_transform but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			void
			inverse_loose_transform(
					GLMatrix &matrix) const;

			/**
			 * Returns the scale part of the transform (same for 'x' and 'y' scale).
			 *
			 * NOTE: The scale should be applied *before* the translate.
			 */
			const double &
			get_scale() const
			{
				return d_scale;
			}

			/**
			 * Same as @a get_scale but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			const double &
			get_loose_scale() const
			{
				// Same as scale.
				return get_scale();
			}

			/**
			 * Returns the scale part of the *inverse* transform (same for 'x' and 'y' scale).
			 *
			 * NOTE: The scale should be applied *before* the translate.
			 */
			const double &
			get_inverse_scale() const
			{
				return d_inverse_scale;
			}

			/**
			 * Same as @a get_scale but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			const double &
			get_inverse_loose_scale() const
			{
				// Same as inverse scale.
				return get_inverse_scale();
			}

			/**
			 * Returns the 'x' translate part of the transform.
			 *
			 * NOTE: The translate should be applied *after* the scale.
			 */
			double
			get_translate_x() const
			{
				// This basically comes from 'GLCubeSubdivision::create_projection_transform()'.
				return d_scale * d_inverse_expand_tile_ratio *
					(1 - (2.0 * d_relative_x_node_offset + 1) * d_inverse_scale);
			}

			/**
			 * Same as @a get_translate_x but for the 'y' component (instead of the 'x' component).
			 */
			double
			get_translate_y() const
			{
				return d_scale * d_inverse_expand_tile_ratio *
					(1 - (2.0 * d_relative_y_node_offset + 1) * d_inverse_scale);
			}

			/**
			 * Same as @a get_translate_x but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_loose_translate_x() const
			{
				// The 0.5 has the same effect as multiplying 'd_expand_tile_ratio' by 2.0 to account for looseness.
				return 0.5 * get_translate_x();
			}

			/**
			 * Same as @a get_translate_y but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_loose_translate_y() const
			{
				// The 0.5 has the same effect as multiplying 'd_expand_tile_ratio' by 2.0 to account for looseness.
				return 0.5 * get_translate_y();
			}

			/**
			 * Returns the 'x' translate part of the *inverse* transform.
			 *
			 * NOTE: The translate should be applied *after* the scale.
			 */
			double
			get_inverse_translate_x() const
			{
				// NOTE: We multiply by 'scale' because the inverse of
				//   M = T * S
				// is
				//   inverse(M) = inverse(S) * inverse(T)
				//              = T' * S'
				// which involves scaling the translation (since the order has been reversed by the inverse).
				return get_inverse_scale() * -get_translate_x();
			}

			/**
			 * Same as @a get_inverse_translate_x but for the 'y' component (instead of the 'x' component).
			 */
			double
			get_inverse_translate_y() const
			{
				// NOTE: We multiply by 'scale' because the inverse of
				//   M = T * S
				// is
				//   inverse(M) = inverse(S) * inverse(T)
				//              = T' * S'
				// which involves scaling the translation (since the order has been reversed by the inverse).
				return get_inverse_scale() * -get_translate_y();
			}

			/**
			 * Same as @a get_inverse_translate_x but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_inverse_loose_translate_x() const
			{
				// NOTE: We multiply by 'scale' because the inverse of
				//   M = T * S
				// is
				//   inverse(M) = inverse(S) * inverse(T)
				//              = T' * S'
				// which involves scaling the translation (since the order has been reversed by the inverse).
				return get_inverse_loose_scale() * -get_loose_translate_x();
			}

			/**
			 * Same as @a get_translate_y but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_inverse_loose_translate_y() const
			{
				// NOTE: We multiply by 'scale' because the inverse of
				//   M = T * S
				// is
				//   inverse(M) = inverse(S) * inverse(T)
				//              = T' * S'
				// which involves scaling the translation (since the order has been reversed by the inverse).
				return get_inverse_loose_scale() * -get_loose_translate_y();
			}

		private:
			double d_expand_tile_ratio;
			double d_inverse_expand_tile_ratio;
			double d_scale;
			double d_inverse_scale; // To avoid extra divisions which are a lot more costly than multiplications.
			unsigned int d_relative_x_node_offset;
			unsigned int d_relative_y_node_offset;
		};


		/**
		 * Used to scale/translate texture coordinates of a descendant quad tree node relative
		 * to its ancestor node (and vice versa).
		 *
		 * This class is very similar to @a QuadTreeClipSpaceTransform except that it uses the
		 * texture coordinate range [0,1] instead of the clip-space range [-1,1].
		 *
		 * An example use of this class is when a tile is partially covered by an ancestor tile and
		 * hence needs texture coordinate adjustments to locate the correct sub-part of the ancestor tile.
		 * In this case it would use the 'inverse' transform.
		 *
		 * The math behind the scales/translates can be explained by the following matrix multiplication:
		 *    M = T2uv * S2uv * T * S * T1uv * S1uv
		 * where S and T are the scale/translations generated by @a QuadTreeClipSpaceTransform because
		 * we're delegating most of the work to that class while we just use T1uv and S1uv to
		 * convert from the texture coordinate range [0,1] to the clip-space range [-1,1].
		 * T2uv and S2uv do the reverse conversion from [-1,1] back to [0,1].
		 * In this class this is condensed down to:
		 *    M = T' * S'
		 * where T' and S' are combinations of T2uv, S2uv, T, S, T1uv and S1uv.
		 * This amounts to a nice multiply-accumulate operation:
		 *    vector2 = M * vector1
		 *            = S' * vector1 + T'
		 * It turns out that S' ends up just being S (the other scales cancel each other out) and the
		 * translate T' is such that the components satisfy:
		 *    Ti' = 0.5 * [Ti - Si + 1]
		 * So:
		 *    vector2 = S' * vector1 + T'
		 *            = S * vector1 + 0.5 * [T - S + 1]
		 */
		class QuadTreeUVTransform
		{
		public:
			/**
			 * Calculates a tile expand ratio for a tile of the specified texel dimension and
			 * the desired overlap in units of texels.
			 *
			 * For example to ensure the centres of border texels lie exactly on the tile
			 * boundaries set @a tile_border_overlap_in_texels to '0.5'.
			 *
			 * NOTE: This is the equivalent of 'GLCubeSubdivision::get_expand_frustum_ratio()'.
			 */
			static
			double
			get_expand_tile_ratio(
					unsigned int tile_texel_dimension,
					const double &tile_border_overlap_in_texels)
			{
				return tile_texel_dimension / (tile_texel_dimension - 2.0 * tile_border_overlap_in_texels);
			}


			/**
			 * Identity transformation.
			 *
			 * The child transforms are always relative to the root node which is always the identity transform.
			 */
			QuadTreeUVTransform(
					const double &expand_tile_ratio = 1.0);

			/**
			 * Scale/translate this quad tree child node relative to its parent.
			 */
			QuadTreeUVTransform(
					const QuadTreeUVTransform &parent_uv_transform,
					unsigned int x_offset,
					unsigned int y_offset);

			/**
			 * Post-multiplies @a matrix with the appropriate scale and translation.
			 */
			void
			transform(
					GLMatrix &matrix) const;

			/**
			 * Same as @a transform but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			void
			loose_transform(
					GLMatrix &matrix) const;

			/**
			 * Post-multiplies @a matrix with the inverse (of the appropriate scale and translation).
			 */
			void
			inverse_transform(
					GLMatrix &matrix) const;

			/**
			 * Same as @a inverse_transform but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			void
			inverse_loose_transform(
					GLMatrix &matrix) const;

			/**
			 * Returns the scale part of the transform (same for 'u' and 'v' scale).
			 *
			 * NOTE: The scale should be applied *before* the translate.
			 */
			const double &
			get_scale() const
			{
				return d_clip_space_transform.get_scale();
			}

			/**
			 * Same as @a get_scale but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			const double &
			get_loose_scale() const
			{
				return d_clip_space_transform.get_loose_scale();
			}

			/**
			 * Returns the scale part of the *inverse* transform (same for 'u' and 'v' scale).
			 *
			 * NOTE: The scale should be applied *before* the translate.
			 */
			const double &
			get_inverse_scale() const
			{
				return d_clip_space_transform.get_inverse_scale();
			}

			/**
			 * Same as @a get_scale but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			const double &
			get_inverse_loose_scale() const
			{
				return d_clip_space_transform.get_inverse_loose_scale();
			}

			/**
			 * Returns the 'u' translate part of the transform.
			 *
			 * NOTE: The translate should be applied *after* the scale.
			 */
			double
			get_translate_u() const
			{
				return 0.5 * (d_clip_space_transform.get_translate_x() - d_clip_space_transform.get_scale() + 1);
			}

			/**
			 * Same as @a get_translate_u but for the 'v' component (instead of the 'u' component).
			 */
			double
			get_translate_v() const
			{
				return 0.5 * (d_clip_space_transform.get_translate_y() - d_clip_space_transform.get_scale() + 1);
			}

			/**
			 * Same as @a get_translate_u but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_loose_translate_u() const
			{
				return 0.5 * (d_clip_space_transform.get_loose_translate_x() - d_clip_space_transform.get_loose_scale() + 1);
			}

			/**
			 * Same as @a get_translate_v but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_loose_translate_v() const
			{
				return 0.5 * (d_clip_space_transform.get_loose_translate_y() - d_clip_space_transform.get_loose_scale() + 1);
			}

			/**
			 * Returns the 'u' translate part of the *inverse* transform.
			 *
			 * NOTE: The translate should be applied *after* the scale.
			 */
			double
			get_inverse_translate_u() const
			{
				// NOTE: We multiply by 'scale' because the inverse of
				//   M = T * S
				// is
				//   inverse(M) = inverse(S) * inverse(T)
				//              = T' * S'
				// which involves scaling the translation (since the order has been reversed by the inverse).
				return get_inverse_scale() * -get_translate_u();
			}

			/**
			 * Same as @a get_inverse_translate_u but for the 'v' component (instead of the 'u' component).
			 */
			double
			get_inverse_translate_v() const
			{
				// NOTE: We multiply by 'scale' because the inverse of
				//   M = T * S
				// is
				//   inverse(M) = inverse(S) * inverse(T)
				//              = T' * S'
				// which involves scaling the translation (since the order has been reversed by the inverse).
				return get_inverse_scale() * -get_translate_v();
			}

			/**
			 * Same as @a get_inverse_translate_u but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_inverse_loose_translate_u() const
			{
				return 0.5 * (d_clip_space_transform.get_inverse_loose_translate_x() - d_clip_space_transform.get_inverse_loose_scale() + 1);
			}

			/**
			 * Same as @a get_translate_v but suitable for a 'loose' tile - see @a GLCubeSubdivision for more details.
			 */
			double
			get_inverse_loose_translate_v() const
			{
				return 0.5 * (d_clip_space_transform.get_inverse_loose_translate_y() - d_clip_space_transform.get_inverse_loose_scale() + 1);
			}

		private:
			//! Delegate the core quad-tree scaling/translation to @a QuadTreeClipSpaceTransform.
			QuadTreeClipSpaceTransform d_clip_space_transform;
		};
	}
}

#endif // GPLATES_OPENGL_GLUTILS_H
