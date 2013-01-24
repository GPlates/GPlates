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

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "GpmlFeatureReaderImpl.h"
#include "GpmlReaderException.h"
#include "GpmlReaderUtils.h"
#include "GpmlStructuralTypeReaderUtils.h"

#include "model/FeatureId.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "model/RevisionId.h"
#include "model/TopLevelPropertyInline.h"
#include "model/XmlElementName.h"
#include "model/XmlNodeUtils.h"

#include "property-values/UninterpretedPropertyValue.h"

GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlFeatureCreator::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	static const GPlatesModel::XmlElementName FEATURE_ID =
			GPlatesModel::XmlElementName::create_gpml("identity");
	static const GPlatesModel::XmlElementName REVISION_ID =
			GPlatesModel::XmlElementName::create_gpml("revision");

	GPlatesModel::XmlNodeUtils::XmlElementNodeExtractionVisitor feature_id_visitor(FEATURE_ID);
	GPlatesModel::XmlNodeUtils::XmlElementNodeExtractionVisitor revision_id_visitor(REVISION_ID);

	// See if the feature-id and revision-id XML nodes exist in the GPML.
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> feature_id_xml_element;
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> revision_id_xml_element;

	// Iterator over the unprocessed feature property nodes and look for the feature-id.
	for (xml_node_seq_type::iterator property_xml_node_iter = unprocessed_feature_property_xml_nodes.begin();
		property_xml_node_iter != unprocessed_feature_property_xml_nodes.end();
		++property_xml_node_iter)
	{
		feature_id_xml_element = feature_id_visitor.get_xml_element_node(*property_xml_node_iter);
		if (feature_id_xml_element)
		{
			// Remove the feature-id property node from the list of unprocessed nodes.
			unprocessed_feature_property_xml_nodes.erase(property_xml_node_iter);
			break;
		}
	}

	// Iterator over the unprocessed feature property nodes and look for the revision-id.
	for (xml_node_seq_type::iterator property_xml_node_iter = unprocessed_feature_property_xml_nodes.begin();
		property_xml_node_iter != unprocessed_feature_property_xml_nodes.end();
		++property_xml_node_iter)
	{
		revision_id_xml_element = revision_id_visitor.get_xml_element_node(*property_xml_node_iter);
		if (revision_id_xml_element)
		{
			// Remove the revision-id property node from the list of unprocessed nodes.
			unprocessed_feature_property_xml_nodes.erase(property_xml_node_iter);
			break;
		}
	}

	boost::optional<GPlatesModel::FeatureId> feature_id;
	boost::optional<GPlatesModel::RevisionId> revision_id;

	// Create the feature-id and revision-id from the XML nodes (if they exist in the GPML).

	if (feature_id_xml_element)
	{
		try
		{
			feature_id = GpmlStructuralTypeReaderUtils::create_feature_id(
					feature_id_xml_element.get(), d_gpml_version, reader_params.errors);
		}
		catch (const GpmlReaderException &exc)
		{
			append_warning(exc.location(), reader_params, exc.description(),
					ReadErrors::PropertyNotInterpreted);
		}
		catch(...)
		{
			append_warning(feature_id_xml_element.get(), reader_params,
					ReadErrors::ParseError, ReadErrors::PropertyNotInterpreted);
		}
	}

	if (revision_id_xml_element)
	{
		try
		{
			revision_id = GpmlStructuralTypeReaderUtils::create_revision_id(
					revision_id_xml_element.get(), d_gpml_version, reader_params.errors);
		}
		catch (const GpmlReaderException &exc)
		{
			append_warning(exc.location(), reader_params, exc.description(),
					ReadErrors::PropertyNotInterpreted);
		}
		catch(...)
		{
			append_warning(revision_id_xml_element.get(), reader_params,
					ReadErrors::ParseError, ReadErrors::PropertyNotInterpreted);
		}
	}

	//
	// Create a new feature.
	//

	const GPlatesModel::FeatureType feature_type(feature_xml_element->get_name());

	if (feature_id && revision_id)
	{
		return GPlatesModel::FeatureHandle::create(
				feature_type,
				feature_id.get(),
				revision_id.get());
	}

	if (feature_id)
	{
		return GPlatesModel::FeatureHandle::create(
				feature_type,
				feature_id.get());
	}

	// Without a feature ID, a revision ID is meaningless.
	// So, even if we have a revision ID, if we don't have a feature ID, then regenerate both.
	return GPlatesModel::FeatureHandle::create(feature_type);
}


GPlatesFileIO::GpmlFeatureReader::GpmlFeatureReader(
		const GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type &gpgim_feature_class,
		const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &parent_feature_reader,
		const GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type &property_structural_type_reader,
		const GPlatesModel::GpgimVersion &gpml_version) :
	d_parent_feature_reader(parent_feature_reader)
{
	// Get the GPGIM feature properties associated with our feature class (and not its ancestors).
	// The ancestor properties are taken care of by our parent feature reader.
	const GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type &gpgim_feature_properties =
			gpgim_feature_class->get_feature_properties_excluding_ancestor_classes();

	// Iterate over the GPGIM feature properties and generate a property reader for each one.
	d_property_readers.reserve(gpgim_feature_properties.size());
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property,
			gpgim_feature_properties)
	{
		// Add a property reader for the current GPGIM feature property.
		d_property_readers.push_back(
				GpmlPropertyReader::create(gpgim_feature_property, property_structural_type_reader, gpml_version));
	}
}


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlFeatureReader::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Get the feature reader associated with the parent GPGIM feature class to read the feature.
	GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			d_parent_feature_reader->read_feature(
					feature_xml_element,
					unprocessed_feature_property_xml_nodes,
					reader_params);

	// Iterate over the feature property readers and read each property into the feature.
	BOOST_FOREACH(
			const GpmlPropertyReader::non_null_ptr_to_const_type &property_reader,
			d_property_readers)
	{
		// Read zero, one or more (depending on GPGIM) property values for the current property.
		//
		// NOTE: We call this even if there are no unprocessed properties left - this gives
		// each GPGIM property a chance to report a missing feature property.
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> property_values;
		property_reader->read_properties(
				property_values,
				feature_xml_element,
				unprocessed_feature_property_xml_nodes,
				reader_params);

		// Even if there is more than one property value read, they will still have the same property name.
		const GPlatesModel::PropertyName &property_name = property_reader->get_property_name();

		// Add each property value to the feature.
		BOOST_FOREACH(
				const GPlatesModel::PropertyValue::non_null_ptr_type &property_value,
				property_values)
		{
			// Top-level properties which also contain xml attributes
			// may be having their attributes read twice (at both the property
			// level, and here). To attempt to get round this, do not read
			// xml attributes at the top level.
			//
			// If this turns out to cause problems with other property types
			// we will have to find another solution.
			//
			// A similar modification has been made in the GpmlOutputVisitor - see
			// GpmlOutputVisitor::visit_top_level_property_inline.
			feature->add(GPlatesModel::TopLevelPropertyInline::create(property_name, property_value));
		}
	}

	return feature;
}


GPlatesFileIO::GpmlUninterpretedFeatureReader::GpmlUninterpretedFeatureReader(
		const GpmlFeatureReaderImpl::non_null_ptr_to_const_type &feature_reader) :
	d_feature_reader(feature_reader)
{
}


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlUninterpretedFeatureReader::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		xml_node_seq_type &unprocessed_feature_property_xml_nodes,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Read the feature.
	GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			d_feature_reader->read_feature(
					feature_xml_element,
					unprocessed_feature_property_xml_nodes,
					reader_params);

	//
	// Read all unprocessed feature properties as 'UninterpretedPropertyValue' property values.
	//
	UninterpretedPropertyValueCreator visitor(feature, reader_params);
	std::for_each(
			unprocessed_feature_property_xml_nodes.begin(),
			unprocessed_feature_property_xml_nodes.end(),
			boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, visitor));

	// We've processed all property XML nodes so clear the list of unprocessed nodes.
	unprocessed_feature_property_xml_nodes.clear();

	return feature;
}


GPlatesFileIO::GpmlUninterpretedFeatureReader::UninterpretedPropertyValueCreator::UninterpretedPropertyValueCreator(
		const GPlatesModel::FeatureHandle::non_null_ptr_type &feature,
		GpmlReaderUtils::ReaderParams &reader_params) :
	d_feature(feature),
	d_reader_params(reader_params)
{
}


void
GPlatesFileIO::GpmlUninterpretedFeatureReader::UninterpretedPropertyValueCreator::visit_text_node(
		const GPlatesModel::XmlTextNode::non_null_ptr_type &xml_text_node)
{
	static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("unnamed-element");

	// We shouldn't get here for a well-structured GPML file because all feature properties should
	// be structural types.
	// But if we do then we'll wrap the XML text node into an XML element node.
	GPlatesModel::XmlElementNode::non_null_ptr_type xml_element_node =
			GPlatesModel::XmlElementNode::create(
					xml_text_node,
					GPlatesModel::XmlElementName(property_name));

	// Read the property value uninterpreted.
	GPlatesModel::PropertyValue::non_null_ptr_type property_value =
			GPlatesPropertyValues::UninterpretedPropertyValue::create(xml_element_node);

	// Add the property value to the feature.
	d_feature->add(GPlatesModel::TopLevelPropertyInline::create(property_name, property_value));

	// The property name was not recognised by our delegate feature reader because it is not allowed by the GPGIM.
	// So we append this warning to the read errors.
	//
	// Note that the feature reader will emit warnings for property names that it *does* recognise
	// (that are allowed by the GPGIM) but that were read as 'UninterpretedPropertyValue's
	// (because the feature reader was unable to process them).
	// But the feature reader ignores property names it doesn't recognised which is why we
	// need to emit a warning.
	append_warning(xml_text_node, d_reader_params,
			ReadErrors::PropertyNameNotRecognisedInFeatureType, ReadErrors::PropertyNotInterpreted);
}


void
GPlatesFileIO::GpmlUninterpretedFeatureReader::UninterpretedPropertyValueCreator::visit_element_node(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &xml_element_node)
{
	// Note that we don't exclude the feature-id and revision-id special-purpose properties that are
	// in every GPlates feature because they should already have been processed by 'd_feature_creator'.

	// Read the property value uninterpreted.
	GPlatesModel::PropertyValue::non_null_ptr_type property_value =
			GPlatesPropertyValues::UninterpretedPropertyValue::create(xml_element_node);

	const GPlatesModel::PropertyName property_name(xml_element_node->get_name());

	// Add the property value to the feature.
	d_feature->add(GPlatesModel::TopLevelPropertyInline::create(property_name, property_value));

	// The property name was not recognised by our delegate feature reader because it is not allowed by the GPGIM.
	// So we append this warning to the read errors.
	//
	// Note that the feature reader will emit warnings for property names that it *does* recognise
	// (that are allowed by the GPGIM) but that were read as 'UninterpretedPropertyValue's
	// (because the feature reader was unable to process them).
	// But the feature reader ignores property names it doesn't recognised which is why we
	// need to emit a warning.
	append_warning(xml_element_node, d_reader_params,
			ReadErrors::PropertyNameNotRecognisedInFeatureType, ReadErrors::PropertyNotInterpreted);
}
