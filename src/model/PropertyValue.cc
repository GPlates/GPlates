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

#include "ModelTransaction.h"
#include "PropertyValueRevisionedReference.h"


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
	if (!d_current_revision->get_context())
	{
		return boost::none;
	}

	return d_current_revision->get_context()->get_model();
}


boost::optional<const GPlatesModel::Model &>
GPlatesModel::PropertyValue::get_model() const
{
	if (!d_current_revision->get_context())
	{
		return boost::none;
	}

	boost::optional<GPlatesModel::Model &> model = d_current_revision->get_context()->get_model();
	if (!model)
	{
		return boost::none;
	}

	return model.get();
}


GPlatesModel::PropertyValueRevision::non_null_ptr_type
GPlatesModel::PropertyValue::create_bubble_up_revision(
		ModelTransaction &transaction) const
{
	// If we don't have a (parent) context then just clone the current revision without any context.
	if (!get_current_revision()->get_context())
	{
		PropertyValueRevision::non_null_ptr_type cloned_revision =
				get_current_revision()->clone_revision();

		transaction.add_property_value_transaction(
				ModelTransaction::PropertyValueTransaction(this, cloned_revision));

		return cloned_revision;
	}

	// We have a parent context so bubble up the revision towards the root
	// (feature store). And our parent will create a new revision for us...
	return get_current_revision()->get_context()->bubble_up(transaction, this);
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const PropertyValue &property_value)
{
	return property_value.print_to(os);
}

