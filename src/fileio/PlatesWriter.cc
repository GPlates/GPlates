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

#include "PlatesWriter.h"

void
PlatesWriter::Visit(const LineData& linedata)
{
	// Output line header information
	
	// Output each of the points that make up the line.
	LineData::Line_t::const_iterator iter = linedata.GetIterator();
	for ( ; *iter; iter++) {
		PointOnSphere point = iter->GetPointOnSphere();
		_strstream << point.GetLatitude() << "  " << point.GetLongitude();
		_strstream << " 2" << std::endl;  // This is the pen-plot command.
	}									  // FIXME: remove magic numbers.
}

bool
PrintWriter::PrintOut(std::ostream& os)
{
	os << _strstream.str();
}
