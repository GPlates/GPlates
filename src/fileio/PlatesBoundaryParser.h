/*$Id$*/

/**
 * @file
 *
 * Most recent change:
 * $Author$
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
 * Authors:
 *   Mike Dowman <mdowman@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_PLATESBOUNDARYPARSER_H_
#define _GPLATES_FILEIO_PLATESBOUNDARYPARSER_H_

#include <istream>
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
		typedef std::map< GPlatesGlobal::rgid_t, Plate >
		 PlatesDataMap;

		using GPlatesGlobal::rgid_t;


		void ReadInPlateBoundaryData(const char *filename,
		 std::istream &input_stream, PlatesDataMap &plates_data);

		void ReadPolyLine(LineBuffer &lb, PlatesDataMap &plates_data);

		void AppendPolyLineToPlatesData(PlatesDataMap &plates_data,
		 const rgid_t &plate_id, const PolyLine &pl);

		void ReadFirstLineOfPolyLineHeader(LineBuffer &lb,
		 std::string &str);

		void ReadSecondLineOfPolyLineHeader(LineBuffer &lb,
		 std::string &str);

		void ReadPolyLinePoints(LineBuffer &lb,
		 std::list< LatLonPoint > &points, size_t num_points_to_expect);

		std::string ReadPolyLinePoint(LineBuffer &lb);
	}
}

#endif // _GPLATES_FILEIO_PLATESBOUNDARYPARSER_H_
