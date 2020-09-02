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
	// All keys that don't depend on capabilities (such as supported number of textures) get hardwired (enum) keys.
	//
	// All the remaining keys must be allocated at runtime (here).
	key_type current_key = NUM_ENUM_KEYS;

	// Enable clip distances.
	d_enable_clip_distance_zero_base_key = current_key;
	current_key += d_capabilities.gl_max_clip_distances;

	// Texture units.
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
		qWarning() << "binding of specified buffer object target not currently supported.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_bind_renderbuffer_key(
		GLenum target) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			target == GL_RENDERBUFFER,
			GPLATES_ASSERTION_SOURCE);

	return KEY_BIND_RENDERBUFFER;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_bind_sampler_key(
		GLuint unit) const
{
	return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_SAMPLER);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_bind_texture_key(
		GLenum texture_target,
		GLenum texture_unit) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + d_capabilities.gl_max_combined_texture_image_units,
			GPLATES_ASSERTION_SOURCE);

	// Convert from enum to integer.
	const GLuint unit = texture_unit - GL_TEXTURE0;

	switch (texture_target)
	{
	case GL_TEXTURE_1D:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_1D);
	case GL_TEXTURE_2D:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D);
	case GL_TEXTURE_3D:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_3D);
	case GL_TEXTURE_CUBE_MAP:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_CUBE_MAP);
	case GL_TEXTURE_RECTANGLE:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_RECTANGLE);
	case GL_TEXTURE_1D_ARRAY:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_1D_ARRAY);
	case GL_TEXTURE_2D_ARRAY:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_ARRAY);
	case GL_TEXTURE_2D_MULTISAMPLE:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_MULTISAMPLE);
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_2D_MULTISAMPLE_ARRAY);
	case GL_TEXTURE_BUFFER:
		return get_texture_image_unit_key_from_key_offset(unit, TEXTURE_IMAGE_UNIT_KEY_BIND_TEXTURE_BUFFER);

	default:
		// Unsupported texture target type.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	// Shouldn't get there - keep the compiler happy.
	throw GPlatesGlobal::AbortException(GPLATES_EXCEPTION_SOURCE);
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_clamp_color_key(
		GLenum target) const
{
	// OpenGL 3.3 core supports only the GL_CLAMP_READ_COLOR target.
	// GL_CLAMP_VERTEX_COLOR and GL_CLAMP_FRAGMENT_COLOR have been deprecated (removed in core).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			target == GL_CLAMP_READ_COLOR,
			GPLATES_ASSERTION_SOURCE);

	return KEY_CLAMP_READ_COLOR;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_capability_key(
		GLenum cap) const
{
	// We're currently only accepting a subset of all capabilities.
	// Add more as needed.
	switch (cap)
	{
	case GL_BLEND:
		return KEY_ENABLE_BLEND;
	case GL_CULL_FACE:
		return KEY_ENABLE_CULL_FACE;
	case GL_DEPTH_CLAMP:
		return KEY_ENABLE_DEPTH_CLAMP;
	case GL_DEPTH_TEST:
		return KEY_ENABLE_DEPTH_TEST;
	case GL_FRAMEBUFFER_SRGB:
		return KEY_ENABLE_FRAMEBUFFER_SRGB;
	case GL_LINE_SMOOTH:
		return KEY_ENABLE_LINE_SMOOTH;
	case GL_MULTISAMPLE:
		return KEY_ENABLE_MULTISAMPLE;
	case GL_POLYGON_OFFSET_FILL:
		return KEY_ENABLE_POLYGON_OFFSET_FILL;
	case GL_POLYGON_OFFSET_LINE:
		return KEY_ENABLE_POLYGON_OFFSET_LINE;
	case GL_POLYGON_OFFSET_POINT:
		return KEY_ENABLE_POLYGON_OFFSET_POINT;
	case GL_POLYGON_SMOOTH:
		return KEY_ENABLE_POLYGON_SMOOTH;
	case GL_PRIMITIVE_RESTART:
		return KEY_ENABLE_PRIMITIVE_RESTART;
	case GL_PROGRAM_POINT_SIZE:
		return KEY_ENABLE_PROGRAM_POINT_SIZE;
	case GL_RASTERIZER_DISCARD:
		return KEY_ENABLE_RASTERIZER_DISCARD;
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
		return KEY_ENABLE_SAMPLE_ALPHA_TO_COVERAGE;
	case GL_SAMPLE_ALPHA_TO_ONE:
		return KEY_ENABLE_SAMPLE_ALPHA_TO_ONE;
	case GL_SAMPLE_COVERAGE:
		return KEY_ENABLE_SAMPLE_COVERAGE;
	case GL_SAMPLE_MASK:
		return KEY_ENABLE_SAMPLE_MASK;
	case GL_SCISSOR_TEST:
		return KEY_ENABLE_SCISSOR_TEST;
	case GL_STENCIL_TEST:
		return KEY_ENABLE_STENCIL_TEST;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS:
		return KEY_ENABLE_TEXTURE_CUBE_MAP_SEAMLESS;
	default:
		break;
	}

	// GL_CLIP_DISTANCEi.
	if (cap >= GL_CLIP_DISTANCE0 &&
		cap < GL_CLIP_DISTANCE0 + d_capabilities.gl_max_clip_distances)
	{
		return d_enable_clip_distance_zero_base_key + (cap - GL_CLIP_DISTANCE0);
	}

	// Unsupported capability.
	qWarning() << "glEnable/glDisable capability not currently supported.";
	GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);

	return 0;  // Avoid compiler error - shouldn't get here though (due to abort).
}


bool
GPlatesOpenGL::GLStateSetKeys::is_capability_indexed(
		GLenum cap) const
{
	// In OpenGL 3.3 core the only indexed capability (glEnablei/glDisablei) is blending.
	return cap == GL_BLEND;
}


GLuint
GPlatesOpenGL::GLStateSetKeys::get_num_capability_indices(
		GLenum cap) const
{
	// In OpenGL 3.3 core the only indexed capability (glEnablei/glDisablei) is blending.
	if (cap == GL_BLEND)
	{
		return d_capabilities.gl_max_draw_buffers;
	}

	// Indexing not supported for specified capability.
	qWarning() << "Indexing (glEnablei/glDisablei) not supported for capability.";
	GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);

	return 0;  // Avoid compiler error - shouldn't get here though (due to abort).
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
		qWarning() << "glHint capability not currently supported.";
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	return key;
}


GPlatesOpenGL::GLStateSetKeys::key_type
GPlatesOpenGL::GLStateSetKeys::get_texture_image_unit_key_from_key_offset(
		GLuint unit,
		TextureImageUnitKeyOffsetType key_offset) const
{
	return d_texture_image_unit_zero_base_key + unit * NUM_TEXTURE_IMAGE_UNIT_KEY_OFFSETS + key_offset;
}
