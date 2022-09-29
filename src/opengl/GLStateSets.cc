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

#include <cmath>  // std::round

#include "GLStateSets.h"

#include "GLState.h"
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/Real.h"


GPlatesOpenGL::GLActiveTextureStateSet::GLActiveTextureStateSet(
		const GLCapabilities &capabilities,
		GLenum active_texture) :
	d_active_texture(active_texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			active_texture >= GL_TEXTURE0 &&
				active_texture < GL_TEXTURE0 + capabilities.gl_max_combined_texture_image_units,
			GPLATES_ASSERTION_SOURCE);
}


bool
GPlatesOpenGL::GLActiveTextureStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_active_texture ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLActiveTextureStateSet &>(current_state_set).d_active_texture)
	{
		return false;
	}

	opengl_functions.glActiveTexture(d_active_texture);

	return true;
}

bool
GPlatesOpenGL::GLActiveTextureStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_active_texture == GL_TEXTURE0)
	{
		return false;
	}

	opengl_functions.glActiveTexture(d_active_texture);

	return true;
}

bool
GPlatesOpenGL::GLActiveTextureStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (GL_TEXTURE0 == d_active_texture)
	{
		return false;
	}

	opengl_functions.glActiveTexture(GL_TEXTURE0);

	return true;
}


bool
GPlatesOpenGL::GLBindBufferStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_buffer_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindBufferStateSet &>(current_state_set).d_buffer_resource)
	{
		return false;
	}

	// Bind the buffer object (can be zero).
	opengl_functions.glBindBuffer(d_target, d_buffer_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindBufferStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_buffer_resource == 0)
	{
		return false;
	}

	// Bind the buffer object.
	opengl_functions.glBindBuffer(d_target, d_buffer_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindBufferStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_buffer_resource == 0)
	{
		return false;
	}

	// The default is zero (no buffer object).
	opengl_functions.glBindBuffer(d_target, 0);

	return true;
}


bool
GPlatesOpenGL::GLBindBufferIndexedStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBindBufferIndexedStateSet &current = dynamic_cast<const GLBindBufferIndexedStateSet &>(current_state_set);

	bool applied_state = false;

	// The current buffer object bound to the *general* binding point can change below when
	// calling glBindBufferBase or glBindBufferRange.
	GLuint current_general_buffer_resource = current.d_general_buffer_resource;

	// Iterate over the *new* non-default state (any indices not present represent the default unbound state)
	// and bind to the indexed binding point (if a state change detected).
	for (const auto &indexed_slot : d_indexed_buffers)
	{
		const GLuint index = indexed_slot.first;
		const IndexedBuffer &indexed_buffer = indexed_slot.second;

		// See if there was a state change (between current and new states).
		auto search_current_index_slot = current.d_indexed_buffers.find(index);
		if (search_current_index_slot != current.d_indexed_buffers.end())
		{
			const IndexedBuffer &current_indexed_buffer = search_current_index_slot->second;
			if (indexed_buffer == current_indexed_buffer)
			{
				// No state change detected, so skip to the next index (if any).
				continue;
			}
		}

		// Bind either the entire buffer object or a sub-range to the current indexed binding point.
		//
		// Note that, unlike the *general* buffer resource, the *indexed* buffer resource is non-zero.
		if (indexed_buffer.range)
		{
			const Range &range = indexed_buffer.range.get();
			opengl_functions.glBindBufferRange(d_target, index, indexed_buffer.buffer_resource, range.offset, range.size);
		}
		else
		{
			opengl_functions.glBindBufferBase(d_target, index, indexed_buffer.buffer_resource);
		}

		// Calling glBindBufferBase or glBindBufferRange also changes the *general* binding point.
		current_general_buffer_resource = indexed_buffer.buffer_resource;

		applied_state = true;
	}

	// Next iterate over the *current* state (but ignore indices already handled in the loop above).
	// These are indices bound in the current state but unbound in the new state.
	for (const auto &current_indexed_slot : current.d_indexed_buffers)
	{
		const GLuint current_index = current_indexed_slot.first;

		// Look at the new state.
		auto search_index_slot = d_indexed_buffers.find(current_index);
		if (search_index_slot != d_indexed_buffers.end())
		{
			// This has already been handled in the loop above, so skip to the next index (if any).
			continue;
		}

		// Unbind the current indexed binding point.
		opengl_functions.glBindBufferBase(d_target, current_index, 0);

		// Calling glBindBufferBase also changes the *general* binding point.
		current_general_buffer_resource = 0;

		applied_state = true;
	}

	// Bind the buffer object at the *general* binding point (if a state change detected).
	if (d_general_buffer_resource != current_general_buffer_resource)
	{
		// Note: 'd_general_buffer_resource' can be zero.
		opengl_functions.glBindBuffer(d_target, d_general_buffer_resource);

		applied_state = true;
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBindBufferIndexedStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	// The current buffer object bound to the *general* binding point can change below when
	// calling glBindBufferBase or glBindBufferRange.
	GLuint current_general_buffer_resource = 0;

	// Iterate over the *new* non-default state (any indices not present represent the default unbound state)
	// and bind to the indexed binding point.
	for (const auto &indexed_slot : d_indexed_buffers)
	{
		const GLuint index = indexed_slot.first;
		const IndexedBuffer &indexed_buffer = indexed_slot.second;

		// Bind either the entire buffer object or a sub-range to the current indexed binding point.
		//
		// Note that, unlike the *general* buffer resource, the *indexed* buffer resource is non-zero.
		if (indexed_buffer.range)
		{
			const Range &range = indexed_buffer.range.get();
			opengl_functions.glBindBufferRange(d_target, index, indexed_buffer.buffer_resource, range.offset, range.size);
		}
		else
		{
			opengl_functions.glBindBufferBase(d_target, index, indexed_buffer.buffer_resource);
		}

		// Calling glBindBufferBase or glBindBufferRange also changes the *general* binding point.
		current_general_buffer_resource = indexed_buffer.buffer_resource;

		applied_state = true;
	}

	// Bind the buffer object at the *general* binding point (if a state change detected).
	if (d_general_buffer_resource != current_general_buffer_resource)
	{
		// Note: 'd_general_buffer_resource' can be zero.
		opengl_functions.glBindBuffer(d_target, d_general_buffer_resource);

		applied_state = true;
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBindBufferIndexedStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	// The current buffer object bound to the *general* binding point can change below when
	// calling glBindBufferBase or glBindBufferRange.
	GLuint current_general_buffer_resource = d_general_buffer_resource;

	// Iterate over the *current* non-default state (any indices not present represent the default unbound state)
	// and unbind from the indexed binding point.
	for (const auto &indexed_slot : d_indexed_buffers)
	{
		const GLuint index = indexed_slot.first;

		// Unbind the current indexed binding point.
		opengl_functions.glBindBufferBase(d_target, index, 0);

		// Calling glBindBufferBase also changes the *general* binding point.
		current_general_buffer_resource = 0;

		applied_state = true;
	}

	// Unbind the buffer object at the *general* binding point (if a state change detected).
	if (0 != current_general_buffer_resource)
	{
		opengl_functions.glBindBuffer(d_target, 0);

		applied_state = true;
	}

	return applied_state;
}


bool
GPlatesOpenGL::GLBindFramebufferStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBindFramebufferStateSet &current = dynamic_cast<const GLBindFramebufferStateSet &>(current_state_set);

	bool applied_state = false;

	// Bind the framebuffer object (can be the default resource) to the draw or read target (or both).
	if (d_draw_framebuffer_resource == d_read_framebuffer_resource)
	{
		if (d_draw_framebuffer_resource != current.d_draw_framebuffer_resource ||
			d_read_framebuffer_resource != current.d_read_framebuffer_resource)
		{
			// Both draw/read targets are the same so bind them in one call
			// (even though it's possible only one of the targets has changed).
			opengl_functions.glBindFramebuffer(GL_FRAMEBUFFER, d_draw_framebuffer_resource);
			applied_state = true;
		}
	}
	else // Draw and read targets are bound to different framebuffers...
	{
		if (d_draw_framebuffer_resource != current.d_draw_framebuffer_resource)
		{
			opengl_functions.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_draw_framebuffer_resource);
			applied_state = true;
		}
		if (d_read_framebuffer_resource != current.d_read_framebuffer_resource)
		{
			opengl_functions.glBindFramebuffer(GL_READ_FRAMEBUFFER, d_read_framebuffer_resource);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBindFramebufferStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	// Bind the framebuffer object to the draw or read target (or both).
	if (d_draw_framebuffer_resource == d_read_framebuffer_resource)
	{
		if (d_draw_framebuffer_resource != d_default_framebuffer_resource)
		{
			// Both draw/read targets are the same (and not the default) so bind them in one call.
			opengl_functions.glBindFramebuffer(GL_FRAMEBUFFER, d_draw_framebuffer_resource);
			applied_state = true;
		}
	}
	else // Draw and read targets are bound to different framebuffers...
	{
		if (d_draw_framebuffer_resource != d_default_framebuffer_resource)
		{
			opengl_functions.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_draw_framebuffer_resource);
			applied_state = true;
		}
		if (d_read_framebuffer_resource != d_default_framebuffer_resource)
		{
			opengl_functions.glBindFramebuffer(GL_READ_FRAMEBUFFER, d_read_framebuffer_resource);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBindFramebufferStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	// Bind the framebuffer object to the draw or read target (or both).
	if (d_draw_framebuffer_resource == d_read_framebuffer_resource)
	{
		if (d_draw_framebuffer_resource != d_default_framebuffer_resource)
		{
			// Both draw/read targets are the same (and not the default) so bind them in one call.
			opengl_functions.glBindFramebuffer(GL_FRAMEBUFFER, d_default_framebuffer_resource);
			applied_state = true;
		}
	}
	else // Draw and read targets are bound to different framebuffers...
	{
		if (d_draw_framebuffer_resource != d_default_framebuffer_resource)
		{
			opengl_functions.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_default_framebuffer_resource);
			applied_state = true;
		}
		if (d_read_framebuffer_resource != d_default_framebuffer_resource)
		{
			opengl_functions.glBindFramebuffer(GL_READ_FRAMEBUFFER, d_default_framebuffer_resource);
			applied_state = true;
		}
	}

	return applied_state;
}


GPlatesOpenGL::GLBindImageTextureStateSet::GLBindImageTextureStateSet(
		const GLCapabilities &capabilities,
		GLuint image_unit,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLboolean layered,
		GLint layer,
		GLenum access,
		GLenum format) :
	d_image_unit(image_unit),
	d_texture(texture),
	d_texture_resource(0),
	d_level(level),
	d_layered(layered),
	d_layer(layer),
	d_access(access),
	d_format(format)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			image_unit < capabilities.gl_max_image_units,
			GPLATES_ASSERTION_SOURCE);

	if (texture)
	{
		d_texture_resource = texture.get()->get_resource_handle();
	}
}

bool
GPlatesOpenGL::GLBindImageTextureStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBindImageTextureStateSet &current = dynamic_cast<const GLBindImageTextureStateSet &>(current_state_set);

	// Note we're NOT comparing the image unit (it should be the same for 'this' and 'current_state_set').
	//
	// Return early if no state change...
	if (d_texture_resource == current.d_texture_resource &&
		d_level == current.d_level &&
		d_layered == current.d_layered &&
		d_layer == current.d_layer &&
		d_access == current.d_access &&
		d_format == current.d_format)
	{
		return false;
	}

	// Bind the texture object (can be zero).
	opengl_functions.glBindImageTexture(d_image_unit, d_texture_resource, d_level, d_layered, d_layer, d_access, d_format);

	return true;
}

bool
GPlatesOpenGL::GLBindImageTextureStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_texture_resource == 0 &&
		d_level == 0 &&
		d_layered == GL_FALSE &&
		d_layer == 0 &&
		d_access == GL_READ_ONLY &&
		d_format == GL_R8)
	{
		return false;
	}

	// Bind the texture object.
	opengl_functions.glBindImageTexture(d_image_unit, d_texture_resource, d_level, d_layered, d_layer, d_access, d_format);

	return true;
}

bool
GPlatesOpenGL::GLBindImageTextureStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_texture_resource == 0 &&
		d_level == 0 &&
		d_layered == GL_FALSE &&
		d_layer == 0 &&
		d_access == GL_READ_ONLY &&
		d_format == GL_R8)
	{
		return false;
	}

	// The default is zero (no texture object).
	opengl_functions.glBindImageTexture(d_image_unit, 0, 0/*level*/, GL_FALSE/*layered*/, 0/*layer*/, GL_READ_ONLY/*access*/, GL_R8/*format*/);

	return true;
}


bool
GPlatesOpenGL::GLBindRenderbufferStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_renderbuffer_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindRenderbufferStateSet &>(current_state_set).d_renderbuffer_resource)
	{
		return false;
	}

	// Bind the renderbuffer object (can be zero).
	opengl_functions.glBindRenderbuffer(d_target, d_renderbuffer_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindRenderbufferStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_renderbuffer_resource == 0)
	{
		return false;
	}

	// Bind the renderbuffer object.
	opengl_functions.glBindRenderbuffer(d_target, d_renderbuffer_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindRenderbufferStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_renderbuffer_resource == 0)
	{
		return false;
	}

	// The default is zero (no renderbuffer object).
	opengl_functions.glBindRenderbuffer(d_target, 0);

	return true;
}


GPlatesOpenGL::GLBindSamplerStateSet::GLBindSamplerStateSet(
		const GLCapabilities &capabilities,
		GLuint unit,
		boost::optional<GLSampler::shared_ptr_type> sampler) :
	d_unit(unit),
	d_sampler(sampler),
	d_sampler_resource(0)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			unit < capabilities.gl_max_combined_texture_image_units,
			GPLATES_ASSERTION_SOURCE);

	if (sampler)
	{
		d_sampler_resource = sampler.get()->get_resource_handle();
	}
}

bool
GPlatesOpenGL::GLBindSamplerStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Note the only state we're comparing is the sampler object (resource handle).
	// The texture unit should be the same for 'this' and 'current_state_set'.
	//
	// Return early if no state change...
	if (d_sampler_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindSamplerStateSet &>(current_state_set).d_sampler_resource)
	{
		return false;
	}

	// Bind the sampler object (can be zero).
	opengl_functions.glBindSampler(d_unit, d_sampler_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindSamplerStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_sampler_resource == 0)
	{
		return false;
	}

	// Bind the sampler object.
	opengl_functions.glBindSampler(d_unit, d_sampler_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindSamplerStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_sampler_resource == 0)
	{
		return false;
	}

	// The default is zero (no sampler object).
	opengl_functions.glBindSampler(d_unit, 0);

	return true;
}


GPlatesOpenGL::GLBindTextureStateSet::GLBindTextureStateSet(
		const GLCapabilities &capabilities,
		GLenum texture_target,
		GLenum texture_unit,
		boost::optional<GLTexture::shared_ptr_type> texture) :
	d_texture_target(texture_target),
	d_texture_unit(texture_unit),
	d_texture(texture),
	d_texture_resource(0)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + capabilities.gl_max_combined_texture_image_units,
			GPLATES_ASSERTION_SOURCE);

	if (texture)
	{
		d_texture_resource = texture.get()->get_resource_handle();
	}
}

bool
GPlatesOpenGL::GLBindTextureStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Note the only state we're comparing is the texture object (resource handle).
	// The texture target or texture unit should be the same for 'this' and 'current_state_set'.
	//
	// Return early if no state change...
	if (d_texture_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindTextureStateSet &>(current_state_set).d_texture_resource)
	{
		return false;
	}

	// Make sure the correct texture unit is currently active when binding texture.
	const GLenum current_active_texture = current_state.get_active_texture();
	if (d_texture_unit != current_active_texture)
	{
		opengl_functions.glActiveTexture(d_texture_unit);
	}

	// Bind the texture object (can be zero).
	opengl_functions.glBindTexture(d_texture_target, d_texture_resource);

	// Restore active texture.
	if (current_active_texture != d_texture_unit)
	{
		opengl_functions.glActiveTexture(current_active_texture);
	}

	return true;
}

bool
GPlatesOpenGL::GLBindTextureStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_texture_resource == 0)
	{
		return false;
	}

	// Make sure the correct texture unit is currently active when binding texture.
	const GLenum current_active_texture = current_state.get_active_texture();
	if (d_texture_unit != current_active_texture)
	{
		opengl_functions.glActiveTexture(d_texture_unit);
	}

	// Bind the texture.
	opengl_functions.glBindTexture(d_texture_target, d_texture_resource);

	// Restore active texture.
	if (current_active_texture != d_texture_unit)
	{
		opengl_functions.glActiveTexture(current_active_texture);
	}

	return true;
}

bool
GPlatesOpenGL::GLBindTextureStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_texture_resource == 0)
	{
		return false;
	}

	// Make sure the correct texture unit is currently active when unbinding texture.
	const GLenum current_active_texture = current_state.get_active_texture();
	if (d_texture_unit != current_active_texture)
	{
		opengl_functions.glActiveTexture(d_texture_unit);
	}

	// Bind the default unnamed texture 0.
	opengl_functions.glBindTexture(d_texture_target, 0);

	// Restore active texture.
	if (current_active_texture != d_texture_unit)
	{
		opengl_functions.glActiveTexture(current_active_texture);
	}

	return true;
}


bool
GPlatesOpenGL::GLBindVertexArrayStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_array_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindVertexArrayStateSet &>(current_state_set).d_array_resource)
	{
		return false;
	}

	// Bind the vertex array object (can be zero).
	opengl_functions.glBindVertexArray(d_array_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindVertexArrayStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_array_resource == 0)
	{
		return false;
	}

	// Bind the vertex array object.
	opengl_functions.glBindVertexArray(d_array_resource);

	return true;
}

bool
GPlatesOpenGL::GLBindVertexArrayStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_array_resource == 0)
	{
		return false;
	}

	// The default is zero (no vertex array object).
	opengl_functions.glBindVertexArray(0);

	return true;
}


bool
GPlatesOpenGL::GLBlendColorStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBlendColorStateSet &current = dynamic_cast<const GLBlendColorStateSet &>(current_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == current.d_red &&
		d_green == current.d_green &&
		d_blue == current.d_blue &&
		d_alpha == current.d_alpha)
	{
		return false;
	}

	opengl_functions.glBlendColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));

	return true;
}

bool
GPlatesOpenGL::GLBlendColorStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return false;
	}

	opengl_functions.glBlendColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));

	return true;
}

bool
GPlatesOpenGL::GLBlendColorStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return false;
	}

	opengl_functions.glBlendColor(0, 0, 0, 0);

	return true;
}


const GLenum GPlatesOpenGL::GLBlendEquationStateSet::DEFAULT_MODE = GL_FUNC_ADD;

bool
GPlatesOpenGL::GLBlendEquationStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBlendEquationStateSet &current = dynamic_cast<const GLBlendEquationStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_mode_RGB == d_mode_alpha)
	{
		// If either RGB or alpha mode changed...
		if (d_mode_RGB != current.d_mode_RGB ||
			d_mode_alpha != current.d_mode_alpha)
		{
			// Both RGB/alpha modes are the same so set them in one call
			// (even though it's possible only one of the modes has changed).
			opengl_functions.glBlendEquation(d_mode_RGB);
			applied_state = true;
		}
	}
	else // RGB and alpha modes are different...
	{
		if (d_mode_RGB != current.d_mode_RGB ||
			d_mode_alpha != current.d_mode_alpha)
		{
			opengl_functions.glBlendEquationSeparate(d_mode_RGB, d_mode_alpha);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBlendEquationStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_mode_RGB == d_mode_alpha)
	{
		if (d_mode_RGB != DEFAULT_MODE)
		{
			// Both RGB/alpha modes are the same so set them in one call.
			opengl_functions.glBlendEquation(d_mode_RGB);
			applied_state = true;
		}
	}
	else // RGB and alpha modes are different...
	{
		// Both RGB and alpha modes are different, so they both cannot be the default state.
		opengl_functions.glBlendEquationSeparate(d_mode_RGB, d_mode_alpha);
		applied_state = true;
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBlendEquationStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_mode_RGB == d_mode_alpha)
	{
		if (d_mode_RGB != DEFAULT_MODE)
		{
			// Both RGB/alpha modes are the same so set them in one call.
			opengl_functions.glBlendEquation(DEFAULT_MODE);
			applied_state = true;
		}
	}
	else // RGB and alpha modes are different...
	{
		// Both RGB and alpha modes are different, so they both cannot be the default state.
		opengl_functions.glBlendEquationSeparate(DEFAULT_MODE, DEFAULT_MODE);
		applied_state = true;
	}

	return applied_state;
}


const GPlatesOpenGL::GLBlendFuncStateSet::Func
GPlatesOpenGL::GLBlendFuncStateSet::DEFAULT_FUNC{ GL_ONE, GL_ZERO };

bool
GPlatesOpenGL::GLBlendFuncStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBlendFuncStateSet &current = dynamic_cast<const GLBlendFuncStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_RGB_func == d_alpha_func)
	{
		// If either RGB or alpha func changed...
		if (d_RGB_func != current.d_RGB_func ||
			d_alpha_func != current.d_alpha_func)
		{
			// Both RGB/alpha funcs are the same so set them in one call
			// (even though it's possible only one of the funcs has changed).
			opengl_functions.glBlendFunc(d_RGB_func.src, d_RGB_func.dst);
			applied_state = true;
		}
	}
	else // RGB and alpha blend funcs are different...
	{
		if (d_RGB_func != current.d_RGB_func ||
			d_alpha_func != current.d_alpha_func)
		{
			opengl_functions.glBlendFuncSeparate(d_RGB_func.src, d_RGB_func.dst, d_alpha_func.src, d_alpha_func.dst);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBlendFuncStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_RGB_func == d_alpha_func)
	{
		if (d_RGB_func != DEFAULT_FUNC)
		{
			// Both RGB/alpha funcs are the same so set them in one call.
			opengl_functions.glBlendFunc(d_RGB_func.src, d_RGB_func.dst);
			applied_state = true;
		}
	}
	else // RGB and alpha blend funcs are different...
	{
		// Both RGB and alpha funcs are different, so they both cannot be the default state.
		opengl_functions.glBlendFuncSeparate(d_RGB_func.src, d_RGB_func.dst, d_alpha_func.src, d_alpha_func.dst);
		applied_state = true;
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLBlendFuncStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_RGB_func == d_alpha_func)
	{
		if (d_RGB_func != DEFAULT_FUNC)
		{
			// Both RGB/alpha funcs are the same so set them in one call.
			opengl_functions.glBlendFunc(DEFAULT_FUNC.src, DEFAULT_FUNC.dst);
			applied_state = true;
		}
	}
	else // RGB and alpha blend funcs are different...
	{
		// Both RGB and alpha funcs are different, so they both cannot be the default state.
		opengl_functions.glBlendFuncSeparate(DEFAULT_FUNC.src, DEFAULT_FUNC.dst, DEFAULT_FUNC.src, DEFAULT_FUNC.dst);
		applied_state = true;
	}

	return applied_state;
}


bool
GPlatesOpenGL::GLClampColorStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLClampColorStateSet &current = dynamic_cast<const GLClampColorStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_clamp == current.d_clamp)
	{
		return false;
	}

	opengl_functions.glClampColor(d_target, d_clamp);

	return true;
}

bool
GPlatesOpenGL::GLClampColorStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_clamp == GL_FIXED_ONLY)
	{
		return false;
	}

	opengl_functions.glClampColor(d_target, d_clamp);

	return true;
}

bool
GPlatesOpenGL::GLClampColorStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_clamp == GL_FIXED_ONLY)
	{
		return false;
	}

	opengl_functions.glClampColor(d_target, GL_FIXED_ONLY);

	return true;
}


bool
GPlatesOpenGL::GLClearColorStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLClearColorStateSet &current = dynamic_cast<const GLClearColorStateSet &>(current_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == current.d_red &&
		d_green == current.d_green &&
		d_blue == current.d_blue &&
		d_alpha == current.d_alpha)
	{
		return false;
	}

	opengl_functions.glClearColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));

	return true;
}

bool
GPlatesOpenGL::GLClearColorStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return false;
	}

	opengl_functions.glClearColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));

	return true;
}

bool
GPlatesOpenGL::GLClearColorStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return false;
	}

	opengl_functions.glClearColor(0, 0, 0, 0);

	return true;
}


bool
GPlatesOpenGL::GLClearDepthStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLClearDepthStateSet &current = dynamic_cast<const GLClearDepthStateSet &>(current_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == current.d_depth)
	{
		return false;
	}

	opengl_functions.glClearDepth(static_cast<GLclampd>(d_depth.dval()));

	return true;
}

bool
GPlatesOpenGL::GLClearDepthStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == 1.0)
	{
		return false;
	}

	opengl_functions.glClearDepth(static_cast<GLclampd>(d_depth.dval()));

	return true;
}

bool
GPlatesOpenGL::GLClearDepthStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == 1.0)
	{
		return false;
	}

	opengl_functions.glClearDepth(1.0);

	return true;
}


bool
GPlatesOpenGL::GLClearStencilStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLClearStencilStateSet &current = dynamic_cast<const GLClearStencilStateSet &>(current_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == current.d_stencil)
	{
		return false;
	}

	opengl_functions.glClearStencil(d_stencil);

	return true;
}

bool
GPlatesOpenGL::GLClearStencilStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == 0)
	{
		return false;
	}

	opengl_functions.glClearStencil(d_stencil);

	return true;
}

bool
GPlatesOpenGL::GLClearStencilStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == 0)
	{
		return false;
	}

	opengl_functions.glClearStencil(0);

	return true;
}


const GPlatesOpenGL::GLColorMaskStateSet::Mask
GPlatesOpenGL::GLColorMaskStateSet::DEFAULT_MASK{ GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };

bool
GPlatesOpenGL::GLColorMaskStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLColorMaskStateSet &current = dynamic_cast<const GLColorMaskStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_all_masks_equal && current.d_all_masks_equal)
	{
		// If state changed...
		if (d_masks[0] != current.d_masks[0])
		{
			opengl_functions.glColorMask(d_masks[0].red, d_masks[0].green, d_masks[0].blue, d_masks[0].alpha);
			applied_state = true;
		}
	}
	else
	{
		const GLuint num_masks = capabilities.gl_max_draw_buffers;
		for (GLuint i = 0; i < num_masks; ++i)
		{
			// If state changed...
			if (d_masks[i] != current.d_masks[i])
			{
				opengl_functions.glColorMaski(i, d_masks[i].red, d_masks[i].green, d_masks[i].blue, d_masks[i].alpha);
				applied_state = true;
			}
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLColorMaskStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_all_masks_equal)
	{
		// If state changed...
		if (d_masks[0] != DEFAULT_MASK)
		{
			opengl_functions.glColorMask(d_masks[0].red, d_masks[0].green, d_masks[0].blue, d_masks[0].alpha);
			applied_state = true;
		}
	}
	else
	{
		const GLuint num_masks = capabilities.gl_max_draw_buffers;
		for (GLuint i = 0; i < num_masks; ++i)
		{
			// If state changed...
			if (d_masks[i] != DEFAULT_MASK)
			{
				opengl_functions.glColorMaski(i, d_masks[i].red, d_masks[i].green, d_masks[i].blue, d_masks[i].alpha);
				applied_state = true;
			}
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLColorMaskStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_all_masks_equal)
	{
		// If state changed...
		if (d_masks[0] != DEFAULT_MASK)
		{
			opengl_functions.glColorMask(DEFAULT_MASK.red, DEFAULT_MASK.green, DEFAULT_MASK.blue, DEFAULT_MASK.alpha);
			applied_state = true;
		}
	}
	else
	{
		const GLuint num_masks = capabilities.gl_max_draw_buffers;
		for (GLuint i = 0; i < num_masks; ++i)
		{
			// If state changed...
			if (d_masks[i] != DEFAULT_MASK)
			{
				opengl_functions.glColorMaski(i, DEFAULT_MASK.red, DEFAULT_MASK.green, DEFAULT_MASK.blue, DEFAULT_MASK.alpha);
				applied_state = true;
			}
		}
	}

	return applied_state;
}


bool
GPlatesOpenGL::GLCullFaceStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLCullFaceStateSet &current = dynamic_cast<const GLCullFaceStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_mode == current.d_mode)
	{
		return false;
	}

	opengl_functions.glCullFace(d_mode);

	return true;
}

bool
GPlatesOpenGL::GLCullFaceStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_BACK)
	{
		return false;
	}

	opengl_functions.glCullFace(d_mode);

	return true;
}

bool
GPlatesOpenGL::GLCullFaceStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_BACK)
	{
		return false;
	}

	opengl_functions.glCullFace(GL_BACK);

	return true;
}


bool
GPlatesOpenGL::GLDepthFuncStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDepthFuncStateSet &current = dynamic_cast<const GLDepthFuncStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_depth_func == current.d_depth_func)
	{
		return false;
	}

	opengl_functions.glDepthFunc(d_depth_func);

	return true;
}

bool
GPlatesOpenGL::GLDepthFuncStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_depth_func == GL_LESS)
	{
		return false;
	}

	opengl_functions.glDepthFunc(d_depth_func);

	return true;
}

bool
GPlatesOpenGL::GLDepthFuncStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_depth_func == GL_LESS)
	{
		return false;
	}

	opengl_functions.glDepthFunc(GL_LESS);

	return true;
}


bool
GPlatesOpenGL::GLDepthMaskStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDepthMaskStateSet &current = dynamic_cast<const GLDepthMaskStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_flag == current.d_flag)
	{
		return false;
	}

	opengl_functions.glDepthMask(d_flag);

	return true;
}

bool
GPlatesOpenGL::GLDepthMaskStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_flag == GL_TRUE)
	{
		return false;
	}

	opengl_functions.glDepthMask(d_flag);

	return true;
}

bool
GPlatesOpenGL::GLDepthMaskStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_flag == GL_TRUE)
	{
		return false;
	}

	opengl_functions.glDepthMask(GL_TRUE);

	return true;
}


bool
GPlatesOpenGL::GLDepthRangeStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDepthRangeStateSet &current = dynamic_cast<const GLDepthRangeStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_n == current.d_n && d_f == current.d_f)  // epsilon tests
	{
		return false;
	}

	opengl_functions.glDepthRange(d_n.dval(), d_f.dval());

	return true;
}

bool
GPlatesOpenGL::GLDepthRangeStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_n == 0.0 && d_f == 1.0)  // epsilon tests
	{
		return false;
	}

	opengl_functions.glDepthRange(d_n.dval(), d_f.dval());

	return true;
}

bool
GPlatesOpenGL::GLDepthRangeStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_n == 0.0 && d_f == 1.0)  // epsilon tests
	{
		return false;
	}

	opengl_functions.glDepthRange(0.0, 1.0);

	return true;
}


bool
GPlatesOpenGL::GLDrawBuffersStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDrawBuffersStateSet &current = dynamic_cast<const GLDrawBuffersStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_draw_buffers == current.d_draw_buffers)
	{
		return false;
	}

	opengl_functions.glDrawBuffers(d_draw_buffers.size(), d_draw_buffers.data());

	return true;
}

bool
GPlatesOpenGL::GLDrawBuffersStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_draw_buffers.size() == 1 &&
		d_draw_buffers[0] == d_default_draw_buffer)
	{
		return false;
	}

	opengl_functions.glDrawBuffers(d_draw_buffers.size(), d_draw_buffers.data());

	return true;
}

bool
GPlatesOpenGL::GLDrawBuffersStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_draw_buffers.size() == 1 &&
		d_draw_buffers[0] == d_default_draw_buffer)
	{
		return false;
	}

	opengl_functions.glDrawBuffer(d_default_draw_buffer);

	return true;
}


bool
GPlatesOpenGL::GLEnableStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableStateSet &>(current_state_set).d_enable)
	{
		return false;
	}

	if (d_enable)
	{
		opengl_functions.glEnable(d_cap);
	}
	else
	{
		opengl_functions.glDisable(d_cap);
	}

	return true;
}

bool
GPlatesOpenGL::GLEnableStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool enable_default = get_default(d_cap);

	// Return early if no state change...
	if (d_enable == enable_default)
	{
		return false;
	}

	if (d_enable)
	{
		opengl_functions.glEnable(d_cap);
	}
	else
	{
		opengl_functions.glDisable(d_cap);
	}

	return true;
}

bool
GPlatesOpenGL::GLEnableStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool enable_default = get_default(d_cap);

	// Return early if no state change...
	if (d_enable == enable_default)
	{
		return false;
	}

	if (enable_default)
	{
		opengl_functions.glEnable(d_cap);
	}
	else
	{
		opengl_functions.glDisable(d_cap);
	}

	return true;
}

bool
GPlatesOpenGL::GLEnableStateSet::get_default(
		GLenum cap)
{
	// All capabilities default to GL_FALSE except two.
	return cap == GL_DITHER || cap == GL_MULTISAMPLE;
}


bool
GPlatesOpenGL::GLEnableIndexedStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLEnableIndexedStateSet &current = dynamic_cast<const GLEnableIndexedStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_all_indices_equal && current.d_all_indices_equal)
	{
		// If state changed...
		if (d_indices[0] != current.d_indices[0])
		{
			// Enable/disable all indices with a single call.
			if (d_indices[0])
			{
				opengl_functions.glEnable(d_cap);
			}
			else
			{
				opengl_functions.glDisable(d_cap);
			}
			applied_state = true;
		}
	}
	else
	{
		const GLuint num_indices = d_indices.size();
		for (GLuint i = 0; i < num_indices; ++i)
		{
			// If state changed...
			if (d_indices[i] != current.d_indices[i])
			{
				if (d_indices[i])
				{
					opengl_functions.glEnablei(d_cap, i);
				}
				else
				{
					opengl_functions.glDisablei(d_cap, i);
				}
				applied_state = true;
			}
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLEnableIndexedStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool default_ = GLEnableStateSet::get_default(d_cap);

	bool applied_state = false;

	if (d_all_indices_equal)
	{
		// If state changed...
		if (d_indices[0] != default_)
		{
			// Enable/disable all indices with a single call.
			if (d_indices[0])
			{
				opengl_functions.glEnable(d_cap);
			}
			else
			{
				opengl_functions.glDisable(d_cap);
			}
			applied_state = true;
		}
	}
	else
	{
		const GLuint num_indices = d_indices.size();
		for (GLuint i = 0; i < num_indices; ++i)
		{
			// If state changed...
			if (d_indices[i] != default_)
			{
				if (d_indices[i])
				{
					opengl_functions.glEnablei(d_cap, i);
				}
				else
				{
					opengl_functions.glDisablei(d_cap, i);
				}
				applied_state = true;
			}
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLEnableIndexedStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool default_ = GLEnableStateSet::get_default(d_cap);

	bool applied_state = false;

	if (d_all_indices_equal)
	{
		// If state changed...
		if (d_indices[0] != default_)
		{
			// Enable/disable all indices with a single call.
			if (default_)
			{
				opengl_functions.glEnable(d_cap);
			}
			else
			{
				opengl_functions.glDisable(d_cap);
			}
			applied_state = true;
		}
	}
	else
	{
		const GLuint num_indices = d_indices.size();
		for (GLuint i = 0; i < num_indices; ++i)
		{
			// If state changed...
			if (d_indices[i] != default_)
			{
				if (default_)
				{
					opengl_functions.glEnablei(d_cap, i);
				}
				else
				{
					opengl_functions.glDisablei(d_cap, i);
				}
				applied_state = true;
			}
		}
	}

	return applied_state;
}


bool
GPlatesOpenGL::GLFrontFaceStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_dir == dynamic_cast<const GLFrontFaceStateSet &>(current_state_set).d_dir)
	{
		return false;
	}

	opengl_functions.glFrontFace(d_dir);

	return true;
}

bool
GPlatesOpenGL::GLFrontFaceStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_dir == GL_CCW)
	{
		return false;
	}

	opengl_functions.glFrontFace(d_dir);

	return true;
}

bool
GPlatesOpenGL::GLFrontFaceStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_dir == GL_CCW)
	{
		return false;
	}

	opengl_functions.glFrontFace(GL_CCW);

	return true;
}


bool
GPlatesOpenGL::GLHintStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_hint == dynamic_cast<const GLHintStateSet &>(current_state_set).d_hint)
	{
		return false;
	}

	opengl_functions.glHint(d_target, d_hint);

	return true;
}

bool
GPlatesOpenGL::GLHintStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_hint == GL_DONT_CARE)
	{
		return false;
	}

	opengl_functions.glHint(d_target, d_hint);

	return true;
}

bool
GPlatesOpenGL::GLHintStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_hint == GL_DONT_CARE)
	{
		return false;
	}

	opengl_functions.glHint(d_target, GL_DONT_CARE);

	return true;
}


bool
GPlatesOpenGL::GLLineWidthStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	// NOTE: This is an epsilon test.
	if (d_width == dynamic_cast<const GLLineWidthStateSet &>(current_state_set).d_width)
	{
		return false;
	}

	opengl_functions.glLineWidth(d_width.dval());

	return true;
}

bool
GPlatesOpenGL::GLLineWidthStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_width == 1.0)
	{
		return false;
	}

	opengl_functions.glLineWidth(d_width.dval());

	return true;
}

bool
GPlatesOpenGL::GLLineWidthStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_width == 1.0)
	{
		return false;
	}

	opengl_functions.glLineWidth(GLfloat(1));

	return true;
}


GPlatesOpenGL::GLPixelStoreStateSet::GLPixelStoreStateSet(
		GLenum pname,
		GLfloat param) :
	d_pname(pname)
{
	// It's a GLfloat parameter but we'll map it to a GLint since there are actually no
	// parameters of type GLfloat.

	// If pname is a boolean type...
	if (pname == GL_PACK_SWAP_BYTES ||
		pname == GL_UNPACK_SWAP_BYTES ||
		pname == GL_PACK_LSB_FIRST ||
		pname == GL_UNPACK_LSB_FIRST)
	{
		// OpenGL 3.3 spec says zero maps to false and non-zero to true, which if specified
		// using an integer (ie, glPixelStorei instead of glPixelStoref) then false is 0 and true
		// can be any non-zero integer (we choose 1).
		d_param = (GPlatesMaths::Real(param) != 0) ? 1 : 0;
	}
	else // all remaining pnames have type integer (in OpenGL 3.3)...
	{
		// OpenGL 3.3 spec says param is rounded to the nearest integer.
		d_param = static_cast<GLint>(std::round(param));
	}
}


GPlatesOpenGL::GLPixelStoreStateSet::GLPixelStoreStateSet(
		GLenum pname,
		GLint param) :
	d_pname(pname)
{
	// If pname is a boolean type...
	if (pname == GL_PACK_SWAP_BYTES ||
		pname == GL_UNPACK_SWAP_BYTES ||
		pname == GL_PACK_LSB_FIRST ||
		pname == GL_UNPACK_LSB_FIRST)
	{
		// OpenGL 3.3 spec says zero maps to false and non-zero to true, which if specified
		// using an integer (ie, glPixelStorei instead of glPixelStoref) then false is 0 and true
		// can be any non-zero integer (we choose 1).
		d_param = (param != 0) ? 1 : 0;
	}
	else // all remaining pnames have type integer (in OpenGL 3.3)...
	{
		d_param = param;
	}
}


bool
GPlatesOpenGL::GLPixelStoreStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Note the only state we're comparing is the parameter value.
	// The parameter name should be the same for 'this' and 'current_state_set'.
	//
	// Return early if no state change...
	if (d_param ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLPixelStoreStateSet &>(current_state_set).d_param)
	{
		return false;
	}

	// We're not using glPixelStoref (since all parameter types are boolean or integer).
	opengl_functions.glPixelStorei(d_pname, d_param);

	return true;
}

bool
GPlatesOpenGL::GLPixelStoreStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const GLint param_default = get_default(d_pname);

	// Return early if no state change...
	if (d_param == param_default)
	{
		return false;
	}

	// We're not using glPixelStoref (since all parameter types are boolean or integer).
	opengl_functions.glPixelStorei(d_pname, d_param);

	return true;
}

bool
GPlatesOpenGL::GLPixelStoreStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const GLint param_default = get_default(d_pname);

	// Return early if no state change...
	if (d_param == param_default)
	{
		return false;
	}

	// We're not using glPixelStoref (since all parameter types are boolean or integer).
	opengl_functions.glPixelStorei(d_pname, param_default);

	return true;
}

GLint
GPlatesOpenGL::GLPixelStoreStateSet::get_default(
		GLenum pname)
{
	switch (pname)
	{
	case GL_PACK_SWAP_BYTES:
		return 0;  // GLint equivalent of false
	case GL_PACK_LSB_FIRST:
		return 0;  // GLint equivalent of false
	case GL_PACK_ROW_LENGTH:
		return 0;
	case GL_PACK_SKIP_ROWS:
		return 0;
	case GL_PACK_SKIP_PIXELS:
		return 0;
	case GL_PACK_ALIGNMENT:
		return 4;
	case GL_PACK_IMAGE_HEIGHT:
		return 0;
	case GL_PACK_SKIP_IMAGES:
		return 0;

	case GL_UNPACK_SWAP_BYTES:
		return 0;  // GLint equivalent of false
	case GL_UNPACK_LSB_FIRST:
		return 0;  // GLint equivalent of false
	case GL_UNPACK_ROW_LENGTH:
		return 0;
	case GL_UNPACK_SKIP_ROWS:
		return 0;
	case GL_UNPACK_SKIP_PIXELS:
		return 0;
	case GL_UNPACK_ALIGNMENT:
		return 4;
	case GL_UNPACK_IMAGE_HEIGHT:
		return 0;
	case GL_UNPACK_SKIP_IMAGES:
		return 0;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		return 0;  // Shouldn't get here, but keep compiler happy.
	}
}


bool
GPlatesOpenGL::GLPointSizeStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	// NOTE: This is an epsilon test.
	if (d_size == dynamic_cast<const GLPointSizeStateSet &>(current_state_set).d_size)
	{
		return false;
	}

	opengl_functions.glPointSize(d_size.dval());

	return true;
}

bool
GPlatesOpenGL::GLPointSizeStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_size == 1.0)
	{
		return false;
	}

	opengl_functions.glPointSize(d_size.dval());

	return true;
}

bool
GPlatesOpenGL::GLPointSizeStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_size == 1.0)
	{
		return false;
	}

	opengl_functions.glPointSize(GLfloat(1));

	return true;
}


bool
GPlatesOpenGL::GLPolygonModeStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_mode == dynamic_cast<const GLPolygonModeStateSet &>(current_state_set).d_mode)
	{
		return false;
	}

	opengl_functions.glPolygonMode(GL_FRONT_AND_BACK, d_mode);

	return true;
}

bool
GPlatesOpenGL::GLPolygonModeStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_FILL)
	{
		return false;
	}

	opengl_functions.glPolygonMode(GL_FRONT_AND_BACK, d_mode);

	return true;
}

bool
GPlatesOpenGL::GLPolygonModeStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_FILL)
	{
		return false;
	}

	opengl_functions.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return true;
}


bool
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLPolygonOffsetStateSet &current = dynamic_cast<const GLPolygonOffsetStateSet &>(current_state_set);

	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == current.d_factor && d_units == current.d_units)
	{
		return false;
	}

	opengl_functions.glPolygonOffset(d_factor.dval(), d_units.dval());

	return true;
}

bool
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == 0 && d_units == 0)
	{
		return false;
	}

	opengl_functions.glPolygonOffset(d_factor.dval(), d_units.dval());

	return true;
}

bool
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == 0 && d_units == 0)
	{
		return false;
	}

	opengl_functions.glPolygonOffset(GLfloat(0), GLfloat(0));

	return true;
}


bool
GPlatesOpenGL::GLPrimitiveRestartIndexStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLPrimitiveRestartIndexStateSet &current = dynamic_cast<const GLPrimitiveRestartIndexStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_index == current.d_index)
	{
		return false;
	}

	opengl_functions.glPrimitiveRestartIndex(d_index);

	return true;
}

bool
GPlatesOpenGL::GLPrimitiveRestartIndexStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_index == 0)
	{
		return false;
	}

	opengl_functions.glPrimitiveRestartIndex(d_index);

	return true;
}

bool
GPlatesOpenGL::GLPrimitiveRestartIndexStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no 0.d_index)
	{
		return false;
	}

	opengl_functions.glPrimitiveRestartIndex(0);

	return true;
}


bool
GPlatesOpenGL::GLReadBufferStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLReadBufferStateSet &current = dynamic_cast<const GLReadBufferStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_read_buffer == current.d_read_buffer)
	{
		return false;
	}

	opengl_functions.glReadBuffer(d_read_buffer);

	return true;
}

bool
GPlatesOpenGL::GLReadBufferStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_read_buffer == d_default_read_buffer)
	{
		return false;
	}

	opengl_functions.glReadBuffer(d_read_buffer);

	return true;
}

bool
GPlatesOpenGL::GLReadBufferStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_read_buffer == d_default_read_buffer)
	{
		return false;
	}

	opengl_functions.glReadBuffer(d_default_read_buffer);

	return true;
}


bool
GPlatesOpenGL::GLSampleCoverageStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLSampleCoverageStateSet &current = dynamic_cast<const GLSampleCoverageStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_value == current.d_value /*epsilon test*/ &&
		d_invert == current.d_invert)
	{
		return false;
	}

	opengl_functions.glSampleCoverage(
			static_cast<GLclampf>(d_value.dval()),
			d_invert);

	return true;
}

bool
GPlatesOpenGL::GLSampleCoverageStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_value == 1 /*epsilon test*/ &&
		d_invert == GL_FALSE)
	{
		return false;
	}

	opengl_functions.glSampleCoverage(
			static_cast<GLclampf>(d_value.dval()),
			d_invert);

	return true;
}

bool
GPlatesOpenGL::GLSampleCoverageStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_value == 1 /*epsilon test*/ &&
		d_invert == GL_FALSE)
	{
		return false;
	}

	opengl_functions.glSampleCoverage(1, GL_FALSE);

	return true;
}


const GLbitfield GPlatesOpenGL::GLSampleMaskStateSet::DEFAULT_MASK = ~GLbitfield(0)/*all ones*/;

bool
GPlatesOpenGL::GLSampleMaskStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLSampleMaskStateSet &current = dynamic_cast<const GLSampleMaskStateSet &>(current_state_set);

	bool applied_state = false;

	const GLuint num_masks = capabilities.gl_max_sample_mask_words;
	for (GLuint i = 0; i < num_masks; ++i)
	{
		// If state changed...
		if (d_masks[i] != current.d_masks[i])
		{
			opengl_functions.glSampleMaski(i, d_masks[i]);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLSampleMaskStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	const GLuint num_masks = capabilities.gl_max_sample_mask_words;
	for (GLuint i = 0; i < num_masks; ++i)
	{
		// If state changed...
		if (d_masks[i] != DEFAULT_MASK)
		{
			opengl_functions.glSampleMaski(i, d_masks[i]);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLSampleMaskStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	const GLuint num_masks = capabilities.gl_max_sample_mask_words;
	for (GLuint i = 0; i < num_masks; ++i)
	{
		// If state changed...
		if (d_masks[i] != DEFAULT_MASK)
		{
			opengl_functions.glSampleMaski(i, DEFAULT_MASK);
			applied_state = true;
		}
	}

	return applied_state;
}


bool
GPlatesOpenGL::GLScissorStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLScissorStateSet &current = dynamic_cast<const GLScissorStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_scissor_rectangle == current.d_scissor_rectangle)
	{
		return false;
	}

	opengl_functions.glScissor(
			d_scissor_rectangle.x(), d_scissor_rectangle.y(),
			d_scissor_rectangle.width(), d_scissor_rectangle.height());

	return true;
}

bool
GPlatesOpenGL::GLScissorStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_scissor_rectangle == d_default_scissor_rectangle)
	{
		return false;
	}

	opengl_functions.glScissor(
			d_scissor_rectangle.x(), d_scissor_rectangle.y(),
			d_scissor_rectangle.width(), d_scissor_rectangle.height());

	return true;
}

bool
GPlatesOpenGL::GLScissorStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_scissor_rectangle == d_default_scissor_rectangle)
	{
		return false;
	}

	opengl_functions.glScissor(
			d_default_scissor_rectangle.x(), d_default_scissor_rectangle.y(),
			d_default_scissor_rectangle.width(), d_default_scissor_rectangle.height());

	return true;
}


const GPlatesOpenGL::GLStencilFuncStateSet::Func
GPlatesOpenGL::GLStencilFuncStateSet::DEFAULT_FUNC{ GL_ALWAYS, 0, ~GLuint(0) };

bool
GPlatesOpenGL::GLStencilFuncStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLStencilFuncStateSet &current = dynamic_cast<const GLStencilFuncStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_front_func == d_back_func)
	{
		// If either front or back func changed...
		if (d_front_func != current.d_front_func ||
			d_back_func != current.d_back_func)
		{
			// Both front/back funcs are the same so set them in one call
			// (even though it's possible only one of the faces has changed).
			opengl_functions.glStencilFunc(d_front_func.func, d_front_func.ref, d_front_func.mask);
			applied_state = true;
		}
	}
	else // Front and back stencil funcs are different...
	{
		if (d_front_func != current.d_front_func)
		{
			opengl_functions.glStencilFuncSeparate(GL_FRONT, d_front_func.func, d_front_func.ref, d_front_func.mask);
			applied_state = true;
		}
		if (d_back_func != current.d_back_func)
		{
			opengl_functions.glStencilFuncSeparate(GL_BACK, d_back_func.func, d_back_func.ref, d_back_func.mask);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLStencilFuncStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_front_func == d_back_func)
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			// Both front/back funcs are the same so set them in one call.
			opengl_functions.glStencilFunc(d_front_func.func, d_front_func.ref, d_front_func.mask);
			applied_state = true;
		}
	}
	else // Front and back stencil funcs are different...
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			opengl_functions.glStencilFuncSeparate(GL_FRONT, d_front_func.func, d_front_func.ref, d_front_func.mask);
			applied_state = true;
		}
		if (d_back_func != DEFAULT_FUNC)
		{
			opengl_functions.glStencilFuncSeparate(GL_BACK, d_back_func.func, d_back_func.ref, d_back_func.mask);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLStencilFuncStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_front_func == d_back_func)
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			// Both front/back funcs are the same so set them in one call.
			opengl_functions.glStencilFunc(DEFAULT_FUNC.func, DEFAULT_FUNC.ref, DEFAULT_FUNC.mask);
			applied_state = true;
		}
	}
	else // Front and back stencil funcs are different...
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			opengl_functions.glStencilFuncSeparate(GL_FRONT, DEFAULT_FUNC.func, DEFAULT_FUNC.ref, DEFAULT_FUNC.mask);
			applied_state = true;
		}
		if (d_back_func != DEFAULT_FUNC)
		{
			opengl_functions.glStencilFuncSeparate(GL_BACK, DEFAULT_FUNC.func, DEFAULT_FUNC.ref, DEFAULT_FUNC.mask);
			applied_state = true;
		}
	}

	return applied_state;
}


const GLuint GPlatesOpenGL::GLStencilMaskStateSet::DEFAULT_MASK = ~GLuint(0)/*all ones*/;

bool
GPlatesOpenGL::GLStencilMaskStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLStencilMaskStateSet &current = dynamic_cast<const GLStencilMaskStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_front_mask == d_back_mask)
	{
		// If either front or back mask changed...
		if (d_front_mask != current.d_front_mask ||
			d_back_mask != current.d_back_mask)
		{
			// Both front/back masks are the same so set them in one call
			// (even though it's possible only one of the faces has changed).
			opengl_functions.glStencilMask(d_front_mask);
			applied_state = true;
		}
	}
	else // Front and back stencil masks are different...
	{
		if (d_front_mask != current.d_front_mask)
		{
			opengl_functions.glStencilMaskSeparate(GL_FRONT, d_front_mask);
			applied_state = true;
		}
		if (d_back_mask != current.d_back_mask)
		{
			opengl_functions.glStencilMaskSeparate(GL_BACK, d_back_mask);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLStencilMaskStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_front_mask == d_back_mask)
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			// Both front/back masks are the same so set them in one call.
			opengl_functions.glStencilMask(d_front_mask);
			applied_state = true;
		}
	}
	else // Front and back stencil masks are different...
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			opengl_functions.glStencilMaskSeparate(GL_FRONT, d_front_mask);
			applied_state = true;
		}
		if (d_back_mask != DEFAULT_MASK)
		{
			opengl_functions.glStencilMaskSeparate(GL_BACK, d_back_mask);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLStencilMaskStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_front_mask == d_back_mask)
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			// Both front/back masks are the same so set them in one call.
			opengl_functions.glStencilMask(DEFAULT_MASK);
			applied_state = true;
		}
	}
	else // Front and back stencil masks are different...
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			opengl_functions.glStencilMaskSeparate(GL_FRONT, DEFAULT_MASK);
			applied_state = true;
		}
		if (d_back_mask != DEFAULT_MASK)
		{
			opengl_functions.glStencilMaskSeparate(GL_BACK, DEFAULT_MASK);
			applied_state = true;
		}
	}

	return applied_state;
}


const GPlatesOpenGL::GLStencilOpStateSet::Op
GPlatesOpenGL::GLStencilOpStateSet::DEFAULT_OP{ GL_KEEP, GL_KEEP, GL_KEEP };

bool
GPlatesOpenGL::GLStencilOpStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLStencilOpStateSet &current = dynamic_cast<const GLStencilOpStateSet &>(current_state_set);

	bool applied_state = false;

	if (d_front_op == d_back_op)
	{
		// If either front or back op changed...
		if (d_front_op != current.d_front_op ||
			d_back_op != current.d_back_op)
		{
			// Both front/back ops are the same so set them in one call
			// (even though it's possible only one of the faces has changed).
			opengl_functions.glStencilOp(d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
			applied_state = true;
		}
	}
	else // Front and back stencil ops are different...
	{
		if (d_front_op != current.d_front_op)
		{
			opengl_functions.glStencilOpSeparate(GL_FRONT, d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
			applied_state = true;
		}
		if (d_back_op != current.d_back_op)
		{
			opengl_functions.glStencilOpSeparate(GL_BACK, d_back_op.sfail, d_back_op.dpfail, d_back_op.dppass);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLStencilOpStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_front_op == d_back_op)
	{
		if (d_front_op != DEFAULT_OP)
		{
			// Both front/back ops are the same so set them in one call.
			opengl_functions.glStencilOp(d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
			applied_state = true;
		}
	}
	else // Front and back stencil ops are different...
	{
		if (d_front_op != DEFAULT_OP)
		{
			opengl_functions.glStencilOpSeparate(GL_FRONT, d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
			applied_state = true;
		}
		if (d_back_op != DEFAULT_OP)
		{
			opengl_functions.glStencilOpSeparate(GL_BACK, d_back_op.sfail, d_back_op.dpfail, d_back_op.dppass);
			applied_state = true;
		}
	}

	return applied_state;
}

bool
GPlatesOpenGL::GLStencilOpStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	bool applied_state = false;

	if (d_front_op == d_back_op)
	{
		if (d_front_op != DEFAULT_OP)
		{
			// Both front/back ops are the same so set them in one call.
			opengl_functions.glStencilOp(DEFAULT_OP.sfail, DEFAULT_OP.dpfail, DEFAULT_OP.dppass);
			applied_state = true;
		}
	}
	else // Front and back stencil ops are different...
	{
		if (d_front_op != DEFAULT_OP)
		{
			opengl_functions.glStencilOpSeparate(GL_FRONT, DEFAULT_OP.sfail, DEFAULT_OP.dpfail, DEFAULT_OP.dppass);
			applied_state = true;
		}
		if (d_back_op != DEFAULT_OP)
		{
			opengl_functions.glStencilOpSeparate(GL_BACK, DEFAULT_OP.sfail, DEFAULT_OP.dpfail, DEFAULT_OP.dppass);
			applied_state = true;
		}
	}

	return applied_state;
}


bool
GPlatesOpenGL::GLUseProgramStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_program_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLUseProgramStateSet &>(current_state_set).d_program_resource)
	{
		return false;
	}

	// Use the program.
	opengl_functions.glUseProgram(d_program_resource);

	return true;
}

bool
GPlatesOpenGL::GLUseProgramStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_program_resource == 0)
	{
		return false;
	}

	// Use the program.
	opengl_functions.glUseProgram(d_program_resource);

	return true;
}

bool
GPlatesOpenGL::GLUseProgramStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_program_resource == 0)
	{
		return false;
	}

	// The default is zero (no program in use).
	opengl_functions.glUseProgram(0);

	return true;
}


bool
GPlatesOpenGL::GLViewportStateSet::apply_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLViewportStateSet &current = dynamic_cast<const GLViewportStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_viewport == current.d_viewport)
	{
		return false;
	}

	opengl_functions.glViewport(
			d_viewport.x(), d_viewport.y(),
			d_viewport.width(), d_viewport.height());

	return true;
}

bool
GPlatesOpenGL::GLViewportStateSet::apply_from_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_viewport == d_default_viewport)
	{
		return false;
	}

	opengl_functions.glViewport(
			d_viewport.x(), d_viewport.y(),
			d_viewport.width(), d_viewport.height());

	return true;
}

bool
GPlatesOpenGL::GLViewportStateSet::apply_to_default_state(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_viewport == d_default_viewport)
	{
		return false;
	}

	opengl_functions.glViewport(
			d_default_viewport.x(), d_default_viewport.y(),
			d_default_viewport.width(), d_default_viewport.height());

	return true;
}
