/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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
#include <iterator>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "PyGeometriesOnSphere.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/VelocityDeltaTime.h"
#include "app-logic/VelocityUnits.h"

#include "global/python.h"

#include "maths/FiniteRotation.h"
#include "maths/UnitQuaternion3D.h"

#include "utils/Earth.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	bp::list
	calculate_velocities_using_finite_rotation(
			PointSequenceFunctionArgument domain_points_function_argument,
			const GPlatesMaths::FiniteRotation &finite_rotation,
			const double &time_inverval_in_my,
			GPlatesAppLogic::VelocityUnits::Value velocity_units,
			const double &earth_radius_in_kms)
	{
		// Get the sequence of points.
 		const std::vector<GPlatesMaths::PointOnSphere> &domain_points =
				domain_points_function_argument.get_points();

		// If identity rotation then return a list of zero vectors.
		if (GPlatesMaths::represents_identity_rotation(finite_rotation.unit_quat()))
		{
			bp::list zero_velocities;
			for (unsigned int n = 0; n < domain_points.size(); ++n)
			{
				zero_velocities.append(GPlatesMaths::Vector3D());
			}
			return zero_velocities;
		}

		bp::list velocities;

		// The axis hint doesn't affect the result (reversed axis and angle give same result).
		const GPlatesMaths::UnitQuaternion3D::RotationParams rotation_params =
				finite_rotation.unit_quat().get_rotation_params(boost::none);

		for (unsigned int n = 0; n < domain_points.size(); ++n)
		{
			const GPlatesMaths::PointOnSphere &domain_point =  domain_points[n];

			GPlatesMaths::Vector3D velocity =
					earth_radius_in_kms *
						(rotation_params.angle / time_inverval_in_my) *
							cross(rotation_params.axis, domain_point.position_vector());

			// Units are currently in kms/my so change if need cms/yr.
			if (velocity_units == GPlatesAppLogic::VelocityUnits::CMS_PER_YR)
			{
				// kms/my -> cm/yr...
				velocity = 1e-1 * velocity;
			}

			velocities.append(velocity);
		}

		return velocities;
	}
}


void
export_calculate_velocities()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesAppLogic::VelocityUnits::Value>("VelocityUnits")
			.value("kms_per_my", GPlatesAppLogic::VelocityUnits::KMS_PER_MY)
			.value("cms_per_yr", GPlatesAppLogic::VelocityUnits::CMS_PER_YR);

	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesAppLogic::VelocityDeltaTime::Type>("VelocityDeltaTimeType")
			.value("t_plus_delta_t_to_t", GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T)
			.value("t_to_t_minus_delta_t", GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T)
			.value("t_plus_minus_half_delta_t", GPlatesAppLogic::VelocityDeltaTime::T_PLUS_MINUS_HALF_DELTA_T);


	bp::def("calculate_velocities",
			&GPlatesApi::calculate_velocities_using_finite_rotation,
			(bp::arg("domain_points"),
					bp::arg("finite_rotation"),
					bp::arg("time_interval_in_my"),
					bp::arg("velocity_units") = GPlatesAppLogic::VelocityUnits::KMS_PER_MY,
					bp::arg("earth_radius_in_kms") = GPlatesUtils::Earth::MEAN_RADIUS_KMS),
			"calculate_velocities(domain_points, finite_rotation, time_interval_in_my, "
			"[velocity_units=pygplates.VelocityUnits.kms_per_my], "
			"[earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms])\n"
			"  Calculate velocities at a sequence of points assuming movement due to a finite rotation "
			"over a time interval.\n"
			"\n"
			"  :param domain_points: sequence of points at which to calculate velocities\n"
			"  :type domain_points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
			"  :param finite_rotation: the rotation pole and angle\n"
			"  :type finite_rotation: FiniteRotation\n"
			"  :param time_interval_in_my: the time interval (in millions of years) that the rotation angle encompasses\n"
			"  :type time_interval_in_my: float\n"
			"  :param velocity_units: whether to return velocities as *kilometres per million years* or "
			"*centimetres per year* (defaults to *kilometres per million years*)\n"
			"  :type velocity_units: VelocityUnits.kms_per_my or VelocityUnits.cms_per_yr\n"
			"  :param earth_radius_in_kms: the radius of the Earth in *kilometres* "
			"(defaults to ``pygplates.Earth.mean_radius_in_kms``)\n"
			"  :type earth_radius_in_kms: float\n"
			"  :rtype: list of Vector3D\n"
			"\n"
			"  Calculating velocities (in cms/yr) of all points in a :class:`ReconstructedFeatureGeometry` "
			"(generated by :class:`ReconstructModel`, :class:`ReconstructSnapshot` or :func:`reconstruct`):\n"
			"  ::\n"
			"\n"
			"    rotation_model = pygplates.RotationModel(...)\n"
			"    \n"
			"    # Get the rotation from 11Ma to 10Ma, and the feature's reconstruction plate ID.\n"
			"    equivalent_stage_rotation = rotation_model.get_rotation(\n"
			"        10, reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id(), 11)\n"
			"    \n"
			"    # Get the reconstructed geometry points.\n"
			"    reconstructed_points = reconstructed_feature_geometry.get_reconstructed_geometry().get_points()\n"
			"    \n"
			"    # Calculate a velocity for each reconstructed point over the 1My time interval.\n"
			"    velocities = pygplates.calculate_velocities(\n"
			"        reconstructed_points,\n"
			"        equivalent_stage_rotation,\n"
			"        1,\n"
			"        pygplates.VelocityUnits.cms_per_yr)\n"
			"\n"
			"  .. note:: | Velocities can be converted from global cartesian vectors to local "
			"``(magnitude, azimuth, inclination)`` coordinates using "
			":meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.\n"
			"            | See the :ref:`pygplates_calculate_velocities_by_plate_id` sample code.\n");
}
