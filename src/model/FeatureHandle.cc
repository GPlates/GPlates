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
#include "TranscribeIdTypeGenerator.h"
#include "TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


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
		child_type::non_null_ptr_type new_child)
{
	ChangesetHandle changeset(model_ptr());

	const boost::intrusive_ptr<child_type> &existing_child = current_revision()->get(iter.index());
	if (existing_child)
	{
		current_revision()->set(iter.index(), new_child);

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
		if ((*iter)->get_property_name() == property_name)
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


GPlatesScribe::TranscribeResult
GPlatesModel::FeatureHandle::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<FeatureHandle> &feature)
{
	if (scribe.is_saving())
	{
		// Save feature type and ID.
		scribe.save(TRANSCRIBE_SOURCE, feature->feature_type(), "feature_type");
		scribe.save(TRANSCRIBE_SOURCE, feature->feature_id(), "feature_id");
	}
	else // loading
	{
		// Load feature type and ID.
		GPlatesScribe::LoadRef<FeatureType> feature_type_ = scribe.load<FeatureType>(TRANSCRIBE_SOURCE, "feature_type");
		GPlatesScribe::LoadRef<FeatureId> feature_id_ = scribe.load<FeatureId>(TRANSCRIBE_SOURCE, "feature_id");
		if (!feature_type_.is_valid() ||
			!feature_id_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the feature.
		feature.construct_object(feature_type_, feature_id_, revision_type::create());
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesModel::FeatureHandle::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			// Save feature type and ID.
			scribe.save(TRANSCRIBE_SOURCE, feature_type(), "feature_type");
			scribe.save(TRANSCRIBE_SOURCE, feature_id(), "feature_id");
		}
		else // loading
		{
			// Load feature type and ID.
			GPlatesScribe::LoadRef<FeatureType> feature_type_ = scribe.load<FeatureType>(TRANSCRIBE_SOURCE, "feature_type");
			GPlatesScribe::LoadRef<FeatureId> feature_id_ = scribe.load<FeatureId>(TRANSCRIBE_SOURCE, "feature_id");
			if (!feature_type_.is_valid() ||
				!feature_id_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			d_feature_type = feature_type_;
			d_feature_id = feature_id_;
		}
	}

	if (scribe.is_saving())
	{
		// Get the current list of properties.
		const std::vector<TopLevelProperty::non_null_ptr_type> properties(begin(), end());

		// Save the properties.
		scribe.save(TRANSCRIBE_SOURCE, properties, "properties");
	}
	else // loading
	{
		// Load the properties.
		std::vector<TopLevelProperty::non_null_ptr_type> properties;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, properties, "properties"))
		{
			return scribe.get_transcribe_result();
		}

		// Add the properties.
		for (auto property : properties)
		{
			add(property);
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
