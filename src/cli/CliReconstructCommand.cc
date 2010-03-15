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


#include "CliReconstructCommand.h"
#include "CliFeatureCollectionFileIO.h"
#include "CliRequiredOptionNotPresent.h"

#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionReaderWriter.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/Model.h"
#include "model/Reconstruction.h"


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

	//! Option name for reconstruction time with short version.
	const char *RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION = "recon-time,t";

	//! Option name for anchor plate id with short version.
	const char *ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "anchor-plate-id,a";
}


GPlatesCli::ReconstructCommand::ReconstructCommand() :
	d_recon_time(0),
	d_anchor_plate_id(0)
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
		(LOAD_RECONSTRUCTABLE_OPTION_NAME_WITH_SHORT_OPTION,
				// std::vector allows multiple load files and
				// 'composing()' allows merging of command-line and config files.
				boost::program_options::value< std::vector<std::string> >()->composing(),
				"load reconstructable feature collection file (multiple options allowed)")
		(LOAD_RECONSTRUCTION_OPTION_NAME_WITH_SHORT_OPTION,
				// std::vector allows multiple load files and
				// 'composing()' allows merging of command-line and config files.
				boost::program_options::value< std::vector<std::string> >()->composing(),
				"load reconstruction feature collection file (multiple options allowed)")
		(RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION,
				boost::program_options::value<double>(&d_recon_time)->default_value(0),
				"set reconstruction time (defaults to zero)")
		(ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
				boost::program_options::value<GPlatesModel::integer_plate_id_type>(
						&d_anchor_plate_id)->default_value(0),
				"set anchor plate id (defaults to zero)")
		;

#if 0
	// The feature collection files can also be specified directly on command-line
	// without requiring the option prefix.
	// '-1' means unlimited arguments are allowed.
	positional_options.add(LOAD_RECONSTRUCTABLE_OPTION_NAME, -1);
#endif
}


int
GPlatesCli::ReconstructCommand::run(
		const boost::program_options::variables_map &vm)
{
	FeatureCollectionFileIO file_io(d_model, vm);

	//
	// Load the feature collection files
	//

	FeatureCollectionFileIO::feature_collection_file_seq_type reconstructable_files =
			file_io.load_files(LOAD_RECONSTRUCTABLE_OPTION_NAME);
	FeatureCollectionFileIO::feature_collection_file_seq_type reconstruction_files =
			file_io.load_files(LOAD_RECONSTRUCTION_OPTION_NAME);

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
			reconstructable_feature_collections,
			reconstruction_feature_collections;
	extract_feature_collections(
			reconstructable_feature_collections, reconstructable_files);
	extract_feature_collections(
			reconstruction_feature_collections, reconstruction_files);


	//
	// Currently we just reconstruct feature collections
	// and export reconstructed geometries to GMT format.
	//

	// Perform reconstruction.
	const GPlatesModel::Reconstruction::non_null_ptr_type reconstruction =
			GPlatesAppLogic::ReconstructUtils::create_reconstruction(
					reconstructable_feature_collections,
					reconstruction_feature_collections,
					d_recon_time,
					d_anchor_plate_id);

	// Get the reconstruction geometries.
	const GPlatesModel::Reconstruction::geometry_collection_type &reconstruction_geometries =
			reconstruction->geometries();

	// Get any ReconstructionGeometry objects that are of type ReconstructedFeatureGeometry.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::reconstructed_feature_geom_seq_type
			reconstruct_feature_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geometries.begin(),
			reconstruction_geometries.end(),
			reconstruct_feature_geom_seq);

	// Get the sequence of reconstructable files as File pointers.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::files_collection_type reconstructable_file_ptrs;
	loaded_feature_collection_file_seq_type::const_iterator file_iter = reconstructable_files.begin();
	loaded_feature_collection_file_seq_type::const_iterator file_end = reconstructable_files.end();
	for ( ; file_iter != file_end; ++file_iter)
	{
		reconstructable_file_ptrs.push_back(file_iter->get());
	}

	// Export filename - currently a GMT format file.
	const QString export_filename("reconstructed.xy");

	// Export the reconstructed feature geometries.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_geometries(
				export_filename,
				reconstruct_feature_geom_seq,
				reconstructable_file_ptrs,
				d_anchor_plate_id,
				d_recon_time);

	return 0;
}
