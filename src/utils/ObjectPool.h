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
#include <boost/pool/object_pool.hpp>
#include <boost/scoped_ptr.hpp>

#include "IntrusiveSinglyLinkedList.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesUtils
{
	/**
	 * A memory pool to add, and reuse, objects individually.
	 *
	 * This class uses boost::object_pool in its implementation - it is very close to the speed
	 * of boost::object_pool (as determined by profiling) and has a faster release/reuse method.
	 *
	 * The main reason for using this class instead of using boost::object_pool directly is
	 * to gain an O(1) release/reuse type method instead of the O(N) provided
	 * by 'boost::object_pool::destroy()'.
	 * The compromise is our 'reuse()' method does not immediately destroy the object being reused.
	 *
	 * If you just need to allocate objects and can destroy them all at the same time then use
	 * boost::object_pool instead (its just boost::object_pool::destroy that you need to be wary of).
	 *
	 * Like boost::object_pool, objects added to this object pool are destroyed when an
	 * object pool instance is destroyed (or cleared). They can also be reused in which case
	 * the time at which they are actually reused (and hence destroyed) is undetermined.
	 *
	 * The objects, of type 'ObjectType', must be copy-constructable and assignable.
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
	 * hence retains its superior speed advantage.
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
	public:
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
		 * The returned pointer, and object it points to, will remain valid as long as this object
		 * pool is alive, or until @a clear is called, at which point the copy will be destroyed.
		 *
		 * A small subtlety is the @a reuse method does not actually destroy the object
		 * immediately - the actual destroy (actually done by assignment operator) can happen
		 * during a later call to @a add (the time at which this happens is effectively undetermined).
		 */
		ObjectType *
		add(
				const ObjectType &object)
		{
			if (d_object_free_list.empty())
			{
				// Allocate memory and copy construct 'object' into the new memory.
				ObjectType *const new_object = d_object_pool->construct(object);
				if (new_object == NULL)
				{
					// Memory allocation failed before object got a chance to be copy-constructed.
					throw std::bad_alloc();
				}

				return new_object;
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
			// the 'reuse' method.
			ObjectType *const free_list_object = free_list_node.object;

			// Assign the object added by the caller to the free list object.
			// This will effectively destroy the existing free list object and
			// copy the added object in place of it.
			*free_list_object = object;

			return free_list_object;
		}


		/**
		 * Makes the specified object available for reuse by a subsequent call to @a add *but*
		 * does *not* immediately destroy the object or release its memory.
		 *
		 * After calling this method you should not refer to @a object again - you can think
		 * of it as scheduled for destruction (but that time is unknown).
		 *
		 * WARNING: This will not destroy the specified object or release its memory.
		 * If the object is later used internally for a call to @a add then it will be assigned
		 * to by a new object (effectively destroying it then).
		 *
		 * If you want the object to be destroyed immediately then you should use
		 * boost::object_pool instead of this class but note that boost::object_pool::destroy()
		 * is O(N) and not O(1) - in other words destroying N objects using
		 * boost::object_pool::destroy() is O(N ^ 2) and not O(N).
		 *
		 * In fact this is the main reason for this class:
		 * - To wrap boost::object_pool to use its speed, and
		 * - To provide an O(1) release/reuse type method instead of O(N).
		 * The compromise is 'reuse' does not destroy immediately.
		 */
		void
		reuse(
				ObjectType *object)
		{
			// If there are no free list nodes then allocate a new list node from the pool.
			if (d_free_list_node_free_list.empty())
			{
				// The list node contains the object pointer that we're storing away so
				// it can be reused later.
				FreeListNode *const new_free_list_node = d_free_list_node_pool->construct(object);
				if (new_free_list_node == NULL)
				{
					// Since this method could be called from a destructor, and we don't want exceptions
					// to leave destructors, we will not throw std::bad_alloc if the list node memory
					// allocation fails - instead we will just silently ignore the reuse request.
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
			free_list_node.object = object;
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
		 * A node in the linked list of free objects that stores a pointer to
		 * an object that is available for reuse.
		 */
		struct FreeListNode :
				public IntrusiveSinglyLinkedList<FreeListNode>::Node
		{
			FreeListNode(
					ObjectType *object_) :
				object(object_)
			{  }

			ObjectType *object;
		};

		//! Typedef for a linked list of free objects.
		typedef IntrusiveSinglyLinkedList<FreeListNode> free_list_type;

		//! Typedef for a pool of linked list nodes.
		typedef boost::object_pool<FreeListNode> free_list_node_pool_type;

		//! Typedef for a pool of objects.
		typedef boost::object_pool<ObjectType> object_pool_type;

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
