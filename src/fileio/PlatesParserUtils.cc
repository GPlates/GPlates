/* $Id$ */

/**
 * \file 
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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include "PlatesParserUtils.h"
#include "FileFormatException.h"

//using namespace GPlatesFileIO;
//using namespace GPlatesFileIO::PlatesParser;

// Don't ask me why, but for some reason explicitly including the
// namespaces like this solves an incomprehensible linking error
// that was occuring.
namespace GPlatesFileIO {
	namespace PlatesParser {

int
attemptToReadInt(const LineBuffer &lb, std::istringstream &iss,
	const char *desc) {

	int i;

	if ( ! (iss >> i)) {

		// For some reason, unable to read an int
		std::ostringstream oss("Unable to extract an int from ");
		oss << lb
		 << " while attempting to parse the "
		 << desc;

		throw FileFormatException(oss.str().c_str());
	}
	return i;
}


fpdata_t
attemptToReadFloat(const LineBuffer &lb, std::istringstream &iss,
	const char *desc) {

	fpdata_t f;

	if ( ! (iss >> f)) {

		// For some reason, unable to read a float
		std::ostringstream oss("Unable to extract a float from ");
		oss << lb
		 << " while attempting to parse the "
		 << desc;

		throw FileFormatException(oss.str().c_str());
	}
	return f;
}


std::string
attemptToReadString(const LineBuffer &lb, std::istringstream &iss,
	const char *desc) {

	std::string s;

	if ( ! (iss >> s)) {

		// For some reason, unable to read a string
		std::ostringstream oss("Unable to extract a string from ");
		oss << lb
		 << " while attempting to parse the "
		 << desc;

		throw FileFormatException(oss.str().c_str());
	}
	return s;
}


plate_id_t
attemptToReadPlateID(const LineBuffer &lb, std::istringstream &iss,
	const char *desc) {

	plate_id_t p;

	if ( ! (iss >> p)) {

		// For some reason, unable to read a plate id
		std::ostringstream oss("Unable to extract plate id from ");
		oss << lb
		 << " while attempting to parse the "
		 << desc;

		throw FileFormatException(oss.str().c_str());
	}
	return p;
}


int
attemptToReadPlotterCode(const LineBuffer &lb, std::istringstream &iss) {

	int i;

	if ( ! (iss >> i)) {

		// For some reason, unable to read an int
		std::ostringstream oss("Unable to extract an integer from ");
		oss << lb
		 << " while attempting to parse the plotter code of a point";

		throw FileFormatException(oss.str().c_str());
	}
	return i;
}

	}  // end namespace PlatesParser
}  // end namespace GPlatesFileIO
