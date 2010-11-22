/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <ostream>

#include "Vector3D.h"
#include "UnitVector3D.h"
#include "UnableToNormaliseZeroVectorException.h"
#include "HighPrecision.h"


GPlatesMaths::Vector3D::Vector3D(
		const UnitVector3D &u) :
	d_x(u.x()),
	d_y(u.y()),
	d_z(u.z())
{  }


GPlatesMaths::UnitVector3D
GPlatesMaths::Vector3D::get_normalisation() const
{
	real_t mag_sqrd = (d_x * d_x) + (d_y * d_y) + (d_z * d_z);
	if (mag_sqrd <= 0.0) {
		throw UnableToNormaliseZeroVectorException(GPLATES_EXCEPTION_SOURCE, *this);
	}
	real_t scale = 1 / sqrt(mag_sqrd);
	return UnitVector3D(d_x * scale, d_y * scale, d_z * scale);
}


const GPlatesMaths::Vector3D
GPlatesMaths::cross(
		const Vector3D &v1,
		const Vector3D &v2)
{
	return GenericVectorOps3D::ReturnType<Vector3D>::cross(v1, v2);
}


const GPlatesMaths::Vector3D
GPlatesMaths::operator*(
		const real_t &s,
		const Vector3D &v)
{
	return GenericVectorOps3D::ReturnType<Vector3D>::scale(s, v);
}


std::ostream &
GPlatesMaths::operator<<(
		std::ostream &os,
		const Vector3D &v)
{
	os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
	return os;
}

