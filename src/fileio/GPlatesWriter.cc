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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#include <string>
#include "PlatesWriter.h"

using namespace GPlatesFileIO;

/**
 * Pen command for 'draw to'.
 */
static const std::string DRAW_TO_POINT = "2";

/**
 * Pen command for 'skip to'.
 */
static const std::string SKIP_TO_POINT = "3";

/**
 * Definition of the end of a line.
 */
static const std::string LINE_END = "  99.0000  99.0000 3";

void
PlatesWriter::Visit(const LineData& linedata)
{
	PointOnSphere point;
	
	// Output line header information
	// First line of header is useless information.  Format is:
	//   <region number> <reference number> <string number> <description>
	// FIXME: we may be storing the correct values for these
	//   in the GeneralisedData.
	_accum << "666 6 6 COMMENT" << std::endl;

	// Format for the second line:
	//   <plate id> <age of appearance> <age of disappearance>
	//   <data type code> <data type code number> <plate id number>
	//   <colour code number> <number of points>
	// FIXME: how do I get this information out of the GeneralisedData?
	
	// Output each of the points that make up the line.
	LineData::Line_t::const_iterator
		iter = linedata.Begin(),
		end = linedata.End();

	// First point in the line should be a SKIP_TO_POINT
	// pen movement.
	point = iter->GetPointOnSphere();
	_accum << "  " << point.GetLatitude() << "  " 
		<< point.GetLongitude() << " " << SKIP_TO_POINT << std::endl;
	iter++;
	for ( ; iter != end; ++iter) {
		point = iter->GetPointOnSphere();
		_accum << "  " << point.GetLatitude() << "  " 
			<< point.GetLongitude() << " " << DRAW_TO_POINT << std::endl;
	}
	_accum << LINE_END << std::endl;
}

bool
PlatesWriter::PrintOut(std::ostream& os)
{
	os << _accum.str();
	return true;
}
