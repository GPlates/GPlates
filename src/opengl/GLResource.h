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
 
#ifndef GPLATES_OPENGL_GLRESOURCE_H
#define GPLATES_OPENGL_GLRESOURCE_H

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "GLResourceManager.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * An RAII wrapper around an OpenGL resource (such as a texture object)
	 * that schedules the resource to be deallocated when its destructor is called.
	 */
	template <typename ResourceType, class ResourceAllocatorType>
	class GLResource :
			public GPlatesUtils::ReferenceCount<GLResource<ResourceType,ResourceAllocatorType> >
	{
	public:
		//! Typedef for this class type.
		typedef GLResource<ResourceType,ResourceAllocatorType> this_type;

		//! A convenience typedef for a shared pointer to a non-const @a GLResource.
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLResource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

		//! Typedef for the manager of this resource type.
		typedef GLResourceManager<ResourceType, ResourceAllocatorType> resource_manager_type;


		/**
		 * Creates a @a GLResource from a resource manager.
		 */
		static
		non_null_ptr_type
		create(
				const typename resource_manager_type::shared_ptr_type &resource_manager)
		{
			return non_null_ptr_type(
					new GLResource(
							resource_manager->allocate_resource(),
							resource_manager));
		}


		~GLResource()
		{
			// Only attempt to release the resource if the resource manager still exists.
			// If it doesn't exist it means the OpenGL context was destroyed which will
			// in turn destroy all existing resources in that context - which means
			// we don't have to worry about it and neither does the resource manager.
			boost::shared_ptr<resource_manager_type> resource_manager = d_resource_manager.lock();
			if (resource_manager)
			{
				resource_manager->queue_resource_for_deallocation(d_resource);
			}
		}


		/**
		 * Returns the resource held internally.
		 */
		ResourceType
		get_resource() const
		{
			return d_resource;
		}

	private:
		ResourceType d_resource;
		boost::weak_ptr<resource_manager_type> d_resource_manager;


		GLResource(
				ResourceType resource,
				const boost::weak_ptr<resource_manager_type> &resource_manager) :
			d_resource(resource),
			d_resource_manager(resource_manager)
		{  }
	};


	/**
	 * Typedef for a texture object resource.
	 */
	typedef GLResource<GLuint, GLTextureObjectAllocator> GLTextureResource;
}

#endif // GPLATES_OPENGL_GLRESOURCE_H
