/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureCollectionHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "FeatureCollectionHandle.h"
#include "FeatureStoreRootHandle.h"
#include "DummyTransactionHandle.h"


const GPlatesModel::FeatureCollectionHandle::children_const_iterator
GPlatesModel::FeatureCollectionHandle::get_const_iterator(
		const children_iterator &iter)
{
	if (iter.collection_handle_ptr() == NULL)
	{
		return children_const_iterator();
	}
	return children_const_iterator::create_for_index(
			*(iter.collection_handle_ptr()), iter.index());
}


const GPlatesModel::FeatureCollectionHandle::const_weak_ref
GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
		const weak_ref &ref)
{
	if (ref.handle_ptr() == NULL)
	{
		return const_weak_ref();
	}
	return const_weak_ref(*(ref.handle_ptr()));
}


const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
GPlatesModel::FeatureCollectionHandle::create()
{
	non_null_ptr_type ptr(
			new FeatureCollectionHandle(),
			GPlatesUtils::NullIntrusivePointerHandler());
	return ptr;
}


GPlatesModel::FeatureCollectionHandle::~FeatureCollectionHandle()
{
}


const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
GPlatesModel::FeatureCollectionHandle::clone() const
{
	non_null_ptr_type dup(
			new FeatureCollectionHandle(*this),
			GPlatesUtils::NullIntrusivePointerHandler());
	return dup;
}


const GPlatesModel::FeatureCollectionHandle::const_weak_ref
GPlatesModel::FeatureCollectionHandle::reference() const
{
	const_weak_ref ref(*this);
	return ref;
}


void
GPlatesModel::FeatureCollectionHandle::unload()
{
	if (d_container_handle == NULL || d_index_in_container == INVALID_INDEX) {
		// Apparently this feature collection has already been unloaded.  So how was this
		// 'unload' member function invoked again?  All weak-refs, etc., should be invalid!
		// FIXME:  How was this 'unload' member function invoked?
		// FIXME:  Should we complain?
		// FIXME:  We should assert that either (handle != NULL && index != INVALID_INDEX)
		// or (handle == NULL && index == INVALID_INDEX).
		return;
	}
	// OK, now we need a collections-iterator which indicates this feature collection within
	// its feature store root.
	FeatureStoreRootHandle::children_iterator iter =
			FeatureStoreRootHandle::children_iterator::create_for_index(
					*d_container_handle, d_index_in_container);

	GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
	d_container_handle->remove_child(iter, transaction);
	transaction.commit();

	// FIXME:  When this operation no longer results in the FeatureCollectionHandle being
	// deallocated when it is "removed" (which in turn results in the FeatureCollectionRevision
	// being deallocated, which in turn results in all the FeatureHandles being deallocated,
	// which in turn unsubscribes all the FeatureHandle weak-refs), we need to ensure that
	// every FeatureHandle is told that it is in a "deleted" state, and ensure further that
	// this message is passed on to all the FeatureHandle weak-refs.
}


const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesModel::FeatureCollectionHandle::reference()
{
	weak_ref ref(*this);
	return ref;
}


const GPlatesModel::FeatureCollectionHandle::children_const_iterator
GPlatesModel::FeatureCollectionHandle::children_begin() const
{
	return children_const_iterator::create_begin(*this);
}


const GPlatesModel::FeatureCollectionHandle::children_iterator
GPlatesModel::FeatureCollectionHandle::children_begin()
{
	return children_iterator::create_begin(*this);
}


const GPlatesModel::FeatureCollectionHandle::children_const_iterator
GPlatesModel::FeatureCollectionHandle::children_end() const
{
	return children_const_iterator::create_end(*this);
}


const GPlatesModel::FeatureCollectionHandle::children_iterator
GPlatesModel::FeatureCollectionHandle::children_end()
{
	return children_iterator::create_end(*this);
}


void
GPlatesModel::FeatureCollectionHandle::set_parent_ptr(
		FeatureStoreRootHandle *new_handle,
		container_size_type new_index)
{
	// FIXME:  We should assert that either (handle != NULL && index != INVALID_INDEX) or
	// (handle == NULL && index == INVALID_INDEX).
	d_container_handle = new_handle;
	d_index_in_container = new_index;
}


const GPlatesModel::FeatureCollectionHandle::children_iterator
GPlatesModel::FeatureCollectionHandle::append_child(
		FeatureHandle::non_null_ptr_type new_feature,
		DummyTransactionHandle &transaction)
{
	container_size_type new_index =
			current_revision()->append_child(new_feature, transaction);
	return children_iterator::create_for_index(*this, new_index);
}


void
GPlatesModel::FeatureCollectionHandle::remove_child(
		children_const_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_child(iter.index(), transaction);
}


void
GPlatesModel::FeatureCollectionHandle::remove_child(
		children_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_child(iter.index(), transaction);
}

bool
GPlatesModel::feature_collection_contains_feature(
		GPlatesModel::FeatureCollectionHandle::weak_ref collection_ref,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	if ( ! collection_ref.is_valid()) 
	{
		return false;
	}
	if ( ! feature_ref.is_valid()) 
	{
		return false;
	}

	GPlatesModel::FeatureCollectionHandle::children_iterator
		it = collection_ref->children_begin();
	GPlatesModel::FeatureCollectionHandle::children_iterator
		it_end = collection_ref->children_end();

	for (; it != it_end; ++it) 
	{
		if (it.is_valid()) 
		{
			GPlatesModel::FeatureHandle &it_handle = **it;
			if (feature_ref.handle_ptr() == &it_handle) 
			{
				return true;
			}
		}
	}
	return false;
}



bool
GPlatesModel::FeatureCollectionHandle::contains_unsaved_changes() const
{
	return d_contains_unsaved_changes;
}


void
GPlatesModel::FeatureCollectionHandle::set_contains_unsaved_changes(
		bool new_status) const
{
	d_contains_unsaved_changes = new_status;
}


const GPlatesModel::FeatureCollectionRevision::non_null_ptr_to_const_type
GPlatesModel::FeatureCollectionHandle::current_revision() const
{
	return d_current_revision;
}


const GPlatesModel::FeatureCollectionRevision::non_null_ptr_type
GPlatesModel::FeatureCollectionHandle::current_revision()
{
	return d_current_revision;
}


void
GPlatesModel::FeatureCollectionHandle::set_current_revision(
		FeatureCollectionRevision::non_null_ptr_type rev)
{
	d_current_revision = rev;
}


GPlatesModel::FeatureStoreRootHandle *
GPlatesModel::FeatureCollectionHandle::parent_ptr() const
{
	return d_container_handle;
}


GPlatesModel::container_size_type
GPlatesModel::FeatureCollectionHandle::index_in_container() const
{
	return d_index_in_container;
}


GPlatesModel::FeatureCollectionHandle::tags_collection_type &
GPlatesModel::FeatureCollectionHandle::tags()
{
	return d_tags;
}


const GPlatesModel::FeatureCollectionHandle::tags_collection_type &
GPlatesModel::FeatureCollectionHandle::tags() const
{
	return d_tags;
}


GPlatesModel::FeatureCollectionHandle::FeatureCollectionHandle() :
	d_current_revision(FeatureCollectionRevision::create()),
	d_container_handle(NULL),
	d_index_in_container(INVALID_INDEX),
	d_contains_unsaved_changes(true)
{
}


GPlatesModel::FeatureCollectionHandle::FeatureCollectionHandle(
		const FeatureCollectionHandle &other) :
	WeakObserverPublisher<FeatureCollectionHandle>(),
	GPlatesUtils::ReferenceCount<FeatureCollectionHandle>(),
	d_current_revision(other.d_current_revision),
	d_container_handle(NULL),
	d_index_in_container(INVALID_INDEX),
	d_contains_unsaved_changes(true)
{
}

