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

#ifndef GPLATES_FILEIO_PLATESDATATYPES_H
#define GPLATES_FILEIO_PLATESDATATYPES_H

#include <list>
#include <string>
#include <utility>

#include "global/types.h"
#include "LineBuffer.h"

// Eliminate use of GPlatesGeo classes
#include "model/GeoTimeInstant.h"

namespace GPlatesFileIO
{
	namespace PlatesParser
	{
		using namespace GPlatesGlobal;


		// The type which will be used for the plate id
		typedef unsigned int plate_id_t;


		/**
		 * The possible values for the plotter code.
		 * Don't worry too much about what this means.
		 */
		namespace PlotterCodes 
		{
			enum PlotterCode
			{ 
				PEN_EITHER, PEN_TERMINATING_POINT, 
				PEN_DOWN = 2, PEN_UP = 3
			};
		}


		struct FiniteRotation;

		const FiniteRotation 
		ParseRotationLine(
				const LineBuffer &lb,
				const std::string &line);

		struct BoundaryLatLonPoint;

		struct LatLonPoint
		{
			friend 
			const FiniteRotation
			ParseRotationLine(
					const LineBuffer &lb,
					const std::string &line);

		public:
			static 
			const BoundaryLatLonPoint
			ParseBoundaryLine(
					const LineBuffer &lb,
					const std::string &line,
					int expected_plotter_code);

			static 
			void
			ParseTermBoundaryLine(
					const LineBuffer &lb,
					const std::string &line,
					int expected_plotter_code);

			static bool isValidLat(const fpdata_t &val);
			static bool isValidLon(const fpdata_t &val);

			fpdata_t d_lat, d_lon;

			// no default constructor

		protected:
			LatLonPoint(
					const fpdata_t &lat,
				 	const fpdata_t &lon) :
				d_lat(lat), 
				d_lon(lon) 
			{  }
		};

		/**
		 * Store a LatLonPoint with its plotter code.
		 */
		struct BoundaryLatLonPoint
		{
			LatLonPoint d_lat_lon_point;
			PlotterCodes::PlotterCode d_plotter_code;
			unsigned d_line_number;

			BoundaryLatLonPoint(
					const LatLonPoint &lat_lon_point,
					PlotterCodes::PlotterCode plotter_code,
					unsigned line_number) :
				d_lat_lon_point(lat_lon_point),
				d_plotter_code(plotter_code),
				d_line_number(line_number)
			{ }
		};


		struct EulerRotation
		{
			friend 
			const FiniteRotation
			ParseRotationLine(
					const LineBuffer &lb,
					const std::string &line);

		public:
			LatLonPoint d_pole;
			fpdata_t d_angle;  // degrees

			// no default constructor

		protected:
			EulerRotation(
					const LatLonPoint &pole,
					const fpdata_t &angle) :
				d_pole(pole), 
				d_angle(angle) 
			{  }
		};


		struct FiniteRotation
		{
			friend 
			const FiniteRotation
			ParseRotationLine(
					const LineBuffer &lb,
					const std::string &line);

			public:
				fpdata_t d_time;  // Millions of years ago
				plate_id_t d_moving_plate, d_fixed_plate;
				EulerRotation d_rot;
				std::string d_comment;

				// no default constructor

			protected:
				FiniteRotation(
						const fpdata_t &time,
						const plate_id_t &moving_plate,
				 		const plate_id_t &fixed_plate,
				 		const EulerRotation &rot,
						const std::string &comment) :
					d_time(time),
					d_moving_plate(moving_plate),
					d_fixed_plate(fixed_plate),
					d_rot(rot),
					d_comment(comment) 
				{  }
		};


		struct RotationSequence
		{
			plate_id_t d_moving_plate, d_fixed_plate;

			/*
			 * The elements in this list are finite rotations
			 * which operate upon the same (moving plate, fixed
			 * plate) pair, and were listed in an uninterrupted
			 * sequence in the rotation file.
			 */
			std::list< FiniteRotation > d_seq;

			// no default constructor

			RotationSequence(
					const plate_id_t &moving_plate,
			 		const plate_id_t &fixed_plate,
			 		const FiniteRotation &rot) :
				d_moving_plate(moving_plate),
				d_fixed_plate(fixed_plate) 
			{
				d_seq.push_back(rot);
			}
		};


		struct PolylineHeader
		{
		public:
			static 
			PolylineHeader
			ParseLines(
					const LineBuffer &lb,
					const std::string &first_line,
					const std::string &second_line);

			static
			void
			ParseSecondLine(
					const LineBuffer &lb,
					const std::string &line, 
					plate_id_t &plate_id,
					fpdata_t &age_appear, 
					fpdata_t &age_disappear,
					size_t &num_points);

			// store both lines in their original format
			std::string d_first_line, d_second_line;

			/*
			 * the plate id of the plate
			 * to which this polyline belongs.
			 */
			plate_id_t d_plate_id;

			/*
			 * the lifetime of this polyline, from
			 * age of appearance to age of disappearance.
			 */
			GPlatesModel::GeoTimeInstant d_time_instant_begin;
			GPlatesModel::GeoTimeInstant d_time_instant_end;

			// the number of points in this polyline
			// note: we think this is out of date
			// FIXME: double-check whether this is actually used anywhere
			size_t d_num_points;

			// no default constructor

		protected:
			PolylineHeader(
					const std::string &first_line,
					const std::string &second_line,
					const plate_id_t &plate_id,
					const GPlatesModel::GeoTimeInstant& time_instant_begin,
					const GPlatesModel::GeoTimeInstant& time_instant_end,
					size_t num_points) :
				d_first_line(first_line),
				d_second_line(second_line),
				d_plate_id(plate_id),
				d_time_instant_begin(time_instant_begin),
				d_time_instant_end(time_instant_end),
				d_num_points(num_points) 
			{  }
		};


		struct Polyline
		{
			PolylineHeader d_header;
			std::list< BoundaryLatLonPoint > d_points;
			unsigned d_line_number;

			// no default constructor

			Polyline(
					const PolylineHeader &header,
					unsigned line_number) :
				d_header(header),
				d_line_number(line_number)	
			{  }
		};


		struct Plate
		{
			plate_id_t d_plate_id;
			std::list< Polyline > d_polylines;

			// no default constructor

			Plate(
					const plate_id_t &plate_id) :
				d_plate_id(plate_id) 
			{  }
		};
	}
}

#endif  // GPLATES_FILEIO_PLATESDATATYPES_H
