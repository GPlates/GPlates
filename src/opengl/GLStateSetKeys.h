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

#ifndef GPLATES_OPENGL_GLSTATESETKEYS_H
#define GPLATES_OPENGL_GLSTATESETKEYS_H

#include <qopengl.h>  // For OpenGL constants and typedefs.

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLCapabilities;

	/**
	 * Used to assign a separate slot for each @a GLStateSet derived state.
	 */
	class GLStateSetKeys :
			public GPlatesUtils::ReferenceCount<GLStateSetKeys>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateSetKeys> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateSetKeys> non_null_ptr_to_const_type;

		//! Typedef for a state set key.
		typedef unsigned int key_type;


		/**
		 * Creates an immutable @a GLStateSetKeys object.
		 */
		static
		non_null_ptr_to_const_type
		create(
				const GLCapabilities &capabilities)
		{
			return non_null_ptr_type(new GLStateSetKeys(capabilities));
		}


		/**
		 * Returns the total number of state set keys.
		 */
		unsigned int
		get_num_state_set_keys() const
		{
			return d_num_state_set_keys;
		}


		/**
		 * These keys can be used directly (though some are used via @a get_capability_key for example).
		 */
		enum
		{
			KEY_ACTIVE_TEXTURE,
			KEY_ALPHA_FUNC,
			KEY_BIND_ARRAY_BUFFER,
			KEY_BIND_COPY_READ_BUFFER,
			KEY_BIND_COPY_WRITE_BUFFER,
			KEY_BIND_ELEMENT_ARRAY_BUFFER,
			KEY_BIND_FRAMEBUFFER,
			KEY_BIND_PIXEL_PACK_BUFFER,
			KEY_BIND_PIXEL_UNPACK_BUFFER,
			KEY_BIND_RENDERBUFFER,
			KEY_BIND_TEXTURE_BUFFER,
			KEY_BIND_TRANSFORM_FEEDBACK_BUFFER,
			KEY_BIND_UNIFORM_BUFFER,
			KEY_BIND_VERTEX_ARRAY,
			KEY_BLEND_COLOR,
			KEY_BLEND_EQUATION,
			KEY_BLEND_FUNC,
			KEY_CLAMP_READ_COLOR,
			KEY_CLEAR_COLOR,
			KEY_CLEAR_DEPTH,
			KEY_CLEAR_STENCIL,
			KEY_COLOR_MASK,
			KEY_CULL_FACE,
			KEY_DEPTH_FUNC,
			KEY_DEPTH_MASK,
			KEY_DEPTH_RANGE,
			KEY_DRAW_BUFFERS,
			KEY_ENABLE_BLEND,
			KEY_ENABLE_CULL_FACE,
			KEY_ENABLE_DEPTH_CLAMP,
			KEY_ENABLE_DEPTH_TEST,
			KEY_ENABLE_FRAMEBUFFER_SRGB,
			KEY_ENABLE_LINE_SMOOTH,
			KEY_ENABLE_MULTISAMPLE,
			KEY_ENABLE_POLYGON_OFFSET_FILL,
			KEY_ENABLE_POLYGON_OFFSET_LINE,
			KEY_ENABLE_POLYGON_OFFSET_POINT,
			KEY_ENABLE_POLYGON_SMOOTH,
			KEY_ENABLE_PRIMITIVE_RESTART,
			KEY_ENABLE_PROGRAM_POINT_SIZE,
			KEY_ENABLE_RASTERIZER_DISCARD,
			KEY_ENABLE_SAMPLE_ALPHA_TO_COVERAGE,
			KEY_ENABLE_SAMPLE_ALPHA_TO_ONE,
			KEY_ENABLE_SAMPLE_COVERAGE,
			KEY_ENABLE_SAMPLE_MASK,
			KEY_ENABLE_SCISSOR_TEST,
			KEY_ENABLE_STENCIL_TEST,
			KEY_ENABLE_TEXTURE_CUBE_MAP_SEAMLESS,
			KEY_FRONT_FACE,
			KEY_HINT_LINE_SMOOTH,
			KEY_HINT_POLYGON_SMOOTH,
			KEY_HINT_TEXTURE_COMPRESSION_HINT,
			KEY_HINT_FRAGMENT_SHADER_DERIVATIVE_HINT,
			KEY_LINE_WIDTH,
			KEY_PIXEL_STORE_PACK_SWAP_BYTES,
			KEY_PIXEL_STORE_PACK_LSB_FIRST,
			KEY_PIXEL_STORE_PACK_ROW_LENGTH,
			KEY_PIXEL_STORE_PACK_SKIP_ROWS,
			KEY_PIXEL_STORE_PACK_SKIP_PIXELS,
			KEY_PIXEL_STORE_PACK_ALIGNMENT,
			KEY_PIXEL_STORE_PACK_IMAGE_HEIGHT,
			KEY_PIXEL_STORE_PACK_SKIP_IMAGES,
			KEY_PIXEL_STORE_UNPACK_SWAP_BYTES,
			KEY_PIXEL_STORE_UNPACK_LSB_FIRST,
			KEY_PIXEL_STORE_UNPACK_ROW_LENGTH,
			KEY_PIXEL_STORE_UNPACK_SKIP_ROWS,
			KEY_PIXEL_STORE_UNPACK_SKIP_PIXELS,
			KEY_PIXEL_STORE_UNPACK_ALIGNMENT,
			KEY_PIXEL_STORE_UNPACK_IMAGE_HEIGHT,
			KEY_PIXEL_STORE_UNPACK_SKIP_IMAGES,
			KEY_POINT_SIZE,
			// OpenGL 3.3 core requires 'face' (parameter of glPolygonMode) to be 'GL_FRONT_AND_BACK'...
			KEY_POLYGON_MODE_FRONT_AND_BACK,
			KEY_POLYGON_OFFSET,
			KEY_PRIMITIVE_RESTART_INDEX,
			KEY_READ_BUFFER,
			KEY_SAMPLE_COVERAGE,
			KEY_SAMPLE_MASK,
			KEY_SCISSOR,
			KEY_STENCIL_FUNC,
			KEY_STENCIL_MASK,
			KEY_STENCIL_OP,
			KEY_USE_PROGRAM,
			KEY_VIEWPORT,

			NUM_ENUM_KEYS // Must be last.
		};


		//! For binding buffer objects.
		key_type
		get_bind_buffer_key(
				GLenum target) const;

		//! For binding renderbuffer objects (note: @a target must be GL_RENDERBUFFER).
		key_type
		get_bind_renderbuffer_key(
				GLenum target) const;

		key_type
		get_bind_sampler_key(
				GLuint unit) const;

		key_type
		get_bind_texture_key(
				GLenum texture_target,
				GLenum texture_unit) const;

		key_type
		get_clamp_color_key(
				GLenum target) const;

		/**
		 * For glEnable/glDisable.
		 */
		key_type
		get_capability_key(
				GLenum cap) const;

		/**
		 * Returns true if a capability (associated with @a get_capability_key) is indexed (glEnablei/glDisablei).
		 */
		bool
		is_capability_indexed(
				GLenum cap) const;

		/**
		 * Returns the number of capability indices supported by glEnablei/glDisablei for the specified @a cap.
		 */
		GLuint
		get_num_capability_indices(
				GLenum cap) const;

		//! For glHint.
		key_type
		get_hint_key(
				GLenum target) const;

		//! For glPixelStoref/glPixelStorei.
		key_type
		get_pixel_store_key(
				GLenum pname) const;

	private:

		//! Key offsets within a particular texture *image* unit - offsets repeat for each subsequent texture unit.
		enum TextureImageUnitKeyOffsetType
		{
			TEXTURE_IMAGE_UNIT_KEY_BIND_SAMPLER,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_1D,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_1D_ARRAY,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_ARRAY,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_MULTISAMPLE,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_MULTISAMPLE_ARRAY,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_3D,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_BUFFER,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_CUBE_MAP,
			TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_RECTANGLE,

			NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS // Must be last.
		};

		const GLCapabilities &d_capabilities;

		key_type d_enable_clip_distance_zero_base_key;
		key_type d_texture_image_unit_zero_base_key;

		unsigned int d_num_state_set_keys;


		//! Default constructor can only be called by @a create.
		GLStateSetKeys(
				const GLCapabilities &capabilities);

		//! Calculate a key for a texture parameter in the specified texture *image* unit.
		key_type
		get_texture_image_unit_key_from_key_offset(
				GLuint unit,
				TextureImageUnitKeyOffsetType key_offset) const;
	};
}

#endif // GPLATES_OPENGL_GLSTATESETKEYS_H
