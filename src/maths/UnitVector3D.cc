/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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
#include <sstream>
#include <boost/python.hpp>
#include "UnitVector3D.h"
#include "HighPrecision.h"
#include "ViolatedUnitVectorInvariantException.h"


GPlatesMaths::UnitVector3D::UnitVector3D(
		const real_t &x_comp,
		const real_t &y_comp,
		const real_t &z_comp) :
	d_x(x_comp),
	d_y(y_comp),
	d_z(z_comp)
{
	AssertInvariant(__LINE__);
}


GPlatesMaths::UnitVector3D::UnitVector3D(
		const Vector3D &v) :
	d_x(v.x()),
	d_y(v.y()),
	d_z(v.z())
{
	AssertInvariant(__LINE__);
}


void
GPlatesMaths::UnitVector3D::AssertInvariant(
		int line) const
{
	/*
	 * Calculate magnitude of vector to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 */
	real_t mag_sqrd = (d_x * d_x) + (d_y * d_y) + (d_z * d_z);
	if (mag_sqrd != 1.0) {
		// invariant has been violated
		std::ostringstream oss;
		oss << "UnitVector3D has magnitude-squared of "
				<< HighPrecision<real_t>(mag_sqrd)
				<< "\n(this assertion was invoked on line "
				<< line
				<< "of the header file).";
		throw ViolatedUnitVectorInvariantException(oss.str().c_str());
	}
}


GPlatesMaths::UnitVector3D
GPlatesMaths::generate_perpendicular(
		const UnitVector3D &u)
{
	/*
	 * Let's start with the three Cartesian basis vectors x, y and z.
	 * Take their dot-products with 'u' to test for orthogonality.
	 *
	 * Of course, since x, y and z are the vectors of an orthonormal
	 * basis, their dot-products with 'u' will simply extract individual
	 * components of 'u'.
	 */
	real_t xdot = u.x();
	real_t ydot = u.y();
	real_t zdot = u.z();

	if (xdot == 0.0) {
		// Instant winner!  x is perpendicular to 'u'.
		return UnitVector3D::xBasis();
	}
	if (ydot == 0.0) {
		// Instant winner!  y is perpendicular to 'u'.
		return UnitVector3D::yBasis();
	}
	if (zdot == 0.0) {
		// Instant winner!  z is perpendicular to 'u'.
		return UnitVector3D::zBasis();
	}

	/*
	 * Ok, so none of x, y or z are perpendicular to 'u'.
	 * As a result, we'll have to take one of them and calculate
	 * the cross-product of that vector with 'u'.  Recall that the
	 * result of a cross-product is perpendicular to its arguments.
	 *
	 * The result of a cross-product is perpendicular to its arguments
	 * because it is one of the two normals to the plane defined by the
	 * arguments.  If the arguments are collinear, it is not possible
	 * to determine a plane, hence, we must ensure that the basis vector
	 * we use is not collinear with 'u'.
	 *
	 * Since our three basis vectors are orthonormal, if one of them
	 * were collinear with 'u', the other two would be perpendicular to
	 * 'u'.  Since we have determined that *none* of them are perp. to
	 * 'u', we can deduce that none of them are collinear with 'u'.
	 *
	 * We want to use the basis vector whose dot-product with 'u' is the
	 * smallest: this vector will be the "most perpendicular" to 'u',
	 * and thus, will have the "most clearly-defined" cross-product.
	 *
	 * Since we will be taking the cross-product of non-collinear
	 * unit-vectors, the result will always have non-zero length, and
	 * hence, we can safely normalise it.
	 */
	if (xdot < ydot) {
		// prefer x over y
		if (xdot < zdot) {
			// prefer x over both y and z
			return cross(u, UnitVector3D::xBasis()).get_normalisation();

		} else {
			// prefer x over y, but z over x
			return cross(u, UnitVector3D::zBasis()).get_normalisation();
		}
	} else {
		// prefer y over x
		if (ydot < zdot) {
			// prefer y over both x and z
			return cross(u, UnitVector3D::yBasis()).get_normalisation();
		} else {
			// prefer y over x, but z over y
			return cross(u, UnitVector3D::zBasis()).get_normalisation();
		}
	}
}


const GPlatesMaths::Vector3D
GPlatesMaths::cross(
		const UnitVector3D &u1,
		const UnitVector3D &u2)
{
	return GenericVectorOps3D::ReturnType<Vector3D>::cross(u1, u2);
}


const GPlatesMaths::Vector3D
GPlatesMaths::cross(
		const UnitVector3D &u,
		const Vector3D &v)
{
	return GenericVectorOps3D::ReturnType<Vector3D>::cross(u, v);
}


const GPlatesMaths::Vector3D
GPlatesMaths::cross(
		const Vector3D &v,
		const UnitVector3D &u)
{
	return GenericVectorOps3D::ReturnType<Vector3D>::cross(v, u);
}


const GPlatesMaths::Vector3D
GPlatesMaths::operator*(
		const real_t &s,
		const UnitVector3D &u)
{
	return GenericVectorOps3D::ReturnType<Vector3D>::scale(s, u);
}


std::ostream &
GPlatesMaths::operator<<(
		std::ostream &os,
		const UnitVector3D &u)
{
	os << "(" << u.x() << ", " << u.y() << ", " << u.z() << ")";
	return os;
}


/**
 * Here begin the Python wrappers
 */


using namespace boost::python;
// This using is so str(self) will find an appropriate operator<<
using GPlatesMaths::operator<<;


void
GPlatesMaths::export_UnitVector3D()
{
	class_<GPlatesMaths::UnitVector3D>("UnitVector3D",
			init<const GPlatesMaths::real_t &, const GPlatesMaths::real_t &, const GPlatesMaths::real_t &>(args("x", "y", "z")))
		.def(init<const GPlatesMaths::UnitVector3D&>())
		.add_property("x", make_function(&GPlatesMaths::UnitVector3D::x, return_value_policy<return_by_value>()))
		.add_property("y", make_function(&GPlatesMaths::UnitVector3D::y, return_value_policy<return_by_value>()))
		.add_property("z", make_function(&GPlatesMaths::UnitVector3D::z, return_value_policy<return_by_value>()))
		// no assignment operator equivalent
		.add_static_property("xBasis", make_function(&GPlatesMaths::UnitVector3D::xBasis, return_value_policy<return_by_value>()))
		.add_static_property("yBasis", make_function(&GPlatesMaths::UnitVector3D::yBasis, return_value_policy<return_by_value>()))
		.add_static_property("zBasis", make_function(&GPlatesMaths::UnitVector3D::zBasis, return_value_policy<return_by_value>()))

		.def(self == other<GPlatesMaths::UnitVector3D>())
		.def(self != other<GPlatesMaths::UnitVector3D>())
		.def(-self)
		.def(other<GPlatesMaths::Real>() * self)
		.def(self * other<GPlatesMaths::Real>())
		.def(self_ns::str(self))
	;
	// MAGIC! This lets us retrieve a function pointer to a specific version of an overloaded function
	const GPlatesMaths::real_t (*dot)(const GPlatesMaths::UnitVector3D &, const GPlatesMaths::UnitVector3D &) = GPlatesMaths::dot;
	def("dot", dot);
	bool (*perpendicular)(const GPlatesMaths::UnitVector3D &, const GPlatesMaths::UnitVector3D &) = GPlatesMaths::perpendicular;
	def("perpendicular", perpendicular);
	bool (*parallel)(const GPlatesMaths::UnitVector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::parallel;
	def("parallel", parallel);
	def("unit_vectors_are_parallel", GPlatesMaths::unit_vectors_are_parallel);
	def("unit_vectors_are_antiparallel", GPlatesMaths::unit_vectors_are_antiparallel);
	bool (*collinear)(const GPlatesMaths::UnitVector3D &, const GPlatesMaths::UnitVector3D &) = GPlatesMaths::collinear;
	def("collinear", collinear);
	def("generate_perpendicular", GPlatesMaths::generate_perpendicular);
	const GPlatesMaths::Vector3D (*cross1)(const GPlatesMaths::UnitVector3D &, const GPlatesMaths::UnitVector3D &) = GPlatesMaths::cross;
	def("cross", cross1);
	const GPlatesMaths::Vector3D (*cross2)(const GPlatesMaths::UnitVector3D &, const GPlatesMaths::Vector3D &) = GPlatesMaths::cross;
	def("cross", cross2);
	const GPlatesMaths::Vector3D (*cross3)(const GPlatesMaths::Vector3D &, const GPlatesMaths::UnitVector3D &) = GPlatesMaths::cross;
	def("cross", cross3);
}

