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
 
#ifndef GPLATES_OPENGL_GLRESOURCEMANAGER_H
#define GPLATES_OPENGL_GLRESOURCEMANAGER_H

#include <vector>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>


namespace GPlatesOpenGL
{
	/**
	 * Allocates and deallocates OpenGL resources (such as texture objects).
	 */
	template <typename ResourceType, class ResourceAllocatorType>
	class GLResourceManager
	{
	public:
		//! Typedef for this class.
		typedef GLResourceManager<ResourceType, ResourceAllocatorType> this_type;

		//! Typedef for a shared pointer to @a GLResourceManager.
		typedef boost::shared_ptr<this_type> shared_ptr_type;


		/**
		 * Creates a @a GLResourceManager object.
		 */
		static
		shared_ptr_type
		create()
		{
			return shared_ptr_type(new this_type());
		}


		/**
		 * Allocates an OpenGL resource using the 'ResourceAllocatorType' policy.
		 */
		ResourceType
		allocate_resource()
		{
			return d_resource_allocator.allocate();
		}

		/**
		 * Queues a resource for deallocation when @a deallocate_queued_resources is called.
		 *
		 * This deferral of deallocation is to ensure that no OpenGL calls (to deallocate)
		 * are made when an OpenGL context is not active - this allows us to destroy
		 * objects containing these OpenGL resources any time (not just when a context is active).
		 */
		void
		queue_resource_for_deallocation(
				ResourceType resource)
		{
			d_resource_deallocation_queue.push_back(resource);
		}

		/**
		 * Deallocates all resources queued up by @a queue_resource_for_deallocation.
		 *
		 * NOTE: This should be called periodically when the OpenGL context is active
		 * to ensure resources get released in a timely manner. Immediately after
		 * a render graph is rendered is a good time.
		 */
		void
		deallocate_queued_resources()
		{
			BOOST_FOREACH(ResourceType resource, d_resource_deallocation_queue)
			{
				d_resource_allocator.deallocate(resource);
			}

			d_resource_deallocation_queue.clear();
		}

	private:
		typedef std::vector<ResourceType> resource_deallocation_queue_type;

		ResourceAllocatorType d_resource_allocator;
		resource_deallocation_queue_type d_resource_deallocation_queue;


		GLResourceManager()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRESOURCEMANAGER_H
