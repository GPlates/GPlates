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

#include "app-logic/AppLogicFwd.h"
#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"
#include "model/types.h"

namespace GPlatesAppLogic
{

	namespace NetRotationUtils
	{
		/**
		 * @brief The NetRotationResult struct - used for storing intermediate results
		 * during point-by-point net-rotation calculations. Each point used in the net-rotation
		 * calculation has its results stored here, which are later summed.
		 */
		struct NetRotationResult
		{
			NetRotationResult(
					const GPlatesMaths::Vector3D &rotation_component,
					const double &weighting_factor,
					const double &plate_area_component,
					const double &plate_angular_velocity):
				d_rotation_component(rotation_component),
				d_weighting_factor(weighting_factor),
				d_plate_area_component(plate_area_component),
				d_plate_angular_velocity(plate_angular_velocity){}

			NetRotationResult():
				d_rotation_component(GPlatesMaths::Vector3D()),
				d_weighting_factor(0.),
				d_plate_area_component(0.),
				d_plate_angular_velocity(0.){}

			GPlatesMaths::Vector3D d_rotation_component;
			double d_weighting_factor;
			double d_plate_area_component;
			double d_plate_angular_velocity;
		};


		typedef std::map<GPlatesModel::integer_plate_id_type, NetRotationResult> net_rotation_map_type;

		/**
		 * @brief calc_net_rotation_components - calculate the contribution to the plate net-rotation for the
		 * point @a point
		 * @param point
		 * @param stage_pole  - stage pole for the plate-id of the polygon which the point belongs to
		 * @param time_interval - for correcting to degrees/Ma
		 * @return - Vector3D: the cartesian form of the rotation for the point
		 *			- double: the weighting factor for the point
		 */
		NetRotationResult
		calc_net_rotation_contribution(
				const GPlatesMaths::PointOnSphere &point,
				const GPlatesMaths::FiniteRotation &stage_pole,
				double time_interval);

		/**
		 * @brief sum_net_rotations - keeps a running total of net-rotation per plate-id.
		 * @param net_rotation - the net-rotation component and plate-id for a point
		 * @param net_rotations - the summed net-rotations per plate-id
		 */
		void
		sum_net_rotations(
				const NetRotationUtils::net_rotation_map_type::value_type &net_rotation,
				NetRotationUtils::net_rotation_map_type &net_rotations);

		/**
		 * @brief display_net_rotation_output - for debug output
		 * @param results
		 * @param time
		 * @param also_by_plate
		 */
		void
		display_net_rotation_output(
				const GPlatesAppLogic::NetRotationUtils::net_rotation_map_type &results,
				const double &time,
				bool also_by_plate = true);

		std::pair<GPlatesMaths::LatLonPoint, double>
		convert_net_rotation_xyz_to_pole(
				const GPlatesMaths::Vector3D v);


		GPlatesMaths::Vector3D
		convert_net_rotation_pole_to_xyz(
				const GPlatesMaths::LatLonPoint &llp, const double &angle);

	}
}
#endif // GPLATES_APP_LOGIC_NETROTATIONUTILS_H
