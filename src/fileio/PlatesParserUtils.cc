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

using namespace GPlatesFileIO;
using namespace GPlatesFileIO::PlatesParser;


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


rgid_t
attemptToReadRGID(const LineBuffer &lb, std::istringstream &iss,
	const char *desc) {

	rgid_t r;

	if ( ! (iss >> r)) {

		// For some reason, unable to read an rgid
		std::ostringstream oss("Unable to extract an rgid from ");
		oss << lb
		 << " while attempting to parse the "
		 << desc;

		throw FileFormatException(oss.str().c_str());
	}
	return r;
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
