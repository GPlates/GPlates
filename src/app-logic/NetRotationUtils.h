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

#include <map>
#include <utility>  // for std::pair
#include <vector>
#include <boost/optional.hpp>

#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "VelocityDeltaTime.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"

#include "model/types.h"


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
			 * Calculate the contribution to the plate net-rotation for the specified point.
			 */
			static
			NetRotationAccumulator
			create(
					const GPlatesMaths::PointOnSphere &point,
					const GPlatesMaths::FiniteRotation &stage_pole,
					const double &time_interval,
					const double &sample_square_length_in_radians);

			/**
			 * Zero net rotation.
			 */
			NetRotationAccumulator():
				d_weighting_factor(0),
				d_area_steradians(0)
			{  }

			/**
			 * Add a net rotation contribution at a point.
			 *
			 * @a sample_square_length_in_radians is area of the sample square surrounding the sample point (in radians).
			 */
			void
			add(
					const GPlatesMaths::PointOnSphere &point,
					const GPlatesMaths::FiniteRotation &stage_pole,
					double time_interval,
					const double &sample_square_length_in_radians)
			{
				add(NetRotationAccumulator::create(point, stage_pole, time_interval, sample_square_length_in_radians));
			}

			void
			add(
					const NetRotationAccumulator &net_rotation);

			/**
			 * Return the accumulated net rotation as a finite rotation.
			 *
			 * Returns identity rotation if there have not been any non-zero net rotation contributions.
			 */
			GPlatesMaths::FiniteRotation
			get_net_rotation() const;

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

			static
			GPlatesMaths::FiniteRotation
			convert_rotation_vector_to_finite_rotation(
					const GPlatesMaths::Vector3D &rotation_vec);

			static
			GPlatesMaths::Vector3D
			convert_finite_rotation_to_rotation_vector(
					const GPlatesMaths::FiniteRotation &finite_rotation);


			NetRotationAccumulator(
					const GPlatesMaths::Vector3D &rotation_component_,
					const double &weighting_factor_,
					const double &area_steradians_) :
				d_rotation_component(rotation_component_),
				d_weighting_factor(weighting_factor_),
				d_area_steradians(area_steradians_)
			{  }


			GPlatesMaths::Vector3D d_rotation_component;
			double d_weighting_factor;
			// Area of accumulated net rotation samples (in steradians, or square radians).
			double d_area_steradians;
		};



		/**
		 * Calculates net rotation from topological rigid plates and deforming networks.
		 */
		class NetRotationCalculator
		{
		public:

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
			 * Accumulate net rotation of the specified resolved topologies over a uniform grid of lat-lon points.
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
					GPlatesModel::integer_plate_id_type anchor_plate_id = 0,
					unsigned int num_samples_along_meridian = DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN);

			/**
			 * Return the accumulated net rotation over all input resolved topologies.
			 */
			NetRotationAccumulator
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

			GPlatesModel::integer_plate_id_type
			get_anchor_plate_id() const
			{
				return d_anchor_plate_id;
			}

			unsigned int
			get_num_samples_along_meridian() const
			{
				return d_num_samples_along_meridian;
			}

		private:

			// A map for storing stage poles (relative to anchor) per plate id.
			typedef std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> stage_pole_map_type;


			bool
			add_net_rotation_contribution_from_resolved_networks(
					const GPlatesMaths::PointOnSphere &position,
					const double &sample_square_length_in_radians);

			bool
			add_net_rotation_contribution_from_resolved_boundaries(
					const GPlatesMaths::PointOnSphere &position,
					const double &sample_square_length_in_radians);

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
			GPlatesModel::integer_plate_id_type d_anchor_plate_id;
			unsigned int d_num_samples_along_meridian;

			topological_boundary_net_rotation_map_type d_topological_boundary_net_rotation_map;
			topological_network_net_rotation_map_type d_topological_network_net_rotation_map;

			plate_id_net_rotation_map_type d_plate_id_net_rotation_map;
			NetRotationAccumulator d_total_net_rotation;

			mutable stage_pole_map_type d_resolved_boundary_stage_pole_map;
		};
	}
}
#endif // GPLATES_APP_LOGIC_NETROTATIONUTILS_H
