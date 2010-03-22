/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureStoreRootHandle.
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

#include "FeatureStoreRootHandle.h"


const GPlatesModel::FeatureStoreRootHandle::non_null_ptr_type
GPlatesModel::FeatureStoreRootHandle::create()
{
	non_null_ptr_type ptr(
			new FeatureStoreRootHandle(),
			GPlatesUtils::NullIntrusivePointerHandler());
	return ptr;
}


const GPlatesModel::FeatureStoreRootHandle::non_null_ptr_type
GPlatesModel::FeatureStoreRootHandle::clone() const
{
	non_null_ptr_type dup(
			new FeatureStoreRootHandle(*this),
			GPlatesUtils::NullIntrusivePointerHandler());
	return dup;
}


const GPlatesModel::FeatureStoreRootHandle::children_iterator
GPlatesModel::FeatureStoreRootHandle::children_begin()
{
	return children_iterator::create_begin(*this);
}


const GPlatesModel::FeatureStoreRootHandle::children_iterator
GPlatesModel::FeatureStoreRootHandle::children_end()
{
	return children_iterator::create_end(*this);
}


GPlatesModel::FeatureStoreRootHandle::~FeatureStoreRootHandle()
{
}


const GPlatesModel::FeatureStoreRootHandle::children_iterator
GPlatesModel::FeatureStoreRootHandle::append_child(
		FeatureCollectionHandle::non_null_ptr_type new_feature_collection,
		DummyTransactionHandle &transaction)
{
	container_size_type new_index =
			current_revision()->append_child(new_feature_collection, transaction);

	// FIXME:  The following line is not transaction roll-back friendly!
	new_feature_collection->set_parent_ptr(this, new_index);

	return children_iterator::create_for_index(*this, new_index);
}


void
GPlatesModel::FeatureStoreRootHandle::remove_child(
		children_iterator iter,
		DummyTransactionHandle &transaction)
{
	if (iter.collection_handle_ptr() != this) {
		// The feature collection is not contained within this feature store root.
		// Hence, nothing we can do.
		// FIXME:  Should we complain?
		return;
	}
	if ( ! iter.is_valid()) {
		// We've already established that the supplied iterator does indicate a position
		// within this feature store root.  Apparently the iterator is invalid, which means
		// that this feature store root is unloaded or deallocated.  In either case, how
		// was this member function invoked?
		// FIXME:  How was this invoked if this was an invalid iterator?
		// FIXME:  Should we complain?
		return;
	}
	// FIXME:  The following line is not transaction roll-back friendly!
	(*iter)->set_parent_ptr(NULL, INVALID_INDEX);

	current_revision()->remove_child(iter.index(), transaction);
}


const GPlatesModel::FeatureStoreRootRevision::non_null_ptr_to_const_type
GPlatesModel::FeatureStoreRootHandle::current_revision() const
{
	return d_current_revision;
}


const GPlatesModel::FeatureStoreRootRevision::non_null_ptr_type
GPlatesModel::FeatureStoreRootHandle::current_revision()
{
	return d_current_revision;
}


void
GPlatesModel::FeatureStoreRootHandle::set_current_revision(
		FeatureStoreRootRevision::non_null_ptr_type rev)
{
	d_current_revision = rev;
}


GPlatesModel::FeatureStoreRootHandle::FeatureStoreRootHandle() :
	d_current_revision(FeatureStoreRootRevision::create())
{
}


GPlatesModel::FeatureStoreRootHandle::FeatureStoreRootHandle(
		const FeatureStoreRootHandle &other) :
	WeakObserverPublisher<FeatureStoreRootHandle>(),
	GPlatesUtils::ReferenceCount<FeatureStoreRootHandle>(),
	d_current_revision(other.d_current_revision)
{
}

