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
#include "PlatesBoundaryParser.h"
#include "FileFormatException.h"


//using namespace GPlatesFileIO;

namespace GPlatesFileIO {
	namespace PlatesParser {
		
void 
ReadInPlateBoundaryData(const char *filename, std::istream &input_stream,
	PlatesDataMap &plates_data) {

	/*
	 * The input stream must point to the beginning of an already-opened
	 * istream containing plates boundary data.
	 */
	LineBuffer lb(input_stream, filename);

	// Each file can contain multiple polylines.
	while ( ! lb.eof()) {

		ReadPolyLine(lb, plates_data);
	}
}


/**
 * Given a LineBuffer, read a single PolyLine and store it.
 */
void 
ReadPolyLine(LineBuffer &lb, PlatesDataMap &plates_data) {

	std::string first_line;
	ReadFirstLineOfPolyLineHeader(lb, first_line);

	/*
	 * Note that, if we had already read the last polyline
	 * before this function was invoked, then the 'getline' in the
	 * just-invoked 'ReadFirstLineOfPolyLineHeader' would have
	 * failed as it reached EOF.
	 *
	 * Hence, we should now test for EOF.
	 */
	if (lb.eof()) {

		return;
	}

	std::string second_line;
	ReadSecondLineOfPolyLineHeader(lb, second_line);

	PolyLineHeader header =
	 PolyLineHeader::ParseLines(lb, first_line, second_line);

	/*
	 * The rest of this polyline will consist of the actual points.
	 */
	PolyLine polyline(header);
	ReadPolyLinePoints(lb, polyline._points, header._num_points);

	/*
	 * Having read the whole polyline, we now insert it into its
	 * containing plate -- where a plate is identified by its plate_id.
	 */
	plate_id_t plate_id = header._plate_id;
	AppendPolyLineToPlatesData(plates_data, plate_id, polyline);
}


void
AppendPolyLineToPlatesData(PlatesDataMap &plates_data,
	const plate_id_t &plate_id, const PolyLine &pl) {

	/*
	 * If this is the first polyline read for this plate_id, the plate
	 * will not yet exist in the map of plates data.
	 */
	PlatesDataMap::iterator it = plates_data.find(plate_id);
	if (it == plates_data.end()) {

		/*
		 * A plate of this plate_id does not yet exist in the map
		 * -- make it so.
		 */
		Plate plate(plate_id);

		/*
		 * Note that it's more efficient to insert the new plate into
		 * the map AND THEN insert the polyline into the (new copy
		 * of the) plate [which requires one list copy], than to
		 * insert the polyline into the plate then insert the plate
		 * [which requires two list copies].
		 */
		std::pair< PlatesDataMap::iterator, bool > pos_and_status =
		 plates_data.insert(PlatesDataMap::value_type(plate_id, plate));

		it = pos_and_status.first;
	}
	it->second._polylines.push_back(pl);
}


/**
 * A reasonable maximum length for each line of a polyline header.
 * This length does not include a terminating character.
 *
 * Note that, if the second argument to an invocation of 'getline' is
 * the integer value 'n', this tells 'getline' to expect a character buffer
 * of 'n' characters, so 'getline' will read at most (n - 1) characters into
 * this buffer.  This (n - 1) characters includes the newline, although the
 * newline will not be stored into the buffer.
 *
 * If, during this call, 'getline' encounters a line which contains *more*
 * than (n - 1) characters, it will set the failbit.  Thus, by testing the
 * status of the stream after the invocation of 'getline', we will be aware
 * of the failure in the unlikely event of a line which exceeds this maxiumum.
 */
static const size_t POLYLINE_HEADER_LINE_LEN = 120;


/**
 * Because this function will "fail" when we reach EOF (which will, of course,
 * happen eventually), we can't simply shrug and throw an exception.
 * Hence, we need to test whether we have reached EOF before we complain.
 */
void
ReadFirstLineOfPolyLineHeader(LineBuffer &lb, std::string &str) {

	static char buf[POLYLINE_HEADER_LINE_LEN + 1];

	if ( ! lb.getline(buf, sizeof(buf))) {

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
		 << "\nwhile attempting to read the first line of a"
		 " polyline header.";

		throw FileFormatException(oss.str().c_str());
	}
	str = std::string(buf);
}


void
ReadSecondLineOfPolyLineHeader(LineBuffer &lb, std::string &str) {

	static char buf[POLYLINE_HEADER_LINE_LEN + 1];

	if ( ! lb.getline(buf, sizeof(buf))) {

		// For some reason, the read was considered "unsuccessful"
		std::ostringstream oss;
		oss << "Unsuccessful read from " << lb
		 << "\nwhile attempting to read the second line of a "
		 "polyline header.";

		throw FileFormatException(oss.str().c_str());
	}
	str = std::string(buf);
}


void
ReadPolyLinePoints(LineBuffer &lb, std::list< BoundaryLatLonPoint > &points,
	size_t num_points_to_expect) {
    
	/*
	 * The number of points to expect was specified in the polyline header.
	 * We have asserted that it must be at least 2.  Read the first point.
	 */
	std::string first_line = ReadPolyLinePoint(lb);
	BoundaryLatLonPoint first_point =
	 LatLonPoint::ParseBoundaryLine(lb, first_line, PlotterCodes::PEN_UP);
	points.push_back(first_point);

	/*
	 * We've already read the first point, and we don't want to read
	 * the "terminating point" inside this loop.
	 */
	for (size_t n = 1; n < num_points_to_expect; n++) {

		std::string line = ReadPolyLinePoint(lb);
		BoundaryLatLonPoint point =
		 LatLonPoint::ParseBoundaryLine(lb, line,
		  PlotterCodes::PEN_EITHER);
		points.push_back(point);
	}

	/*
	 * Now, finally, read the "terminating point".
	 * This is not really a "point", since its lat and lon are both 99.0.
	 * No "point" object will be created.
	 */
	std::string term_line = ReadPolyLinePoint(lb);
	LatLonPoint::ParseTermBoundaryLine(lb, term_line, PlotterCodes::PEN_UP);
}


/**
 * A reasonable maximum length for each line representing a polyline point.
 * This length does not include a terminating character.
 */
static const size_t POLYLINE_POINT_LINE_LEN = 40;


std::string
ReadPolyLinePoint(LineBuffer &lb) {

	static char buf[POLYLINE_POINT_LINE_LEN + 1];

	if ( ! lb.getline(buf, sizeof(buf))) {

		// For some reason, the read was considered "unsuccessful"
		std::ostringstream oss;
		oss << "Unsuccessful read from " << lb
		 << "\nwhile attempting to read a polyline point.";

		throw FileFormatException(oss.str().c_str());
	}
	return std::string(buf);
}

}  // end namespace PlatesParser
}  // end namespace GPlatesFileIO
