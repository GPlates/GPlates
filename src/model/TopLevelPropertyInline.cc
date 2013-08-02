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

// Suppress warning being emitted from Boost 1.35 header.
#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/lambda.hpp>

#include "TopLevelPropertyInline.h"
#include "FeatureVisitor.h"

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


GPlatesModel::TopLevelPropertyInline::~TopLevelPropertyInline()
{
	// Remove parent references in our child property values.
	// This is in case one or more child property values outlive us, eg, because some client
	// is holding an intrusive pointer to the property value(s).
	unset_parent_on_child_property_values();
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


void
GPlatesModel::TopLevelPropertyInline::set_parent_on_child_property_values()
{
	Revision &revision = get_current_revision<Revision>();
	container_type::iterator values_iter = revision.values.begin();
	container_type::iterator values_end = revision.values.end();
	for ( ; values_iter != values_end; ++values_iter)
	{
		PropertyValue::RevisionedReference &revisioned_property_value = *values_iter;

		// Enable property value modifications to bubble up to us.
		// Note that we didn't clone the property value - so an external client could modify
		// it (if it has a pointer to the property value) and this modification will bubble up
		// to us and cause us to create a new TopLevelProperty revision.
		revisioned_property_value.get_property_value()->set_parent(*this);
	}
}


void
GPlatesModel::TopLevelPropertyInline::unset_parent_on_child_property_values()
{
	Revision &revision = get_current_revision<Revision>();

	// Remove parent references in our child property values.
	container_type::iterator values_iter = revision.values.begin();
	container_type::iterator values_end = revision.values.end();
	for ( ; values_iter != values_end; ++values_iter)
	{
		PropertyValue::RevisionedReference &revisioned_property_value = *values_iter;

		revisioned_property_value.get_property_value()->unset_parent();
	}
}


GPlatesModel::TopLevelPropertyInline::Revision::Revision(
		const Revision &other) :
	TopLevelProperty::Revision(other)
{
	// Clone the property values.
	BOOST_FOREACH(const PropertyValue::RevisionedReference &other_value, other.values)
	{
		PropertyValue::non_null_ptr_type cloned_pval = other_value.get_property_value()->clone();
		values.push_back(cloned_pval->get_current_revisioned_reference());
	}
}


void
GPlatesModel::TopLevelPropertyInline::Revision::reference_bubbled_up_property_value_revision(
		const PropertyValue::RevisionedReference &property_value_revisioned_reference)
{
	// In this method we are operating on a (bubble up) cloned version of the current revision.
	// We just need to change the existing property value reference so it points to the new property value.

	// Search for the property value in our property value list.
	container_type::iterator values_iter = values.begin();
	container_type::iterator values_end = values.end();
	for ( ; values_iter != values_end; ++values_iter)
	{
		// If the new revision and the existing revision are associated with the same property value
		// then update the new revision to reference the bubbled-up property value revision.
		if (values_iter->get_property_value() == property_value_revisioned_reference.get_property_value())
		{
			*values_iter = property_value_revisioned_reference;
			return;
		}
	}

	// Shouldn't get here if the revisions have been linked up correctly.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


bool
GPlatesModel::TopLevelPropertyInline::Revision::equality(
		const TopLevelProperty::Revision &other) const
{
	const Revision &other_revision = static_cast<const Revision &>(other);

	if (values.size() != other_revision.values.size())
	{
		return false;
	}

	for (std::size_t n = 0; n < values.size(); ++n)
	{
		// Compare PropertyValues, not pointers to PropertyValues...
		if (*values[n].get_property_value() != *other_revision.values[n].get_property_value())
		{
			return false;
		}
	}

	return TopLevelProperty::Revision::equality(other);
}


// See above.
POP_MSVC_WARNINGS

