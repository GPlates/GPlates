/* $Id$ */

/**
 * @file 
 * Interface for a DOM-like XML parser.  
 * This is a C++ wrapper for eXpat.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _GPLATES_FILEIO_XMLPARSER_H_
#define _GPLATES_FILEIO_XMLPARSER_H_

extern "C" {
#include <expat.h>
}
#include <iosfwd>
#include <string>
#include <utility>  /* std::pair */
#include <map>
#include <list>

namespace GPlatesFileIO
{
	/**
	 * XMLParser is a simple DOM-like wrapper around the
	 * eXpat XML parser [http://expat.sourceforge.net].
	 */
	class XMLParser
	{
		public:
			class Element;

			/**
			 * Convert the given istream into an XML document tree.
			 * If parsing was successful, returns the root Element of
			 * the tree, otherwise FileFormatException is thrown.
			 * Cleanup of returned Element and all of its children
			 * is the obligation OF THE CALLER.
			 * 
			 * @param is The stream from which to construct the XML 
			 *   document tree.
			 * @return A (valid) pointer to the root of the XML document tree.
			 * @throw FileFormatException Thrown when a parsing error occurs.
			 * @pre is.good() is true -- i.e. the stream is in a valid state.
			 */
			static Element*
			Parse(std::istream& is);

		private:
			XMLParser();  /*< Prevent construction. */
	};
	
	/**
	 * The main node in the document tree.  Has a link to the
	 * parent and a list of links to the children (list is 
	 * empty for leaf nodes, parent is NULL for root node).
	 */
	class XMLParser::Element
	{
		public:
			/**
			 * A name/value pair.
			 */
			typedef std::pair< std::string, 
							   std::string > Attribute_type;
						   
			/**
			 * A list of Elements; used when returning those elements
			 * that match a given Element name query.
			 * @see GetChild().
			 */
			typedef std::list< const Element* > ElementList_type;

			/**
			 * @name Map Types
			 * ElementMap_type and AttributeMap_type will be used to store
			 * the data for an Element.  Ideally they would be hash_maps,
			 * but unfortunately there is no standard hash_map in the STL.
			 */
			/* @{ */

			/**
			 * Maps Element names to a list of the corresponding Element 
			 * pointers.  This container is used to hold the children.
			 */
			typedef std::map< std::string, ElementList_type > ElementMap_type;
	
			/**
			 * Maps Attribute_type names to the corresponding 
			 * Attribute_types.
			 */
			typedef std::map< std::string, Attribute_type > AttributeMap_type;
			/* @} */
	
			/**
			 * Create an Element in the XML document tree that has
			 * the given @a name, and which begins on the given
			 * @a line_num.  The Element initially has no children,
			 * no attributes and no parent.
			 */
			Element(const std::string& name, unsigned int line_num)
				: _name(name), _parent(NULL), _line_num(line_num)
			{  }

			~Element();
		
			std::string
			GetName() const { return _name; }

			unsigned int
			GetLineNumber() const { return _line_num; }

			std::string
			GetContent() const { return _content; }

			std::string&
			GetContent() { return _content; }

			/**
			 * Get the Attribute that has the 
			 * given @a name.  The bool of the pair is true if the
			 * requested attribute was found.
			 */
			std::pair< Attribute_type, bool >
			GetAttribute(const std::string& name) const { 
				AttributeMap_type::const_iterator attr = _attributes.find(name); 
				return attr == _attributes.end() ?
					std::make_pair(Attribute_type(), false) :
					std::make_pair(attr->second, true);
			}

			/**
			 * Insert an Attribute into the map of Attributes.
			 * @return true if the operation was successful, or false
			 *   if an attribute with the same name was already present.
			 */
			bool
			InsertAttribute(const Attribute_type& attr) {
				return _attributes.insert(
					std::make_pair(attr.first, attr)).second;
			}

			Element*
			GetParent() const { return _parent; }

			void
			SetParent(Element* parent) { _parent = parent; }

			/**
			 * Get a list of child Elements of this Element whose names
			 * are @a name.
			 * @param name The name of the child Element.
			 * @return A list (possibly empty) of Elements whose names 
			 *   are @a name.
			 */
			ElementList_type
			GetChildren(const std::string& name) const {
				ElementMap_type::const_iterator child = _children.find(name);
				return child == _children.end() ? 
					ElementList_type() : child->second;
			}

			void
			InsertChild(const Element* element) {
				_children.insert(
					std::make_pair(element->GetName(),
					               ElementList_type())
				).first->second.push_back(element);
			}
			
		private:
			std::string			_name;
			AttributeMap_type	_attributes;
			std::string			_content;
	
			Element*		_parent;
			ElementMap_type	_children;    /*< sub-elements */
			unsigned int	_line_num;
	};
}

#endif  /* _GPLATES_FILEIO_XMLPARSER_H_ */
