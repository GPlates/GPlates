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

#include <iostream>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLTextureEnvironmentState.h"

#include "GLContext.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLTextureEnvironmentState::GLTextureEnvironmentState() :
	d_active_texture_ARB(GL_TEXTURE0_ARB)
{
}


void
GPlatesOpenGL::GLTextureEnvironmentState::gl_active_texture_ARB(
		GLenum texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture >= GL_TEXTURE0_ARB &&
					texture < GL_TEXTURE0_ARB + GLContext::get_texture_parameters().gl_texture0_ARB,
			GPLATES_ASSERTION_SOURCE);

	d_active_texture_ARB = texture;
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLTextureEnvironmentState::enter_state_set() const
{
	if (GLEW_ARB_multitexture)
	{
		// Select the texture unit we want to set texture environment state on.
		glActiveTextureARB(d_active_texture_ARB);
	}

	if (d_enable_texture_2D)
	{
		if (d_enable_texture_2D.get())
		{
			glEnable(GL_TEXTURE_2D);
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
		}
	}

	if (d_tex_env_mode)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, d_tex_env_mode.get());
	}

	if (d_tex_env_colour)
	{
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, d_tex_env_colour.get());
	}
}


void
GPlatesOpenGL::GLTextureEnvironmentState::leave_state_set() const
{
	if (GLEW_ARB_multitexture)
	{
		// Select the texture unit that we initially set the texture environment state on.
		glActiveTextureARB(d_active_texture_ARB);
	}

	// Set states back to the default state.
	if (d_enable_texture_2D)
	{
		glDisable(GL_TEXTURE_2D);
	}

	if (d_tex_env_mode)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if (d_tex_env_colour)
	{
		const GLfloat default_colour[4] = { 0, 0, 0, 0 };
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, default_colour);
	}

	if (GLEW_ARB_multitexture)
	{
		if (d_active_texture_ARB != GL_TEXTURE0_ARB)
		{
			// But switch the active texture unit back to the default.
			glActiveTextureARB(GL_TEXTURE0_ARB/* default texture unit */);
		}
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")
