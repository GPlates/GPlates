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

#ifndef GPLATES_MODEL_XMLNODEUTILS_H
#define GPLATES_MODEL_XMLNODEUTILS_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "XmlNode.h"

#include "QualifiedXmlName.h"
#include "XmlElementName.h"


namespace GPlatesModel
{
	namespace XmlNodeUtils
	{
		/**
		 * Determines if an @a XmlNode is an @a XmlElementNode.
		 *
		 * Also optionally tests for matching element name.
		 */
		class XmlElementNodeExtractionVisitor :
				public XmlNodeVisitor
		{
		public:

			//! Constructor that does not match the XML element name.
			XmlElementNodeExtractionVisitor()
			{  }

			//! Constructor that matches the XML element name.
			explicit
			XmlElementNodeExtractionVisitor(
					const XmlElementName &xml_element_name) :
				d_xml_element_name(xml_element_name)
			{  }


			/**
			 * Returns XML element node if @a xml_node is a @a XmlElementNode and optionally
			 * matches element name passed into constructor, otherwise boost::none.
			 */
			boost::optional<XmlElementNode::non_null_ptr_type>
			get_xml_element_node(
					const XmlNode::non_null_ptr_type &xml_node);

		private:

			virtual
			void
			visit_text_node(
					const XmlTextNode::non_null_ptr_type &text)
			{
				d_xml_element_node = boost::none;
			}

			virtual
			void
			visit_element_node(
					const XmlElementNode::non_null_ptr_type &xml_element_node);


			/**
			 * If the element name is specified then it must be matched to the visited XML element.
			 */
			boost::optional<XmlElementName> d_xml_element_name;

			boost::optional<XmlElementNode::non_null_ptr_type> d_xml_element_node;
		};


		/**
		 * Convenience iterator wrapper over a sequence of XML element nodes with same element name.
		 *
		 * Template parameter 'XmlNodeForwardIter' should dereference to a
		 * 'XmlElementNode::non_null_ptr_type'.
		 */
		template <typename XmlNodeForwardIter>
		class NamedXmlElementNodeIterator
		{
		public:

			/**
			 * Constructor begins iteration at the first XML element node found with the element name, if any.
			 */
			NamedXmlElementNodeIterator(
					const XmlNodeForwardIter &xml_elements_begin,
					const XmlNodeForwardIter &xml_elements_end,
					const XmlElementName &element_name) :
				d_xml_element_node_visitor(element_name),
				d_xml_nodes_begin(xml_elements_begin),
				d_xml_nodes_end(xml_elements_end),
				d_named_xml_element_iterator(xml_elements_begin)
			{
				first();
			}


			/**
			 * Begins iteration at the first XML element node found with the element name, if any.
			 *
			 * That note the constructor also begins iteration so this call is only necessary if
			 * doing a second iteration pass.
			 */
			void
			first()
			{
				// Start at the beginning.
				d_named_xml_element_iterator = NamedXmlElementIterator(d_xml_nodes_begin);
				d_next_named_xml_element_iterator = boost::none;

				// Find the first XML element node with a matching name.
				find_xml_element_node_with_matching_name(d_named_xml_element_iterator);
			}


			/**
			 * Note: @a finished should be false when this is called.
			 */
			void
			next()
			{
				// If 'has_next()' was called then use its result to avoid duplicate queries.
				if (d_next_named_xml_element_iterator)
				{
					d_named_xml_element_iterator = d_next_named_xml_element_iterator.get();
					d_next_named_xml_element_iterator = boost::none;
					return;
				}

				// Find the next XML element node with a matching name.
				d_named_xml_element_iterator.next_xml_node();
				find_xml_element_node_with_matching_name(d_named_xml_element_iterator);
			}


			/**
			 * Note: @a finished should be false when this is called.
			 */
			bool
			has_next() const
			{
				// In case 'has_next()' called multiple times in a row, we only want to increment once.
				if (!d_next_named_xml_element_iterator)
				{
					d_next_named_xml_element_iterator = d_named_xml_element_iterator;

					// Find the next XML element node with a matching name.
					d_next_named_xml_element_iterator->next_xml_node();
					find_xml_element_node_with_matching_name(d_next_named_xml_element_iterator.get());
				}

				return d_next_named_xml_element_iterator->node_iter != d_xml_nodes_end;
			}


			/**
			 * Returns true if finished iterating.
			 */
			bool
			finished() const
			{
				return d_named_xml_element_iterator.node_iter == d_xml_nodes_end;
			}


			/**
			 * Note: @a finished should be false, if this is to be called.
			 */
			XmlElementNode::non_null_ptr_type
			get_xml_element() const
			{
				return d_named_xml_element_iterator.element_node.get();
			}


			/**
			 * Returns the current XML node iterator.
			 */
			XmlNodeForwardIter
			get_xml_node_iterator() const
			{
				return d_named_xml_element_iterator.node_iter;
			}

		private:

			//! An iterator over XML element node's with matching names.
			struct NamedXmlElementIterator
			{
				explicit
				NamedXmlElementIterator(
						const XmlNodeForwardIter &node_iter_) :
					node_iter(node_iter_)
				{  }

				void
				next_xml_node()
				{
					++node_iter;
					element_node = boost::none;
				}

				XmlNodeForwardIter node_iter;
				boost::optional<XmlElementNode::non_null_ptr_type> element_node;
			};


			mutable XmlNodeUtils::XmlElementNodeExtractionVisitor d_xml_element_node_visitor;

			XmlNodeForwardIter d_xml_nodes_begin;
			XmlNodeForwardIter d_xml_nodes_end;

			/**
			 * The iterator to the current XML element node with matching name, if any.
			 */
			NamedXmlElementIterator d_named_xml_element_iterator;

			/**
			 * Only used when @a has_next is called - in which case it temporarily caches result of next iterator.
			 */
			mutable boost::optional<NamedXmlElementIterator> d_next_named_xml_element_iterator;


			/**
			 * Finds the XML element node with matching name starting at the specified iterator.
			 */
			void
			find_xml_element_node_with_matching_name(
					NamedXmlElementIterator &name_xml_element_iterator) const
			{
				for ( ;
					name_xml_element_iterator.node_iter != d_xml_nodes_end;
					name_xml_element_iterator.next_xml_node())
				{
					if ((name_xml_element_iterator.element_node =
							d_xml_element_node_visitor.get_xml_element_node(*name_xml_element_iterator.node_iter)))
					{
						break;
					}
				}
			}

		};


		/**
		 * Extracts text from a visited XML text node.
		 */
		class TextExtractionVisitor :
				public XmlNodeVisitor,
				private boost::noncopyable
		{
		public:
			explicit
			TextExtractionVisitor():
				d_encountered_subelement(false)
			{ }

			virtual
			void
			visit_element_node(
					const XmlElementNode::non_null_ptr_type &elem)
			{
				d_encountered_subelement = true;
			}

			virtual
			void
			visit_text_node(
					const XmlTextNode::non_null_ptr_type &text)
			{
				d_text += text->get_text();
			}

			bool
			encountered_subelement() const
			{
				return d_encountered_subelement;
			}

			const QString &
			get_text() const
			{
				return d_text;
			}

		private:

			QString d_text;
			bool d_encountered_subelement;
		};


		/**
		 * Returns the text string contents of the specified XML element node.
		 *
		 * NOTE: The returned string is *not* trimmed of whitespace at the beginning and end of the string.
		 *
		 * Returns boost::none if @a elem has any child nodes that are *element* nodes.
		 */
		boost::optional<QString>
		get_text_without_trimming(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		/**
		 * Returns the text string contents of the specified XML element node.
		 *
		 * NOTE: The returned string is trimmed of whitespace at the beginning and end of the string.
		 *
		 * Returns boost::none if @a elem has any child nodes that are *element* nodes.
		 */
		boost::optional<QString>
		get_text(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem);


		/**
		 * Reads the text string contents of the specified XML element node as a fully qualified name.
		 *
		 * An example usage:
		 *     boost::optional<PropertyName> type = get_qualified_xml_name<PropertyName>(...);
		 *
		 * Returns boost::none if:
		 *  - @a elem has any child nodes that are *element* nodes, or
		 *  - the namespace alias part of the text string is not recognised, or
		 *  - the unqualified name part of the text string is empty.
		 */
		template <class QualifiedXmlNameType>
		boost::optional<QualifiedXmlNameType>
		get_qualified_xml_name(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
		{
			boost::optional<QString> qualified_xml_string = get_text(elem);
			if (!qualified_xml_string)
			{
				return boost::none;
			}

			// First chunk before a ':'.
			const QString namespace_alias = qualified_xml_string->section(':', 0, 0);
			boost::optional<QString> ns = elem->get_namespace_from_alias(namespace_alias);
			if (!ns)
			{
				return boost::none;
			}

			// The chunk after the ':'.
			const QString name = qualified_xml_string->section(':', 1);
			if (name.isEmpty())
			{
				return boost::none;
			}

			return QualifiedXmlNameType(ns.get(), namespace_alias, name);
		}
	}
}

#endif // GPLATES_MODEL_XMLNODEUTILS_H
