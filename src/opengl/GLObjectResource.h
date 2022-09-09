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
 
#ifndef GPLATES_OPENGL_GLOBJECTRESOURCE_H
#define GPLATES_OPENGL_GLOBJECTRESOURCE_H

#include <QDebug>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "GLContext.h"
#include "OpenGLFunctions.h"


namespace GPlatesOpenGL
{
	/**
	 * An RAII wrapper around an OpenGL object resource (such as a texture object)
	 * that deallocates it, through the @a GLContext, when its destructor is called.
	 */
	template <typename ResourceHandleType, class ResourceAllocatorType>
	class GLObjectResource
	{
	public:

		GLObjectResource(
				OpenGLFunctions &opengl_functions,
				GLContext &context) :
			// Allocate OpenGL resource...
			d_resource_handle(ResourceAllocatorType::allocate(opengl_functions)),
			d_context_handle(context.get_context_handle())
		{  }

		/**
		 * Overload supporting a ResourceAllocatorType::allocate() requiring an extra argument.
		 */
		template <typename AllocateArg1>
		GLObjectResource(
				OpenGLFunctions &opengl_functions,
				GLContext &context,
				const AllocateArg1 &allocate_arg1) :
			// Allocate OpenGL resource...
			d_resource_handle(ResourceAllocatorType::allocate(opengl_functions, allocate_arg1)),
			d_context_handle(context.get_context_handle())
		{  }


		~GLObjectResource()
		{
			//
			// Only attempt to deallocate if the GLContext still exists and it is between initialisation and shutdown.
			// If all owners of OpenGL resources cleaned up properly then this will be the case.
			// So if this is not the case then emit a warning.
			//

			// See if GLContext still exists.
			boost::shared_ptr<GLContext> context = d_context_handle.lock();
			if (!context)
			{
				qWarning() << "OpenGL resource not destroyed: context no longer exists.";
				return;
			}

			// See if GLContext is between initialisation and shutdown.
			boost::optional<OpenGLFunctions &> opengl_functions = context->get_opengl_functions();
			if (!opengl_functions)
			{
				qWarning() << "OpenGL resource not destroyed: context not active.";
				return;
			}

			// Deallocate OpenGL resource.
			ResourceAllocatorType::deallocate(opengl_functions.get(), d_resource_handle);
		}


		/**
		 * Returns the resource handle held internally.
		 */
		ResourceHandleType
		get_resource_handle() const
		{
			return d_resource_handle;
		}

	private:
		ResourceHandleType d_resource_handle;

		/**
		 * A pointer to @a GLContext that does not keep it alive.
		 */
		boost::weak_ptr<GLContext> d_context_handle;
	};
}

#endif // GPLATES_OPENGL_GLOBJECTRESOURCE_H
