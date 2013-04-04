/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <QDebug>

#include "GLRenderTarget.h"

#include "GLRenderer.h"

#include "utils/Base2Utils.h"


bool
GPlatesOpenGL::GLRenderTarget::is_supported(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer,
		unsigned int render_target_width,
		unsigned int render_target_height)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Test support for the texture internal format.
	if (!GLRenderTargetImpl::is_supported(renderer, texture_internalformat, include_depth_buffer))
	{
		return false;
	}

	// Require support for non-power-of-two textures if texture dimensions are not a power-of-two.
	if (!GPlatesUtils::Base2::is_power_of_two(render_target_width) ||
		!GPlatesUtils::Base2::is_power_of_two(render_target_height))
	{
		if (!capabilities.texture.gl_ARB_texture_non_power_of_two)
		{
			return false;
		}
	}

	// Must not exceed maximum texture dimensions.
	if (render_target_width > capabilities.texture.gl_max_texture_size ||
		render_target_height > capabilities.texture.gl_max_texture_size)
	{
		return false;
	}

	return true;
}


GPlatesOpenGL::GLRenderTarget::GLRenderTarget(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer,
		unsigned int render_target_width,
		unsigned int render_target_height) :
	d_impl(renderer, texture_internalformat, include_depth_buffer)
{
	d_impl.set_render_target_dimensions(renderer, render_target_width, render_target_height);
}


void
GPlatesOpenGL::GLRenderTarget::begin_render(
		GLRenderer &renderer)
{
	d_impl.begin_render(renderer);
}


void
GPlatesOpenGL::GLRenderTarget::end_render(
		GLRenderer &renderer)
{
	d_impl.end_render(renderer);
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLRenderTarget::get_texture() const
{
	return d_impl.get_texture();
}


GPlatesOpenGL::GLRenderTarget::RenderScope::RenderScope(
		GLRenderTarget &screen_render_target,
		GLRenderer &renderer) :
	d_screen_render_target(screen_render_target),
	d_renderer(renderer),
	d_called_end_render(false)
{
	d_screen_render_target.begin_render(renderer);
}


GPlatesOpenGL::GLRenderTarget::RenderScope::~RenderScope()
{
	if (!d_called_end_render)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_screen_render_target.end_render(d_renderer);
		}
		catch (std::exception &exc)
		{
			qWarning() << "GLRenderTarget: exception thrown during render scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "GLRenderTarget: exception thrown during render scope: Unknown error";
		}
	}
}


void
GPlatesOpenGL::GLRenderTarget::RenderScope::end_render()
{
	if (!d_called_end_render)
	{
		d_screen_render_target.end_render(d_renderer);
		d_called_end_render = true;
	}
}
