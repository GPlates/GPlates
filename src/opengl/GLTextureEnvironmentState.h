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
 
#ifndef GPLATES_OPENGL_GLTEXTUREENVIRONMENTSTATE_H
#define GPLATES_OPENGL_GLTEXTUREENVIRONMENTSTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets 'glTexEnv' state.
	 */
	class GLTextureEnvironmentState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLTextureEnvironmentState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLTextureEnvironmentState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLTextureEnvironmentState object with no state.
		 *
		 * Call @a gl_enable_texture_2D, etc to initialise the state.
		 * For example:
		 *   tex_env_state->gl_enable_texture_2D(GL_TRUE);
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLTextureEnvironmentState());
		}


		/**
		 * Selects the current texture unit that the other 'gl_...' calls apply to.
		 *
		 * Like the other 'gl*()' methods in this class the same-named call to OpenGL
		 * is not made here (it is delayed until @a enter_state_set is called).
		 *
		 * The default texture unit is texture unit 0, in which case it is not necessary
		 * to call this method. The default is texture unit 0 regardless of the currently
		 * active unit for some other @a GLTextureEnvironmentState object.
		 *
		 * If the runtime system doesn't support the GL_ARB_multitexture extension
		 * (and hence only supports one texture unit) then it is not necessary
		 * to call this function.
		 *
		 * If doesn't matter if you call this before or after the other 'gl_...' methods.
		 */
		void
		gl_active_texture_ARB(
				GLenum texture);


		GLTextureEnvironmentState &
		gl_enable_texture_2D(
				GLboolean enable)
		{
			d_enable_texture_2D = enable;
			return *this;
		}


		GLTextureEnvironmentState &
		gl_tex_env_mode(
				GLint mode = GL_MODULATE)
		{
			d_tex_env_mode = mode;
			return *this;
		}


		GLTextureEnvironmentState &
		gl_tex_env_colour(
				const GPlatesGui::Colour &colour)
		{
			d_tex_env_colour = colour;
			return *this;
		}


		virtual
		void
		enter_state_set() const;


		virtual
		void
		leave_state_set() const;

	private:
		//! The texture unit we are setting state for.
		GLenum d_active_texture_ARB;

		boost::optional<GLboolean> d_enable_texture_2D;
		boost::optional<GLint> d_tex_env_mode;
		boost::optional<GPlatesGui::Colour> d_tex_env_colour;

		//! Constructor.
		GLTextureEnvironmentState();
	};
}

#endif // GPLATES_OPENGL_GLTEXTUREENVIRONMENTSTATE_H
