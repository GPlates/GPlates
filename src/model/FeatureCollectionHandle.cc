/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureCollectionHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <iostream>

#include "FeatureCollectionHandle.h"
#include "FeatureStoreRootHandle.h"
#include "DummyTransactionHandle.h"


GPlatesModel::FeatureCollectionHandle::~FeatureCollectionHandle()
{
	weak_observer_unsubscribe_forward(d_first_const_weak_observer);
	weak_observer_unsubscribe_forward(d_first_weak_observer);
	std::cout << "Destroying FeatureCollectionHandle" << std::endl;
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
	FeatureStoreRootHandle::collections_iterator iter =
			FeatureStoreRootHandle::collections_iterator::create_for_index(
					*d_container_handle, d_index_in_container);

	GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
	d_container_handle->remove_feature_collection(iter, transaction);
	transaction.commit();

	// FIXME:  When this operation no longer results in the FeatureCollectionHandle being
	// deallocated when it is "removed" (which in turn results in the FeatureCollectionRevision
	// being deallocated, which in turn results in all the FeatureHandles being deallocated,
	// which in turn unsubscribes all the FeatureHandle weak-refs), we need to ensure that
	// every FeatureHandle is told that it is in a "deleted" state, and ensure further that
	// this message is passed on to all the FeatureHandle weak-refs.
}


void
GPlatesModel::FeatureCollectionHandle::set_container(
		FeatureStoreRootHandle *new_handle,
		container_size_type new_index)
{
	// FIXME:  We should assert that either (handle != NULL && index != INVALID_INDEX) or
	// (handle == NULL && index == INVALID_INDEX).
	d_container_handle = new_handle;
	d_index_in_container = new_index;
}


const GPlatesModel::FeatureCollectionHandle::features_iterator
GPlatesModel::FeatureCollectionHandle::append_feature(
		FeatureHandle::non_null_ptr_type new_feature,
		DummyTransactionHandle &transaction)
{
	container_size_type new_index =
			current_revision()->append_feature(new_feature, transaction);
	return features_iterator::create_for_index(*this, new_index);
}


void
GPlatesModel::FeatureCollectionHandle::remove_feature(
		features_const_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_feature(iter.index(), transaction);
}


void
GPlatesModel::FeatureCollectionHandle::remove_feature(
		features_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_feature(iter.index(), transaction);
}
