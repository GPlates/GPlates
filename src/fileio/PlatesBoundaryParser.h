/*$Id$*/

/**
 * @file
 *
 * Most recent change:
 * $Date$
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

#ifndef _GPLATES_FILEIO_PLATESBOUNDARYPARSER_H_
#define _GPLATES_FILEIO_PLATESBOUNDARYPARSER_H_

#include <iostream>
#include <string>
#include <list>
#include <map>

#include "global/types.h"
#include "PlatesDataTypes.h"
#include "LineBuffer.h"


namespace GPlatesFileIO
{
	namespace PlatesParser
	{
		typedef std::map< plate_id_t, Plate > PlatesDataMap;

		void ReadInPlateBoundaryData(const char *filename,
		 std::istream &input_stream, PlatesDataMap &plates_data);

		void ReadPolyline(LineBuffer &lb, PlatesDataMap &plates_data);

		void AppendPolylineToPlatesData(PlatesDataMap &plates_data,
		 const plate_id_t &plate_id, const Polyline &pl);

		void ReadFirstLineOfPolylineHeader(LineBuffer &lb,
		 std::string &str);

		void ReadSecondLineOfPolylineHeader(LineBuffer &lb,
		 std::string &str);

		void ReadPolylinePoints(LineBuffer &lb,
		 std::list< BoundaryLatLonPoint > &points);

		std::string ReadPolylinePoint(LineBuffer &lb);
	}
}

#endif // _GPLATES_FILEIO_PLATESBOUNDARYPARSER_H_
