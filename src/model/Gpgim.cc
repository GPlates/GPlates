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

#include <map>
#include <vector>
#include <QFile>
#include <QXmlStreamReader>

#include "Gpgim.h"

#include "GpgimInitialisationException.h"
#include "PropertyName.h"
#include "XmlElementName.h"
#include "XmlNode.h"
#include "XmlNodeUtils.h"

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "property-values/StructuralType.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/XmlNamespaces.h"

#include <boost/foreach.hpp>

namespace GPlatesModel
{
	namespace
	{
		/**
		 * Returns true if the given @a namespaceUri and @a name match @a reader.namespaceUri()
		 * and @a reader.name(), false otherwise.
		 */
		bool
		qualified_names_are_equal(
				const QXmlStreamReader &reader,
				const QString &namespaceUri,
				const QString &name)
		{
			return (reader.namespaceUri() == namespaceUri) && (reader.name() == name);
		}


		/**
		 * Returns the qualified name from the text in the specified XML element node, or throws exception.
		 */
		template <class QualifiedXmlNameType>
		QualifiedXmlNameType
		get_qualified_xml_name(
				const XmlElementNode::non_null_ptr_type &xml_element,
				const QString &gpgim_resource_filename)
		{
			boost::optional<QualifiedXmlNameType> qualified_xml_name =
					XmlNodeUtils::get_qualified_xml_name<QualifiedXmlNameType>(xml_element);
			if (!qualified_xml_name)
			{
				throw GpgimInitialisationException(
						GPLATES_EXCEPTION_SOURCE,
						gpgim_resource_filename,
						xml_element->line_number(),
						QString("unable to get qualified XML name from XML element '%1'")
								.arg(convert_qualified_xml_name_to_qstring(xml_element->get_name())));
			}

			return qualified_xml_name.get();
		}


		/**
		 * Returns the text string in the specified XML element node, or throws exception.
		 */
		QString
		get_text(
				const XmlElementNode::non_null_ptr_type &xml_element,
				const QString &gpgim_resource_filename)
		{
			boost::optional<QString> text = XmlNodeUtils::get_text(xml_element);
			if (!text)
			{
				throw GpgimInitialisationException(
						GPLATES_EXCEPTION_SOURCE,
						gpgim_resource_filename,
						xml_element->line_number(),
						QString("unable to get text string from XML element '%1'")
								.arg(convert_qualified_xml_name_to_qstring(xml_element->get_name())));
			}

			return text.get();
		}


		/**
		 * Find zero or one child elements, of @a xml_element, with element name @a child_xml_element_name.
		 *
		 * If more than one child element is found then an exception is thrown.
		 */
		boost::optional<XmlElementNode::non_null_ptr_type>
		find_zero_or_one_child_xml_elements(
				const XmlElementNode::non_null_ptr_type &xml_element,
				const XmlElementName &child_xml_element_name,
				const QString &gpgim_resource_filename)
		{
			XmlElementNode::named_child_const_iterator 
					child_xml_element_iter = xml_element->get_next_child_by_name(
							child_xml_element_name,
							xml_element->children_begin());
			if (child_xml_element_iter.first == xml_element->children_end())
			{
				return boost::none;
			}

			const XmlElementNode::non_null_ptr_type child_xml_element =
					child_xml_element_iter.second.get();

			// Make sure a duplicate element, with same element name, is not found.
			child_xml_element_iter = xml_element->get_next_child_by_name(
					child_xml_element_name,
					++child_xml_element_iter.first);
			if (child_xml_element_iter.first != xml_element->children_end())
			{
				// Found duplicate!
				throw GpgimInitialisationException(
						GPLATES_EXCEPTION_SOURCE,
						gpgim_resource_filename,
						child_xml_element_iter.second.get()->line_number(),
						QString("duplicate '%1' element found")
								.arg(convert_qualified_xml_name_to_qstring(child_xml_element_name)));
			}

			return child_xml_element;
		}


		/**
		 * Find exactly one child element, of @a xml_element, with element name @a child_xml_element_name.
		 *
		 * If not exactly one child element is found then an exception is thrown.
		 */
		XmlElementNode::non_null_ptr_type
		find_one_child_xml_element(
				const XmlElementNode::non_null_ptr_type &xml_element,
				const XmlElementName &child_xml_element_name,
				const QString &gpgim_resource_filename)
		{
			boost::optional<XmlElementNode::non_null_ptr_type> child_xml_element =
					find_zero_or_one_child_xml_elements(
							xml_element,
							child_xml_element_name,
							gpgim_resource_filename);
			if (!child_xml_element)
			{
				throw GpgimInitialisationException(
						GPLATES_EXCEPTION_SOURCE,
						gpgim_resource_filename,
						xml_element->line_number(),
						QString("'%1' element not found in element '%2'")
								.arg(convert_qualified_xml_name_to_qstring(child_xml_element_name))
								.arg(convert_qualified_xml_name_to_qstring(xml_element->get_name())));
			}

			return child_xml_element.get();
		}


		/**
		 * Find zero or more child elements, of @a xml_element, with element name @a child_xml_element_name.
		 */
		void
		find_zero_or_more_child_xml_elements(
				std::vector<XmlElementNode::non_null_ptr_type> &child_xml_elements,
				const XmlElementNode::non_null_ptr_type &xml_element,
				const XmlElementName &child_xml_element_name,
				const QString &gpgim_resource_filename)
		{
			XmlElementNode::named_child_const_iterator 
					child_xml_element_iter = xml_element->get_next_child_by_name(
							child_xml_element_name,
							xml_element->children_begin());

			while (child_xml_element_iter.first != xml_element->children_end())
			{
				const XmlElementNode::non_null_ptr_type child_xml_element =
						child_xml_element_iter.second.get();

				child_xml_elements.push_back(child_xml_element);

				// Increment the iterator.
				child_xml_element_iter =
						xml_element->get_next_child_by_name(
								child_xml_element_name,
								++child_xml_element_iter.first);
			}
		}


		/**
		 * Find one or more child elements, of @a xml_element, with element name @a child_xml_element_name.
		 *
		 * If no child elements are found then an exception is thrown.
		 */
		void
		find_one_or_more_child_xml_elements(
				std::vector<XmlElementNode::non_null_ptr_type> &child_xml_elements,
				const XmlElementNode::non_null_ptr_type &xml_element,
				const XmlElementName &child_xml_element_name,
				const QString &gpgim_resource_filename)
		{
			const unsigned int initial_child_xml_elements_size = child_xml_elements.size();

			find_zero_or_more_child_xml_elements(
					child_xml_elements,
					xml_element,
					child_xml_element_name,
					gpgim_resource_filename);

			// If no child elements were found then throw an exception.
			if (child_xml_elements.size() == initial_child_xml_elements_size)
			{
				throw GpgimInitialisationException(
						GPLATES_EXCEPTION_SOURCE,
						gpgim_resource_filename,
						xml_element->line_number(),
						QString("'%1' element not found in element '%2'")
								.arg(convert_qualified_xml_name_to_qstring(child_xml_element_name))
								.arg(convert_qualified_xml_name_to_qstring(xml_element->get_name())));
			}
		}
	}
}


const QString GPlatesModel::Gpgim::DEFAULT_GPGIM_RESOURCE_FILENAME = ":/gpgim/gpgim.xml";


GPlatesModel::Gpgim::Gpgim(
		const QString &gpgim_resource_filename)
{
	QXmlStreamReader xml_reader;

	QFile input_file(gpgim_resource_filename);
	if (!input_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename);
	}
	xml_reader.setDevice(&input_file);

	boost::shared_ptr<XmlElementNode::AliasToNamespaceMap> alias_to_namespace_map(
			new XmlElementNode::AliasToNamespaceMap);

	boost::optional<XmlElementNode::non_null_ptr_type> property_type_list_xml_element;
	boost::optional<XmlElementNode::non_null_ptr_type> property_list_xml_element;
	boost::optional<XmlElementNode::non_null_ptr_type> feature_class_list_xml_element;

	// Read the root GPGIM element and return the GPGIM version.
	d_version = read_gpgim_element(
			property_type_list_xml_element,
			property_list_xml_element,
			feature_class_list_xml_element,
			xml_reader,
			gpgim_resource_filename,
			alias_to_namespace_map);

	if (xml_reader.error())
	{
		// The XML was malformed somewhere along the line.
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("XML parse error"));
	}

	// Create the property structural types.
	// NOTE: We do this before creating the properties since they refer to the
	// property structural types we create here.
	create_property_structural_types(property_type_list_xml_element.get(), gpgim_resource_filename);

	// Create the properties.
	// NOTE: We do this before creating the feature classes since they refer to the
	// properties we create here.
	create_properties(property_list_xml_element.get(), gpgim_resource_filename);

	// Read the GPGIM feature class XML elements into a temporary
	// "feature-type -> XML-element" map so we can process the feature classes
	// in a different order than they appear in the XML file - we follow the feature class
	// inheritance chain order as we encounter each unprocessed feature class.
	feature_class_xml_element_node_map_type feature_class_xml_element_node_map;
	read_feature_class_xml_elements(
			feature_class_xml_element_node_map,
			feature_class_list_xml_element.get(),
			gpgim_resource_filename);

	// Create the feature classes.
	create_feature_classes(feature_class_xml_element_node_map, gpgim_resource_filename);
}


const GPlatesModel::GpgimVersion &
GPlatesModel::Gpgim::get_version() const
{
	// Constructor should always set the GPGIM version.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_version,
			GPLATES_ASSERTION_SOURCE);

	return d_version.get();
}


boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type>
GPlatesModel::Gpgim::get_feature_class(
		const FeatureType &feature_type) const
{
	feature_class_map_type::const_iterator iter = d_feature_class_map.find(feature_type);
	if (iter == d_feature_class_map.end())
	{
		return boost::none;
	}

	return iter->second;
}


boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
GPlatesModel::Gpgim::get_feature_property(
		const FeatureType &feature_type,
		const PropertyName &property_name) const
{
	// Get the feature class from the feature type.
	boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
			get_feature_class(feature_type);
	if (!gpgim_feature_class)
	{
		// The feature type was not recognised.
		return boost::none;
	}

	// Query the feature class.
	return gpgim_feature_class.get()->get_feature_property(property_name);
}


bool
GPlatesModel::Gpgim::get_feature_properties(
		const FeatureType &feature_type,
		const GPlatesPropertyValues::StructuralType &property_type,
		boost::optional<property_seq_type &> feature_properties) const
{
	// Get the feature class from the feature type.
	boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
			get_feature_class(feature_type);
	if (!gpgim_feature_class)
	{
		// The feature type was not recognised.
		return false;
	}

	// Query the feature class.
	return gpgim_feature_class.get()->get_feature_properties(property_type, feature_properties);
}


boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
GPlatesModel::Gpgim::get_property(
		const PropertyName &property_name) const
{
	property_map_type::const_iterator iter =
			d_property_map.find(property_name);
	if (iter == d_property_map.end())
	{
		return boost::none;
	}

	return iter->second;
}


boost::optional<GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type>
GPlatesModel::Gpgim::get_property_structural_type(
		const GPlatesPropertyValues::StructuralType &structural_type) const
{
	property_structural_type_map_type::const_iterator iter =
			d_property_structural_type_map.find(structural_type);
	if (iter == d_property_structural_type_map.end())
	{
		return boost::none;
	}

	return iter->second;
}


boost::optional<GPlatesModel::GpgimEnumerationType::non_null_ptr_to_const_type>
GPlatesModel::Gpgim::get_property_enumeration_type(
		const GPlatesPropertyValues::StructuralType &structural_type) const
{
	property_enumeration_type_map_type::const_iterator iter =
			d_property_enumeration_type_map.find(structural_type);
	if (iter == d_property_enumeration_type_map.end())
	{
		return boost::none;
	}

	return iter->second;
}


GPlatesModel::GpgimVersion
GPlatesModel::Gpgim::read_gpgim_element(
		boost::optional<XmlElementNode::non_null_ptr_type> &property_type_list_xml_element,
		boost::optional<XmlElementNode::non_null_ptr_type> &property_list_xml_element,
		boost::optional<XmlElementNode::non_null_ptr_type> &feature_class_list_xml_element,
		QXmlStreamReader &xml_reader,
		const QString &gpgim_resource_filename,
		boost::shared_ptr<XmlElementNode::AliasToNamespaceMap> alias_to_namespace_map)
{
	// The XML element name for the root element in the GPGIM XML file.
	static const XmlElementName GPGIM_XML_ELEMENT_NAME = XmlElementName::create_gpgim("GPGIM");

	if (xml_reader.atEnd())
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				"failed to find root XML element");
	}

	// Find the start of the next XML element node.
	while (!xml_reader.atEnd())
	{
		xml_reader.readNext();
		if (xml_reader.isStartElement())
		{
			break;
		}
	}

	// Did not find the start of the next XML element node.
	if (xml_reader.atEnd())
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				"failed to find root XML element");
	}

	QXmlStreamNamespaceDeclarations ns_decls = xml_reader.namespaceDeclarations();
	QXmlStreamNamespaceDeclarations::iterator iter = ns_decls.begin();
	QXmlStreamNamespaceDeclarations::iterator end = ns_decls.end();
	for ( ; iter != end; ++ iter)
	{
		alias_to_namespace_map->insert(
				std::make_pair(
					iter->prefix().toString(),
					iter->namespaceUri().toString()));
	}

	const XmlElementName gpgim_element(
			xml_reader.namespaceUri().toString(), 
			xml_reader.name().toString());

	if (gpgim_element != GPGIM_XML_ELEMENT_NAME)
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the GPGIM document root element was not a '%1'")
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME)));
	}

	//
	// Determine the GPGIM version.
	//

	const QStringRef gpgim_version_string = xml_reader.attributes().value(
			GPlatesUtils::XmlNamespaces::GPGIM_NAMESPACE_QSTRING, "version");

	boost::optional<GpgimVersion> gpgim_version = GpgimVersion::create(gpgim_version_string.toString());
	if (!gpgim_version)
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				"failed to read a valid GPGIM version");
	}

	//
	// Read the list of property types.
	//

	// The XML element name for the property type list in the GPGIM XML file.
	static const XmlElementName PROPERTY_TYPE_LIST_ELEMENT_NAME = XmlElementName::create_gpgim("PropertyTypeList");

	// Find the start of the next XML element node.
	while (!xml_reader.atEnd())
	{
		xml_reader.readNext();
		if (xml_reader.isStartElement())
		{
			break;
		}
	}

	// Did not find the start of the next XML element node.
	if (xml_reader.atEnd())
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the '%1' child XML element of '%2' is missing" )
						.arg(convert_qualified_xml_name_to_qstring(PROPERTY_TYPE_LIST_ELEMENT_NAME))
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME)));
	}

	if (!qualified_names_are_equal(
		xml_reader,
		GPlatesUtils::XmlNamespaces::GPGIM_NAMESPACE_QSTRING,
		GPlatesUtils::make_qstring_from_icu_string(PROPERTY_TYPE_LIST_ELEMENT_NAME.get_name())))
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the element '%1' inside the '%2' is expected to be '%3'" )
						.arg(xml_reader.qualifiedName().toString())
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME))
						.arg(convert_qualified_xml_name_to_qstring(PROPERTY_TYPE_LIST_ELEMENT_NAME)));
	}

	// Create the XML element for the property type list.
	property_type_list_xml_element = XmlElementNode::create(xml_reader, alias_to_namespace_map);

	//
	// Read the list of properties.
	//

	// The XML element name for the property list in the GPGIM XML file.
	static const XmlElementName PROPERTY_LIST_ELEMENT_NAME = XmlElementName::create_gpgim("PropertyList");

	// Find the start of the next XML element node.
	while (!xml_reader.atEnd())
	{
		xml_reader.readNext();
		if (xml_reader.isStartElement())
		{
			break;
		}
	}

	// Did not find the start of the next XML element node.
	if (xml_reader.atEnd())
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the '%1' child XML element of '%2' is missing" )
						.arg(convert_qualified_xml_name_to_qstring(PROPERTY_LIST_ELEMENT_NAME))
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME)));
	}

	if (!qualified_names_are_equal(
		xml_reader,
		GPlatesUtils::XmlNamespaces::GPGIM_NAMESPACE_QSTRING,
		GPlatesUtils::make_qstring_from_icu_string(PROPERTY_LIST_ELEMENT_NAME.get_name())))
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the element '%1' inside the '%2' is expected to be '%3'" )
						.arg(xml_reader.qualifiedName().toString())
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME))
						.arg(convert_qualified_xml_name_to_qstring(PROPERTY_LIST_ELEMENT_NAME)));
	}

	// Create the XML element for the property list.
	property_list_xml_element = XmlElementNode::create(xml_reader, alias_to_namespace_map);

	//
	// Read the list of feature classes.
	//

	// The XML element name for the feature class list in the GPGIM XML file.
	static const XmlElementName FEATURE_CLASS_LIST_ELEMENT_NAME = XmlElementName::create_gpgim("FeatureClassList");

	// Find the start of the next XML element node.
	while (!xml_reader.atEnd())
	{
		xml_reader.readNext();
		if (xml_reader.isStartElement())
		{
			break;
		}
	}

	// Did not find the start of the next XML element node.
	if (xml_reader.atEnd())
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the '%1' child XML element of '%2' is missing" )
						.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_LIST_ELEMENT_NAME))
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME)));
	}

	if (!qualified_names_are_equal(
		xml_reader,
		GPlatesUtils::XmlNamespaces::GPGIM_NAMESPACE_QSTRING,
		GPlatesUtils::make_qstring_from_icu_string(FEATURE_CLASS_LIST_ELEMENT_NAME.get_name())))
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				xml_reader.lineNumber(),
				QString("the element '%1' inside the '%2' is expected to be '%3'" )
						.arg(xml_reader.qualifiedName().toString())
						.arg(convert_qualified_xml_name_to_qstring(GPGIM_XML_ELEMENT_NAME))
						.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_LIST_ELEMENT_NAME)));
	}

	// Create the XML element for the feature class list.
	feature_class_list_xml_element = XmlElementNode::create(xml_reader, alias_to_namespace_map);

	return gpgim_version.get();
}


void
GPlatesModel::Gpgim::create_property_structural_types(
		const XmlElementNode::non_null_ptr_type &property_type_list_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for the property type list element in the GPGIM XML file.
	static const XmlElementName PROPERTY_TYPE_LIST_ELEMENT_NAME = XmlElementName::create_gpgim("PropertyTypeList");

	// The XML element name for enumeration elements in the GPGIM XML file.
	static const XmlElementName ENUMERATION_ELEMENT_NAME = XmlElementName::create_gpgim("Enumeration");
	// The XML element name for native property elements in the GPGIM XML file.
	static const XmlElementName NATIVE_PROPERTY_ELEMENT_NAME = XmlElementName::create_gpgim("NativeProperty");

	// Iterate over the property types.
	for (XmlElementNode::child_const_iterator property_type_xml_node_iter = property_type_list_xml_element->children_begin();
		property_type_xml_node_iter != property_type_list_xml_element->children_end();
		++property_type_xml_node_iter)
	{
		// See if the XML node is an XML element.
		XmlNodeUtils::XmlElementNodeExtractionVisitor xml_element_node_extraction_visitor;
		boost::optional<XmlElementNode::non_null_ptr_type> property_type_xml_element_opt =
				xml_element_node_extraction_visitor.get_xml_element_node(
						*property_type_xml_node_iter);
		if (!property_type_xml_element_opt)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_type_list_xml_element->line_number(),
					QString("the '%1' element should only contain '%2' and '%3' elements, not text" )
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_TYPE_LIST_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(ENUMERATION_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(NATIVE_PROPERTY_ELEMENT_NAME)));
		}
		XmlElementNode::non_null_ptr_type property_type_xml_element =
				property_type_xml_element_opt.get();

		// Make sure there's only property type elements in the property type list element.
		if (property_type_xml_element->get_name() != ENUMERATION_ELEMENT_NAME &&
			property_type_xml_element->get_name() != NATIVE_PROPERTY_ELEMENT_NAME)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_type_xml_element.get()->line_number(),
					QString("the element '%1' inside the '%2' is not '%3' or '%4'" )
							.arg(convert_qualified_xml_name_to_qstring(property_type_xml_element.get()->get_name()))
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_TYPE_LIST_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(ENUMERATION_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(NATIVE_PROPERTY_ELEMENT_NAME)));
		}

		// The 'Enumeration' property type contains extra data - the allowed enumeration values.
		const bool is_enumeration = (property_type_xml_element->get_name() == ENUMERATION_ELEMENT_NAME);

		// Create the GPGIM property structural type.
		const GpgimStructuralType::non_null_ptr_to_const_type gpgim_structural_type =
				create_property_structural_type(
						property_type_xml_element,
						is_enumeration,
						gpgim_resource_filename);

		// Add to our mapping of structural type to GPGIM property structural type.
		// Make sure the same structural type does not appear more than once.
		if (!d_property_structural_type_map.insert(
			std::make_pair(gpgim_structural_type->get_structural_type(), gpgim_structural_type)).second)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_type_xml_element->line_number(),
					QString("duplicate property structural type '%1'")
							.arg(convert_qualified_xml_name_to_qstring(gpgim_structural_type->get_structural_type())));
		}

		// Add to the list of GPGIM property structural types.
		d_property_structural_types.push_back(gpgim_structural_type);

		// If it's an enumeration (structural type) then also add it to our list of GPGIM enumeration types.
		if (is_enumeration)
		{
			const GpgimEnumerationType::non_null_ptr_to_const_type gpgim_enumeration_type =
					GPlatesUtils::dynamic_pointer_cast<const GpgimEnumerationType>(gpgim_structural_type);

			// Add to the list of enumeration types.
			d_property_enumeration_types.push_back(gpgim_enumeration_type);

			// Also insert into the map of structural types to GPGIM enumerations.
			d_property_enumeration_type_map.insert(
					std::make_pair(gpgim_enumeration_type->get_structural_type(), gpgim_enumeration_type));
		}
	}
}


GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type
GPlatesModel::Gpgim::create_property_structural_type(
		const XmlElementNode::non_null_ptr_type &property_type_xml_element,
		bool is_enumeration,
		const QString &gpgim_resource_filename)
{
	// The XML element name for the structural type of a property in the GPGIM XML file.
	static const XmlElementName STRUCTURAL_TYPE_ELEMENT_NAME = XmlElementName::create_gpgim("Type");
	// The XML element name for the structural description of a property in the GPGIM XML file.
	static const XmlElementName STRUCTURAL_DESCRIPTION_ELEMENT_NAME = XmlElementName::create_gpgim("Description");

	// Look for the structural type element.
	// Both 'Enumeration' and 'NativeProperty' have one.
	const XmlElementNode::non_null_ptr_type structural_type_element =
			find_one_child_xml_element(
					property_type_xml_element,
					STRUCTURAL_TYPE_ELEMENT_NAME,
					gpgim_resource_filename);
	// Get the structural type qualified name.
	const GPlatesPropertyValues::StructuralType structural_type =
			get_qualified_xml_name<GPlatesPropertyValues::StructuralType>(
					structural_type_element,
					gpgim_resource_filename);

	// Look for the structural description element.
	// Both 'Enumeration' and 'NativeProperty' have one.
	const XmlElementNode::non_null_ptr_type structural_description_element =
			find_one_child_xml_element(
					property_type_xml_element,
					STRUCTURAL_DESCRIPTION_ELEMENT_NAME,
					gpgim_resource_filename);
	// Get the structural description.
	const QString structural_description =
			get_text(structural_description_element, gpgim_resource_filename);

	if (!is_enumeration)
	{
		// Create the GPGIM property structural type.
		return GpgimStructuralType::create(structural_type, structural_description);
	}
	// ...else the 'Enumeration' property type contains extra data - the allowed enumeration values.

	GpgimEnumerationType::content_seq_type enumeration_contents;

	// The XML element name for the structural description of a property in the GPGIM XML file.
	static const XmlElementName CONTENT_ELEMENT_NAME = XmlElementName::create_gpgim("Content");

	// Look for the content elements.
	// There should be at least one of these per enumeration type.
	std::vector<XmlElementNode::non_null_ptr_type> content_elements;
	find_one_or_more_child_xml_elements(
			content_elements,
			property_type_xml_element.get(),
			CONTENT_ELEMENT_NAME,
			gpgim_resource_filename);

	// Iterate over the content elements.
	BOOST_FOREACH(
			const XmlElementNode::non_null_ptr_type &content_element,
			content_elements)
	{
		// The XML element name for the content value of an enumeration in the GPGIM XML file.
		static const XmlElementName CONTENT_VALUE_ELEMENT_NAME = XmlElementName::create_gpgim("Value");
		// The XML element name for the content description of an enumeration in the GPGIM XML file.
		static const XmlElementName CONTENT_DESCRIPTION_ELEMENT_NAME = XmlElementName::create_gpgim("Description");

		// Look for the content value element.
		const XmlElementNode::non_null_ptr_type content_value_element =
				find_one_child_xml_element(
						content_element,
						CONTENT_VALUE_ELEMENT_NAME,
						gpgim_resource_filename);
		// Get the content value.
		const QString content_value = get_text(content_value_element, gpgim_resource_filename);

		// Look for the content description element.
		const XmlElementNode::non_null_ptr_type content_description_element =
				find_one_child_xml_element(
						content_element,
						CONTENT_DESCRIPTION_ELEMENT_NAME,
						gpgim_resource_filename);
		// Get the content description.
		const QString content_description = get_text(content_description_element, gpgim_resource_filename);

		// Add the content to our list.
		enumeration_contents.push_back(
				GpgimEnumerationType::Content(content_value, content_description));
	}

	// Create the GPGIM property enumeration (structural) type.
	GpgimEnumerationType::non_null_ptr_to_const_type gpgim_enumeration_type =
			GpgimEnumerationType::create(
					structural_type,
					structural_description,
					enumeration_contents.begin(),
					enumeration_contents.end());

	return gpgim_enumeration_type;
}


void
GPlatesModel::Gpgim::create_properties(
		const XmlElementNode::non_null_ptr_type &property_list_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for the property list element in the GPGIM XML file.
	static const XmlElementName PROPERTY_LIST_ELEMENT_NAME = XmlElementName::create_gpgim("PropertyList");

	// The XML element name for property elements in the GPGIM XML file.
	static const XmlElementName PROPERTY_ELEMENT_NAME = XmlElementName::create_gpgim("Property");

	// Iterate over the properties.
	for (XmlElementNode::child_const_iterator property_xml_node_iter = property_list_xml_element->children_begin();
		property_xml_node_iter != property_list_xml_element->children_end();
		++property_xml_node_iter)
	{
		// See if the XML node is an XML element.
		XmlNodeUtils::XmlElementNodeExtractionVisitor xml_element_node_extraction_visitor;
		boost::optional<XmlElementNode::non_null_ptr_type> property_xml_element_opt =
				xml_element_node_extraction_visitor.get_xml_element_node(
						*property_xml_node_iter);
		if (!property_xml_element_opt)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_list_xml_element->line_number(),
					QString("the '%1' element should only contain '%2' elements, not text" )
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_LIST_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_ELEMENT_NAME)));
		}
		XmlElementNode::non_null_ptr_type property_xml_element =
				property_xml_element_opt.get();

		// Make sure there's only property elements in the property list element.
		if (property_xml_element->get_name() != PROPERTY_ELEMENT_NAME)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_xml_element.get()->line_number(),
					QString("the element '%1' inside the '%2' is not '%3'" )
							.arg(convert_qualified_xml_name_to_qstring(property_xml_element.get()->get_name()))
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_LIST_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_ELEMENT_NAME)));
		}

		// Create the GPGIM property.
		const GpgimProperty::non_null_ptr_type gpgim_property =
				create_property(property_xml_element, gpgim_resource_filename);

		// Add to our mapping of property name to GPGIM property.
		// Make sure the same property name does not appear more than once.
		if (!d_property_map.insert(
			std::make_pair(gpgim_property->get_property_name(), gpgim_property)).second)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_xml_element->line_number(),
					QString("duplicate property name '%1'")
							.arg(convert_qualified_xml_name_to_qstring(gpgim_property->get_property_name())));
		}

		// Add to the list of GPGIM properties.
		d_properties.push_back(gpgim_property);
	}
}


GPlatesModel::GpgimProperty::non_null_ptr_type
GPlatesModel::Gpgim::create_property(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	// Read the property name.
	const PropertyName property_name = read_feature_property_name(
			property_xml_element,
			gpgim_resource_filename);

	// Read the user-friendly name.
	const QString property_user_friendly_name = read_feature_property_user_friendly_name(
			property_xml_element,
			property_name,
			gpgim_resource_filename);

	// Read the property description.
	const QString property_description = read_feature_property_description(
			property_xml_element,
			gpgim_resource_filename);

	// Read the property multiplicity.
	const GpgimProperty::MultiplicityType property_multiplicity = read_feature_property_multiplicity(
			property_xml_element,
			gpgim_resource_filename);

	// Read the property structural types.
	GpgimProperty::structural_type_seq_type property_structural_types;
	GpgimProperty::structural_type_seq_type::size_type default_property_structural_type_index =
			read_feature_property_structural_types(
					property_structural_types,
					property_xml_element,
					gpgim_resource_filename);

	// Read the property time-dependent types.
	const GpgimProperty::time_dependent_flags_type property_time_dependent_types =
			read_feature_property_time_dependent_types(
					property_xml_element,
					gpgim_resource_filename);

	// Create the GPGIM feature property.
	return GpgimProperty::create(
			property_name,
			property_user_friendly_name,
			property_description,
			property_multiplicity,
			property_structural_types.begin(),
			property_structural_types.end(),
			default_property_structural_type_index,
			property_time_dependent_types);
}


GPlatesModel::PropertyName
GPlatesModel::Gpgim::read_feature_property_name(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property name in the GPGIM XML file.
	static const XmlElementName PROPERTY_NAME_ELEMENT_NAME = XmlElementName::create_gpgim("Name");

	// Look for the property name element.
	// There should be exactly one of this element.
	const XmlElementNode::non_null_ptr_type property_name_element =
			find_one_child_xml_element(
					property_xml_element,
					PROPERTY_NAME_ELEMENT_NAME,
					gpgim_resource_filename);

	// Get the property name.
	return get_qualified_xml_name<PropertyName>(property_name_element, gpgim_resource_filename);
}


QString
GPlatesModel::Gpgim::read_feature_property_user_friendly_name(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const PropertyName &property_name,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property user-friendly name in the GPGIM XML file.
	static const XmlElementName PROPERTY_USER_FRIENDLY_NAME_ELEMENT_NAME =
			XmlElementName::create_gpgim("UserFriendlyName");

	// Look for the property user-friendly name element.
	// This element is optional.
	boost::optional<XmlElementNode::non_null_ptr_type> property_user_friendly_name_element =
			find_zero_or_one_child_xml_elements(
					property_xml_element,
					PROPERTY_USER_FRIENDLY_NAME_ELEMENT_NAME,
					gpgim_resource_filename);
	// If there's no user-friendly name then use the local name part of the property name instead.
	if (!property_user_friendly_name_element)
	{
		return GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
	}

	// Get the string text.
	return get_text(property_user_friendly_name_element.get(), gpgim_resource_filename);
}


QString
GPlatesModel::Gpgim::read_feature_property_description(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property description in the GPGIM XML file.
	static const XmlElementName PROPERTY_DESCRIPTION_ELEMENT_NAME = XmlElementName::create_gpgim("Description");

	// Look for the property description element.
	// There should be exactly one of this element.
	const XmlElementNode::non_null_ptr_type property_description_element =
			find_one_child_xml_element(
					property_xml_element,
					PROPERTY_DESCRIPTION_ELEMENT_NAME,
					gpgim_resource_filename);

	// Get the string text.
	return get_text(property_description_element, gpgim_resource_filename);
}


GPlatesModel::GpgimProperty::MultiplicityType
GPlatesModel::Gpgim::read_feature_property_multiplicity(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property multiplicity in the GPGIM XML file.
	static const XmlElementName PROPERTY_MULTIPLICITY_ELEMENT_NAME = XmlElementName::create_gpgim("Multiplicity");

	// Look for the property multiplicity element.
	// There should be exactly one of this element.
	const XmlElementNode::non_null_ptr_type property_multiplicity_element =
			find_one_child_xml_element(
					property_xml_element,
					PROPERTY_MULTIPLICITY_ELEMENT_NAME,
					gpgim_resource_filename);

	// Get the property multiplicity string.
	const QString property_multiplicity_string =
			get_text(property_multiplicity_element, gpgim_resource_filename);

	// Determine the property multiplicity.
	GpgimProperty::MultiplicityType property_multiplicity = GpgimProperty::ZERO_OR_MORE;

	static const QString ZERO_OR_ONE = "0..1";
	static const QString ONE = "1";
	static const QString ZERO_OR_MORE = "0..*";
	static const QString ONE_OR_MORE = "1..*";
	if (property_multiplicity_string == ZERO_OR_ONE)
	{
		property_multiplicity = GpgimProperty::ZERO_OR_ONE;
	}
	else if (property_multiplicity_string == ONE)
	{
		property_multiplicity = GpgimProperty::ONE;
	}
	else if (property_multiplicity_string == ZERO_OR_MORE)
	{
		property_multiplicity = GpgimProperty::ZERO_OR_MORE;
	}
	else if (property_multiplicity_string == ONE_OR_MORE)
	{
		property_multiplicity = GpgimProperty::ONE_OR_MORE;
	}
	else
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				property_multiplicity_element->line_number(),
				QString("XML element '%1' should contain one of '%2', '%3', '%4' or '%5'")
						.arg(convert_qualified_xml_name_to_qstring(PROPERTY_MULTIPLICITY_ELEMENT_NAME))
						.arg(ZERO_OR_ONE)
						.arg(ONE)
						.arg(ZERO_OR_MORE)
						.arg(ONE_OR_MORE));
	}

	return property_multiplicity;
}


GPlatesModel::GpgimProperty::structural_type_seq_type::size_type
GPlatesModel::Gpgim::read_feature_property_structural_types(
		GpgimProperty::structural_type_seq_type &gpgim_property_structural_types,
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property type in the GPGIM XML file.
	static const XmlElementName PROPERTY_TYPE_ELEMENT_NAME = XmlElementName::create_gpgim("Type");

	// Look for property time-dependent elements.
	// There must be one or more of these elements.
	std::vector<XmlElementNode::non_null_ptr_type> property_type_elements;
	find_one_or_more_child_xml_elements(
			property_type_elements,
			property_xml_element,
			PROPERTY_TYPE_ELEMENT_NAME,
			gpgim_resource_filename);

	// The 'gpgim:defaultType' attribute is expected if more than one structural type is listed.
	boost::optional<GPlatesPropertyValues::StructuralType> default_property_structural_type;
	if (property_type_elements.size() > 1)
	{
		default_property_structural_type =
				read_default_feature_property_structural_type(property_xml_element, gpgim_resource_filename);
	}

	// Index of the default property structural type.
	boost::optional<unsigned int> default_property_structural_type_index;

	// Iterate over the property structural type elements.
	for (unsigned int property_type_index = 0;
		property_type_index < property_type_elements.size();
		++property_type_index)
	{
		const XmlElementNode::non_null_ptr_type &property_type_element =
				property_type_elements[property_type_index];

		// Get the property structural type.
		const GPlatesPropertyValues::StructuralType property_structural_type =
				get_qualified_xml_name<GPlatesPropertyValues::StructuralType>(
						property_type_element,
						gpgim_resource_filename);

		// Make sure it's a recognised property structural type.
		// Note: We've already read in the list of supported property structural types from the GPGIM XML file.
		property_structural_type_map_type::const_iterator gpgim_property_structural_type_iter =
				d_property_structural_type_map.find(property_structural_type);
		if (gpgim_property_structural_type_iter == d_property_structural_type_map.end())
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_type_element->line_number(),
					QString("'%1' is not a recognised property structural type")
							.arg(convert_qualified_xml_name_to_qstring(property_structural_type)));
		}

		// Add the property structural type to the list.
		const GpgimStructuralType::non_null_ptr_to_const_type &gpgim_property_structural_type =
				gpgim_property_structural_type_iter->second;
		gpgim_property_structural_types.push_back(gpgim_property_structural_type);

		// See if the current structural type is the default type.
		if (default_property_structural_type &&
			default_property_structural_type.get() == property_structural_type)
		{
			// Record the default type index.
			default_property_structural_type_index = property_type_index;
		}
	}

	// If we're expecting to find a default type but didn't find one...
	if (default_property_structural_type && !default_property_structural_type_index)
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				property_xml_element->line_number(),
				QString("the default structural type '%1' was not listed in the structural types")
						.arg(convert_qualified_xml_name_to_qstring(default_property_structural_type.get())));
	}

	// If we have a default property structural type index then it means there are multiple
	// structural types.
	// Otherwise there's only one structural type and we return index zero.
	return default_property_structural_type_index ? default_property_structural_type_index.get() : 0;
}


GPlatesPropertyValues::StructuralType
GPlatesModel::Gpgim::read_default_feature_property_structural_type(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	static const XmlAttributeName DEFAULT_TYPE_ATTRIBUTE_NAME = XmlAttributeName::create_gpgim("defaultType");

	// Look for the 'gpgim:defaultType' attribute.
	const XmlElementNode::attribute_const_iterator default_type_attribute_iter =
			property_xml_element->get_attribute_by_name(DEFAULT_TYPE_ATTRIBUTE_NAME);
	if (default_type_attribute_iter == property_xml_element->attributes_end())
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				property_xml_element->line_number(),
				QString("properties with multiple types should have the '%1' attribute")
						.arg(convert_qualified_xml_name_to_qstring(DEFAULT_TYPE_ATTRIBUTE_NAME)));
	}

	// Convert the attribute value string to a qualified structural type name.
	const XmlAttributeValue &attribute_value = default_type_attribute_iter->second;
	boost::optional<GPlatesPropertyValues::StructuralType> default_property_structural_type =
			convert_qstring_to_qualified_xml_name<GPlatesPropertyValues::StructuralType>(
					GPlatesUtils::make_qstring_from_icu_string(attribute_value.get()));

	// If there was a failure converting string to qualified structural type name.
	if (!default_property_structural_type)
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				property_xml_element->line_number(),
				QString("failed to read attribute '%1'='%2' as a qualified structural type")
						.arg(convert_qualified_xml_name_to_qstring(DEFAULT_TYPE_ATTRIBUTE_NAME))
						.arg(GPlatesUtils::make_qstring_from_icu_string(attribute_value.get())));
	}

	return default_property_structural_type.get();
}


GPlatesModel::GpgimProperty::time_dependent_flags_type
GPlatesModel::Gpgim::read_feature_property_time_dependent_types(
		const XmlElementNode::non_null_ptr_type &property_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property time-dependent types in the GPGIM XML file.
	static const XmlElementName PROPERTY_TIME_DEPENDENT_ELEMENT_NAME = XmlElementName::create_gpgim("TimeDependent");

	// Look for property time-dependent elements.
	// These are optional and there can be multiple elements.
	std::vector<XmlElementNode::non_null_ptr_type> property_time_dependent_elements;
	find_zero_or_more_child_xml_elements(
			property_time_dependent_elements,
			property_xml_element,
			PROPERTY_TIME_DEPENDENT_ELEMENT_NAME,
			gpgim_resource_filename);

	GpgimProperty::time_dependent_flags_type time_dependent_flags;

	// Iterate over the time-dependent elements.
	BOOST_FOREACH(
			const XmlElementNode::non_null_ptr_type &property_time_dependent_element,
			property_time_dependent_elements)
	{
		// Get the time-dependent type.
		const XmlElementName time_dependent_type = get_qualified_xml_name<XmlElementName>(
				property_time_dependent_element,
				gpgim_resource_filename);

		static const XmlElementName CONSTANT_VALUE = XmlElementName::create_gpml("ConstantValue");
		static const XmlElementName PIECEWISE_AGGREGATION = XmlElementName::create_gpml("PiecewiseAggregation");
		static const XmlElementName IRREGULAR_SAMPLING = XmlElementName::create_gpml("IrregularSampling");
		if (time_dependent_type == CONSTANT_VALUE)
		{
			time_dependent_flags.set(GpgimProperty::CONSTANT_VALUE);
		}
		else if (time_dependent_type == PIECEWISE_AGGREGATION)
		{
			time_dependent_flags.set(GpgimProperty::PIECEWISE_AGGREGATION);
		}
		else if (time_dependent_type == IRREGULAR_SAMPLING)
		{
			time_dependent_flags.set(GpgimProperty::IRREGULAR_SAMPLING);
		}
		else
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_time_dependent_element->line_number(),
					QString("XML element '%1' should contain one of '%2', '%3' or '%4'")
							.arg(convert_qualified_xml_name_to_qstring(PROPERTY_TIME_DEPENDENT_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(CONSTANT_VALUE))
							.arg(convert_qualified_xml_name_to_qstring(PIECEWISE_AGGREGATION))
							.arg(convert_qualified_xml_name_to_qstring(IRREGULAR_SAMPLING)));
		}
	}

	return time_dependent_flags;
}


void
GPlatesModel::Gpgim::read_feature_class_xml_elements(
		feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
		const XmlElementNode::non_null_ptr_type &feature_class_list_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for the feature class list element in the GPGIM XML file.
	static const XmlElementName FEATURE_CLASS_LIST_ELEMENT_NAME = XmlElementName::create_gpgim("FeatureClassList");

	// The XML element name for feature class elements in the GPGIM XML file.
	static const XmlElementName FEATURE_CLASS_ELEMENT_NAME = XmlElementName::create_gpgim("FeatureClass");

	// Iterate over the feature classes.
	for (XmlElementNode::child_const_iterator feature_class_xml_node_iter = feature_class_list_xml_element->children_begin();
		feature_class_xml_node_iter != feature_class_list_xml_element->children_end();
		++feature_class_xml_node_iter)
	{
		// See if the XML node is an XML element.
		XmlNodeUtils::XmlElementNodeExtractionVisitor xml_element_node_extraction_visitor;
		boost::optional<XmlElementNode::non_null_ptr_type> feature_class_xml_element_opt =
				xml_element_node_extraction_visitor.get_xml_element_node(
						*feature_class_xml_node_iter);
		if (!feature_class_xml_element_opt)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					feature_class_list_xml_element->line_number(),
					QString("the '%1' element should only contain '%2' elements, not text" )
							.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_LIST_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_ELEMENT_NAME)));
		}
		const XmlElementNode::non_null_ptr_type feature_class_xml_element =
				feature_class_xml_element_opt.get();

		// Make sure there's only feature class elements in the feature class list element.
		if (feature_class_xml_element->get_name() != FEATURE_CLASS_ELEMENT_NAME)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					feature_class_xml_element->line_number(),
					QString("the element '%1' inside the '%2' is not '%3'" )
							.arg(convert_qualified_xml_name_to_qstring(feature_class_xml_element->get_name()))
							.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_LIST_ELEMENT_NAME))
							.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_ELEMENT_NAME)));
		}

		// The XML element name for the name of a feature class in the GPGIM XML file.
		static const XmlElementName FEATURE_CLASS_NAME_ELEMENT_NAME = XmlElementName::create_gpgim("Name");

		// Look for the feature class name element.
		const XmlElementNode::non_null_ptr_type feature_class_name_element =
				find_one_child_xml_element(
						feature_class_xml_element,
						FEATURE_CLASS_NAME_ELEMENT_NAME,
						gpgim_resource_filename);

		// Get the feature class qualified name which is the same as 'FeatureType'.
		const FeatureType feature_type = get_qualified_xml_name<FeatureType>(
				feature_class_name_element,
				gpgim_resource_filename);

		// Make sure there's no unclassified feature type in the GPGIM XML file.
		// We'll be explicitly adding one later as a special-case feature type.
		static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE_TYPE =
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
		if (feature_type == UNCLASSIFIED_FEATURE_TYPE)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					feature_class_name_element->line_number(),
					QString("'%1' is a special-case feature type - it should not be added to the file")
							.arg(convert_qualified_xml_name_to_qstring(UNCLASSIFIED_FEATURE_TYPE)));
		}

		// Add to our mapping of feature type to feature class XML element node.
		// Make sure the same feature class name (feature type) does not appear more than once.
		if (!feature_class_xml_element_node_map.insert(
			std::make_pair(feature_type, feature_class_xml_element)).second)
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					feature_class_name_element->line_number(),
					QString("duplicate feature class name '%1'")
							.arg(convert_qualified_xml_name_to_qstring(feature_type)));
		}
	}
}


void
GPlatesModel::Gpgim::create_feature_classes(
		const feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
		const QString &gpgim_resource_filename)
{
	// Iterate through the feature class XML element nodes.
	BOOST_FOREACH(
			const feature_class_xml_element_node_map_type::value_type &feature_class_xml_element_entry,
			feature_class_xml_element_node_map)
	{
		const FeatureType &feature_type = feature_class_xml_element_entry.first;
		XmlElementNode::non_null_ptr_type feature_class_xml_element = feature_class_xml_element_entry.second;

		// Create the GPGIM feature class if it hasn't already been created.
		create_feature_class_if_necessary(
				feature_type,
				feature_class_xml_element/*feature_class_reference_xml_element*/,
				feature_class_xml_element_node_map,
				gpgim_resource_filename);
	}

	// Create the special-case 'gpml:UnclassifiedFeature' that can contain *any* GPGIM property
	// in any quantity (a multiplicity of '0..*').
	create_unclassified_feature_class(gpgim_resource_filename);
}


GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
GPlatesModel::Gpgim::create_unclassified_feature_class(
		const QString &gpgim_resource_filename)
{
	// Iterate over all GPGIM properties and create a copy of each one that has a multiplicity of '0..*'.
	// This is a bit risky because it means the unclassified feature class has GPGIM properties
	// that are not in the global GPGIM property list (as is the case with the other feature types).
	// But it's a lot better than having special-case code scattered throughout GPlates to handle
	// 'gpml:UnclassifiedFeature'.
	GpgimFeatureClass::gpgim_property_seq_type gpgim_unclassified_feature_properties;
	BOOST_FOREACH(const property_map_type::value_type &property_map_element, d_property_map)
	{
		GPlatesModel::GpgimProperty::non_null_ptr_to_const_type gpgim_feature_property =
				property_map_element.second;

		GPlatesModel::GpgimProperty::non_null_ptr_type gpgim_unclassified_feature_property =
				gpgim_feature_property->clone();

		gpgim_unclassified_feature_property->set_multiplicity(GPlatesModel::GpgimProperty::ZERO_OR_MORE);

		gpgim_unclassified_feature_properties.push_back(gpgim_unclassified_feature_property);
	}

	// Unclassified feature type.
	static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
	static const QString UNCLASSIFIED_FEATURE_DESCRIPTION =
			"Unclassified feature containing any number of any GPGIM properties.";

	// Create the unclassified feature class.
	const GpgimFeatureClass::non_null_ptr_type unclassified_feature_class =
			GpgimFeatureClass::create(
					UNCLASSIFIED_FEATURE_TYPE,
					UNCLASSIFIED_FEATURE_DESCRIPTION,
					gpgim_unclassified_feature_properties.begin(),
					gpgim_unclassified_feature_properties.end());

	// Add to our feature class map.
	d_feature_class_map.insert(
			std::make_pair(unclassified_feature_class->get_feature_type(), unclassified_feature_class));

	// Unclassified is a concrete feature type.
	d_concrete_feature_types.push_back(unclassified_feature_class->get_feature_type());

	return unclassified_feature_class;
}


GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
GPlatesModel::Gpgim::create_feature_class_if_necessary(
		const FeatureType &feature_type,
		const XmlElementNode::non_null_ptr_type &feature_class_reference_xml_element,
		const feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
		const QString &gpgim_resource_filename)
{
	// See if we've already created the feature class.
	feature_class_map_type::const_iterator feature_class_iter = d_feature_class_map.find(feature_type);
	if (feature_class_iter != d_feature_class_map.end())
	{
		return feature_class_iter->second;
	}

	// Lookup the XML element node associated with the feature type.
	feature_class_xml_element_node_map_type::const_iterator feature_class_xml_element_iter =
			feature_class_xml_element_node_map.find(feature_type);
	if (feature_class_xml_element_iter == feature_class_xml_element_node_map.end())
	{
		// The feature class is not defined in the XML document.
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				feature_class_reference_xml_element->line_number(),
				QString("Feature class '%1' is not defined")
						.arg(convert_qualified_xml_name_to_qstring(feature_type)));
	}
	const XmlElementNode::non_null_ptr_type &feature_class_xml_element =
			feature_class_xml_element_iter->second;

	// Create the feature class from the XML element node.
	return create_feature_class(
			feature_type,
			feature_class_xml_element,
			feature_class_xml_element_node_map,
			gpgim_resource_filename);
}


GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type
GPlatesModel::Gpgim::create_feature_class(
		const FeatureType &feature_type,
		const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
		const feature_class_xml_element_node_map_type &feature_class_xml_element_node_map,
		const QString &gpgim_resource_filename)
{
	// The XML element name for the feature class description in the GPGIM XML file.
	static const XmlElementName FEATURE_CLASS_DESCRIPTION_ELEMENT_NAME = XmlElementName::create_gpgim("Description");

	// Look for the feature class description element.
	XmlElementNode::non_null_ptr_type feature_class_description_element =
			find_one_child_xml_element(
					feature_class_xml_element,
					FEATURE_CLASS_DESCRIPTION_ELEMENT_NAME,
					gpgim_resource_filename);
	const QString feature_description = get_text(
			feature_class_description_element,
			gpgim_resource_filename);

	// The optional parent feature class.
	boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> parent_feature_class;

	// The XML element name for the inherited feature class in the GPGIM XML file.
	static const XmlElementName FEATURE_CLASS_INHERITS_ELEMENT_NAME = XmlElementName::create_gpgim("Inherits");

	// Look for the optional feature class inherits element.
	boost::optional<XmlElementNode::non_null_ptr_type> feature_class_inherits_element =
			find_zero_or_one_child_xml_elements(
					feature_class_xml_element,
					FEATURE_CLASS_INHERITS_ELEMENT_NAME,
					gpgim_resource_filename);
	if (feature_class_inherits_element)
	{
		// Get the feature class inherited type.
		const FeatureType parent_feature_type = get_qualified_xml_name<FeatureType>(
				feature_class_inherits_element.get(),
				gpgim_resource_filename);

		// Create the parent feature class if we haven't already.
		parent_feature_class =
				create_feature_class_if_necessary(
						parent_feature_type,
						feature_class_inherits_element.get()/*feature_class_reference_xml_element*/,
						feature_class_xml_element_node_map,
						gpgim_resource_filename);
	}

	// Read the feature class properties.
	GpgimFeatureClass::gpgim_property_seq_type gpgim_feature_properties;
	create_feature_properties(
			gpgim_feature_properties,
			feature_class_xml_element,
			gpgim_resource_filename);

	// Create the feature class.
	const GpgimFeatureClass::non_null_ptr_type feature_class =
			GpgimFeatureClass::create(
					feature_type,
					feature_description,
					gpgim_feature_properties.begin(),
					gpgim_feature_properties.end(),
					parent_feature_class);

	// Add to our feature class map.
	// We don't need to check for duplicate feature types since that was already done
	// when reading the feature class XML element nodes.
	d_feature_class_map.insert(
			std::make_pair(feature_class->get_feature_type(), feature_class));

	// If the feature class is concrete then add it to the list of concrete feature types.
	if (is_concrete_feature_class(feature_class_xml_element, gpgim_resource_filename))
	{
		d_concrete_feature_types.push_back(feature_class->get_feature_type());
	}

	return feature_class;
}


void
GPlatesModel::Gpgim::create_feature_properties(
		GpgimFeatureClass::gpgim_property_seq_type &gpgim_feature_properties,
		const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
		const QString &gpgim_resource_filename)
{
	// The XML element name for property elements in the GPGIM XML file.
	static const XmlElementName PROPERTY_ELEMENT_NAME = XmlElementName::create_gpgim("Property");

	// Look for the property elements.
	// There can be any number of property definitions and there can be none defined.
	std::vector<XmlElementNode::non_null_ptr_type> property_xml_elements;
	find_zero_or_more_child_xml_elements(
			property_xml_elements,
			feature_class_xml_element,
			PROPERTY_ELEMENT_NAME,
			gpgim_resource_filename);

	// Get a GPGIM feature property for each property XML element.
	// These properties have already been created in 'create_properties()'.
	// We just need to look them up and reference them.
	BOOST_FOREACH(
			const XmlElementNode::non_null_ptr_type &property_xml_element,
			property_xml_elements)
	{
		// Get the (qualified) property name.
		const PropertyName property_name =
				get_qualified_xml_name<PropertyName>(
						property_xml_element,
						gpgim_resource_filename);

		// Lookup the map of properties.
		property_map_type::const_iterator gpgim_property_iter = d_property_map.find(property_name);
		if (gpgim_property_iter == d_property_map.end())
		{
			throw GpgimInitialisationException(
					GPLATES_EXCEPTION_SOURCE,
					gpgim_resource_filename,
					property_xml_element->line_number(),
					QString("'%1' is not a recognised property name")
							.arg(convert_qualified_xml_name_to_qstring(property_name)));
		}

		// Add to the list of GPGIM properties referenced by the current feature.
		const GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property =
				gpgim_property_iter->second;
		gpgim_feature_properties.push_back(gpgim_feature_property);
	}
}


bool
GPlatesModel::Gpgim::is_concrete_feature_class(
		const XmlElementNode::non_null_ptr_type &feature_class_xml_element,
		const QString &gpgim_resource_filename) const
{
	// The XML element name for the type of a feature class in the GPGIM XML file.
	static const XmlElementName FEATURE_CLASS_TYPE_ELEMENT_NAME = XmlElementName::create_gpgim("ClassType");

	// Look for the feature class type element.
	const XmlElementNode::non_null_ptr_type feature_class_type_element =
			find_one_child_xml_element(
					feature_class_xml_element,
					FEATURE_CLASS_TYPE_ELEMENT_NAME,
					gpgim_resource_filename);

	// Get the feature class type.
	const QString feature_class_type = get_text(feature_class_type_element, gpgim_resource_filename);

	// Make sure feature class type is one of the expected values.
	static const QString ABSTRACT = "abstract";
	static const QString CONCRETE = "concrete";
	if (feature_class_type != ABSTRACT &&
		feature_class_type != CONCRETE)
	{
		throw GpgimInitialisationException(
				GPLATES_EXCEPTION_SOURCE,
				gpgim_resource_filename,
				feature_class_type_element->line_number(),
				QString("XML element '%1' should contain either '%2' or '%2'")
						.arg(convert_qualified_xml_name_to_qstring(FEATURE_CLASS_TYPE_ELEMENT_NAME))
						.arg(ABSTRACT)
						.arg(CONCRETE));
	}

	return feature_class_type == CONCRETE;
}
