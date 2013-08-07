/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class TopLevelProperty.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <typeinfo>

#include "TopLevelProperty.h"

#include "FeatureHandle.h"
#include "ModelTransaction.h"


void
GPlatesModel::TopLevelProperty::set_xml_attributes(
		const xml_attributes_type &xml_attributes)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().xml_attributes = xml_attributes;
	revision_handler.handle_revision_modification();
}


bool
GPlatesModel::TopLevelProperty::operator==(
		const TopLevelProperty &other) const
{
	// Both objects must have the same type before testing for equality.
	// This also means derived classes need no type-checking.
	if (typeid(*this) != typeid(other))
	{
		return false;
	}

	// Compare the derived type objects.
	return equality(other);
}


bool
GPlatesModel::TopLevelProperty::equality(
		const TopLevelProperty &other) const
{
	return d_property_name == other.d_property_name &&
			// Compare the mutable data that is contained in the revisions...
			d_current_revision->equality(*other.d_current_revision);
}


boost::optional<GPlatesModel::Model &>
GPlatesModel::TopLevelProperty::get_model()
{
	if (!d_current_parent)
	{
		return boost::none;
	}

	Model *model = d_current_parent->model_ptr();
	if (!model)
	{
		return boost::none;
	}

	return *model;
}


boost::optional<const GPlatesModel::Model &>
GPlatesModel::TopLevelProperty::get_model() const
{
	if (!d_current_parent)
	{
		return boost::none;
	}

	const Model *model = d_current_parent->model_ptr();
	if (!model)
	{
		return boost::none;
	}

	return *model;
}


void
GPlatesModel::TopLevelProperty::bubble_up_modification(
		const PropertyValue::RevisionedReference &property_value_revisioned_reference,
		ModelTransaction &transaction)
{
	// The current top level property revision is immutable so we create
	// a new revision by cloning it for bubble up...
	Revision::non_null_ptr_type top_level_property_revision = 
			d_current_revision->clone_for_bubble_up_modification();

	// Get the derived top level property revision class to set the property value revision reference.
	top_level_property_revision->reference_bubbled_up_property_value_revision(
			property_value_revisioned_reference);

	// Store the new revision in the model transaction.
	RevisionedReference top_level_property_revisioned_reference(this, top_level_property_revision);
	transaction.set_top_level_property_revision(top_level_property_revisioned_reference);

	// If we have a parent feature then bubble up the modification towards the root (feature store).
	if (d_current_parent)
	{
#if 0 // TODO: Add this back once implemented in BasicHandle.
		d_current_parent->bubble_up_modification(top_level_property_revisioned_reference, transaction);
#endif
	}
}


GPlatesModel::TopLevelProperty::MutableRevisionHandler::MutableRevisionHandler(
		const TopLevelProperty::non_null_ptr_type &top_level_property) :
	d_model(top_level_property->get_model()),
	d_top_level_property(top_level_property),
	d_mutable_revision(
		// The current top level property revision is immutable so we create a new revision by cloning it for bubble up...
		d_top_level_property->d_current_revision->clone_for_bubble_up_modification())
{
}


void
GPlatesModel::TopLevelProperty::MutableRevisionHandler::handle_revision_modification()
{
	// Create a model transaction that will switch the current revision to the new one.
	ModelTransaction transaction;

	RevisionedReference revision(d_top_level_property, d_mutable_revision);
	transaction.set_top_level_property_revision(revision);

	// If the top level property has a parent then bubble up the modification towards the root (feature store).
	if (d_top_level_property->d_current_parent)
	{
#if 0 // TODO: Add this back once implemented in BasicHandle.
		d_top_level_property->d_current_parent->bubble_up_modification(revision, transaction);
#endif
	}

	// If the bubble-up reaches the top of the model hierarchy (ie, connected all the way up to the model)
	// then the model will store the new version in the undo/redo queue.
	transaction.commit();

	// Emit the model events if either there's no model (ie, not attached to model), or
	// we are attached to the model but the model notification guard is currently active
	// (in which case the events will be re-determined and then emitted when the notification
	// guard is released).
	if (!d_model ||
		!d_model->has_notification_guard())
	{
		// TODO: Emit model events.
	}
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const TopLevelProperty &top_level_prop)
{
	return top_level_prop.print_to(os);
}

