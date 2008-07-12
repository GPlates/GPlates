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
		throw UnableToNormaliseZeroVectorException(*this);
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


#ifdef HAVE_PYTHON
/**
 * Here begin the Python wrappers
 */


using namespace boost::python;
// This using is so str(self) will find an appropriate operator<<
using GPlatesMaths::operator<<;


void
GPlatesMaths::export_Vector3D()
{
	class_<GPlatesMaths::Vector3D>("Vector3D",
			init<const GPlatesMaths::real_t &, const GPlatesMaths::real_t &, const GPlatesMaths::real_t &>(args("x", "y", "z")))
		.def(init<const GPlatesMaths::Vector3D&>())
		.add_property("x", make_function(&GPlatesMaths::Vector3D::x, return_value_policy<return_by_value>()))
		.add_property("y", make_function(&GPlatesMaths::Vector3D::y, return_value_policy<return_by_value>()))
		.add_property("z", make_function(&GPlatesMaths::Vector3D::z, return_value_policy<return_by_value>()))
		.def("magSqrd", &GPlatesMaths::Vector3D::magSqrd)
		.def("magnitude", &GPlatesMaths::Vector3D::magnitude)
		.def("get_normalisation", &GPlatesMaths::Vector3D::get_normalisation)

		.def(self == other<GPlatesMaths::Vector3D>())
		.def(self != other<GPlatesMaths::Vector3D>())
		.def(-self)
		.def(other<GPlatesMaths::Real>() * self)
		.def(self * other<GPlatesMaths::Real>())
		.def(self + other<GPlatesMaths::Vector3D>())
		.def(self - other<GPlatesMaths::Vector3D>())
		.def(self_ns::str(self))
	;
	// MAGIC! This lets us retrieve a function pointer to a specific version of an overloaded function
	const GPlatesMaths::real_t (*dot)(const GPlatesMaths::Vector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::dot;
	def("dot", dot);
	bool (*parallel)(const GPlatesMaths::Vector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::parallel;
	def("parallel", parallel);
	bool (*perpendicular)(const GPlatesMaths::Vector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::perpendicular;
	def("perpendicular", perpendicular);
	bool (*collinear)(const GPlatesMaths::Vector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::collinear;
	def("collinear", collinear);
	const GPlatesMaths::Vector3D (*cross)(const GPlatesMaths::Vector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::cross;
	def("cross", cross);
}
#endif

