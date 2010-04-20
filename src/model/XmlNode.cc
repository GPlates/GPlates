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

#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/bind.hpp>
#include "XmlNode.h"


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
			new XmlTextNode(reader.lineNumber(), reader.columnNumber(), text),
			GPlatesUtils::NullIntrusivePointerHandler());
}


void
GPlatesModel::XmlTextNode::write_to(
		QXmlStreamWriter &writer) const 
{
	writer.writeCharacters(d_text);
}


namespace
{
	const QString
	get_alias_from_qualified_name(
			const QString &name)
	{
		static const QChar SEPARATOR = ':';
		static const int START_FIRST_SECTION = 0;
		static const int END_FIRST_SECTION = 0;

		return name.section(SEPARATOR, START_FIRST_SECTION, END_FIRST_SECTION);
	}
}


const GPlatesModel::XmlElementNode::non_null_ptr_type
GPlatesModel::XmlElementNode::create(
		QXmlStreamReader &reader,
		const boost::shared_ptr<GPlatesModel::XmlElementNode::AliasToNamespaceMap> &
			parent_alias_map)
{
	// Make sure reader is at starting element
	Q_ASSERT(reader.isStartElement());

	// Store the tag name of start element.
	PropertyName prop_name(
			reader.namespaceUri().toString(),
			get_alias_from_qualified_name(reader.qualifiedName().toString()),
			reader.name().toString());

	non_null_ptr_type elem(
			new XmlElementNode(reader.lineNumber(), reader.columnNumber(), prop_name),
			GPlatesUtils::NullIntrusivePointerHandler());

	QXmlStreamNamespaceDeclarations ns_decls = reader.namespaceDeclarations();
	// If this element contains namespace declarations...
	if ( ! ns_decls.empty()) {
		// ... copy the parent's map...
		elem->d_alias_map = boost::shared_ptr<AliasToNamespaceMap>(
				new AliasToNamespaceMap(
					parent_alias_map->begin(), 
					parent_alias_map->end()));

		// ... and add the new declarations.
		QXmlStreamNamespaceDeclarations::iterator 
			iter = ns_decls.begin(), 
			end = ns_decls.end();
		for ( ; iter != end; ++iter) {
			elem->d_alias_map->insert(
					std::make_pair(
						iter->prefix().toString(),
						iter->namespaceUri().toString()));
		}
	}
	else {
		// Otherwise, just link to the parent's map.
		elem->d_alias_map = parent_alias_map;
	}

	elem->load_attributes(reader.attributes());

	// .atEnd() can not be relied upon when reading a QProcess,
	// so we must make sure we block for a moment to make sure
	// the process is ready to feed us data.
	reader.device()->waitForReadyRead(1000);
	while ( ! reader.atEnd()) {
		reader.readNext();

		if (reader.isEndElement()) {
			break;
		}

		try {
			if (reader.isStartElement()) {
				XmlNode::non_null_ptr_type child = 
					XmlElementNode::create(reader, elem->d_alias_map);
				elem->d_children.push_back(child);
			} else if (reader.isCharacters() && ! reader.isWhitespace()) {
				XmlNode::non_null_ptr_type child = XmlTextNode::create(reader);
				elem->d_children.push_back(child);
			}

		// FIXME: Fix these catch clauses.
		} catch (const char *ex) {
			std::cerr << "Caught exception: " << ex << std::endl;
			throw;
		} catch (...) {
			throw;
		}
		reader.device()->waitForReadyRead(1000);
	}

	return elem;
}


const GPlatesModel::XmlElementNode::non_null_ptr_type
GPlatesModel::XmlElementNode::create(
		const GPlatesModel::XmlTextNode::non_null_ptr_type &text,
		const GPlatesModel::PropertyName &prop_name)
{	
	non_null_ptr_type elem(
			new XmlElementNode(text->line_number(), text->column_number(), prop_name),
			GPlatesUtils::NullIntrusivePointerHandler());

	elem->d_children.push_back(text);
	return elem;
}


namespace
{
	const GPlatesModel::XmlElementNode::Attribute
	convert_qxmlstreamattribute_to_attribute(
			const QXmlStreamAttribute &attribute)
	{
		return std::make_pair(
				GPlatesModel::XmlAttributeName(
					attribute.namespaceUri().toString(),
					get_alias_from_qualified_name(attribute.qualifiedName().toString()),
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
				std::ptr_fun(convert_attribute_to_qxmlstreamattribute));
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
			std::ptr_fun(convert_qxmlstreamattribute_to_attribute));
}


namespace
{
	class FindChildByNameVisitor
		: public GPlatesModel::XmlNodeVisitor
	{
	public:
		FindChildByNameVisitor(
				const GPlatesModel::PropertyName &name) :
			d_name(name), found(false)
		{ }

		virtual
		~FindChildByNameVisitor()
		{ }

		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
		get_child() const {
			return d_child;
		}

		bool
		check_name(
				const GPlatesModel::XmlNode::non_null_ptr_type &node) {
			node->accept_visitor(*this);
			return found;
		}

		virtual
		void
		visit_element_node(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem) {
			found = (elem->get_name() == d_name);
			if (found) {
				d_child = elem;
			}
		}

		virtual
		void
		visit_text_node(
				const GPlatesModel::XmlTextNode::non_null_ptr_type &text) {
			found = false;
		}

	private:
		GPlatesModel::PropertyName d_name;
		bool found;
		boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> d_child;
	};
}


boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
GPlatesModel::XmlElementNode::get_child_by_name(
		const GPlatesModel::PropertyName &name) const
{
	return get_next_child_by_name(name, children_begin()).second;
}


std::pair<
	GPlatesModel::XmlElementNode::child_const_iterator, 
	boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> >
GPlatesModel::XmlElementNode::get_next_child_by_name(
		const PropertyName &name,
		const child_const_iterator &begin) const
{
	FindChildByNameVisitor visitor(name);
	child_const_iterator child = std::find_if(begin, children_end(),
			boost::bind(&FindChildByNameVisitor::check_name, boost::ref(visitor), _1));
	return std::make_pair(child, visitor.get_child());
}


void
GPlatesModel::XmlTextNode::accept_visitor(
		GPlatesModel::XmlNodeVisitor &visitor)
{
	visitor.visit_text_node(non_null_ptr_type(this,
			GPlatesUtils::NullIntrusivePointerHandler()));
}


void
GPlatesModel::XmlElementNode::accept_visitor(
		GPlatesModel::XmlNodeVisitor &visitor)
{
	// FIXME: This is nasty, but I can't think of any other way to work it
	// at the moment.
	//XmlElementNode *ptr = const_cast<XmlElementNode *>(this);
	visitor.visit_element_node(non_null_ptr_type(this,
			GPlatesUtils::NullIntrusivePointerHandler()));
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

