/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <iostream>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "CliEquivalentTotalRotation.h"

#include "CliFeatureCollectionFileIO.h"
#include "CliInvalidOptionValue.h"
#include "CliRequiredOptionNotPresent.h"

#include "app-logic/ReconstructParams.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/LogException.h"

#include "maths/LatLonPoint.h"

#include "model/Model.h"


namespace
{
	//! Option name for loading reconstruction feature collection file(s).
	const char *LOAD_RECONSTRUCTION_OPTION_NAME = "load-reconstruction";
	//! Option name for loading reconstruction feature collection file(s) with short version.
	const char *LOAD_RECONSTRUCTION_OPTION_NAME_WITH_SHORT_OPTION = "load-reconstruction,r";

	//! Option name for reconstruction time with short version.
	const char *RECONSTRUCTION_TIME_OPTION_NAME_WITH_SHORT_OPTION = "recon-time,t";

	//! Option name for anchor plate id with short version.
	const char *ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "anchor-plate-id,a";

	//! Option name for plate id with short version.
	const char *PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "plate-id,p";

	//! Option name for replacing 'Indeterminate' rotations with zero-angle north pole.
	const char *INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME =
			"indeterminate-is-zero-angle-north-pole";
	//! Option name for replacing 'Indeterminate' rotations with zero-angle north pole with short version.
	const char *INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME_WITH_SHORT_OPTION =
			"indeterminate-is-zero-angle-north-pole,i";
}


GPlatesCli::EquivalentTotalRotationCommand::EquivalentTotalRotationCommand() :
	d_recon_time(0),
	d_anchor_plate_id(0),
	d_plate_id(0)
{
}


void
GPlatesCli::EquivalentTotalRotationCommand::add_options(
		boost::program_options::options_description &generic_options,
		boost::program_options::options_description &config_options,
		boost::program_options::options_description &hidden_options,
		boost::program_options::positional_options_description &positional_options)
{
	config_options.add_options()
		(
			LOAD_RECONSTRUCTION_OPTION_NAME_WITH_SHORT_OPTION,
			// std::vector allows multiple load files and
			// 'composing()' allows merging of command-line and config files.
			boost::program_options::value< std::vector<std::string> >()->composing(),
			"load reconstruction feature collection file (multiple options allowed)"
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
			PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<GPlatesModel::integer_plate_id_type>(
					&d_plate_id)->default_value(0),
			"set plate id (defaults to zero)"
		)
		(
			INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME_WITH_SHORT_OPTION,
			"output '(90.0, 0.0, 0.0)' instead of 'Indeterminate' for identity rotations"
		)
		;

	// The feature collection files can also be specified directly on command-line
	// without requiring the option prefix.
	// '-1' means unlimited arguments are allowed.
	positional_options.add(LOAD_RECONSTRUCTION_OPTION_NAME, -1);
}


void
GPlatesCli::EquivalentTotalRotationCommand::run(
		const boost::program_options::variables_map &vm)
{
	// Output 'Indeterminate' unless specified otherwise.
	const bool output_indeterminate_for_identity_rotations =
			vm.count(INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME) == 0;

	FeatureCollectionFileIO file_io(d_model, vm);

	// Load the reconstruction feature collection files
	FeatureCollectionFileIO::feature_collection_file_seq_type reconstruction_files =
			file_io.load_files(LOAD_RECONSTRUCTION_OPTION_NAME);

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstruction_feature_collections;
	FeatureCollectionFileIO::extract_feature_collections(
			reconstruction_feature_collections, reconstruction_files);

	// Create a reconstruction tree from the rotation features.
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree =
			GPlatesAppLogic::create_reconstruction_tree(
					d_recon_time,
					d_anchor_plate_id,
					reconstruction_feature_collections);

	const std::pair<GPlatesMaths::FiniteRotation, GPlatesAppLogic::ReconstructionTree::ReconstructionCircumstance>
			equivalent_rotation_result =
					reconstruction_tree->get_composed_absolute_rotation(d_plate_id);

	if (equivalent_rotation_result.second == GPlatesAppLogic::ReconstructionTree::NoPlateIdMatchesFound)
	{
		// Return failure if plate is was not found in the reconstruction tree.
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to find plate id in reconstruction tree.");
	}

	// Get the equivalent rotation.
	const GPlatesMaths::FiniteRotation &equivalent_rotation = equivalent_rotation_result.first;
	const GPlatesMaths::UnitQuaternion3D &unit_quaternion = equivalent_rotation.unit_quat();
	
	if (GPlatesMaths::represents_identity_rotation(unit_quaternion)) 
	{
		if (output_indeterminate_for_identity_rotations)
		{
			std::cout << "Indeterminate" << std::endl;
		}
		else
		{
			std::cout << "(90.0, 0.0, 0.0)" << std::endl;
		}
	} 
	else 
	{
		GPlatesMaths::UnitQuaternion3D::RotationParams finite_rotation_params =
				unit_quaternion.get_rotation_params(equivalent_rotation.axis_hint());

		GPlatesMaths::PointOnSphere euler_pole(finite_rotation_params.axis);
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(euler_pole);

		std::cout << "("
				<< llp.latitude() << ", "
				<< llp.longitude() << ", "
				<< GPlatesMaths::convert_rad_to_deg(finite_rotation_params.angle).dval()
				<< ")" << std::endl;
	}
}
