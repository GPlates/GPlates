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
 
#ifndef GPLATES_OPENGL_GLTEXTURE_H
#define GPLATES_OPENGL_GLTEXTURE_H

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLResource.h"


namespace GPlatesOpenGL
{
	class GLTexture
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLTexture.
		typedef boost::shared_ptr<GLTexture> shared_ptr_type;
		typedef boost::shared_ptr<const GLTexture> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLTexture.
		typedef boost::weak_ptr<GLTexture> weak_ptr_type;
		typedef boost::weak_ptr<const GLTexture> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLTexture object.
		 *
		 * The internal texture resource is allocated by @a texture_resource_manager.
		 */
		static
		shared_ptr_type
		create(
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager)
		{
			return shared_ptr_type(new GLTexture(
					GLTextureResource::create(texture_resource_manager)));
		}


		/**
		 * Binds this texture to the current texture unit.
		 *
		 * It is the caller's responsibility to ensure that the texture unit that this
		 * texture should be bound to is the currently active texture unit.
		 *
		 * This calls same-named OpenGL function directly using the internal texture object.
		 */
		void
		gl_bind_texture(
				GLenum target) const;


		/**
		 * Returns the texture resource so it can be bound to a render-target for example.
		 */
		const GLTextureResource &
		get_texture_resource() const
		{
			return *d_texture_resource;
		}


	private:
		GLTextureResource::non_null_ptr_to_const_type d_texture_resource;


		//! Constructor.
		explicit
		GLTexture(
				const GLTextureResource::non_null_ptr_to_const_type &texture_resource) :
			d_texture_resource(texture_resource)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLTEXTURE_H
