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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>  /* transform */
#include <memory>  /* std::auto_ptr */
#include <iomanip>
#include <iterator>
#include "File.h"
#include "Reconstruct.h"
#include "Dialogs.h"
#include "state/Data.h"
#include "fileio/PlatesRotationParser.h"
#include "maths/OperationsOnSphere.h"
#include "fileio/PlatesBoundaryParser.h"
#include "fileio/PlatesPostParseTranslator.h"
#include "fileio/GPlatesReader.h"
#include "fileio/GPlatesWriter.h"
#include "fileio/FileIOException.h"
#include "fileio/FileFormatException.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "global/types.h"  /* rid_t */

using namespace GPlatesControls;
using namespace GPlatesFileIO;
using namespace GPlatesGeo;

namespace
{
	/**
	 * @todo Remove this definition, since it is a duplicate of
	 *   a function in PlatesPostParseTranslator.
	 */
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

		return GPlatesMaths::LatLonPoint::CreateLatLonPoint(lat, lon);
	}

	void
	OpenFileErrorMessage(const std::string &fname, const char *fail_result_msg) {

		std::ostringstream msg;
		msg << "The file \"" << fname << "\" could not\n"
		 << "be opened for reading.";

		Dialogs::ErrorMessage("Unable to open file", msg.str().c_str(),
		 fail_result_msg);
	}


	void
	HandleGPMLFile(const std::string& filename)
	{
		std::ifstream file(filename.c_str());
		if (!file) 
		{
			OpenFileErrorMessage(filename, "No GPML data was loaded.");
			return;
		}

		try 
		{
			GPlatesReader reader(file);
			DataGroup *data = reader.Read();
			GPlatesState::Data::SetDataGroup(data);
		} 
		// XXX Factor this shit out, MAN.
		catch (const Exception& e)
		{
			std::ostringstream msg, result;

			msg << "Parse error occurred.  Error message:\n"
				<< e;
			
			result << "No GPML data was loaded from \"" << filename
				<< "\"." << std::endl;
		
			Dialogs::ErrorMessage(
				"Error encountered.",
				msg.str().c_str(),
				result.str().c_str());
		}
	}


	using namespace GPlatesState;

	class AddGeoDataToDrawableMap
	{
		public:
			AddGeoDataToDrawableMap(Data::DrawableMap_type* map)
				: _map(map) {  }

			void
			operator()(GeologicalData* data) {
				
				const DataGroup* dg = dynamic_cast<const DataGroup*>(data);
				if (dg) {
					// Handle 'recursive' case
					
					std::for_each(dg->ChildrenBegin(),
								  dg->ChildrenEnd(), 
								  AddGeoDataToDrawableMap(_map));
					return;
				}
				
				DrawableData* drawabledata = dynamic_cast<DrawableData*>(data);
				if (!drawabledata) {
					// Anything other than a DataGroup should be drawable.
					// Therefore, if the cast above fails we have issues.
					std::cerr << "Internal error: DrawableData cast failure." 
						<< std::endl;
					File::Quit(1);
				}
				
				rid_t plate_id = drawabledata->GetRotationGroupId();
				(*_map)[plate_id].push_back(drawabledata);
			}
			
		private:
			Data::DrawableMap_type* _map;
	};

	void
	ConvertDataGroupToDrawableDataMap(DataGroup* data)
	{
		Data::DrawableMap_type* map = new Data::DrawableMap_type;

		AddGeoDataToDrawableMap AddData(map);
		AddData(data); 
		
		Data::SetDrawableData(map);
	}

	void
	HandlePLATESFile(const std::string& filename)
	{
		std::ifstream file(filename.c_str());
		if (!file) {
			OpenFileErrorMessage(filename, "No PLATES data was loaded.");
			return;
		}

		// filename is good for reading.
		PlatesParser::PlatesDataMap map;
		try 
		{
			PlatesParser::
				ReadInPlateBoundaryData(filename.c_str(), file, map);
			DataGroup *data = 
				PlatesPostParseTranslator::
					GetDataGroupFromPlatesDataMap(map);

			GPlatesState::Data::SetDataGroup(data);
		} 
		catch (const FileIOException& e)
		{
			std::ostringstream msg;
			msg << "An error was encountered in \"" << filename 
				<< "\"" << std::endl
				<< "Error message: " << std::endl << e;
			Dialogs::ErrorMessage(
				"Error in data file",
				msg.str().c_str(),
				"No PLATES data was loaded.");  
		} 
		catch (const GPlatesGlobal::Exception& ex)
		{
			std::cerr << "Internal exception: " << ex << std::endl;
			exit(1);
		}
	}
}

void
File::OpenData(const std::string& filename)
{
	enum { UNKNOWN, GPML, PLATES } filetype = UNKNOWN;

	// Read in some magic
	std::ifstream ifs(filename.c_str());
	if (!ifs) {
		OpenFileErrorMessage(filename, "Couldn't open file.");
		return;
	}
	char magic[8];
	ifs.get(magic, 7);
	magic[7] = '\0';
	ifs.close();

	// Break out of this loop when we know what type of file we're reading
	do {
		// Try magic
		if (!std::strncmp(magic, "<?xml", 5)) {
			filetype = GPML;
			break;
		} else if (strtol(magic, NULL, 0) != 0) {
			filetype = PLATES;
			break;
		}

		// Try file extension
		if (filename.rfind(".gpml") != std::string::npos) {
			filetype = GPML;
			break;
		} else if (filename.rfind(".dat") != std::string::npos) {
			filetype = PLATES;
			break;
		}
	} while (0);

	if (filetype == GPML)
		HandleGPMLFile(filename);
	else if (filetype == PLATES)
		HandlePLATESFile(filename);
	else {
		std::ostringstream msg;
		msg << "The file \"" << filename << "\" is in an unknown "
								"format.";
		Dialogs::ErrorMessage(
			"File type not recognised",
			msg.str().c_str(),
			"Attempting to parse the file as a PLATES data file.");
		// TODO: is this wise to guess that it's PLATES format? confirm with user?
		HandlePLATESFile(filename);
	}

	DataGroup* data = GPlatesState::Data::GetDataGroup();
	if (!data)
		return;

	ConvertDataGroupToDrawableDataMap(data);

	// Draw the data on the screen in its present-day layout
	Reconstruct::Present();
}


static GPlatesMaths::real_t
ConvertPlatesParserAngleToGPlatesMathsAngle(const fpdata_t &pp_angle) {

	GPlatesMaths::real_t angle_in_degrees(pp_angle);
	return GPlatesMaths::degreesToRadians(angle_in_degrees);
}


static GPlatesMaths::PointOnSphere
ConvertPlatesParserLLPToGPlatesMathsPOS(PlatesParser::LatLonPoint pp_llp) {

	GPlatesMaths::LatLonPoint llp =
	 ConvertPlatesParserLatLonToMathsLatLon(pp_llp);
	return GPlatesMaths::OperationsOnSphere::
	 convertLatLonPointToPointOnSphere(llp);
}


static GPlatesMaths::FiniteRotation
ConvertPlatesParserFinRotToGPlatesMathsFinRot(const
	PlatesParser::FiniteRotation &pp_fin_rot) {

	GPlatesMaths::real_t time(pp_fin_rot._time);
	GPlatesMaths::PointOnSphere pole =
	 ConvertPlatesParserLLPToGPlatesMathsPOS(pp_fin_rot._rot._pole);
	GPlatesMaths::real_t angle =
	 ConvertPlatesParserAngleToGPlatesMathsAngle(pp_fin_rot._rot._angle);

	return GPlatesMaths::FiniteRotation::CreateFiniteRotation(pole, angle, 
	 time);
}


static GPlatesMaths::RotationSequence
ConvertPlatesParserRotSeqToGPlatesMathsRotSeq(const
	PlatesParser::RotationSequence &pp_rot_seq) {

	GPlatesGlobal::rid_t fixed_plate(pp_rot_seq._fixed_plate);

	/*
	 * It is assumed that a PlatesParser RotationSequence will always
	 * contain at least one PlatesParser::FiniteRotation.
	 */
	std::list< PlatesParser::FiniteRotation >::const_iterator it =
	 pp_rot_seq._seq.begin();
	std::list< PlatesParser::FiniteRotation >::const_iterator end_it =
	 pp_rot_seq._seq.end();

	GPlatesMaths::FiniteRotation first_fin_rot =
	 ConvertPlatesParserFinRotToGPlatesMathsFinRot(*it);
	GPlatesMaths::RotationSequence rot_seq(fixed_plate, first_fin_rot);

	for (it++ ; it != end_it; it++) {

		GPlatesMaths::FiniteRotation fin_rot =
		 ConvertPlatesParserFinRotToGPlatesMathsFinRot(*it);
		rot_seq.insert(fin_rot);
	}
	return rot_seq;
}


static void
ConvertPlatesRotationDataToRotationMap(const
	PlatesParser::PlatesRotationData &data) {

	/*
	 * Avoid memory leaks which would occur if an exception were thrown
	 * in this function.
	 */
	std::auto_ptr< Data::RotationMap_type >
	 rotation_map(new Data::RotationMap_type());

	for (PlatesParser::PlatesRotationData::const_iterator it = data.begin();
	     it != data.end();
	     it++) {

		GPlatesGlobal::rid_t moving_plate((*it)._moving_plate);
		GPlatesMaths::RotationSequence rot_seq =
		 ConvertPlatesParserRotSeqToGPlatesMathsRotSeq(*it);

		(*rotation_map)[moving_plate].insert(rot_seq);
	}

	// Release the pointer from the auto_ptr
	Data::SetRotationHistories(rotation_map.release());
}


void
File::OpenRotation(const std::string& filename)
{
	std::ifstream f(filename.c_str());
	if ( ! f) {

		// attempt to open file was unsuccessful
		OpenFileErrorMessage(filename, "No rotation data was loaded.");
		return;
	}

	PlatesParser::PlatesRotationData data;
	try {

		PlatesParser::ReadInRotationData(filename.c_str(), f, data);
		ConvertPlatesRotationDataToRotationMap(data);

	} catch (const FileIOException &e) {

		std::ostringstream msg;
		msg << "An error was encountered in \"" << filename << "\":\n"
		 << e;
		Dialogs::ErrorMessage("Error in rotation file",
		 msg.str().c_str(), "No rotation data was loaded.");
		return;

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

void
File::SaveData(const std::string& filepath)
{
	std::ofstream outfile(filepath.c_str());
	if ( ! outfile) {
		
		// Could not open filepath for writing.
		std::ostringstream msg;
		msg << "The file \"" << filepath << "\" could not\n"
		 << "be opened for writing.";

		Dialogs::ErrorMessage("Unable to open file", msg.str().c_str(),
		 "No GPML data was saved!");
		return;
	}

	GPlatesWriter writer;
	DataGroup* data = GPlatesState::Data::GetDataGroup();

	if ( ! data ) {

		// No data to write.
		Dialogs::ErrorMessage(
			"You want me to create an empty file?", 
			"There is currently no data loaded for you to save.",
			"No GPML data was saved! -- Try loading something first.");
		return;
	}
	
	writer.Visit(*data);
	writer.PrintOut(outfile);
}
