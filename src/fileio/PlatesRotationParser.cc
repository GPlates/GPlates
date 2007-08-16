/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2003, 2004, 2005, 2006,
 * 2007 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sstream>
#include <utility>
#include "PlatesRotationParser.h"
#include "FileFormatException.h"


//using namespace GPlatesFileIO;

namespace GPlatesFileIO {
	namespace PlatesParser {

void 
ReadInRotationData(const char *filename, std::istream &input_stream,
	PlatesRotationData &rotation_data) {

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


static const plate_id_t PLATE_ID_TO_IGNORE = 999;


/**
 * Given a LineBuffer, read a single rotation and store it.
 */
void
ReadRotation(LineBuffer &lb, PlatesRotationData &rotation_data) {

	std::string line;
	ReadRotationLine(lb, line);

	// Test for EOF
	if (lb.eof()) {

		return;
	}

	FiniteRotation rot = ParseRotationLine(lb, line);

	plate_id_t moving_plate = rot.d_moving_plate;
	plate_id_t fixed_plate = rot.d_fixed_plate;

	/*
	 * Ignore "commented-out" finite rotations.
	 */
	if (moving_plate == PLATE_ID_TO_IGNORE) return;

	/*
	 * If there is not yet any rotation data, this will be the start
	 * of the first rotation sequence.
	 */
	if (rotation_data.empty()) {

		// simply create a new rotation sequence and append it
		RotationSequence rot_seq(moving_plate, fixed_plate, rot);
		rotation_data.push_back(rot_seq);

		return;
	}
	// else, the rotation data must contain at least one rotation sequence.

	/*
	 * Now, will this finite rotation become part of an already-existing
	 * rotation sequence, or will it be the start of a new one?
	 *
	 * It all depends on its moving plate and fixed plate:  If they are
	 * the same as those of the most-recently-appended (ie, last) rotation
	 * sequence in the rotation data, then this finite rotation must be
	 * another item in that rotation sequence.  Else, it will become the
	 * start of a new rotation sequence.
	 */
	PlatesRotationData::reverse_iterator rit = rotation_data.rbegin();
	// recall that the rotation data must contain at least one element
	RotationSequence &last_rot_seq = *(rit);
	if (last_rot_seq.d_moving_plate == moving_plate &&
	    last_rot_seq.d_fixed_plate == fixed_plate) {

		// another item in this rotation sequence
		last_rot_seq.d_seq.push_back(rot);

	} else {

		// the start of a new rotation sequence
		RotationSequence rot_seq(moving_plate, fixed_plate, rot);
		rotation_data.push_back(rot_seq);
	}
}


void
ReadRotationLine(LineBuffer &lb, std::string &str) {

	if ( ! lb.getline(str)) {

		/*
		 * For some reason, the read was considered "unsuccessful".
		 * This might be because we have reached EOF.
		 *
		 * Test whether we have reached EOF, and if so, return.
		 */
		if (lb.eof()) return;

		// else, there *was* an unexplained failure
		std::ostringstream oss;
		oss << "Unsuccessful read from " << lb
		 << "\nwhile attempting to read a rotation line.";

		throw FileFormatException(oss.str().c_str());
	}
}

}  // end namespace PlatesParser
}  // end namespace GPlatesFileIO
