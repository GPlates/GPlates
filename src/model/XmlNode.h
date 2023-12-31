/* $Id$ */

/**
 * @file 
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

#ifndef GPLATES_MODEL_XMLNODE_H
#define GPLATES_MODEL_XMLNODE_H

#include <map>
#include <list>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <QXmlStreamReader>

#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"
#include "XmlElementName.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"
#include "utils/StringSet.h"

namespace GPlatesModel
{

	class XmlNodeVisitor;


	/**
	 * XmlNode is the base class for a hierarchy used to store an uninterpreted
	 * XML tree in memory.
	 */
	class XmlNode :
			public GPlatesUtils::ReferenceCount<XmlNode>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<XmlNode,
				GPlatesUtils::NullIntrusivePointerHandler>
						non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const XmlNode,
				GPlatesUtils::NullIntrusivePointerHandler>
						non_null_ptr_to_const_type;


		virtual
		~XmlNode()
		{ }


#if 0
		/**
		 * This is a factory method for XmlNodes.  One should avoid the use
		 * of the static "create" methods in the subclasses.
		 */
		static
		const non_null_ptr_type
		create(
				QXmlStreamReader &reader);
#endif


		virtual
		void
		write_to(
				QXmlStreamWriter &writer) const = 0;

		virtual
		void
		accept_visitor(
				XmlNodeVisitor &visitor) = 0;


		qint64
		line_number() const {
			return d_line_num;
		}


		qint64
		column_number() const {
			return d_col_num;
		}


	protected:
		XmlNode(
				const qint64 &line_num,
				const qint64 &col_num) :
			d_line_num(line_num), d_col_num(col_num)
		{ }

	private:
		qint64 d_line_num;
		qint64 d_col_num;

		XmlNode &
		operator=(
				const XmlNode &);

	};


	class XmlTextNode
		: public XmlNode
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<XmlTextNode,
				GPlatesUtils::NullIntrusivePointerHandler> 
						non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const XmlTextNode,
				GPlatesUtils::NullIntrusivePointerHandler>
						non_null_ptr_to_const_type;

		const QString &
		get_text() const
		{
			return d_text;
		}

		virtual
		~XmlTextNode()
		{ }


		static
		const non_null_ptr_type
		create(
				QXmlStreamReader &reader);


		virtual
		void
		write_to(
				QXmlStreamWriter &writer) const;

		virtual
		void
		accept_visitor(
				XmlNodeVisitor &visitor);

	private:
		QString d_text;

		XmlTextNode(
				const qint64 &line_num,
				const qint64 &col_num,
				const QString &text) :
			XmlNode(line_num, col_num), d_text(text)
		{ }


		XmlTextNode &
		operator=(
				const XmlTextNode &);
	};


	/**
	 * Holds information associated with a node in an XML DOM-like tree.
	 */
	class XmlElementNode 
		: public XmlNode
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<XmlElementNode,
				GPlatesUtils::NullIntrusivePointerHandler> 
						non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const XmlElementNode,
				GPlatesUtils::NullIntrusivePointerHandler> 
						non_null_ptr_to_const_type;

		/**
		 * Convenience typedefs for dealing with this node's attributes.
		 */
		typedef std::map<XmlAttributeName, XmlAttributeValue> AttributeCollection;
		typedef AttributeCollection::value_type Attribute;
		typedef AttributeCollection::iterator attribute_iterator;
		typedef AttributeCollection::const_iterator attribute_const_iterator;

		/**
		 * Convenience typedefs for dealing with this node's children.
		 */
		typedef std::list<XmlNode::non_null_ptr_type> ChildCollection;
		typedef ChildCollection::iterator child_iterator;
		typedef ChildCollection::const_iterator child_const_iterator;

		typedef std::pair<child_const_iterator, boost::optional<non_null_ptr_type> >
			named_child_const_iterator;

		typedef std::map<QString, QString> AliasToNamespaceMap;

		const XmlElementName &
		get_name() const
		{
			return d_name;
		}

		const attribute_const_iterator
		get_attribute_by_name(
				const XmlAttributeName &name) const
		{
			return d_attributes.find(name);
		}

		const attribute_const_iterator
		attributes_begin() const
		{
			return d_attributes.begin();
		}

		const attribute_const_iterator
		attributes_end() const
		{
			return d_attributes.end();
		}

		/**
		 * Warning: O(n).  Map should be small though...
		 */
		size_t
		number_of_attributes() const
		{
			return d_attributes.size();
		}

		bool
		attributes_empty() const
		{
			return d_attributes.empty();
		}

		boost::optional<non_null_ptr_type>
		get_child_by_name(
				const XmlElementName &name) const;

		named_child_const_iterator
		get_next_child_by_name(
				const XmlElementName &name,
				const child_const_iterator &begin) const;

		const child_const_iterator
		children_begin() const
		{
			return d_children.begin();
		}

		const child_const_iterator
		children_end() const
		{
			return d_children.end();
		}

		/**
		 * Warning: O(n).  List should be small though...
		 */
		size_t
		number_of_children() const
		{
			return d_children.size();
		}

		bool
		children_empty() const
		{
			return d_children.empty();
		}

		const boost::optional<QString>
		get_namespace_from_alias(
				const QString &alias) const
		{
			boost::optional<QString> ns_for_alias;
			AliasToNamespaceMap::const_iterator iter = d_alias_map->find(alias);
			if (iter != d_alias_map->end())
			{
				ns_for_alias = iter->second;
			}
			return ns_for_alias;
		}

		virtual
		~XmlElementNode()
		{ }


		static
		const non_null_ptr_type
		create(
				QXmlStreamReader &reader,
				const boost::shared_ptr<AliasToNamespaceMap> &parent_alias_map);

		static
		const non_null_ptr_type
		create(
				const XmlTextNode::non_null_ptr_type &text,
				const XmlElementName &element_name);

		virtual
		void
		write_to(
				QXmlStreamWriter &writer) const;

		virtual
		void
		accept_visitor(
				XmlNodeVisitor &visitor);

		bool
		operator==(
				const XmlElementNode &other) const;

	private:

		XmlElementName d_name;
		AttributeCollection d_attributes;
		ChildCollection d_children;
		boost::shared_ptr<AliasToNamespaceMap> d_alias_map;

		XmlElementNode(
				const qint64 &line_num,
				const qint64 &col_num,
				const XmlElementName &name):
			XmlNode(line_num, col_num), d_name(name)
		{ }

		XmlElementNode &
		operator=(
				const XmlElementNode &);

		void
		load_attributes(
				const QXmlStreamAttributes &attributes);
	};


	class XmlNodeVisitor
	{
	public:
		virtual
		~XmlNodeVisitor()
		{ }

		virtual
		void
		visit_text_node(
				const XmlTextNode::non_null_ptr_type &text)
		{ }

		virtual
		void
		visit_element_node(
				const XmlElementNode::non_null_ptr_type &elem)
		{ }
	};
}

#endif // GPLATES_MODEL_XMLNODE_H
