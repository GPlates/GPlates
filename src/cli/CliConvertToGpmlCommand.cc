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

#include "CliConvertToGpmlCommand.h"

#include "CliRequiredOptionNotPresent.h"

#include "app-logic/AppLogicUtils.h"

#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionReaderWriter.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/Model.h"


namespace
{
	//! Option name for loading feature collection file(s).
	const char *LOAD_FEATURE_COLLECTION_OPTION_NAME = "load-fc";
	//! Option name for loading feature collection file(s) with short version.
	const char *LOAD_FEATURE_COLLECTION_OPTION_NAME_WITH_SHORT_OPTION = "load-fc,l";

	//! Same as @a OUTPUT_SUFFIX_OPTION_NAME with added short version.
	const char *OUTPUT_SUFFIX_OPTION_NAME_WITH_SHORT_OPTION = "suffix,s";
}


GPlatesCli::ConvertToGpmlCommand::ConvertToGpmlCommand()
{
}


void
GPlatesCli::ConvertToGpmlCommand::add_options(
		boost::program_options::options_description &generic_options,
		boost::program_options::options_description &config_options,
		boost::program_options::options_description &hidden_options,
		boost::program_options::positional_options_description &positional_options)
{
	config_options.add_options()
		(LOAD_FEATURE_COLLECTION_OPTION_NAME_WITH_SHORT_OPTION,
				// std::vector allows multiple load files and
				// 'composing()' allows merging of command-line and config files.
				boost::program_options::value< std::vector<std::string> >()->composing(),
				"load feature collection file (multiples options allowed)")
		(OUTPUT_SUFFIX_OPTION_NAME_WITH_SHORT_OPTION,
				boost::program_options::value<std::string>(
						&d_output_basename_suffix)->default_value("_BATCH"),
				"the suffix added to input file basename to get output filename")
		;

	// The feature collection files can also be specified directly on command-line
	// without requiring the option prefix.
	// '-1' means unlimited arguments are allowed.
	positional_options.add(LOAD_FEATURE_COLLECTION_OPTION_NAME, -1);
}


int
GPlatesCli::ConvertToGpmlCommand::run(
		const boost::program_options::variables_map &vm)
{
    if (!vm.count(LOAD_FEATURE_COLLECTION_OPTION_NAME))
    {
		throw RequiredOptionNotPresent(
				GPLATES_EXCEPTION_SOURCE, LOAD_FEATURE_COLLECTION_OPTION_NAME);
	}

	// Get the feature collection filenames.
	const std::vector<std::string> filenames =
			vm[LOAD_FEATURE_COLLECTION_OPTION_NAME].as< std::vector<std::string> >();

	typedef std::vector<GPlatesFileIO::File> loaded_feature_collection_file_seq_type;

	// Load each feature collection file and save it in GPML format.
	std::vector<std::string>::const_iterator filename_iter = filenames.begin();
	std::vector<std::string>::const_iterator filename_end = filenames.end();
	for ( ; filename_iter != filename_end; ++filename_iter)
	{
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// Read the feature collection from the file.
		const QString filename(filename_iter->c_str());
		const GPlatesFileIO::FileInfo file_info(filename);
		const GPlatesFileIO::File::shared_ref file =
				GPlatesFileIO::read_feature_collection(file_info, d_model, read_errors);

		// Generate the output filename.
		QString output_filename = filename;
		const int ext_index = output_filename.lastIndexOf('.');
		if (ext_index >= 0)
		{
			// Remove extension.
			output_filename = output_filename.left(ext_index);
		}
		output_filename.append(d_output_basename_suffix.c_str());
		output_filename.append(".gpml");

		// Save feature collection in GPML format.
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
				file->get_const_feature_collection();
		GPlatesFileIO::FileInfo output_file_info(output_filename);
		GPlatesFileIO::GpmlOnePointSixOutputVisitor gpml_writer(output_file_info, false);
		GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
				feature_collection, gpml_writer);
	}

	return 0;
}
