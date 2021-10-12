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
#include "LatLonPointConversions.h"


GPlatesMaths::CartesianConvMatrix3D::CartesianConvMatrix3D(const PointOnSphere
	&pos)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

	real_t lam = degreesToRadians(llp.latitude()),
	       phi = degreesToRadians(llp.longitude());

	_nx = -sin(lam) * cos(phi);
	_ny = -sin(lam) * sin(phi);
	_nz =  cos(lam);
	_ex = -sin(phi);
	_ey =  cos(phi);
	_ez =  0;
	_dx = -cos(lam) * cos(phi);
	_dy = -cos(lam) * sin(phi);
	_dz = -sin(lam);
}


GPlatesMaths::Vector3D
GPlatesMaths::operator*(const CartesianConvMatrix3D &ccm, const Vector3D &v)
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


