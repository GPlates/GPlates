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
 *   Mike Dowman <mdowman@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <sstream>
#include <utility>
#include "PlatesRotationParser.h"
#include "FileFormatException.h"


using namespace GPlatesFileIO;
using namespace GPlatesFileIO::PlatesParser;


void 
ReadInPlateRotationData(const char *filename, std::istream &input_stream,
	RotationDataMap &rotation_data) {

	/*
	 * The input stream must point to the beginning of an already-opened
	 * istream containing plates rotation data.
	 */

	LineBuffer lb(input_stream, filename);

	// Each file can contain multiple rotation lines.
	while ( ! lb.eof()) {

		ReadRotation(lb, rotation_data);
	}
}


/**
 * Given a LineBuffer, read a single rotation and store it.
 */
void
ReadRotation(LineBuffer &lb, RotationDataMap &rotation_data) {

	std::string line;
	ReadRotationLine(lb, line);

	// Test for EOF
	if (lb.eof()) {

		return;
	}

	FiniteRotation rot = ParseRotationLine(lb, line);

	plate_id_t plate_id = rot._moving_plate;
	RotationDataMap::iterator it = rotation_data.find(plate_id);
	if (it == rotation_data.end()) {

		/*
		 * A FiniteRotationsOfPlateMap of this plate_id does not yet
		 * exist in the map -- make it so.
		 */
		std::pair< RotationDataMap::iterator, bool > pos_and_status =
		 rotation_data.insert(RotationDataMap::value_type(plate_id,
		  std::multimap< fpdata_t, FiniteRotation >()));

		it = pos_and_status.first;
	}
	fpdata_t time = rot._time;
	it->second.insert(FiniteRotationsOfPlateMap::value_type(time, rot));
}


/**
 * A reasonable maximum length for each line of the rotation data.
 * This length does not include a terminating character.
 */
static const size_t ROTATION_LINE_LEN = 120;


void
ReadRotationLine(LineBuffer &lb, std::string &str) {

	static char buf[ROTATION_LINE_LEN + 1];

	if ( ! lb.getline(buf, sizeof(buf))) {

		/*
		 * For some reason, the read was considered "unsuccessful".
		 * This might be because we have reached EOF.
		 *
		 * Test whether we have reached EOF, and if so, return.
		 */
		if (lb.eof()) return;

		// else, there *was* an unexplained failure
		std::ostringstream oss("Unsuccessful read from ");
		oss << lb
		 << " while attempting to read a rotation line";

		throw FileFormatException(oss.str().c_str());
	}
	str = std::string(buf);
}
