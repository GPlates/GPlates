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

#include <cstring>
#include "XMLParser.h"
#include "global/Assert.h"

using namespace GPlatesFileIO;
typedef XMLParser::Element Element;

// eXpat Callback functions.  Userdata is a pointer to a pointer to
// the current Element.
static void
StartElementHandler(void* userdata, 
					const XML_Char*  name,
					const XML_Char** attrs)
{
	Element* result = new Element;
	result->_name = new std::string(name);
	
	result->_attributes = new XMLParser::Element::AttributeList;
	const XML_Char* attr = *attrs;
	for ( ; attr; attr += 2) {
		result->_attributes->push_back(
			std::make_pair(std::string(attr), std::string(attr + 1)));
	}
	
	Element* current = *static_cast<Element**>(userdata);
	result->_parent = current;
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
	*current->_content += content;
}


XMLParser::XMLParser()
	: _root(NULL), _parser(NULL)
{  }

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

	// XXX The following should be done with
	//   XML_ParserReset(_parser, NULL);
	// but that function is only a recent addtion to expat.
	
	if (_parser)
		XML_ParserFree(_parser);
	
	_parser = XML_ParserCreate(NULL);
	if (!_parser) {
		std::cerr << "Could not allocate memory to create parser." 
			<< std::endl;
		exit(-1);
	}

	// Set up callbacks
	XML_SetElementHandler(_parser,
						  &StartElementHandler,
						  &EndElementHandler);
	XML_SetCharacterDataHandler(_parser, &CharacterDataHandler);
	XML_SetUserData(_parser, &_root);

	static const size_t BUFFER_SIZE = 8192;
	int done;
	char buf[BUFFER_SIZE];
	while (istr.read(&buf[0], BUFFER_SIZE)) {
		size_t len = std::strlen(&buf[0]);
		done = len < BUFFER_SIZE;
		if (XML_Parse(_parser, buf, len, done) == 0) {
			std::cerr << XML_ErrorString(XML_GetErrorCode(_parser))
				<< " at line " << XML_GetCurrentLineNumber(_parser)
				<< std::endl;
			exit(-1);
		}
	}
	GPlatesGlobal::Assert(done, 
		std::string("Error: Parse loop exited before finish."));
	
	return _root;
}

XMLParser::Element::~Element()
{
	if (_name)
		delete _name;
	if (_attributes)
		delete _attributes;
	if (_children) {
		ElementList::iterator iter = _children->begin();
		for ( ; iter != _children->end(); ++iter)
			delete *iter;

		delete _children;
	}
	if (_content)
		delete _content;
}
