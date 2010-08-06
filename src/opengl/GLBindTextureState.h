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
 
#ifndef GPLATES_OPENGL_GLBINDTEXTURESTATE_H
#define GPLATES_OPENGL_GLBINDTEXTURESTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"
#include "GLTexture.h"


namespace GPlatesOpenGL
{
	/**
	 * Used to bind a texture to a texture unit.
	 */
	class GLBindTextureState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLBindTextureState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLBindTextureState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLBindTextureState object with no bound texture.
		 *
		 * Call @a gl_bind_texture (and optionally @a gl_active_texture_ARB) to
		 * specify a texture to be bound.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLBindTextureState());
		}


		/**
		 * Selects the current texture unit that a subsequent @a gl_bind_texture
		 * should be directed to.
		 *
		 * Like the other 'gl*()' methods in this class the same-named call to OpenGL
		 * is not made here (it is delayed until @a enter_state_set is called).
		 *
		 * The default texture unit is texture unit 0, in which case it is not necessary
		 * to call this before calling @a gl_bind_texture. The default is texture unit 0
		 * regardless of the currently active unit for some other @a GLBindTextureState object.
		 *
		 * If the runtime system doesn't support the GL_ARB_multitexture extension
		 * (and hence only supports one texture unit) then it is not necessary
		 * to call this function.
		 */
		void
		gl_active_texture_ARB(
				GLenum texture);


		/**
		 * Binds @a texture to the current texture unit (see @a gl_active_texture_ARB).
		 *
		 * The same-named call to OpenGL is delayed until @a enter_state_set is called.
		 */
		void
		gl_bind_texture(
				GLenum target,
				const GLTexture::shared_ptr_to_const_type &texture);


		virtual
		void
		enter_state_set() const;


		/**
		 * Leaves the active texture unit as the default (first texture unit), but
		 * does not unbind the texture.
		 */
		virtual
		void
		leave_state_set() const;

	private:
		//! The texture target (eg, GL_TEXTURE_2D).
		GLenum d_target;

		// Which texture unit are we currently directing to ?
		GLenum d_active_texture_ARB;

		boost::optional<GLTexture::shared_ptr_to_const_type> d_bind_texture;


		GLBindTextureState();
	};
}

#endif // GPLATES_OPENGL_GLBINDTEXTURESTATE_H
