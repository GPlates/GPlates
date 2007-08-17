/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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
#include "PlatesBoundaryParser.h"
#include "FileFormatException.h"

namespace {
	struct ParserError
	{
		ParserError(
				GPlatesFileIO::ReadErrors::Description description):
			d_description(description)
		{ }

		GPlatesFileIO::ReadErrors::Description d_description;
	};

	void
	AppendPolylineToPlatesData(
			GPlatesFileIO::PlatesParser::PlatesDataMap &plates_data,
			const GPlatesFileIO::PlatesParser::plate_id_t &plate_id, 
			const GPlatesFileIO::PlatesParser::Polyline &pl) 
	{
		using namespace GPlatesFileIO::PlatesParser;

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
			std::pair<PlatesDataMap::iterator, bool> pos_and_status =
					plates_data.insert(PlatesDataMap::value_type(plate_id, plate));
	
			it = pos_and_status.first;
		}
		it->second.d_polylines.push_back(pl);
	}
	
	
	/**
	* Because this function will "fail" when we reach EOF (which will, of course,
	* happen eventually), we can't simply shrug and throw an exception.
	* Hence, we need to test whether we have reached EOF before we complain.
	*/
	void
	ReadFirstLineOfPolylineHeader(
			GPlatesFileIO::LineBuffer &lb, 
			std::string &str) 
	{
		using namespace GPlatesFileIO;

		if ( ! lb.getline(str)) {
			/*
			* For some reason, the read was considered "unsuccessful".
			* This might be because we have reached EOF.
			*
			* Test whether we have reached EOF, and if so, return.
			*/
			if (lb.eof()) {
				return;
			}
	
			// else, there *was* an unexplained failure
			throw ParserError(ReadErrors::InvalidFirstHeaderLine);
		}
	}
	
	
	void
	ReadSecondLineOfPolylineHeader(
			GPlatesFileIO::LineBuffer &lb, 
			std::string &str) 
	{
		using namespace GPlatesFileIO;

		if ( ! lb.getline(str)) {
			throw ParserError(ReadErrors::InvalidSecondHeaderLine);
		}
	}
	

	std::string
	ReadPolylinePoint(
			GPlatesFileIO::LineBuffer &lb) 
	{
		using namespace GPlatesFileIO;

		std::string buf; 
	
		if ( ! lb.getline(buf)) {
			throw ParserError(ReadErrors::InvalidPolylinePoint);
		}

		return std::string(buf);
	}

	
	void
	ReadPolylinePoints(
			GPlatesFileIO::LineBuffer &lb, 
			std::list<GPlatesFileIO::PlatesParser::BoundaryLatLonPoint> &points) 
	{	
		using namespace GPlatesFileIO::PlatesParser;

		/*
		* The number of points to expect was specified in the polyline header.
		* We have asserted that it must be at least 2.  Read the first point.
		*/
		std::string first_line = ReadPolylinePoint(lb);
		BoundaryLatLonPoint first_point =
		LatLonPoint::ParseBoundaryLine(lb, first_line, PlotterCodes::PEN_UP);
		points.push_back(first_point);
	
		/*
		* We've already read the first point.  This loop reads until
		* a "terminating point" is found.
		*/
		for ( ; ; ) {
			std::string line = ReadPolylinePoint(lb);
			BoundaryLatLonPoint point =
			LatLonPoint::ParseBoundaryLine(lb, line,
			PlotterCodes::PEN_EITHER);
			
			/*
			* Test for terminating point.
			* According to the PLATES data file spec, this has the 
			* uninformative magical values:
			*  lat = 99.0, lon = 99.0, plot code = 3 ('skip to')
			* No "point" object will be created.
			*/
			if (point.d_plotter_code == PlotterCodes::PEN_TERMINATING_POINT) {
				break;
			}
	
			points.push_back(point);
		}
	}


	/**
	* Given a LineBuffer, read a single Polyline and store it.
	*/
	void 
	ReadPolyline(
			GPlatesFileIO::LineBuffer &lb, 
			GPlatesFileIO::PlatesParser::PlatesDataMap &plates_data) 
	{
		using namespace GPlatesFileIO::PlatesParser;

		std::string first_line;
		ReadFirstLineOfPolylineHeader(lb, first_line);
	
		/*
		* Note that, if we had already read the last polyline
		* before this function was invoked, then the 'getline' in the
		* just-invoked 'ReadFirstLineOfPolylineHeader' would have
		* failed as it reached EOF.
		*
		* Hence, we should now test for EOF.
		*/
		if (lb.eof()) {
			return;
		}
	
		std::string second_line;
		ReadSecondLineOfPolylineHeader(lb, second_line);
	
		PolylineHeader header =
				PolylineHeader::ParseLines(lb, first_line, second_line);
	
		/*
		* The rest of this polyline will consist of the actual points.
		*/
		Polyline polyline(header, lb.lineNum());
		ReadPolylinePoints(lb, polyline.d_points);
	
		/*
		* Having read the whole polyline, we now insert it into its
		* containing plate -- where a plate is identified by its plate_id.
		*/
		plate_id_t plate_id = header.d_plate_id;
		AppendPolylineToPlatesData(plates_data, plate_id, polyline);
	}
}

namespace GPlatesFileIO {
	
namespace PlatesParser {
		
	void 
	ReadInPlateBoundaryData(
			const char *filename, 
			std::istream &input_stream,
			PlatesDataMap &plates_data,
			ReadErrorAccumulation errors) 
	{
		boost::shared_ptr<DataSource> source( 
				new LocalFileDataSource(filename, DataFormats::PlatesLine));
		/*
		* The input stream must point to the beginning of an already-opened
		* istream containing plates boundary data.
		*/
		LineBuffer lb(input_stream, filename);
	
		// Each file can contain multiple polylines.
		while ( ! lb.eof()) {
			try {
				ReadPolyline(lb, plates_data);
			} catch (const ParserError &error) {
				const boost::shared_ptr<LocationInDataSource> location(
						new LineNumberInFile(lb.lineNum()));
				errors.d_recoverable_errors.push_back(ReadErrorOccurrence(source, location,
						error.d_description, ReadErrors::PolylineDiscarded));
			}
		}
	}

}  // end namespace PlatesParser
}  // end namespace GPlatesFileIO
