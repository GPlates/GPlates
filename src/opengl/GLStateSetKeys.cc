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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLStateSetKeys.h"

#include "GLContext.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLStateSetKeys::GLStateSetKeys() :
	d_num_state_set_keys(NUM_ENUM_KEYS) // ...it'll get higher as we assign more keys though.
{
	// Start assigning keys after all the hardwired (enum) keys.
	//
	// All keys that don't depend on availability of extensions or that don't depend on capabilities
	// (such as supported number of textures) get hardwired (enum) keys.
	//
	// All the remaining keys must be allocated at runtime (here).
	key_type current_key = NUM_ENUM_KEYS;

	d_generic_vertex_attribute_index_zero_base_key = current_key;
	current_key +=
			NUM_GENERIC_VERTEX_ATTRIBUTE_KEY_OFFSETS *
			GLContext::get_parameters().shader.gl_max_vertex_attribs;

	d_texture_unit_zero_base_key = current_key;
	current_key +=
			NUM_TEXTURE_UNIT_KEY_OFFSETS *
			GLContext::get_parameters().texture.gl_max_texture_units;

	d_num_state_set_keys = current_key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_bind_buffer_object_key(
		GLenum target) const
{
	key_type key = 0;

	switch (target)
	{
	case GL_ARRAY_BUFFER_ARB:
		key = KEY_BIND_ARRAY_BUFFER_OBJECT;
		break;
	case GL_ELEMENT_ARRAY_BUFFER_ARB:
		key = KEY_BIND_ELEMENT_ARRAY_BUFFER_OBJECT;
		break;
	case GL_PIXEL_PACK_BUFFER_ARB:
		key = KEY_BIND_PIXEL_PACK_BUFFER_OBJECT;
		break;
	case GL_PIXEL_UNPACK_BUFFER_ARB:
		key = KEY_BIND_PIXEL_UNPACK_BUFFER_OBJECT;
		break;
	case GL_TEXTURE_BUFFER_ARB:
		key = KEY_BIND_TEXTURE_BUFFER_OBJECT;
		break;

	default:
		// Unsupported capability.
		qWarning() << "binding of specified GL_ARB_vertex_buffer_object target not currently supported - should be easy to add though.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_enable_key(
		GLenum cap) const
{
	// Must be a non-texture capability - texture capabilities provided by 'get_texture_enable_key'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			cap != GL_TEXTURE_1D && cap != GL_TEXTURE_2D && cap != GL_TEXTURE_3D && cap != GL_TEXTURE_CUBE_MAP
#ifdef GL_ARB_texture_rectangle
				&& cap != GL_TEXTURE_RECTANGLE_ARB
#endif
			, GPLATES_ASSERTION_SOURCE);

	key_type key = 0;

	// We're currently only accepting a subset of all capabilities.
	// Add more as needed.
	switch (cap)
	{
	case GL_ALPHA_TEST:
		key = KEY_ENABLE_ALPHA_TEST;
		break;
	case GL_BLEND:
		key = KEY_ENABLE_BLEND;
		break;
	case GL_CULL_FACE:
		key = KEY_ENABLE_CULL_FACE;
		break;
	case GL_DEPTH_TEST:
		key = KEY_ENABLE_DEPTH_TEST;
		break;
	case GL_LINE_SMOOTH:
		key = KEY_ENABLE_LINE_SMOOTH;
		break;
	case GL_POINT_SMOOTH:
		key = KEY_ENABLE_POINT_SMOOTH;
		break;
	case GL_POLYGON_OFFSET_FILL:
		key = KEY_ENABLE_POLYGON_OFFSET_FILL;
		break;
	case GL_POLYGON_OFFSET_LINE:
		key = KEY_ENABLE_POLYGON_OFFSET_LINE;
		break;
	case GL_POLYGON_OFFSET_POINT:
		key = KEY_ENABLE_POLYGON_OFFSET_POINT;
		break;
	case GL_POLYGON_SMOOTH:
		key = KEY_ENABLE_POLYGON_SMOOTH;
		break;
	case GL_SCISSOR_TEST:
		key = KEY_ENABLE_SCISSOR_TEST;
		break;
	case GL_STENCIL_TEST:
		key = KEY_ENABLE_STENCIL_TEST;
		break;
	default:
		// Unsupported capability.
		qWarning() << "glEnable capability not currently supported - should be easy to add though.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_polygon_mode_key(
		GLenum face) const
{
	key_type key = 0;

	switch (face)
	{
	case GL_FRONT:
		key = KEY_POLYGON_MODE_FRONT;
		break;
	case GL_BACK:
		key = KEY_POLYGON_MODE_BACK;
		break;
	case GL_FRONT_AND_BACK:
		// We shouldn't get here since higher level code should split into separate front and back keys.
		qWarning() << "Interal Error: glPolygonMode 'GL_FRONT_AND_BACK' was not handled at a higher level.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	default:
		// Unsupported capability.
		qWarning() << "glPolygonMode: unrecognised parameter 'face'.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_hint_key(
		GLenum target) const
{
	key_type key = 0;

	// We're currently only accepting a subset of all hints.
	// Add more as needed.
	switch (target)
	{
	case GL_LINE_SMOOTH_HINT:
		key = KEY_HINT_LINE_SMOOTH;
		break;
	case GL_POINT_SMOOTH_HINT:
		key = KEY_HINT_POINT_SMOOTH;
		break;
	case GL_POLYGON_SMOOTH_HINT:
		key = KEY_HINT_POLYGON_SMOOTH;
		break;
	default:
		// Unsupported capability.
		qWarning() << "glHint capability not currently supported - should be easy to add though.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_texture_enable_key(
		GLenum texture_unit,
		GLenum texture_target) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + GLContext::get_parameters().texture.gl_max_texture_units,
			GPLATES_ASSERTION_SOURCE);

	TextureKeyOffsetType key_offset = static_cast<TextureKeyOffsetType>(0);

	// Note that other texture targets (like GL_TEXTURE_2D_ARRAY) are not supported by 'glEnable' and
	// 'glDisable' since they are used by shaders which don't require 'glEnable' and 'glDisable'.
	switch (texture_target)
	{
	case GL_TEXTURE_1D:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_1D;
		break;
	case GL_TEXTURE_2D:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_2D;
		break;
	case GL_TEXTURE_3D:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_3D;
		break;
	case GL_TEXTURE_CUBE_MAP:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_CUBE_MAP;
		break;
#ifdef GL_ARB_texture_rectangle
	case GL_TEXTURE_RECTANGLE_ARB:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_RECTANGLE;
		break;
#endif
	case GL_TEXTURE_GEN_S:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_GEN_S;
		break;
	case GL_TEXTURE_GEN_T:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_GEN_T;
		break;
	case GL_TEXTURE_GEN_R:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_GEN_R;
		break;
	case GL_TEXTURE_GEN_Q:
		key_offset = TEXTURE_KEY_ENABLE_TEXTURE_GEN_Q;
		break;
	default:
		// Unsupported texture target type.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return get_texture_key_from_key_offset(texture_unit, key_offset);
}

GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_bind_texture_key(
		GLenum texture_unit,
		GLenum texture_target) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + GLContext::get_parameters().texture.gl_max_texture_units,
			GPLATES_ASSERTION_SOURCE);

	TextureKeyOffsetType key_offset = static_cast<TextureKeyOffsetType>(0);

	// Note that other texture targets (like GL_TEXTURE_2D_ARRAY) are not supported by 'glEnable' and
	// 'glDisable' since they are used by shaders which don't require 'glEnable' and 'glDisable'.
	switch (texture_target)
	{
	case GL_TEXTURE_1D:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_1D;
		break;
	case GL_TEXTURE_2D:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_2D;
		break;
	case GL_TEXTURE_3D:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_3D;
		break;
	case GL_TEXTURE_CUBE_MAP:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_CUBE_MAP;
		break;
#ifdef GL_ARB_texture_rectangle // In case old 'glew.h' header
	case GL_TEXTURE_RECTANGLE_ARB:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_RECTANGLE;
		break;
#endif
#ifdef GL_EXT_texture_array // In case old 'glew.h' header
	case GL_TEXTURE_1D_ARRAY_EXT:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_1D_ARRAY;
		break;
	case GL_TEXTURE_2D_ARRAY_EXT:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_2D_ARRAY;
		break;
#endif
#ifdef GL_ARB_texture_multisample // In case old 'glew.h' header
	case GL_TEXTURE_2D_MULTISAMPLE:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_2D_MULTISAMPLE;
		break;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_2D_MULTISAMPLE_ARRAY;
		break;
#endif
#ifdef GL_EXT_texture_buffer_object // In case old 'glew.h' header
	case GL_TEXTURE_BUFFER_EXT:
		key_offset = TEXTURE_KEY_BIND_TEXTURE_BUFFER;
		break;
#endif

	default:
		// Unsupported texture target type.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return get_texture_key_from_key_offset(texture_unit, key_offset);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_tex_env_key(
		GLenum texture_unit,
		GLenum target,
		GLenum pname) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + GLContext::get_parameters().texture.gl_max_texture_units,
			GPLATES_ASSERTION_SOURCE);

	TextureKeyOffsetType key_offset = static_cast<TextureKeyOffsetType>(0);

	// We're only supporting what we currently use - more can be added though.
	// Also add to GLTexEnvStateSet::apply_default_state.
	switch (target)
	{
	case GL_TEXTURE_ENV:
		switch (pname)
		{
		case GL_TEXTURE_ENV_MODE:
			key_offset = TEXTURE_KEY_TEX_ENV_MODE;
			break;
		default:
			// Unsupported pname.
			GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
			break;
		}
		break;

	default:
		// Unsupported target.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return get_texture_key_from_key_offset(texture_unit, key_offset);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_tex_gen_key(
		GLenum texture_unit,
		GLenum coord,
		GLenum pname) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + GLContext::get_parameters().texture.gl_max_texture_units,
			GPLATES_ASSERTION_SOURCE);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			coord >= GL_S && coord <= GL_Q,
			GPLATES_ASSERTION_SOURCE);

	TextureKeyOffsetType key_offset = static_cast<TextureKeyOffsetType>(0);

	// We're only supporting what we currently use - more can be added though.
	// Also add to GLTexGenStateSet::apply_default_state.
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		key_offset = TextureKeyOffsetType(TEXTURE_KEY_TEXTURE_GEN_MODE_S + (coord - GL_S));
		break;
	case GL_OBJECT_PLANE:
		key_offset = TextureKeyOffsetType(TEXTURE_KEY_TEXTURE_OBJECT_PLANE_S + (coord - GL_S));
		break;
	case GL_EYE_PLANE:
		key_offset = TextureKeyOffsetType(TEXTURE_KEY_TEXTURE_EYE_PLANE_S + (coord - GL_S));
		break;
	default:
		// Unsupported pname.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return get_texture_key_from_key_offset(texture_unit, key_offset);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_enable_client_texture_state_key(
		GLenum texture_unit) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + GLContext::get_parameters().texture.gl_max_texture_units,
			GPLATES_ASSERTION_SOURCE);

	return get_texture_key_from_key_offset(texture_unit, TEXTURE_KEY_ENABLE_CLIENT_STATE_TEXTURE_COORD_ARRAY);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_tex_coord_pointer_state_key(
		GLenum texture_unit) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + GLContext::get_parameters().texture.gl_max_texture_units,
			GPLATES_ASSERTION_SOURCE);

	return get_texture_key_from_key_offset(texture_unit, TEXTURE_KEY_VERTEX_ARRAY_TEX_COORD_POINTER);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_enable_client_state_key(
		GLenum array) const
{
	key_type key = 0;

	// We're not including GL_INDEX_ARRAY since using RGBA mode (and not colour index mode)
	// and not including GL_EDGE_FLAG_ARRAY for now.
	switch (array)
	{
	case GL_VERTEX_ARRAY:
		key = KEY_ENABLE_CLIENT_STATE_VERTEX_ARRAY;
		break;
	case GL_COLOR_ARRAY:
		key = KEY_ENABLE_CLIENT_STATE_COLOR_ARRAY;
		break;
	case GL_NORMAL_ARRAY:
		key = KEY_ENABLE_CLIENT_STATE_NORMAL_ARRAY;
		break;

	default:
		// Unsupported capability.
		qWarning() << "glEnableClientState capability not currently supported - should be easy to add though.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_enable_vertex_attrib_array_key(
		GLuint attribute_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			attribute_index < GLContext::get_parameters().shader.gl_max_vertex_attribs,
			GPLATES_ASSERTION_SOURCE);

	return d_generic_vertex_attribute_index_zero_base_key +
		attribute_index * NUM_GENERIC_VERTEX_ATTRIBUTE_KEY_OFFSETS +
		GENERIC_VERTEX_ATTRIBUTE_KEY_ENABLE_VERTEX_ATTRIB_POINTER;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_vertex_attrib_array_key(
		GLuint attribute_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			attribute_index < GLContext::get_parameters().shader.gl_max_vertex_attribs,
			GPLATES_ASSERTION_SOURCE);

	return d_generic_vertex_attribute_index_zero_base_key +
		attribute_index * NUM_GENERIC_VERTEX_ATTRIBUTE_KEY_OFFSETS +
		GENERIC_VERTEX_ATTRIBUTE_KEY_VERTEX_ATTRIB_POINTER;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_load_matrix_key(
		GLenum mode) const
{
	key_type key = 0;

	switch (mode)
	{
	case GL_MODELVIEW:
		key = KEY_LOAD_MATRIX_MODELVIEW;
		break;
	case GL_PROJECTION:
		key = KEY_LOAD_MATRIX_PROJECTION;
		break;

	default:
		// Unsupported capability.
		qWarning() << "glLoadMatrix capability not currently supported - should be easy to add though.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_texture_key_from_key_offset(
		GLenum texture_unit,
		TextureKeyOffsetType key_offset) const
{
	return d_texture_unit_zero_base_key +
		(texture_unit - GLContext::Parameters::Texture::gl_texture0) * NUM_TEXTURE_UNIT_KEY_OFFSETS +
		key_offset;
}
