/* $Id$ */

/**
 * \file 
 * Contains the implementation of the FeatureHandle class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "FeatureHandle.h"

#include "ChangesetHandle.h"
#include "FeatureCollectionHandle.h"


namespace
{
	bool
	new_child_equals_existing(
			const GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type &new_child,
			const boost::intrusive_ptr<GPlatesModel::TopLevelProperty> &existing_child)
	{
		// Assume existing_child != NULL.
		return *new_child == *existing_child;
	}
}


const GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesModel::FeatureHandle::create(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_,
		const RevisionId &revision_id_)
{
	return non_null_ptr_type(
			new FeatureHandle(
				feature_type_,
				feature_id_,
				revision_type::create(revision_id_)));
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::FeatureHandle::create(
		const WeakReference<FeatureCollectionHandle> &feature_collection,
		const FeatureType &feature_type_,
		const FeatureId &feature_id_,
		const RevisionId &revision_id_)
{
	non_null_ptr_type feature = create(
			feature_type_,
			feature_id_,
			revision_id_);
	FeatureCollectionHandle::iterator iter = feature_collection->add(feature);
	
	return (*iter)->reference();
}


const GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesModel::FeatureHandle::clone() const
{
	return non_null_ptr_type(
			new FeatureHandle(
				d_feature_type,
				FeatureId(),
				current_revision()->clone()));
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::FeatureHandle::clone(
		const WeakReference<FeatureCollectionHandle> &feature_collection) const
{
	non_null_ptr_type feature = clone();
	FeatureCollectionHandle::iterator iter = feature_collection->add(feature);

	return (*iter)->reference();
}


const GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesModel::FeatureHandle::clone(
		const property_predicate_type &clone_properties_predicate) const
{
	return non_null_ptr_type(
			new FeatureHandle(
				d_feature_type,
				FeatureId(),
				current_revision()->clone(
					clone_properties_predicate)));
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::FeatureHandle::clone(
		const WeakReference<FeatureCollectionHandle> &feature_collection,
		const property_predicate_type &clone_properties_predicate) const
{
	non_null_ptr_type feature = clone(clone_properties_predicate);
	FeatureCollectionHandle::iterator iter = feature_collection->add(feature);

	return (*iter)->reference();
}


GPlatesModel::FeatureHandle::iterator
GPlatesModel::FeatureHandle::add(
		GPlatesGlobal::PointerTraits<TopLevelProperty>::non_null_ptr_type new_child)
{
	ChangesetHandle changeset(model_ptr());

	ChangesetHandle *changeset_ptr = current_changeset_handle_ptr();
	// changeset_ptr will be NULL if we're not connected to a model.
	if (changeset_ptr)
	{
		// changeset_ptr might not point to our changeset.
		if (!changeset_ptr->has_handle(this))
		{
			current_revision()->update_revision_id();
		}
	}

	return BasicHandle<FeatureHandle>::add(new_child);
}


void
GPlatesModel::FeatureHandle::remove(
		const_iterator iter)
{
	ChangesetHandle changeset(model_ptr());

	ChangesetHandle *changeset_ptr = current_changeset_handle_ptr();
	// changeset_ptr will be NULL if we're not connected to a model.
	if (changeset_ptr)
	{
		// changeset_ptr might not point to our changeset.
		if (!changeset_ptr->has_handle(this))
		{
			current_revision()->update_revision_id();
		}
	}

	BasicHandle<FeatureHandle>::remove(iter);
}


void
GPlatesModel::FeatureHandle::set(
		iterator iter,
		child_type::non_null_ptr_to_const_type new_child)
{
	ChangesetHandle changeset(model_ptr());

	const boost::intrusive_ptr<child_type> &existing_child = current_revision()->get(iter.index());
	if (existing_child && !new_child_equals_existing(new_child, existing_child))
	{
		current_revision()->set(iter.index(), new_child->deep_clone());

		notify_listeners_of_modification(false, true);

		ChangesetHandle *changeset_ptr = current_changeset_handle_ptr();
		if (changeset_ptr)
		{
			if (!changeset_ptr->has_handle(this))
			{
				current_revision()->update_revision_id();
			}
			changeset_ptr->add_handle(this);
		}
	}
}


void
GPlatesModel::FeatureHandle::remove_properties_by_name(
		const PropertyName &property_name)
{
	for (iterator iter = begin(); iter != end(); ++iter)
	{
		if ((*iter)->property_name() == property_name)
		{
			remove(iter);
		}
	}
}


void
GPlatesModel::FeatureHandle::set_feature_type(
		const FeatureType &feature_type_)
{
	d_feature_type = feature_type_;
	notify_listeners_of_modification(true, false);
}


const GPlatesModel::RevisionId &
GPlatesModel::FeatureHandle::revision_id() const
{
	return current_revision()->revision_id();
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_,
		GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type revision_) :
	BasicHandle<FeatureHandle>(
			this,
			revision_),
	d_feature_type(feature_type_),
	d_feature_id(feature_id_),
	d_creation_time(time(NULL))
{
	d_feature_id.set_back_ref_target(*this);
}

