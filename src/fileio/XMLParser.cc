/* $Id$ */

/**
 * @file 
 * Implementation of the XML parser.
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

#include <string>

#include "XMLParser.h"

using namespace GPlatesFileIO;
using XMLParser::Element;

// eXpat Callback functions.  Userdata is a pointer to a pointer to
// the current Element.
static void
StartElementHandler(void* userdata, 
					const XML_Char*  name,
					const XML_Char** attrs)
{
	using XMLParser::Element::AttributeList;
   
	Element* result = new Element;
	result->name = new std::string(name);
	
	result->attributes = new AttributeList;
	const XML_Char* attr = *attrs;
	for ( ; attr; attr += 2) {
		attrlist->push_back(
			std::make_pair(std::string(attr), std::string(attr + 1)));
	}
	
	Element* current = *static_cast<Element**>(userdata);
	result->_parent = parent;
	current = result;
	
}

void
EndElementHandler(void* userdata, const XML_Char* name)
{
	Element* current = *static_cast<Element**>(userdata);
	if (current->_parent)  { // Parent is NULL for the root only.
		// Add current element to the list of its parent's elements.
		current->_parent->_children->push_back(current);

		// Return to parent.
		current = current->_parent;
	}
}

void
CharacterDataHandler(void* userdata, const XML_Char* str, int len)
{
	const std::string content(str, len);
	Element* current = *static_cast<Element**>(userdata);
	current->_content += content;
}


XMLParser::XMLParser()
	: _root(NULL)
{
	_parser = XML_ParserCreate();
	if (!_parser) {
		std::cerr << "Could not allocate memory to create parser." 
			<< std::endl;
		exit(-1);
	}
}

XMLParser::~XMLParser()
{
	delete _root;
	
	XML_ParserFree(_parser);
}

Element*
XMLParser::Parse(std::istream& istr)
{
	if (_root)
		delete _root;
	_root = NULL;

	XML_ParserReset(_parser);

	// Set up callbacks
	XML_SetElementHandler(_parser,
						  &StartElementHandler,
						  &EndElementHandler);
	XML_SetCharacterDataHandler(_parser, &CharacterDataHandler);
	XML_SetUserData(_parser, &_root);
	
	return _root;
}

XMLParser::Element::~Element()
{
	if (_name)
		delete name;
	if (_attributes) {
		AttributeList::iterator iter = _attributes->begin();
		for ( ; iter != _attributes->end(); ++iter)
			delete *iter;
	}
	if (_elements) {
		ElementList::iterator iter = _elements->begin();
		for ( ; iter != _elements->end(); ++iter)
			delete *iter;
	}
	if (_content)
		delete _content;
}
