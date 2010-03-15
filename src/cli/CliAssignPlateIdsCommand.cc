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
#include <string>
#include <boost/bind.hpp>

#include "CliAssignPlateIdsCommand.h"
#include "CliInvalidOptionValue.h"
#include "CliFeatureCollectionFileIO.h"
#include "CliRequiredOptionNotPresent.h"

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

	//! Option name for assign plate ids method.
	const char *ASSIGN_METHOD_OPTION_NAME = "assign-method";
	//! Option name for assign plate ids method with short version.
	const char *ASSIGN_METHOD_OPTION_NAME_WITH_SHORT_OPTION = "assign-method,m";

	//! Option name for reconstruction time with short version.
	const char *RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION = "recon-time,t";

	//! Option name for anchor plate id with short version.
	const char *ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "anchor-plate-id,a";

	//
	// Values specified by user on command-line for method used to assign plate ids.
	//
	const unsigned int ASSIGN_METHOD_ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE = 1;
	const unsigned int ASSIGN_METHOD_ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE = 2;
	const unsigned int ASSIGN_METHOD_PARTITION_FEATURE = 3;


	/**
	 * Parses command-line option to get the assign plate ids method.
	 */
	GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType
	get_assign_plate_ids_method(
			const boost::program_options::variables_map &vm)
	{
		const unsigned int assign_method =
				vm[ASSIGN_METHOD_OPTION_NAME].as<unsigned int>();

		switch (assign_method)
		{
		case ASSIGN_METHOD_ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE:
			return GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE;

		case ASSIGN_METHOD_ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE:
			return GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE;

		case ASSIGN_METHOD_PARTITION_FEATURE:
			return GPlatesAppLogic::AssignPlateIds::PARTITION_FEATURE;
		}

		throw GPlatesCli::InvalidOptionValue(
				GPLATES_EXCEPTION_SOURCE,
				ASSIGN_METHOD_OPTION_NAME,
				"valid values are '1', '2' and '3'.");
	}
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
		(ASSIGN_METHOD_OPTION_NAME_WITH_SHORT_OPTION,
				boost::program_options::value<unsigned int>()->default_value(
						ASSIGN_METHOD_ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE),
				"method used to assign plate ids (defaults to '1') - valid values are:\n"
				"1 - assign features to most overlapping plate\n"
				"2 - assign feature sub geometries to most overlapping plate\n"
				"3 - partition features into plates\n")
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

	//
	// Load the feature collection files
	//

	// The topological closed plate boundary features and the boundary features they reference.
	FeatureCollectionFileIO::feature_collection_file_seq_type topological_boundary_files =
			file_io.load_files(TOPOLOGICAL_BOUNDARY_FILES_OPTION_NAME);
	// The features that will have their plate ids (re)assigned.
	FeatureCollectionFileIO::feature_collection_file_seq_type assign_plate_ids_files =
			file_io.load_files(ASSIGN_PLATE_ID_FILES_OPTION_NAME);
	// The rotation files used to rotate both the topological boundary features and
	// the features having their plate ids (re)assigned.
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
			get_assign_plate_ids_method(vm);

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
