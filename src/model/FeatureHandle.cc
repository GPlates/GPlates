/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureHandle.
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

#include "FeatureHandle.h"
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"
#include "WeakObserverVisitor.h"
#include "FeatureHandleWeakRefBackInserter.h"


GPlatesModel::FeatureHandle::~FeatureHandle()
{
	weak_observer_unsubscribe_forward(d_first_const_weak_observer);
	weak_observer_unsubscribe_forward(d_first_weak_observer);
}


const GPlatesModel::RevisionId &
GPlatesModel::FeatureHandle::revision_id() const
{
	return current_revision()->revision_id();
}


const GPlatesModel::FeatureHandle::properties_iterator
GPlatesModel::FeatureHandle::append_top_level_property(
		TopLevelProperty::non_null_ptr_type new_top_level_property,
		DummyTransactionHandle &transaction)
{
	container_size_type new_index =
			current_revision()->append_top_level_property(new_top_level_property, transaction);
	return properties_iterator::create_for_index(*this, new_index);
}


void
GPlatesModel::FeatureHandle::remove_top_level_property(
		properties_const_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_top_level_property(iter.index(), transaction);
}


void
GPlatesModel::FeatureHandle::remove_top_level_property(
		properties_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_top_level_property(iter.index(), transaction);
}


void
GPlatesModel::FeatureHandle::apply_weak_observer_visitor(
		WeakObserverVisitor<FeatureHandle> &visitor)
{
	weak_observer_type *curr = first_weak_observer();
	while (curr != NULL) {
		curr->accept_weak_observer_visitor(visitor);
		curr = curr->next_link_ptr();
	}
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_):
	d_current_revision(FeatureRevision::create()),
	d_feature_type(feature_type_),
	d_feature_id(feature_id_),
	d_first_const_weak_observer(NULL),
	d_first_weak_observer(NULL),
	d_last_const_weak_observer(NULL),
	d_last_weak_observer(NULL),
	d_feature_collection_handle_ptr(NULL)
{
	d_feature_id.set_back_ref_target(*this);
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_,
		const RevisionId &revision_id_):
	d_current_revision(FeatureRevision::create(revision_id_)),
	d_feature_type(feature_type_),
	d_feature_id(feature_id_),
	d_first_const_weak_observer(NULL),
	d_first_weak_observer(NULL),
	d_last_const_weak_observer(NULL),
	d_last_weak_observer(NULL),
	d_feature_collection_handle_ptr(NULL)
{
	d_feature_id.set_back_ref_target(*this);

#if 0
	// Verify that the back-ref logic is working properly.
	std::vector<FeatureHandle::weak_ref> back_ref_targets;
	d_feature_id.find_back_ref_targets(append_as_weak_refs(back_ref_targets));
	std::cout << "Num back-ref targets found: " << back_ref_targets.size() << std::endl;
#endif
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureHandle &other):
	GPlatesUtils::ReferenceCount<FeatureHandle>(),
	d_current_revision(other.d_current_revision),
	d_feature_type(other.d_feature_type),
	d_feature_id(FeatureId()),
	d_first_const_weak_observer(NULL),
	d_first_weak_observer(NULL),
	d_last_const_weak_observer(NULL),
	d_last_weak_observer(NULL),
	d_feature_collection_handle_ptr(NULL)
{
	d_feature_id.set_back_ref_target(*this);
}
