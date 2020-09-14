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

#include <cmath>  // std::round

#include "GLStateSets.h"

#include "GLState.h"
#include "OpenGLException.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


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


void
GPlatesOpenGL::GLActiveTextureStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_active_texture ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLActiveTextureStateSet &>(current_state_set).d_active_texture)
	{
		return;
	}

	glActiveTexture(d_active_texture);
}

void
GPlatesOpenGL::GLActiveTextureStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_active_texture == GL_TEXTURE0)
	{
		return;
	}

	glActiveTexture(d_active_texture);
}

void
GPlatesOpenGL::GLActiveTextureStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (GL_TEXTURE0 == d_active_texture)
	{
		return;
	}

	glActiveTexture(GL_TEXTURE0);
}


void
GPlatesOpenGL::GLBindBufferStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_buffer_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindBufferStateSet &>(current_state_set).d_buffer_resource)
	{
		return;
	}

	// Bind the buffer object (can be zero).
	glBindBuffer(d_target, d_buffer_resource);
}

void
GPlatesOpenGL::GLBindBufferStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_buffer_resource == 0)
	{
		return;
	}

	// Bind the buffer object.
	glBindBuffer(d_target, d_buffer_resource);
}

void
GPlatesOpenGL::GLBindBufferStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_buffer_resource == 0)
	{
		return;
	}

	// The default is zero (no buffer object).
	glBindBuffer(d_target, 0);
}


void
GPlatesOpenGL::GLBindFramebufferStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBindFramebufferStateSet &current = dynamic_cast<const GLBindFramebufferStateSet &>(current_state_set);

	// Bind the framebuffer object (can be zero) to the draw or read target (or both).
	if (d_draw_framebuffer_resource == d_read_framebuffer_resource)
	{
		if (d_draw_framebuffer_resource != current.d_draw_framebuffer_resource ||
			d_read_framebuffer_resource != current.d_read_framebuffer_resource)
		{
			// Both draw/read targets are the same so bind them in one call
			// (even though it's possible only one of the targets has changed).
			glBindFramebuffer(GL_FRAMEBUFFER, d_draw_framebuffer_resource);
		}
	}
	else // Draw and read targets are bound to different framebuffers...
	{
		if (d_draw_framebuffer_resource != current.d_draw_framebuffer_resource)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_draw_framebuffer_resource);
		}
		if (d_read_framebuffer_resource != current.d_read_framebuffer_resource)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, d_read_framebuffer_resource);
		}
	}
}

void
GPlatesOpenGL::GLBindFramebufferStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Bind the framebuffer object to the draw or read target (or both).
	if (d_draw_framebuffer_resource == d_read_framebuffer_resource)
	{
		if (d_draw_framebuffer_resource != 0)
		{
			// Both draw/read targets are the same (and non-zero) so bind them in one call.
			glBindFramebuffer(GL_FRAMEBUFFER, d_draw_framebuffer_resource);
		}
	}
	else // Draw and read targets are bound to different framebuffers...
	{
		if (d_draw_framebuffer_resource != 0)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_draw_framebuffer_resource);
		}
		if (d_read_framebuffer_resource != 0)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, d_read_framebuffer_resource);
		}
	}
}

void
GPlatesOpenGL::GLBindFramebufferStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Bind the framebuffer object to the draw or read target (or both).
	if (d_draw_framebuffer_resource == d_read_framebuffer_resource)
	{
		if (d_draw_framebuffer_resource != 0)
		{
			// Both draw/read targets are the same (and non-zero) so bind them in one call.
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	else // Draw and read targets are bound to different framebuffers...
	{
		if (d_draw_framebuffer_resource != 0)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
		if (d_read_framebuffer_resource != 0)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
	}
}


void
GPlatesOpenGL::GLBindRenderbufferStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_renderbuffer_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindRenderbufferStateSet &>(current_state_set).d_renderbuffer_resource)
	{
		return;
	}

	// Bind the renderbuffer object (can be zero).
	glBindRenderbuffer(d_target, d_renderbuffer_resource);
}

void
GPlatesOpenGL::GLBindRenderbufferStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_renderbuffer_resource == 0)
	{
		return;
	}

	// Bind the renderbuffer object.
	glBindRenderbuffer(d_target, d_renderbuffer_resource);
}

void
GPlatesOpenGL::GLBindRenderbufferStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_renderbuffer_resource == 0)
	{
		return;
	}

	// The default is zero (no renderbuffer object).
	glBindRenderbuffer(d_target, 0);
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

void
GPlatesOpenGL::GLBindSamplerStateSet::apply_state(
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
		return;
	}

	// Bind the sampler object (can be zero).
	glBindSampler(d_unit, d_sampler_resource);
}

void
GPlatesOpenGL::GLBindSamplerStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_sampler_resource == 0)
	{
		return;
	}

	// Bind the sampler object.
	glBindSampler(d_unit, d_sampler_resource);
}

void
GPlatesOpenGL::GLBindSamplerStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_sampler_resource == 0)
	{
		return;
	}

	// The default is zero (no sampler object).
	glBindSampler(d_unit, 0);
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

void
GPlatesOpenGL::GLBindTextureStateSet::apply_state(
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
		return;
	}

	// Make sure the correct texture unit is currently active when binding texture.
	const GLenum current_active_texture = current_state.get_active_texture();
	if (d_texture_unit != current_active_texture)
	{
		glActiveTexture(d_texture_unit);
	}

	// Bind the texture object (can be zero).
	glBindTexture(d_texture_target, d_texture_resource);

	// Restore active texture.
	if (current_active_texture != d_texture_unit)
	{
		glActiveTexture(current_active_texture);
	}
}

void
GPlatesOpenGL::GLBindTextureStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_texture_resource == 0)
	{
		return;
	}

	// Make sure the correct texture unit is currently active when binding texture.
	const GLenum current_active_texture = current_state.get_active_texture();
	if (d_texture_unit != current_active_texture)
	{
		glActiveTexture(d_texture_unit);
	}

	// Bind the texture.
	glBindTexture(d_texture_target, d_texture_resource);

	// Restore active texture.
	if (current_active_texture != d_texture_unit)
	{
		glActiveTexture(current_active_texture);
	}
}

void
GPlatesOpenGL::GLBindTextureStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_texture_resource == 0)
	{
		return;
	}

	// Make sure the correct texture unit is currently active when unbinding texture.
	const GLenum current_active_texture = current_state.get_active_texture();
	if (d_texture_unit != current_active_texture)
	{
		glActiveTexture(d_texture_unit);
	}

	// Bind the default unnamed texture 0.
	glBindTexture(d_texture_target, 0);

	// Restore active texture.
	if (current_active_texture != d_texture_unit)
	{
		glActiveTexture(current_active_texture);
	}
}


void
GPlatesOpenGL::GLBindVertexArrayStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_array_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindVertexArrayStateSet &>(current_state_set).d_array_resource)
	{
		return;
	}

	// Bind the vertex array object (can be zero).
	glBindVertexArray(d_array_resource);
}

void
GPlatesOpenGL::GLBindVertexArrayStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_array_resource == 0)
	{
		return;
	}

	// Bind the vertex array object.
	glBindVertexArray(d_array_resource);
}

void
GPlatesOpenGL::GLBindVertexArrayStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_array_resource == 0)
	{
		return;
	}

	// The default is zero (no vertex array object).
    glBindVertexArray(0);
}


void
GPlatesOpenGL::GLBlendColorStateSet::apply_state(
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
		return;
	}

	glBlendColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));
}

void
GPlatesOpenGL::GLBlendColorStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return;
	}

	glBlendColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));
}

void
GPlatesOpenGL::GLBlendColorStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return;
	}

	glBlendColor(0, 0, 0, 0);
}


const GLenum GPlatesOpenGL::GLBlendEquationStateSet::DEFAULT_MODE = GL_FUNC_ADD;

void
GPlatesOpenGL::GLBlendEquationStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBlendEquationStateSet &current = dynamic_cast<const GLBlendEquationStateSet &>(current_state_set);

	if (d_mode_RGB == d_mode_alpha)
	{
		// If either RGB or alpha mode changed...
		if (d_mode_RGB != current.d_mode_RGB ||
			d_mode_alpha != current.d_mode_alpha)
		{
			// Both RGB/alpha modes are the same so set them in one call
			// (even though it's possible only one of the modes has changed).
			glBlendEquation(d_mode_RGB);
		}
	}
	else // RGB and alpha modes are different...
	{
		if (d_mode_RGB != current.d_mode_RGB ||
			d_mode_alpha != current.d_mode_alpha)
		{
			glBlendEquationSeparate(d_mode_RGB, d_mode_alpha);
		}
	}
}

void
GPlatesOpenGL::GLBlendEquationStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_mode_RGB == d_mode_alpha)
	{
		if (d_mode_RGB != DEFAULT_MODE)
		{
			// Both RGB/alpha modes are the same so set them in one call.
			glBlendEquation(d_mode_RGB);
		}
	}
	else // RGB and alpha modes are different...
	{
		// Both RGB and alpha modes are different, so they both cannot be the default state.
		glBlendEquationSeparate(d_mode_RGB, d_mode_alpha);
	}
}

void
GPlatesOpenGL::GLBlendEquationStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_mode_RGB == d_mode_alpha)
	{
		if (d_mode_RGB != DEFAULT_MODE)
		{
			// Both RGB/alpha modes are the same so set them in one call.
			glBlendEquation(DEFAULT_MODE);
		}
	}
	else // RGB and alpha modes are different...
	{
		// Both RGB and alpha modes are different, so they both cannot be the default state.
		glBlendEquationSeparate(DEFAULT_MODE, DEFAULT_MODE);
	}
}


const GPlatesOpenGL::GLBlendFuncStateSet::Func
GPlatesOpenGL::GLBlendFuncStateSet::DEFAULT_FUNC{ GL_ONE, GL_ZERO };

void
GPlatesOpenGL::GLBlendFuncStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLBlendFuncStateSet &current = dynamic_cast<const GLBlendFuncStateSet &>(current_state_set);

	if (d_RGB_func == d_alpha_func)
	{
		// If either RGB or alpha func changed...
		if (d_RGB_func != current.d_RGB_func ||
			d_alpha_func != current.d_alpha_func)
		{
			// Both RGB/alpha funcs are the same so set them in one call
			// (even though it's possible only one of the funcs has changed).
			glBlendFunc(d_RGB_func.src, d_RGB_func.dst);
		}
	}
	else // RGB and alpha blend funcs are different...
	{
		if (d_RGB_func != current.d_RGB_func ||
			d_alpha_func != current.d_alpha_func)
		{
			glBlendFuncSeparate(d_RGB_func.src, d_RGB_func.dst, d_alpha_func.src, d_alpha_func.dst);
		}
	}
}

void
GPlatesOpenGL::GLBlendFuncStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_RGB_func == d_alpha_func)
	{
		if (d_RGB_func != DEFAULT_FUNC)
		{
			// Both RGB/alpha funcs are the same so set them in one call.
			glBlendFunc(d_RGB_func.src, d_RGB_func.dst);
		}
	}
	else // RGB and alpha blend funcs are different...
	{
		// Both RGB and alpha funcs are different, so they both cannot be the default state.
		glBlendFuncSeparate(d_RGB_func.src, d_RGB_func.dst, d_alpha_func.src, d_alpha_func.dst);
	}
}

void
GPlatesOpenGL::GLBlendFuncStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_RGB_func == d_alpha_func)
	{
		if (d_RGB_func != DEFAULT_FUNC)
		{
			// Both RGB/alpha funcs are the same so set them in one call.
			glBlendFunc(DEFAULT_FUNC.src, DEFAULT_FUNC.dst);
		}
	}
	else // RGB and alpha blend funcs are different...
	{
		// Both RGB and alpha funcs are different, so they both cannot be the default state.
		glBlendFuncSeparate(DEFAULT_FUNC.src, DEFAULT_FUNC.dst, DEFAULT_FUNC.src, DEFAULT_FUNC.dst);
	}
}


void
GPlatesOpenGL::GLClampColorStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLClampColorStateSet &current = dynamic_cast<const GLClampColorStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_clamp == current.d_clamp)
	{
		return;
	}

	glClampColor(d_target, d_clamp);
}

void
GPlatesOpenGL::GLClampColorStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_clamp == GL_FIXED_ONLY)
	{
		return;
	}

	glClampColor(d_target, d_clamp);
}

void
GPlatesOpenGL::GLClampColorStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_clamp == GL_FIXED_ONLY)
	{
		return;
	}

	glClampColor(d_target, GL_FIXED_ONLY);
}


void
GPlatesOpenGL::GLClearColorStateSet::apply_state(
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
		return;
	}

	glClearColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));
}

void
GPlatesOpenGL::GLClearColorStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return;
	}

	glClearColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));
}

void
GPlatesOpenGL::GLClearColorStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return;
	}

	glClearColor(0, 0, 0, 0);
}


void
GPlatesOpenGL::GLClearDepthStateSet::apply_state(
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
		return;
	}

	glClearDepth(static_cast<GLclampd>(d_depth.dval()));
}

void
GPlatesOpenGL::GLClearDepthStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == 1.0)
	{
		return;
	}

	glClearDepth(static_cast<GLclampd>(d_depth.dval()));
}

void
GPlatesOpenGL::GLClearDepthStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == 1.0)
	{
		return;
	}

	glClearDepth(1.0);
}


void
GPlatesOpenGL::GLClearStencilStateSet::apply_state(
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
		return;
	}

	glClearStencil(d_stencil);
}

void
GPlatesOpenGL::GLClearStencilStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == 0)
	{
		return;
	}

	glClearStencil(d_stencil);
}

void
GPlatesOpenGL::GLClearStencilStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == 0)
	{
		return;
	}

	glClearStencil(0);
}


const GPlatesOpenGL::GLColorMaskStateSet::Mask
GPlatesOpenGL::GLColorMaskStateSet::DEFAULT_MASK{ GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };

void
GPlatesOpenGL::GLColorMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLColorMaskStateSet &current = dynamic_cast<const GLColorMaskStateSet &>(current_state_set);

	if (d_all_masks_equal && current.d_all_masks_equal)
	{
		// If state changed...
		if (d_masks[0] != current.d_masks[0])
		{
			glColorMask(d_masks[0].red, d_masks[0].green, d_masks[0].blue, d_masks[0].alpha);
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
				glColorMaski(i, d_masks[i].red, d_masks[i].green, d_masks[i].blue, d_masks[i].alpha);
			}
		}
	}
}

void
GPlatesOpenGL::GLColorMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_all_masks_equal)
	{
		// If state changed...
		if (d_masks[0] != DEFAULT_MASK)
		{
			glColorMask(d_masks[0].red, d_masks[0].green, d_masks[0].blue, d_masks[0].alpha);
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
				glColorMaski(i, d_masks[i].red, d_masks[i].green, d_masks[i].blue, d_masks[i].alpha);
			}
		}
	}
}

void
GPlatesOpenGL::GLColorMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_all_masks_equal)
	{
		// If state changed...
		if (d_masks[0] != DEFAULT_MASK)
		{
			glColorMask(DEFAULT_MASK.red, DEFAULT_MASK.green, DEFAULT_MASK.blue, DEFAULT_MASK.alpha);
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
				glColorMaski(i, DEFAULT_MASK.red, DEFAULT_MASK.green, DEFAULT_MASK.blue, DEFAULT_MASK.alpha);
			}
		}
	}
}


void
GPlatesOpenGL::GLCullFaceStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLCullFaceStateSet &current = dynamic_cast<const GLCullFaceStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_mode == current.d_mode)
	{
		return;
	}

	glCullFace(d_mode);
}

void
GPlatesOpenGL::GLCullFaceStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_BACK)
	{
		return;
	}

	glCullFace(d_mode);
}

void
GPlatesOpenGL::GLCullFaceStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_BACK)
	{
		return;
	}

	glCullFace(GL_BACK);
}


void
GPlatesOpenGL::GLDepthFuncStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDepthFuncStateSet &current = dynamic_cast<const GLDepthFuncStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_depth_func == current.d_depth_func)
	{
		return;
	}

	glDepthFunc(d_depth_func);
}

void
GPlatesOpenGL::GLDepthFuncStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_depth_func == GL_LESS)
	{
		return;
	}

	glDepthFunc(d_depth_func);
}

void
GPlatesOpenGL::GLDepthFuncStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_depth_func == GL_LESS)
	{
		return;
	}

	glDepthFunc(GL_LESS);
}


void
GPlatesOpenGL::GLDepthMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDepthMaskStateSet &current = dynamic_cast<const GLDepthMaskStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_flag == current.d_flag)
	{
		return;
	}

	glDepthMask(d_flag);
}

void
GPlatesOpenGL::GLDepthMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_flag == GL_TRUE)
	{
		return;
	}

	glDepthMask(d_flag);
}

void
GPlatesOpenGL::GLDepthMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_flag == GL_TRUE)
	{
		return;
	}

	glDepthMask(GL_TRUE);
}


void
GPlatesOpenGL::GLDepthRangeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDepthRangeStateSet &current = dynamic_cast<const GLDepthRangeStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_n == current.d_n && d_f == current.d_f)  // epsilon tests
	{
		return;
	}

	glDepthRange(d_n.dval(), d_f.dval());
}

void
GPlatesOpenGL::GLDepthRangeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_n == 0.0 && d_f == 1.0)  // epsilon tests
	{
		return;
	}

	glDepthRange(d_n.dval(), d_f.dval());
}

void
GPlatesOpenGL::GLDepthRangeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_n == 0.0 && d_f == 1.0)  // epsilon tests
	{
		return;
	}

	glDepthRange(0.0, 1.0);
}


void
GPlatesOpenGL::GLDrawBuffersStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLDrawBuffersStateSet &current = dynamic_cast<const GLDrawBuffersStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_draw_buffers == current.d_draw_buffers)
	{
		return;
	}

	glDrawBuffers(d_draw_buffers.size(), d_draw_buffers.data());
}

void
GPlatesOpenGL::GLDrawBuffersStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_draw_buffers.size() == 1 &&
		d_draw_buffers[0] == d_default_draw_buffer)
	{
		return;
	}

	glDrawBuffers(d_draw_buffers.size(), d_draw_buffers.data());
}

void
GPlatesOpenGL::GLDrawBuffersStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_draw_buffers.size() == 1 &&
		d_draw_buffers[0] == d_default_draw_buffer)
	{
		return;
	}

	glDrawBuffer(d_default_draw_buffer);
}


void
GPlatesOpenGL::GLEnableStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableStateSet &>(current_state_set).d_enable)
	{
		return;
	}

	if (d_enable)
	{
		glEnable(d_cap);
	}
	else
	{
		glDisable(d_cap);
	}
}

void
GPlatesOpenGL::GLEnableStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool enable_default = get_default(d_cap);

	// Return early if no state change...
	if (d_enable == enable_default)
	{
		return;
	}

	if (d_enable)
	{
		glEnable(d_cap);
	}
	else
	{
		glDisable(d_cap);
	}
}

void
GPlatesOpenGL::GLEnableStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool enable_default = get_default(d_cap);

	// Return early if no state change...
	if (d_enable == enable_default)
	{
		return;
	}

	if (enable_default)
	{
		glEnable(d_cap);
	}
	else
	{
		glDisable(d_cap);
	}
}

bool
GPlatesOpenGL::GLEnableStateSet::get_default(
		GLenum cap)
{
	// All capabilities default to GL_FALSE except two.
	return cap == GL_DITHER || cap == GL_MULTISAMPLE;
}


void
GPlatesOpenGL::GLEnableIndexedStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLEnableIndexedStateSet &current = dynamic_cast<const GLEnableIndexedStateSet &>(current_state_set);

	if (d_all_indices_equal && current.d_all_indices_equal)
	{
		// If state changed...
		if (d_indices[0] != current.d_indices[0])
		{
			// Enable/disable all indices with a single call.
			if (d_indices[0])
			{
				glEnable(d_cap);
			}
			else
			{
				glDisable(d_cap);
			}
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
					glEnablei(d_cap, i);
				}
				else
				{
					glDisablei(d_cap, i);
				}
			}
		}
	}
}

void
GPlatesOpenGL::GLEnableIndexedStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool default_ = GLEnableStateSet::get_default(d_cap);

	if (d_all_indices_equal)
	{
		// If state changed...
		if (d_indices[0] != default_)
		{
			// Enable/disable all indices with a single call.
			if (d_indices[0])
			{
				glEnable(d_cap);
			}
			else
			{
				glDisable(d_cap);
			}
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
					glEnablei(d_cap, i);
				}
				else
				{
					glDisablei(d_cap, i);
				}
			}
		}
	}
}

void
GPlatesOpenGL::GLEnableIndexedStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const bool default_ = GLEnableStateSet::get_default(d_cap);

	if (d_all_indices_equal)
	{
		// If state changed...
		if (d_indices[0] != default_)
		{
			// Enable/disable all indices with a single call.
			if (default_)
			{
				glEnable(d_cap);
			}
			else
			{
				glDisable(d_cap);
			}
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
					glEnablei(d_cap, i);
				}
				else
				{
					glDisablei(d_cap, i);
				}
			}
		}
	}
}


void
GPlatesOpenGL::GLFrontFaceStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_dir == dynamic_cast<const GLFrontFaceStateSet &>(current_state_set).d_dir)
	{
		return;
	}

	glFrontFace(d_dir);
}

void
GPlatesOpenGL::GLFrontFaceStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_dir == GL_CCW)
	{
		return;
	}

	glFrontFace(d_dir);
}

void
GPlatesOpenGL::GLFrontFaceStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_dir == GL_CCW)
	{
		return;
	}

	glFrontFace(GL_CCW);
}


void
GPlatesOpenGL::GLHintStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_hint == dynamic_cast<const GLHintStateSet &>(current_state_set).d_hint)
	{
		return;
	}

	glHint(d_target, d_hint);
}

void
GPlatesOpenGL::GLHintStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_hint == GL_DONT_CARE)
	{
		return;
	}

	glHint(d_target, d_hint);
}

void
GPlatesOpenGL::GLHintStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_hint == GL_DONT_CARE)
	{
		return;
	}

	glHint(d_target, GL_DONT_CARE);
}


void
GPlatesOpenGL::GLLineWidthStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	// NOTE: This is an epsilon test.
	if (d_width == dynamic_cast<const GLLineWidthStateSet &>(current_state_set).d_width)
	{
		return;
	}

	glLineWidth(d_width.dval());
}

void
GPlatesOpenGL::GLLineWidthStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_width == 1.0)
	{
		return;
	}

	glLineWidth(d_width.dval());
}

void
GPlatesOpenGL::GLLineWidthStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_width == 1.0)
	{
		return;
	}

	glLineWidth(GLfloat(1));
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
		d_param = (param != 0) ? 1 : 0;
	}
	else // all remaining pnames have type integer (in OpenGL 3.3)...
	{
		// OpenGL 3.3 spec says param is rounded to the nearest integer.
		d_param = static_cast<GLint>(std::round(param));
	}
}


void
GPlatesOpenGL::GLPixelStoreStateSet::apply_state(
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
		return;
	}

	// We're not using glPixelStoref (since all parameter types are boolean or integer).
	glPixelStorei(d_pname, d_param);
}

void
GPlatesOpenGL::GLPixelStoreStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const GLint param_default = get_default(d_pname);

	// Return early if no state change...
	if (d_param == param_default)
	{
		return;
	}

	// We're not using glPixelStoref (since all parameter types are boolean or integer).
	glPixelStorei(d_pname, d_param);
}

void
GPlatesOpenGL::GLPixelStoreStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const GLint param_default = get_default(d_pname);

	// Return early if no state change...
	if (d_param == param_default)
	{
		return;
	}

	// We're not using glPixelStoref (since all parameter types are boolean or integer).
	glPixelStorei(d_pname, param_default);
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


void
GPlatesOpenGL::GLPointSizeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	// NOTE: This is an epsilon test.
	if (d_size == dynamic_cast<const GLPointSizeStateSet &>(current_state_set).d_size)
	{
		return;
	}

	glPointSize(d_size.dval());
}

void
GPlatesOpenGL::GLPointSizeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_size == 1.0)
	{
		return;
	}

	glPointSize(d_size.dval());
}

void
GPlatesOpenGL::GLPointSizeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_size == 1.0)
	{
		return;
	}

	glPointSize(GLfloat(1));
}


void
GPlatesOpenGL::GLPolygonModeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_mode == dynamic_cast<const GLPolygonModeStateSet &>(current_state_set).d_mode)
	{
		return;
	}

	glPolygonMode(GL_FRONT_AND_BACK, d_mode);
}

void
GPlatesOpenGL::GLPolygonModeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_FILL)
	{
		return;
	}

	glPolygonMode(GL_FRONT_AND_BACK, d_mode);
}

void
GPlatesOpenGL::GLPolygonModeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_mode == GL_FILL)
	{
		return;
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_state(
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
		return;
	}

	glPolygonOffset(d_factor.dval(), d_units.dval());
}

void
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == 0 && d_units == 0)
	{
		return;
	}

	glPolygonOffset(d_factor.dval(), d_units.dval());
}

void
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == 0 && d_units == 0)
	{
		return;
	}

	glPolygonMode(GLfloat(0), GLfloat(0));
}


void
GPlatesOpenGL::GLPrimitiveRestartIndexStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLPrimitiveRestartIndexStateSet &current = dynamic_cast<const GLPrimitiveRestartIndexStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_index == current.d_index)
	{
		return;
	}

	glPrimitiveRestartIndex(d_index);
}

void
GPlatesOpenGL::GLPrimitiveRestartIndexStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_index == 0)
	{
		return;
	}

	glPrimitiveRestartIndex(d_index);
}

void
GPlatesOpenGL::GLPrimitiveRestartIndexStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no 0.d_index)
	{
		return;
	}

	glPrimitiveRestartIndex(0);
}


void
GPlatesOpenGL::GLReadBufferStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLReadBufferStateSet &current = dynamic_cast<const GLReadBufferStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_read_buffer == current.d_read_buffer)
	{
		return;
	}

	glReadBuffer(d_read_buffer);
}

void
GPlatesOpenGL::GLReadBufferStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_read_buffer == d_default_read_buffer)
	{
		return;
	}

	glReadBuffer(d_read_buffer);
}

void
GPlatesOpenGL::GLReadBufferStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_read_buffer == d_default_read_buffer)
	{
		return;
	}

	glReadBuffer(d_default_read_buffer);
}


void
GPlatesOpenGL::GLSampleCoverageStateSet::apply_state(
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
		return;
	}

	glSampleCoverage(
			static_cast<GLclampf>(d_value.dval()),
			d_invert);
}

void
GPlatesOpenGL::GLSampleCoverageStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_value == 1 /*epsilon test*/ &&
		d_invert == GL_FALSE)
	{
		return;
	}

	glSampleCoverage(
			static_cast<GLclampf>(d_value.dval()),
			d_invert);
}

void
GPlatesOpenGL::GLSampleCoverageStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_value == 1 /*epsilon test*/ &&
		d_invert == GL_FALSE)
	{
		return;
	}

	glSampleCoverage(1, GL_FALSE);
}


const GLbitfield GPlatesOpenGL::GLSampleMaskStateSet::DEFAULT_MASK = ~GLbitfield(0)/*all ones*/;

void
GPlatesOpenGL::GLSampleMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLSampleMaskStateSet &current = dynamic_cast<const GLSampleMaskStateSet &>(current_state_set);

	const GLuint num_masks = capabilities.gl_max_sample_mask_words;
	for (GLuint i = 0; i < num_masks; ++i)
	{
		// If state changed...
		if (d_masks[i] != current.d_masks[i])
		{
			glSampleMaski(i, d_masks[i]);
		}
	}
}

void
GPlatesOpenGL::GLSampleMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const GLuint num_masks = capabilities.gl_max_sample_mask_words;
	for (GLuint i = 0; i < num_masks; ++i)
	{
		// If state changed...
		if (d_masks[i] != DEFAULT_MASK)
		{
			glSampleMaski(i, d_masks[i]);
		}
	}
}

void
GPlatesOpenGL::GLSampleMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	const GLuint num_masks = capabilities.gl_max_sample_mask_words;
	for (GLuint i = 0; i < num_masks; ++i)
	{
		// If state changed...
		if (d_masks[i] != DEFAULT_MASK)
		{
			glSampleMaski(i, DEFAULT_MASK);
		}
	}
}


void
GPlatesOpenGL::GLScissorStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLScissorStateSet &current = dynamic_cast<const GLScissorStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_scissor_rectangle == current.d_scissor_rectangle)
	{
		return;
	}

	glScissor(
			d_scissor_rectangle.x(), d_scissor_rectangle.y(),
			d_scissor_rectangle.width(), d_scissor_rectangle.height());
}

void
GPlatesOpenGL::GLScissorStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_scissor_rectangle == d_default_scissor_rectangle)
	{
		return;
	}

	glScissor(
			d_scissor_rectangle.x(), d_scissor_rectangle.y(),
			d_scissor_rectangle.width(), d_scissor_rectangle.height());
}

void
GPlatesOpenGL::GLScissorStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_scissor_rectangle == d_default_scissor_rectangle)
	{
		return;
	}

	glScissor(
			d_default_scissor_rectangle.x(), d_default_scissor_rectangle.y(),
			d_default_scissor_rectangle.width(), d_default_scissor_rectangle.height());
}


const GPlatesOpenGL::GLStencilFuncStateSet::Func
GPlatesOpenGL::GLStencilFuncStateSet::DEFAULT_FUNC{ GL_ALWAYS, 0, ~GLuint(0) };

void
GPlatesOpenGL::GLStencilFuncStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLStencilFuncStateSet &current = dynamic_cast<const GLStencilFuncStateSet &>(current_state_set);

	if (d_front_func == d_back_func)
	{
		// If either front or back func changed...
		if (d_front_func != current.d_front_func ||
			d_back_func != current.d_back_func)
		{
			// Both front/back funcs are the same so set them in one call
			// (even though it's possible only one of the faces has changed).
			glStencilFunc(d_front_func.func, d_front_func.ref, d_front_func.mask);
		}
	}
	else // Front and back stencil funcs are different...
	{
		if (d_front_func != current.d_front_func)
		{
			glStencilFuncSeparate(GL_FRONT, d_front_func.func, d_front_func.ref, d_front_func.mask);
		}
		if (d_back_func != current.d_back_func)
		{
			glStencilFuncSeparate(GL_BACK, d_back_func.func, d_back_func.ref, d_back_func.mask);
		}
	}
}

void
GPlatesOpenGL::GLStencilFuncStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_front_func == d_back_func)
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			// Both front/back funcs are the same so set them in one call.
			glStencilFunc(d_front_func.func, d_front_func.ref, d_front_func.mask);
		}
	}
	else // Front and back stencil funcs are different...
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			glStencilFuncSeparate(GL_FRONT, d_front_func.func, d_front_func.ref, d_front_func.mask);
		}
		if (d_back_func != DEFAULT_FUNC)
		{
			glStencilFuncSeparate(GL_BACK, d_back_func.func, d_back_func.ref, d_back_func.mask);
		}
	}
}

void
GPlatesOpenGL::GLStencilFuncStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_front_func == d_back_func)
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			// Both front/back funcs are the same so set them in one call.
			glStencilFunc(DEFAULT_FUNC.func, DEFAULT_FUNC.ref, DEFAULT_FUNC.mask);
		}
	}
	else // Front and back stencil funcs are different...
	{
		if (d_front_func != DEFAULT_FUNC)
		{
			glStencilFuncSeparate(GL_FRONT, DEFAULT_FUNC.func, DEFAULT_FUNC.ref, DEFAULT_FUNC.mask);
		}
		if (d_back_func != DEFAULT_FUNC)
		{
			glStencilFuncSeparate(GL_BACK, DEFAULT_FUNC.func, DEFAULT_FUNC.ref, DEFAULT_FUNC.mask);
		}
	}
}


const GLuint GPlatesOpenGL::GLStencilMaskStateSet::DEFAULT_MASK = ~GLuint(0)/*all ones*/;

void
GPlatesOpenGL::GLStencilMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLStencilMaskStateSet &current = dynamic_cast<const GLStencilMaskStateSet &>(current_state_set);

	if (d_front_mask == d_back_mask)
	{
		// If either front or back mask changed...
		if (d_front_mask != current.d_front_mask ||
			d_back_mask != current.d_back_mask)
		{
			// Both front/back masks are the same so set them in one call
			// (even though it's possible only one of the faces has changed).
			glStencilMask(d_front_mask);
		}
	}
	else // Front and back stencil masks are different...
	{
		if (d_front_mask != current.d_front_mask)
		{
			glStencilMaskSeparate(GL_FRONT, d_front_mask);
		}
		if (d_back_mask != current.d_back_mask)
		{
			glStencilMaskSeparate(GL_BACK, d_back_mask);
		}
	}
}

void
GPlatesOpenGL::GLStencilMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_front_mask == d_back_mask)
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			// Both front/back masks are the same so set them in one call.
			glStencilMask(d_front_mask);
		}
	}
	else // Front and back stencil masks are different...
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			glStencilMaskSeparate(GL_FRONT, d_front_mask);
		}
		if (d_back_mask != DEFAULT_MASK)
		{
			glStencilMaskSeparate(GL_BACK, d_back_mask);
		}
	}
}

void
GPlatesOpenGL::GLStencilMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_front_mask == d_back_mask)
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			// Both front/back masks are the same so set them in one call.
			glStencilMask(DEFAULT_MASK);
		}
	}
	else // Front and back stencil masks are different...
	{
		if (d_front_mask != DEFAULT_MASK)
		{
			glStencilMaskSeparate(GL_FRONT, DEFAULT_MASK);
		}
		if (d_back_mask != DEFAULT_MASK)
		{
			glStencilMaskSeparate(GL_BACK, DEFAULT_MASK);
		}
	}
}


const GPlatesOpenGL::GLStencilOpStateSet::Op
GPlatesOpenGL::GLStencilOpStateSet::DEFAULT_OP{ GL_KEEP, GL_KEEP, GL_KEEP };

void
GPlatesOpenGL::GLStencilOpStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLStencilOpStateSet &current = dynamic_cast<const GLStencilOpStateSet &>(current_state_set);

	if (d_front_op == d_back_op)
	{
		// If either front or back op changed...
		if (d_front_op != current.d_front_op ||
			d_back_op != current.d_back_op)
		{
			// Both front/back ops are the same so set them in one call
			// (even though it's possible only one of the faces has changed).
			glStencilOp(d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
		}
	}
	else // Front and back stencil ops are different...
	{
		if (d_front_op != current.d_front_op)
		{
			glStencilOpSeparate(GL_FRONT, d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
		}
		if (d_back_op != current.d_back_op)
		{
			glStencilOpSeparate(GL_BACK, d_back_op.sfail, d_back_op.dpfail, d_back_op.dppass);
		}
	}
}

void
GPlatesOpenGL::GLStencilOpStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_front_op == d_back_op)
	{
		if (d_front_op != DEFAULT_OP)
		{
			// Both front/back ops are the same so set them in one call.
			glStencilOp(d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
		}
	}
	else // Front and back stencil ops are different...
	{
		if (d_front_op != DEFAULT_OP)
		{
			glStencilOpSeparate(GL_FRONT, d_front_op.sfail, d_front_op.dpfail, d_front_op.dppass);
		}
		if (d_back_op != DEFAULT_OP)
		{
			glStencilOpSeparate(GL_BACK, d_back_op.sfail, d_back_op.dpfail, d_back_op.dppass);
		}
	}
}

void
GPlatesOpenGL::GLStencilOpStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	if (d_front_op == d_back_op)
	{
		if (d_front_op != DEFAULT_OP)
		{
			// Both front/back ops are the same so set them in one call.
			glStencilOp(DEFAULT_OP.sfail, DEFAULT_OP.dpfail, DEFAULT_OP.dppass);
		}
	}
	else // Front and back stencil ops are different...
	{
		if (d_front_op != DEFAULT_OP)
		{
			glStencilOpSeparate(GL_FRONT, DEFAULT_OP.sfail, DEFAULT_OP.dpfail, DEFAULT_OP.dppass);
		}
		if (d_back_op != DEFAULT_OP)
		{
			glStencilOpSeparate(GL_BACK, DEFAULT_OP.sfail, DEFAULT_OP.dpfail, DEFAULT_OP.dppass);
		}
	}
}


void
GPlatesOpenGL::GLUseProgramStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_program_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLUseProgramStateSet &>(current_state_set).d_program_resource)
	{
		return;
	}

	// Use the program.
	glUseProgram(d_program_resource);
}

void
GPlatesOpenGL::GLUseProgramStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_program_resource == 0)
	{
		return;
	}

	// Use the program.
	glUseProgram(d_program_resource);
}

void
GPlatesOpenGL::GLUseProgramStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_program_resource == 0)
	{
		return;
	}

	// The default is zero (no program in use).
    glUseProgram(0);
}


void
GPlatesOpenGL::GLViewportStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &current_state_set,
		const GLState &current_state) const
{
	// Throws exception if downcast fails...
	const GLViewportStateSet &current = dynamic_cast<const GLViewportStateSet &>(current_state_set);

	// Return early if no state change...
	if (d_viewport == current.d_viewport)
	{
		return;
	}

	glViewport(
			d_viewport.x(), d_viewport.y(),
			d_viewport.width(), d_viewport.height());
}

void
GPlatesOpenGL::GLViewportStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_viewport == d_default_viewport)
	{
		return;
	}

	glViewport(
			d_viewport.x(), d_viewport.y(),
			d_viewport.width(), d_viewport.height());
}

void
GPlatesOpenGL::GLViewportStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		const GLState &current_state) const
{
	// Return early if no state change...
	if (d_viewport == d_default_viewport)
	{
		return;
	}

	glViewport(
			d_default_viewport.x(), d_default_viewport.y(),
			d_default_viewport.width(), d_default_viewport.height());
}
