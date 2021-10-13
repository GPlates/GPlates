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

#include "PointOnSphere.h"
#include "PointLiesOnGreatCircleArc.h"
#include "PointProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "ConstGeometryOnSphereVisitor.h"


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::north_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90.0, 0.0));


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::south_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(-90.0, 0.0));


const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesMaths::PointOnSphere::get_non_null_pointer() const
{
	if (get_reference_count() == 0) {
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
	} else {
		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return non_null_ptr_to_const_type(
				this,
				GPlatesUtils::NullIntrusivePointerHandler());
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


std::ostream &
GPlatesMaths::operator<<(
	std::ostream &os,
	const PointOnSphere &p)
{
	os << p.position_vector();
	return os;
}

