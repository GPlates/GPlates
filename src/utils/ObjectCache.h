/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_UTILS_OBJECTCACHE_H
#define GPLATES_UTILS_OBJECTCACHE_H

#include <cstddef> // For std::size_t
#include <memory> // For std::auto_ptr
#include <boost/checked_delete.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <QDebug>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/ObjectPool.h"
#include "utils/ReferenceCount.h"
#include "utils/SmartNodeLinkedList.h"


namespace GPlatesUtils
{
	/**
	 * Maintains a limited number of objects in a cache that can be recycled for
	 * future cache requests.
	 *
	 * It takes the burden of having to explicitly release objects and it does this by reusing
	 * the least-recently allocated objects for subsequent allocation requests.
	 * The flip-side, however, is the client is responsible for checking object references,
	 * before using an object, to see if the object has been recycled (stolen) by the cache.
	 *
	 * It is different to an object 'pool' in that a pool requires you to explicitly release
	 * an object before it can be reused for a future allocation. An object 'cache' can reuse
	 * an object that has not been explicitly released (although you can also explicitly release
	 * an object back to the cache by destroying the volatile object - see below).
	 *
	 * The object cache allocates 'volatile' objects which are like weak references to
	 * the real objects you're caching.
	 * The basic procedure for accessing an object (via a volatile object) is:
	 * 1) Attempt to retrieve the real object,
	 * 2) If that fails then attempt to recycle an unused object,
	 * 3) If that fails then create a new object (and add it to the cache).
	 *
	 * Each 'volatile' object can reference a real object but that real object can be stolen
	 * from underneath it, for example, if you request the real object from a different
	 * volatile object then that request could steal the object from the first volatile object -
	 * if you then request the real object from the first volatile object it will return false
	 * and you will have to recycle a real object from the cache (and if that fails you will have
	 * to create a new real object and give that to the volatile object).
	 *
	 * Note an 'unused' object means an object that it not currently held by a boost shared_ptr.
	 * If an object is held by a shared_ptr it can never be recycled (and step (1) above will always succeed).
	 * Once all shared_ptr's to that object are destroyed then that object is available for recycling.
	 * Step (2) can fail if all currently created real objects are held by client shared_ptr's.
	 *
	 * So in general the object cache works under the assumption that clients only retain
	 * shared_ptr's to the real objects temporarily otherwise the cache will continue to grow
	 * in size to fit the number of objects held by clients - in which case an object pool
	 * would have been sufficient.
	 *
	 * So the object cache is suited to situations where you have a larger number of objects
	 * but only a subset of them are used at any time - this is a smaller 'working' set of objects
	 * - instead of creating a large number of objects you only create enough for the 'working' set.
	 *
	 * An example usage:
	 *
	 *  ObjectCache<MyObjectType>::shared_ptr_type my_object_cache =
	 *      ObjectCache<MyObjectType>::create(10);
	 *  boost::shared_ptr<ObjectCache<MyObjectType>::VolatileObject> volatile_object =
	 *      my_object_cache->allocate_volatile_object();
	 *
	 *  boost::shared_ptr<MyObjectType> object = volatile_object->get_cached_object();
	 *  if (!object)
	 *  {
	 *      object = volatile_object->recycle_an_unused_object();
	 *      if (!object)
	 *      {
	 *          std::auto_ptr new_object = ...; // Create a new object.
	 *          object = volatile_object->set_cached_object(new_object);
	 *      }
	 *      // else might also want to initialise recycled object to a specific state.
	 *  }
	 *  ... // Use 'object'.
	 */
	template <typename ObjectType>
	class ObjectCache :
			public boost::enable_shared_from_this< ObjectCache<ObjectType> >,
			private boost::noncopyable
	{
	public:
		//! Typedef for this class.
		typedef ObjectCache<ObjectType> object_cache_type;

		//! A convenience typedef for a shared pointer to a @a object_cache_type.
		typedef boost::shared_ptr<object_cache_type> shared_ptr_type;
		typedef boost::shared_ptr<const object_cache_type> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a object_cache_type.
		typedef boost::weak_ptr<object_cache_type> weak_ptr_type;
		typedef boost::weak_ptr<const object_cache_type> weak_ptr_to_const_type;


		/**
		 * Creates a @a ObjectCache object.
		 *
		 * @a max_num_objects is a soft limit on the number of objects in the cache.
		 * If that limit will be exceeded then, to prevent that, the least-recently used object
		 * will be recycled if it is not being referenced.
		 * Otherwise the limit may have to be exceeded.
		 */
		static
		shared_ptr_type
		create(
				std::size_t max_num_objects)
		{
			return shared_ptr_type(new ObjectCache(max_num_objects));
		}


		// Forward declaration.
		class VolatileObject;

	private:
		using boost::enable_shared_from_this<object_cache_type>::shared_from_this;

		// Forward declaration.
		struct ObjectInfo;

		/**
		 * Typedef for a list of object infos.
		 *
		 * Using 'GPlatesUtils::SmartNodeLinkedList' instead of 'std::list' because
		 * the effect of splicing on iterators in 'std::list' is unknown (SGI say one thing,
		 * C++ standard says another) according to...
		 * http://stackoverflow.com/questions/143156/splice-on-stdlist-and-iterator-invalidation
		 */
		typedef GPlatesUtils::SmartNodeLinkedList<ObjectInfo> object_seq_type;

		/**
		 * Custom boost::shared_ptr deleter to either:
		 *  - return an object to the unused list of objects, if clients are finished with it, or
		 *  - to delete the object when the object cache is destroyed.
		 */
		class ObjectDeleter
		{
		public:
			ObjectDeleter(
					const typename object_cache_type::shared_ptr_type &object_cache_,
					typename object_seq_type::Node *cached_object_iter_) :
				d_object_cache(object_cache_),
				d_cached_object_iter(cached_object_iter_)
			{  }

			void
			operator()(
					ObjectType *cached_object)
			{
				// See if the object cache still exists (hasn't been destroyed yet).
				const typename object_cache_type::shared_ptr_type object_cache_shared_ptr =
						d_object_cache.lock();
				if (!object_cache_shared_ptr)
				{
					// The object cache no longer exists so just delete the cached object.
					// The object cache won't delete a cached object if it knows there are clients
					// out there referencing the cached object.

					// Since it was originally passed into the object cache as a 'std::auto_ptr'
					// we know it was allocated with global 'new' so we can 'delete' it.
					boost::checked_delete(cached_object);

					return;
				}

				// It's safe to deference the cached object.
				ObjectInfo &cached_object_info = d_cached_object_iter->element();

				if (cached_object_info.is_object_in_use)
				{
					// At this point, in fact just before this method was called, the only
					// reference to the cached object is 'cached_object', otherwise we
					// wouldn't be here.
					// Since 'cached_object' was originally passed into the object cache as a
					// 'std::auto_ptr' we know it was allocated with global 'new' but we
					// don't want to 'delete' it yet - instead we return it to the
					// object cache wrapped in a brand new shared_ptr - with its own
					// reference count - because it's a cached object and clients may later
					// want to access it some more.
					const boost::shared_ptr<ObjectType> return_cached_object(
							cached_object,
							ObjectDeleter(object_cache_shared_ptr, d_cached_object_iter));

					// The cached object was in use by clients and all clients have just
					// finished using it so now we can return it to its non-in-use status
					// making it available for recycling.
					object_cache_shared_ptr->return_cached_object_from_clients(
							return_cached_object, d_cached_object_iter);
				}
				else
				{
					// The cached object was *not* in use by clients.
					// The object cache has the only reference to it and now the object cache
					// is presumably being destroyed in turn destroying its cached objects.
					// So delete the cached object.
					// Since it was originally passed into the object cache as a 'std::auto_ptr'
					// we know it was allocated with global 'new' so we can 'delete' it.
					boost::checked_delete(cached_object);
				}
			}

		private:
			/**
			 * We keep only a weak reference to the object cache because we don't want to
			 * keep the object cache alive as long as there are shared_ptr references to
			 * cached objects out there - those references should be able to be stored anywhere
			 * for however long and not be linked to the lifetime of the object cache.
			 */
			typename object_cache_type::weak_ptr_type d_object_cache;

			//! Keep a reference to the cached object.
			typename object_seq_type::Node *d_cached_object_iter;
		};

		/**
		 * Contains information about the state of a cached object - whether it's in use or not.
		 */
		struct ObjectInfo
		{
			explicit
			ObjectInfo(
					const boost::shared_ptr<ObjectType> &not_in_use_object_ = boost::shared_ptr<ObjectType>()) :
				volatile_object(NULL),
				is_object_in_use(false),
				not_in_use_object(not_in_use_object_)
			{  }


			/**
			 * The volatile object referencing this object.
			 *
			 * Is Null if no volatile object is referencing this object.
			 */
			VolatileObject *volatile_object;

			/**
			 * Determines which of @a not_in_use_object and @a in_use_object is currently valid.
			 */
			bool is_object_in_use;

			/**
			 * The object is not currently is use by clients (no one has a shared_ptr)
			 * so we retain ownership using an shared_ptr until the next client comes along.
			 */
			boost::shared_ptr<ObjectType> not_in_use_object;

			/**
			 * If the object is in use by clients (they have a shared_ptr) then
			 * we keep a weak_ptr so that the shared_ptr reference count will drop to
			 * zero when the last client has finished and the shared_ptr custom deleter
			 * will switch us back over to using a shared_ptr instead of a weak_ptr.
			 */
			boost::weak_ptr<ObjectType> in_use_object;
		};

		// Make a friend so can access 'ObjectInfo' which is private.
		friend class VolatileObject;

	public:
		/**
		 * A volatile object allocated from the object cache - it is volatile because the object
		 * it references can be recycled, by the object cache, for another request.
		 */
		class VolatileObject
		{
		public:
			//! Destructor.
			~VolatileObject()
			{
				invalidate();
			}


			/**
			 * Attempt to return the cached object if it's still available, or exists yet.
			 *
			 * If the object is not available (false/NULL is returned) then it means the
			 * object was recycled by another cached object request (or an object has not yet
			 * been cached for this volatile object).
			 * In this case you need to call @a recycle_an_unused_object.
			 *
			 * The returned shared object reference (if false was not returned) will
			 * prevent 'this' volatile object from being recycled by the object cache.
			 * So it should be used temporarily and then destroyed to allow the object
			 * it references to be recycled unless you want to ensure it is not recycled
			 * (for example if you know you're going to use it again soon).
			 */
			boost::shared_ptr<ObjectType>
			get_cached_object()
			{
				if (!d_object_info_iter)
				{
					return boost::shared_ptr<ObjectType>();
				}

				return d_object_cache->return_cached_object_to_client(d_object_info_iter);
			}


			/**
			 * Attempt to recycle another object from the cache to assign to this volatile object.
			 *
			 * NOTE: This should only be called if @a get_cached_object fails.
			 *
			 * If returns false/NULL then no objects were available for recycling and
			 * @a set_cached_object must be called.
			 */
			boost::shared_ptr<ObjectType>
			recycle_an_unused_object()
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						!d_object_info_iter,
						GPLATES_ASSERTION_SOURCE);

				// Attempt to recycle an unused object.
				d_object_info_iter = d_object_cache->recycle_an_unused_object();
				if (!d_object_info_iter)
				{
					return boost::shared_ptr<ObjectType>();
				}

				connect_to_cached_object();

				return d_object_cache->return_cached_object_to_client(d_object_info_iter);
			}


			/**
			 * Sets the object to be cached - a new object must be created by the caller.
			 *
			 * NOTE: This should only be called if @a recycle_an_unused_object failed.
			 *
			 * The new object @a created_object is returned as a smart_ptr with a custom deleter
			 * that is used to notify the object cache when there are no externals references.
			 *
			 * The returned shared_ptr is guaranteed to be non-NULL.
			 */
			boost::shared_ptr<ObjectType>
			set_cached_object(
					std::auto_ptr<ObjectType> created_object)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						!d_object_info_iter,
						GPLATES_ASSERTION_SOURCE);

				// Store the newly created object in the object cache.
				d_object_info_iter = d_object_cache->add_cached_object(created_object);
				// We should definitely have a cached object now.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_object_info_iter,
						GPLATES_ASSERTION_SOURCE);

				connect_to_cached_object();

				return d_object_cache->return_cached_object_to_client(d_object_info_iter);
			}


			/**
			 * Marks this object as invalid so that @a get_cached_object will return false.
			 *
			 * If @a get_cached_object is subsequently called then another object will need to be
			 * recycled or a new object created.
			 */
			void
			invalidate()
			{
				if (d_object_info_iter)
				{
					disconnect_from_cached_object();
				}
			}

		private:
			/**
			 * NOTE: It's important for this *not* to be a shared_ptr because
			 * this introduces an ownership cycle resulting in an island.
			 *
			 * 'this' volatile object is owned by a pool in the object cache
			 * and 'this' won't get destroyed until that pool gets destroyed which won't
			 * happen until the object cache gets destroyed but it can't get destroyed
			 * if we're keeping it alive with a shared_ptr.
			 *
			 * The sublety is that 'this' volatile can get returned to the pool but
			 * not necessarily destroyed.
			 */
			object_cache_type *d_object_cache;

			/**
			 * Reference the object info for the cached object.
			 *
			 * If this is NULL then we are not referencing anything.
			 */
			typename object_seq_type::Node *d_object_info_iter;


			//! Constructor.
			explicit
			VolatileObject(
					object_cache_type *object_cache) :
				d_object_cache(object_cache),
				d_object_info_iter(NULL)
			{  }

			void
			connect_to_cached_object()
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_object_info_iter->element().volatile_object == NULL,
						GPLATES_ASSERTION_SOURCE);

				// Tell the cached object to notify us if it gets recycled.
				d_object_info_iter->element().volatile_object = this;
			}

			void
			disconnect_from_cached_object()
			{
				// If we're referencing a cached object then it should be referencing us.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_object_info_iter->element().volatile_object == this,
						GPLATES_ASSERTION_SOURCE);

				// Tell the cached object not to notify us anymore.
				d_object_info_iter->element().volatile_object = NULL;

				// Release our connection to the cached object.
				d_object_info_iter = NULL;
			}

			// Make friend so can construct.
			friend class ObjectCache<ObjectType>;
		};

		//! Typedef for a volatile object managed by this cache.
		typedef VolatileObject volatile_object_type;

		//! Typedef for a pointer to a volatile object managed by this cache.
		typedef boost::shared_ptr<volatile_object_type> volatile_object_ptr_type;


		/**
		 * Allocates a new volatile object that can be used to reference a cached object.
		 *
		 * The returned shared_ptr ensure the object cache lives as long as there are
		 * volatile objects referencing it.
		 */
		boost::shared_ptr<volatile_object_type>
		allocate_volatile_object()
		{
			// Allocate from the object pool.
			typename volatile_object_pool_type::object_ptr_type volatile_object_ptr =
					d_volatile_object_pool.add(volatile_object_type(this));

			return boost::shared_ptr<volatile_object_type>(
					volatile_object_ptr.get_ptr(),
					// Return to the object pool when no more shared_ptr's
					ReturnVolatileObjectToPoolDeleter(
							volatile_object_ptr, shared_from_this(), d_volatile_object_pool));
		}

	private:
		/**
		 * Typedef for a pool of volatile objects.
		 */
		typedef GPlatesUtils::ObjectPool<volatile_object_type> volatile_object_pool_type;

		/**
		 * Custom boost::shared_ptr deleter to return an object to its pool
		 * when all clients have finished with it.
		 */
		struct ReturnVolatileObjectToPoolDeleter
		{
			ReturnVolatileObjectToPoolDeleter(
					typename volatile_object_pool_type::object_ptr_type volatile_object_ptr_,
					const typename object_cache_type::shared_ptr_type &object_cache_,
					volatile_object_pool_type &volatile_object_pool_) :
				volatile_object_ptr(volatile_object_ptr_),
				object_cache(object_cache_),
				volatile_object_pool(&volatile_object_pool_)
			{  }

			void
			operator()(
					volatile_object_type *)
			{
				// Return the object to the pool so it can be reused.
				// We're not actually using the pointer passed in since its a raw pointer.
				// Instead we're using the 'object_ptr_type' pointer passed into the constructor.
				volatile_object_pool->release(volatile_object_ptr);
			}

			/**
			 * The actual volatile object we're managing.
			 */
			typename volatile_object_pool_type::object_ptr_type volatile_object_ptr;

			/**
			 * We keep the object cache alive while there are volatile objects referencing it.
			 *
			 * This is so requests via the volatile object don't crash because there's no object cache.
			 */
			typename object_cache_type::shared_ptr_type object_cache;

			/**
			 * The object pool to return the volatile object to.
			 */
			volatile_object_pool_type *volatile_object_pool;
		};

		/**
		 * Typedef for a pool of list nodes for the object linked list.
		 *
		 * We use boost::object_pool instead of GPlatesUtils::ObjectPool because we don't
		 * need to return the list nodes to the pool in which case boost::object_pool
		 * is slightly faster.
		 * The reason we don't return to the pool is because the list itself is our reuse list.
		 */
		typedef boost::object_pool<typename object_seq_type::Node> object_seq_node_pool_type;


		/**
		 * Used to allocate volatile objects.
		 */
		volatile_object_pool_type d_volatile_object_pool;

		/**
		 * Used to allocate linked list nodes for the object list.
		 */
		object_seq_node_pool_type d_object_seq_node_pool;

		/**
		 * List of cached objects that are currently being used (ie, clients have shared pointers
		 * to them) - these are ordered from least-recently to most-recently requested.
		 */
		object_seq_type d_objects_in_use;

		/**
		 * List of cached objects that are *not* currently being used -
		 * these are also ordered from least-recently to most-recently returned.
		 */
		object_seq_type d_objects_not_in_use;

		std::size_t d_num_objects_allocated;
		std::size_t d_max_num_objects;


		//! Constructor.
		explicit
		ObjectCache(
				std::size_t max_num_objects) :
			d_num_objects_allocated(0),
			d_max_num_objects(max_num_objects)
		{  }


		/**
		 * Returns true if we were able to recycle an existing object.
		 */
		typename object_seq_type::Node *
		recycle_an_unused_object()
		{
			// If we've not yet allocated the maximum number of objects then
			// don't attempt to recycle yet.
			if (d_num_objects_allocated < d_max_num_objects)
			{
				return NULL;
			}

			// We have already allocated the maximum number of objects so
			// attempt to recycle an existing object.

			// If the object-not-in-use list is empty then we are unable to recycle any objects.
			if (d_objects_not_in_use.empty())
			{
				return NULL;
			}

			// Get an object to recycle from the front of the not-in-use list (least-recent).
			typename object_seq_type::Node *recycled_object = d_objects_not_in_use.begin().get();

			// If there's a volatile object referencing the recycled object then let it know
			// the object has been recycled.
			volatile_object_type *&recycled_volatile_object = recycled_object->element().volatile_object;
			if (recycled_volatile_object)
			{
				recycled_volatile_object->d_object_info_iter = NULL;
				recycled_volatile_object = NULL;
			}

			return recycled_object;
		}


		/**
		 * Adds a newly created object to the cache.
		 */
		typename object_seq_type::Node *
		add_cached_object(
				std::auto_ptr<ObjectType> new_object)
		{
			// Allocate a list node from our pool.
			typename object_seq_type::Node *cached_object_iter =
					d_object_seq_node_pool.construct(ObjectInfo());

			// Transfer ownership of new object into a cached shared_ptr with a custom deleter.
			const boost::shared_ptr<ObjectType> cached_object(
					// Transfer ownership of the object itself
					new_object.release(),
					// The smart_ptr custom deleter for the object
					ObjectDeleter(
							shared_from_this(),
							cached_object_iter));

			// Set the cached object in the list node.
			// After returning from this method the list node should be the only one
			// referencing the cached object.
			cached_object_iter->element().not_in_use_object = cached_object;

			// Add to our list of cached objects that are not currently in use (by clients).
			// Add to the end of the list as that's where the most recent requests go.
			d_objects_not_in_use.append(*cached_object_iter);
			++d_num_objects_allocated;

			// Return iterator to the object just added.
			return cached_object_iter;
		}


		boost::shared_ptr<ObjectType>
		return_cached_object_to_client(
				typename object_seq_type::Node *cached_object_iter)
		{
			ObjectInfo &cached_object_info = cached_object_iter->element();

			// If some clients out there already reference the cached object then
			// just return another reference.
			if (cached_object_info.is_object_in_use)
			{
				// This should not fail.
				// If it does fail then an exception is thrown - meaning it's a bug.
				// Note that the custom deleter in the weak_ptr should also get shared
				// with the returned shared_ptr.
				return boost::shared_ptr<ObjectType>(cached_object_info.in_use_object);
			}
			// ...else we need to setup some state before returning our first reference to a client...

			// The cached object to return.
			// NOTE: This is a copy of the shared_ptr in the list - this is important because
			// we are going to reset the one in the list after making the copy.
			const boost::shared_ptr<ObjectType> cached_object = cached_object_info.not_in_use_object;

			// Move the cached object list node to the end of the 'objects in use' list -
			// this also removes it from the 'objects not in use' list.
			// All lists are ordered least-recently to most-recently.
			splice<ObjectInfo>(d_objects_in_use.end(), *cached_object_iter);

			// The list node remains valid even after the splice.
			cached_object_info.is_object_in_use = true;

			// NOTE: Reset the shared_ptr otherwise the custom deleter will never get called
			// when all clients have finished with the cached object.
			cached_object_info.not_in_use_object.reset();

			// Assign a weak_ptr to the shared_ptr so subsequent calls to
			// 'return_cached_object_to_client' can return a shared_ptr.
			cached_object_info.in_use_object = cached_object;

			return cached_object;
		}


		void
		return_cached_object_from_clients(
				const boost::shared_ptr<ObjectType> &cached_object,
				typename object_seq_type::Node *cached_object_iter)
		{
			ObjectInfo &cached_object_info = cached_object_iter->element();

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					cached_object_info.is_object_in_use,
					GPLATES_ASSERTION_SOURCE);

			// Move the list node to the end of the 'objects not in use' list.
			// All lists are ordered least-recently to most-recently.
			splice<ObjectInfo>(d_objects_not_in_use.end(), *cached_object_iter);

			// The list node remains valid even after the splice.
			cached_object_info.is_object_in_use = false;

			// NOTE: Reset the weak_ptr - it's not needed now - it probably got
			// reset anyway when the last client shared_ptr reference was destroyed
			// which just happened otherwise we wouldn't be here.
			cached_object_info.in_use_object.reset();

			// Assign a shared_ptr to the returned shared_ptr so subsequent calls to
			// 'return_cached_object_to_client' can return a shared_ptr.
			cached_object_info.not_in_use_object = cached_object;
		}
	};
}

#endif // GPLATES_UTILS_OBJECTCACHE_H
