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
#include <QString>

#include "ConstGeometryOnSphereVisitor.h"
#include "PointLiesOnGreatCircleArc.h"
#include "PointOnSphere.h"
#include "PointProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "MathsUtils.h"


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::north_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90.0, 0.0));


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::south_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(-90.0, 0.0));


const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesMaths::PointOnSphere::get_non_null_pointer() const
{
	if (get_reference_count() == 0)
	{
		// There are no intrusive-pointers referencing this class.  Hence, this instance is
		// either on the stack or on the heap managed by a non-intrusive-pointer mechanism.
		// 
		// Either way, we should clone this instance, so the clone can be managed by
		// intrusive-pointers.
		//

		// NOTE: this error message has been commented out because a valid usage of
		// this method ('get_non_null_pointer()') occurs when calling 'test_proxmity()' on
		// a PointOnSphere that is not reference counted.
		//
		//std::cerr << "PointOnSphere::get_non_null_pointer just cloned something!"
		//		<< std::endl;

		return clone_as_point();
	}
	else
	{
		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return non_null_ptr_to_const_type(this);
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PointOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	double closeness = calculate_closeness(criteria.test_point(), *this).dval();
	if (closeness > criteria.closeness_inclusion_threshold()) {
		return make_maybe_null_ptr(PointProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness));
	} else {
		return ProximityHitDetail::null;
	}
}

GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PointOnSphere::test_vertex_proximity(
	const ProximityCriteria &criteria) const
{
	double closeness = calculate_closeness(criteria.test_point(), *this).dval();
	unsigned int index = 0;
	if (closeness > criteria.closeness_inclusion_threshold()) {
		return make_maybe_null_ptr(PointProximityHitDetail::create(
			this->get_non_null_pointer(),
			closeness,
			index));
	} else {
		return ProximityHitDetail::null;
	}
}


void
GPlatesMaths::PointOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_point_on_sphere(this->get_non_null_pointer());
}


bool
GPlatesMaths::PointOnSphere::is_close_to(
		const PointOnSphere &test_point,
		const real_t &closeness_inclusion_threshold,
		real_t &closeness) const
{
	closeness = calculate_closeness(test_point, *this);
	return closeness.is_precisely_greater_than(closeness_inclusion_threshold.dval());
}


bool
GPlatesMaths::PointOnSphere::lies_on_gca(
		const GreatCircleArc &gca) const
{
	PointLiesOnGreatCircleArc test_whether_lies_on_gca(gca);
	return test_whether_lies_on_gca(*this);
}


const GPlatesMaths::real_t
GPlatesMaths::calculate_distance_on_surface_of_sphere(
		const PointOnSphere &p1,
		const PointOnSphere &p2,
		real_t radius_of_sphere)
{
	if (p1 == p2)
	{
		return 0.0;
	}
	else
	{
		return acos(calculate_closeness(p1, p2)) * radius_of_sphere;
	}
}


std::ostream &
GPlatesMaths::operator<<(
	std::ostream &os,
	const PointOnSphere &p)
{
	os << p.position_vector();
	return os;
}


QDebug
GPlatesMaths::operator <<(
		QDebug dbg,
		const PointOnSphere &p)
{
	std::ostringstream output_string_stream;
	output_string_stream << p;

	dbg.nospace() << QString::fromStdString(output_string_stream.str());

	return dbg.space();
}


QTextStream &
GPlatesMaths::operator <<(
		QTextStream &stream,
		const PointOnSphere &p)
{
	std::ostringstream output_string_stream;
	output_string_stream << p;

	stream << QString::fromStdString(output_string_stream.str());

	return stream;
}


bool
GPlatesMaths::PointOnSphereMapPredicate::operator()(
		const PointOnSphere &lhs,
		const PointOnSphere &rhs) const
{
	const UnitVector3D &left = lhs.position_vector();
	const UnitVector3D &right = rhs.position_vector();

	const double dx = right.x().dval() - left.x().dval();
	if (dx > EPSILON) // left_x < right_x
	{
		return true;
	}
	if (-dx > EPSILON) // left_x > right_x
	{
		return false;
	}

	const double dy = right.y().dval() - left.y().dval();
	if (dy > EPSILON) // left_y < right_y
	{
		return true;
	}
	if (-dy > EPSILON) // left_y > right_y
	{
		return false;
	}

	const double dz = right.z().dval() - left.z().dval();
	if (dz > EPSILON) // left_z < right_z
	{
		return true;
	}
	if (-dz > EPSILON) // left_z > right_z
	{
		return false;
	}

	return false;
}
