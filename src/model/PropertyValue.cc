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


GPlatesModel::Model *
GPlatesModel::PropertyValue::model_ptr()
{
	// Re-use 'const' overload.
	return const_cast<Model *>(
			static_cast<const PropertyValue *>(this)->model_ptr());
}


const GPlatesModel::Model *
GPlatesModel::PropertyValue::model_ptr() const
{
	if (!d_parent)
	{
		return NULL;
	}

	boost::optional<const TopLevelProperty *> top_level_property_parent =
			d_parent->get_top_level_property_parent();
	if (top_level_property_parent)
	{
		return top_level_property_parent.get()->model_ptr();
	}

	boost::optional<const PropertyValue *> property_value_parent =
			d_parent->get_property_value_parent();
	if (property_value_parent)
	{
		return property_value_parent.get()->model_ptr();
	}

	// Shouldn't get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	return NULL;
}


boost::optional<const GPlatesModel::TopLevelProperty *>
GPlatesModel::PropertyValue::ParentReference::get_top_level_property_parent() const
{
	// See if boost variant contains a 'TopLevelProperty *'.
	TopLevelProperty *const *ptr = boost::get<TopLevelProperty *>(&d_parent);
	if (ptr == NULL)
	{
		return boost::none;
	}

	return *ptr;
}


boost::optional<GPlatesModel::TopLevelProperty *>
GPlatesModel::PropertyValue::ParentReference::get_top_level_property_parent()
{
	// See if boost variant contains a 'TopLevelProperty *'.
	TopLevelProperty **ptr = boost::get<TopLevelProperty *>(&d_parent);
	if (ptr == NULL)
	{
		return boost::none;
	}

	return *ptr;
}


boost::optional<const GPlatesModel::PropertyValue *>
GPlatesModel::PropertyValue::ParentReference::get_property_value_parent() const
{
	// See if boost variant contains a 'PropertyValue *'.
	PropertyValue *const *ptr = boost::get<PropertyValue *>(&d_parent);
	if (ptr == NULL)
	{
		return boost::none;
	}

	return *ptr;
}


boost::optional<GPlatesModel::PropertyValue *>
GPlatesModel::PropertyValue::ParentReference::get_property_value_parent()
{
	// See if boost variant contains a 'PropertyValue *'.
	PropertyValue **ptr = boost::get<PropertyValue *>(&d_parent);
	if (ptr == NULL)
	{
		return boost::none;
	}

	return *ptr;
}


boost::optional<const GPlatesModel::PropertyValue::ParentReference>
GPlatesModel::PropertyValue::RevisionedReference::get_parent() const
{
	if (!d_property_value->d_parent)
	{
		return boost::none;
	}
	return boost::optional<const ParentReference>(d_property_value->d_parent.get());
}


GPlatesModel::PropertyValue::MutableRevisionHandler::MutableRevisionHandler(
		const PropertyValue::non_null_ptr_type &property_value) :
	d_model(property_value->model_ptr()),
	d_property_value(property_value),
	d_mutable_revision(
			d_model
					// If we're attached to a model then revisioning is active so clone the current revision,
					// but we don't clone nested property values since they are separately revisioned...
					? d_property_value->d_current_revision->clone_with_shared_nested_property_values()
					// Revisioning is not active so the current revision will be modified...
					: d_property_value->d_current_revision)
{
}


void
GPlatesModel::PropertyValue::MutableRevisionHandler::handle_revision_modification()
{
	if (d_model)
	{
		// Create a transaction that will switch the new revision with the current one.
		// After it's committed the transaction can later be rolled back, re-committed, etc, if the
		// user performs undo, redo, etc, without requiring this property value to be involved.
		Transaction::non_null_ptr_type transaction =
				Transaction::create(d_property_value, d_mutable_revision);

		// The model will commit the transaction and store it in the undo/redo queue.
		// The commit also takes care of model events - either emitting an event immediately
		// or queuing it if a model notification guard is currently active.
#if 0
		d_model->commit_transaction(transaction);
#endif
	}
	else // not attached to a model...
	{
#if 0
		// We are not attached to a model so there's no need for model transactions, but
		// if we belong to a parent feature then we should emit an event that signals
		// the parent feature was modified (because one of its properties changed).
		if (d_property_value->d_parent_feature_ref)
		{
			WeakReferencePublisherModifiedVisitor<FeatureHandle> visitor(
					WeakReferencePublisherModifiedEvent<FeatureHandle>::PUBLISHER_MODIFIED);
			d_property_value->d_parent_feature_ref->apply_weak_observer_visitor(visitor);
			WeakReferencePublisherModifiedVisitor<const FeatureHandle> const_visitor(
					WeakReferencePublisherModifiedEvent<const FeatureHandle>::PUBLISHER_MODIFIED);
			d_property_value->d_parent_feature_ref->apply_const_weak_observer_visitor(const_visitor);
		}
#endif
	}
}


void
GPlatesModel::PropertyValue::Transaction::commit()
{
	GPlatesUtils::swap(d_revision, d_property_value->d_current_revision);
}


void
GPlatesModel::PropertyValue::Transaction::rollback()
{
	GPlatesUtils::swap(d_revision, d_property_value->d_current_revision);
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const PropertyValue &property_value)
{
	return property_value.print_to(os);
}

