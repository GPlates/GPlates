/* $Id$ */

/**
 * @file 
 * Implementation of the XML parser.
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

#include <string>
#include <iostream>
#include <iterator>  /* for istream_iterator */
#include <algorithm> /* for unique_copy */
#include <sstream>

#include <cstring>
#include "XMLParser.h"
#include "FileFormatException.h"
#include "global/Assert.h"

using namespace GPlatesFileIO;
typedef XMLParser::Element Element;

/**
 * Returns true when both of the parameters are space
 * characters.
 */
static bool
cmp_space(char c1, char c2) 
{
	return isspace(c1) && isspace(c2);
}

/**
 * Replace contiguous blocks of whitespace with a single space
 * character.
 */
static void
CompressWhitespace(std::string& str)
{
	// Copy blocks of whitespace as a single space using the
	// BothWhitespace functor.
	str.erase(
		std::unique(str.begin(), str.end(), &cmp_space),
		str.end());
}

static XML_Parser parser;

// eXpat Callback functions.  Userdata is a pointer to a pointer to
// the current Element.

/**
 * Called for start tags, such as \<datagroup> or <meta ...
 *
 * attrs is an array of XML_Char* strings, arranged in name/value
 * pairs.  Thus it will always have an even length.
 */
static void
StartElementHandler(void* userdata, 
					const XML_Char*  name,
					const XML_Char** attrs)
{
	// XXX XXX XXX
	Element* result = new Element(name, XML_GetCurrentLineNumber(parser));
	
	// For each pair of attributes...
	for (size_t i = 0; attrs[i]; i += 2) {
		result->InsertAttribute(
			std::make_pair(std::string(attrs[i]), 
						   std::string(attrs[i + 1]))
		);
	}
	
	Element** current = static_cast<Element**>(userdata);

	// Push the new Element onto the stack.
	result->SetParent(*current);
	*current = result;
}

/**
 * Called for an end tag, such as \</datagroup> or ... />.
 */
static void
EndElementHandler(void* userdata, const XML_Char* name)
{
	Element* current = *static_cast<Element**>(userdata);

	// The name of the current Element should be the same
	// as the terminating element name (name).  If it isn't,
	// then my stack is broken (i.e. out of sync).
	if (std::string(name) != current->GetName()) {
		std::cerr << "Error: Parser: Broken stack." << std::endl;
		std::cerr << "current name: " << name << std::endl
			<< "current line: " << XML_GetCurrentLineNumber(parser) << std::endl
			<< "element name: " << current->GetName() << std::endl
			<< "element line: " << current->GetLineNumber() << std::endl;
		exit(-1);
	}
	
	// Attempt to move back up the stack.
	if (current->GetParent())  { // Parent is NULL for the root only.
		// Add current element to the list of its parent's elements.
		current->GetParent()->InsertChild(current);

		// Return to parent (pop off stack).
		Element** newcurr = static_cast<Element**>(userdata);
		*newcurr = current->GetParent();
	}
}

/**
 * Called for character data within an element.  All whitespace
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
	std::string content(str, str + len);
	CompressWhitespace(content);
	
	Element* current = *static_cast<Element**>(userdata);
	if (!current) // document not started yet
		return;
	
	// Append previously seen content.
	current->GetContent() += content;
}

/**
 * Set the callbacks for the parser.
 */
static void
SetCallbacks(Element** root)
{
	// Register callbacks
	XML_SetElementHandler(parser,
						  &StartElementHandler,
						  &EndElementHandler);
	XML_SetCharacterDataHandler(parser, &CharacterDataHandler);

	// The user data is a pointer to the Element* currently being
	// parsed.  So it is of type Element**: a pointer to a pointer
	// to the current element.  The current element is initially the
	// document root.
	XML_SetUserData(parser, root);
}

Element*
XMLParser::Parse(std::istream& istr)
{
	static const size_t BUFFER_SIZE = 8192;
	static const XML_Char* ENCODING = NULL;
	
	Element *root = NULL;

	parser = XML_ParserCreate(ENCODING);
	if (!parser) {
		std::cerr << "Could not allocate memory to create parser." 
			<< std::endl;
		exit(-1);
	}

	SetCallbacks(&root);

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

		// This calculation relies on the memset above.
		len = std::strlen(&buf[0]);

		// done is true if we did not fill all of buf; i.e. we
		// finished reading the input before buf was full.
		done = (len < BUFFER_SIZE-1);  

		// XML_Parse calls the various handlers registered in
		// XMLParser::Initialise() and does syntax checking.
		if (XML_Parse(parser, buf, len, done) == 0) {

			// Handle a file format error.
			std::ostringstream oss;
			oss << "Parse error: " << std::endl
				<< XML_ErrorString(XML_GetErrorCode(parser))
				<< " at line " << XML_GetCurrentLineNumber(parser)
				<< std::endl;
			
			// Ensure proper cleanup
			XML_ParserFree(parser);
			if (root)
				delete root;

			throw FileFormatException(oss.str().c_str());
		}
	} while (!done);

	// done must be true, because we exited the loop.  So if we
	// are not at the end of the input then something bad has
	// happened.
	if (!istr.eof()) {
		std::ostringstream oss;
		oss << "Error: Parse loop exited before EOF." << std::endl;

		// Ensure proper cleanup
		XML_ParserFree(parser);  
		if (root)
			delete root;

		throw FileFormatException(oss.str().c_str());
	}
	
	XML_ParserFree(parser);
	return root;
}

Element::~Element()
{
	ElementMap_type::iterator iter = _children.begin();
	for ( ; iter != _children.end(); ++iter)
	{
		ElementList_type::iterator jter = iter->second.begin();
		for ( ; jter != iter->second.end(); ++jter)
			delete *jter;
	}
}
