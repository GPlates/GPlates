/* $Id$ */

/**
 * @file 
 * File specific comments.
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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */
#include <iostream>
#include <xercesc/sax/Locator.hpp>
#include "GPlatesFileHandler.h"
#include "XMLString.h"

using namespace GPlatesFileIO;

/**
 * For some reason the Locator never seems to be set,
 * hence there is nothing to print out.
 */
#if 0
void
Print(std::ostream& os, const xercesc::Locator* loc)
{
	if (!loc) {
		os << "Locator is not defined." << std::endl;
	} else {
		os <<  "Public Id: " << XMLString(loc->getPublicId()) << std::endl
			<< "System Id: " << XMLString(loc->getSystemId()) << std::endl
			<< "Line:      " << loc->getLineNumber() << std::endl
			<< "Column:    " << loc->getColumnNumber() << std::endl;
	}
}
#endif

void
GPlatesFileHandler::endDocument()
{
	// Blah.
}

// FIXME: need a way to find out the actual element values.
void
GPlatesFileHandler::startElement(const XMLCh* const name, 
 xercesc::AttributeList& attrs)
{
	std::cout << "GPlatesFileHandler::startElement("
	 << XMLString(name) << "):" << std::endl;
	size_t len = attrs.getLength();
	for (size_t i = 0; i < len; ++i) {
		std::cout << "\t" << XMLString(attrs.getName(i)) << std::endl;
		std::cout << "\t = " << XMLString(attrs.getValue(i)) << std::endl;
	}
	std::cout << std::endl;
}

void
GPlatesFileHandler::endElement(const XMLCh* const name)
{
	std::cout << "GPlatesFileHandler::endElement("
	 << XMLString(name) << ")." << std::endl << std::endl;
}
