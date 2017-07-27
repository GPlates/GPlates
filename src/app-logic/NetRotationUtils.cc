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

#include "NetRotationUtils.h"
#include "ReconstructionTree.h"
#include "RotationUtils.h"

namespace
{



	void
	display_net_rotation_output_(
			const GPlatesAppLogic::NetRotationUtils::net_rotation_map_type &results,
			const double &time,
			bool also_by_plate = true)
	{
		qDebug() << "Time: " << time;
		if (also_by_plate)
		{
			qDebug() << "\tBegin print out of net rotations per plate id";
			qDebug() << "\tNet rotation per plate id: ";
		}

		try
		{

			GPlatesAppLogic::NetRotationUtils::net_rotation_map_type::const_iterator it = results.begin();

			GPlatesMaths::Vector3D total_weighted;
			GPlatesMaths::Vector3D total_unweighted;
			double total_weight = 0.;
			for (; it != results.end() ; ++it)
			{
				GPlatesAppLogic::NetRotationUtils::NetRotationResult result = it->second;
				if (it->first == 0)
				{
					continue;
				}

				if (GPlatesMaths::are_almost_exactly_equal(result.d_weighting_factor,0.)) return;
				GPlatesMaths::Vector3D omega(result.d_rotation_component.x()/result.d_weighting_factor,
											 result.d_rotation_component.y()/result.d_weighting_factor,
											 result.d_rotation_component.z()/result.d_weighting_factor);

				total_weighted = total_weighted + omega;
				total_unweighted = total_unweighted + result.d_rotation_component;
				total_weight += result.d_weighting_factor;


				double mag = omega.magnitude().dval();

				if (also_by_plate)
				{
					qDebug() << "\t\tPlate id: " << it->first;
					qDebug() << "\t\tOmega pre weight (xyz): " << result.d_rotation_component.x().dval() << result.d_rotation_component.y().dval() << result.d_rotation_component.z().dval();
					qDebug() << "\t\tOmega post weight (xyz): " << omega.x().dval() << omega.y().dval() << omega.z().dval();
					if (!GPlatesMaths::are_almost_exactly_equal(mag,0))
					{
						std::pair<GPlatesMaths::LatLonPoint, double> pole =
								GPlatesAppLogic::NetRotationUtils::convert_net_rotation_xyz_to_pole(omega);
						qDebug() << "\t\tOmega (pole): " << pole.first.latitude() << pole.first.longitude() << pole.second;
					}

					qDebug() << "\t\tWeighting factor " << result.d_weighting_factor;
					qDebug();
				}
			}

			if (GPlatesMaths::are_almost_exactly_equal(total_weight,0.))
			{
				qDebug() << "Zero total weights";
				return;
			}
			GPlatesMaths::Vector3D total(total_unweighted.x()/total_weight,
										 total_unweighted.y()/total_weight,
										 total_unweighted.z()/total_weight);

			double mag = total.magnitude().dval();
			qDebug() << "\t\tTotal Omega (xyz): " << total.x().dval() << total.y().dval() << total.z().dval();
			if (!GPlatesMaths::are_almost_exactly_equal(mag,0))
			{
				std::pair<GPlatesMaths::LatLonPoint, double> pole =
						GPlatesAppLogic::NetRotationUtils::convert_net_rotation_xyz_to_pole(total);
				qDebug() << "\t\t Total Omega (pole): " << pole.first.latitude() << pole.first.longitude() << pole.second;
				qDebug() << "FLAG " << "\tTime: " << time << "\t Total Omega (pole): " << pole.first.latitude() << pole.first.longitude() << pole.second;
			}
		}
		catch(std::exception &e)
		{
			qDebug() << "Caught exception: " << e.what();
		}
	}
}

void
GPlatesAppLogic::NetRotationUtils::sum_net_rotations(
		const NetRotationUtils::net_rotation_map_type::value_type &net_rotation,
		NetRotationUtils::net_rotation_map_type &net_rotations)
{
	GPlatesModel::integer_plate_id_type id = net_rotation.first;
	NetRotationUtils::NetRotationResult result = net_rotation.second;

	NetRotationUtils::net_rotation_map_type::iterator it =
			net_rotations.find(id);

	// If we don't have a map entry for our plate-id, make a new one.
	if (it == net_rotations.end())
	{
		net_rotations.insert(NetRotationUtils::net_rotation_map_type::value_type(id,result));
	}
	else
	{
		// Add the current rotation data to the running total for the plate-id
		NetRotationUtils::NetRotationResult old_result = it->second;

		old_result.d_rotation_component = old_result.d_rotation_component + result.d_rotation_component;
		old_result.d_weighting_factor += result.d_weighting_factor;
		old_result.d_plate_area_component += result.d_plate_area_component;
		it->second = old_result;
	}
}

void
GPlatesAppLogic::NetRotationUtils::display_net_rotation_output(
		const GPlatesAppLogic::NetRotationUtils::net_rotation_map_type &results,
		const double &time,
		bool also_by_plate)
{
	display_net_rotation_output_(results,time,also_by_plate);
}

std::pair<GPlatesMaths::LatLonPoint, double>
GPlatesAppLogic::NetRotationUtils::convert_net_rotation_xyz_to_pole(
		const GPlatesMaths::Vector3D v)
{
	using namespace GPlatesMaths;
	double mag = v.magnitude().dval();

	UnitVector3D uv = v.get_normalisation();

	LatLonPoint llp = make_lat_lon_point(PointOnSphere(uv));

	return std::make_pair(llp,mag);
}


GPlatesMaths::Vector3D
GPlatesAppLogic::NetRotationUtils::convert_net_rotation_pole_to_xyz(
		const GPlatesMaths::LatLonPoint &llp, const double &angle)
{
	// Angle should already be in form degrees per million years
	using namespace GPlatesMaths;
	PointOnSphere p = make_point_on_sphere(llp);
	UnitVector3D uv = p.position_vector();
	Vector3D v = angle*Vector3D(uv);
	return v;
}

GPlatesAppLogic::NetRotationUtils::NetRotationResult
GPlatesAppLogic::NetRotationUtils::calc_net_rotation_contribution(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesMaths::FiniteRotation &stage_pole,
		double time_interval)
{
	using namespace GPlatesMaths;

	if (represents_identity_rotation(stage_pole.unit_quat()))
	{
		return NetRotationResult(Vector3D(),0.,0.,0.);
	}

	if (are_almost_exactly_equal(time_interval,0.))
	{
		return NetRotationResult(Vector3D(),0.,0.,0.);
	}

	const UnitQuaternion3D uq = stage_pole.unit_quat();

	const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = stage_pole.axis_hint();
	UnitQuaternion3D::RotationParams params = uq.get_rotation_params(axis_hint);

	// Angle from quaternion is radians
	// Convert angle to degrees per Ma.
	double angle = convert_rad_to_deg(params.angle).dval()/time_interval;

	LatLonPoint llp = make_lat_lon_point(PointOnSphere(params.axis));

	Vector3D stage_pole_xyz = convert_net_rotation_pole_to_xyz(llp,angle);

	Vector3D v = cross(stage_pole_xyz,point.position_vector());

	Vector3D omega = cross(point.position_vector(),v);

	double cos_latitude = std::cos(std::asin(point.position_vector().z().dval()));
	omega = omega*cos_latitude;
	double x = point.position_vector().x().dval();
	double y = point.position_vector().y().dval();
	double weighting_factor = (x*x + y*y)*cos_latitude;

#if 0
	double test = std::pow((x*x + y*y),1.5);
	qDebug() << "x,y,cos_lat: " << x << y << cos_latitude;
	qDebug() << "weighting factor: " << weighting_factor;
	qDebug() << "test: " << test;
#endif
	return NetRotationResult(omega,weighting_factor,cos_latitude,0.);
}
