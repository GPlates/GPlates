/* $Id$ */

/**
 * @file 
 * Interface for a DOM-like XML parser.  
 * This is a C++ wrapper for eXpat.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_XMLPARSER_H_
#define _GPLATES_FILEIO_XMLPARSER_H_

extern "C" {
#include <expat.h>
}
#include <iostream>
#include <string>
#include <list>

namespace GPlatesFileIO
{
	class XMLParser
	{
		public:
			struct Element;

			XMLParser();

			~XMLParser();

			/**
			 * Convert the given istream into an XML document tree.
			 * If parsing was successful, returns the root Element of
			 * the tree, otherwise NULL is returned.
			 * Cleanup of returned Element and all of its children will
			 * be done by the parser.
			 */
			Element*
			Parse(std::istream&);

		private:
			Element* _root;     /*< Root of the XML document tree. */
			XML_Parser _parser;	/*< eXpat parser instance. */
	};
	
	struct XMLParser::Element
	{
		typedef std::pair<const std::string, 
						  const std::string> Attribute;
		typedef std::list<Attribute> 		 AttributeList;
		typedef std::list<Element*> 		 ElementList;

		Element()
			: _name(NULL), _attributes(NULL), _parent(NULL), 
			  _children(NULL), _content(NULL) {  }

		~Element();
		
		std::string*	_name;
		AttributeList*	_attributes;
		Element*		_parent;
		ElementList*	_children;    /*< sub-elements */
		std::string*	_content;
	};
}

#endif  /* _GPLATES_FILEIO_XMLPARSER_H_ */
