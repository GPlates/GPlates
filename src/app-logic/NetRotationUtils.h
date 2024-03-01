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
#ifndef GPLATES_APP_LOGIC_NETROTATIONUTILS_H
#define GPLATES_APP_LOGIC_NETROTATIONUTILS_H

#include <cmath>
#include <map>
#include <utility>  // for std::pair
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "VelocityDeltaTime.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"

#include "model/types.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{

	namespace NetRotationUtils
	{
		/**
		 * Used to store and accumulate net rotation point-by-point.
		 */
		class NetRotationAccumulator
		{
		public:

			/**
			 * Calculate the contribution to the plate net-rotation for the specified point and stage pole rotation over a time interval.
			 *
			 * @a stage_pole should be the stage rotation over the time interval @a time_interval, and @a time_interval should be in Ma.
			 *
			 * Also, you'll need to provide the sample area of the point (@a sample_area_steradians) based on how the points are distributed.
			 * If you have a distribution that is uniformly spaced in latitude-longitude space then use @a calc_lat_lon_point_sample_area_steradians.
			 * Alternatively, if you have a distribution that is uniformly spaced on the surface of the sphere then this will be the same for all points
			 * (ie, 4*pi steradians divided by the total number of points).
			 */
			static
			NetRotationAccumulator
			create(
					const GPlatesMaths::PointOnSphere &point,
					const GPlatesMaths::FiniteRotation &stage_pole,
					const double &time_interval,
					const double &sample_area_steradians);

			/**
			 * Calculate the contribution to the plate net-rotation for the specified point and rotation rate vector.
			 *
			 * @a rotation_rate_vector should have a magnitude in radians/myr.
			 *
			 * Also, you'll need to provide the sample area of the point (@a sample_area_steradians) based on how the points are distributed.
			 * If you have a distribution that is uniformly spaced in latitude-longitude space then use @a calc_lat_lon_point_sample_area_steradians.
			 * Alternatively, if you have a distribution that is uniformly spaced on the surface of the sphere then this will be the same for all points
			 * (ie, 4*pi steradians divided by the total number of points).
			 */
			static
			NetRotationAccumulator
			create(
					const GPlatesMaths::PointOnSphere &point,
					const GPlatesMaths::Vector3D &rotation_rate_vector,
					const double &sample_area_steradians);

			/**
			 * Calculate the sample area (on surface of globe) of the specified point on a uniform latitude/longitude grid.
			 *
			 * Since the specified point is expected to be on a uniformly distributed grid of points in latitude-longitude space,
			 * the sample area is 'cosine(theta) * grid_spacing * grid_spacing' where 'grid_spacing' is the latitude-longitude
			 * spacing between grid points (in radians) and 'theta' is the latitude of 'point'
			 * (the cosine is because points near the North/South poles are closer together).
			 */
			static
			double
			calc_lat_lon_point_sample_area_steradians(
					const GPlatesMaths::PointOnSphere &point,
					const double &lat_lon_grid_spacing_radians)
			{
				// Calculate the point's sample area based on the uniform lat/lon grid spacing.
				const double z = point.position_vector().z().dval();
				const double cos_latitude = std::sqrt(1 - z * z);

				return cos_latitude * lat_lon_grid_spacing_radians * lat_lon_grid_spacing_radians;
			}

			/**
			 * Convert a rotation rate vector (with magnitude in radians/myr) to a finite rotation over @a time interval.
			 */
			static
			GPlatesMaths::FiniteRotation
			convert_rotation_rate_vector_to_finite_rotation(
					const GPlatesMaths::Vector3D &rotation_vec,
					const double &time_interval);

			/**
			 * Convert a finite rotation over @a time interval to a rotation rate vector (with magnitude in radians/myr).
			 */
			static
			GPlatesMaths::Vector3D
			convert_finite_rotation_to_rotation_rate_vector(
					const GPlatesMaths::FiniteRotation &finite_rotation,
					const double &time_interval);

			/**
			 * Zero net rotation.
			 */
			NetRotationAccumulator() :
				d_net_rotation_component(),
				d_weighting_factor(0),
				d_area_steradians(0)
			{  }

			void
			add(
					const NetRotationAccumulator &net_rotation);

			/**
			 * Return the accumulated net rotation as a finite rotation (over a time interval of 1myr).
			 *
			 * Returns identity rotation if there have not been any non-zero net rotation contributions.
			 */
			GPlatesMaths::FiniteRotation
			get_net_finite_rotation() const;

			/**
			 * Return the accumulated net rotation as a rotation rate vector with a magnitude of radians/myr.
			 *
			 * Returns zero vector if there have not been any non-zero net rotation contributions.
			 */
			GPlatesMaths::Vector3D
			get_net_rotation_rate_vector() const;

			/**
			 * Return the accumulated net rotation as a lat-lon pole and angle (in degrees).
			 *
			 * Returns none if there have not been any non-zero net rotation contributions.
			 */
			boost::optional<std::pair<GPlatesMaths::LatLonPoint, double>>
			get_net_rotation_lat_lon_pole_and_angle() const;

			/**
			 * Return the accumulated area in steradians (square radians).
			 */
			double
			get_area_in_steradians() const
			{
				return d_area_steradians;
			}

		private:

			NetRotationAccumulator(
					const GPlatesMaths::Vector3D &net_rotation_component_,
					const double &weighting_factor_,
					const double &area_steradians_) :
				d_net_rotation_component(net_rotation_component_),
				d_weighting_factor(weighting_factor_),
				d_area_steradians(area_steradians_)
			{  }


			GPlatesMaths::Vector3D d_net_rotation_component;
			double d_weighting_factor;
			// Area of accumulated net rotation samples (in steradians, or square radians).
			double d_area_steradians;

		private: // Transcribe...

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);
		};



		/**
		 * Calculates net rotation from topological rigid plates and deforming networks.
		 */
		class NetRotationCalculator
		{
		public:

			/**
			 * An arbitrary distribution of points and their sample areas (in steradians).
			 *
			 * For example, if you have a distribution that is uniformly spaced on the surface of the sphere then each sample area
			 * will be the same (a constant) for all points (ie, 4*pi steradians divided by the total number of points).
			 * However, if the distribution is not quite uniform on the sphere then each sample area will be slightly different.
			 */
			typedef std::vector<std::pair<GPlatesMaths::PointOnSphere, double/*sample_area_steradians*/>> arbitrary_point_distribution_type;
			/**
			 * How the points, to calculate net rotation, are distributed across the globe.
			 *
			 * If a single integer then the points are uniformly distributed in latitude-longitude space and
			 * it is the number of grid points along each meridian. The same (longitude) spacing is used along parallels.
			 * The default is 180 x 360 uniform lat-lon samples.
			 *
			 * Otherwise it's an arbitrary distribution of points and their sample areas (in steradians).
			 */
			typedef boost::variant<
					// Number of uniform latitude-longitude points along each meridian...
					unsigned int,
					// An arbitrary distribution of points and their sample areas (in steradians)...
					arbitrary_point_distribution_type
			> point_distribution_type;

			//! Convenience typedef for sequence of resolved topological boundaries.
			typedef std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> resolved_topological_boundary_seq_type;

			//! Convenience typedef for sequence of resolved topological networks.
			typedef std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> resolved_topological_network_seq_type;


			// A map of rigid plates to net rotations.
			typedef std::map<ResolvedTopologicalBoundary::non_null_ptr_to_const_type, NetRotationAccumulator> topological_boundary_net_rotation_map_type;

			// A map of deforming networks to net rotations.
			typedef std::map<ResolvedTopologicalNetwork::non_null_ptr_to_const_type, NetRotationAccumulator> topological_network_net_rotation_map_type;

			// A map for storing net rotation per plate ID (with plate ID 'none' used for deforming networks that have no plate ID).
			typedef std::map<boost::optional<GPlatesModel::integer_plate_id_type>, NetRotationAccumulator> plate_id_net_rotation_map_type;


			static const unsigned int DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN = 180;


			/**
			 * Accumulate net rotation of the specified resolved topologies over a uniform grid of latitude-longitude points.
			 *
			 * @a num_samples_along_meridian is the number of grid points along each meridian.
			 * The same (longitude) spacing is used along parallels.
			 * The default is 180 x 360 uniform lat-lon samples.
			 */
			NetRotationCalculator(
					const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
					const resolved_topological_network_seq_type &resolved_topological_networks,
					const double &time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					unsigned int num_samples_along_meridian = DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN,
					GPlatesModel::integer_plate_id_type anchor_plate_id = 0);

			/**
			 * Accumulate net rotation of the specified resolved topologies over an arbitrary distribution of points.
			 *
			 * @a points is an arbitrary distribution of points and their sample areas (in steradians).
			 * For example, if you have a distribution that is uniformly spaced on the surface of the sphere each sample area
			 * will be the same (a constant) for all points (ie, 4*pi steradians divided by the total number of points).
			 * However, if the distribution is not quite uniform on the sphere then each sample area will be slightly different.
			 */
			NetRotationCalculator(
					const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
					const resolved_topological_network_seq_type &resolved_topological_networks,
					const double &time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					const arbitrary_point_distribution_type &arbitrary_points,
					GPlatesModel::integer_plate_id_type anchor_plate_id = 0);

			/**
			 * Combination of the other constructors where point distribution is explicitly specified using a @a point_distribution_type.
			 *
			 * @a point_distribution is the point distribution (specified as either a uniform lat-lon grid spacing or arbitrary points).
			 */
			NetRotationCalculator(
					const resolved_topological_boundary_seq_type &resolved_topological_boundaries,
					const resolved_topological_network_seq_type &resolved_topological_networks,
					const double &time,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type,
					const point_distribution_type &point_distribution,
					GPlatesModel::integer_plate_id_type anchor_plate_id = 0);

			/**
			 * Return the accumulated net rotation over all input resolved topologies.
			 */
			const NetRotationAccumulator &
			get_total_net_rotation() const
			{
				return d_total_net_rotation;
			}

			/**
			 * Return a mapping of rigid plates to their accumulated net rotation.
			 *
			 * Note: Topological boundaries (rigid plates) that don't have a plate ID are excluded altogether
			 *       because we cannot determine a stage rotation from them.
			 */
			const topological_boundary_net_rotation_map_type &
			get_topological_boundary_net_rotation_map() const
			{
				return d_topological_boundary_net_rotation_map;
			}

			/**
			 * Return a mapping of deforming networks to their accumulated net rotation.
			 */
			const topological_network_net_rotation_map_type &
			get_topological_network_net_rotation_map() const
			{
				return d_topological_network_net_rotation_map;
			}

			/**
			 * Return a mapping of plate IDs to their accumulated net rotation.
			 *
			 * Note: Networks are no longer required to have a plate ID because it doesn't make sense
			 *       (network is deforming, not rigidly rotated by plate ID). If a deforming network
			 *       doesn't have a plate ID then it will be grouped under plate ID 'none'.
			 *       Note that topological boundaries (rigid plates) that don't have a plate ID are excluded altogether.
			 */
			const plate_id_net_rotation_map_type &
			get_plate_id_net_rotation_map() const
			{
				return d_plate_id_net_rotation_map;
			}

			double
			get_time() const
			{
				return d_time;
			}

			double
			get_velocity_delta_time() const
			{
				return d_velocity_delta_time;
			}

			VelocityDeltaTime::Type
			get_velocity_delta_time_type() const
			{
				return d_velocity_delta_time_type;
			}

			const point_distribution_type &
			get_point_distribution() const
			{
				return d_point_distribution;
			}

			GPlatesModel::integer_plate_id_type
			get_anchor_plate_id() const
			{
				return d_anchor_plate_id;
			}

		private:

			// A map for storing stage poles (relative to anchor) per plate id.
			typedef std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> stage_pole_map_type;

			void
			initialise_from_latitude_longitude_points(
					unsigned int num_samples_along_meridian);

			void
			initialise_from_arbitrary_points(
					const arbitrary_point_distribution_type &arbitrary_points);

			bool
			add_net_rotation_contribution_from_resolved_networks(
					const GPlatesMaths::PointOnSphere &position,
					const double &sample_area_steradians);

			bool
			add_net_rotation_contribution_from_resolved_boundaries(
					const GPlatesMaths::PointOnSphere &position,
					const double &sample_area_steradians);

			void
			add_net_rotation_contribution(
					ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network,
					const NetRotationAccumulator &net_rotation_result);

			void
			add_net_rotation_contribution(
					ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary,
					const NetRotationAccumulator &net_rotation_result);

			boost::optional<GPlatesMaths::FiniteRotation>
			get_resolved_boundary_stage_pole(
					ResolvedTopologicalBoundary::non_null_ptr_to_const_type resolved_topological_boundary) const;


			resolved_topological_boundary_seq_type d_resolved_topological_boundaries;
			resolved_topological_network_seq_type d_resolved_topological_networks;

			double d_time;
			double d_velocity_delta_time;
			VelocityDeltaTime::Type d_velocity_delta_time_type;
			std::pair<double/*older*/, double/*younger*/> d_velocity_time_period;
			//! How the points, to calculate net rotation, are distributed across the globe.
			point_distribution_type d_point_distribution;
			GPlatesModel::integer_plate_id_type d_anchor_plate_id;

			topological_boundary_net_rotation_map_type d_topological_boundary_net_rotation_map;
			topological_network_net_rotation_map_type d_topological_network_net_rotation_map;

			plate_id_net_rotation_map_type d_plate_id_net_rotation_map;
			NetRotationAccumulator d_total_net_rotation;

			mutable stage_pole_map_type d_resolved_boundary_stage_pole_map;
		};
	}
}
#endif // GPLATES_APP_LOGIC_NETROTATIONUTILS_H
