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

#include "PointOnSphere.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "PointLiesOnGreatCircleArc.h"
#include "PointProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "MathsUtils.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeDelegateProtocol.h"


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::north_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90.0, 0.0));


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::south_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(-90.0, 0.0));


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


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PointOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	double closeness = calculate_closeness(criteria.test_point(), *this).dval();
	if (closeness > criteria.closeness_inclusion_threshold())
	{
		return make_maybe_null_ptr(PointProximityHitDetail::create(*this, closeness));
	}
	else
	{
		return ProximityHitDetail::null;
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PointOnSphere::test_vertex_proximity(
	const ProximityCriteria &criteria) const
{
	double closeness = calculate_closeness(criteria.test_point(), *this).dval();
	unsigned int index = 0;
	if (closeness > criteria.closeness_inclusion_threshold())
	{
		return make_maybe_null_ptr(PointProximityHitDetail::create(*this, closeness, index));
	}
	else
	{
		return ProximityHitDetail::null;
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesMaths::PointOnSphere::get_geometry_on_sphere() const
{
	return PointGeometryOnSphere::create(*this);
}


GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type
GPlatesMaths::PointOnSphere::get_point_geometry_on_sphere() const
{
	return PointGeometryOnSphere::create(*this);
}


GPlatesScribe::TranscribeResult
GPlatesMaths::PointOnSphere::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<PointOnSphere> &point_on_sphere)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, point_on_sphere->d_position_vector, "position_vector");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<UnitVector3D> position_vector_ =
				scribe.load<UnitVector3D>(TRANSCRIBE_SOURCE, "position_vector");
		if (!position_vector_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		point_on_sphere.construct_object(position_vector_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesMaths::PointOnSphere::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, d_position_vector, "position_vector");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<UnitVector3D> position_vector_ =
					scribe.load<UnitVector3D>(TRANSCRIBE_SOURCE, "position_vector");
			if (!position_vector_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			d_position_vector = position_vector_;
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesMaths::PointGeometryOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_point_on_sphere(this);
}


GPlatesScribe::TranscribeResult
GPlatesMaths::PointGeometryOnSphere::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<PointGeometryOnSphere> &point_geometry_on_sphere)
{
	if (scribe.is_saving())
	{
		// Make PointGeometryOnSphere transcription compatible with PointOnSphere.
		GPlatesScribe::save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, point_geometry_on_sphere->d_position);
	}
	else // loading
	{
		// Make PointGeometryOnSphere transcription compatible with PointOnSphere.
		GPlatesScribe::LoadRef<PointOnSphere> position_ =
				GPlatesScribe::load_delegate_protocol<PointOnSphere>(TRANSCRIBE_SOURCE, scribe);
		if (!position_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		point_geometry_on_sphere.construct_object(position_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesMaths::PointGeometryOnSphere::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			// Make PointGeometryOnSphere transcription compatible with PointOnSphere.
			GPlatesScribe::save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, d_position);
		}
		else // loading
		{
			// Make PointGeometryOnSphere transcription compatible with PointOnSphere.
			GPlatesScribe::LoadRef<PointOnSphere> position_ =
					GPlatesScribe::load_delegate_protocol<PointOnSphere>(TRANSCRIBE_SOURCE, scribe);
			if (!position_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			d_position = position_;
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GeometryOnSphere, PointGeometryOnSphere>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
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


std::ostream &
GPlatesMaths::operator<<(
	std::ostream &os,
	const PointGeometryOnSphere &p)
{
	os << p.position();
	return os;
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
