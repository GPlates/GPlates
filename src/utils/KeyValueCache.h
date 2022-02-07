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

#ifndef GPLATES_UTILS_KEYVALUECACHE_H
#define GPLATES_UTILS_KEYVALUECACHE_H

#include <list>
#include <map>
#include <boost/function.hpp>
#include <loki/ScopeGuard.h>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/OverloadResolution.h"


namespace GPlatesUtils
{
	/**
	 * A least-recently used cache where the cached object is the value and it is inserted
	 * and retrieved from the cache using its associated key.
	 *
	 * When the cache reaches its maximum size limit the least-recently used key/value is evicted.
	 * If the value object is a shared reference (such as non_null_intrusive_ptr) then the referenced
	 * object will still exist, once evicted from the cache, provided someone else still has a shared reference.
	 *
	 * NOTE: Both 'KeyType' and 'ValueType' must be copy-constructable/copy-assignable and
	 * 'KeyType' must be less-than comparable.
	 *
	 * Also note the comment for the @a get_value method which states the returned reference to
	 * a value object can be invalidated by a subsequent call to @a get_value.
	 * So it's best to use a shared pointer for the value object such as
	 * non_null_intrusive_ptr or shared_ptr, or make a copy of the return value object.
	 *
	 * This differs from the class @a ObjectCache in the following ways:
	 * - ObjectCache does not have a key (the object stored in the cache is the equivalent of the value),
	 * - with ObjectCache the cached object will not be recycled until client(s) release strong reference to it,
	 * - with ObjectCache the client is responsible for creating a new object if none can be recycled,
	 *   and with KeyValueCache the cache itself creates a new object if it's key doesn't exist in the cache,
	 * - with ObjectCache a volatile object handle (weak reference) is returned which is like a key
	 *   but cannot be compared to other volatile object handles.
	 */
	template <typename KeyType, typename ValueType>
	class KeyValueCache
	{
	public:
		typedef KeyType key_type;
		typedef ValueType value_type;

		/**
		 * Typedef for a function to create a @a value_type object given a @a key_type.
		 */
		typedef boost::function< value_type (const key_type &) > create_value_object_function_type;


		/**
		 * Constructor accepting a function that creates a value object given a key object.
		 *
		 * @throws @a PreconditionViolationError if @a maximum_num_values_in_cache is zero.
		 */
		KeyValueCache(
				const create_value_object_function_type &create_value_object_function,
				unsigned int maximum_num_values_in_cache);


		/**
		 * Sets the maximum number of values in the cache.
		 *
		 * If the current number of values exceeds the maximum then the least-recently used
		 * values are removed and destroyed.
		 */
		void
		set_maximum_num_values_in_cache(
				unsigned int maximum_num_values_in_cache);


		/**
		 * Clears the cache by removing all cached value objects.
		 */
		void
		clear();


		/**
		 * Returns true if @a key currently exists in the cache.
		 *
		 * Note: It is not necessary to call this before calling @a get_value.
		 */
		bool
		has_key(
				const key_type &key) const;


		/**
		 * Returns the 'non-const' value object corresponding to the specified key.
		 *
		 * Creates a new value object from the specified key if the object is not cached
		 * (either because never previously requested from cache or because it was evicted).
		 *
		 * NOTE: If the least-recently used value is evicted (due to exceeding maximum number of
		 * cached value objects) then it will be evicted *after* the new value is created.
		 * This is beneficial for a few use cases where the new value depends (indirectly)
		 * on the old value (an example is where maximum cache size is one and the old value contains
		 * some shared data - when the new value is created it can access the shared data if
		 * the old value still exists at the time).
		 *
		 * WARNING: The returned reference can be invalidated by a subsequent call to @a get_value
		 * since a subsequent call might evict the value object returned by this call.
		 * For this reason it is best to use value objects that are shared pointers such as
		 * non_null_intrusive_ptr or shared_ptr, or make a copy of the return value object.
		 */
		value_type &
		get_value(
				const key_type &key);

	private:
		//! Typedef for this class.
		typedef KeyValueCache<KeyType,ValueType> this_type;

		// Forward declaration.
		struct ValueObjectInfo;

		//! Typedef for a sequence of value objects.
		typedef std::list<ValueObjectInfo> value_object_seq_type;

		/**
		 * Typedef to map a key/value pair.
		 *
		 * An iterator to @a value_object_seq_type is stored instead of the value object itself
		 * simply so we can combine the find and insert into one operation (just insert) to avoid
		 * two log(N) lookups (by inserting a dummy iterator to see if key exists and then
		 * setting iterator to newly created value object if key doesn't exist).
		 */
		typedef std::map<key_type, typename value_object_seq_type::iterator> key_value_map_type;

		//! Typedef to track least-recently to most-recently requested keys.
		typedef std::list<typename key_value_map_type::iterator> key_value_order_seq_type;

		/**
		 * Contains the value object and a reference to its entry in the least-recently used order list.
		 */
		struct ValueObjectInfo
		{
			value_type value_object;
			typename key_value_order_seq_type::iterator value_order_seq_iter;
		};


		create_value_object_function_type d_create_value_object_function;
		unsigned int d_maximum_num_value_objects_in_cache;

		value_object_seq_type d_value_objects;
		key_value_map_type d_key_value_map;
		key_value_order_seq_type d_key_value_order_seq;
		unsigned int d_num_value_objects_in_cache;


		void
		remove_least_recently_used_value();
	};


	////////////////////
	// Implementation // 
	////////////////////

	
	template <typename KeyType, typename ValueType>
	KeyValueCache<KeyType,ValueType>::KeyValueCache(
			const create_value_object_function_type &create_value_object_function,
			unsigned int maximum_num_values_in_cache) :
		d_create_value_object_function(create_value_object_function),
		d_maximum_num_value_objects_in_cache(maximum_num_values_in_cache),
		d_num_value_objects_in_cache(0)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				maximum_num_values_in_cache > 0,
				GPLATES_ASSERTION_SOURCE);
	}


	template <typename KeyType, typename ValueType>
	void
	KeyValueCache<KeyType,ValueType>::clear()
	{
		d_key_value_order_seq.clear();
		d_key_value_map.clear();
		d_value_objects.clear();
		d_num_value_objects_in_cache = 0;
	}


	template <typename KeyType, typename ValueType>
	void
	KeyValueCache<KeyType,ValueType>::set_maximum_num_values_in_cache(
			unsigned int maximum_num_values_in_cache)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				maximum_num_values_in_cache > 0,
				GPLATES_ASSERTION_SOURCE);

		d_maximum_num_value_objects_in_cache = maximum_num_values_in_cache;

		// If the current number of values exceeds the maximum then the least-recently used
		// values are removed and destroyed.
		while (d_num_value_objects_in_cache > d_maximum_num_value_objects_in_cache)
		{
			remove_least_recently_used_value();
		}
	}


	template <typename KeyType, typename ValueType>
	bool
	KeyValueCache<KeyType,ValueType>::has_key(
			const key_type &key) const
	{
		return d_key_value_map.find(key) != d_key_value_map.end();
	}

	
	template <typename KeyType, typename ValueType>
	typename KeyValueCache<KeyType,ValueType>::value_type &
	KeyValueCache<KeyType,ValueType>::get_value(
			const key_type &key)
	{
		// See if 'key' is in the cache.
		std::pair<typename key_value_map_type::iterator, bool> key_value_insert_result =
				d_key_value_map.insert(
						typename key_value_map_type::value_type(
								key,
								d_value_objects.end()/*dummy iterator*/));

		// If the key exists in the map (ie, was *not* inserted) then return the associated value object.
		if (!key_value_insert_result.second)
		{
			typename value_object_seq_type::iterator value_object_iter = key_value_insert_result.first->second;

			// The returned value object is now the most-recently requested object so move it to the
			// back of the ordering list where the most-recent object requests go - this means
			// it's furthest from the front of the ordering where the least-recently requested
			// objects get evicted when the cache is too full.
			d_key_value_order_seq.splice(
					d_key_value_order_seq.end(),
					d_key_value_order_seq,
					value_object_iter->value_order_seq_iter);

			// Update the value object's new iterator into the ordering list.
			// Note that this is not strictly necessary because std::list::splice shouldn't
			// invalidate any iterators but we'll do it anyway.
			value_object_iter->value_order_seq_iter = d_key_value_order_seq.end();
			--value_object_iter->value_order_seq_iter;

			return value_object_iter->value_object;
		}

		// The insert was successful which means the key was *not* in the cache.
		// So we need to create the value object.

		typename key_value_map_type::iterator new_key_value_map_iter = key_value_insert_result.first;

		// Undo the insert if an exception is thrown.
		// Note that the 'erase' function does not throw.
		Loki::ScopeGuard guard_key_value_insert = Loki::MakeGuard(
				GPlatesUtils::OverloadResolution::resolve<
						typename GPlatesUtils::OverloadResolution::mem_fn_types<key_value_map_type>::erase1>(
								&key_value_map_type::erase),
				d_key_value_map,
				new_key_value_map_iter);

		// Add to the back of the ordered list where most-recent requests go.
		typename key_value_order_seq_type::iterator new_key_value_order_iter =
				d_key_value_order_seq.insert(
						d_key_value_order_seq.end(),
						new_key_value_map_iter);

		// Undo the insert if an exception is thrown.
		// Note that the 'erase' function does not throw.
		Loki::ScopeGuard guard_key_value_order_insert = Loki::MakeGuard(
				GPlatesUtils::OverloadResolution::resolve<
						key_value_order_seq_type,
						void,
						GPlatesUtils::OverloadResolution::Params<> >(
								&key_value_order_seq_type::pop_back),
				d_key_value_order_seq);

		// Create a new value object (from 'key').
		const ValueObjectInfo value_object_info =
		{
			d_create_value_object_function(key),
			new_key_value_order_iter
		};

		// Add the new object to the sequence of cached value objects.
		typename value_object_seq_type::iterator new_value_object_iter =
				d_value_objects.insert(d_value_objects.end(), value_object_info);

		// Undo the insert if an exception is thrown.
		// Note that the 'erase' function does not throw.
		Loki::ScopeGuard guard_value_object_seq_insert = Loki::MakeGuard(
				GPlatesUtils::OverloadResolution::resolve<
						value_object_seq_type,
						void,
						GPlatesUtils::OverloadResolution::Params<> >(&value_object_seq_type::pop_back),
				d_value_objects);

		// Store the new value object iterator in the key/value map.
		new_key_value_map_iter->second = new_value_object_iter;

		++d_num_value_objects_in_cache;

		// We've made it this far so we can dismiss the undos.
		guard_key_value_insert.Dismiss();
		guard_key_value_order_insert.Dismiss();
		guard_value_object_seq_insert.Dismiss(); 

		// If we already have the maximum number of value objects cached then
		// release the least-recently cached one to free a slot.
		//
		// NOTE: We do this *after* adding the new value in case the new value depends (indirectly)
		// on the old value (an example is where maximum cache size is one and the old value contains
		// some shared data - when the new value is created above it can access the shared data if
		// the old value still exists).
		if (d_num_value_objects_in_cache > d_maximum_num_value_objects_in_cache)
		{
			// Since we're using std::list and std::map and we're removing the *least* recently used
			// then it should not invalidate the new value iterator which is *most* recently used.
			remove_least_recently_used_value();
		}

		return new_value_object_iter->value_object;
	}


	template <typename KeyType, typename ValueType>
	void
	KeyValueCache<KeyType,ValueType>::remove_least_recently_used_value()
	{
		// Pop off the front of the list where the least-recent requests are.
		typename key_value_map_type::iterator lru_key_value_map_iter = d_key_value_order_seq.front();
		typename value_object_seq_type::iterator lru_value_object_iter = lru_key_value_map_iter->second;

		// The key/value order iterator stored with the value object should refer to the
		// beginning of the key/order sequence since that's what we are removing.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				lru_value_object_iter->value_order_seq_iter == d_key_value_order_seq.begin(),
				GPLATES_ASSERTION_SOURCE);

		// Remove the least-recently cached value object.
		d_value_objects.erase(lru_value_object_iter);
		// Remove the key/value mapping entry.
		d_key_value_map.erase(lru_key_value_map_iter);
		// Remove from the ordering list.
		d_key_value_order_seq.pop_front();
		--d_num_value_objects_in_cache;
	}
}

#endif // GPLATES_UTILS_KEYVALUECACHE_H
