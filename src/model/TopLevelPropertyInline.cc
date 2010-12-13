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
#include <typeinfo>
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
		const container_type &values_,
		const xml_attributes_type &xml_attributes_)
{
	non_null_ptr_type ptr(
			new TopLevelPropertyInline(
				property_name_,
				values_,
				xml_attributes_));
	return ptr;
}


const GPlatesModel::TopLevelPropertyInline::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::create(
		const PropertyName &property_name_,
		PropertyValue::non_null_ptr_type value_,
		const xml_attributes_type &xml_attributes_)
{
	non_null_ptr_type ptr(
			new TopLevelPropertyInline(
				property_name_,
				value_,
				xml_attributes_));
	return ptr;
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


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::clone() const
{
	TopLevelProperty::non_null_ptr_type dup(
			new TopLevelPropertyInline(*this));
	return dup;
}


const GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesModel::TopLevelPropertyInline::deep_clone() const 
{
	TopLevelPropertyInline::non_null_ptr_type dup = create(
			property_name(),
			container_type(),
			xml_attributes());

	container_type::const_iterator iter = d_values.begin(), end_ = d_values.end();
	for ( ; iter != end_; ++iter)
	{
		PropertyValue::non_null_ptr_type cloned_pval = (*iter)->deep_clone_as_prop_val();
		dup->d_values.push_back(cloned_pval);
	}
	return TopLevelProperty::non_null_ptr_type(dup);
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
		os << **iter;
	}
	os << " ]";

	return os;
}


bool
GPlatesModel::TopLevelPropertyInline::operator==(
		const TopLevelProperty &other) const
{
	try
	{
		const TopLevelPropertyInline &other_inline = dynamic_cast<const TopLevelPropertyInline &>(other);
		if (property_name() == other.property_name() &&
			xml_attributes() == other.xml_attributes() &&
			d_values.size() == other_inline.d_values.size())
		{
			return std::equal(
					d_values.begin(),
					d_values.end(),
					other_inline.d_values.begin(),
					// Compare PropertyValues, not pointers to PropertyValues.
					*boost::lambda::_1 == *boost::lambda::_2);
		}
		else
		{
			return false;
		}
	}
	catch (const std::bad_cast &)
	{
		return false;
	}
}


// See above.
POP_MSVC_WARNINGS

