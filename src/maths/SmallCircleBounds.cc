/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <QDebug>

#include "SmallCircleBounds.h"

#include "GreatCircleArc.h"

#include "FiniteRotation.h"
#include "Rotation.h"
#include "types.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesMaths::BoundingSmallCircle::Result
GPlatesMaths::BoundingSmallCircle::test(
		const UnitVector3D &test_point) const
{
	const double point_dot_small_circle_centre =
			dot(d_small_circle_centre, test_point).dval();

	// See if the test point is clearly outside the bound.
	if (point_dot_small_circle_centre < d_angular_extent.get_cosine().dval())
	{
		return OUTSIDE_BOUNDS;
	}
	// The test point must be inside the bound...

	return INSIDE_BOUNDS;
}


GPlatesMaths::BoundingSmallCircle::Result
GPlatesMaths::BoundingSmallCircle::test(
		const GreatCircleArc &gca) const
{
	double gca_min_dot_product = 1.0;
	double gca_max_dot_product = -1.0;

	update_min_max_dot_product(
		d_small_circle_centre, gca, gca_min_dot_product, gca_max_dot_product);

	if (gca_max_dot_product < d_angular_extent.get_cosine().dval())
	{
		return OUTSIDE_BOUNDS;
	}

	if (gca_min_dot_product > d_angular_extent.get_cosine().dval())
	{
		return INSIDE_BOUNDS;
	}

	return INTERSECTING_BOUNDS;
}


GPlatesMaths::BoundingSmallCircle::Result
GPlatesMaths::BoundingSmallCircle::test(
		const MultiPointOnSphere &multi_point) const
{
	//double point_min_dot_product = 1.0;
	//double point_max_dot_product = -1.0;

	const PointOnSphere &first_point = *multi_point.begin();

	const double first_point_dot_small_circle_centre =
			dot(d_small_circle_centre, first_point.position_vector()).dval();

	// See if the test point is clearly outside the bound.
	if (first_point_dot_small_circle_centre < d_angular_extent.get_cosine().dval())
	{
		// We only need to test for intersection now.
		MultiPointOnSphere::const_iterator multi_point_iter = multi_point.begin();
		for ( ++multi_point_iter;
			multi_point_iter != multi_point.end();
			++multi_point_iter)
		{
			const double point_dot_small_circle_centre =
					dot(d_small_circle_centre, multi_point_iter->position_vector()).dval();

			if (point_dot_small_circle_centre >= d_angular_extent.get_cosine().dval())
			{
				return INTERSECTING_BOUNDS;
			}
		}

		return OUTSIDE_BOUNDS;
	}
	// The test point must then be inside the bound...

	// We only need to test for intersection now.
	MultiPointOnSphere::const_iterator multi_point_iter = multi_point.begin();
	for ( ++multi_point_iter;
		multi_point_iter != multi_point.end();
		++multi_point_iter)
	{
		const double point_dot_small_circle_centre =
				dot(d_small_circle_centre, multi_point_iter->position_vector()).dval();

		if (point_dot_small_circle_centre <= d_angular_extent.get_cosine().dval())
		{
			return INTERSECTING_BOUNDS;
		}
	}

	return INSIDE_BOUNDS;
}


GPlatesMaths::BoundingSmallCircle::Result
GPlatesMaths::BoundingSmallCircle::test_filled_polygon(
		const PolygonOnSphere &polygon) const
{
	// Test the boundary of the polygon.
	Result result = test(polygon);

	// If the polygon boundary is outside the small circle then it's still possible
	// for the polygon to completely surround the small circle in which case it's
	// actually intersecting the bounding region.
	// We test this by seeing if the small circle centre is inside the polygon.
	if (result == OUTSIDE_BOUNDS)
	{
		const PointOnSphere small_circle_centre_point(d_small_circle_centre);

		const PointInPolygon::Result point_in_polygon_result =
				polygon.is_point_in_polygon(small_circle_centre_point);

		// If the small circle centre point is inside the polygon then the polygon is intersecting.
		if (point_in_polygon_result == PointInPolygon::POINT_INSIDE_POLYGON)
		{
			result = INTERSECTING_BOUNDS;
		}
	}

	return result;
}


const GPlatesMaths::BoundingSmallCircle
GPlatesMaths::operator*(
		const FiniteRotation &rotation,
		const BoundingSmallCircle &bounding_small_circle)
{
	// Make a copy so that the rotated small circle inherits any cached data (such as sine).
	BoundingSmallCircle rotated_bounding_small_circle(bounding_small_circle);

	// We only need to rotate the small circle centre - the other parameters remain the same.
	rotated_bounding_small_circle.set_centre(rotation * bounding_small_circle.get_centre());

	return rotated_bounding_small_circle;
}


const GPlatesMaths::BoundingSmallCircle
GPlatesMaths::operator*(
		const Rotation &rotation,
		const BoundingSmallCircle &bounding_small_circle)
{
	// Make a copy so that the rotated small circle inherits any cached data (such as sine).
	BoundingSmallCircle rotated_bounding_small_circle(bounding_small_circle);

	// We only need to rotate the small circle centre - the other parameters remain the same.
	rotated_bounding_small_circle.set_centre(rotation * bounding_small_circle.get_centre());

	return rotated_bounding_small_circle;
}


GPlatesMaths::BoundingSmallCircle
GPlatesMaths::create_optimal_bounding_small_circle(
		const BoundingSmallCircle &bounding_small_circle_1,
		const BoundingSmallCircle &bounding_small_circle_2)
{
	//PROFILE_FUNC();

	//
	// NOTE: We don't optimise away 'acos' in this function (using cosine and sine) because
	// it's a bit difficult since it involves (see below) half angle trigonometric identities
	// and clamping the final small circle bounds angle to PI.
	//

	const double angle_between_centres = acos(dot(
			bounding_small_circle_1.get_centre(),
			bounding_small_circle_2.get_centre())).dval();

	// We could cache the results of 'acos' angle with each bounding small circle but this
	// function is only really needed when building a bounding small circle binary tree
	// (in a bottom-up fashion) in which case the 'acos' angle is only queried once per
	// bounding small circle so there's currently no real need to cache it.
	const double angle_bounding_small_circle_1 =
			std::acos(bounding_small_circle_1.get_angular_extent().get_cosine().dval());
	const double angle_bounding_small_circle_2 =
			std::acos(bounding_small_circle_2.get_angular_extent().get_cosine().dval());

	// If the second small circle is inside the first small circle then the optimal bounding
	// small circle is just the first small circle.
	//
	// bounding_angle = angle_between_centres + small_circle_2_bounding_angle
	if (angle_between_centres + angle_bounding_small_circle_2 <= angle_bounding_small_circle_1)
	{
		return bounding_small_circle_1;
	}

	// If the first small circle is inside the second small circle then the optimal bounding
	// small circle is just the second small circle.
	//
	// bounding_angle = angle_between_centres + small_circle_1_bounding_angle
	if (angle_between_centres + angle_bounding_small_circle_1 <= angle_bounding_small_circle_2)
	{
		return bounding_small_circle_2;
	}

	//
	// Neither small circle is bounded by the other so we need to find the optimal centre and radius
	// for the new bounding small circle.
	//

	//
	// The bounding small circle centre is on the great circle arc whose end points are the two
	// small circle centres (C1 and C2).
	//
	// If that great circle arc spans an angle of A (from globe centre) then the total angle that
	// includes that great circle arc *and* both small circles is...
	//    A + R1 + R2
	// ...where R1 and R2 are the small circle angles (radii) of the two small circles.
	// Therefore the bounding small circle radius is...
	//    R = min[PI, (A + R1 + R2)/2]
	// ...and the 'min' is necessary because a small circle angle cannot be larger than PI since
	// PI means covering the entire globe and each of A, R1 and R2 can be PI so (A+R1+R2)/2 can
	// be 1.5 * PI.
	//
	// The bounding small circle centre position C is at the following angle relative to the
	// first small circle centre position C1...
	//    theta = R - R1
	//          = (A + R1 + R2)/2 - R1
	//          = (A + R2 - R1)/2
	// ...and the centre position is...
	//    C = cos(theta) * C1 + sin(theta) * normalise((C1 x C2) x C1)
	// ...where C2 is the second small circle centre position and 'x' is the vector cross product.
	// The term 'normalise((C1 x C2) x C1)' is the unit vector orthogonal to C1 that lies on the
	// great circle containing the great circle arc (C1,C2).
	//
	// If we got here then 'A + R2 > R1' because second small circle is not *inside* first small circle
	// and 'A + R1 > R2' because first small circle is not *inside* second small circle.
	// This means that...
	//    (A + R2 - R1)/2 > 0 because 'A + R2 > R1'
	//    (A + R2 - R1)/2 < A because 'A + R1 > R2' -> 'R2 - R1 < A'
	// ...which means that...
	//    0 < theta < A
	// ...thus the bounding small circle centre lies on the great circle arc (C1,C2).
	//

	const double theta = 0.5 * (
			angle_between_centres + angle_bounding_small_circle_2 - angle_bounding_small_circle_1);

	const Vector3D c1_cross_c2 = cross(
			bounding_small_circle_1.get_centre(),
			bounding_small_circle_2.get_centre());
	// If both bounding small circles centres are coincident then once small circle should have
	// been inside the other (because one will have a greater radius angle).
	// This might not have been caught above due to numerical precision issues so we'll just
	// return the small circle with the largest radius here.
	if (c1_cross_c2.magSqrd() <= 0)
	{
		return (angle_bounding_small_circle_1 > angle_bounding_small_circle_2)
				? bounding_small_circle_1
				: bounding_small_circle_2;
	}

	// Get the direction orthogonal to the first bounding small circle centre but pointing
	// towards the second bounding small circle centre (point on sphere).
	Vector3D orthogonal_vector = cross(c1_cross_c2, bounding_small_circle_1.get_centre());

	const UnitVector3D bounding_small_circle_centre = (
				std::cos(theta) * Vector3D(bounding_small_circle_1.get_centre()) +
				// We need to normalise the orthogonal vector before we use it...
				std::sin(theta) * (1.0 / orthogonal_vector.magnitude()) * orthogonal_vector
			// The new bounding small circle centre should have unit length but we normalize it
			// anyway due to numerical precision issues (otherwise UnitVector constructor might throw)...
			).get_normalisation();

	const double bounding_small_circle_angle = 0.5 * (
			angle_between_centres + angle_bounding_small_circle_1 + angle_bounding_small_circle_2);
	// Clamp to maximum possible bounding radius angle (PI) that covers the entire globe.
	const double cosine_bounding_small_circle_angle = (bounding_small_circle_angle < PI)
			? std::cos(bounding_small_circle_angle)
			: -1;

	return BoundingSmallCircle(
			bounding_small_circle_centre,
			AngularExtent::create_from_cosine(cosine_bounding_small_circle_angle));
}


GPlatesMaths::BoundingSmallCircleBuilder::BoundingSmallCircleBuilder(
		const UnitVector3D &small_circle_centre) :
	d_small_circle_centre(small_circle_centre),
	d_min_dot_product(1.0)
{
}


void
GPlatesMaths::BoundingSmallCircleBuilder::add(
		const UnitVector3D &point)
{
	const double point_dot_small_circle_centre =
			dot(point, d_small_circle_centre).dval();

	// See if the point is further than the current furthest so far.
	if (point_dot_small_circle_centre < d_min_dot_product)
	{
		d_min_dot_product = point_dot_small_circle_centre;
	}
}


void
GPlatesMaths::BoundingSmallCircleBuilder::add(
		const MultiPointOnSphere &multi_point)
{
	for (MultiPointOnSphere::const_iterator multi_point_iter = multi_point.begin();
		multi_point_iter != multi_point.end();
		++multi_point_iter)
	{
		add(*multi_point_iter);
	}
}


void
GPlatesMaths::BoundingSmallCircleBuilder::add(
		const BoundingSmallCircle &bounding_small_circle)
{
	//
	// new_bounding_angle = angle_between_centres + other_small_circle_bounding_angle
	//
	// ...and where cos(angle_between_centres) = dot(centre_circle_1, centre_circle_2).
	//

	// Get the cosine/sine of angle between the centres of both small circles.
	const AngularExtent angular_extent_between_small_circle_centres =
			AngularExtent::create_from_cosine(
					dot(d_small_circle_centre, bounding_small_circle.get_centre()));

	const AngularExtent angular_extent_new_bounding_angle =
			angular_extent_between_small_circle_centres + bounding_small_circle.get_angular_extent();

	// If the other small circle bound intersects, or is outside, our small circle then expand our
	// small circle to include it.
	if (angular_extent_new_bounding_angle.get_cosine().dval() < d_min_dot_product)
	{
		d_min_dot_product = angular_extent_new_bounding_angle.get_cosine().dval();
	}
}


GPlatesMaths::BoundingSmallCircle
GPlatesMaths::BoundingSmallCircleBuilder::get_bounding_small_circle(
		const double &expand_bound_delta_dot_product) const
{
	// The epsilon expands the dot product range covered
	// as a protection against numerical precision.
	// This epsilon should be larger than used in class Real (which is about 1e-12).
	double expanded_min_dot_product = d_min_dot_product - expand_bound_delta_dot_product;
	if (expanded_min_dot_product < -1)
	{
		expanded_min_dot_product = -1;
	}

	return BoundingSmallCircle(
			d_small_circle_centre,
			AngularExtent::create_from_cosine(expanded_min_dot_product));
}


GPlatesMaths::InnerOuterBoundingSmallCircle::Result
GPlatesMaths::InnerOuterBoundingSmallCircle::test(
		const UnitVector3D &test_point) const
{
	const double point_dot_small_circle_centre =
			dot(d_outer_small_circle.d_small_circle_centre, test_point).dval();

	// See if the test point is clearly outside the outer small circle.
	if (point_dot_small_circle_centre < d_outer_small_circle.get_angular_extent().get_cosine().dval())
	{
		return OUTSIDE_OUTER_BOUNDS;
	}

	// See if the test point is clearly inside the inner small circle.
	if (point_dot_small_circle_centre > d_inner_angular_extent.get_cosine().dval())
	{
		return INSIDE_INNER_BOUNDS;
	}

	return INTERSECTING_BOUNDS;
}


GPlatesMaths::InnerOuterBoundingSmallCircle::Result
GPlatesMaths::InnerOuterBoundingSmallCircle::test(
		const GreatCircleArc &gca) const
{
	double gca_min_dot_product = 1.0;
	double gca_max_dot_product = -1.0;

	update_min_max_dot_product(
		d_outer_small_circle.d_small_circle_centre, gca, gca_min_dot_product, gca_max_dot_product);

	if (gca_max_dot_product < d_outer_small_circle.get_angular_extent().get_cosine().dval())
	{
		return OUTSIDE_OUTER_BOUNDS;
	}

	if (gca_min_dot_product > d_inner_angular_extent.get_cosine().dval())
	{
		return INSIDE_INNER_BOUNDS;
	}

	return INTERSECTING_BOUNDS;
}


GPlatesMaths::InnerOuterBoundingSmallCircle::Result
GPlatesMaths::InnerOuterBoundingSmallCircle::test(
		const MultiPointOnSphere &multi_point) const
{
	//double point_min_dot_product = 1.0;
	//double point_max_dot_product = -1.0;

	const PointOnSphere &first_point = *multi_point.begin();

	const double first_point_dot_small_circle_centre =
			dot(d_outer_small_circle.d_small_circle_centre, first_point.position_vector()).dval();

	// See if the test point is clearly outside the outer small circle.
	if (first_point_dot_small_circle_centre < d_outer_small_circle.get_angular_extent().get_cosine().dval())
	{
		// We only need to test for intersection now.
		MultiPointOnSphere::const_iterator multi_point_iter = multi_point.begin();
		for ( ++multi_point_iter;
			multi_point_iter != multi_point.end();
			++multi_point_iter)
		{
			const double point_dot_small_circle_centre =
					dot(d_outer_small_circle.d_small_circle_centre, multi_point_iter->position_vector()).dval();

			if (point_dot_small_circle_centre >= d_outer_small_circle.get_angular_extent().get_cosine().dval())
			{
				return INTERSECTING_BOUNDS;
			}
		}

		return OUTSIDE_OUTER_BOUNDS;
	}

	// See if the test point is clearly inside the inner small circle.
	if (first_point_dot_small_circle_centre > d_inner_angular_extent.get_cosine().dval())
	{
		// We only need to test for intersection now.
		MultiPointOnSphere::const_iterator multi_point_iter = multi_point.begin();
		for ( ++multi_point_iter;
			multi_point_iter != multi_point.end();
			++multi_point_iter)
		{
			const double point_dot_small_circle_centre =
					dot(d_outer_small_circle.d_small_circle_centre, multi_point_iter->position_vector()).dval();

			if (point_dot_small_circle_centre <= d_inner_angular_extent.get_cosine().dval())
			{
				return INTERSECTING_BOUNDS;
			}
		}

		return INSIDE_INNER_BOUNDS;
	}

	return INTERSECTING_BOUNDS;
}


GPlatesMaths::InnerOuterBoundingSmallCircle::Result
GPlatesMaths::InnerOuterBoundingSmallCircle::test_filled_polygon(
		const PolygonOnSphere &polygon) const
{
	// Test the boundary of the polygon.
	Result result = test(polygon);

	// If the polygon boundary is outside the outer small circle then it's still possible
	// for the polygon to completely surround the outer small circle in which case it's
	// actually intersecting the bounding region.
	// We test this by seeing if the outer small circle centre is inside the polygon.
	if (result == OUTSIDE_OUTER_BOUNDS)
	{
		const PointOnSphere small_circle_centre_point(d_outer_small_circle.d_small_circle_centre);

		const PointInPolygon::Result point_in_polygon_result =
				polygon.is_point_in_polygon(small_circle_centre_point);

		// If the small circle centre point is inside the polygon then the polygon is intersecting.
		if (point_in_polygon_result == PointInPolygon::POINT_INSIDE_POLYGON)
		{
			result = INTERSECTING_BOUNDS;
		}
	}

	return result;
}


const GPlatesMaths::InnerOuterBoundingSmallCircle
GPlatesMaths::operator*(
		const FiniteRotation &rotation,
		const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
{
	// Make a copy so that the rotated small circle inherits any cached data (such as sine).
	InnerOuterBoundingSmallCircle rotated_inner_outer_bounding_small_circle(inner_outer_bounding_small_circle);

	// We only need to rotate the small circle centre - the other parameters remain the same.
	rotated_inner_outer_bounding_small_circle.set_centre(
			rotation * inner_outer_bounding_small_circle.get_centre());

	return rotated_inner_outer_bounding_small_circle;
}


const GPlatesMaths::InnerOuterBoundingSmallCircle
GPlatesMaths::operator*(
		const Rotation &rotation,
		const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
{
	// Make a copy so that the rotated small circle inherits any cached data (such as sine).
	InnerOuterBoundingSmallCircle rotated_inner_outer_bounding_small_circle(inner_outer_bounding_small_circle);

	// We only need to rotate the small circle centre - the other parameters remain the same.
	rotated_inner_outer_bounding_small_circle.set_centre(
			rotation * inner_outer_bounding_small_circle.get_centre());

	return rotated_inner_outer_bounding_small_circle;
}


bool
GPlatesMaths::SmallCircleBoundsImpl::intersect(
		const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
		const BoundingSmallCircle &bounding_small_circle,
		const double &dot_product_circle_centres)
{
	// If 'bounding_small_circle' is completely outside the outer bounds
	// of 'inner_outer_bounding_small_circle' or completely inside its inner bounds
	// then there's no intersection.
	return
		intersect(
			inner_outer_bounding_small_circle.get_outer_bounding_small_circle(),
			bounding_small_circle,
			dot_product_circle_centres) &&
		!is_inside_inner_bounding_small_circle(
			inner_outer_bounding_small_circle,
			bounding_small_circle,
			dot_product_circle_centres);
}


bool
GPlatesMaths::SmallCircleBoundsImpl::intersect(
		const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
		const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2,
		const double &dot_product_circle_centres)
{
	// Detect most likely case first - that both outer small circles do not intersect.
	if (!intersect(
			inner_outer_bounding_small_circle_1.get_outer_bounding_small_circle(),
			inner_outer_bounding_small_circle_2.get_outer_bounding_small_circle(),
			dot_product_circle_centres))
	{
		return false;
	}

	// See if the outer small circle of circle 2 is inside the inner small circle of circle 1.
	if (is_inside_inner_bounding_small_circle(
			inner_outer_bounding_small_circle_1,
			inner_outer_bounding_small_circle_2.get_outer_bounding_small_circle(),
			dot_product_circle_centres))
	{
		return false;
	}

	// See if the outer small circle of circle 1 is inside the inner small circle of circle 2.
	if (is_inside_inner_bounding_small_circle(
			inner_outer_bounding_small_circle_2,
			inner_outer_bounding_small_circle_1.get_outer_bounding_small_circle(),
			dot_product_circle_centres))
	{
		return false;
	}

	return true;
}


GPlatesMaths::InnerOuterBoundingSmallCircleBuilder::InnerOuterBoundingSmallCircleBuilder(
		const UnitVector3D &small_circle_centre) :
	d_small_circle_centre(small_circle_centre),
	d_min_dot_product(1.0),
	d_max_dot_product(-1.0)
{
}


void
GPlatesMaths::InnerOuterBoundingSmallCircleBuilder::add(
		const UnitVector3D &point)
{
	const double point_dot_small_circle_centre =
			dot(point, d_small_circle_centre).dval();

	// See if the point is closer/further than the current closest/furthest so far.
	if (point_dot_small_circle_centre > d_max_dot_product)
	{
		d_max_dot_product = point_dot_small_circle_centre;
	}
	if (point_dot_small_circle_centre < d_min_dot_product)
	{
		d_min_dot_product = point_dot_small_circle_centre;
	}
}


void
GPlatesMaths::InnerOuterBoundingSmallCircleBuilder::add(
		const MultiPointOnSphere &multi_point)
{
	for (MultiPointOnSphere::const_iterator multi_point_iter = multi_point.begin();
		multi_point_iter != multi_point.end();
		++multi_point_iter)
	{
		add(*multi_point_iter);
	}
}


void
GPlatesMaths::InnerOuterBoundingSmallCircleBuilder::add(
		const BoundingSmallCircle &bounding_small_circle)
{
	//
	// new_outer_bounding_angle = angle_between_centres + other_small_circle_bounding_angle
	//
	// ...and where cos(angle_between_centres) = dot(centre_circle_1, centre_circle_2).
	//

	// Get the cosine/sine of angle between the centres of both small circles.
	const AngularExtent angular_extent_between_small_circle_centres =
			AngularExtent::create_from_cosine(
					dot(d_small_circle_centre, bounding_small_circle.get_centre()));

	const AngularExtent angular_extent_new_outer_bounding_angle =
			angular_extent_between_small_circle_centres + bounding_small_circle.get_angular_extent();

	// If the other small circle bound intersects or is outside our outer small circle then expand
	// our outer small circle to include it.
	if (angular_extent_new_outer_bounding_angle.get_cosine().dval() < d_min_dot_product)
	{
		d_min_dot_product = angular_extent_new_outer_bounding_angle.get_cosine().dval();
	}

	// First test to see if the other small circle overlaps our small circle centre...
	if (angular_extent_between_small_circle_centres > bounding_small_circle.get_angular_extent())
	{
		// Our small circle centre is *not* contained within the other small circle.

		//
		// new_inner_bounding_angle = angle_between_centres - other_small_circle_bounding_angle
		//
		// ...and where cos(angle_between_centres) = dot(centre_circle_1, centre_circle_2).
		//

		// The angle from our small circle centre to the inner small circle that
		// puts the other small circle outside of it.
		const AngularExtent angular_extent_new_inner_bounding_angle =
				angular_extent_between_small_circle_centres - bounding_small_circle.get_angular_extent();

		// If the other small circle bound intersects or is inside our inner small circle then
		// contract our inner small circle to exclude it.
		if (angular_extent_new_inner_bounding_angle.get_cosine().dval() > d_max_dot_product)
		{
			d_max_dot_product = angular_extent_new_inner_bounding_angle.get_cosine().dval();
		}
	}
	else
	{
		// The other small circle overlaps our small circle centre which effectively removes
		// our inner small circle (shrinks it to a radius of zero).
		d_max_dot_product = 1;
	}
}


void
GPlatesMaths::InnerOuterBoundingSmallCircleBuilder::add(
		const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
{
	//
	// new_outer_bounding_angle = angle_between_centres + other_small_circle_outer_bounding_angle
	//
	// ...and where cos(angle_between_centres) = dot(centre_circle_1, centre_circle_2).
	//

	// Get the cosine/sine of angle between the centres of both small circles.
	const AngularExtent angular_extent_between_small_circle_centres =
			AngularExtent::create_from_cosine(
					dot(d_small_circle_centre, inner_outer_bounding_small_circle.get_centre()));

	// The angle from our small circle centre to the outer small circle that encompasses
	// the other outer small circle.
	const AngularExtent angular_extent_new_outer_bounding_angle =
			angular_extent_between_small_circle_centres +
				inner_outer_bounding_small_circle.get_outer_angular_extent();

	// If the other small circle outer bound intersects or is outside our outer small circle then expand
	// our outer small circle to include it.
	if (angular_extent_new_outer_bounding_angle.get_cosine().dval() < d_min_dot_product)
	{
		d_min_dot_product = angular_extent_new_outer_bounding_angle.get_cosine().dval();
	}

	// First test to see if the other small circle *inner* bound overlaps our small circle centre...
	if (angular_extent_between_small_circle_centres < inner_outer_bounding_small_circle.get_inner_angular_extent())
	{
		// Our small circle centre is *inside* the other small circle's inner bound.

		//
		// new_inner_bounding_angle = other_small_circle_inner_bounding_angle - angle_between_centres
		//
		// ...and where cos(angle_between_centres) = dot(centre_circle_1, centre_circle_2).
		//

		// The angle from our small circle centre to the inner small circle that
		// excludes the other small circle inner bound.
		const AngularExtent angular_extent_new_inner_bounding_angle =
				inner_outer_bounding_small_circle.get_inner_angular_extent() -
					angular_extent_between_small_circle_centres;

		// If the other small circle inner bound intersects or is inside our inner small circle then
		// contract our inner small circle to exclude it.
		if (angular_extent_new_inner_bounding_angle.get_cosine().dval() > d_max_dot_product)
		{
			d_max_dot_product = angular_extent_new_inner_bounding_angle.get_cosine().dval();
		}
	}
	// Next test to see if the other small circle *outer* bound overlaps our small circle centre...
	else if (angular_extent_between_small_circle_centres > inner_outer_bounding_small_circle.get_outer_angular_extent())
	{
		// Our small circle centre is *outside* the other small circle's outer bound.

		//
		// new_inner_bounding_angle = angle_between_centres - other_small_circle_outer_bounding_angle
		//
		// ...and where cos(angle_between_centres) = dot(centre_circle_1, centre_circle_2).
		//

		// The angle from our small circle centre to the inner small circle that
		// excludes the other small circle outer bound.
		const AngularExtent angular_extent_new_inner_bounding_angle =
				angular_extent_between_small_circle_centres -
					inner_outer_bounding_small_circle.get_outer_angular_extent();

		// If the other small circle outer bound intersects or is inside our inner small circle then
		// contract our inner small circle to exclude it.
		if (angular_extent_new_inner_bounding_angle.get_cosine().dval() > d_max_dot_product)
		{
			d_max_dot_product = angular_extent_new_inner_bounding_angle.get_cosine().dval();
		}
	}
	else
	{
		// The other small circle region (between its inner and outer small circles) overlaps our small
		// circle centre which effectively removes our inner small circle (shrinks it to a radius of zero).
		d_max_dot_product = 1;
	}
}


GPlatesMaths::InnerOuterBoundingSmallCircle
GPlatesMaths::InnerOuterBoundingSmallCircleBuilder::get_inner_outer_bounding_small_circle(
		const double &contract_inner_bound_delta_dot_product,
		const double &expand_outer_bound_delta_dot_product) const
{
	// If no primitives have been added then return an inner-outer bounding small circle
	// that has zero radius for both inner and outer small circles.
	// We can detect this by testing any min/max dot product is not its initial value of 1/-1.
	// Choose a difference half-way between (1 - (-1) = 2) of one to avoid numerical issues.
	if (d_min_dot_product - d_max_dot_product > 1)
	{
		qWarning() << "InnerOuterBoundingSmallCircleBuilder: no primitives added";
		return InnerOuterBoundingSmallCircle(
				d_small_circle_centre,
				AngularExtent::create_from_cosine(1.0),
				AngularExtent::create_from_cosine(1.0));
	}

	// The epsilon expands the dot product range covered
	// as a protection against numerical precision.
	// This epsilon should be larger than used in class Real (which is about 1e-12).
	double expanded_min_dot_product = d_min_dot_product - expand_outer_bound_delta_dot_product;
	if (expanded_min_dot_product < -1)
	{
		expanded_min_dot_product = -1;
	}
	double expanded_max_dot_product = d_max_dot_product + contract_inner_bound_delta_dot_product;
	if (expanded_max_dot_product > 1)
	{
		expanded_max_dot_product = 1;
	}

	return InnerOuterBoundingSmallCircle(
			d_small_circle_centre,
			AngularExtent::create_from_cosine(expanded_min_dot_product),
			AngularExtent::create_from_cosine(expanded_max_dot_product));
}


void
GPlatesMaths::update_min_max_dot_product(
		const UnitVector3D &small_circle_centre,
		const GreatCircleArc &gca,
		double &min_dot_product,
		double &max_dot_product)
{
	const UnitVector3D &edge_start_point = gca.start_point().position_vector();
	const UnitVector3D &edge_end_point = gca.end_point().position_vector();

	const double edge_start_dot_small_circle_centre =
			dot(edge_start_point, small_circle_centre).dval();
	const double edge_end_dot_small_circle_centre =
			dot(edge_end_point, small_circle_centre).dval();

	// See if the great circle arc endpoints are closer/further than
	// the current closest/furthest so far.
	if (edge_start_dot_small_circle_centre > max_dot_product)
	{
		max_dot_product = edge_start_dot_small_circle_centre;
	}
	if (edge_start_dot_small_circle_centre < min_dot_product)
	{
		min_dot_product = edge_start_dot_small_circle_centre;
	}
	if (edge_end_dot_small_circle_centre > max_dot_product)
	{
		max_dot_product = edge_end_dot_small_circle_centre;
	}
	if (edge_end_dot_small_circle_centre < min_dot_product)
	{
		min_dot_product = edge_end_dot_small_circle_centre;
	}

	if (gca.is_zero_length())
	{
		return;
	}

	// Great circle arc is not zero length and hence has a rotation axis...
	const UnitVector3D &edge_plane_normal = gca.rotation_axis();

	const Vector3D small_circle_centre_cross_edge_plane_normal =
			cross(small_circle_centre, edge_plane_normal);
	if (small_circle_centre_cross_edge_plane_normal.magSqrd() <= 0)
	{
		// The great circle arc rotation axis is aligned, within numerical precision,
		// with the small circle centre axis. Therefore the dot product
		// distance of all points on the great circle arc to the small circle centre
		// are the same (within numerical precision).
		// And we've already covered this case above.
		// The issue of precision can be covered by subtracting/adding an
		// epsilon to the min/max dot product to expand the region covered
		// by the great circle arc slightly.
		return;
	}

	// See if the great circle arc forms a local minima or maxima between
	// its endpoints. This happens if its endpoints are on opposite sides
	// of the dividing plane.
	const bool edge_start_point_on_negative_side = dot(
			small_circle_centre_cross_edge_plane_normal,
			edge_start_point).dval() <= 0;
	const bool edge_end_point_on_negative_side = dot(
			small_circle_centre_cross_edge_plane_normal,
			edge_end_point).dval() <= 0;

	// If endpoints on opposite sides of the plane.
	if (edge_start_point_on_negative_side ^ edge_end_point_on_negative_side)
	{
		if (edge_start_point_on_negative_side)
		{
			// The arc forms a local maxima...

			const UnitVector3D maxima_point = cross(
					small_circle_centre_cross_edge_plane_normal,
					edge_plane_normal)
				.get_normalisation();
			const double maxima_point_dot_small_circle_centre =
					dot(maxima_point, small_circle_centre).dval();
			// See if the current arc is the furthest so far from the
			// small circle centre point.
			if (maxima_point_dot_small_circle_centre < min_dot_product)
			{
				min_dot_product = maxima_point_dot_small_circle_centre;
			}
		}
		else
		{
			// The arc forms a local minima...

			const UnitVector3D minima_point = cross(
					edge_plane_normal,
					small_circle_centre_cross_edge_plane_normal)
				.get_normalisation();
			const double minima_point_dot_small_circle_centre =
					dot(minima_point, small_circle_centre).dval();
			// See if the current arc is the closest so far to the
			// small circle centre point.
			if (minima_point_dot_small_circle_centre > max_dot_product)
			{
				max_dot_product = minima_point_dot_small_circle_centre;
			}
		}
	}
}


void
GPlatesMaths::update_min_dot_product(
		const UnitVector3D &small_circle_centre,
		const GreatCircleArc &gca,
		double &min_dot_product)
{
	const UnitVector3D &edge_start_point = gca.start_point().position_vector();
	const UnitVector3D &edge_end_point = gca.end_point().position_vector();

	const double edge_start_dot_small_circle_centre =
			dot(edge_start_point, small_circle_centre).dval();
	const double edge_end_dot_small_circle_centre =
			dot(edge_end_point, small_circle_centre).dval();

	// See if the great circle arc endpoints are further than
	// the current furthest so far.
	if (edge_start_dot_small_circle_centre < min_dot_product)
	{
		min_dot_product = edge_start_dot_small_circle_centre;
	}
	if (edge_end_dot_small_circle_centre < min_dot_product)
	{
		min_dot_product = edge_end_dot_small_circle_centre;
	}

	if (gca.is_zero_length())
	{
		return;
	}

	// Great circle arc is not zero length and hence has a rotation axis...
	const UnitVector3D &edge_plane_normal = gca.rotation_axis();

	const Vector3D small_circle_centre_cross_edge_plane_normal =
			cross(small_circle_centre, edge_plane_normal);
	if (small_circle_centre_cross_edge_plane_normal.magSqrd() <= 0)
	{
		// The great circle arc rotation axis is aligned, within numerical precision,
		// with the small circle centre axis. Therefore the dot product
		// distance of all points on the great circle arc to the small circle centre
		// are the same (within numerical precision).
		// And we've already covered this case above.
		// The issue of precision can be covered by subtracting/adding an
		// epsilon to the min/max dot product to expand the region covered
		// by the great circle arc slightly.
		return;
	}

	// See if the great circle arc forms a local maxima between its endpoints.
	// This happens if its endpoints are on opposite sides of the dividing plane *and*
	// the edge start point is on the negative side of the dividing plane.
	if (dot(
			small_circle_centre_cross_edge_plane_normal,
			edge_start_point).dval() <= 0)
	{
		if (dot(
				small_circle_centre_cross_edge_plane_normal,
				edge_end_point).dval() >= 0)
		{
			// The arc forms a local maxima...

			const UnitVector3D maxima_point = cross(
					small_circle_centre_cross_edge_plane_normal,
					edge_plane_normal)
				.get_normalisation();
			const double maxima_point_dot_small_circle_centre =
					dot(maxima_point, small_circle_centre).dval();
			// See if the current arc is the furthest so far from the
			// small circle centre point.
			if (maxima_point_dot_small_circle_centre < min_dot_product)
			{
				min_dot_product = maxima_point_dot_small_circle_centre;
			}
		}
	}
}


void
GPlatesMaths::update_max_dot_product(
		const UnitVector3D &small_circle_centre,
		const GreatCircleArc &gca,
		double &max_dot_product)
{
	const UnitVector3D &edge_start_point = gca.start_point().position_vector();
	const UnitVector3D &edge_end_point = gca.end_point().position_vector();

	const double edge_start_dot_small_circle_centre =
			dot(edge_start_point, small_circle_centre).dval();
	const double edge_end_dot_small_circle_centre =
			dot(edge_end_point, small_circle_centre).dval();

	// See if the great circle arc endpoints are closer than
	// the current closest so far.
	if (edge_start_dot_small_circle_centre > max_dot_product)
	{
		max_dot_product = edge_start_dot_small_circle_centre;
	}
	if (edge_end_dot_small_circle_centre > max_dot_product)
	{
		max_dot_product = edge_end_dot_small_circle_centre;
	}

	if (gca.is_zero_length())
	{
		return;
	}

	// Great circle arc is not zero length and hence has a rotation axis...
	const UnitVector3D &edge_plane_normal = gca.rotation_axis();

	const Vector3D small_circle_centre_cross_edge_plane_normal =
			cross(small_circle_centre, edge_plane_normal);
	if (small_circle_centre_cross_edge_plane_normal.magSqrd() <= 0)
	{
		// The great circle arc rotation axis is aligned, within numerical precision,
		// with the small circle centre axis. Therefore the dot product
		// distance of all points on the great circle arc to the small circle centre
		// are the same (within numerical precision).
		// And we've already covered this case above.
		// The issue of precision can be covered by subtracting/adding an
		// epsilon to the min/max dot product to expand the region covered
		// by the great circle arc slightly.
		return;
	}

	// See if the great circle arc forms a local minima between its endpoints.
	// This happens if its endpoints are on opposite sides of the dividing plane *and*
	// the edge start point is on the positive side of the dividing plane.
	if (dot(
			small_circle_centre_cross_edge_plane_normal,
			edge_start_point).dval() >= 0)
	{
		if (dot(
				small_circle_centre_cross_edge_plane_normal,
				edge_end_point).dval() <= 0)
		{
			// The arc forms a local minima...

			const UnitVector3D minima_point = cross(
					edge_plane_normal,
					small_circle_centre_cross_edge_plane_normal)
				.get_normalisation();
			const double minima_point_dot_small_circle_centre =
					dot(minima_point, small_circle_centre).dval();
			// See if the current arc is the closest so far to the
			// small circle centre point.
			if (minima_point_dot_small_circle_centre > max_dot_product)
			{
				max_dot_product = minima_point_dot_small_circle_centre;
			}
		}
	}
}
