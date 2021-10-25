/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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

#include <boost/utility/in_place_factory.hpp>

#include "ResolvedVertexSourceInfo.h"

#include "PlateVelocityUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructMethodType.h"
#include "RotationUtils.h"

#include "maths/MathsUtils.h"


GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedVertexSourceInfo::get_stage_rotation(
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	const stage_rotation_key_type stage_rotation_key = stage_rotation_key_type(
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);
	// If first time called or key does not match then calculate and cache a new stage rotation.
	if (!d_cached_stage_rotation ||
		stage_rotation_key != d_cached_stage_rotation->first)
	{
		// Calculate the stage rotation.
		d_cached_stage_rotation = std::make_pair(
				stage_rotation_key,
				calc_stage_rotation(reconstruction_time, velocity_delta_time, velocity_delta_time_type));
	}

	return d_cached_stage_rotation->second;
}


GPlatesMaths::Vector3D
GPlatesAppLogic::ResolvedVertexSourceInfo::get_velocity_vector(
		const GPlatesMaths::PointOnSphere &point,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	CalcVelocityVectorVisitor visitor(
			*this,
			point,
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);

	return boost::apply_visitor(visitor, d_source);
}


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::ResolvedVertexSourceInfo::get_reconstruction_tree_creator() const
{
	return boost::apply_visitor(GetReconstructionTreeCreatorVisitor(), d_source);
}


bool
GPlatesAppLogic::ResolvedVertexSourceInfo::operator==(
		const ResolvedVertexSourceInfo &rhs) const
{
	return boost::apply_visitor(EqualityVisitor(), d_source, rhs.d_source);
}


GPlatesAppLogic::ResolvedVertexSourceInfo::source_type
GPlatesAppLogic::ResolvedVertexSourceInfo::create_source_from_reconstruction_properties(
		const ReconstructedFeatureGeometry::non_null_ptr_to_const_type &reconstruction_properties)
{
	const ReconstructionTreeCreator reconstruction_tree_creator =
			reconstruction_properties->get_reconstruction_tree_creator();

	// Everything reconstructs either by plate ID or using half stage rotations.
	// If it's not reconstructed by half stage rotations then it defaults to by-plate-ID.
	//
	// Note that the topology builder tools now only allow RFGs by-plate-id and by-half-stage-rotation,
	// so these shouldn't occur in practice (but could if constructed outside GPlates somehow).
	if (reconstruction_properties->get_reconstruct_method_type() == ReconstructMethod::HALF_STAGE_ROTATION)
	{
		// Reconstruct using half-stage rotations.
		return source_type(HalfStageRotationProperties(reconstruction_tree_creator, reconstruction_properties));
	}
	else // reconstruct by plate ID...
	{
		// Get the reconstruction plate ID (defaults to zero).
		GPlatesModel::integer_plate_id_type plate_id = 0;
		if (reconstruction_properties->reconstruction_plate_id())
		{
			plate_id = reconstruction_properties->reconstruction_plate_id().get();
		}

		return source_type(PlateIdProperties(reconstruction_tree_creator, plate_id));
	}
}


GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedVertexSourceInfo::calc_stage_rotation(
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	CalcStageRotationVisitor visitor(
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);

	return boost::apply_visitor(visitor, d_source);
}


GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedVertexSourceInfo::CalcStageRotationVisitor::operator()(
			const PlateIdProperties &source) const
{
	return PlateVelocityUtils::calculate_stage_rotation(
			source.plate_id,
			source.reconstruction_tree_creator,
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedVertexSourceInfo::CalcStageRotationVisitor::operator()(
		const HalfStageRotationProperties &source) const
{
	const std::pair<double, double> time_range = VelocityDeltaTime::get_time_range(
			velocity_delta_time_type, reconstruction_time, velocity_delta_time);

	const ReconstructionFeatureProperties &source_reconstruction_params = source.get_reconstruction_params();

	return GPlatesMaths::calculate_stage_rotation(
			RotationUtils::get_half_stage_rotation(
					time_range.second/*young*/,
					source_reconstruction_params,
					source.reconstruction_tree_creator),
			RotationUtils::get_half_stage_rotation(
					time_range.first/*old*/,
					source_reconstruction_params,
					source.reconstruction_tree_creator));
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedVertexSourceInfo::CalcStageRotationVisitor::operator()(
		const FixedPointVelocityAdapter &source) const
{
	// Just delegate to our source (ignore our fixed point).
	return source.source_info->get_stage_rotation(
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedVertexSourceInfo::CalcStageRotationVisitor::operator()(
		const InterpolateVertexSourceInfos &source) const
{
	// Interpolate the stage rotation from both sources.
	return GPlatesMaths::interpolate(
			source.source_info1->get_stage_rotation(
					reconstruction_time,
					velocity_delta_time,
					velocity_delta_time_type),
			source.source_info2->get_stage_rotation(
					reconstruction_time,
					velocity_delta_time,
					velocity_delta_time_type),
			source.interpolate_ratio);
}


bool
GPlatesAppLogic::ResolvedVertexSourceInfo::EqualityVisitor::operator()(
		const PlateIdProperties &lhs,
		const PlateIdProperties &rhs) const
{
	return lhs.plate_id == rhs.plate_id;
}

bool
GPlatesAppLogic::ResolvedVertexSourceInfo::EqualityVisitor::operator()(
		const HalfStageRotationProperties &lhs,
		const HalfStageRotationProperties &rhs) const
{
	const ReconstructionFeatureProperties &lhs_reconstruction_params = lhs.get_reconstruction_params();
	const ReconstructionFeatureProperties &rhs_reconstruction_params = rhs.get_reconstruction_params();

	// Compare properties used in half stage rotation calculation.
	return lhs_reconstruction_params.get_left_plate_id() == rhs_reconstruction_params.get_left_plate_id() &&
			lhs_reconstruction_params.get_right_plate_id() == rhs_reconstruction_params.get_right_plate_id() &&
			lhs_reconstruction_params.get_geometry_import_time() == rhs_reconstruction_params.get_geometry_import_time() &&
			lhs_reconstruction_params.get_spreading_asymmetry() == rhs_reconstruction_params.get_spreading_asymmetry() &&
			lhs_reconstruction_params.get_reconstruction_method() == rhs_reconstruction_params.get_reconstruction_method();
}

bool
GPlatesAppLogic::ResolvedVertexSourceInfo::EqualityVisitor::operator()(
		const FixedPointVelocityAdapter &lhs,
		const FixedPointVelocityAdapter &rhs) const
{
	// Compare the fixed points and the delegated sources.
	return lhs.fixed_point == rhs.fixed_point &&
			boost::apply_visitor(EqualityVisitor(), lhs.source_info->d_source, rhs.source_info->d_source);
}

bool
GPlatesAppLogic::ResolvedVertexSourceInfo::EqualityVisitor::operator()(
		const InterpolateVertexSourceInfos &lhs,
		const InterpolateVertexSourceInfos &rhs) const
{
	// To be equal the interpolate ratios must match and each the source infos to be interpolated must match.
	return (GPlatesMaths::are_almost_exactly_equal(lhs.interpolate_ratio, rhs.interpolate_ratio) &&
				boost::apply_visitor(EqualityVisitor(), lhs.source_info1->d_source, rhs.source_info1->d_source) &&
				boost::apply_visitor(EqualityVisitor(), lhs.source_info2->d_source, rhs.source_info2->d_source)) ||
			// Also check for inverted interpolate ratio and sources...
			(GPlatesMaths::are_almost_exactly_equal(lhs.interpolate_ratio, 1.0 - rhs.interpolate_ratio) &&
				boost::apply_visitor(EqualityVisitor(), lhs.source_info1->d_source, rhs.source_info2->d_source) &&
				boost::apply_visitor(EqualityVisitor(), lhs.source_info2->d_source, rhs.source_info1->d_source));
}

bool
GPlatesAppLogic::ResolvedVertexSourceInfo::EqualityVisitor::operator()(
		const StageRotation &lhs,
		const StageRotation &rhs) const
{
	return lhs.stage_rotation.unit_quat() == rhs.stage_rotation.unit_quat();
}
