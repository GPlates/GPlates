/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2020 The University of Sydney, Australia
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

#include "GLSampler.h"

#include "GLContext.h"
#include "GL.h"


GPlatesOpenGL::GLSampler::GLSampler(
		GL &gl) :
	d_resource(
			resource_type::create(
					gl.get_capabilities(),
					gl.get_context().get_shared_state()->get_sampler_resource_manager()))
{
}


GLuint
GPlatesOpenGL::GLSampler::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}


GLuint
GPlatesOpenGL::GLSampler::Allocator::allocate(
		const GLCapabilities &capabilities)
{
	GLuint sampler;
	glGenSamplers(1, &sampler);
	return sampler;
}


void
GPlatesOpenGL::GLSampler::Allocator::deallocate(
		GLuint sampler)
{
	glDeleteSamplers(1, &sampler);
}
