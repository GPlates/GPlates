/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <boost/none.hpp>

#include "GpmlFiniteRotation.h"
#include "GmlPoint.h"
#include "maths/LatLonPointConversions.h"
#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create(
		const GPlatesMaths::FiniteRotation &finite_rotation)
{
	non_null_ptr_type gpml_finite_rotation(
			new GpmlFiniteRotation(finite_rotation),
			GPlatesUtils::NullIntrusivePointerHandler());

	return gpml_finite_rotation;
}


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create(
		const std::pair<double, double> &gpml_euler_pole,
		const double &gml_angle_in_degrees)
{
	const double &lon = gpml_euler_pole.first;
	const double &lat = gpml_euler_pole.second;

	// FIXME:  Check the validity of the lat/lon coords using functions in LatLonPoint.
	GPlatesMaths::LatLonPoint llp(lat, lon);
	GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
	GPlatesMaths::FiniteRotation fr =
			GPlatesMaths::FiniteRotation::create(
					p,
					GPlatesMaths::degreesToRadians(gml_angle_in_degrees));

	return create(fr);
}


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create(
		const GmlPoint::non_null_ptr_type &gpml_euler_pole,
		const GpmlMeasure::non_null_ptr_type &gml_angle_in_degrees)
{
	GPlatesMaths::FiniteRotation fr =
			GPlatesMaths::FiniteRotation::create(
					*gpml_euler_pole->point(),
					GPlatesMaths::degreesToRadians(gml_angle_in_degrees->quantity()));

	return create(fr);
}


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create_zero_rotation()
{
	using namespace ::GPlatesMaths;

	FiniteRotation fr = FiniteRotation::create(
			UnitQuaternion3D::create_identity_rotation(),
			boost::none);

	non_null_ptr_type finite_rotation_ptr(new GpmlFiniteRotation(fr),
			GPlatesUtils::NullIntrusivePointerHandler());
	return finite_rotation_ptr;
}


bool
GPlatesPropertyValues::GpmlFiniteRotation::is_zero_rotation() const
{
	return ::GPlatesMaths::represents_identity_rotation(d_finite_rotation.unit_quat());
}


const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesPropertyValues::calculate_euler_pole(
		const GpmlFiniteRotation &fr)
{
	// FIXME:  This code should probably move into the GPlatesMaths namespace somewhere.
	using namespace ::GPlatesMaths;

	// If 'fr' is a zero rotation, this will throw an exception.
	UnitQuaternion3D::RotationParams rp =
			fr.finite_rotation().unit_quat().get_rotation_params(
					fr.finite_rotation().axis_hint());
	return GmlPoint::create(PointOnSphere(rp.axis));
}


const GPlatesMaths::real_t
GPlatesPropertyValues::calculate_angle(
		const GpmlFiniteRotation &fr)
{
	// FIXME:  This code should probably move into the GPlatesMaths namespace somewhere.
	using namespace ::GPlatesMaths;

	// If 'fr' is a zero rotation, this will throw an exception.
	UnitQuaternion3D::RotationParams rp =
			fr.finite_rotation().unit_quat().get_rotation_params(
					fr.finite_rotation().axis_hint());
	return GPlatesMaths::radiansToDegrees(rp.angle);
}
