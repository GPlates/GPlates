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

// Xerces C++ Includes.
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/validators/common/Grammar.hpp>

#include "XMLString.h"
#include "GPlatesReader.h"
#include "GPlatesFileHandler.h"
#include "ErrorHandler.h"

using namespace GPlatesFileIO;
using namespace GPlatesGeo;
using xercesc::XMLPlatformUtils;
using xercesc::XMLException;
using xercesc::SAXParser;
using xercesc::SAXException;

/**
 * Return true if the parser was initialised successfully, false
 * otherwise.
 * Sets the parameter to a newly allocated SAXParser.
 */
static bool
InitialiseParser(SAXParser*& parser)
{
	try {
		XMLPlatformUtils::Initialize();
	} catch (const XMLException& ex) {
		std::cerr << "Error during XML parser initialisation: "
			<< XMLString(ex.getMessage()) << std::endl
			<< "No data loaded!" << std::endl;
		return false;
	}

	parser = new SAXParser;

	// Enable schema processing.
	// FIXME: Load the GPlates App Schema explicitly.
	parser->setValidationScheme(SAXParser::Val_Always);
	parser->setDoNamespaces(true);
	parser->setDoSchema(true);
	// Setting this to true makes GML barf at the moment.
	parser->setValidationSchemaFullChecking(false);
	parser->setValidationConstraintFatal(true);
	parser->setExitOnFirstFatalError(true);

	return true;
}

void
GPlatesReader::Read(DataGroup& group)
{
	SAXParser* parser;

	if (!InitialiseParser(parser))
		return;
	
	// Set handlers
	GPlatesFileHandler doc_handler;
	parser->setDocumentHandler(&doc_handler);

	ErrorHandler err_handler;
	parser->setErrorHandler(&err_handler);
	// Reset error count.
	err_handler.resetErrors();

	try {
		// FIXME: Use XMLPlatformUtils to convert filepath
		// properly.
		parser->parse(_filepath.c_str());  

	} catch (const XMLException& ex) {
		std::cerr << "XML Error whilst parsing: " 
		 << XMLString(ex.getMessage()) << std::endl;
	} catch (const SAXException& ex) {
		std::cerr << "SAX Error whilst parsing: " 
		 << XMLString(ex.getMessage()) << std::endl;
	} catch (...) {
		std::cerr << "Unexpected exception during parsing."
		 << std::endl;
	}
	// The parser must be deleted before Terminate is called.
	delete parser;

	// Call the termination method
	XMLPlatformUtils::Terminate();
}
