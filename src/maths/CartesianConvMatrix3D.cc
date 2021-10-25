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

	_nx = -sin_lam * cos_phi;
	_ny = -sin_lam * sin_phi;
	_nz =  cos_lam;
	_ex = -sin_phi;
	_ey =  cos_phi;
	_ez =  0;
	_dx = -cos_lam * cos_phi;
	_dy = -cos_lam * sin_phi;
	_dz = -sin_lam;
}


GPlatesMaths::Vector3D
GPlatesMaths::operator*(
		const CartesianConvMatrix3D &ccm,
		const Vector3D &v)
{
	real_t n = ccm.nx() * v.x() +
	           ccm.ny() * v.y() +
	           ccm.nz() * v.z();

	real_t e = ccm.ex() * v.x() +
	           ccm.ey() * v.y() +
	           ccm.ez() * v.z();

	real_t d = ccm.dx() * v.x() +
	           ccm.dy() * v.y() +
	           ccm.dz() * v.z();

	return Vector3D(n, e, d);
}



GPlatesMaths::Vector3D
GPlatesMaths::inverse_multiply(
		const CartesianConvMatrix3D &ccm,
		const Vector3D &v)
{
	real_t n = v.x();
	real_t e = v.y();
	real_t d = v.z();

	real_t x = ccm.nx() * n +
	           ccm.ex() * e +
	           ccm.dx() * d;

	real_t y = ccm.ny() * n +
	           ccm.ey() * e +
	           ccm.dy() * d;

	real_t z = ccm.nz() * n +
	           ccm.ez() * e +
	           ccm.dz() * d;

	return Vector3D(x, y, z);
}
