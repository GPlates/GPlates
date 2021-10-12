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

#include "GLTextureTransformState.h"

#include "GLContext.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLTextureTransformState::GLTextureTransformState() :
	d_active_texture_ARB(GL_TEXTURE0_ARB)
{
}


void
GPlatesOpenGL::GLTextureTransformState::gl_active_texture_ARB(
		GLenum texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture >= GL_TEXTURE0_ARB &&
					texture < GL_TEXTURE0_ARB + GLContext::TextureParameters::gl_texture0_ARB,
			GPLATES_ASSERTION_SOURCE);

	d_active_texture_ARB = texture;
}


GPlatesOpenGL::GLTextureTransformState &
GPlatesOpenGL::GLTextureTransformState::set_tex_gen_coord_state(
		GLenum coord,
		const TexGenCoordState &tex_gen_coord_state)
{
	switch (coord)
	{
	case GL_S:
		d_tex_gen_s = tex_gen_coord_state;
		break;

	case GL_T:
		d_tex_gen_t = tex_gen_coord_state;
		break;

	case GL_R:
		d_tex_gen_r = tex_gen_coord_state;
		break;

	case GL_Q:
		d_tex_gen_q = tex_gen_coord_state;
		break;

	default:
		// Throws exception in release builds.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	return *this;
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLTextureTransformState::enter_state_set() const
{
	if (GLEW_ARB_multitexture)
	{
		// Select the texture unit we want to set texture transform state on.
		glActiveTextureARB(d_active_texture_ARB);
	}

	if (d_tex_gen_s)
	{
		enter_tex_gen_state(GL_S, d_tex_gen_s.get());
	}
	if (d_tex_gen_t)
	{
		enter_tex_gen_state(GL_T, d_tex_gen_t.get());
	}
	if (d_tex_gen_r)
	{
		enter_tex_gen_state(GL_R, d_tex_gen_r.get());
	}
	if (d_tex_gen_q)
	{
		enter_tex_gen_state(GL_Q, d_tex_gen_q.get());
	}

	if (d_texture_matrix)
	{
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixd(d_texture_matrix.get().get_matrix());
		glMatrixMode(GL_MODELVIEW);
	}
}


void
GPlatesOpenGL::GLTextureTransformState::leave_state_set() const
{
	if (GLEW_ARB_multitexture)
	{
		// Select the texture unit that we initially set the texture environment state on.
		glActiveTextureARB(d_active_texture_ARB);
	}

	// Set states back to the default state.

	if (d_tex_gen_s)
	{
		leave_tex_gen_state(GL_S, d_tex_gen_s.get());
	}
	if (d_tex_gen_t)
	{
		leave_tex_gen_state(GL_T, d_tex_gen_t.get());
	}
	if (d_tex_gen_r)
	{
		leave_tex_gen_state(GL_R, d_tex_gen_r.get());
	}
	if (d_tex_gen_q)
	{
		leave_tex_gen_state(GL_Q, d_tex_gen_q.get());
	}

	if (d_texture_matrix)
	{
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
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


void
GPlatesOpenGL::GLTextureTransformState::enter_tex_gen_state(
		GLenum coord,
		const TexGenCoordState &tex_gen_coord_state) const
{
	if (tex_gen_coord_state.d_enable_texture_gen)
	{
		if (tex_gen_coord_state.d_enable_texture_gen.get())
		{
			glEnable(GL_TEXTURE_GEN_S + (coord - GL_S));
		}
		else
		{
			glDisable(GL_TEXTURE_GEN_S + (coord - GL_S));
		}
	}

	if (tex_gen_coord_state.d_tex_gen_mode)
	{
		glTexGeni(coord, GL_TEXTURE_GEN_MODE, tex_gen_coord_state.d_tex_gen_mode.get());
	}

	if (tex_gen_coord_state.d_object_plane)
	{
		glTexGendv(coord, GL_OBJECT_PLANE, tex_gen_coord_state.d_object_plane->plane);
	}

	if (tex_gen_coord_state.d_eye_plane)
	{
		glTexGendv(coord, GL_OBJECT_PLANE, tex_gen_coord_state.d_eye_plane->plane);
	}
}


void
GPlatesOpenGL::GLTextureTransformState::leave_tex_gen_state(
		GLenum coord,
		const TexGenCoordState &tex_gen_coord_state) const
{
	if (tex_gen_coord_state.d_enable_texture_gen)
	{
		glDisable(GL_TEXTURE_GEN_S + (coord - GL_S));
	}

	if (tex_gen_coord_state.d_tex_gen_mode)
	{
		glTexGeni(coord, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	}
}
