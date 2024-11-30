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

#include "scribe/Scribe.h"


GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::create(
		const GPlatesMaths::PointOnSphere &point,
		const double &sample_area_steradians,
		const GPlatesMaths::FiniteRotation &stage_pole,
		const double &time_interval)
{
	if (GPlatesMaths::are_almost_exactly_equal(time_interval, 0))
	{
		// Contributes zero net rotation, but still contributes non-zero area.
		return NetRotationAccumulator(sample_area_steradians);
	}

	if (represents_identity_rotation(stage_pole.unit_quat()))
	{
		// Contributes zero net rotation, but still contributes non-zero area.
		return NetRotationAccumulator(sample_area_steradians);
	}

	// Convert finite rotation (over 'time_interval') to a rotation rate vector (with magnitude in radians/myr).
	const GPlatesMaths::Vector3D rotation_rate_vector = convert_finite_rotation_to_rotation_rate_vector(stage_pole, time_interval);

	return create(point, sample_area_steradians, rotation_rate_vector);
}

GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::create(
		const GPlatesMaths::PointOnSphere &point,
		const double &sample_area_steradians,
		const GPlatesMaths::Vector3D &rotation_rate_vector)
{
	//
	// The net rotation is the integral of 'R x (W x R)' over a surface (eg, a plate, or the entire the sphere):
	//
	//         /
	//         |
	//   W = k | R x (W x R) dA
	//         |
	//        /
	//
	// ...where 'k' is inversely proportional to:
	//
	//         /
	//         |
	//         | dA
	//         |
	//        /
	//
	// These two integrals are approximated by summing the net rotation contributions at sample points
	// (multiplied by their sample areas), and separately summing the sample areas:
	//
	//     sum(R_i x (W_i x R_i) dA_i)
	//
	//     sum(dA_i)
	//
	// So, for the current sample point 'i', we store both:
	//   net_rotation_component = R_i x (W_i x R_i) dA_i
	//   sample_area_steradians = dA_i
	//
	// And later we sum these individual contributions (see 'add()').
	//
	const GPlatesMaths::Vector3D v = cross(rotation_rate_vector, point.position_vector());
	const GPlatesMaths::Vector3D omega = cross(point.position_vector(), v);
	const GPlatesMaths::Vector3D net_rotation_component = omega * sample_area_steradians;

	return NetRotationAccumulator(net_rotation_component, sample_area_steradians);
}

GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator &
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::operator+=(
		const NetRotationAccumulator &other)
{
	// Sum the area-weighted net rotation contribution.
	d_net_rotation_component = d_net_rotation_component + other.d_net_rotation_component;

	// Sum the area contribution.
	d_area_steradians = d_area_steradians + other.d_area_steradians;

	return *this;
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_finite_rotation() const
{
	// Extract finite rotation from rotation rate vector.
	return convert_rotation_rate_vector_to_finite_rotation(get_net_rotation_rate_vector(), 1.0/*time_interval*/);
}

GPlatesMaths::Vector3D
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_rotation_rate_vector() const
{
	if (GPlatesMaths::are_almost_exactly_equal(d_area_steradians, 0))
	{
		return GPlatesMaths::Vector3D();  // zero vector
	}

	//
	// The net rotation is the integral of 'R x (W x R)' over a surface (eg, a plate, or the entire the sphere):
	//
	//         /
	//         |
	//   W = k | R x (W x R) dA
	//         |
	//        /
	//
	//         /
	//         |
	//     = k | [W(R.R) - R(R.W)] dA
	//         |
	//        /
	//
	// ...where "R x (W x R) = [W(R.R) - R(R.W)]" using vector triple product expansion - https://en.wikipedia.org/wiki/Triple_product#Vector_triple_product
	//
	// We wish to calculate 'k' which is a normalization factor that ensures, for example, the hypothetical case of a single plate
	// covering the *entire* globe that is rotating about the z-axis, will return a total net rotation equal to that rotation.
	// In this example the rotation is 'W = <0, 0, Wz>' and we get a total net rotation using spherical coordinates:
	//
	//                  /2pi       /pi/2
	//                  |          |
	//          W  =  k | d(phi)   | [W(R.R) - R(R.W)] cos(theta) d(theta)
	//                  |          |
	//                 / 0        /-pi/2
	//
	//                  /2pi       /pi/2
	//                  |          |
	//   <0,0,Wz>  =  k | d(phi)   | [<0,0,Wz> - <x,y,z> Wz sin(theta)] cos(theta) d(theta)
	//                  |          |
	//                 / 0        /-pi/2
	//
	// ...where we used "R.R = 1" since dot product of a unit vector with itself is the scalar one, and "R.W = <x,y,z>.<0,0,Wz> = Wz z = Wz sin(theta)"
	//
	//                  /2pi       /pi/2
	//                  |          |
	//   <0,0,Wz>  =  k | d(phi)   | [<0,0,Wz> - <cos(phi) cos(theta), sin(phi) cos(theta), sin(theta)> Wz sin(theta)] cos(theta) d(theta)
	//                  |          |
	//                 / 0        /-pi/2
	//
	//
	// ...and noting that...
	//
	//    /2pi                 /2pi
	//    |                    |
	//    | d(phi) cos(phi) =  | d(phi) sin(phi) =  0
	//    |                    |
	//   / 0                  / 0
	//
	// ...we get...
	//
	//                  /2pi       /pi/2
	//                  |          |
	//   <0,0,Wz>  =  k | d(phi)   | [<0,0,Wz> - <0,0,sin(theta)> Wz sin(theta)] cos(theta) d(theta)
	//                  |          |
	//                 / 0        /-pi/2
	//
	// ...where only the z-component of vector on both sides is non-zero, which we write as...
	//
	//                  /2pi       /pi/2
	//                  |          |
	//         Wz  =  k | d(phi)   | [Wz - Wz sin(theta)^2] cos(theta) d(theta)
	//                  |          |
	//                 / 0        /-pi/2
	//
	//                             /pi/2
	//                             |
	//             =  k (2 pi) Wz  | [1 - sin(theta)^2] cos(theta) d(theta)
	//                             |
	//                            /-pi/2
	//
	//                             /pi/2
	//                             |
	//             =  k (2 pi) Wz  | cos(theta)^3 d(theta)
	//                             |
	//                            /-pi/2
	//
	// ...which results in the normalization factor 'k' being...
	//
	//                               1
	//         k =   ------------------------------------
	//                            /pi/2
	//                            |
	//                       2 pi | cos(theta)^3 d(theta)
	//                            |
	//                           /-pi/2
	//
	// ...hence the weighting factor (the denominator of 'k') is the integral of "cos(latitude)^3"
	// (when calculating net rotation over the *entire* globe).
	//
	// Also note that 'k' evaluates to (using "integral[cos(theta)^3] = sin(theta) - (1/3) sin(theta)^3"):
	//
	//         k =      1
	//             ----------
	//             (8 pi) / 3
	//
	// ...which is normalization factor '3 / (8 pi)' mentioned in the Torsvik 2010 paper:
	//   "Plate tectonics and net lithosphere rotation over the past 150 My".
	//
	// Note that if we had tried to match a rotation of '<Wx, 0, 0>' instead of '<0, 0, Wz>' then the integral
	// in the denominator of 'k' would have been over 'cos(theta) - 0.5 cos^3(theta)' instead of 'cos^3(theta)'.
	// But both integrals evaluate to the same result ('4/3'), so 'k' ends up the same.
	//

	//
	// The normalized net rotation is:
	//
	//        /
	//        |
	//   W =  | R x (W x R) dA
	//        |
	//       /
	//      -------------------
	//             /
	//             |
	//       (2/3) | dA
	//             |
	//            /
	//
	// ...where the surface integrals could be over an individual topology or over the entire globe
	// (depending on whether the net rotation is for an individual topology or for the *total* net rotation of the globe).
	//
	// Note that when integrating over the *entire* globe the denominator equates to the expected '(8 pi) / 3' (ie, denominator of 'k' above)
	// since 'integral(dA)' is '4 pi'.
	//
	// This is approximated by summing the net rotation contributions at sample points (multiplied by their sample areas) and normalizing:
	//
	//       sum(R_i x (W_i x R_i) dA_i)
	//   W ~ ---------------------------
	//             (2/3) sum(dA_i)
	//
	// In our case we have:
	//
	//   d_net_rotation_component = sum(R_i x (W_i x R_i) dA_i)
	//   d_area_steradians        = sum(dA_i)
	//
	// ...so we get:
	//
	//      d_net_rotation_component
	//  W ~ ------------------------
	//      (2/3) d_area_steradians
	//
	//
	// Note: This means the net rotation for a single plate (that doesn't cover the entire globe) will only be normalized by
	//       the area covered by that plate (not the entire surface of the globe).
	//
	// Note: Previously (in GPlates <= 2.4) the denominator was the following (instead of '(2/3) sum(dA_i)'):
	//               sum(cos^2(latitude_i) dA_i)     = sum(cos^2(latitude_i) cos(latitude_i) d_phi d_theta)
	//                                               = sum(cos^3(latitude_i) d_phi d_theta)
	//                                               ~ (8 pi) / 3  # over *entire* surface of globe
	//       ...in order to emulate the denominator of 'k' above.
	//       However that was derived using 'W = <0,0,Wz>'. If we had instead used 'W = <Wx,0,0>' then we would have gotten:
	//         sum((1 - 0.5 cos^2(latitude_i)) dA_i) = sum((1 - 0.5 cos^2(latitude_i)) cos(latitude_i) d_phi d_theta)
	//                                               = sum((cos(latitude_i) - 0.5 cos^3(latitude_i)) d_phi d_theta)
	//                                               ~ (8 pi) / 3  # over *entire* surface of globe
	//       While this gives the same result when integrating over the surface of the *entire* globe,
	//       it will give different results for individual plates (depending on their latitude).
	//       Now (in GPlates > 2.4) the normalization is proportional to the area only:
	//                     (2/3) sum(dA_i)           = (2/3) sum(cos(latitude_i) d_phi d_theta)
	//                                               = (2/3) (4 pi)  # over *entire* surface of globe
	//                                               ~ (8 pi) / 3    # over *entire* surface of globe
	//
	return (3 / (2 * d_area_steradians)) * d_net_rotation_component;
}

boost::optional<std::pair<GPlatesMaths::LatLonPoint, double>>
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_rotation_lat_lon_pole_and_angle() const
{
	// Get the net rotation as a finite rotation.
	const GPlatesMaths::FiniteRotation finite_rotation = get_net_finite_rotation();

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
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::convert_rotation_rate_vector_to_finite_rotation(
		const GPlatesMaths::Vector3D &rotation_rate_vector,
		const double &time_interval)
{
	if (rotation_rate_vector.is_zero_magnitude())
	{
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}

	const double rotation_rate_angle = rotation_rate_vector.magnitude().dval();

	const GPlatesMaths::PointOnSphere rotation_pole(
			GPlatesMaths::UnitVector3D((1.0 / rotation_rate_angle) * rotation_rate_vector));

	const double finite_rotation_angle = rotation_rate_angle * time_interval;
	return GPlatesMaths::FiniteRotation::create(rotation_pole, finite_rotation_angle);
}

GPlatesMaths::Vector3D
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::convert_finite_rotation_to_rotation_rate_vector(
		const GPlatesMaths::FiniteRotation &finite_rotation,
		const double &time_interval)
{
	const GPlatesMaths::UnitQuaternion3D &uq = finite_rotation.unit_quat();

	const GPlatesMaths::UnitQuaternion3D::RotationParams params = uq.get_rotation_params(finite_rotation.axis_hint());

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!GPlatesMaths::are_almost_exactly_equal(time_interval, 0),
			GPLATES_ASSERTION_SOURCE);

	// Convert angle from radians to radians/Myr, and scale the axis with it.
	return (params.angle / time_interval) * GPlatesMaths::Vector3D(params.axis);
}

GPlatesScribe::TranscribeResult
GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_net_rotation_component, "net_rotation_component"))
	{
		return scribe.get_transcribe_result();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_area_steradians, "area_steradians"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
GPlatesAppLogic::NetRotationUtils::operator+(
		const NetRotationAccumulator &net_rotation_accumulator1,
		const NetRotationAccumulator &net_rotation_accumulator2)
{
	NetRotationAccumulator net_rotation_accumulator(net_rotation_accumulator1);
	net_rotation_accumulator += net_rotation_accumulator2;

	return net_rotation_accumulator;
}


GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::NetRotationCalculator(
		const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
		const resolved_topological_network_seq_type &resolved_topological_networks,
		const double &time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		unsigned int num_samples_along_meridian,
		GPlatesModel::integer_plate_id_type anchor_plate_id) :
	d_resolved_topological_boundaries(resolved_topological_boundaries),
	d_resolved_topological_networks(resolved_topological_networks),
	d_time(time),
	d_velocity_delta_time(velocity_delta_time),
	d_velocity_delta_time_type(velocity_delta_time_type),
	d_velocity_time_period(VelocityDeltaTime::get_time_range(velocity_delta_time_type, time, velocity_delta_time)),
	d_point_distribution(num_samples_along_meridian),
	d_anchor_plate_id(anchor_plate_id)
{
	initialise_from_latitude_longitude_points(num_samples_along_meridian);
}

GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::NetRotationCalculator(
		const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
		const resolved_topological_network_seq_type &resolved_topological_networks,
		const double &time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const arbitrary_point_distribution_type &arbitrary_points,
		GPlatesModel::integer_plate_id_type anchor_plate_id) :
	d_resolved_topological_boundaries(resolved_topological_boundaries),
	d_resolved_topological_networks(resolved_topological_networks),
	d_time(time),
	d_velocity_delta_time(velocity_delta_time),
	d_velocity_delta_time_type(velocity_delta_time_type),
	d_velocity_time_period(VelocityDeltaTime::get_time_range(velocity_delta_time_type, time, velocity_delta_time)),
	d_point_distribution(arbitrary_points),
	d_anchor_plate_id(anchor_plate_id)
{
	initialise_from_arbitrary_points(arbitrary_points);
}


GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::NetRotationCalculator(
		const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
		const resolved_topological_network_seq_type &resolved_topological_networks,
		const double &time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const point_distribution_type &point_distribution,
		GPlatesModel::integer_plate_id_type anchor_plate_id) :
	d_resolved_topological_boundaries(resolved_topological_boundaries),
	d_resolved_topological_networks(resolved_topological_networks),
	d_time(time),
	d_velocity_delta_time(velocity_delta_time),
	d_velocity_delta_time_type(velocity_delta_time_type),
	d_velocity_time_period(VelocityDeltaTime::get_time_range(velocity_delta_time_type, time, velocity_delta_time)),
	d_point_distribution(point_distribution),
	d_anchor_plate_id(anchor_plate_id)
{
	if (const unsigned int *num_samples_along_meridian = boost::get<unsigned int>(&point_distribution))
	{
		initialise_from_latitude_longitude_points(*num_samples_along_meridian);
	}
	else
	{
		const arbitrary_point_distribution_type &arbitrary_points = boost::get<arbitrary_point_distribution_type>(point_distribution);

		initialise_from_arbitrary_points(arbitrary_points);
	}
}

void
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::initialise_from_latitude_longitude_points(
		unsigned int num_samples_along_meridian)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			num_samples_along_meridian > 0,
			GPLATES_ASSERTION_SOURCE);
	const unsigned int num_samples_along_parallel = 2 * num_samples_along_meridian;

	const double delta_in_degrees = 180.0 / num_samples_along_meridian;
	const double delta_in_radians = GPlatesMaths::convert_deg_to_rad(delta_in_degrees);

	// Loop over lat-lon grid and calculate the rotation contribution at each point.
	for (unsigned int latitude_index = 0; latitude_index < num_samples_along_meridian; ++latitude_index)
	{
		const double latitude = -90.0 + (latitude_index + 0.5) * delta_in_degrees;

		for (unsigned int longitude_index = 0; longitude_index < num_samples_along_parallel; ++longitude_index)
		{
			const double longitude = -180.0 + (longitude_index + 0.5) * delta_in_degrees;

			const GPlatesMaths::PointOnSphere position = GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(latitude, longitude));

			// Calculate sample area around current latitude/longitude points (based on the grid spacing and the point's position).
			const double sample_area_steradians = NetRotationAccumulator::calc_lat_lon_point_sample_area_steradians(position, delta_in_radians);

			// Add net rotation contribution from a deforming network first, otherwise from a rigid plate.
			if (!add_net_rotation_contribution_from_resolved_networks(position, sample_area_steradians))
			{
				add_net_rotation_contribution_from_resolved_boundaries(position, sample_area_steradians);
			}
		}
	}
}

void
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::initialise_from_arbitrary_points(
		const arbitrary_point_distribution_type &arbitrary_points)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			arbitrary_points.size() > 0,
			GPLATES_ASSERTION_SOURCE);

	// Loop over the arbitrary distribution of points and calculate the rotation contribution at each point.
	for (const auto &point_and_sample_area : arbitrary_points)
	{
		const GPlatesMaths::PointOnSphere position = point_and_sample_area.first;
		const double sample_area_steradians = point_and_sample_area.second;

		// Add net rotation contribution from a deforming network first, otherwise from a rigid plate.
		if (!add_net_rotation_contribution_from_resolved_networks(position, sample_area_steradians))
		{
			add_net_rotation_contribution_from_resolved_boundaries(position, sample_area_steradians);
		}
	}
}

bool
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution_from_resolved_networks(
		const GPlatesMaths::PointOnSphere &position,
		const double &sample_area_steradians)
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
							sample_area_steradians,
							point_stage_rotation->first,
							d_velocity_delta_time);

			add_net_rotation_contribution(network_ptr, net_rotation_contribution);

			return true;
		}
	}

	return false;
}

bool
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution_from_resolved_boundaries(
		const GPlatesMaths::PointOnSphere &position,
		const double &sample_area_steradians)
{
	// Check which rigid (non-deforming) plate (if any) the position lies in.
	for (auto boundary_ptr : d_resolved_topological_boundaries)
	{
		// Get the stage rotation from the plate ID.
		// If resolved boundary has no plate ID then the position does not contribute net rotation.
		const boost::optional<GPlatesMaths::FiniteRotation> boundary_stage_pole = get_resolved_boundary_stage_pole(boundary_ptr);
		if (boundary_stage_pole)
		{
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type boundary = boundary_ptr->resolved_topology_boundary();

			if (boundary->is_point_in_polygon(position,GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
			{
				const NetRotationAccumulator net_rotation_contribution =
						NetRotationAccumulator::create(
							position,
							sample_area_steradians,
							boundary_stage_pole.get(),
							d_velocity_delta_time);

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
	d_topological_network_net_rotation_map[resolved_topological_network] += net_rotation_contribution;
	d_plate_id_net_rotation_map[resolved_topological_network->plate_id()] += net_rotation_contribution;
	d_total_net_rotation += net_rotation_contribution;
}

void
GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::add_net_rotation_contribution(
		ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary,
		const NetRotationAccumulator &net_rotation_contribution)
{
	d_topological_boundary_net_rotation_map[resolved_topological_boundary] += net_rotation_contribution;
	d_plate_id_net_rotation_map[resolved_topological_boundary->plate_id()] += net_rotation_contribution;
	d_total_net_rotation += net_rotation_contribution;
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
