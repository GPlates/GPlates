/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#include <iostream>
#include <boost/none.hpp>

#include "GmlPoint.h"
#include "GpmlFiniteRotation.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create(
		const GPlatesMaths::FiniteRotation &finite_rotation)
{
	return non_null_ptr_type(
			new GpmlFiniteRotation(finite_rotation));
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
					GPlatesMaths::convert_deg_to_rad(gml_angle_in_degrees));

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
					GPlatesMaths::convert_deg_to_rad(gml_angle_in_degrees->quantity()));

	return create(fr);
}


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create_zero_rotation()
{
	using namespace ::GPlatesMaths;

	FiniteRotation fr = FiniteRotation::create_identity_rotation();

	return create(fr);
}


bool
GPlatesPropertyValues::GpmlFiniteRotation::is_zero_rotation() const
{
	return GPlatesMaths::represents_identity_rotation(d_finite_rotation.unit_quat());
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
	return GPlatesMaths::convert_rad_to_deg(rp.angle);
}


std::ostream &
GPlatesPropertyValues::GpmlFiniteRotation::print_to(
		std::ostream &os) const
{
	return os << d_finite_rotation;
}

