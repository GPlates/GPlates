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
#include <typeinfo>
#include <boost/bind/bind.hpp>

#include "XmlNode.h"

#include "XmlNodeUtils.h"

#include "model/TranscribeQualifiedXmlName.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "scribe/Scribe.h"

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


bool
GPlatesModel::XmlNode::operator==(
		const XmlNode &other) const
{
	// Both objects must have the same type before testing for equality.
	// This also means derived classes need no type-checking.
	if (typeid(*this) != typeid(other))
	{
		return false;
	}

	// Compare base class data.
	if (d_line_num != other.d_line_num ||
		d_col_num != other.d_col_num)
	{
		return false;
	}

	// Compare the derived class data.
	return equality(other);
}


GPlatesScribe::TranscribeResult
GPlatesModel::XmlNode::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_line_num, "line_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_col_num, "column_number"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


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


void
GPlatesModel::XmlTextNode::accept_visitor(
		GPlatesModel::XmlNodeVisitor &visitor)
{
	visitor.visit_text_node(non_null_ptr_type(this));
}


bool
GPlatesModel::XmlTextNode::equality(
		const XmlNode &other) const
{
	// Can use 'static_cast' (instead of 'dynamic_cast') since XmlNode::operator==() has confirmed that.
	const XmlTextNode &other_xml_text_node = static_cast<const XmlTextNode &>(other);

	return d_text == other_xml_text_node.d_text;
}


GPlatesScribe::TranscribeResult
GPlatesModel::XmlTextNode::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<XmlTextNode> &xml_text_node)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, xml_text_node->line_number(), "line_number");
		scribe.save(TRANSCRIBE_SOURCE, xml_text_node->column_number(), "column_number");
		scribe.save(TRANSCRIBE_SOURCE, xml_text_node->get_text(), "text");
	}
	else // loading
	{
		qint64 line_number_;
		qint64 column_number_;
		QString text_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, line_number_, "line_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, column_number_, "column_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, text_, "text"))
		{
			return scribe.get_transcribe_result();
		}

		xml_text_node.construct_object(line_number_, column_number_, text_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesModel::XmlTextNode::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (transcribed_construct_data)
	{
		// Base class (XmlNode) has already been transcribed (in 'XmlTextNode::transcribe_construct_data()').
		// So just record base/derived inheritance relationship.
		if (!scribe.transcribe_base<XmlNode, XmlTextNode>(TRANSCRIBE_SOURCE))
		{
			return scribe.get_transcribe_result();
		}

		// Our data member 'd_text' has also been transcribed (in 'XmlTextNode::transcribe_construct_data()').
	}
	else  // 'XmlTextNode::transcribe_construct_data()' has NOT been called...
	{
		// Transcribe base class (XmlNode) and our data members normally initialised from constructor (d_text).
		if (!scribe.transcribe_base<XmlNode>(TRANSCRIBE_SOURCE, *this, "XmlNode") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_text, "text"))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Transcribe data members NOT initialised from constructor.
	// Currently none.

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
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
				boost::bind(&XmlNode::write_to, boost::placeholders::_1, boost::ref(writer)));
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
GPlatesModel::XmlElementNode::accept_visitor(
		GPlatesModel::XmlNodeVisitor &visitor)
{
	// FIXME: This is nasty, but I can't think of any other way to work it
	// at the moment.
	//XmlElementNode *ptr = const_cast<XmlElementNode *>(this);
	visitor.visit_element_node(non_null_ptr_type(this));
}


bool
GPlatesModel::XmlElementNode::equality(
		const XmlNode &other) const
{
	// Can use 'static_cast' (instead of 'dynamic_cast') since XmlNode::operator==() has confirmed that.
	const XmlElementNode &other_xml_element_node = static_cast<const XmlElementNode &>(other);

	if (d_children.size() != other_xml_element_node.d_children.size())
	{
		return false;
	}

	auto children_iter = d_children.begin();
	auto other_children_iter = other_xml_element_node.d_children.begin();
	for (; children_iter != d_children.end(); ++children_iter, ++other_children_iter)
	{
		// Compare values (not pointers).
		if (**children_iter != **other_children_iter)
		{
			return false;
		}
	}

	return d_name == other_xml_element_node.d_name &&
			d_attributes == other_xml_element_node.d_attributes &&
			*d_alias_map == *other_xml_element_node.d_alias_map;
}


GPlatesScribe::TranscribeResult
GPlatesModel::XmlElementNode::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<XmlElementNode> &xml_element_node)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, xml_element_node->line_number(), "line_number");
		scribe.save(TRANSCRIBE_SOURCE, xml_element_node->column_number(), "column_number");
		scribe.save(TRANSCRIBE_SOURCE, xml_element_node->get_name(), "name");
	}
	else // loading
	{
		qint64 line_number_;
		qint64 column_number_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, line_number_, "line_number") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, column_number_, "column_number"))
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<XmlElementName> name_ = scribe.load<XmlElementName>(TRANSCRIBE_SOURCE, "name");
		if (!name_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		xml_element_node.construct_object(line_number_, column_number_, name_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesModel::XmlElementNode::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (transcribed_construct_data)
	{
		// Base class (XmlNode) has already been transcribed (in 'XmlElementNode::transcribe_construct_data()').
		// So just record base/derived inheritance relationship.
		if (!scribe.transcribe_base<XmlNode, XmlElementNode>(TRANSCRIBE_SOURCE))
		{
			return scribe.get_transcribe_result();
		}

		// Our data member 'd_name' has also been transcribed (in 'XmlElementNode::transcribe_construct_data()').
	}
	else  // 'XmlElementNode::transcribe_construct_data()' has NOT been called...
	{
		// Transcribe base class (XmlNode) and our data members normally initialised from constructor (d_name).
		if (!scribe.transcribe_base<XmlNode>(TRANSCRIBE_SOURCE, *this, "XmlNode") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_name, "name"))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Transcribe data members NOT initialised from constructor.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_attributes, "attributes") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_children, "children") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_alias_map, "alias_map"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
