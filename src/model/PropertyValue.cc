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
	// Re-use 'const' overload.
	boost::optional<const GPlatesModel::Model &> model =
			static_cast<const PropertyValue *>(this)->get_model();
	if (!model)
	{
		return boost::none;
	}

	return const_cast<Model &>(model.get());
}


boost::optional<const GPlatesModel::Model &>
GPlatesModel::PropertyValue::get_model() const
{
	boost::optional<parent_reference_type> parent = d_current_revision->get_parent();
	if (!parent)
	{
		return boost::none;
	}

	// Our parent could be one of two different types...
	struct ModelPtrVisitor :
			public boost::static_visitor<>
	{
		void
		operator()(
				const TopLevelProperty *parent)
		{
			model = parent->get_model();
		}

		void
		operator()(
				const PropertyValue *parent)
		{
			model = parent->get_model();
		}

		boost::optional<const Model &> model;
	};

	ModelPtrVisitor visitor;
	boost::apply_visitor(visitor, parent.get());

	return visitor.model;
}


GPlatesModel::PropertyValue::RevisionedReference
GPlatesModel::PropertyValue::set_parent(
		TopLevelProperty &parent)
{
	// Create a new revision to set the parent reference in.
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision()->set_parent(parent_reference_type(&parent));
	revision_handler.handle_revision_modification();

	return get_current_revisioned_reference();
}


GPlatesModel::PropertyValue::RevisionedReference
GPlatesModel::PropertyValue::set_parent(
		PropertyValue &parent)
{
	// Create a new revision to set the parent reference in.
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision()->set_parent(parent_reference_type(&parent));
	revision_handler.handle_revision_modification();

	return get_current_revisioned_reference();
}


void
GPlatesModel::PropertyValue::unset_parent()
{
	// Create a new revision to unset the parent reference in.
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision()->set_parent();
	revision_handler.handle_revision_modification();
}


GPlatesModel::PropertyValue::MutableRevisionHandler::MutableRevisionHandler(
		const PropertyValue::non_null_ptr_type &property_value) :
	d_model(property_value->get_model()),
	d_property_value(property_value),
	d_mutable_revision(
		// The current property value revision is immutable so we create a new revision by cloning it.
		// But we don't clone nested property values because (being property values) they are already revisioned...
		d_property_value->d_current_revision->clone_for_bubble_up_modification())
{
}


void
GPlatesModel::PropertyValue::MutableRevisionHandler::handle_revision_modification()
{
	// Create a model transaction that will switch the new revision with the current one.
	ModelTransaction transaction;

	RevisionedReference revision(d_property_value, d_mutable_revision);
	transaction.add_property_value_revision(revision);

	// If we have a parent then bubble up our modification towards the root (feature store).
	boost::optional<parent_reference_type> parent = d_mutable_revision->get_parent();
	if (parent)
	{
		// Our parent could be one of two different types...
		struct BubbleUpVisitor :
				public boost::static_visitor<>
		{
			BubbleUpVisitor(
					const RevisionedReference &revision_,
					ModelTransaction &transaction_) :
				revision(revision_),
				transaction(transaction_)
			{  }

			void
			operator()(
					TopLevelProperty *parent)
			{
				parent->bubble_up_modification(revision, transaction);
			}

			void
			operator()(
					PropertyValue *parent)
			{
				parent->bubble_up_modification(revision, transaction);
			}

			const RevisionedReference &revision;
			ModelTransaction &transaction;
		};

		BubbleUpVisitor visitor(revision, transaction);
		boost::apply_visitor(visitor, parent.get());
	}

	// If the bubble-up reaches the top of the model hierarchy (ie, connected all the way up to the model)
	// then the model will store the new version in the undo/redo queue.
	// The commit also takes care of model events - either emitting an event immediately
	// or queuing it if a model notification guard is currently active.
	transaction.commit();

	if (!d_model ||
		!d_model->has_notification_guard())
	{
	}

#if 0
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
#endif
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const PropertyValue &property_value)
{
	return property_value.print_to(os);
}

