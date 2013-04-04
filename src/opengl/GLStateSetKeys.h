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

#include "utils/non_null_intrusive_ptr.h"
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
		 * These keys can be used directly.
		 *
		 * NOTE: There is no 'get' method for these keys - use the enum value directly.
		 */
		enum
		{
			KEY_ACTIVE_TEXTURE,
			KEY_ALPHA_FUNC,
			KEY_BIND_ARRAY_BUFFER_OBJECT,
			KEY_BIND_ELEMENT_ARRAY_BUFFER_OBJECT,
			KEY_BIND_FRAME_BUFFER,
			KEY_BIND_PIXEL_PACK_BUFFER_OBJECT,
			KEY_BIND_PIXEL_UNPACK_BUFFER_OBJECT,
			KEY_BIND_PROGRAM_OBJECT,
			KEY_BIND_TEXTURE_BUFFER_OBJECT,
			KEY_BIND_VERTEX_ARRAY_OBJECT,
			KEY_BLEND_EQUATION,
			KEY_BLEND_FUNC,
			KEY_CLEAR_COLOR,
			KEY_CLEAR_DEPTH,
			KEY_CLEAR_STENCIL,
			KEY_CLIENT_ACTIVE_TEXTURE,
			KEY_COLOR_MASK,
			KEY_CULL_FACE,
			KEY_DEPTH_FUNC,
			KEY_DEPTH_MASK,
			KEY_DEPTH_RANGE,
			KEY_ENABLE_ALPHA_TEST,
			KEY_ENABLE_BLEND,
			KEY_ENABLE_DEPTH_TEST,
			KEY_ENABLE_CLIENT_STATE_COLOR_ARRAY,
			KEY_ENABLE_CLIENT_STATE_NORMAL_ARRAY,
			KEY_ENABLE_CLIENT_STATE_VERTEX_ARRAY,
			KEY_ENABLE_CULL_FACE,
			KEY_ENABLE_LINE_SMOOTH,
			KEY_ENABLE_POINT_SMOOTH,
			KEY_ENABLE_POLYGON_OFFSET_FILL,
			KEY_ENABLE_POLYGON_OFFSET_LINE,
			KEY_ENABLE_POLYGON_OFFSET_POINT,
			KEY_ENABLE_POLYGON_SMOOTH,
			KEY_ENABLE_SCISSOR_TEST,
			KEY_ENABLE_STENCIL_TEST,
			KEY_FRONT_FACE,
			KEY_HINT_LINE_SMOOTH,
			KEY_HINT_POINT_SMOOTH,
			KEY_HINT_POLYGON_SMOOTH,
			KEY_LINE_WIDTH,
			KEY_LOAD_MATRIX_MODELVIEW,
			KEY_LOAD_MATRIX_PROJECTION,
			KEY_MATRIX_MODE,
			KEY_POINT_SIZE,
			KEY_POLYGON_MODE_BACK,
			KEY_POLYGON_MODE_FRONT,
			KEY_POLYGON_OFFSET,
			KEY_SCISSOR,
			KEY_STENCIL_FUNC,
			KEY_STENCIL_MASK,
			KEY_STENCIL_OP,
			KEY_VERTEX_ARRAY_COLOR_POINTER,
			KEY_VERTEX_ARRAY_NORMAL_POINTER,
			KEY_VERTEX_ARRAY_VERTEX_POINTER,
			KEY_VIEWPORT,

			NUM_ENUM_KEYS // Must be last.
		};


		//! For binding buffer objects (objects that target GL_ARB_vertex_buffer_object extension).
		key_type
		get_bind_buffer_object_key(
				GLenum target) const;

		//! For glEnable with non-texture targets.
		key_type
		get_enable_key(
				GLenum cap) const;

		/**
		 * For glPolygonMode.
		 *
		 * NOTE: Caller must detect 'GL_FRONT_AND_BACK' and request separate keys
		 * for 'GL_FRONT' and 'GL_BACK'.
		 */
		key_type
		get_polygon_mode_key(
				GLenum face) const;

		//! For glHint.
		key_type
		get_hint_key(
				GLenum target) const;

		/**
		 * For glEnable with texture targets.
		 *
		 * Note that this also includes texgen state (eg, GL_TEXTURE_GEN_S, etc).
		 */
		key_type
		get_texture_enable_key(
				GLenum texture_unit,
				GLenum texture_target) const;

		key_type
		get_bind_texture_key(
				GLenum texture_unit,
				GLenum texture_target) const;

		key_type
		get_tex_env_key(
				GLenum texture_unit,
				GLenum target,
				GLenum pname) const;

		key_type
		get_tex_gen_key(
				GLenum texture_unit,
				GLenum coord,
				GLenum pname) const;

		key_type
		get_enable_client_texture_state_key(
				GLenum texture_unit) const;

		key_type
		get_tex_coord_pointer_state_key(
				GLenum texture_unit) const;

		key_type
		get_enable_client_state_key(
				GLenum array) const;

		key_type
		get_enable_vertex_attrib_array_key(
				GLuint attribute_index) const;

		key_type
		get_vertex_attrib_array_key(
				GLuint attribute_index) const;

		key_type
		get_load_matrix_key(
				GLenum mode) const;

		key_type
		get_load_texture_matrix_key(
				GLenum texture_unit) const
		{
			return get_texture_coord_key_from_key_offset(texture_unit, TEXTURE_COORD_KEY_LOAD_MATRIX_TEXTURE);
		}

	private:
		//! Key offsets within a particular generic vertex attribute index - offsets repeat for each subsequent index.
		enum GenericVertexAttributeKeyOffsetType
		{
			GENERIC_VERTEX_ATTRIBUTE_KEY_ENABLE_VERTEX_ATTRIB_POINTER,
			GENERIC_VERTEX_ATTRIBUTE_KEY_VERTEX_ATTRIB_POINTER,

			NUM_GENERIC_VERTEX_ATTRIBUTE_KEY_OFFSETS // Must be last.
		};

		//! Key offsets within a particular texture *image* unit - offsets repeat for each subsequent texture unit.
		enum TextureImageUnitKeyOffsetType
		{
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
			TEXTURE_IMAGE_UNIT_KEY_ENABLE_TEXTURE_1D,
			TEXTURE_IMAGE_UNIT_KEY_ENABLE_TEXTURE_2D,
			TEXTURE_IMAGE_UNIT_KEY_ENABLE_TEXTURE_3D,
			TEXTURE_IMAGE_UNIT_KEY_ENABLE_TEXTURE_CUBE_MAP,
			TEXTURE_IMAGE_UNIT_KEY_ENABLE_TEXTURE_RECTANGLE,
			TEXTURE_IMAGE_UNIT_KEY_TEX_ENV_MODE,
			TEXTURE_IMAGE_UNIT_KEY_TEX_ENV_COLOR,
			TEXTURE_IMAGE_UNIT_KEY_COMBINE_RGB,
			TEXTURE_IMAGE_UNIT_KEY_COMBINE_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_SOURCE0_RGB,
			TEXTURE_IMAGE_UNIT_KEY_SOURCE1_RGB,
			TEXTURE_IMAGE_UNIT_KEY_SOURCE2_RGB,
			TEXTURE_IMAGE_UNIT_KEY_SOURCE0_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_SOURCE1_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_SOURCE2_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_OPERAND0_RGB,
			TEXTURE_IMAGE_UNIT_KEY_OPERAND1_RGB,
			TEXTURE_IMAGE_UNIT_KEY_OPERAND2_RGB,
			TEXTURE_IMAGE_UNIT_KEY_OPERAND0_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_OPERAND1_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_OPERAND2_ALPHA,
			TEXTURE_IMAGE_UNIT_KEY_RGB_SCALE,
			TEXTURE_IMAGE_UNIT_KEY_ALPHA_SCALE,

			NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS // Must be last.
		};

		//! Key offsets within a particular texture *coordinate* set - offsets repeat for each subsequent texture unit.
		enum TextureCoordKeyOffsetType
		{
			TEXTURE_COORD_KEY_ENABLE_CLIENT_STATE_TEXTURE_COORD_ARRAY,
			TEXTURE_COORD_KEY_ENABLE_TEXTURE_GEN_S,
			TEXTURE_COORD_KEY_ENABLE_TEXTURE_GEN_T,
			TEXTURE_COORD_KEY_ENABLE_TEXTURE_GEN_R,
			TEXTURE_COORD_KEY_ENABLE_TEXTURE_GEN_Q,
			TEXTURE_COORD_KEY_LOAD_MATRIX_TEXTURE,
			TEXTURE_COORD_KEY_TEXTURE_EYE_PLANE_S,
			TEXTURE_COORD_KEY_TEXTURE_EYE_PLANE_T,
			TEXTURE_COORD_KEY_TEXTURE_EYE_PLANE_R,
			TEXTURE_COORD_KEY_TEXTURE_EYE_PLANE_Q,
			TEXTURE_COORD_KEY_TEXTURE_GEN_MODE_S,
			TEXTURE_COORD_KEY_TEXTURE_GEN_MODE_T,
			TEXTURE_COORD_KEY_TEXTURE_GEN_MODE_R,
			TEXTURE_COORD_KEY_TEXTURE_GEN_MODE_Q,
			TEXTURE_COORD_KEY_TEXTURE_OBJECT_PLANE_S,
			TEXTURE_COORD_KEY_TEXTURE_OBJECT_PLANE_T,
			TEXTURE_COORD_KEY_TEXTURE_OBJECT_PLANE_R,
			TEXTURE_COORD_KEY_TEXTURE_OBJECT_PLANE_Q,
			TEXTURE_COORD_KEY_VERTEX_ARRAY_TEX_COORD_POINTER,

			NUM_TEXTURE_COORD_KEY_OFFSETS // Must be last.
		};

		const GLCapabilities &d_capabilities;

		key_type d_generic_vertex_attribute_index_zero_base_key;

		key_type d_texture_image_unit_zero_base_key;
		key_type d_texture_coord_zero_base_key;

		unsigned int d_num_state_set_keys;

		//! Default constructor can only be called by @a create.
		GLStateSetKeys(
				const GLCapabilities &capabilities);

		//! Calculate a key for a texture parameter in the specified texture *image* unit.
		key_type
		get_texture_image_unit_key_from_key_offset(
				GLenum texture_unit,
				TextureImageUnitKeyOffsetType key_offset) const;

		//! Calculate a key for a texture parameter for the specified texture *coordinate* set.
		key_type
		get_texture_coord_key_from_key_offset(
				GLenum texture_unit,
				TextureCoordKeyOffsetType key_offset) const;
	};
}

#endif // GPLATES_OPENGL_GLSTATESETKEYS_H
