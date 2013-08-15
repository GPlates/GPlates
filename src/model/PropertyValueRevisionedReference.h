/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_PROPERTYVALUEREVISIONEDREFERENCE_H
#define GPLATES_MODEL_PROPERTYVALUEREVISIONEDREFERENCE_H

#include <boost/optional.hpp>

#include "ModelTransaction.h"
#include "PropertyValue.h"
#include "PropertyValueRevision.h"
#include "PropertyValueRevisionContext.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	/**
	 * Reference to a property value and one of its revision snapshots.
	 *
	 * Note that the revision is not the current revision of the property value until the
	 * associated @a ModelTransaction has been committed.
	 *
	 * Template parameter 'PropertyValueType' is PropertyValue or one of its derived types and
	 * can be const or non-const (eg, 'GpmlPlateId' or 'const GpmlPlateId').
	 */
	template <class PropertyValueType>
	class PropertyValueRevisionedReference
	{
	public:

		/**
		 * Creates a revisioned reference by attaching the specified property value to the
		 * specific revision context.
		 */
		static
		PropertyValueRevisionedReference
		attach(
				ModelTransaction &transaction,
				PropertyValueRevisionContext &revision_context,
				const GPlatesUtils::non_null_intrusive_ptr<PropertyValueType> &property_value);


		/**
		 * Detaches the current property value from its revision context (leaving it without a context).
		 *
		 * Returns the detached revision context if there was one.
		 */
		boost::optional<PropertyValueRevisionContext &>
		detach(
				ModelTransaction &transaction);


		/**
		 * Changes the property value.
		 *
		 * This detaches the current property value and attaches the specified property value.
		 * And the revision context, if any, is transferred.
		 */
		void
		change(
				ModelTransaction &transaction,
				const GPlatesUtils::non_null_intrusive_ptr<PropertyValueType> &property_value);


		/**
		 * Makes 'this' revision reference a shallow copy of the current property value.
		 *
		 * Essentially clones the property value's revision (which does not recursively
		 * copy nested property values).
		 *
		 * Also returns the cloned revision as a modifiable (non-const) object.
		 */
		PropertyValueRevision::non_null_ptr_type
		clone_revision(
				ModelTransaction &transaction);


		/**
		 * Makes 'this' revision reference a deep copy of the current property value.
		 *
		 * This recursively clones the property value and its revision
		 * (including nested property values and their revisions).
		 */
		void
		clone(
				PropertyValueRevisionContext &revision_context);


		/**
		 * Returns the property value.
		 */
		GPlatesUtils::non_null_intrusive_ptr<PropertyValueType>
		get_property_value() const
		{
			return d_property_value;
		}


		//
		// NOTE: We don't return the property value 'revision' (const or non-const).
		// Since revisions are immutable, @a clone_revision should be used when a
		// property value is to be modified.
		//

	private:

		explicit
		PropertyValueRevisionedReference(
				const GPlatesUtils::non_null_intrusive_ptr<PropertyValueType> &property_value,
				const PropertyValueRevision::non_null_ptr_to_const_type &revision) :
			d_property_value(property_value),
			d_revision(revision)
		{  }

		GPlatesUtils::non_null_intrusive_ptr<PropertyValueType> d_property_value;
		PropertyValueRevision::non_null_ptr_to_const_type d_revision;
	};
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesModel
{
	template <class PropertyValueType>
	PropertyValueRevisionedReference<PropertyValueType>
	PropertyValueRevisionedReference<PropertyValueType>::attach(
			ModelTransaction &transaction,
			PropertyValueRevisionContext &revision_context,
			const GPlatesUtils::non_null_intrusive_ptr<PropertyValueType> &property_value)
	{
		// Clone the property value's current revision but attach to the new revision context.
		PropertyValueRevisionedReference revisioned_reference(
				property_value,
				property_value->d_current_revision->clone_revision(revision_context));

		transaction.add_property_value_transaction(
				ModelTransaction::PropertyValueTransaction(
						revisioned_reference.d_property_value,
						revisioned_reference.d_revision));

		return revisioned_reference;
	}


	template <class PropertyValueType>
	boost::optional<PropertyValueRevisionContext &>
	PropertyValueRevisionedReference<PropertyValueType>::detach(
			ModelTransaction &transaction)
	{
		boost::optional<PropertyValueRevisionContext &> revision_context = d_revision->get_context();

		// Detach the current property value by creating a revision with no context.
		d_revision = d_revision->clone_revision();

		transaction.add_property_value_transaction(
				ModelTransaction::PropertyValueTransaction(d_property_value, d_revision));

		return revision_context;
	}


	template <class PropertyValueType>
	void
	PropertyValueRevisionedReference<PropertyValueType>::change(
			ModelTransaction &transaction,
			const GPlatesUtils::non_null_intrusive_ptr<PropertyValueType> &property_value)
	{
		boost::optional<PropertyValueRevisionContext &> revision_context = d_revision->get_context();

		// Detach the current property value by creating a revision with no context.
		transaction.add_property_value_transaction(
				ModelTransaction::PropertyValueTransaction(
						d_property_value,
						d_revision->clone_revision()));

		// Attach the new property value by creating a revision with the detached context.
		d_property_value = property_value;
		d_revision = d_property_value->d_current_revision->clone_revision(revision_context);
		transaction.add_property_value_transaction(
				ModelTransaction::PropertyValueTransaction(d_property_value, d_revision));
	}


	template <class PropertyValueType>
	PropertyValueRevision::non_null_ptr_type
	PropertyValueRevisionedReference<PropertyValueType>::clone_revision(
			ModelTransaction &transaction)
	{
		boost::optional<PropertyValueRevisionContext &> revision_context = d_revision->get_context();

		// The cloned revision's context is the same as the original revision.
		// Essentially this means the parent property value (or top-level property) is the
		// same for both revisions.
		PropertyValueRevision::non_null_ptr_type mutable_revision =
				d_revision->clone_revision(revision_context);
		d_revision = mutable_revision;

		transaction.add_property_value_transaction(
				ModelTransaction::PropertyValueTransaction(d_property_value, d_revision));

		return mutable_revision;
	}


	template <class PropertyValueType>
	void
	PropertyValueRevisionedReference<PropertyValueType>::clone(
			PropertyValueRevisionContext &revision_context)
	{
		d_property_value = d_property_value->clone_impl(revision_context);
		d_revision = d_property_value->d_current_revision;

		// No model transaction needed here since the cloned property value already
		// points to its cloned revision.
	}
}

#endif // GPLATES_MODEL_PROPERTYVALUEREVISIONEDREFERENCE_H
