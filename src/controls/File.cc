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

#include <algorithm>  /* transform */
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>  /* std::auto_ptr */
#include <netcdfcpp.h>
#include <sstream>
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
#include "fileio/NetCDFReader.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "global/NotYetImplementedException.h"
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

		return GPlatesMaths::LatLonPoint::LatLonPoint(lat, lon);
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

	void HandleNetCDFFile (const std::string &filename)
	{
		NcFile ncf (filename.c_str ());
		std::ostringstream msg;

		if (!ncf.is_valid ()) {
			OpenFileErrorMessage (filename, "Couldn't open file!");
			return;
		}

		int i;
		NcAtt *att_title = 0;
		for (i = 0; i < ncf.num_atts (); ++i)
			if (strcmp (ncf.get_att (i)->as_string (0), "title")) {
				att_title = ncf.get_att (i);
				break;
			}
		if (!att_title) {
			Dialogs::InfoMessage ("netCDF File",
				"Data file doesn't have a title;"
				"trying to continue anyway.");
		} else {
			msg << "Loading data file with title:\n";
			msg << "    \"" << att_title->as_string (0) << "\"\n";
			Dialogs::InfoMessage ("netCDF File",
							msg.str ().c_str ());
			delete att_title;
		}

		GPlatesGeo::GridData *gdata =
			GPlatesFileIO::NetCDFReader::Read (&ncf);
		if (!gdata) {
			Dialogs::ErrorMessage ("netCDF File",
					"Loading Failed!", "...");
			return;
		}

		// TODO: do something useful with this GridData ptr!
		delete gdata;
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


namespace DataFormats {

	enum data_format {

		ERROR,
		UNKNOWN,
		GPML,
		PLATES,
		NETCDF
	};

	enum data_format testGPML(const std::string &filename);
	enum data_format testPLATES(const std::string &filename);
	enum data_format testNetCDF(const std::string &filename);

	// A pointer to a function which attempts to determine the file format.
	typedef enum data_format (*DataFormatTest)(const std::string &);

	DataFormatTest NATIVE_DATA_FORMAT_TESTS[] = {

		testGPML
	};

	DataFormatTest NONNATIVE_DATA_FORMAT_TESTS[] = {

		testPLATES,
		testNetCDF
	};


	enum data_format
	determineDataFormat(const std::string &filename, DataFormatTest *tests,
	                    size_t num_tests) {

		for (size_t i = 0; i < num_tests; i++) {

			enum data_format df = (tests[i])(filename);
			if (df == ERROR) return ERROR;
			else if (df != UNKNOWN) return df;
		}
		return UNKNOWN;
	}


	bool
	extensionMatches(const std::string &fname, const std::string &ext) {

		std::string::size_type idx,
		                       fname_len = fname.length(),
		                       ext_len = ext.length();

		if (ext_len >= fname_len) {

			/* Filename is not long enough to contain extension.
			 * Note that we use '>=' instead of '>' to disallow
			 * filenames which consist entirely of the extension. */
			return false;
		}
		idx = fname.rfind(ext);
		if (idx + ext_len != fname_len) {

			// Extension string is not where extension should be.
			return false;
		}
		return true;
	}


	enum data_format
	testGPML(const std::string &filename) {

		// Test file suffix for a quick disqualification.
		if ( ! extensionMatches(filename, ".gpml")) {

			// expected extension does not match
			return UNKNOWN;
		}

		// Attempt to recognise the file-type by reading a bit.
		std::ifstream ifs(filename.c_str());
		if ( ! ifs) {

			OpenFileErrorMessage(filename, "Couldn't open file.");
			return ERROR;
		}

		static const char expected_start[] = "<?xml";
		char start[sizeof(expected_start)];

		size_t expected_start_len = std::strlen(expected_start);
		ifs.get(start, expected_start_len);
		start[expected_start_len] = '\0';
		ifs.close();

		if (std::strcmp(expected_start, start) != 0) {

			// does not match expected GPML
			return UNKNOWN;
		}
		return GPML;
	}


	enum data_format
	testPLATES(const std::string &filename) {

		// Test file suffix for a quick disqualification.
		if ( ! extensionMatches(filename, ".dat")) {

			// expected extension does not match
			return UNKNOWN;
		}
		return PLATES;
	}


	enum data_format
	testNetCDF(const std::string &filename) {

		// Test file suffix for a quick disqualification.
		if ( ! extensionMatches(filename, ".grd")) {

			// expected extension does not match
			return UNKNOWN;
		}
		return NETCDF;
	}
}


void
GPlatesControls::File::OpenData(const std::string& filename)
{
	enum DataFormats::data_format file_type =
	 DataFormats::determineDataFormat(filename,
	  DataFormats::NATIVE_DATA_FORMAT_TESTS,
	  (sizeof(DataFormats::NATIVE_DATA_FORMAT_TESTS) /
	   sizeof(DataFormats::DataFormatTest)));

	switch (file_type) {

		case DataFormats::GPML:
			// Recognised as a GPML file
			HandleGPMLFile(filename);
			break;

		case DataFormats::ERROR:
			// Already complained about this
			return;

		default:
			// No luck finding a match
			std::ostringstream msg;
			msg << "The file \"" << filename
			 << "\" is in an unrecognised format.";
			Dialogs::ErrorMessage("File type not recognised",
			 msg.str().c_str(),
			 "Couldn't open file.");
			return;
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
GPlatesControls::File::LoadRotation(const std::string& filename)
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


void GPlatesControls::File::ImportData(const std::string& filename)
{
	enum DataFormats::data_format file_type =
	 DataFormats::determineDataFormat(filename,
	  DataFormats::NONNATIVE_DATA_FORMAT_TESTS,
	  (sizeof(DataFormats::NONNATIVE_DATA_FORMAT_TESTS) /
	   sizeof(DataFormats::DataFormatTest)));

	switch (file_type) {

		case DataFormats::PLATES:
			// Recognised as a Plates file
			HandlePLATESFile(filename);
			break;

		case DataFormats::NETCDF:
			// Recognised as a NetCDF file
			HandleNetCDFFile(filename);
			break;

		case DataFormats::ERROR:
			// Already complained about this
			return;

		default:
			// No luck finding a match
			std::ostringstream msg;
			msg << "The file \"" << filename
			 << "\" is in an unrecognised format.";
			Dialogs::ErrorMessage("File type not recognised",
			 msg.str().c_str(),
			 "Couldn't open file.");
			return;
	}

	// Currently works for PLATES data files only...
	if (file_type == DataFormats::PLATES) {

		DataGroup* data = GPlatesState::Data::GetDataGroup();
		if (!data)
			return;

		ConvertDataGroupToDrawableDataMap(data);

		// Draw the data on the screen in its present-day layout
		Reconstruct::Present();
	}
}

void
GPlatesControls::File::Quit(const GPlatesGlobal::integer_t& exit_status)
{
	exit(exit_status);
}

void
GPlatesControls::File::SaveData(const std::string& filepath)
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
