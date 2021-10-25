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
#include <QString>

#include "CliStageRotationCommand.h"

#include "CliFeatureCollectionFileIO.h"
#include "CliInvalidOptionValue.h"
#include "CliRequiredOptionNotPresent.h"

#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/ReconstructParams.h"
#include "app-logic/ReconstructUtils.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/LogException.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"

#include "model/Model.h"

namespace
{
	//! Option name for loading reconstruction feature collection file(s).
	const char *LOAD_RECONSTRUCTION_OPTION_NAME = "load-reconstruction";
	//! Option name for loading reconstruction feature collection file(s) with short version.
	const char *LOAD_RECONSTRUCTION_OPTION_NAME_WITH_SHORT_OPTION = "load-reconstruction,r";

	//! Option name for start time with short version.
	const char *START_TIME_OPTION_NAME_WITH_SHORT_OPTION = "start-time,s";

	//! Option name for end time with short version.
	const char *END_TIME_OPTION_NAME_WITH_SHORT_OPTION = "end-time,e";

	//! Option name for anchor plate id with short version.
	const char *ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "anchor-plate-id,a";

	//! Option name for fixed plate id with short version.
	const char *FIXED_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "fixed-plate-id,f";

	//! Option name for moving plate id with short version.
	const char *MOVING_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION = "moving-plate-id,m";

	//! Option name for asymmetry with short version.
	const char *ASYMMETRY_OPTION_NAME_WITH_SHORT_OPTION = "asymmetry,y";

	//! Option name for enabling stage rotations relative to the anchor plate.
	const char *RELATIVE_TO_ANCHOR_PLATE_OPTION_NAME =
			"relative-to-anchor-plate";
	//! Option name enabling stage rotations relative to the anchor plate with short version.
	const char *RELATIVE_TO_ANCHOR_PLATE_OPTION_NAME_WITH_SHORT_OPTION =
			"relative-to-anchor-plate,l";

	//! Option name for replacing 'Indeterminate' rotations with zero-angle north pole.
	const char *INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME =
			"indeterminate-is-zero-angle-north-pole";
	//! Option name for replacing 'Indeterminate' rotations with zero-angle north pole with short version.
	const char *INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME_WITH_SHORT_OPTION =
			"indeterminate-is-zero-angle-north-pole,i";
}


GPlatesCli::StageRotationCommand::StageRotationCommand() :
	d_start_time(0),
	d_end_time(0),
	d_anchor_plate_id(0),
	d_fixed_plate_id(0),
	d_moving_plate_id(0),
	d_asymmetry(1)
{
}


void
GPlatesCli::StageRotationCommand::add_options(
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
			"load reconstruction feature collection (rotation) file (multiple options allowed)"
		)
		(
			START_TIME_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<double>(&d_start_time)->default_value(0),
			"set start time (defaults to zero)"
		)
		(
			END_TIME_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<double>(&d_end_time)->default_value(0),
			"set end time (defaults to zero)"
		)
		(
			ANCHOR_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<GPlatesModel::integer_plate_id_type>(
					&d_anchor_plate_id)->default_value(0),
			QString("set anchor plate id (defaults to zero) - only used with '%1' option")
					.arg(RELATIVE_TO_ANCHOR_PLATE_OPTION_NAME)
					.toLatin1().data()
		)
		(
			FIXED_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<GPlatesModel::integer_plate_id_type>(
					&d_fixed_plate_id)->default_value(0),
			"set fixed plate id (defaults to zero)"
		)
		(
			MOVING_PLATE_ID_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<GPlatesModel::integer_plate_id_type>(
					&d_moving_plate_id)->default_value(0),
			"set moving plate id (defaults to zero)"
		)
		(
			ASYMMETRY_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<double>(&d_asymmetry)->default_value(1),
			"set stage pole spreading rate asymmetry in range [-1,1] (defaults to 1.0) - "
			"asymmetry determines the ratio of the full-stage rotation angle according to "
			"'angle_ratio = (1 + a) / 2' - "
			"1.0 is a full-stage rotation and 0.0 is a half-stage rotation"
		)
		(
			RELATIVE_TO_ANCHOR_PLATE_OPTION_NAME_WITH_SHORT_OPTION,
			"output stage rotation relative to the anchor plate instead of relative to the fixed plate - "
			"this option uses the anchor plate id - "
			"useful for mid-ocean ridge stage rotations relative to the spin axis - "
			"not necessary when 'asymmetry' is '1.0' (full-stage rotation) since can instead set "
			"fixed plate id to the anchor plate"
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
GPlatesCli::StageRotationCommand::run(
		const boost::program_options::variables_map &vm)
{
	// Output 'Indeterminate' unless specified otherwise.
	const bool output_indeterminate_for_identity_rotations =
			vm.count(INDETERMINATE_IS_ZERO_ANGLE_NORTH_POLE_OPTION_NAME) == 0;

	// Output stage rotation relative to the anchor plate.
	const bool output_stage_rotation_relative_to_anchor_plate =
			vm.count(RELATIVE_TO_ANCHOR_PLATE_OPTION_NAME) != 0;

	FeatureCollectionFileIO file_io(d_model, vm);
	GPlatesFileIO::ReadErrorAccumulation read_errors;

	// Load the reconstruction feature collection files
	FeatureCollectionFileIO::feature_collection_file_seq_type reconstruction_files =
			file_io.load_files(LOAD_RECONSTRUCTION_OPTION_NAME, read_errors);

	// Report all file load errors (if any).
	FeatureCollectionFileIO::report_load_file_errors(read_errors);

	// Extract the feature collections from the owning files.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstruction_feature_collections;
	FeatureCollectionFileIO::extract_feature_collections(
			reconstruction_feature_collections, reconstruction_files);

	// Create reconstruction trees from the rotation features.
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type start_reconstruction_tree =
			GPlatesAppLogic::create_reconstruction_tree(
					d_start_time,
					d_anchor_plate_id,
					reconstruction_feature_collections);
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type end_reconstruction_tree =
			GPlatesAppLogic::create_reconstruction_tree(
					d_end_time,
					d_anchor_plate_id,
					reconstruction_feature_collections);

	// Let's make sure the anchor/fixed/moving plate ids are actually in the rotation files.
	if (start_reconstruction_tree->get_composed_absolute_rotation(d_anchor_plate_id).second ==
		GPlatesAppLogic::ReconstructionTree::NoPlateIdMatchesFound)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to find anchor plate id in rotation files.");
	}
	if (start_reconstruction_tree->get_composed_absolute_rotation(d_fixed_plate_id).second ==
		GPlatesAppLogic::ReconstructionTree::NoPlateIdMatchesFound)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to find fixed plate id in rotation files.");
	}
	if (start_reconstruction_tree->get_composed_absolute_rotation(d_moving_plate_id).second ==
		GPlatesAppLogic::ReconstructionTree::NoPlateIdMatchesFound)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to find moving plate id in rotation files.");
	}

	// Make sure the asymmetry parameter is within the range [-1,1].
	if (d_asymmetry < -1 || d_asymmetry > 1)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Asymmetry is not in the range [-1,1].");
	}

	// Get the full-stage pole rotation.
	const GPlatesMaths::FiniteRotation full_stage_rotation =
			GPlatesAppLogic::ReconstructUtils::get_stage_pole(
					*start_reconstruction_tree,
					*end_reconstruction_tree,
					d_moving_plate_id,
					d_fixed_plate_id);

	// Calculate the asymmetric stage rotation (if asymmetry is not 1.0).
	const GPlatesMaths::UnitQuaternion3D::RotationParams full_stage_rotation_params =
			full_stage_rotation.unit_quat().get_rotation_params(full_stage_rotation.axis_hint());

	const double asymmetry_angle_factor = 0.5 * (1 + d_asymmetry);
	const double asymmetry_angle = asymmetry_angle_factor * full_stage_rotation_params.angle.dval();

	// The asymmetric stage pole rotation.
	GPlatesMaths::FiniteRotation asymmetric_stage_rotation =
			GPlatesMaths::FiniteRotation::create(
					GPlatesMaths::UnitQuaternion3D::create_rotation(
							full_stage_rotation_params.axis,
							asymmetry_angle),
					full_stage_rotation.axis_hint());

	// If the stage rotation is meant to be relative to the anchor plate (instead of the fixed plate)...
	if (output_stage_rotation_relative_to_anchor_plate)
	{
		//
		// Rotation from anchor plate 'A' to mid-ocean ridge 'MOR' via left (or fixed) plate 'L'
		// from time 't1' to 't2':
		//
		// R(t1->t2,A->MOR)
		// R(0->t2,A->MOR) * R(t1->0,A->MOR)
		// R(0->t2,A->MOR) * inverse[R(0->t1,A->MOR)] // See NOTE 1
		// R(0->t2,A->MOR) * inverse[R(0->t1,A->L) * R(0->t1,L->MOR)]
		// R(0->t2,A->MOR) * inverse[R(0->t1,L->MOR)] * inverse[R(0->t1,A->L)]
		// R(0->t2,A->L) * R(0->t2,L->MOR) * inverse[R(0->t1,L->MOR)] * inverse[R(0->t1,A->L)]
		// R(0->t2,A->L) * R(0->t2,L->MOR) * R(t1->0,L->MOR) * inverse[R(0->t1,A->L)]
		// R(0->t2,A->L) * R(t1->t2,L->MOR) * inverse[R(0->t1,A->L)]
		// R(0->t2,A->L) * AsymmetricStageRotation(t1->t2,L->R) * inverse[R(0->t1,A->L)]
		//
		// Where A->B means rotation of plate B relative to plate A.
		//
		// NOTE 1: A rotation must be relative to present day (0Ma) before it can be separated into
		// a (plate circuit) chain of moving/fixed plate pairs.
		// See "ReconstructUtils::get_stage_pole()" for more details.
		//

		asymmetric_stage_rotation =
				compose(
						compose(
								end_reconstruction_tree->get_composed_absolute_rotation(d_fixed_plate_id).first,
								asymmetric_stage_rotation),
						get_reverse(start_reconstruction_tree->get_composed_absolute_rotation(d_fixed_plate_id).first));
	}

	// Output the stage rotation relative to the anchor plate.
	output_stage_rotation(
			asymmetric_stage_rotation,
			output_indeterminate_for_identity_rotations);
}


void
GPlatesCli::StageRotationCommand::output_stage_rotation(
		const GPlatesMaths::FiniteRotation &stage_rotation,
		bool output_indeterminate_for_identity_rotations)
{
	if (represents_identity_rotation(stage_rotation.unit_quat()))
	{
		if (output_indeterminate_for_identity_rotations)
		{
			std::cout << "Indeterminate" << std::endl;
		}
		else
		{
			std::cout << "(90.0, 0.0, 0.0)" << std::endl;
		}
		return;
	}

	const GPlatesMaths::UnitQuaternion3D::RotationParams stage_rotation__params =
			stage_rotation.unit_quat().get_rotation_params(stage_rotation.axis_hint());

	GPlatesMaths::PointOnSphere euler_pole(stage_rotation__params.axis);
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(euler_pole);

	std::cout << "("
			<< llp.latitude() << ", "
			<< llp.longitude() << ", "
			<< GPlatesMaths::convert_rad_to_deg(stage_rotation__params.angle)
			<< ")" << std::endl;
}
