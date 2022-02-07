/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class TopLevelPropertyInline.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <algorithm>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

// Suppress warning being emitted from Boost 1.35 header.
#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/lambda.hpp>

#include "TopLevelPropertyInline.h"

#include "FeatureVisitor.h"
#include "ModelTransaction.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/UnicodeStringUtils.h"


const GPlatesModel::TopLevelPropertyInline::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::create(
		const PropertyName &property_name_,
		PropertyValue::non_null_ptr_type value_,
		const GPlatesUtils::UnicodeString &attribute_name_string,
		const GPlatesUtils::UnicodeString &attribute_value_string)
{
	xml_attributes_type xml_attributes_;

	XmlAttributeName xml_attribute_name =
		XmlAttributeName::create_gpml(
				GPlatesUtils::make_qstring_from_icu_string(attribute_name_string));
	XmlAttributeValue xml_attribute_value(attribute_value_string);
	xml_attributes_.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	return create(
			property_name_,
			value_,
			xml_attributes_);
}


void
GPlatesModel::TopLevelPropertyInline::accept_visitor(
		ConstFeatureVisitor &visitor) const 
{
	visitor.visit_top_level_property_inline(*this);
}


void
GPlatesModel::TopLevelPropertyInline::accept_visitor(
		FeatureVisitor &visitor)
{
	visitor.visit_top_level_property_inline(*this);
}


std::ostream &
GPlatesModel::TopLevelPropertyInline::print_to(
		std::ostream &os) const
{
	os << get_property_name().build_aliased_name() << " [ ";

	bool first = true;
	for (const_iterator iter = begin(); iter != end(); ++iter)
	{
		if (first)
		{
			first = false;
		}
		else
		{
			os << " , ";
		}
		os << **iter;
	}
	os << " ]";

	return os;
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::bubble_up(
		ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	boost::optional<GPlatesModel::Revision::non_null_ptr_type> child_revision;

	// Search for the child property value in our property value list.
	property_value_container_type::iterator values_iter = revision.values.begin();
	property_value_container_type::iterator values_end = revision.values.end();
	for ( ; values_iter != values_end; ++values_iter)
	{
		RevisionedReference<PropertyValue> &revisioned_reference = *values_iter;

		if (child_revisionable == revisioned_reference.get_revisionable())
		{
			// Create a new revision for the child property value.
			child_revision = revisioned_reference.clone_revision(transaction);
			break;
		}
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revision,
			GPLATES_ASSERTION_SOURCE);

	return child_revision.get();
}


bool
GPlatesModel::TopLevelPropertyInline::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (values.size() != other_revision.values.size())
	{
		return false;
	}

	for (std::size_t n = 0; n < values.size(); ++n)
	{
		// Compare PropertyValues, not pointers to PropertyValues...
		if (*values[n].get_revisionable() != *other_revision.values[n].get_revisionable())
		{
			return false;
		}
	}

	return TopLevelProperty::Revision::equality(other);
}


// See above.
POP_MSVC_WARNINGS

