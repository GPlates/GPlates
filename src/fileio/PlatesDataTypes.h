/* $Id$ */

/**
 * \file 
 * This file contains a collection of data types which will be used by
 * the PLATES-format parser, and will probably _not_ be used by any other
 * parser.
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

#ifndef _GPLATES_FILEIO_PLATESDATATYPES_H_
#define _GPLATES_FILEIO_PLATESDATATYPES_H_

#include <list>
#include <string>
#include "global/types.h"
#include "PrimitiveDataTypes.h"
#include "geo/TimeWindow.h"


namespace GPlatesFileIO
{
	using namespace GPlatesGlobal;

	struct PlatesPolyLineHeader {

		std::string _first_line, _second_line;  // stores the whole line
		rgid_t      _plate_id;

		GPlatesGeo::TimeWindow _lifetime;

		// no default constructor

		PlatesPolyHeader(const std::string &first_line,
		 const std::string &second_line,
		 const rgid_t &plate_id,
		 GPlatesGeo::TimeWindow &lifetime)

		 : _first_line(first_line),
		   _second_line(second_line),
		   _plate_id(plate_id),
		   _lifetime(lifetime) {  }
	};


	struct PlatesPolyLine {

		PlatesPolyLineHeader     _header;
		std::list< LatLonPoint > _points;

		// no default constructor

		PlatesPolyLine(const PlatesPolyLineHeader &header)
		 : _header(header) {  }
	};


	struct PlatesPlate {

		rgid_t                      _plate_id;
		std::list< PlatesPolyLine > _polylines;

		// no default constructor

		PlatesPlate(const rgid_t &plate_id)
		 : _plate_id(plate_id) {  }
	};
}

#endif  // _GPLATES_FILEIO_PLATESDATATYPES_H_
