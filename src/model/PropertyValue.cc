/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class PropertyValue.
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

#include "PropertyValue.h"

#include "Model.h"
#include "ModelTransaction.h"
#include "TopLevelProperty.h"
#include "WeakReferenceCallback.h"


bool
GPlatesModel::PropertyValue::operator==(
		const PropertyValue &other) const
{
	// Both objects must have the same type before testing for equality.
	// This also means derived classes need no type-checking.
	if (typeid(*this) != typeid(other))
	{
		return false;
	}

	// Compare the derived type objects.
	// Since most (all) of the value data is contained in the revisions, which is handled by the
	// base PropertyValue class, the derived property value classes don't typically do any comparison
	// and so its usually all handled by PropertyValue::equality() which compares the revisions.
	return equality(other);
}


bool
GPlatesModel::PropertyValue::equality(
		const PropertyValue &other) const
{
	// Compare the mutable data that is contained in the revisions.
	return d_current_revision->equality(*other.d_current_revision);
}


boost::optional<GPlatesModel::Model &>
GPlatesModel::PropertyValue::get_model()
{
	if (!d_current_parent)
	{
		return boost::none;
	}

	return d_current_parent->get_model();
}


boost::optional<const GPlatesModel::Model &>
GPlatesModel::PropertyValue::get_model() const
{
	if (!d_current_parent)
	{
		return boost::none;
	}

	boost::optional<GPlatesModel::Model &> model = d_current_parent->get_model();
	if (!model)
	{
		return boost::none;
	}

	return model.get();
}


GPlatesModel::PropertyValue::MutableRevisionHandler::MutableRevisionHandler(
		const PropertyValue::non_null_ptr_type &property_value) :
	d_model(property_value->get_model()),
	d_property_value(property_value),
	d_mutable_revision(
		// The current property value revision is immutable so we create a new revision by cloning it.
		d_property_value->d_current_revision->clone_for_bubble_up_modification())
{
}


void
GPlatesModel::PropertyValue::MutableRevisionHandler::handle_revision_modification()
{
	// Create a model transaction that will switch the current revision to the new one.
	ModelTransaction transaction;

	RevisionedReference revision(d_property_value, d_mutable_revision);
	transaction.set_property_value_revision(revision);

	// If the property value has a parent then bubble up the modification towards the root (feature store).
	if (d_property_value->d_current_parent)
	{
		d_property_value->d_current_parent->bubble_up_modification(revision, transaction);
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
		const PropertyValue &property_value)
{
	return property_value.print_to(os);
}

