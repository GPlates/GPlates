/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#include "PlatesPostParseTranslator.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>  /* transform */
#include <memory>  /* std::auto_ptr */
#include <iomanip>
#include <iterator>
#include "maths/OperationsOnSphere.h"
#include "fileio/PlatesBoundaryParser.h"
#include "fileio/FileFormatException.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "global/types.h"  /* rid_t */

using namespace GPlatesFileIO;
using namespace GPlatesGeo;


namespace
{

	GPlatesMaths::LatLonPoint
	ConvertPlatesParserLatLonToMathsLatLon(
		const PlatesParser::LatLonPoint& point) 
	{
		/*
		 * Note that GPlates considers a valid longitude to be a value in
		 * the half-open range (-180.0, 180.0].  Note that this appears
		 * to be different to the range used by PLATES, which seems to be 
		 * [-360.0, 360.0].
		 */
		GPlatesMaths::real_t lat(point._lat);
		GPlatesMaths::real_t lon(point._lon);
		if (lon <= -180.0) 
			lon += 360.0;
		else if (lon > 180.0)
			lon -= 360.0;

		return GPlatesMaths::LatLonPoint::LatLonPoint(lat, lon);
	}


	// List of LatLonPoint Lists.  XXX There is no God.
	typedef std::list< std::list< PlatesParser::LatLonPoint > > LLLPL_t;


	void
	ConvertPolyLineToListOfLatLonPointLists(
		const PlatesParser::PolyLine& line,
		LLLPL_t& plate_segments)
	{
		std::list< PlatesParser::LatLonPoint > llpl;

		std::list< PlatesParser::BoundaryLatLonPoint >::const_iterator 
			iter = line._points.begin();
		// Handle first point
		llpl.push_back(iter->first);
		++iter;

		for ( ; iter != line._points.end(); ++iter) 
		{
			if (iter->second == PlatesParser::PlotterCodes::PEN_UP)
			{
				// We need to split this line, so push_back a copy of
				// the line so far
				plate_segments.push_back(llpl);
				llpl.clear();  // start a new segment
			}
			llpl.push_back(iter->first);
		}
		// There should always be one segment left to add.
		if (llpl.size())
			plate_segments.push_back(llpl);
	}

	void
	GetLineDataListFromPolyLine(
		const PlatesParser::PolyLine& line, 
		std::list< LineData* >& result)
	{
		using namespace GPlatesMaths;

		GPlatesGlobal::rid_t plate_id = line._header._plate_id;
		TimeWindow lifetime = line._header._lifetime;
		
		// Filter line._points such that it is split up according to PlotterCode.

		LLLPL_t plate_segments;
		ConvertPolyLineToListOfLatLonPointLists(line, plate_segments);

		LLLPL_t::const_iterator iter = plate_segments.begin();
		for ( ; iter != plate_segments.end(); ++iter)
		{
			std::list< LatLonPoint > llpl;
			std::transform(iter->begin(), iter->end(), 
						   std::back_inserter(llpl),
						   &ConvertPlatesParserLatLonToMathsLatLon);
			llpl.unique();  // Eliminate identical consecutive points.
		
			// Point is "commented"
			if (llpl.size() <= 1)
				continue;
			
			PolyLineOnSphere polyline =
			 OperationsOnSphere::convertLatLonPointListToPolyLineOnSphere(llpl);
			
			result.push_back(new LineData(GeologicalData::NO_DATATYPE, 
				plate_id, lifetime, GeologicalData::NO_ATTRIBUTES, polyline));
		}
	}

	PointData*
	GetPointDataFromPolyLine(const PlatesParser::PolyLine& line)
	{
		using namespace GPlatesMaths;

		// FIXME: This should be factored out from here and 
		// GetLineDataFromPolyLine.
		GPlatesGlobal::rid_t plate_id = line._header._plate_id;
		TimeWindow lifetime = line._header._lifetime;

		// Get an iterator to the first and only point in the line.
		std::list< PlatesParser::BoundaryLatLonPoint >::const_iterator 
			point = line._points.begin();

		LatLonPoint 
			llp = ConvertPlatesParserLatLonToMathsLatLon(point->first);
		PointOnSphere
			pos = OperationsOnSphere::convertLatLonPointToPointOnSphere(llp);
		return new PointData(GeologicalData::NO_DATATYPE, plate_id,
			lifetime, GeologicalData::NO_ATTRIBUTES, pos);
	}

	void
	AddLinesFromPlate(DataGroup* data, const PlatesParser::Plate& plate)
	{
		using namespace PlatesParser;

		if (plate._polylines.size() <= 0) {

			/* 
			 * Some how we got this far with only one plate.  Better
			 * throw a reggie, well a FileFormatException anyway.
			 */
			std::ostringstream oss;
			oss << "No data found on plate with ID: "
			 << plate._plate_id << std::endl;
			throw FileFormatException(oss.str().c_str());
		}

		std::list<PolyLine>::const_iterator iter = plate._polylines.begin();

		for ( ; iter != plate._polylines.end(); ++iter)
		{
			// A polyline with a single point is actually PointData.
			if (iter->_points.size() == 1)
			{
				PointData* pd = GetPointDataFromPolyLine(*iter);
				data->Add(pd);
				continue;
			}

			std::list< LineData* > ld;
			GetLineDataListFromPolyLine(*iter, ld);

			std::list< LineData* >::iterator jter = ld.begin();
			for ( ; jter != ld.end(); ++jter)
				data->Add(*jter);
		}
	}
} // End anonymous namespace


namespace GPlatesFileIO
{

namespace PlatesPostParseTranslator
{
	DataGroup*
	GetDataGroupFromPlatesDataMap(const PlatesParser::PlatesDataMap& map)
	{
		DataGroup *data = new DataGroup(GeologicalData::NO_DATATYPE,
										GeologicalData::NO_ROTATIONGROUP,
										GeologicalData::NO_TIMEWINDOW,
										GeologicalData::NO_ATTRIBUTES);

		PlatesParser::PlatesDataMap::const_iterator iter = map.begin();
		for ( ; iter != map.end(); ++iter) 
		{
			// Insert the PlatesParser::Plate into the new DataGroup
			AddLinesFromPlate(data, iter->second);
		}

		return data;
	}
	
} // End namespace PlatesPostParseTranslator

} // End namespace GPlatesFileIO
