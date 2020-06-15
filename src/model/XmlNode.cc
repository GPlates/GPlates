/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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
#include <iterator>
#include <boost/bind.hpp>

#include "XmlNode.h"

#include "XmlNodeUtils.h"

#include "utils/CallStackTracker.h"


namespace
{
	/**
	 * Creates an @a QualifiedXmlName from a namespace URI, namespace alias (prefix) and local name.
	 *
	 * If the namespace alias (prefix) is an empty string then it is determined from the namespace URI
	 * which is assumed to be one of the standard namespaces used by GPlates (the namespace alias
	 * is determined internally by @a QualifiedXmlName).
	 */
	template <class QualifiedXmlNameType>
	QualifiedXmlNameType
	get_qualified_xml_name(
			const QString &namespace_uri,
			const QString &namespace_prefix,
			const QString &local_name)
	{
		if (namespace_prefix.isEmpty())
		{
			return QualifiedXmlNameType(namespace_uri, local_name);
		}

		return QualifiedXmlNameType(namespace_uri, namespace_prefix, local_name);
	}


	const GPlatesModel::XmlElementNode::Attribute
	convert_qxmlstreamattribute_to_attribute(
			const QXmlStreamAttribute &attribute)
	{
		return std::make_pair(
				get_qualified_xml_name<GPlatesModel::XmlAttributeName>(
					attribute.namespaceUri().toString(),
					attribute.prefix().toString(),
					attribute.name().toString()),
				GPlatesModel::XmlAttributeValue(
					GPlatesUtils::make_icu_string_from_qstring(attribute.value().toString())));
	}


	const QXmlStreamAttribute
	convert_attribute_to_qxmlstreamattribute(
			const GPlatesModel::XmlElementNode::Attribute &attribute)
	{
		return QXmlStreamAttribute(
				GPlatesUtils::make_qstring_from_icu_string(attribute.first.get_namespace()),
				GPlatesUtils::make_qstring_from_icu_string(attribute.first.get_name()),
				GPlatesUtils::make_qstring_from_icu_string(attribute.second.get()));
	}
}


#if 0
const GPlatesModel::XmlNode::non_null_ptr_type
GPlatesModel::XmlNode::create(
		QXmlStreamReader &reader)
{
	bool found_parsable = false;
	// .atEnd() can not be relied upon when reading a QProcess,
	// so we must make sure we block for a moment to make sure
	// the process is ready to feed us data.
	reader.device()->waitForReadyRead(1000);
	while ( ! reader.atEnd()) 
	{
		switch (reader.tokenType()) 
		{
			case QXmlStreamReader::Invalid:
				// FIXME: Throw correct exception.
				std::cerr << "Invalid data at line " << reader.lineNumber()
					<< " column " << reader.columnNumber() << std::endl;
				throw "Invalid data.";
				break;
			case QXmlStreamReader::StartElement:
				return XmlElementNode::create(reader);
				break;
			case QXmlStreamReader::Comment:
			case QXmlStreamReader::DTD:
			case QXmlStreamReader::EntityReference:
				// FIXME: Issue warning (to ReadErrorAccumulation, or whatever).
				std::cerr << "Encountered a comment, DTD or entity reference.  "\
					"Time to die." << std::endl;
				std::cerr << "Location: line " << reader.lineNumber()
					<< " column " << reader.columnNumber() << std::endl;
				break;
			case QXmlStreamReader::Characters:
				found_parsable = true;
				break;
			default:
				// FIXME: Throw correct exception.
				throw "Invalid QXmlStreamReader::TokenType.";
				break;
		}
		if (found_parsable)
			break;

		reader.readNext();
		reader.device()->waitForReadyRead(1000);
	}
	// .atEnd() can not be relied upon when reading a QProcess,
	// so we must make sure we block for a moment to make sure
	// the process is ready to feed us data.
	reader.device()->waitForReadyRead(1000);
	Q_ASSERT( ! reader.atEnd());
	Q_ASSERT(reader.tokenType() == QXmlStreamReader::Characters);
	return XmlTextNode::create(reader);
}
#endif


const GPlatesModel::XmlTextNode::non_null_ptr_type
GPlatesModel::XmlTextNode::create(
		QXmlStreamReader &reader)
{
	// We don't trim here because a string that contains an ampersand is broken up
	// into two nodes for some reason, and if we trim here, the spacing around the
	// ampersand will not be read in correctly.
	QString text = reader.text().toString();
	return non_null_ptr_type(
			new XmlTextNode(reader.lineNumber(), reader.columnNumber(), text));
}


void
GPlatesModel::XmlTextNode::write_to(
		QXmlStreamWriter &writer) const 
{
	writer.writeCharacters(d_text);
}


const GPlatesModel::XmlElementNode::non_null_ptr_type
GPlatesModel::XmlElementNode::create(
		QXmlStreamReader &reader,
		const boost::shared_ptr<GPlatesModel::XmlElementNode::AliasToNamespaceMap> &parent_alias_map)
{
	// Add this scope to the call stack trace that is printed for an exception thrown in this scope.
	TRACK_CALL_STACK();

	// Make sure reader is at starting element
	Q_ASSERT(reader.isStartElement());

	// Store the tag name of start element.
	const XmlElementName element_name =
			get_qualified_xml_name<XmlElementName>(
					reader.namespaceUri().toString(),
					reader.prefix().toString(),
					reader.name().toString());

	non_null_ptr_type elem(
			new XmlElementNode(reader.lineNumber(), reader.columnNumber(), element_name));

	QXmlStreamNamespaceDeclarations ns_decls = reader.namespaceDeclarations();
	// If this element contains namespace declarations...
	if ( ! ns_decls.empty())
	{
		// ... copy the parent's map...
		elem->d_alias_map = boost::shared_ptr<AliasToNamespaceMap>(
				new AliasToNamespaceMap(
					parent_alias_map->begin(), 
					parent_alias_map->end()));

		// ... and add the new declarations.
		QXmlStreamNamespaceDeclarations::iterator 
			iter = ns_decls.begin(), 
			end = ns_decls.end();
		for ( ; iter != end; ++iter)
		{
			elem->d_alias_map->insert(
					std::make_pair(
						iter->prefix().toString(),
						iter->namespaceUri().toString()));
		}
	}
	else
	{
		// Otherwise, just link to the parent's map.
		elem->d_alias_map = parent_alias_map;
	}

	elem->load_attributes(reader.attributes());

	// .atEnd() can not be relied upon when reading a QProcess,
	// so we must make sure we block for a moment to make sure
	// the process is ready to feed us data.
	reader.device()->waitForReadyRead(1000);
	while ( ! reader.atEnd())
	{
		reader.readNext();

		if (reader.isEndElement())
		{
			break;
		}

		if (reader.isStartElement())
		{
			XmlNode::non_null_ptr_type child = 
				XmlElementNode::create(reader, elem->d_alias_map);
			elem->d_children.push_back(child);
		}
		else if (reader.isCharacters() && ! reader.isWhitespace())
		{
			XmlNode::non_null_ptr_type child = XmlTextNode::create(reader);
			elem->d_children.push_back(child);
		}

		reader.device()->waitForReadyRead(1000);
	}

	return elem;
}


const GPlatesModel::XmlElementNode::non_null_ptr_type
GPlatesModel::XmlElementNode::create(
		const GPlatesModel::XmlTextNode::non_null_ptr_type &text,
		const GPlatesModel::XmlElementName &element_name)
{	
	non_null_ptr_type elem(
			new XmlElementNode(text->line_number(), text->column_number(), element_name));

	elem->d_children.push_back(text);
	return elem;
}


void
GPlatesModel::XmlElementNode::write_to(
		QXmlStreamWriter &writer) const
{
	writer.writeStartElement(
			GPlatesUtils::make_qstring_from_icu_string(d_name.get_namespace()),
			GPlatesUtils::make_qstring_from_icu_string(d_name.get_name()));
		QXmlStreamAttributes attributes;
		std::transform(d_attributes.begin(), d_attributes.end(),
				std::back_inserter(attributes),
				[] (const Attribute &attr) { return convert_attribute_to_qxmlstreamattribute(attr); });
		writer.writeAttributes(attributes);
		std::for_each(d_children.begin(), d_children.end(),
				boost::bind(&XmlNode::write_to, _1, boost::ref(writer)));
	writer.writeEndElement();
}


void
GPlatesModel::XmlElementNode::load_attributes(
		const QXmlStreamAttributes &attributes)
{
	std::transform(attributes.begin(), attributes.end(),
			std::inserter(d_attributes, d_attributes.begin()),
			[] (const QXmlStreamAttribute &attr) { return convert_qxmlstreamattribute_to_attribute(attr); });
}


boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
GPlatesModel::XmlElementNode::get_child_by_name(
		const GPlatesModel::XmlElementName &name) const
{
	return get_next_child_by_name(name, children_begin()).second;
}


std::pair<
	GPlatesModel::XmlElementNode::child_const_iterator, 
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> >
GPlatesModel::XmlElementNode::get_next_child_by_name(
		const XmlElementName &name,
		const child_const_iterator &begin) const
{
	XmlNodeUtils::XmlElementNodeExtractionVisitor visitor(name);

	boost::optional<XmlElementNode::non_null_ptr_type> child_xml_element_node;

	// Iterator over the child nodes.
	for (child_const_iterator child_iter = begin; child_iter != children_end(); ++child_iter)
	{
		child_xml_element_node = visitor.get_xml_element_node(*child_iter);
		if (child_xml_element_node)
		{
			return std::make_pair(child_iter, child_xml_element_node);
		}
	}

	// No child XML element node with matching element name.
	return std::make_pair(children_end(), boost::none);
}


void
GPlatesModel::XmlTextNode::accept_visitor(
		GPlatesModel::XmlNodeVisitor &visitor)
{
	visitor.visit_text_node(non_null_ptr_type(this));
}


void
GPlatesModel::XmlElementNode::accept_visitor(
		GPlatesModel::XmlNodeVisitor &visitor)
{
	// FIXME: This is nasty, but I can't think of any other way to work it
	// at the moment.
	//XmlElementNode *ptr = const_cast<XmlElementNode *>(this);
	visitor.visit_element_node(non_null_ptr_type(this));
}


bool
GPlatesModel::XmlElementNode::operator==(
		const XmlElementNode &other) const
{
	return d_name == other.d_name &&
		d_attributes == other.d_attributes &&
		d_children == other.d_children &&
		d_alias_map == other.d_alias_map;
}

