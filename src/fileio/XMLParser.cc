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
#include <iostream>
#include <iomanip>

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
	std::cout << "Entered\t" << __PRETTY_FUNCTION__ << std::endl;
	Element* result = new Element(static_cast<const char*>(name));
	
	std::cout << "\tElement name: " << *result->_name << std::endl;
	for (size_t i = 0; attrs[i]; i += 2) {
		result->_attributes->push_back(
			std::make_pair(std::string(attrs[i]), 
						   std::string(attrs[i + 1])));
	}
	
	Element** current = static_cast<Element**>(userdata);
	result->_parent = *current;
	*current = result;
	std::cout << "Left\t" << __PRETTY_FUNCTION__ << std::endl;
}

static void
EndElementHandler(void* userdata, const XML_Char* name)
{
	std::cout << "Entered\t" << __PRETTY_FUNCTION__ << std::endl;
	std::cout << "\tElement name: " << name << std::endl;

	Element* current = *static_cast<Element**>(userdata);

#if 0	
	GPlatesGlobal::Assert(std::string(name) == *current->_name, 
		std::string("Broken stack."));
#endif
	
	if (current->_parent)  { // Parent is NULL for the root only.
		// Add current element to the list of its parent's elements.
		current->_parent->_children->push_back(current);

		// Return to parent.
		Element** newcurr = static_cast<Element**>(userdata);
		*newcurr = current->_parent;
	}
	std::cout << "Left\t" << __PRETTY_FUNCTION__ << std::endl;
}

static void
CharacterDataHandler(void* userdata, const XML_Char* str, int len)
{
	std::cout << "Entered\t" << __PRETTY_FUNCTION__ << std::endl;

	const std::string content(str, str+len);
	
	Element* current = *static_cast<Element**>(userdata);
	if (!current) // document not started yet
		return;
	
	*current->_content += content;
	
	std::cout << "Left\t" << __PRETTY_FUNCTION__ << std::endl;
}


XMLParser::XMLParser()
	: _root(NULL), _parser(NULL)
{  }

XMLParser::~XMLParser()
{
	std::cout << "1 >>> " << __PRETTY_FUNCTION__ << std::endl;

	delete _root;
	
	std::cout << "2 >>> " << __PRETTY_FUNCTION__ << std::endl;

	XML_ParserFree(_parser);

	std::cout << "3 >>> " << __PRETTY_FUNCTION__ << std::endl;
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
	XML_SetUserData(_parser, static_cast<void *>(&_root));

	static const size_t BUFFER_SIZE = 8192;
	bool done;
	char buf[BUFFER_SIZE];
	do {
		std::memset(&buf[0], 0, BUFFER_SIZE*sizeof(char));
		istr.read(&buf[0], BUFFER_SIZE-1);  // -1: space for \0.
		size_t len = std::strlen(&buf[0]);
		done = (len < BUFFER_SIZE-1);  // done if we did not fill all of buf
		if (XML_Parse(_parser, buf, len, done) == 0) {
			std::cerr << XML_ErrorString(XML_GetErrorCode(_parser))
				<< " at line " << XML_GetCurrentLineNumber(_parser)
				<< std::endl;
			exit(-1);
		}
	} while (!done);

#if 0
	GPlatesGlobal::Assert(istr.eof(), 
		std::string("Error: Parse loop exited before EOF."));
#endif
	
	return _root;
}

XMLParser::Element::~Element()
{
	std::cout << ">>> " << __PRETTY_FUNCTION__ << std::endl;
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
