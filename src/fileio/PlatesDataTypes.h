/* $Id$ */

/**
 * \file 
 * This file contains a collection of data types which will be used
 * by the PLATES-format parser.  They will probably _not_ be used
 * by any other parser.  These types are really only intended to be
 * temporary place-holders, providing data-types for the parsing
 * before the geometry engine takes over.  These types are currently
 * all structs due to their primitive and temporary nature;
 * if necessary, they may become classes (with a correspondingly more
 * complicated interface of accessors and modifiers).
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

#include "LineBuffer.h"
#include "global/types.h"
#include "geo/TimeWindow.h"

namespace GPlatesFileIO
{
	namespace PlatesParser
	{
		using namespace GPlatesGlobal;


		struct FiniteRotation;

		FiniteRotation ParseRotationLine(const LineBuffer &lb,
		 const std::string &line);


		struct LatLonPoint {

				friend FiniteRotation
				ParseRotationLine(const LineBuffer &lb,
				 const std::string &line);

			public:
				static LatLonPoint
				ParseBoundaryLine(const LineBuffer &lb,
				 const std::string &line,
				 int expected_plotter_code);

				static void
				ParseTermBoundaryLine(const LineBuffer &lb,
				 const std::string &line,
				 int expected_plotter_code);

				static bool isValidLat(const fpdata_t &val);
				static bool isValidLon(const fpdata_t &val);

				fpdata_t _lat, _lon;

				// no default constructor

			protected:

				LatLonPoint(const fpdata_t &lat,
				 const fpdata_t &lon)
				 : _lat(lat), _lon(lon) {  }
		};


		struct EulerRotation {

				friend FiniteRotation
				ParseRotationLine(const LineBuffer &lb,
				 const std::string &line);

			public:
				LatLonPoint _pole;
				fpdata_t    _angle;  // degrees

				// no default constructor

			protected:
				EulerRotation(const LatLonPoint &pole,
				 const fpdata_t &angle)
				 : _pole(pole), _angle(angle) {  }
		};


		struct FiniteRotation {

				friend FiniteRotation
				ParseRotationLine(const LineBuffer &lb,
				 const std::string &line);

			public:
				fpdata_t      _time;  // Millions of years ago
				rgid_t        _moving_plate, _fixed_plate;
				EulerRotation _rot;
				std::string   _comment;

				// no default constructor

			protected:
				FiniteRotation(const fpdata_t &time,
				 const rgid_t &moving_plate,
				 const rgid_t &fixed_plate,
				 const EulerRotation &rot,
				 const std::string &comment)
				 : _time(time),
				   _moving_plate(moving_plate),
				   _fixed_plate(fixed_plate),
				   _rot(rot),
				   _comment(comment) {  }
		};


		struct PolyLineHeader {

			public:
				static PolyLineHeader
				ParseLines(const LineBuffer &lb,
				 const std::string &first_line,
				 const std::string &second_line);

				static void
				ParseSecondLine(const LineBuffer &lb,
				 const std::string &line, rgid_t &plate_id,
				 fpdata_t &age_appear, fpdata_t &age_disappear,
				 size_t &num_points);

				// store both lines in their original format
				std::string _first_line, _second_line;

				/*
				 * the plate id of the plate
				 * to which this polyline belongs.
				 */
				rgid_t _plate_id;

				/*
				 * the lifetime of this polyline, from
				 * age of appearance to age of disappearance.
				 */
				GPlatesGeo::TimeWindow _lifetime;

				// the number of points in this polyline
				size_t _num_points;

				// no default constructor

			protected:

				PolyLineHeader(const std::string &first_line,
				 const std::string &second_line,
				 const rgid_t &plate_id,
				 const GPlatesGeo::TimeWindow &lifetime,
				 size_t num_points)

				 : _first_line(first_line),
				   _second_line(second_line),
				   _plate_id(plate_id),
				   _lifetime(lifetime),
				   _num_points(num_points) {  }
		};


		struct PolyLine {

			PolyLineHeader           _header;
			std::list< LatLonPoint > _points;

			// no default constructor

			PolyLine(const PolyLineHeader &header)
			 : _header(header) {  }
		};


		struct Plate {

			rgid_t                _plate_id;
			std::list< PolyLine > _polylines;

			// no default constructor

			Plate(const rgid_t &plate_id)
			 : _plate_id(plate_id) {  }
		};
	}
}

#endif  // _GPLATES_FILEIO_PLATESDATATYPES_H_
