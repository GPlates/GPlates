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
#include <iostream>
#include <algorithm>

// Suppress warning being emitted from Boost 1.35 header.
#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/lambda.hpp>

#include "TopLevelPropertyInline.h"
#include "FeatureVisitor.h"

#include "utils/UnicodeStringUtils.h"


const GPlatesModel::TopLevelPropertyInline::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::create(
		const PropertyName &property_name_,
		PropertyValue::non_null_ptr_type value_,
		const xml_attributes_type &xml_attributes_)
{
	return create(property_name_, &value_, &value_ + 1, xml_attributes_);
}


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


GPlatesModel::TopLevelPropertyInline::TopLevelPropertyInline(
		const TopLevelPropertyInline &other) :
	TopLevelProperty(other)
{
	// Clone the property values.
	container_type::const_iterator other_iter = other.d_values.begin();
	container_type::const_iterator other_end = other.d_values.end();
	for ( ; other_iter != other_end; ++other_iter)
	{
		PropertyValue::non_null_ptr_type cloned_pval = other_iter->get_property_value()->clone();
		d_values.push_back(cloned_pval->get_current_revisioned_reference());
	}
}


GPlatesModel::TopLevelPropertyInline::~TopLevelPropertyInline()
{
	// Remove parent references from property values to us.
	// This is in case one or more property values outlive us, eg, because a client is holding
	// an intrusive pointer to the property value(s).
	container_type::iterator values_iter = d_values.begin();
	container_type::iterator values_end = d_values.end();
	for ( ; values_iter != values_end; ++values_iter)
	{
		PropertyValue::RevisionedReference &revisioned_property_value = *values_iter;

		revisioned_property_value.get_property_value()->unset_parent();
	}
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
	os << property_name().build_aliased_name() << " [ ";

	bool first = true;
	for (container_type::const_iterator iter = d_values.begin(); iter != d_values.end(); ++iter)
	{
		if (first)
		{
			first = false;
		}
		else
		{
			os << " , ";
		}
		os << *iter->get_property_value();
	}
	os << " ]";

	return os;
}


bool
GPlatesModel::TopLevelPropertyInline::equality(
		const TopLevelProperty &other) const
{
	const TopLevelPropertyInline &other_inline = dynamic_cast<const TopLevelPropertyInline &>(other);

	return d_values.size() == other_inline.d_values.size() &&
			// Use the client iterator (instead of internal iterator) in order to compare
			// property values and not revisioned property values...
			std::equal(
					begin(),
					end(),
					other_inline.begin(),
					// Compare PropertyValues, not pointers to PropertyValues...
					*boost::lambda::_1 == *boost::lambda::_2) &&
			TopLevelProperty::equality(other);
}


// See above.
POP_MSVC_WARNINGS

