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

#include "GLRenderTargetManager.h"

#include "GLContext.h"


GPlatesOpenGL::GLFrameBufferRenderTarget::non_null_ptr_type
GPlatesOpenGL::GLRenderTargetManager::get_frame_buffer_render_target()
{
	if (!d_frame_buffer_render_target)
	{
		d_frame_buffer_render_target =
				GLContext::get_render_target_factory().create_frame_buffer_render_target(d_context);
	}

	return d_frame_buffer_render_target.get();
}


GPlatesOpenGL::GLTextureRenderTarget::non_null_ptr_type
GPlatesOpenGL::GLRenderTargetManager::get_texture_render_target(
		unsigned int texture_width,
		unsigned int texture_height)
{
	const dimensions_type dimensions(texture_width, texture_height);

	// If we already have a texture render target matching the dimensions then return it.
	texture_render_target_map_type::iterator texture_rt_iter =
			d_texture_render_targets.find(dimensions);
	if (texture_rt_iter != d_texture_render_targets.end())
	{
		return texture_rt_iter->second;
	}

	// Create a new one.
	GLTextureRenderTarget::non_null_ptr_type texture_render_target =
			GLContext::get_render_target_factory().create_texture_render_target(
					texture_width, texture_height);

	// Add it to our map.
	d_texture_render_targets.insert(
			texture_render_target_map_type::value_type(
					dimensions, texture_render_target));

	return texture_render_target;
}
