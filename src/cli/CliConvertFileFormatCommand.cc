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

#include <cstddef> // For std::size_t

#include "CliConvertFileFormatCommand.h"
#include "CliFeatureCollectionFileIO.h"
#include "CliRequiredOptionNotPresent.h"

#include "file-io/FileInfo.h"

#include "model/FeatureCollectionHandle.h"


namespace
{
	//! Option name for loading feature collection file(s).
	const char *LOAD_FEATURE_COLLECTION_OPTION_NAME = "load-fc";
	//! Option name for loading feature collection file(s) with short version.
	const char *LOAD_FEATURE_COLLECTION_OPTION_NAME_WITH_SHORT_OPTION = "load-fc,l";

	//! Option name for type of file to save.
	const char *SAVE_FILE_TYPE_OPTION_NAME = "save-file-type";
	//! Option name for type of file to save with short version.
	const char *SAVE_FILE_TYPE_OPTION_NAME_WITH_SHORT_OPTION = "save-file-type,e";

	//! Option name for prefix of saved filenames.
	const char *SAVE_FILE_PREFIX_OPTION_NAME = "save-file-prefix";
	//! Option name for prefix of saved filenames with short option.
	const char *SAVE_FILE_PREFIX_OPTION_NAME_WITH_SHORT_OPTION = "save-file-prefix,p";

	//! Option name for suffix of saved filenames.
	const char *SAVE_FILE_SUFFIX_OPTION_NAME = "save-file-suffix";
	//! Option name for suffix of saved filenames with short option.
	const char *SAVE_FILE_SUFFIX_OPTION_NAME_WITH_SHORT_OPTION = "save-file-suffix,s";
}


GPlatesCli::ConvertFileFormatCommand::ConvertFileFormatCommand()
{
}


void
GPlatesCli::ConvertFileFormatCommand::add_options(
		boost::program_options::options_description &generic_options,
		boost::program_options::options_description &config_options,
		boost::program_options::options_description &hidden_options,
		boost::program_options::positional_options_description &positional_options)
{
	config_options.add_options()
		(
			LOAD_FEATURE_COLLECTION_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			"load feature collection file (multiples options allowed)")
		(
			SAVE_FILE_PREFIX_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>(&d_save_file_prefix)->default_value(""),
			"prefix to prepend to filename of saved files (defaults to '')"
		)
		(
			SAVE_FILE_SUFFIX_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>(&d_save_file_suffix)->default_value(""),
			"suffix to append to filename of saved files (defaults to '')"
		)
		(
			SAVE_FILE_TYPE_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>(&d_save_file_type)->default_value(
					FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML),
			(std::string(
					"file type to save feature collections(defaults to '")
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML
					+ "') - valid values are:\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML
					+ " - GPlates native GPML format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML_GZ
					+ " - GPlates native GPML format compressed with gzip\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_SHAPEFILE
					+ " - ArcGIS Shapefile format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT
					+ " - Generic Mapping Tools (GMT) format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_PLATES_LINE
					+ " - PLATES version 4.0 line format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_PLATES_ROTATION
					+ " - PLATES version 4.0 rotation format\n").c_str()
		)
		;

	// The feature collection files can also be specified directly on command-line
	// without requiring the option prefix.
	// '-1' means unlimited arguments are allowed.
	positional_options.add(LOAD_FEATURE_COLLECTION_OPTION_NAME, -1);
}


int
GPlatesCli::ConvertFileFormatCommand::run(
		const boost::program_options::variables_map &vm)
{
	FeatureCollectionFileIO file_io(d_model, vm);

	//
	// Load the feature collection files
	//

	FeatureCollectionFileIO::feature_collection_file_seq_type files =
			file_io.load_files(LOAD_FEATURE_COLLECTION_OPTION_NAME);

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collections;
	FeatureCollectionFileIO::extract_feature_collections(
			feature_collections, files);

	// Iterate through the feature collection files and save them to files.
	for (std::size_t file_index = 0; 
		file_index < files.size();
		++file_index)
	{
		const GPlatesFileIO::File &input_file = *files[file_index];
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
				GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
						feature_collections[file_index]);

		// Get the save filename.
		const GPlatesFileIO::FileInfo save_file_info =
				FeatureCollectionFileIO::get_save_file_info(
						input_file.get_file_info(),
						d_save_file_type,
						d_save_file_prefix.c_str(),
						d_save_file_suffix.c_str());

		// Save the file with (re)assigned plate ids.
		FeatureCollectionFileIO::save_file(save_file_info, feature_collection);
	}

	return 0;
}
