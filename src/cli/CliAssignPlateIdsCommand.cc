/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
#include <cstddef> // For std::size_t
#include <string>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <QString>

#include "CliAssignPlateIdsCommand.h"
#include "CliInvalidOptionValue.h"
#include "CliFeatureCollectionFileIO.h"
#include "CliRequiredOptionNotPresent.h"

#include "app-logic/AssignPlateIds.h"
#include "app-logic/Reconstruction.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/Model.h"


namespace
{
	//! Option name for partitioning feature collection file(s).
	const char *PARTITIONING_FILES_OPTION_NAME = "load-partitioning-features";
	//! Option name with short version for partitioning file(s).
	const char *PARTITIONING_FILES_OPTION_NAME_WITH_SHORT_OPTION = "load-partitioning-features,p";

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

	//! Option name for assign plate id with short version.
	const char *ASSIGN_PLATE_ID_WITH_SHORT_OPTION = "assign-plate-id,d";

	//! Option name for assign time period with short version.
	const char *ASSIGN_TIME_PERIOD_WITH_SHORT_OPTION = "assign-time-period,e";

	//! Option name for respect time period with short version.
	const char *RESPECT_TIME_PERIOD_WITH_SHORT_OPTION = "respect-time-period,i";

	//! Option name for type of file to save.
	const char *SAVE_FILE_TYPE_OPTION_NAME = "save-file-type";
	//! Option name for type of file to save with short version.
	const char *SAVE_FILE_TYPE_OPTION_NAME_WITH_SHORT_OPTION = "save-file-type,s";

	//! Option name for prefix of saved filenames.
	const char *SAVE_FILE_PREFIX_OPTION_NAME = "save-file-prefix";

	//! Option name for suffix of saved filenames.
	const char *SAVE_FILE_SUFFIX_OPTION_NAME = "save-file-suffix";

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
				ASSIGN_METHOD_OPTION_NAME);
	}


	/**
	 * Parses command-line option to get the save file type.
	 */
	std::string
	get_save_file_type(
			const boost::program_options::variables_map &vm)
	{
		const std::string &save_file_type =
				vm[SAVE_FILE_TYPE_OPTION_NAME].as<std::string>();

		// We're only allowing a subset of the save file types that make sense for us.
		if (save_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML ||
			save_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GPMLZ ||
			save_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_PLATES_LINE ||
			save_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_SHAPEFILE ||
			save_file_type == GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT)
		{
			return save_file_type;
		}

		throw GPlatesCli::InvalidOptionValue(
				GPLATES_EXCEPTION_SOURCE,
				save_file_type.c_str());
	}
}


GPlatesCli::AssignPlateIdsCommand::AssignPlateIdsCommand() :
	d_recon_time(0),
	d_assign_plate_id(true),
	d_assign_time_period(false),
	d_respect_time_period(false),
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
		(
			PARTITIONING_FILES_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			"load partitioning feature collection file - dynamic or static polygons (multiple options allowed)"
		)
		(
			ASSIGN_PLATE_ID_FILES_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			"load feature collection file to have plate ids (re)assigned (multiple options allowed)"
		)
		(
			RECONSTRUCTION_FILES_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			(std::string("load reconstruction feature collection file (multiple options allowed) - "
					"this is optional if '") + RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION +
					"' is zero.").c_str()
		)
		(
			ASSIGN_METHOD_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<unsigned int>()->default_value(
					ASSIGN_METHOD_PARTITION_FEATURE),
			(std::string("method used to assign plate ids (defaults to '")
					+ boost::lexical_cast<std::string>(
							ASSIGN_METHOD_PARTITION_FEATURE)
					+ "') - valid values are:\n"
					+ boost::lexical_cast<std::string>(
							ASSIGN_METHOD_ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE)
					+ " - assign features to most overlapping plate\n"
					+ boost::lexical_cast<std::string>(
							ASSIGN_METHOD_ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE)
					+ " - assign feature sub geometries to most overlapping plate\n"
					+ boost::lexical_cast<std::string>(
							ASSIGN_METHOD_PARTITION_FEATURE)
					+ " - partition features into plates\n").c_str()
		)
		(
			ASSIGN_PLATE_ID_WITH_SHORT_OPTION,
			boost::program_options::value<bool>(&d_assign_plate_id)->default_value(true),
			"assign reconstruction plate id (defaults to 'true')."
		)
		(
			ASSIGN_TIME_PERIOD_WITH_SHORT_OPTION,
			boost::program_options::value<bool>(&d_assign_time_period)->default_value(false),
			"assign time period (defaults to 'false')."
		)
		(
			RESPECT_TIME_PERIOD_WITH_SHORT_OPTION,
			boost::program_options::value<bool>(&d_respect_time_period)->default_value(false),
			"only partition features that exist at the reconstruction time (defaults to 'false')."
		)
		(
			SAVE_FILE_TYPE_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>()->default_value(
					FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML),
			(std::string(
					"file type to save feature collections with (re)assigned plate ids (defaults to '")
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML
					+ "') - valid values are:\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML
					+ " - GPlates native GPML format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GPMLZ
					+ " - GPlates native GPML format compressed with gzip\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_SHAPEFILE
					+ " - ArcGIS Shapefile format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT
					+ " - Generic Mapping Tools (GMT) format\n"
					+ FeatureCollectionFileIO::SAVE_FILE_TYPE_PLATES_LINE
					+ " - PLATES version 4.0 line format\n").c_str()
		)
		(
			SAVE_FILE_PREFIX_OPTION_NAME,
			boost::program_options::value<std::string>(&d_save_file_prefix)->default_value(""),
			"prefix to prepend to filename of saved files (defaults to '')"
		)
		(
			SAVE_FILE_SUFFIX_OPTION_NAME,
			boost::program_options::value<std::string>(&d_save_file_suffix)->default_value(""),
			"suffix to append to filename of saved files (defaults to '')"
		)
		(
			RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<double>(&d_recon_time)->default_value(0),
			"set reconstruction time at which to cookie-cut when assigning plate ids (defaults to zero)"
		)
		(
			ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<GPlatesModel::integer_plate_id_type>(
					&d_anchor_plate_id)->default_value(0),
			"set anchor plate id (defaults to zero)"
		)
		;

	// The (re)assigned plate id feature collection files can also be specified directly
	// on the command-line without requiring the option prefix.
	// '-1' means unlimited arguments are allowed.
	positional_options.add(ASSIGN_PLATE_ID_FILES_OPTION_NAME, -1);
}


void
GPlatesCli::AssignPlateIdsCommand::run(
		const boost::program_options::variables_map &vm)
{
	FeatureCollectionFileIO file_io(d_model, vm);

	//
	// Load the feature collection files
	//

	// The partitioning features. Either:
	//   * topological closed plate boundary features and the boundary features they reference, or
	//   * static polygon features.
	FeatureCollectionFileIO::feature_collection_file_seq_type partitioning_files =
			file_io.load_files(PARTITIONING_FILES_OPTION_NAME);
	// The features that will have their plate ids (re)assigned.
	FeatureCollectionFileIO::feature_collection_file_seq_type assign_plate_ids_files =
			file_io.load_files(ASSIGN_PLATE_ID_FILES_OPTION_NAME);
	// The rotation files used to rotate both the topological boundary features and
	// the features having their plate ids (re)assigned.
	FeatureCollectionFileIO::feature_collection_file_seq_type reconstruction_files;
	// Reconstruction files are optional as long as the reconstruction time is zero.
	if (vm.count(RECONSTRUCTION_FILES_OPTION_NAME) == 0)
	{
		if (d_recon_time > 0)
		{
			throw RequiredOptionNotPresent(
					GPLATES_EXCEPTION_SOURCE,
					RECONSTRUCTION_FILES_OPTION_NAME,
					std::string("A reconstruction feature collection is required for a "
							"non-zero reconstruction time."));
		}
	}
	else
	{
		reconstruction_files = file_io.load_files(RECONSTRUCTION_FILES_OPTION_NAME);
	}

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
			partitioning_feature_collections,
			assign_plate_ids_feature_collections,
			reconstruction_feature_collections;
	FeatureCollectionFileIO::extract_feature_collections(
			partitioning_feature_collections, partitioning_files);
	FeatureCollectionFileIO::extract_feature_collections(
			assign_plate_ids_feature_collections, assign_plate_ids_files);
	FeatureCollectionFileIO::extract_feature_collections(
			reconstruction_feature_collections, reconstruction_files);

	// The method used to assign plate ids.
	const GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_ids_method =
			get_assign_plate_ids_method(vm);

	// Get the feature properties to assign.
	GPlatesAppLogic::AssignPlateIds::feature_property_flags_type assign_feature_property_flags;
	if (d_assign_plate_id)
	{
		assign_feature_property_flags.set(GPlatesAppLogic::AssignPlateIds::RECONSTRUCTION_PLATE_ID);
	}
	if (d_assign_time_period)
	{
		assign_feature_property_flags.set(GPlatesAppLogic::AssignPlateIds::VALID_TIME);
	}

	// The save filename information used to save the feature collections.
	const std::string save_file_type = get_save_file_type(vm);

	// Create the object used to assign plate ids.
	const GPlatesAppLogic::AssignPlateIds::non_null_ptr_type plate_id_assigner =
			GPlatesAppLogic::AssignPlateIds::create(
					assign_plate_ids_method,
					partitioning_feature_collections,
					reconstruction_feature_collections,
					d_recon_time,
					d_anchor_plate_id,
					assign_feature_property_flags,
					true/*allow_partitioning_using_topological_plate_polygons*/,
					true/*allow_partitioning_using_topological_networks*/,
					true/*allow_partitioning_using_static_polygons*/,
					d_respect_time_period);

	// Assign plate ids to the features.
	// Do this after checking all command-line parameters since assigning plate ids
	// can take a long time and we don't want to pop up a command-line error afterwards.
	std::for_each(
			assign_plate_ids_feature_collections.begin(),
			assign_plate_ids_feature_collections.end(),
			boost::bind(
					&GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids,
					boost::ref(plate_id_assigner),
					_1));

	// Iterate through the feature collection files that had their plate ids (re)assigned and
	// save them to file.
	for (std::size_t file_index = 0; 
		file_index < assign_plate_ids_files.size();
		++file_index)
	{
		const GPlatesFileIO::File::Reference &input_file = *assign_plate_ids_files[file_index];
		const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				assign_plate_ids_feature_collections[file_index];

		// Get the save filename.
		const GPlatesFileIO::FileInfo save_file_info =
				file_io.get_save_file_info(
						input_file.get_file_info(),
						save_file_type,
						d_save_file_prefix.c_str(),
						d_save_file_suffix.c_str());

		// Save the file with (re)assigned plate ids.
		file_io.save_file(save_file_info, feature_collection);
	}
}
