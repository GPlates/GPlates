/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015, 2016 Geological Survey of Norway
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

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"

#include "NetRotationUtils.h"
#include "ReconstructionTree.h"
#include "RotationUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::create(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesMaths::FiniteRotation &stage_pole,
		const double &time_interval,
		const double &sample_square_length_in_radians)
{
	if (GPlatesMaths::are_almost_exactly_equal(time_interval, 0))
	{
		return NetRotationAccumulator();
	}

	if (represents_identity_rotation(stage_pole.unit_quat()))
	{
		return NetRotationAccumulator();
	}

	const GPlatesMaths::Vector3D stage_pole_vector = convert_finite_rotation_to_rotation_vector(stage_pole, time_interval);

	const GPlatesMaths::Vector3D v = cross(stage_pole_vector, point.position_vector());

	const GPlatesMaths::Vector3D omega = cross(point.position_vector(), v);

	const double x = point.position_vector().x().dval();
	const double y = point.position_vector().y().dval();
	const double cos_latitude_squared = x * x + y * y;
	// Cosine is positive for the full latitude range [-pi/2, pi/2].
	const double cos_latitude = std::sqrt(cos_latitude_squared);

	// Area of the sample square surrounding the sample point (in steradians, or square radians).
	const double area_steradians = cos_latitude * sample_square_length_in_radians * sample_square_length_in_radians;

	// Rotation component is weighted by the sample area.
	const GPlatesMaths::Vector3D rotation_component = omega * area_steradians;

	// Rotation component is weight by the sample area.
	const double weighting_factor = cos_latitude_squared * area_steradians;

	return NetRotationAccumulator(rotation_component, weighting_factor, area_steradians);
}

void
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::add(
		const NetRotationAccumulator &net_rotation)
{
	d_rotation_component = d_rotation_component + net_rotation.d_rotation_component;
	d_weighting_factor += net_rotation.d_weighting_factor;
	d_area_steradians += net_rotation.d_area_steradians;
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_rotation() const
{
	if (GPlatesMaths::are_almost_exactly_equal(d_weighting_factor, 0))
	{
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}

	const GPlatesMaths::Vector3D weighted_rotation_component = (1.0 / d_weighting_factor) * d_rotation_component;

	// Extract finite rotation from rotation rate vector.
	return convert_rotation_vector_to_finite_rotation(weighted_rotation_component);
}

boost::optional<std::pair<GPlatesMaths::LatLonPoint, double>>
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_rotation_lat_lon_pole_and_angle() const
{
	// Get the net rotation as a finite rotation.
	const GPlatesMaths::FiniteRotation finite_rotation = get_net_rotation();

	const GPlatesMaths::UnitQuaternion3D &uq = finite_rotation.unit_quat();
	if (represents_identity_rotation(uq))
	{
		return boost::none;
	}

	// Convert finite rotation to a lat-lon pole and angle (in degrees).
	const GPlatesMaths::UnitQuaternion3D::RotationParams params = uq.get_rotation_params(finite_rotation.axis_hint());
	const GPlatesMaths::LatLonPoint llp = make_lat_lon_point(GPlatesMaths::PointOnSphere(params.axis));
	const double angle = convert_rad_to_deg(params.angle).dval();

	return std::make_pair(llp, angle);
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::convert_rotation_vector_to_finite_rotation(
		const GPlatesMaths::Vector3D &rotation_vec)
{
	if (rotation_vec.is_zero_magnitude())
	{
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}

	const double rotation_angle = rotation_vec.magnitude().dval();

	const GPlatesMaths::PointOnSphere rotation_pole(
			GPlatesMaths::UnitVector3D((1.0 / rotation_angle) * rotation_vec));

	return GPlatesMaths::FiniteRotation::create(rotation_pole, rotation_angle);
}

GPlatesMaths::Vector3D
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::convert_finite_rotation_to_rotation_vector(
		const GPlatesMaths::FiniteRotation &finite_rotation,
		const double &time_interval)
{
	const GPlatesMaths::UnitQuaternion3D &uq = finite_rotation.unit_quat();

	const GPlatesMaths::UnitQuaternion3D::RotationParams params = uq.get_rotation_params(finite_rotation.axis_hint());

	// Convert angle from radians to radians/Myr, and scale the axis with it.
	return (params.angle / time_interval) * GPlatesMaths::Vector3D(params.axis);
}


GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::NetRotationCalculator(
		const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
		const resolved_topological_network_seq_type &resolved_topological_networks,
		const double &time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		unsigned int num_samples_along_meridian) :
	d_resolved_topological_boundaries(resolved_topological_boundaries),
	d_resolved_topological_networks(resolved_topological_networks),
	d_time(time),
	d_velocity_delta_time(velocity_delta_time),
	d_velocity_delta_time_type(velocity_delta_time_type),
	d_velocity_time_period(VelocityDeltaTime::get_time_range(velocity_delta_time_type, time, velocity_delta_time)),
	d_anchor_plate_id(anchor_plate_id),
	d_num_samples_along_meridian(num_samples_along_meridian)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			num_samples_along_meridian > 0,
			GPLATES_ASSERTION_SOURCE);
	const unsigned int num_samples_along_parallel = 2 * num_samples_along_meridian;

	const double delta_in_degrees = 180.0 / num_samples_along_meridian;
	const double delta_in_radians = GPlatesMaths::convert_deg_to_rad(delta_in_degrees);

	// Loop over lat-lon grid and calculate the rotation contribution at each point.
	for (unsigned int latitude_index = 0; latitude_index <= num_samples_along_meridian; ++latitude_index)
	{
		const double latitude = (latitude_index == num_samples_along_meridian)
				? 90.0
				: -90.0 + latitude_index * delta_in_degrees;

		for (unsigned int longitude_index = 0; longitude_index <= num_samples_along_parallel; ++longitude_index)
		{
			const double longitude = (longitude_index == num_samples_along_parallel)
					? 180.0
					: -180.0 + longitude_index * delta_in_degrees;

			const GPlatesMaths::PointOnSphere position = GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(latitude, longitude));

			// Add net rotation contribution from a deforming network first, otherwise from a rigid plate.
			if (!add_net_rotation_contribution_from_resolved_networks(position, delta_in_radians))
			{
				add_net_rotation_contribution_from_resolved_boundaries(position, delta_in_radians);
			}
		}
	}
}

bool
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution_from_resolved_networks(
		const GPlatesMaths::PointOnSphere &position,
		const double &sample_square_length_in_radians)
{
	// Check which deforming network (if any) the position lies in.
	for (auto network_ptr : d_resolved_topological_networks)
	{
		// See if point is in network boundary and if so, return the stage rotation.
		boost::optional< std::pair<
				GPlatesMaths::FiniteRotation,
				ResolvedTriangulation::Network::PointLocation> > point_stage_rotation =
						network_ptr->get_triangulation_network().calculate_stage_rotation(
								position,
								d_velocity_delta_time,
								d_velocity_delta_time_type);
		if (point_stage_rotation)
		{
			const NetRotationAccumulator net_rotation_contribution =
					NetRotationAccumulator::create(
							position,
							point_stage_rotation->first,
							d_velocity_delta_time,
							sample_square_length_in_radians);

			add_net_rotation_contribution(network_ptr, net_rotation_contribution);

			return true;
		}
	}

	return false;
}

bool
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution_from_resolved_boundaries(
		const GPlatesMaths::PointOnSphere &position,
		const double &sample_square_length_in_radians)
{
	// Check which rigid (non-deforming) plate (if any) the position lies in.
	for (auto boundary_ptr : d_resolved_topological_boundaries)
	{
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type boundary = boundary_ptr->resolved_topology_boundary();

		// Get the stage rotation from the plate ID.
		// If resolved boundary has no plate ID then the position does not contribute net rotation.
		const boost::optional<GPlatesMaths::FiniteRotation> boundary_stage_pole = get_resolved_boundary_stage_pole(boundary_ptr);
		if (boundary_stage_pole)
		{
			if (boundary->is_point_in_polygon(position,GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
			{
				const NetRotationAccumulator net_rotation_contribution =
						NetRotationAccumulator::create(
							position,
							boundary_stage_pole.get(),
							d_velocity_delta_time,
							sample_square_length_in_radians);

				add_net_rotation_contribution(boundary_ptr, net_rotation_contribution);

				return true;
			}
		}
	}

	return false;
}

void
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution(
		ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network,
		const NetRotationAccumulator &net_rotation_contribution)
{
	d_topological_network_net_rotation_map[resolved_topological_network].add(net_rotation_contribution);
	d_plate_id_net_rotation_map[resolved_topological_network->plate_id()].add(net_rotation_contribution);
	d_total_net_rotation.add(net_rotation_contribution);
}

void
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution(
		ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary,
		const NetRotationAccumulator &net_rotation_contribution)
{
	d_topological_boundary_net_rotation_map[resolved_topological_boundary].add(net_rotation_contribution);
	d_plate_id_net_rotation_map[resolved_topological_boundary->plate_id()].add(net_rotation_contribution);
	d_total_net_rotation.add(net_rotation_contribution);
}

boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::get_resolved_boundary_stage_pole(
		ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary) const
{
	const boost::optional<GPlatesModel::integer_plate_id_type> boundary_plate_id = resolved_topological_boundary->plate_id();
	if (!boundary_plate_id)
	{
		return boost::none;
	}

	// See if plate ID is already in the map. If not then calculate and insert into map.
	stage_pole_map_type::const_iterator it = d_resolved_boundary_stage_pole_map.find(boundary_plate_id.get());
	if (it != d_resolved_boundary_stage_pole_map.end())
	{
		return it->second;
	}

	// Get the stage pole for the plate ID.
	const ReconstructionTree::non_null_ptr_to_const_type tree_older =
			resolved_topological_boundary->get_reconstruction_tree_creator().get_reconstruction_tree(
					d_velocity_time_period.first/*older*/);

	const ReconstructionTree::non_null_ptr_to_const_type tree_younger =
			resolved_topological_boundary->get_reconstruction_tree_creator().get_reconstruction_tree(
					d_velocity_time_period.second/*younger*/);

	const GPlatesMaths::FiniteRotation stage_pole = RotationUtils::get_stage_pole(
			*tree_older, *tree_younger,
			boundary_plate_id.get(), d_anchor_plate_id);

	// Insert the stage pole into the plate ID map.
	auto insert_result = d_resolved_boundary_stage_pole_map.insert(stage_pole_map_type::value_type(boundary_plate_id.get(), stage_pole));

	return insert_result.first->second;
}
