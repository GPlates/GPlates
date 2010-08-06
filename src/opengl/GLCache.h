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
 
#ifndef GPLATES_OPENGL_GLCACHE_H
#define GPLATES_OPENGL_GLCACHE_H

#include <cstddef> // For std::size_t
#include <list>
#include <utility>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLVolatileObject.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Maintains a limited number of objects in a cache that are recycled for
	 * future cache requests.
	 *
	 * 'ObjectType' should have 'shared_ptr_type' and 'weak_ptr_type' nested typedefs
	 * which behave like boost::shared_ptr and boost::weak_ptr.
	 *
	 * 'ObjectCreatorType' must be copy-constructable and have a 'create()' method that
	 * takes no arguments and returns a 'ObjectType::shared_ptr_type'.
	 */
	template <typename ObjectType, class ObjectCreatorType>
	class GLCache :
			public GPlatesUtils::ReferenceCount<GLCache<ObjectType, ObjectCreatorType> >
	{
	public:
		//! Typedef for this class.
		typedef GLCache<ObjectType, ObjectCreatorType> this_type;

		//! Typedef for the volatile object managed by this cache.
		typedef GLVolatileObject<ObjectType> volatile_object_type;

		//! A convenience typedef for a shared pointer to a non-const @a GLCache.
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCache.
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLCache object.
		 *
		 * @a max_num_objects is a soft limit on the number of objects in the cache.
		 * If that will be limit exceeded then to prevent that the least-recently used object
		 * will be recycled if it is not being referenced.
		 * Otherwise the limit may have to be exceeded.
		 */
		static
		non_null_ptr_type
		create(
				std::size_t max_num_objects,
				const ObjectCreatorType &object_creator = ObjectCreatorType())
		{
			return non_null_ptr_type(new GLCache(max_num_objects, object_creator));
		}


		/**
		 * Returns a volatile object - an object that can be recycled.
		 *
		 * Also returns a boolean notifying the caller whether the object was
		 * created from scratch or whether it's an existing object being recycled.
		 *
		 * The returned volatile object can be recycled by a subsequent object cache request.
		 * Recycling happens when the maximum number of objects have been returned to clients
		 * and a new request forces the least recently requested object to be recycled.
		 *
		 * The returned volatile object can be converted to a shared reference (to a
		 * regular object) during scene rendering, for example, to ensure it doesn't get
		 * recycled in the middle of rendering, but once rendering has finished that shared
		 * reference should be destroyed to enable that object to be recycled for a
		 * subsequent rendering of the scene.
		 */
		std::pair<volatile_object_type, bool/*recycled*/>
		allocate_object();

	private:
		struct ObjectInfo
		{
			explicit
			ObjectInfo(
					const typename ObjectType::shared_ptr_type &object_) :
				object(object_),
				// The content of the token is irrelevant - can be any type and any value
				volatile_token(new int())
			{  }

			//! An owning reference to the cached object.
			typename ObjectType::shared_ptr_type object;

			/**
			 * A token to communicate to @a GLVolatileObject whether it has
			 * been recycled or not.
			 *
			 * The content of the token is irrelevant - we're just using the
			 * boost shared_ptr/weak_ptr as an observer mechanism where destruction
			 * if the token makes the weak pointers expire and that is what the
			 * @a GLVolatileObject objects check for to see if they've been recycled.
			 */
			boost::shared_ptr<void> volatile_token;
		};

		typedef std::list<ObjectInfo> object_seq_type;

		/**
		 * Used to allocate objects.
		 */
		ObjectCreatorType d_object_creator;

		/**
		 * List of cached objects ordered from least-recently to most-recently requested.
		 */
		object_seq_type d_objects;

		std::size_t d_num_objects_allocated;
		std::size_t d_max_num_objects;


		//! Constructor.
		explicit
		GLCache(
				std::size_t max_num_objects,
				const ObjectCreatorType &object_creator) :
			d_object_creator(object_creator),
			d_num_objects_allocated(0),
			d_max_num_objects(max_num_objects)
		{  }

		/**
		 * Returns true if we were able to recycle an existing object.
		 */
		boost::optional<volatile_object_type>
		recycle_object();
	};
}


namespace GPlatesOpenGL
{
	////////////////////
	// Implementation //
	////////////////////


	template <typename ObjectType, class ObjectCreatorType>
	std::pair<
			typename GLCache<ObjectType, ObjectCreatorType>::volatile_object_type,
			bool/*recycled*/>
	GLCache<ObjectType, ObjectCreatorType>::allocate_object()
	{
		if (d_num_objects_allocated >= d_max_num_objects)
		{
			// We have already allocated the maximum number of objects so
			// attempt to recycle an existing object.
			boost::optional<volatile_object_type> recycled_object = recycle_object();
			if (recycled_object)
			{
				return std::make_pair(recycled_object.get(), true/*recycled*/);
			}

			// If we get here then we were unable to recycle any existing objects
			// because clients had shared references to all of them.
			// So we'll have to create a new object.
			qWarning() << "GLCache: cache limit exceeded";
		}

		// Create a new object.
		typename ObjectType::shared_ptr_type new_object = d_object_creator.create();

		// Give new object a volatile token.
		const ObjectInfo new_object_info(new_object);

		// Add to our list of cached objects.
		// Add to the end of the list as that's where the most recent requests go.
		d_objects.push_back(new_object_info);
		++d_num_objects_allocated;

		return std::make_pair(
				volatile_object_type(new_object_info.object, new_object_info.volatile_token),
				false/*recycled*/);
	}


	template <typename ObjectType, class ObjectCreatorType>
	boost::optional<typename GLCache<ObjectType, ObjectCreatorType>::volatile_object_type>
	GLCache<ObjectType, ObjectCreatorType>::recycle_object()
	{
		// See if we can recycle an object.
		// Start with the least-recently allocated objects first.
		// The sequence is ordered from least-recently to most-recently requested.
		typename object_seq_type::iterator objects_iter = d_objects.begin();
		typename object_seq_type::iterator objects_end = d_objects.end();
		for ( ; objects_iter != objects_end; ++objects_iter)
		{
			// NOTE: This is a *reference* so that we don't increase the reference count
			// of the object.
			const ObjectInfo &object_info = *objects_iter;

			// If we are the only one referencing this object (not including client
			// weak references in the volatile objects) then we can recycle the object.
			if (object_info.object.unique())
			{
				// Get a shared reference to the recycled object so we don't lose it when
				// we delete its ObjectInfo wrapper.
				typename ObjectType::shared_ptr_type recycled_object = object_info.object;

				// Notify any volatile objects referencing this object that it's been recycled.
				// We do this by destroying the volatile token - we're the only ones who have
				// an owning (shared_ptr) reference to the token so this will notify the
				// token weak pointers in the volatile objects - in other words the
				// volatile objects will check for null.
				d_objects.erase(objects_iter);
				// NOTE: 'object_info' is now an invalid reference.

				// Create a new volatile token.
				// Add to the back of the list since that's where the most recent requests go.
				const ObjectInfo recycled_object_info(recycled_object);
				d_objects.push_back(recycled_object_info);

				return volatile_object_type(
						recycled_object_info.object,
						recycled_object_info.volatile_token);
			}
		}

		// Unable to find any objects that could be recycled.
		return boost::none;
	}
}

#endif // GPLATES_OPENGL_GLCACHE_H
