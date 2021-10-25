/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLRENDERBUFFEROBJECT_H
#define GPLATES_OPENGL_GLRENDERBUFFEROBJECT_H

#include <memory> // For std::auto_ptr
#include <utility>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A render buffer object to be used with @a GLFrameBufferObject.
	 *
	 * Note that the GL_EXT_framebuffer_object extension must be supported (ie, GLFrameBufferObject).
	 */
	class GLRenderBufferObject :
			public GLObject,
			public boost::enable_shared_from_this<GLRenderBufferObject>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLRenderBufferObject.
		typedef boost::shared_ptr<GLRenderBufferObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLRenderBufferObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLRenderBufferObject.
		typedef boost::weak_ptr<GLRenderBufferObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLRenderBufferObject> weak_ptr_to_const_type;


		//! Typedef for a resource handle.
		typedef GLuint resource_handle_type;

		/**
		 * Policy class to allocate and deallocate OpenGL render buffer objects.
		 */
		class Allocator
		{
		public:
			resource_handle_type
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					resource_handle_type render_buffer);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<resource_handle_type, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<resource_handle_type, Allocator> resource_manager_type;


		/**
		 * Creates a shared pointer to a @a GLRenderBufferObject object.
		 *
		 * Note that the GL_EXT_framebuffer_object extension must be supported.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(new GLRenderBufferObject(renderer));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 *
		 * Note that the GL_EXT_framebuffer_object extension must be supported.
		 */
		static
		std::auto_ptr<GLRenderBufferObject>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLRenderBufferObject>(new GLRenderBufferObject(renderer));
		}


		/**
		 * Performs same function as the glRenderBufferStorage OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a width or @a height is greater than
		 * context.get_capabilities().framebuffer.gl_max_renderbuffer_size.
		 */
		void
		gl_render_buffer_storage(
				GLRenderer &renderer,
				GLint internalformat,
				GLsizei width,
				GLsizei height);


		//
		// General query methods.
		//


		/**
		 * Returns the dimensions of the render buffer.
		 *
		 * Returns boost::none unless @a gl_render_buffer_storage has been called.
		 */
		const boost::optional< std::pair<GLuint/*width*/, GLuint/*height*/> > &
		get_dimensions() const
		{
			return d_dimensions;
		}


		/**
		 * Returns the internal format of the texture.
		 *
		 * Returns boost::none unless @a gl_render_buffer_storage has been called.
		 */
		boost::optional<GLint>
		get_internal_format() const
		{
			return d_internal_format;
		}


		/**
		 * Returns the render buffer resource handle.
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		resource_handle_type
		get_render_buffer_resource_handle() const
		{
			return d_resource->get_resource_handle();
		}

	private:
		resource_type::non_null_ptr_to_const_type d_resource;

		boost::optional< std::pair<GLuint/*width*/, GLuint/*height*/> > d_dimensions;

		boost::optional<GLint> d_internal_format;

		//! Constructor.
		explicit
		GLRenderBufferObject(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLRENDERBUFFEROBJECT_H
