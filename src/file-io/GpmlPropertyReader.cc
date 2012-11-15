/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <sstream>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "GpmlPropertyReader.h"

#include "GpmlPropertyStructuralTypeReaderUtils.h"
#include "GpmlReaderException.h"
#include "GpmlReaderUtils.h"
#include "GpmlStructuralTypeReaderUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/GpgimVersion.h"
#include "model/ModelUtils.h"
#include "model/XmlNodeUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/StructuralType.h"
#include "property-values/UninterpretedPropertyValue.h"

#include "utils/UnicodeStringUtils.h"


#include <boost/foreach.hpp>
GPlatesFileIO::GpmlPropertyReader::GpmlPropertyReader(
		const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_property,
		const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version) :
	d_gpgim_property(gpgim_property),
	d_property_structural_type_reader(property_structural_type_reader),
	d_gpml_version(gpml_version),
	d_constant_value_structural_reader_function(
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_constant_value,
							_1, boost::cref(*property_structural_type_reader), _2, _3)),
	d_irregular_sampling_structural_reader_function(
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_irregular_sampling,
							_1, boost::cref(*property_structural_type_reader), _2, _3)),
	d_piecewise_aggregation_structural_reader_function(
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_piecewise_aggregation,
							_1, boost::cref(*property_structural_type_reader), _2, _3))
{
	// Get the allowed structural types for this property.
	const GPlatesModel::GpgimProperty::structural_type_seq_type &gpgim_structural_types =
			d_gpgim_property->get_structural_types();

	// For each structural type look up the structural reader function.
	d_structural_reader_types.reserve(gpgim_structural_types.size());
	BOOST_FOREACH(
			const GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type &gpgim_structural_type,
			gpgim_structural_types)
	{
		const GPlatesPropertyValues::StructuralType &structural_type =
				gpgim_structural_type->get_structural_type();

		// Get the reader function for the structural type.
		boost::optional<GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
				structural_reader_function =
						d_property_structural_type_reader->get_structural_type_reader_function(structural_type);
		if (!structural_reader_function)
		{
			// We shouldn't get here because when we read the GPGIM from an XML file we should already
			// have checked that the structural types in the GPGIM XML were recognised.
			qWarning() << "Error in GPlates Geological Information Model (GPGIM):";
			qWarning() << "   Unrecognised structural property type '"
					<< convert_qualified_xml_name_to_qstring(structural_type)
					<< "'.";
			continue;
		}

		d_structural_reader_types.push_back(
				StructuralReaderType(
						structural_type,
						structural_reader_function.get()));
	}
}


void
GPlatesFileIO::GpmlPropertyReader::read_properties(
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &property_values,
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Iterator over those feature properties whose property name matches 'get_property_name()'.
	GPlatesModel::XmlNodeUtils::NamedXmlElementNodeIterator<xml_node_seq_type::iterator> property_iter(
			unprocessed_feature_property_xml_nodes.begin(),
			unprocessed_feature_property_xml_nodes.end(),
			GPlatesModel::XmlElementName(get_property_name()));

	// How many times the property (name) can be instantiated.
	const GPlatesModel::GpgimProperty::MultiplicityType multiplicity =
			d_gpgim_property->get_multiplicity();

	// If the property is required (at least once), but none are found then emit a warning.
	if (multiplicity == GPlatesModel::GpgimProperty::ONE ||
		multiplicity == GPlatesModel::GpgimProperty::ONE_OR_MORE)
	{
		if (property_iter.finished())
		{
			// This property (name) must be instantiated at least once per feature.
			// Direct warning to the feature XML element.
			append_warning(feature_xml_element, reader_params,
					ReadErrors::NecessaryPropertyNotFound, ReadErrors::PropertyNotInterpreted);

			// Also log a warning message to the console since the read errors dialog does not
			// tell the user which property (name) was missing.
			std::stringstream filename_string_stream;
			reader_params.source->write_full_name(filename_string_stream);
			qWarning() << "Failed to find property '"
				<< convert_qualified_xml_name_to_qstring(get_property_name())
				<< "' in the feature at line '" << feature_xml_element->line_number()
				<< "' in the file '" << filename_string_stream.str().c_str() << "'";

			// No properties to read, so return.
			return;
		}
	}

	// If the property cannot occur more than once, but multiple properties are found then emit a warning.
	if (multiplicity == GPlatesModel::GpgimProperty::ZERO_OR_ONE ||
		multiplicity == GPlatesModel::GpgimProperty::ONE)
	{
		// See if there are two properties whose property name matches 'get_property_name()'.
		if (!property_iter.finished()/*found one property*/ &&
			property_iter.has_next()/*found two properties*/)
		{
			// This property (name) is not allowed to be instantiated more than once per feature.
			// Direct warning to the first duplicate property so user knows which property caused problem.
			append_warning(property_iter.get_xml_element(), reader_params,
					ReadErrors::DuplicateProperty, ReadErrors::PropertyNotInterpreted);

			// Read all duplicate properties as 'UninterpretedPropertyValue' property values.
			// This ensures they get stored in the GPML file when it gets written back out to disk.
			// Also our interface dictates we will generate a property value for each property that
			// has a property name accepted by the GPGIM.
			while (!property_iter.finished())
			{
				GPlatesModel::XmlElementNode::non_null_ptr_type property_xml_element =
						property_iter.get_xml_element();

				// Add 'UninterpretedPropertyValue' property value to the list of property values created.
				property_values.push_back(
						GPlatesPropertyValues::UninterpretedPropertyValue::create(property_xml_element));

				xml_node_seq_type::iterator unprocessed_xml_node_iter = property_iter.get_xml_node_iterator();
				// Increment to the next matching property before we erase the XML node - otherwise list iteration breaks.
				property_iter.next();
				// Remove the XML node from the unprocessed property node list.
				unprocessed_feature_property_xml_nodes.erase(unprocessed_xml_node_iter);
			}

			// Properties already read as 'UninterpretedPropertyValue' property values.
			return;
		}
	}

	// Iterate over the properties whose property name matches 'get_property_name()' and
	// attempt to interpret them.
	while (!property_iter.finished())
	{
		GPlatesModel::XmlElementNode::non_null_ptr_type property_xml_element =
				property_iter.get_xml_element();

		// Attempt to read/interpret the current property.
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> property_value =
				read_property(property_xml_element, reader_params);
		if (!property_value)
		{
			// The current property was not interpreted.
			// So read as an 'UninterpretedPropertyValue' property value.
			// This ensures it gets stored in the GPML file when it gets written back out to disk.
			property_value = GPlatesPropertyValues::UninterpretedPropertyValue::create(property_xml_element);
		}

		// Add to the list of property values created.
		property_values.push_back(property_value.get());

		xml_node_seq_type::iterator unprocessed_xml_node_iter = property_iter.get_xml_node_iterator();
		// Increment to the next matching property before we erase the XML node - otherwise list iteration breaks.
		property_iter.next();
		// Remove the XML node from the unprocessed property node list.
		unprocessed_feature_property_xml_nodes.erase(unprocessed_xml_node_iter);
	}
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesFileIO::GpmlPropertyReader::read_property(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> structural_xml_element;

	// See if the current property XML element has a child structural XML element.
	if (property_xml_element->number_of_children() != 0)
	{
		GPlatesModel::XmlNodeUtils::XmlElementNodeExtractionVisitor xml_element_node_extraction_visitor;
		structural_xml_element = xml_element_node_extraction_visitor.get_xml_element_node(
				*property_xml_element->children_begin());
		if (structural_xml_element)
		{
			// Verify there's only one child XML element node of the current property XML element.
			if (property_xml_element->number_of_children() > 1)
			{
				// Properties with multiple inline structural elements are not (yet) handled!
				append_warning(property_xml_element, reader_params,
						ReadErrors::NonUniqueStructuralElement, ReadErrors::PropertyNotInterpreted);

				// Failed to read/interpret the property.
				return boost::none;
			}
		}
		// ...else the current property contains text only (and does not need verification at this stage).
	}

	// Attempt to read the property structural type if it matches the GPGIM.
	return read_property_structural_type(property_xml_element, structural_xml_element, reader_params);
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesFileIO::GpmlPropertyReader::read_property_structural_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> structural_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	static const GPlatesModel::XmlElementName CONSTANT_VALUE =
			GPlatesModel::XmlElementName::create_gpml("ConstantValue");
	static const GPlatesModel::XmlElementName IRREGULAR_SAMPLING =
			GPlatesModel::XmlElementName::create_gpml("IrregularSampling");
	static const GPlatesModel::XmlElementName PIECEWISE_AGGREGATION =
			GPlatesModel::XmlElementName::create_gpml("PiecewiseAggregation");

	// If any time-dependent flags are set then the property is expected to have a time-dependent wrapper.
	const GPlatesModel::GpgimProperty::time_dependent_flags_type &time_dependent_flags =
			d_gpgim_property->get_time_dependent_types();

	// See if the property is expected to be a time-dependent property.
	if (time_dependent_flags.any())
	{
		// The structural element is expected to be a time-dependent wrapper.
		// So there actually needs to be a structural element in the first place.
		if (!structural_xml_element)
		{
			// The property is not wrapped in a time-dependent structure (but it should have been).
			// So attempt to wrap the unwrapped property in a time-dependent wrapper.
			// This method handles warning messages and can return success or failure.
			// If it fails then we have no remaining options, so we always return.
			return convert_unwrapped_to_time_dependent_wrapped_structural_type(
					property_xml_element,
					structural_xml_element,
					reader_params);
		}

		const GPlatesModel::XmlElementName &structural_element_name = structural_xml_element.get()->get_name();

		// The structural element is expected to be a time-dependent wrapper.
		// If it's not then attempt to wrap it in one.
		if (structural_element_name != CONSTANT_VALUE &&
			structural_element_name != IRREGULAR_SAMPLING &&
			structural_element_name != PIECEWISE_AGGREGATION)
		{
			// The property is not wrapped in a time-dependent structure (but it should have been).
			// So attempt to wrap the unwrapped property in a time-dependent wrapper.
			// This method handles warning messages and can return success or failure.
			// If it fails then we have no remaining options, so we always return.
			return convert_unwrapped_to_time_dependent_wrapped_structural_type(
					property_xml_element,
					structural_xml_element.get(),
					reader_params);
		}

		// The structural element is a time-dependent wrapper (as it should be).
		// See if the wrapped structural type is allowed (by the GPGIM property).
		if (!get_time_dependent_wrapped_structural_type(structural_xml_element.get(), reader_params))
		{
			// The property's structural element type is not allowed (by the GPGIM).
			append_warning(structural_xml_element.get(), reader_params,
					ReadErrors::UnexpectedPropertyStructuralElement, ReadErrors::PropertyNotInterpreted);

			// Failed to read/interpret the property.
			return boost::none;
		}

		// See if the structural type is a 'gpml:ConstantValue' and whether that is allowed by the GPGIM.
		if (time_dependent_flags.test(GPlatesModel::GpgimProperty::CONSTANT_VALUE) &&
			structural_element_name == CONSTANT_VALUE)
		{
			return read_structural_type(
					property_xml_element,
					d_constant_value_structural_reader_function,
					reader_params);
		}

		// See if the structural type is a 'gpml:IrregularSampling' and whether that is allowed by the GPGIM.
		if (time_dependent_flags.test(GPlatesModel::GpgimProperty::IRREGULAR_SAMPLING) &&
			structural_element_name == IRREGULAR_SAMPLING)
		{
			return read_structural_type(
					property_xml_element,
					d_irregular_sampling_structural_reader_function,
					reader_params);
		}

		// See if the structural type is a 'gpml:PiecewiseAggregation' and whether that is allowed by the GPGIM.
		if (time_dependent_flags.test(GPlatesModel::GpgimProperty::PIECEWISE_AGGREGATION) &&
			structural_element_name == PIECEWISE_AGGREGATION)
		{
			return read_structural_type(
					property_xml_element,
					d_piecewise_aggregation_structural_reader_function,
					reader_params);
		}

		// The property was wrapped in the wrong type of time-dependent structure, so attempt to fix that.

		// If the structural type is a 'gpml:ConstantValue', and 'gpml:PiecewiseAggregation' is
		// allowed by the GPGIM, then wrap the 'gpml:ConstantValue' in a 'gpml:PiecewiseAggregation'.
		if (time_dependent_flags.test(GPlatesModel::GpgimProperty::PIECEWISE_AGGREGATION) &&
			structural_element_name == CONSTANT_VALUE)
		{
			// Read/interpret the 'gpml:ConstantValue' structural element.
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> constant_value_property_value =
					read_structural_type(
							property_xml_element,
							d_constant_value_structural_reader_function,
							reader_params);
			if (!constant_value_property_value)
			{
				// Failed to read/interpret the 'gpml:ConstantValue' property.
				// Warning message already emitted by 'read_structural_type()'.
				return boost::none;
			}

			// Wrap the 'gpml:ConstantValue' property value into a 'gpml:PiecewiseAggregation'.
			const GPlatesModel::PropertyValue::non_null_ptr_type piecewise_aggregration_property_value =
					GPlatesModel::ModelUtils::create_gpml_piecewise_aggregation(
							GPlatesUtils::dynamic_pointer_cast<
									GPlatesPropertyValues::GpmlConstantValue>(
											constant_value_property_value.get()));

			// Although the property was wrapped in the wrong type of time-dependent structure,
			// we were able to fix it.
			append_warning(structural_xml_element.get(), reader_params,
					ReadErrors::IncorrectTimeDependentPropertyStructuralElementFound,
					ReadErrors::PropertyConvertedBetweenTimeDependentTypes);

			return piecewise_aggregration_property_value;
		}

		// Property was wrapped in the wrong type of time-dependent structure, and we were unable to fix it.
		append_warning(structural_xml_element.get(), reader_params,
				ReadErrors::IncorrectTimeDependentPropertyStructuralElementFound, ReadErrors::PropertyNotInterpreted);

		// Failed to read/interpret the property.
		return boost::none;
	}

	// If we get here then the property is expected to *not* be a time-dependent property...

	// Only if the property contains a child XML *element* node (and hence has a structural type name)
	// can we verify that the structural type specified in the GPML file is allowed by the GPGIM.
	if (structural_xml_element)
	{
		const GPlatesModel::XmlElementName &structural_element_name = structural_xml_element.get()->get_name();

		// See if the structural element is a time-dependent wrapper.
		if (structural_element_name == CONSTANT_VALUE ||
			structural_element_name == IRREGULAR_SAMPLING ||
			structural_element_name == PIECEWISE_AGGREGATION)
		{
			// The property is wrapped in a time-dependent structure (but it should not have been).
			// So attempt to unwrap it.
			// This method handles warning messages and can return success or failure.
			// If it fails then we have no remaining options, so we always return.
			return convert_time_dependent_wrapped_to_unwrapped_structural_type(
					property_xml_element,
					structural_xml_element.get(),
					reader_params);
		}

		// Get the (non-time-dependent) structural reader function.
		// If the unwrapped structural XML element is one of the allowed structural types then interpret it.
		boost::optional<GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
				structural_reader_function = get_structural_creation_function(
						structural_xml_element.get());
		if (!structural_reader_function)
		{
			// The property's structural element is not allowed (by the GPGIM).
			append_warning(structural_xml_element.get(), reader_params,
					ReadErrors::UnexpectedPropertyStructuralElement, ReadErrors::PropertyNotInterpreted);

			// Failed to read/interpret the property.
			return boost::none;
		}

		// Read/interpret the unwrapped structural element.
		return read_structural_type(
				property_xml_element,
				structural_reader_function.get(),
				reader_params);
	}

	// The current property does not contain an XML *element* node so there's no structural type
	// specified in the GPML file and so we cannot verify that the type is allowed by the GPGIM.
	// In this case we try each structural type, specified in the GPGIM, until one does not
	// throw a reader exception - ideally there should only be one structural type specified.
	return read_unspecified_structural_type(property_xml_element, reader_params);
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesFileIO::GpmlPropertyReader::convert_unwrapped_to_time_dependent_wrapped_structural_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> structural_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	const GPlatesModel::GpgimProperty::time_dependent_flags_type &time_dependent_flags =
			d_gpgim_property->get_time_dependent_types();

	// If 'gpml:ConstantValue' or 'gpml:PiecewiseAggregation' are allowed, then fix up the
	// unwrapped property by wrapping it (preferably in a 'gpml:ConstantValue' since it better
	// conveys the constant-for-all-time equivalent of an unwrapped property).
	//
	// Note that we don't do this for 'gpml:IrregularSampling' since that involves
	// interpolating the property value which is not always defined
	// (eg, categorical property types cannot be interpolated).
	if (time_dependent_flags.test(GPlatesModel::GpgimProperty::CONSTANT_VALUE) ||
		time_dependent_flags.test(GPlatesModel::GpgimProperty::PIECEWISE_AGGREGATION))
	{
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> property_value;

		if (structural_xml_element)
		{
			// If the unwrapped structural XML element is one of the allowed structural types then interpret it.
			boost::optional<GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
					structural_reader_function = get_structural_creation_function(
							structural_xml_element.get());
			if (!structural_reader_function)
			{
				// The property's structural element is not allowed (by the GPGIM).
				append_warning(structural_xml_element.get(), reader_params,
						ReadErrors::UnexpectedPropertyStructuralElement, ReadErrors::PropertyNotInterpreted);

				// Failed to read/interpret the property.
				return boost::none;
			}

			// Read/interpret the unwrapped structural element.
			property_value = read_structural_type(
					property_xml_element,
					structural_reader_function.get(),
					reader_params);
			if (!property_value)
			{
				// Failed to read/interpret the unwrapped property.
				// Warning message already emitted by 'read_structural_type()'.
				return boost::none;
			}
		}
		else // no structural XML element...
		{
			// Read/interpret the unwrapped structural element.
			unsigned int structural_reader_type_index = 0;
			property_value = read_unspecified_structural_type(
					property_xml_element,
					reader_params,
					structural_reader_type_index);
			if (!property_value)
			{
				// Failed to read/interpret the unwrapped property.
				// Warning message already emitted by 'read_unspecified_structural_type()'.
				return boost::none;
			}
		}

		// Wrap in a 'gpml:ConstantValue'.
		GPlatesModel::PropertyValue::non_null_ptr_type wrapped_property_value =
				GPlatesModel::ModelUtils::create_gpml_constant_value(property_value.get());

		// If a 'gpml:ConstantValue' is not allowed then wrap it, in turn, into a
		// 'gpml:PiecewiseAggregation' - which must be allowed (otherwise can't get here).
		if (!time_dependent_flags.test(GPlatesModel::GpgimProperty::CONSTANT_VALUE))
		{
			wrapped_property_value = GPlatesModel::ModelUtils::create_gpml_piecewise_aggregation(
					GPlatesUtils::dynamic_pointer_cast<
							GPlatesPropertyValues::GpmlConstantValue>(wrapped_property_value));
		}

		// Although we could not find expected time-dependent wrapper we were able to add one.
		append_warning(property_xml_element, reader_params,
				ReadErrors::TimeDependentPropertyStructuralElementNotFound,
				ReadErrors::PropertyConvertedToTimeDependent);

		// Property was interpreted (after some modification).
		return wrapped_property_value;
	}

	// Could not find expected time-dependent wrapper, and could not add one.
	append_warning(property_xml_element, reader_params,
			ReadErrors::TimeDependentPropertyStructuralElementNotFound, ReadErrors::PropertyNotInterpreted);

	// Property not interpreted.
	return boost::none;
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesFileIO::GpmlPropertyReader::convert_time_dependent_wrapped_to_unwrapped_structural_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
		const GPlatesModel::XmlElementNode::non_null_ptr_type &structural_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	static const GPlatesModel::XmlElementName CONSTANT_VALUE =
			GPlatesModel::XmlElementName::create_gpml("ConstantValue");

	// If structural element is a 'gpml:ConstantValue' wrapper then remove the wrapper.
	// We don't try to remove a 'gpml:PiecewiseAggregation' or 'gpml:IrregularSampling' though
	// since they both allow the property to vary with time.
	if (structural_xml_element->get_name() == CONSTANT_VALUE)
	{
		// See if the wrapped structural type is allowed (by the GPGIM property).
		if (!get_time_dependent_wrapped_structural_type(structural_xml_element, reader_params))
		{
			// The property's structural element type is not allowed (by the GPGIM).
			append_warning(structural_xml_element, reader_params,
					ReadErrors::UnexpectedPropertyStructuralElement, ReadErrors::PropertyNotInterpreted);

			// Failed to read/interpret the property.
			return boost::none;
		}

		try
		{
			// Read/interpret the 'gpml:ConstantValue' structural element.
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value_property_value =
					GpmlPropertyStructuralTypeReaderUtils::create_gpml_constant_value(
							property_xml_element,
							*d_property_structural_type_reader,
							d_gpml_version,
							reader_params.errors);

			// The property was incorrectly wrapped in a time-dependent wrapper,
			// but the wrapper was removed.
			append_warning(structural_xml_element, reader_params,
					ReadErrors::TimeDependentPropertyStructuralElementFound,
					ReadErrors::PropertyConvertedFromTimeDependent);

			// Return the unwrapped property value (ie, without the GpmlConstantValue wrapper).
			return constant_value_property_value->value();
		}
		catch (const GpmlReaderException &exc)
		{
			append_warning(exc.location(), reader_params, exc.description(),
					ReadErrors::PropertyNotInterpreted);
		}
		catch(...)
		{
			append_warning(structural_xml_element, reader_params,
					ReadErrors::ParseError, ReadErrors::PropertyNotInterpreted);
		}

		// Property not interpreted.
		return boost::none;
	}

	// The property was incorrectly wrapped in a time-dependent wrapper, and the wrapper could not be removed.
	append_warning(structural_xml_element, reader_params,
			ReadErrors::TimeDependentPropertyStructuralElementFound, ReadErrors::PropertyNotInterpreted);

	// Property not interpreted.
	return boost::none;
}


boost::optional<GPlatesPropertyValues::StructuralType>
GPlatesFileIO::GpmlPropertyReader::get_time_dependent_wrapped_structural_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &structural_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// All time-dependent structural types have the 'valueType' property.
	static const GPlatesModel::XmlElementName VALUE_TYPE =
			GPlatesModel::XmlElementName::create_gpml("valueType");

	// Get the value-type structural element.
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> value_type_xml_element =
			structural_xml_element->get_child_by_name(VALUE_TYPE);
	if (!value_type_xml_element)
	{
		// Shouldn't happen if 'structural_xml_element' is a time-dependent structural type
		// (such as 'gpml:ConstantValue').
		return false;
	}

	// The allowed structural types.
	const GPlatesModel::GpgimProperty::structural_type_seq_type &gpgim_structural_types =
			d_gpgim_property->get_structural_types();

	try
	{
		// Get the value-type string.
		const GPlatesPropertyValues::StructuralType value_type =
				GpmlStructuralTypeReaderUtils::create_template_type_parameter_type(
						value_type_xml_element.get(),
						d_gpml_version,
						reader_params.errors);

		// See if the value-type structural type is allowed by the GPGIM.
		BOOST_FOREACH(
				const GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type &gpgim_structural_type,
				gpgim_structural_types)
		{
			if (value_type == gpgim_structural_type->get_structural_type())
			{
				return value_type;
			}
		}
	}
	catch(...)
	{
		// An exception can get thrown if the XML is malformed.
		// We won't emit a warning here since we're just testing to see if a structural type is allowed.
		// A warning will get emitted when the property value is actually read/interpreted.
	}

	return boost::none;
}


boost::optional<GPlatesFileIO::GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
GPlatesFileIO::GpmlPropertyReader::get_structural_creation_function(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &structural_xml_element) const
{
	// Iterate over the allowed structural types to determine if matches the structural type being read.
	BOOST_FOREACH(
			const StructuralReaderType &structural_reader_type,
			d_structural_reader_types)
	{
		// Look for a matching structural type.
		if (structural_xml_element->get_name() == structural_reader_type.structural_type)
		{
			return structural_reader_type.structural_reader_function;
		}
	}

	// Specified structural type is not allowed (by GPGIM).
	return boost::none;
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesFileIO::GpmlPropertyReader::read_structural_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
		const GpmlPropertyStructuralTypeReader::structural_type_reader_function_type &structural_reader_function,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// We found a match so read the property structural type into a property value.
	try
	{
		// Create the property value.
		const GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				structural_reader_function(
						property_xml_element,
						d_gpml_version,
						reader_params.errors);

		// Successfully read structural type.
		return property_value;
	}
	catch (const GpmlReaderException &exc)
	{
		append_warning(exc.location(), reader_params, exc.description(),
				ReadErrors::PropertyNotInterpreted);
	}
	catch(...)
	{
		append_warning(property_xml_element, reader_params,
				ReadErrors::ParseError, ReadErrors::PropertyNotInterpreted);
	}

	// Failed to read structural type.
	return boost::none;
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesFileIO::GpmlPropertyReader::read_unspecified_structural_type(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &property_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params,
		boost::optional<unsigned int &> structural_creation_type_index) const
{
	// The current property does not contain an XML *element* node so there's no structural type
	// specified in the GPML file and so we cannot verify that the type is allowed by the GPGIM.
	// In this case we try each structural type, specified in the GPGIM, until one does not
	// throw a reader exception - ideally there should only be one structural type specified -
	// but there might be multiple allowed types specified in the GPGIM XML document so we should
	// try them all.

	// We should always have at least one allowed structural type - if not then there's an error
	// in the GPGIM - which also shouldn't happen since the GPGIM verifies at least one recognised
	// structural type per property.
	if (d_structural_reader_types.empty())
	{
		append_warning(property_xml_element, reader_params,
				ReadErrors::UnexpectedPropertyStructuralElement, ReadErrors::PropertyNotInterpreted);

		// Failed to read/interpret the property.
		return boost::none;
	}

	// Iterate over the allowed structural types and attempt to read property using each one in turn.
	for (unsigned int structural_type_index = 0;
		structural_type_index < d_structural_reader_types.size();
		++structural_type_index)
	{
		const StructuralReaderType &structural_reader_type =
				d_structural_reader_types[structural_type_index];

		try
		{
			// Create the property value.
			const GPlatesModel::PropertyValue::non_null_ptr_type property_value =
					structural_reader_type.structural_reader_function(
							property_xml_element,
							d_gpml_version,
							reader_params.errors);

			// If the caller requested it, then let them know which structural type was successful.
			if (structural_creation_type_index)
			{
				structural_creation_type_index = structural_type_index;
			}

			// Successfully read structural type.
			return property_value;
		}
		catch (const GpmlReaderException &exc)
		{
			// If this is the last structural type attempted then report its warning.
			// We don't want to report a warning until we've tried all structural types and failed.
			// It's a bit arbitrary as to which structural type warning we report (we choose the last one).
			if (structural_type_index == d_structural_reader_types.size() - 1)
			{
				append_warning(exc.location(), reader_params, exc.description(),
						ReadErrors::PropertyNotInterpreted);
			}
		}
		catch(...)
		{
			// If this is the last structural type attempted then report its warning.
			// We don't want to report a warning until we've tried all structural types and failed.
			// It's a bit arbitrary as to which structural type warning we report (we choose the last one).
			if (structural_type_index == d_structural_reader_types.size() - 1)
			{
				append_warning(property_xml_element, reader_params,
						ReadErrors::ParseError, ReadErrors::PropertyNotInterpreted);
			}
		}
	}

	// Failed to read/interpret the property.
	return boost::none;
}


GPlatesFileIO::GpmlPropertyReader::StructuralReaderType::StructuralReaderType(
		const GPlatesPropertyValues::StructuralType &structural_type_,
		const GpmlPropertyStructuralTypeReader::structural_type_reader_function_type &structural_reader_function_) :
	structural_type(structural_type_),
	structural_reader_function(structural_reader_function_)
{
}
