/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <boost/foreach.hpp>

#include "CliReconstructCommand.h"
#include "CliFeatureCollectionFileIO.h"
#include "CliInvalidOptionValue.h"
#include "CliRequiredOptionNotPresent.h"

#include "app-logic/ReconstructParams.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/Model.h"


namespace
{
	//! Option name for loading reconstructable feature collection file(s).
	const char *LOAD_RECONSTRUCTABLE_OPTION_NAME = "load-reconstructable";
	//! Option name for loading reconstructable feature collection file(s) with short version.
	const char *LOAD_RECONSTRUCTABLE_OPTION_NAME_WITH_SHORT_OPTION = "load-reconstructable,l";

	//! Option name for loading reconstruction feature collection file(s).
	const char *LOAD_RECONSTRUCTION_OPTION_NAME = "load-reconstruction";
	//! Option name for loading reconstruction feature collection file(s) with short version.
	const char *LOAD_RECONSTRUCTION_OPTION_NAME_WITH_SHORT_OPTION = "load-reconstruction,r";

	//! Option name for filename to export with short version.
	const char *EXPORT_FILENAME_OPTION_NAME_WITH_SHORT_OPTION = "export-filename,o";

	//! Option name for type of file to export.
	const char *EXPORT_FILE_TYPE_OPTION_NAME = "export-file-type";
	//! Option name for type of file to export with short version.
	const char *EXPORT_FILE_TYPE_OPTION_NAME_WITH_SHORT_OPTION = "export-file-type,e";

	//! Option name for reconstruction time with short version.
	const char *RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION = "recon-time,t";

	//! Option name for anchor plate id with short version.
	const char *ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "anchor-plate-id,a";

	//! Option name for outputting to a single file with short version.
	const char *SINGLE_OUTPUT_FILE_OPTION_NAME_WITH_SHORT_OPTION = "single-output-file,s";


	/**
	 * Parses command-line option to get the export file type.
	 */
	std::string
	get_export_file_type(
			const boost::program_options::variables_map &vm)
	{
		const std::string &export_file_type =
				vm[EXPORT_FILE_TYPE_OPTION_NAME].as<std::string>();

		// We're only allowing a subset of the save file types that make sense for us.
		if (export_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT ||
			export_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_SHAPEFILE)
		{
			return export_file_type;
		}

		throw GPlatesCli::InvalidOptionValue(
				GPLATES_EXCEPTION_SOURCE,
				export_file_type.c_str());
	}
}


GPlatesCli::ReconstructCommand::ReconstructCommand() :
	d_recon_time(0),
	d_anchor_plate_id(0),
	d_export_single_output_file(true)
{
}


void
GPlatesCli::ReconstructCommand::add_options(
		boost::program_options::options_description &generic_options,
		boost::program_options::options_description &config_options,
		boost::program_options::options_description &hidden_options,
		boost::program_options::positional_options_description &positional_options)
{
	config_options.add_options()
		(
			LOAD_RECONSTRUCTABLE_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			"load reconstructable feature collection file (multiple options allowed)"
		)
		(
			LOAD_RECONSTRUCTION_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			"load reconstruction feature collection file (multiple options allowed)"
		)
		(
			EXPORT_FILENAME_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>(&d_export_filename)
					->default_value("reconstructed"),
			"export filename without extension (defaults to 'reconstructed')"
		)
		(
			EXPORT_FILE_TYPE_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>()->default_value(
					FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT),
			(std::string(
					"file type to export reconstructed geometries (defaults to '")
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT
					+ "') - valid values are:\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT
					+ " - Generic Mapping Tools (GMT) format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_SHAPEFILE
					+ " - ArcGIS Shapefile format\n").c_str()
		)
		(
			RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<double>(&d_recon_time)->default_value(0),
			"set reconstruction time (defaults to zero)"
		)
		(
			ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<GPlatesModel::integer_plate_id_type>(
					&d_anchor_plate_id)->default_value(0),
			"set anchor plate id (defaults to zero)"
		)
		(
			SINGLE_OUTPUT_FILE_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<bool>(&d_export_single_output_file)->default_value(true),
			"output to a single file (defaults to 'true')\n"
			"  NOTE: Only applies if export file type is Shapefile in which case\n"
			"  'false' will generate a matching output file for each input file."
		)
		;

	// The feature collection files can also be specified directly on command-line
	// without requiring the option prefix.
	// '-1' means unlimited arguments are allowed.
	positional_options.add(LOAD_RECONSTRUCTABLE_OPTION_NAME, -1);
}


int
GPlatesCli::ReconstructCommand::run(
		const boost::program_options::variables_map &vm)
{
	FeatureCollectionFileIO file_io(d_model, vm);

	//
	// Load the feature collection files
	//

	qDebug() << "Single: " << d_export_single_output_file;

	FeatureCollectionFileIO::feature_collection_file_seq_type reconstructable_files =
			file_io.load_files(LOAD_RECONSTRUCTABLE_OPTION_NAME);
	FeatureCollectionFileIO::feature_collection_file_seq_type reconstruction_files =
			file_io.load_files(LOAD_RECONSTRUCTION_OPTION_NAME);

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
			reconstructable_feature_collections,
			reconstruction_feature_collections;
	FeatureCollectionFileIO::extract_feature_collections(
			reconstructable_feature_collections, reconstructable_files);
	FeatureCollectionFileIO::extract_feature_collections(
			reconstruction_feature_collections, reconstruction_files);

	// The export filename information.
	const std::string export_file_type = get_export_file_type(vm);

	//
	// Currently we just reconstruct feature collections
	// and export reconstructed geometries to GMT format.
	//

	// Perform reconstruction.
	std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
	GPlatesAppLogic::ReconstructUtils::reconstruct(
			reconstructed_feature_geometries,
			d_recon_time,
			d_anchor_plate_id,
			reconstructable_feature_collections,
			reconstruction_feature_collections);

	// Converts to raw pointers.
	std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> reconstruct_feature_geom_seq;
	reconstruct_feature_geom_seq.reserve(reconstructed_feature_geometries.size());
	BOOST_FOREACH(
			const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg,
			reconstructed_feature_geometries)
	{
		reconstruct_feature_geom_seq.push_back(rfg.get());
	}

	// Get the sequence of reconstructable files as File pointers.
	std::vector<const GPlatesFileIO::File::Reference *> reconstructable_file_ptrs;
	loaded_feature_collection_file_seq_type::const_iterator file_iter = reconstructable_files.begin();
	loaded_feature_collection_file_seq_type::const_iterator file_end = reconstructable_files.end();
	for ( ; file_iter != file_end; ++file_iter)
	{
		reconstructable_file_ptrs.push_back(&(*file_iter)->get_reference());
	}

	// Export filename.
	const GPlatesFileIO::FileInfo export_filename =
			file_io.get_save_file_info(
					d_export_filename.c_str(),
					export_file_type);

	// Export the reconstructed feature geometries.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
				export_filename.get_qfileinfo().filePath(),
				GPlatesFileIO::ReconstructedFeatureGeometryExport::get_export_file_format(
						export_filename.get_qfileinfo().filePath(),
						file_io.get_file_format_registry()),
				reconstruct_feature_geom_seq,
				reconstructable_file_ptrs,
				d_anchor_plate_id,
				d_recon_time,
				d_export_single_output_file/*export_single_output_file*/,
				!d_export_single_output_file/*export_per_input_file*/);

	return 0;
}
