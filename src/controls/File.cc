/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>  /* transform */
#include "File.h"
#include "Dialogs.h"
#include "state/Data.h"
#include "fileio/PlatesRotationParser.h"
#include "maths/OperationsOnSphere.h"
#include "fileio/PlatesBoundaryParser.h"
#include "fileio/GPlatesReader.h"
#include "fileio/FileIOException.h"
#include "geo/LineData.h"
#include "global/types.h"  /* rid_t */
#include <iomanip>
#include <iterator>

using namespace GPlatesControls;
using namespace GPlatesFileIO;
using namespace GPlatesGeo;

static void
OpenFileErrorMessage(const std::string &fname, const char *fail_result_msg) {

	std::ostringstream msg;
	msg << "The file \"" << fname << "\" could not\n"
	 << "be opened for reading.";

	Dialogs::ErrorMessage("Unable to open file", msg.str().c_str(),
	 fail_result_msg);
}


static void
HandleGPMLFile(const std::string& filename)
{
#if 0
	std::ostringstream result;
	result << "No GPML data was loaded from \"" << filename
		<< "\"." << std::endl;
	
	Dialogs::ErrorMessage(
		"GPML Not Implemented",
		"The GPML parser is still under development,\n"
		"hence GPlates cannot yet load GPML data files.\n",
		result.str().c_str());
#endif

	std::ifstream file(filename.c_str());
	if (!file) {
		OpenFileErrorMessage(filename, "No GPML data was loaded.");
		return;
	}

	GPlatesReader reader(file);
	DataGroup *data = reader.Read();
	GPlatesState::Data::SetDataGroup(data);
}


static GPlatesMaths::LatLonPoint
ConvertPlatesParserLatLonToMathsLatLon(const PlatesParser::LatLonPoint& point) 
{
	return GPlatesMaths::LatLonPoint::CreateLatLonPoint(
	 GPlatesMaths::real_t(point._lat), GPlatesMaths::real_t(point._lon));
}


static LineData*
GetLineDataFromPolyLine(const PlatesParser::PolyLine& line)
{
	using namespace GPlatesMaths;

	GPlatesGlobal::rid_t plate_id = line._header._plate_id;
	TimeWindow lifetime = line._header._lifetime;
	
	std::list< LatLonPoint > llpl;
	std::transform(line._points.begin(), line._points.end(), 
	               std::back_inserter(llpl),
	               &ConvertPlatesParserLatLonToMathsLatLon);
	llpl.unique();  // Eliminate identical consecutive points.

	PolyLineOnSphere polyline =
	 OperationsOnSphere::convertLatLonPointListToPolyLineOnSphere(llpl);
	
	return new LineData(GeologicalData::NO_DATATYPE, 
	                    plate_id, 
	                    lifetime,
	                    GeologicalData::NO_ATTRIBUTES,
	                    polyline);
}


static void
AddLinesFromPlate(DataGroup* data, const PlatesParser::Plate& plate)
{
	using namespace PlatesParser;

	std::list<PolyLine>::const_iterator iter = plate._polylines.begin();
	for ( ; iter != plate._polylines.end(); ++iter) {
		data->Add(GetLineDataFromPolyLine(*iter));
	}
}


static void
LoadDataGroupFromPlatesDataMap(const PlatesParser::PlatesDataMap& map)
{
	DataGroup *data = new DataGroup(GeologicalData::NO_DATATYPE,
	                                GeologicalData::NO_ROTATIONGROUP,
	                                GeologicalData::NO_TIMEWINDOW,
	                                GeologicalData::NO_ATTRIBUTES);

	PlatesParser::PlatesDataMap::const_iterator iter = map.begin();
	for ( ; iter != map.end(); ++iter) {
		// Insert the PlatesParser::Plate into the new DataGroup
		AddLinesFromPlate(data, iter->second);
	}

	GPlatesState::Data::SetDataGroup(data);
}


static void
HandlePLATESFile(const std::string& filename)
{
	std::ifstream file(filename.c_str());
	if (!file) {
		OpenFileErrorMessage(filename, "No PLATES data was loaded.");
		return;
	}

	// filename is good for reading.
	PlatesParser::PlatesDataMap map;
	try {

		PlatesParser::ReadInPlateBoundaryData(filename.c_str(), file,
		 map);
		LoadDataGroupFromPlatesDataMap(map);

	} catch (const FileIOException& e) {

		std::ostringstream msg;
		msg << "An error was encountered in \"" << filename 
			<< "\"" << std::endl
			<< "Error message: " << std::endl << e;
		Dialogs::ErrorMessage(
			"Error in data file",
			msg.str().c_str(),
			"No PLATES data was loaded.");  

	} catch (const GPlatesGlobal::Exception& ex) {

		std::cerr << "Internal exception: " << ex << std::endl;
		exit(1);
	}
}


void
File::OpenData(const std::string& filename)
{
	// XXX file type determined by extension!
	// XXX extension could occur anywhere in the filename!
	static const char* GPML_EXT = ".gpml";
	static const char* PLATES_EXT = ".dat";

	if (filename.rfind(GPML_EXT) != std::string::npos) {
		// File is a GPML file
		HandleGPMLFile(filename);
	} else if (filename.rfind(PLATES_EXT) != std::string::npos) {
		// File is a PLATES file.
		HandlePLATESFile(filename);
	} else {
		std::ostringstream msg;
		msg << "The file \"" << filename << "\" does not have a" <<std::endl
			<< "supported extension.  Supported extensions " << std::endl
			<< "include: " << std::endl
			<< "-- " << GPML_EXT << ": for GPML files; and" << std::endl
			<< "-- " << PLATES_EXT << ": for PLATES files." << std::endl;
		Dialogs::ErrorMessage(
			"File extension not recognised",
			msg.str().c_str(),
			"No data was loaded.");
	}
}


void
File::OpenRotation(const std::string& filename)
{
	std::ifstream f(filename.c_str());
	if ( ! f) {

		// attempt to open file was unsuccessful
		OpenFileErrorMessage(filename, "No rotation data was loaded.");
	}
	PlatesParser::PlatesRotationData rotation_data;
	try {

		PlatesParser::ReadInRotationData(filename.c_str(), f,
		 rotation_data);

	} catch (const FileIOException &e) {

		std::ostringstream msg("An error was encountered in \"");
		msg << filename << "\":\n" << e;
		Dialogs::ErrorMessage("Error in rotation file",
		 msg.str().c_str(), "No rotation data was loaded.");

	} catch (const GPlatesGlobal::Exception &e) {

		std::cerr << "Internal exception: " << e << std::endl;
		exit(1);
	}
}


void
File::Quit(const GPlatesGlobal::integer_t& exit_status)
{
	exit(exit_status);
}
