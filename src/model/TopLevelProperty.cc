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
#include "TopLevelPropertyBubbleUpRevisionHandler.h"


void
GPlatesModel::TopLevelProperty::set_xml_attributes(
		const xml_attributes_type &xml_attributes)
{
	TopLevelPropertyBubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<TopLevelPropertyRevision>().xml_attributes = xml_attributes;
	revision_handler.commit();
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
	// Since most (all) of the value data is contained in the revisions, which is handled by the
	// base TopLevelProperty class, the derived top-level property classes don't typically do any comparison
	// and so its usually all handled by TopLevelProperty::equality() which compares the revisions.
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
	// TODO: Implement this once we can connect to a parent feature handle.
	return boost::none;
}


boost::optional<const GPlatesModel::Model &>
GPlatesModel::TopLevelProperty::get_model() const
{
	// TODO: Implement this once we can connect to a parent feature handle.
	return boost::none;
}


GPlatesModel::TopLevelPropertyRevision::non_null_ptr_type
GPlatesModel::TopLevelProperty::create_bubble_up_revision(
		ModelTransaction &transaction) const
{
	// If we don't have a (parent) context then just clone the current revision without any context.
#if 0 // TODO: Enable this when can connect to parent context.
	if (!get_current_revision()->get_context())
#endif
	{
		TopLevelPropertyRevision::non_null_ptr_type cloned_revision =
				get_current_revision()->clone_revision();

		transaction.set_top_level_property_transaction(
				ModelTransaction::TopLevelPropertyTransaction(this, cloned_revision));

		return cloned_revision;
	}

#if 0 // TODO: Enable this when can connect to parent context.
	// We have a parent context so bubble up the revision towards the root
	// (feature store). And our parent will create a new revision for us...
	return get_current_revision()->get_context()->bubble_up(transaction, this);
#endif
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const TopLevelProperty &top_level_prop)
{
	return top_level_prop.print_to(os);
}

