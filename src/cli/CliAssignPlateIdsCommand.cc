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


#include <algorithm>
#include <boost/bind.hpp>

#include "CliAssignPlateIdsCommand.h"
#include "CliRequiredOptionNotPresent.h"
#include "CliFeatureCollectionFileIO.h"

#include "app-logic/AssignPlateIds.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionReaderWriter.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/Model.h"
#include "model/Reconstruction.h"


namespace
{
	//! Option name for topological close plate boundary feature collection file(s).
	const char *TOPOLOGICAL_BOUNDARY_FILES_OPTION_NAME = "load-topological-boundaries";
	//! Option name with short version for topological close plate boundary file(s).
	const char *TOPOLOGICAL_BOUNDARY_FILES_OPTION_NAME_WITH_SHORT_OPTION = "load-topological-boundaries,p";

	//! Option name for feature collection file(s) having plate ids (re)assigned.
	const char *ASSIGN_PLATE_ID_FILES_OPTION_NAME = "load-assign-plate-id-files";
	//! Option name with short version for feature collection file(s) having plate ids (re)assigned.
	const char *ASSIGN_PLATE_ID_FILES_OPTION_NAME_WITH_SHORT_OPTION = "load-assign-plate-id-files,l";

	//! Option name for loading reconstruction feature collection file(s).
	const char *RECONSTRUCTION_FILES_OPTION_NAME = "load-reconstruction";
	//! Option name for loading reconstruction feature collection file(s) with short version.
	const char *RECONSTRUCTION_FILES_OPTION_NAME_WITH_SHORT_OPTION = "load-reconstruction,r";

	//! Option name for reconstruction time with short version.
	const char *RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION = "recon-time,t";

	//! Option name for anchor plate id with short version.
	const char *ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "anchor-plate-id,a";
}


GPlatesCli::AssignPlateIdsCommand::AssignPlateIdsCommand() :
	d_recon_time(0),
	d_anchor_plate_id(0)
{
}


void
GPlatesCli::AssignPlateIdsCommand::add_options(
		boost::program_options::options_description &generic_options,
		boost::program_options::options_description &config_options,
		boost::program_options::options_description &hidden_options,
		boost::program_options::positional_options_description &positional_options)
{
	config_options.add_options()
		(TOPOLOGICAL_BOUNDARY_FILES_OPTION_NAME_WITH_SHORT_OPTION,
				// std::vector allows multiple load files and
				// 'composing()' allows merging of command-line and config files.
				boost::program_options::value< std::vector<std::string> >()->composing(),
				"load topological boundaries feature collection file (multiple options allowed)")
		(ASSIGN_PLATE_ID_FILES_OPTION_NAME_WITH_SHORT_OPTION,
				// std::vector allows multiple load files and
				// 'composing()' allows merging of command-line and config files.
				boost::program_options::value< std::vector<std::string> >()->composing(),
				"load feature collection file to have plate ids (re)assigned (multiple options allowed)")
		(RECONSTRUCTION_FILES_OPTION_NAME_WITH_SHORT_OPTION,
				// std::vector allows multiple load files and
				// 'composing()' allows merging of command-line and config files.
				boost::program_options::value< std::vector<std::string> >()->composing(),
				"load reconstruction feature collection file (multiple options allowed)")
		(RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION,
				boost::program_options::value<double>(&d_recon_time)->default_value(0),
				"set reconstruction time at which to cookie-cut when assigning plate ids (defaults to zero)")
		(ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
				boost::program_options::value<GPlatesModel::integer_plate_id_type>(
						&d_anchor_plate_id)->default_value(0),
				"set anchor plate id (defaults to zero)")
		;
}


int
GPlatesCli::AssignPlateIdsCommand::run(
		const boost::program_options::variables_map &vm)
{
	FeatureCollectionFileIO file_io(d_model, vm);

	// Load the feature collection files.
	// The feature collections exist as long as these file sequences exist.
	FeatureCollectionFileIO::feature_collection_file_seq_type topological_boundary_files =
			file_io.load_files(TOPOLOGICAL_BOUNDARY_FILES_OPTION_NAME);
	FeatureCollectionFileIO::feature_collection_file_seq_type assign_plate_ids_files =
			file_io.load_files(ASSIGN_PLATE_ID_FILES_OPTION_NAME);
	FeatureCollectionFileIO::feature_collection_file_seq_type reconstruction_files =
			file_io.load_files(RECONSTRUCTION_FILES_OPTION_NAME);

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
			topological_boundary_feature_collections,
			assign_plate_ids_feature_collections,
			reconstruction_feature_collections;
	extract_feature_collections(
			topological_boundary_feature_collections, topological_boundary_files);
	extract_feature_collections(
			assign_plate_ids_feature_collections, assign_plate_ids_files);
	extract_feature_collections(
			reconstruction_feature_collections, reconstruction_files);

	// The method used to assign plate ids.
	const GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_ids_method =
			GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE;

	// Create the object used to assign plate ids.
	const GPlatesAppLogic::AssignPlateIds::non_null_ptr_type plate_id_assigner =
			GPlatesAppLogic::AssignPlateIds::create_at_new_reconstruction_time(
					assign_plate_ids_method,
					d_model,
					topological_boundary_feature_collections,
					reconstruction_feature_collections,
					d_recon_time,
					d_anchor_plate_id);

	// Assign plate ids to the features.
	std::for_each(
			assign_plate_ids_feature_collections.begin(),
			assign_plate_ids_feature_collections.end(),
			boost::bind(
					&GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids,
					boost::ref(plate_id_assigner),
					_1));

	return 0;
}


void
GPlatesCli::AssignPlateIdsCommand::load_feature_collections(
		const boost::program_options::variables_map &vm)
{
    if (!vm.count(ASSIGN_PLATE_ID_FILES_OPTION_NAME))
    {
		throw RequiredOptionNotPresent(
				GPLATES_EXCEPTION_SOURCE, ASSIGN_PLATE_ID_FILES_OPTION_NAME);
	}

    if (!vm.count(RECONSTRUCTION_FILES_OPTION_NAME))
    {
		throw RequiredOptionNotPresent(
				GPLATES_EXCEPTION_SOURCE, RECONSTRUCTION_FILES_OPTION_NAME);
	}

	// Get the feature collection filenames.
	const std::vector<std::string> reconstructable_filenames =
			vm[ASSIGN_PLATE_ID_FILES_OPTION_NAME].as< std::vector<std::string> >();
	const std::vector<std::string> reconstruction_filenames =
			vm[RECONSTRUCTION_FILES_OPTION_NAME].as< std::vector<std::string> >();

	load_feature_collections(reconstructable_filenames, d_loaded_assign_plate_id_files);
	load_feature_collections(reconstruction_filenames, d_loaded_reconstruction_files);
}


void
GPlatesCli::AssignPlateIdsCommand::load_feature_collections(
		const std::vector<std::string> &filenames,
		loaded_feature_collection_file_seq_type &files)
{
	// Load each feature collection file.
	std::vector<std::string>::const_iterator filename_iter = filenames.begin();
	std::vector<std::string>::const_iterator filename_end = filenames.end();
	for ( ; filename_iter != filename_end; ++filename_iter)
	{
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// Read the feature collection from the file.
		const QString filename(filename_iter->c_str());
		const GPlatesFileIO::FileInfo file_info(filename);
		GPlatesFileIO::File::shared_ref file =
				GPlatesFileIO::read_feature_collection(file_info, d_model, read_errors);

		files.push_back(file);
	}
}


void
GPlatesCli::AssignPlateIdsCommand::get_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
		loaded_feature_collection_file_seq_type &files)
{
	// Load each feature collection file.
	loaded_feature_collection_file_seq_type::iterator file_iter = files.begin();
	loaded_feature_collection_file_seq_type::iterator file_end = files.end();
	for ( ; file_iter != file_end; ++file_iter)
	{
		feature_collections.push_back((*file_iter)->get_feature_collection());
	}
}
