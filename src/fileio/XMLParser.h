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
#include <utility>  /* std::pair */
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
			struct Element;

			/**
			 * Constructor creates the eXpat parser.
			 */
			XMLParser();

			/**
			 * Destructor frees the eXpat parser and all of the
			 * elements created during the parse.  Thus a pointer
			 * to a document root returned from Parse() will be
			 * invalid after the XMLParser is deleted.
			 */ 
			~XMLParser();

			/**
			 * Convert the given istream into an XML document tree.
			 * If parsing was successful, returns the root Element of
			 * the tree, otherwise FileFormatException is thrown.
			 * Cleanup of returned Element and all of its children will
			 * be done by the parser.
			 */
			const Element*
			Parse(std::istream&);

		private:
			Element* _root;     /*< Root of the XML document tree. */
			XML_Parser _parser;	/*< eXpat parser instance. */

			/**
			 * Perform various initialisation tasks that are required
			 * for the parser to be in a state where parsing can begin.
			 */
			void
			Initialise();
	};
	
	/**
	 * The main node in the document tree.  Has a link to the
	 * parent and a list of links to the children (list is 
	 * empty for leaf nodes, parent is NULL for root node).
	 */
	struct XMLParser::Element
	{
		/**
		 * A name/value pair.
		 */
		typedef std::pair< const std::string, 
						   const std::string >  Attribute;
		typedef std::list< Attribute >          AttributeList;

		/**
		 * Container to hold the children.
		 */
		typedef std::list< const Element* >  ElementList;

		Element(const char* name)
			: _name(name), _parent(NULL)
		{  }

		~Element();
		
		const std::string	_name;
		AttributeList		_attributes;
		std::string			_content;

		Element*		_parent;
		ElementList		_children;    /*< sub-elements */
	};
}

#endif  /* _GPLATES_FILEIO_XMLPARSER_H_ */
