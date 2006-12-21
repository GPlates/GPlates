/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include "GpmlFiniteRotation.h"
#include "GmlPoint.h"
#include "maths/LatLonPointConversions.h"
#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"


const boost::intrusive_ptr<GPlatesModel::GpmlFiniteRotation>
GPlatesModel::GpmlFiniteRotation::create(
		const std::pair<double, double> &gpml_euler_pole,
		const double &gml_angle_in_degrees)
{
	using namespace ::GPlatesMaths;

	const double &lon = gpml_euler_pole.first;
	const double &lat = gpml_euler_pole.second;

	// FIXME:  Check the validity of the lat/lon coords using functions in LatLonPoint.
	LatLonPoint llp(lat, lon);
	PointOnSphere p = LatLonPointConversions::convertLatLonPointToPointOnSphere(llp);
	FiniteRotation fr = FiniteRotation::create(p, degreesToRadians(gml_angle_in_degrees));

	boost::intrusive_ptr<GpmlFiniteRotation> finite_rotation_ptr(new GpmlFiniteRotation(fr));
	return finite_rotation_ptr;
}


const boost::intrusive_ptr<GPlatesModel::GpmlFiniteRotation>
GPlatesModel::GpmlFiniteRotation::create_zero_rotation()
{
	using namespace ::GPlatesMaths;

	FiniteRotation fr = FiniteRotation::create(UnitQuaternion3D::create_identity_rotation());

	boost::intrusive_ptr<GpmlFiniteRotation> finite_rotation_ptr(new GpmlFiniteRotation(fr));
	return finite_rotation_ptr;
}


bool
GPlatesModel::GpmlFiniteRotation::is_zero_rotation() const
{
	return ::GPlatesMaths::represents_identity_rotation(d_finite_rotation.unit_quat());
}


const boost::intrusive_ptr<GPlatesModel::GmlPoint>
GPlatesModel::calculate_euler_pole(
		const GpmlFiniteRotation &fr)
{
	// FIXME:  This code should probably move into the GPlatesMaths namespace somewhere.
	using namespace ::GPlatesMaths;

	// If 'fr' is a zero rotation, this will throw an exception.
	UnitQuaternion3D::RotationParams rp = fr.finite_rotation().unit_quat().get_rotation_params();
	boost::intrusive_ptr<GmlPoint> euler_pole_ptr = GmlPoint::create(PointOnSphere(rp.axis));
	return euler_pole_ptr;
}


const GPlatesMaths::real_t
GPlatesModel::calculate_angle(
		const GpmlFiniteRotation &fr)
{
	// FIXME:  This code should probably move into the GPlatesMaths namespace somewhere.
	using namespace ::GPlatesMaths;

	// If 'fr' is a zero rotation, this will throw an exception.
	UnitQuaternion3D::RotationParams rp = fr.finite_rotation().unit_quat().get_rotation_params();
	return GPlatesMaths::radiansToDegrees(rp.angle);
}
