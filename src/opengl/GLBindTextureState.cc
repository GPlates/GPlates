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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLBindTextureState.h"
#include "GLContext.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLBindTextureState::GLBindTextureState() :
	d_target(GL_TEXTURE_2D),
	d_active_texture_ARB(GL_TEXTURE0_ARB)
{
}


void
GPlatesOpenGL::GLBindTextureState::gl_active_texture_ARB(
		GLenum texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture >= GL_TEXTURE0_ARB &&
					texture < GL_TEXTURE0_ARB + GLContext::get_max_texture_units_ARB(),
			GPLATES_ASSERTION_SOURCE);

	d_active_texture_ARB = texture;
}


void
GPlatesOpenGL::GLBindTextureState::gl_bind_texture(
		GLenum target,
		const GLTexture::shared_ptr_to_const_type &texture)
{
	d_target = target;
	d_bind_texture = texture;
}


void
GPlatesOpenGL::GLBindTextureState::enter_state_set() const
{
	const std::size_t MAX_TEXTURE_UNITS = GLContext::get_max_texture_units_ARB();

	if (MAX_TEXTURE_UNITS == 1)
	{
		// Only have one texture unit - might mean GL_ARB_multitexture is not supported
		// so avoid calling 'glActiveTextureARB()'.
		if (d_bind_texture)
		{
			// Bind the texture - there's only one texture unit so no need
			// to select the active texture unit.
			d_bind_texture.get()->gl_bind_texture(d_target);
		}

		return;
	}

	// else MAX_TEXTURE_UNITS > 1 ...

	if (d_bind_texture)
	{
		// Select the texture unit we want to bind the texture on.
		glActiveTextureARB(d_active_texture_ARB);

		// Bind the texture.
		d_bind_texture.get()->gl_bind_texture(d_target);
	}
}


void
GPlatesOpenGL::GLBindTextureState::leave_state_set() const
{
	// Leave the texture bound to the texture unit.
	// When we delete textures or switch OpenGL contexts we'll unbind textures.

	// But switch the active texture unit back to the default.
	if (d_active_texture_ARB != GL_TEXTURE0_ARB)
	{
		// If the active texture unit is not the first unit then to get here
		// we must have had support for more than one texture unit so we can
		// call the OpenGL extension function.
		glActiveTextureARB(GL_TEXTURE0_ARB/* default texture unit */);
	}
}
