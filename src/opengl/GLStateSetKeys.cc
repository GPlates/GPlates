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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <QtGlobal>

#include "GLStateSetKeys.h"

#include "GLCapabilities.h"

#include "global/AbortException.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLStateSetKeys::GLStateSetKeys(
		const GLCapabilities &capabilities) :
	d_capabilities(capabilities),
	d_num_state_set_keys(NUM_ENUM_KEYS) // ...it'll get higher as we assign more keys though.
{
	// Start assigning keys after all the hardwired (enum) keys.
	//
	// All keys that don't depend on availability of extensions or that don't depend on capabilities
	// (such as supported number of textures) get hardwired (enum) keys.
	//
	// All the remaining keys must be allocated at runtime (here).
	key_type current_key = NUM_ENUM_KEYS;

	d_texture_image_unit_zero_base_key = current_key;
	current_key +=
			NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS *
			d_capabilities.gl_max_combined_texture_image_units;

	d_num_state_set_keys = current_key;

	//qDebug() << "GLStateSetKeys: Number keys: " << d_num_state_set_keys;
	//qDebug() << "GLStateSetKeys: gl_max_combined_texture_image_units: " << d_capabilities.gl_max_combined_texture_image_units;
	//qDebug() << "GLStateSetKeys: NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS: " << NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_bind_buffer_key(
		GLenum target) const
{
	key_type key = 0;

	switch (target)
	{
	case GL_ARRAY_BUFFER:
		key = KEY_BIND_ARRAY_BUFFER;
		break;
	case GL_COPY_READ_BUFFER:
		key = KEY_BIND_COPY_READ_BUFFER;
		break;
	case GL_COPY_WRITE_BUFFER:
		key = KEY_BIND_COPY_WRITE_BUFFER;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		key = KEY_BIND_ELEMENT_ARRAY_BUFFER;
		break;
	case GL_PIXEL_PACK_BUFFER:
		key = KEY_BIND_PIXEL_PACK_BUFFER;
		break;
	case GL_PIXEL_UNPACK_BUFFER:
		key = KEY_BIND_PIXEL_UNPACK_BUFFER;
		break;
	case GL_TEXTURE_BUFFER:
		key = KEY_BIND_TEXTURE_BUFFER;
		break;
	case GL_TRANSFORM_FEEDBACK_BUFFER:
		key = KEY_BIND_TRANSFORM_FEEDBACK_BUFFER;
		break;
	case GL_UNIFORM_BUFFER:
		key = KEY_BIND_UNIFORM_BUFFER;
		break;

	default:
		// Unsupported capability.
		qWarning() << "binding of specified buffer object target not currently supported - should be easy to add though.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_enable_key(
		GLenum cap) const
{
	key_type key = 0;

	// We're currently only accepting a subset of all capabilities.
	// Add more as needed.
	switch (cap)
	{
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
	case GL_PRIMITIVE_RESTART:
		key = KEY_ENABLE_PRIMITIVE_RESTART;
		break;
	case GL_RASTERIZER_DISCARD:
		key = KEY_ENABLE_RASTERIZER_DISCARD;
		break;
	case GL_SCISSOR_TEST:
		key = KEY_ENABLE_SCISSOR_TEST;
		break;
	case GL_STENCIL_TEST:
		key = KEY_ENABLE_STENCIL_TEST;
		break;
	default:
		// Unsupported capability.
		qWarning() << "glEnable/glDisable capability not currently supported - should be easy to add though.";
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
	case GL_POLYGON_SMOOTH_HINT:
		key = KEY_HINT_POLYGON_SMOOTH;
		break;
	case GL_TEXTURE_COMPRESSION_HINT:
		key = KEY_HINT_TEXTURE_COMPRESSION_HINT;
		break;
	case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
		key = KEY_HINT_FRAGMENT_SHADER_DERIVATIVE_HINT;
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
GPlatesOpenGL::GLStateSetKeys::get_bind_texture_key(
		GLenum texture_target,
		GLenum texture_unit) const
{
	// Note that other texture targets (like GL_TEXTURE_2D_ARRAY) are not supported by 'glEnable' and
	// 'glDisable' since they are used by shaders which don't require 'glEnable' and 'glDisable'.
	switch (texture_target)
	{
	case GL_TEXTURE_1D:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_1D);
	case GL_TEXTURE_2D:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D);
	case GL_TEXTURE_3D:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_3D);
	case GL_TEXTURE_CUBE_MAP:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_CUBE_MAP);
	case GL_TEXTURE_RECTANGLE:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_RECTANGLE);
	case GL_TEXTURE_1D_ARRAY:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_1D_ARRAY);
	case GL_TEXTURE_2D_ARRAY:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_ARRAY);
	case GL_TEXTURE_2D_MULTISAMPLE:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_MULTISAMPLE);
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_MULTISAMPLE_ARRAY);
	case GL_TEXTURE_BUFFER:
		return get_texture_image_unit_key_from_key_offset(texture_unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_BUFFER);

	default:
		// Unsupported texture target type.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	// Shouldn't get there - keep the compiler happy.
	throw GPlatesGlobal::AbortException(GPLATES_EXCEPTION_SOURCE);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_texture_image_unit_key_from_key_offset(
		GLenum texture_unit,
		TextureImageUnitKeyOffsetType key_offset) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + d_capabilities.gl_max_combined_texture_image_units,
			GPLATES_ASSERTION_SOURCE);

	return d_texture_image_unit_zero_base_key +
		(texture_unit - GLCapabilities::gl_TEXTURE0) * NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS +
		key_offset;
}
