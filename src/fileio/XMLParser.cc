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
#include <locale>
#include <iterator>  /* for istream_iterator */
#include <algorithm> /* for unique_copy */
#include <sstream>

#include <cstring>
#include "XMLParser.h"
#include "global/Assert.h"

using namespace GPlatesFileIO;
typedef XMLParser::Element Element;

/**
 * Binary functor (predicate) for use with std::unique_copy.
 */
class BothWhitespace
{
	public:
		BothWhitespace(const std::locale& loc)
			: _loc(loc)  {  }

		/**
		 * Returns true when both of the parameters are space
		 * characters.
		 */
		bool
		operator()(char c1, char c2) {
			return std::isspace(c1, _loc) && std::isspace(c2, _loc);
		}

	private:
		const std::locale& _loc;
};


/**
 * Replace contiguous blocks of whitespace with a single space
 * character.
 *
 * Based on an example from N. Josuttis.
 */
static void
CompressWhitespace(std::string& str)
{
	// Create a stream representation of the parameter.
	std::istringstream istr(str);
	
	// Don't skip leading whitespaces.
	istr.unsetf(std::ios_base::skipws);

	// Copy from the stringstream back into our parameter (str).
	// Copy blocks of whitespace as a single space using the
	// BothWhitespace functor.
	std::unique_copy(std::istream_iterator<char>(istr),
					 std::istream_iterator<char>(),
					 std::back_inserter(str),
					 BothWhitespace(istr.getloc()));
}

// eXpat Callback functions.  Userdata is a pointer to a pointer to
// the current Element.

/**
 * Called for start tags, such as <datagroup> or <meta ...
 *
 * attrs is an array of XML_Char* strings, arranged in name/value
 * pairs.  Thus it will always have an even length.
 */
static void
StartElementHandler(void* userdata, 
					const XML_Char*  name,
					const XML_Char** attrs)
{
	Element* result = new Element(name);
	
	// For each pair of attributes...
	for (size_t i = 0; attrs[i]; i += 2) {
		result->_attributes.push_back(
			std::make_pair(std::string(attrs[i]), 
						   std::string(attrs[i + 1])));
	}
	
	Element** current = static_cast<Element**>(userdata);

	// Push the new Element onto the stack.
	result->_parent = *current;
	*current = result;
}

/**
 * Called for an end tag, such as </datagroup> or ... />.
 */
static void
EndElementHandler(void* userdata, const XML_Char* name)
{
	Element* current = *static_cast<Element**>(userdata);

	// The name of the current Element should be the same
	// as the terminating element name (name).  If it isn't,
	// then my stack is broken (i.e. out of sync).
	if (std::string(name) != current->_name) {
		std::cerr <<  "Error: Parser: Broken stack." << std::endl;
		exit(-1);
	}
	
	// Attempt to move back up the stack.
	if (current->_parent)  { // Parent is NULL for the root only.
		// Add current element to the list of its parent's elements.
		current->_parent->_children.push_back(current);

		// Return to parent (pop off stack).
		Element** newcurr = static_cast<Element**>(userdata);
		*newcurr = current->_parent;
	}
}

/**
 * Called when character data within an element.  All whitespace
 * blocks are compressed into a single space character.  There will
 * be no markup in str.
 *
 * *userdata is not modified, but the underlying Element it points
 * to is.
 *
 * Quoting from the eXpat page:
 *   "A single block of contiguous text free of markup may still result in a
 *   sequence of calls to this handler."
 * For this reason, the content (str) is appended to the content of the
 * current element.
 */
static void
CharacterDataHandler(void* userdata, const XML_Char* str, int len)
{
	std::string content(str, str+len);
	CompressWhitespace(content);
	
	Element* current = *static_cast<Element**>(userdata);
	if (!current) // document not started yet
		return;
	
	// Append previously seen content.
	current->_content += content;
}

void
XMLParser::Initialise()
{
	static const XML_Char* ENCODING = NULL;

	// If the parser has already been run, then _root will still be
	// set from the last run.  So get rid of it.
	if (_root) {
		delete _root;
		_root = NULL;
	}

	// XXX The following should be done with
	//   XML_ParserReset(_parser, ENCODING);
	// but that function is only a recent addtion to expat.
	
	if (_parser)
		XML_ParserFree(_parser);
	
	_parser = XML_ParserCreate(ENCODING);
	if (!_parser) {
		std::cerr << "Could not allocate memory to create parser." 
			<< std::endl;
		exit(-1);
	}

	// Register callbacks
	XML_SetElementHandler(_parser,
						  &StartElementHandler,
						  &EndElementHandler);
	XML_SetCharacterDataHandler(_parser, &CharacterDataHandler);

	// The user data is a pointer to the Element* currently being
	// parsed.  So it is of type Element**: a pointer to a pointer
	// to the current element.  The current element is initially the
	// document root.
	XML_SetUserData(_parser, &_root);
}

XMLParser::XMLParser()
	: _root(NULL), _parser(NULL)
{
	Initialise();
}

XMLParser::~XMLParser()
{
	delete _root;

	XML_ParserFree(_parser);
}

const Element*
XMLParser::Parse(std::istream& istr)
{
	static const size_t BUFFER_SIZE = 8192;

	// Clear any previous parsing information.
	Initialise();

	bool done;
	size_t len;
	char buf[BUFFER_SIZE];
	do {
		// istream::read doesn't zero-terminate the string,
		// so we set all the data to zero thus ensuring that
		// the string can be read as a normal C string.
		std::memset(&buf[0], 0, BUFFER_SIZE*sizeof(char));

		// Read a block of input.  -1 leaves space for \0.
		istr.read(&buf[0], BUFFER_SIZE-1);  

		// This calculation counts on the memset above.
		len = std::strlen(&buf[0]);

		// done is true if we did not fill all of buf; i.e. we
		// finished reading the input before buf was full.
		done = (len < BUFFER_SIZE-1);  

		// XML_Parse calls the various handlers registered in
		// XMLParser::Initialise() and does syntax checking.
		if (XML_Parse(_parser, buf, len, done) == 0) {
			// Handle a syntax error.
			std::cerr << XML_ErrorString(XML_GetErrorCode(_parser))
				<< " at line " << XML_GetCurrentLineNumber(_parser)
				<< std::endl
				<< "No data was loaded." << std::endl;
			return static_cast<const Element *>(NULL);
		}
	} while (!done);

	// done must be true, because we exited the loop.  So if we
	// are not at the end of the input then something bad has
	// happened.
	if (!istr.eof()) {
		std::cerr << "Error: Parse loop exited before EOF." << std::endl
			<< "No data was loaded." << std::endl;
		return static_cast<const Element *>(NULL);
	}
	
	return _root;
}

Element::~Element()
{
	ElementList::iterator iter = _children.begin();
	for ( ; iter != _children.end(); ++iter)
		delete *iter;  // *iter is an Element*.
}
