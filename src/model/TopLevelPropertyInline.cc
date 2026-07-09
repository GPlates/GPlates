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

#include "BubbleUpRevisionHandler.h"
#include "FeatureVisitor.h"
#include "ModelTransaction.h"
#include "TranscribeQualifiedXmlName.h"
#include "TranscribeStringContentTypeGenerator.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "scribe/Scribe.h"

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

#include "property-values/GpmlPlateId.h"

GPlatesScribe::TranscribeResult
GPlatesModel::TopLevelPropertyInline::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<TopLevelPropertyInline> &top_level_property_inline)
{
	if (scribe.is_saving())
	{
		// Save the property name.
		scribe.save(TRANSCRIBE_SOURCE, top_level_property_inline->get_property_name(), "property_name");

		// Save the property values.
		const std::vector<PropertyValue::non_null_ptr_type> property_values(
				top_level_property_inline->begin(),
				top_level_property_inline->end());
		scribe.save(TRANSCRIBE_SOURCE, property_values, "property_values");

		// Save the XML attributes.
		scribe.save(TRANSCRIBE_SOURCE, top_level_property_inline->get_xml_attributes(), "xml_attributes");
	}
	else // loading
	{
		// Load the property name.
		GPlatesScribe::LoadRef<PropertyName> property_name = scribe.load<PropertyName>(TRANSCRIBE_SOURCE, "property_name");
		if (!property_name.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Load the property values.
		std::vector<PropertyValue::non_null_ptr_type> property_values;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, property_values, "property_values"))
		{
			return scribe.get_transcribe_result();
		}

		// Load the XML attributes.
		xml_attributes_type xml_attributes;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, xml_attributes, "xml_attributes"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property.
		GPlatesModel::ModelTransaction transaction;
		top_level_property_inline.construct_object(
				boost::ref(transaction),  // non-const ref
				property_name,
				property_values.begin(),
				property_values.end(),
				xml_attributes);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesModel::TopLevelPropertyInline::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			// Save the property name.
			scribe.save(TRANSCRIBE_SOURCE, get_property_name(), "property_name");

			// Save the property values.
			const std::vector<PropertyValue::non_null_ptr_type> property_values(begin(), end());
			scribe.save(TRANSCRIBE_SOURCE, property_values, "property_values");

			// Save the XML attributes.
			scribe.save(TRANSCRIBE_SOURCE, get_xml_attributes(), "xml_attributes");
		}
		else // loading
		{
			// Load the property name.
			GPlatesScribe::LoadRef<PropertyName> property_name = scribe.load<PropertyName>(TRANSCRIBE_SOURCE, "property_name");
			if (!property_name.is_valid())
			{
				return scribe.get_transcribe_result();
			}
			d_property_name = property_name;

			// Load the property values.
			std::vector<PropertyValue::non_null_ptr_type> property_values;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, property_values, "property_values"))
			{
				return scribe.get_transcribe_result();
			}

			// Load the XML attributes.
			xml_attributes_type xml_attributes;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, xml_attributes, "xml_attributes"))
			{
				return scribe.get_transcribe_result();
			}

			// Modify 'this' TopLevelPropertyInline object.
			//
			// There's no set method for assigning revisioned property values and XML attributes.
			// So we do the equivalent inline here.
			BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();

			// Set the XML attributes.
			revision.xml_attributes = xml_attributes;

			// First remove any property values.
			for (auto &revisioned_property_value : revision.values)
			{
				revisioned_property_value.detach(revision_handler.get_model_transaction());
			}
			revision.values.clear();

			// Then add our loaded property values.
			for (auto property_value : property_values)
			{
				revision.values.push_back(
						RevisionedReference<PropertyValue>::attach(
								revision_handler.get_model_transaction(), *this, property_value));
			}

			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::TopLevelProperty, TopLevelPropertyInline>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
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

