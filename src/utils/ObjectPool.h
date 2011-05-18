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

#ifndef GPLATES_UTILS_OBJECTPOOL_H
#define GPLATES_UTILS_OBJECTPOOL_H

#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "IntrusiveSinglyLinkedList.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/SafeBool.h"


namespace GPlatesUtils
{
	/**
	 * A memory pool to add, and release, objects individually.
	 *
	 * The pool objects, of type 'ObjectType', must be copy-constructable and copy-assignable.
	 *
	 * This class uses boost::object_pool in its implementation - it is very close to the speed
	 * of boost::object_pool (as determined by profiling) and has a faster release method.
	 * And it uses up to an extra 8 bytes per object above boost::object_pool (4 bytes extra
	 * per object if don't call @a release).
	 *
	 * The main reason for using this class instead of using boost::object_pool directly is
	 * to gain an O(1) release type method instead of the O(N) provided
	 * by 'boost::object_pool::destroy()'.
	 *
	 * If you just need to allocate objects and can destroy them all at the same time then use
	 * boost::object_pool instead (it's just boost::object_pool::destroy that you need to be wary of).
	 *
	 * And like boost::object_pool, objects added to this object pool are destroyed when an
	 * object pool instance is destroyed (or cleared).
	 *
	 * Previously this class was implemented without using boost::object_pool but has now
	 * switched to using boost::object_pool because of the following performance profile...
	 * Profiling done on a 3.07GHz system.
	 *
	 * boost::object_pool
	 * ------------------
	 *   20 CPU cycles per object allocation using 'construct()' method.
	 *   7  CPU cycles per object deallocation (all deallocation done in object pool destructor).
	 *
	 * Original GPlatesUtils::ObjectPool (that did *not* wrap boost::object_pool)
	 * --------------------------------------------------------------------------
	 *   60 CPU cycles per object allocation using 'add()' method.
	 *   2  CPU cycles per object deallocation (all deallocation done in object pool destructor).
	 *
	 * So boost::object_pool was roughly three times faster during allocation and three times
	 * slower during deallocation. Overall this made boost::object_pool faster.
	 * Because of this, GPlatesUtils::ObjectPool is now a wrapper around boost::object_pool and
	 * hence retains boost::object_pool's superior speed advantage.
	 *
	 * Implementation detail: The slower deallocation is because boost::object_pool orders its
	 * free list by memory address in order to detect, in the object pool destructor, if any
	 * objects have already been deallocated by the client.
	 *
	 * Also deallocating objects *individually* in boost::object_pool, using
	 * 'boost::object_pool::destroy()', is O(N) and not O(1) - in other words boost::object_pool
	 * is meant to be added to but not removed from.
	 * This is the main reason for this class.
	 */
	template <typename ObjectType>
	class ObjectPool :
			private boost::noncopyable
	{
	private:
		/**
		 * Wraps an 'ObjectType' in a boost::optional purely to give us the ability
		 * to destroy the object when it gets returned to the pool.
		 *
		 * This uses an extra 4 bytes per object (for the boost optional boolean variable).
		 *
		 * NOTE: We can't simply call boost::object_pool::destroy on the returned object
		 * because that is O(N) in the number of objects in the pool and we want O(1).
		 */
		struct ObjectWrapper
		{
			explicit
			ObjectWrapper(
					const ObjectType &object_) :
				object(object_)
			{  }

			void
			assign_object(
					const ObjectType &object_)
			{
				object = object_;
			}

			void
			destroy_object()
			{
				object = boost::none;
			}

			boost::optional<ObjectType> object;
		};

	public:
		/**
		 * Pointer to an object obtained from the pool.
		 *
		 * It has the same memory usage as a raw pointer.
		 *
		 * Usage:
		 *   ObjectPtr ptr = pool.add(object); // 'ptr' points to a copy of 'object'.
		 *   ptr->do_something();
		 *   pool.release(ptr); // Destroys the object pointed to by 'ptr'.
		 */
		class ObjectPtr :
				public GPlatesUtils::SafeBool<ObjectPtr>
		{
		public:
			/**
			 * Use "if (ptr)" or "if (!ptr)" to effect this boolean test.
			 *
			 * Used by GPlatesUtils::SafeBool<>
			 */
			bool
			boolean_test() const
			{
				return d_object_wrapper != NULL;
			}

			const ObjectType &
			operator*() const
			{
				return d_object_wrapper->object.get();
			}

			ObjectType &
			operator*()
			{
				return d_object_wrapper->object.get();
			}


			const ObjectType *
			operator->() const
			{
				return d_object_wrapper->object.get_ptr();
			}

			ObjectType *
			operator->()
			{
				return d_object_wrapper->object.get_ptr();
			}


			const ObjectType &
			get() const
			{
				return d_object_wrapper->object.get();
			}

			ObjectType &
			get()
			{
				return d_object_wrapper->object.get();
			}


			const ObjectType *
			get_ptr() const
			{
				return d_object_wrapper ? d_object_wrapper->object.get_ptr() : NULL;
			}

			ObjectType *
			get_ptr()
			{
				return d_object_wrapper ? d_object_wrapper->object.get_ptr() : NULL;
			}


			ObjectPtr() :
				d_object_wrapper(NULL)
			{  }

		private:
			ObjectWrapper *d_object_wrapper;

			explicit
			ObjectPtr(
					ObjectWrapper *object_wrapper) :
				d_object_wrapper(object_wrapper)
			{  }

			friend class ObjectPool<ObjectType>;
		};

		//! Typedef for a non-owning pointer to an object.
		typedef ObjectPtr object_ptr_type;

		//! Typedef for a shared owning pointer to an object - see @a add_with_auto_release.
		typedef boost::shared_ptr<ObjectType> shared_object_ptr_type;


		/**
		 * Default constructor.
		 */
		ObjectPool() :
			d_object_pool(new object_pool_type()),
			d_free_list_node_pool(new free_list_node_pool_type())
		{  }


		/**
		 * Copies @a object to a fixed memory address and returns a pointer to the copy.
		 *
		 * The returned pointer will not release the object on destruction (it's not an owning pointer).
		 * It's only purpose to hide an implementation detail to do with 'ObjectType' destruction.
		 *
		 * The returned object will remain valid as long as this pool is alive, or until @a clear
		 * is called, at which point the object will be destroyed and the pointer will be left dangling.
		 */
		object_ptr_type
		add(
				const ObjectType &object)
		{
			if (d_object_free_list.empty())
			{
				// Allocate memory and copy construct 'object' into the new memory.
				ObjectWrapper *const new_object = d_object_pool->construct(object);
				if (new_object == NULL)
				{
					// Memory allocation failed before object got a chance to be copy-constructed.
					throw std::bad_alloc();
				}

				return object_ptr_type(new_object);
			}

			// Pop a list node off the list of free objects.
			FreeListNode &free_list_node = d_object_free_list.front();
			d_object_free_list.pop_front();
			// Keep track of the list node object so we can reuse it later.
			// Note this is done *after* removing it from the other list otherwise
			// the linked lists will get corrupted because both lists are using the
			// same 'next' pointers (inside the 'FreeListNode' object).
			d_free_list_node_free_list.push_front(&free_list_node);

			// Get the object on the free list.
			// Note that this object is still alive and has not been destroyed by
			// the 'release' method.
			ObjectWrapper *const free_list_object = free_list_node.object_wrapper;

			// Assign the object added by the caller to the free list object.
			free_list_object->object = object;

			return object_ptr_type(free_list_object);
		}


		/**
		 * A convenience wrapper around @a add and @a release.
		 *
		 * This method is the equivalent to @a add and when all returned shared_ptr's are
		 * destroyed then @a release will be called.
		 *
		 * NOTE: You must ensure that this object pool lives longer than any returned shared_ptr's
		 * otherwise a crash is likely to occur.
		 */
		shared_object_ptr_type
		add_with_auto_release(
				const ObjectType &object)
		{
			// Allocate from the object pool.
			object_ptr_type object_ptr = add(object);

			return boost::shared_ptr<ObjectType>(
					object_ptr.get_ptr(),
					// Return to the object pool when no more shared_ptr's
					ReturnObjectToPoolDeleter(object_ptr, this));
		}


		/**
		 * Makes the specified object available for reuse by a subsequent call to @a add.
		 *
		 * After calling this method you should not refer to @a object again.
		 */
		void
		release(
				object_ptr_type object_ptr)
		{
			// Destroy the embedded object first.
			object_ptr.d_object_wrapper->object = boost::none;

			// If there are no free list nodes then allocate a new list node from the pool.
			if (d_free_list_node_free_list.empty())
			{
				// The list node contains the object pointer that we're storing away so
				// it can be reused later.
				FreeListNode *const new_free_list_node =
						d_free_list_node_pool->construct(object_ptr.d_object_wrapper);
				if (new_free_list_node == NULL)
				{
					// Since this method could be called from a destructor, and we don't want exceptions
					// to leave destructors, we will not throw std::bad_alloc if the list node memory
					// allocation fails - instead we will just silently ignore the release request.
					return;
				}

				// Add the list node to the free list and return.
				d_object_free_list.push_front(new_free_list_node);
				return;
			}

			// There are some free list nodes available so we can reuse them instead
			// of allocating new ones from the pool.
			FreeListNode &free_list_node = d_free_list_node_free_list.front();
			d_free_list_node_free_list.pop_front();

			// Store the object pointer in the list node and add the list node to the free list.
			free_list_node.object_wrapper = object_ptr.d_object_wrapper;
			d_object_free_list.push_front(&free_list_node);
		}


		/**
		 * Destroys all objects and releases all memory allocated.
		 *
		 * Note that the destructor effectively does the same thing so this call is only
		 * necessary if you wish to add more objects after the @a clear.
		 */
		void
		clear()
		{
			// The only way to destroy all objects and release memory is to destroy
			// the boost::object_pool objects.
			d_object_free_list.clear();
			d_free_list_node_free_list.clear();
			d_object_pool.reset();
			d_free_list_node_pool.reset();

			// Allocate new boost::object_pool objects.
			d_object_pool.reset(new object_pool_type());
			d_free_list_node_pool.reset(new free_list_node_pool_type());
		}

	private:
		/**
		 * Custom boost::shared_ptr deleter to return an object to its pool
		 * when all clients have finished with it.
		 */
		struct ReturnObjectToPoolDeleter
		{
			ReturnObjectToPoolDeleter(
					object_ptr_type object_ptr_,
					ObjectPool<ObjectType> *object_pool_) :
				object_ptr(object_ptr_),
				object_pool(object_pool_)
			{  }

			void
			operator()(
					ObjectType *)
			{
				// Return the object to the pool so it can be reused.
				// We're not actually using the pointer passed in since its a raw pointer.
				// Instead we're using the 'object_ptr_type' pointer passed into the constructor.
				object_pool->release(object_ptr);
			}

			object_ptr_type object_ptr;
			ObjectPool<ObjectType> *object_pool;
		};


		/**
		 * A node in the linked list of free objects that stores a pointer to
		 * an object that is available for reuse.
		 */
		struct FreeListNode :
				public IntrusiveSinglyLinkedList<FreeListNode>::Node
		{
			explicit
			FreeListNode(
					ObjectWrapper *object_wrapper_) :
				object_wrapper(object_wrapper_)
			{  }

			ObjectWrapper *object_wrapper;
		};


		//! Typedef for a linked list of free objects.
		typedef IntrusiveSinglyLinkedList<FreeListNode> free_list_type;

		//! Typedef for a pool of linked list nodes.
		typedef boost::object_pool<FreeListNode> free_list_node_pool_type;

		//! Typedef for a pool of objects.
		typedef boost::object_pool<ObjectWrapper> object_pool_type;


		//! List of objects available for reuse.
		free_list_type d_object_free_list;

		//! List of linked list nodes available for reuse in the linked list of free objects.
		free_list_type d_free_list_node_free_list;

		//! Pool of objects.
		boost::scoped_ptr<object_pool_type> d_object_pool;

		//! Pool of linked list nodes.
		boost::scoped_ptr<free_list_node_pool_type> d_free_list_node_pool;
	};
}

#endif // GPLATES_UTILS_OBJECTPOOL_H
