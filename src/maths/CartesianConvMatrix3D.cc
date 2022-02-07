/* $Id: CartesianConvMatrix3D.cc,v 1.1.4.2 2006/03/14 00:29:01 mturner Exp $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author: mturner $
 *   $Date: 2006/03/14 00:29:01 $
 * 
 * Copyright (C) 2003, 2005, 2006 The University of Sydney, Australia.
 *
 * --------------------------------------------------------------------
 *
 * This file is part of GPlates.  GPlates is free software; you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License, version 2, as published by the Free Software
 * Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to 
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 */


#include "CartesianConvMatrix3D.h"

#include "LatLonPoint.h"
#include "MathsUtils.h"


GPlatesMaths::CartesianConvMatrix3D::CartesianConvMatrix3D(
		const PointOnSphere &pos)
{
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

	const real_t lam = convert_deg_to_rad(llp.latitude());
	const real_t phi = convert_deg_to_rad(llp.longitude());

	const real_t sin_lam = sin(lam);
	const real_t cos_lam = cos(lam);

	const real_t sin_phi = sin(phi);
	const real_t cos_phi = cos(phi);

	d_north = Vector3D(
			-sin_lam * cos_phi,
			-sin_lam * sin_phi,
			cos_lam);
	d_east = Vector3D(
			-sin_phi,
			cos_phi,
			0);
	d_down = Vector3D(
			-cos_lam * cos_phi,
			-cos_lam * sin_phi,
			-sin_lam);
}


GPlatesMaths::Vector3D
GPlatesMaths::convert_from_geocentric_to_north_east_down(
		const CartesianConvMatrix3D &ccm,
		const Vector3D &geocentric_vec)
{
	return Vector3D(
			dot(ccm.north(), geocentric_vec),
			dot(ccm.east(), geocentric_vec),
			dot(ccm.down(), geocentric_vec));
}



GPlatesMaths::Vector3D
GPlatesMaths::convert_from_north_east_down_to_geocentric(
		const CartesianConvMatrix3D &ccm,
		const Vector3D &north_east_down_vec)
{
	//
	// The 3x3 matrix 'ccm' is purely a rotation, so its inverse is equal to its transpose.
	//

	return
			north_east_down_vec.x() * ccm.north() +
			north_east_down_vec.y() * ccm.east() +
			north_east_down_vec.z() * ccm.down();
}


boost::tuple<
		GPlatesMaths::real_t/*magnitude*/,
		GPlatesMaths::real_t/*azimuth*/,
		GPlatesMaths::real_t/*inclination*/>
GPlatesMaths::convert_from_geocentric_to_magnitude_azimuth_inclination(
		const CartesianConvMatrix3D &ccm,
		const Vector3D &geocentric_vec)
{
	return convert_from_north_east_down_to_magnitude_azimuth_inclination(
			convert_from_geocentric_to_north_east_down(ccm, geocentric_vec));
}


GPlatesMaths::Vector3D
GPlatesMaths::convert_from_magnitude_azimuth_inclination_to_geocentric(
		const CartesianConvMatrix3D &ccm,
		const boost::tuple<real_t/*magnitude*/, real_t/*azimuth*/, real_t/*inclination*/> &magnitude_azimuth_inclination)
{
	return convert_from_north_east_down_to_geocentric(
			ccm,
			convert_from_magnitude_azimuth_inclination_to_north_east_down(magnitude_azimuth_inclination));
}


boost::tuple<
		GPlatesMaths::real_t/*magnitude*/,
		GPlatesMaths::real_t/*azimuth*/,
		GPlatesMaths::real_t/*inclination*/>
GPlatesMaths::convert_from_north_east_down_to_magnitude_azimuth_inclination(
		const Vector3D &north_east_down_vec)
{
	const real_t magnitude = north_east_down_vec.magnitude();
	if (magnitude == 0) // Epsilon test
	{
		return boost::tuple<real_t, real_t, real_t>(0, 0, 0);
	}

	real_t azimuth = atan2(north_east_down_vec.y() , north_east_down_vec.x());
	// Convert [-PI, PI] to [0, 2*PI]...
	if (azimuth.dval() < 0)
	{
		azimuth += 2 * PI;
	}

	const real_t inclination = asin(north_east_down_vec.z() / magnitude);

	return boost::make_tuple(magnitude, azimuth, inclination);
}


GPlatesMaths::Vector3D
GPlatesMaths::convert_from_magnitude_azimuth_inclination_to_north_east_down(
		const boost::tuple<real_t/*magnitude*/, real_t/*azimuth*/, real_t/*inclination*/> &magnitude_azimuth_inclination)
{
	real_t magnitude = boost::get<0>(magnitude_azimuth_inclination);
	real_t azimuth = boost::get<1>(magnitude_azimuth_inclination);
	real_t inclination = boost::get<2>(magnitude_azimuth_inclination);

	const real_t cos_inclination = cos(inclination);

	return Vector3D(
			magnitude * cos_inclination * cos(azimuth)/*North*/,
			magnitude * cos_inclination * sin(azimuth)/*East*/,
			magnitude * sin(inclination)/*Down*/);
}
