/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include <sstream>
#include "GridOnSphere.h"
#include "InvalidGridException.h"
#include "FunctionDomainException.h"
#include "ViolatedClassInvariantException.h"
#include "LatLonPoint.h"
#include "Rotation.h"
#include "Vector3D.h"


GPlatesMaths::GridOnSphere
GPlatesMaths::GridOnSphere::Create(const PointOnSphere &origin,
 const PointOnSphere &next_along_lat, const PointOnSphere &next_along_lon) {

	GridOnSphere::EnsureValidOrigin(origin);

	/*
	 * Create the small-circle of latitude (whose axis is the North pole)
	 * and the great-circle of longitude (which is defined to be "directed"
	 * from the South Pole up to the origin).
	 */
	SmallCircle line_of_lat(GPlatesMaths::PointOnSphere::north_pole.position_vector(), origin);
	GreatCircle line_of_lon(GPlatesMaths::PointOnSphere::south_pole, origin);

	/*
	 * Ensure that the point 'next_along_lat' lies on the circle of
	 * latitude.
	 */
	if ( ! line_of_lat.contains(next_along_lat)) {

		// origin and next_along_lat are on different latitudes
		std::ostringstream oss;
		oss << "Attempted to define a grid using points\n"
		 << "(" << origin << " and " << next_along_lat
		 << ") which were expected\n"
		 << "to lie on the same line of latitude, but do not.";
		throw InvalidGridException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}

	/*
	 * Ensure that the point 'next_along_lon' lies on the circle of
	 * longitude.
	 */
	if ( ! line_of_lon.contains(next_along_lon)) {

		// origin and next_along_lon are on different longitudes
		std::ostringstream oss;
		oss << "Attempted to define a grid using points\n"
		 << "(" << origin << " and " << next_along_lon
		 << ") which were expected\n"
		 << "to lie on the same line of longitude, but do not.";
		throw InvalidGridException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}

	/*
	 * Calculate the angular deltas along the lines of lat and lon.
	 *
	 * Note that these functions are used (in preference to the obvious
	 * PointOnSphere -> LatLonPoint conversion, followed by a subtraction
	 * of the corresponding lats or lons) in order to avoid the annoyances
	 * associated with the lat/lon coordinate system: in the best case,
	 * such annoyances lead to code filled with special cases; in the
	 * worst case, such annoyances lead to subtle bugs.
	 *
	 * Consider, for example: what is the difference in latitude (along
	 * a circle of longitude) between the points (88, -90) and (89, 90)?
	 * A naive subtraction of latitudes would yield a delta of 1 degree,
	 * when the delta is actually 3 degrees.  The naive method is broken
	 * by the boundary of the latitude system at the poles.
	 *
	 * A similar example can be constructed to demonstrate the problems
	 * caused by the boundary of the longitude system.
	 */
	real_t delta_along_lat =
	 calcDeltaAlongLat(origin.position_vector(), next_along_lat.position_vector(),
	                   line_of_lat.axis_vector());

	real_t delta_along_lon =
	 calcDelta(origin.position_vector(), next_along_lon.position_vector(),
	           line_of_lon.axis_vector());

	return GridOnSphere(line_of_lat, line_of_lon, origin,
	                    delta_along_lat, delta_along_lon);
}


void
GPlatesMaths::GridOnSphere::EnsureValidOrigin(const PointOnSphere &o) {

	real_t dp = dot(o.position_vector(), GPlatesMaths::PointOnSphere::north_pole.position_vector());
	if (dp >= 1.0) {

		// origin lies on North pole
		std::ostringstream oss;
		oss << "Attempted to define a grid using an origin\n"
		 << o << " which lies on the North pole.";
		throw InvalidGridException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
	if (dp <= -1.0) {

		// origin lies on South pole
		std::ostringstream oss;
		oss << "Attempted to define a grid using an origin\n"
		 << o << " which lies on the South pole.";
		throw InvalidGridException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
}


GPlatesMaths::real_t
GPlatesMaths::GridOnSphere::calcDeltaAlongLat(const UnitVector3D &orig,
 const UnitVector3D &next, const UnitVector3D &north) {

	/*
	 * We already know that 'orig' and 'next' indicate points which lie
	 * on the same circle of latitude around 'north'.  We also know that
	 * 'orig' is neither parallel nor antiparallel with 'north'.
	 *
	 * Now, calculate the parallel projection of 'orig' and 'next' onto
	 * 'north'.  [Since they lie on the same circle of latitude, their
	 * parallel projections will be equal.]
	 */
	real_t dp = dot(orig, north);  // equivalent to dot(next, north)
	Vector3D par = dp * north;

	/*
	 * Now calculate the orthogonal projections of 'orig' and 'next' from
	 * 'north'.
	 */
	Vector3D orig_orth = Vector3D(orig) - par;
	Vector3D next_orth = Vector3D(next) - par;

	/*
	 * Since 'orig' and 'next' are neither parallel nor antiparallel
	 * with 'north', they must possess a non-zero orthogonal projection.
	 * Hence, we can normalise them.
	 */
	return
	 calcDelta(orig_orth.get_normalisation(),
	  next_orth.get_normalisation(), north);
}


GPlatesMaths::real_t
GPlatesMaths::GridOnSphere::calcDelta(const UnitVector3D &orig,
 const UnitVector3D &next, const UnitVector3D &axis) {

	real_t   dp = dot(orig, next);
	Vector3D xp = cross(orig, next);
	real_t   tp = dot(xp, Vector3D(axis));

	if (tp > 0.0) {

		/*
		 * This indicates:
		 * -> 'xp' is non-zero, and in the same direction
		 *     as 'axis'.
		 * -> 'xp' is equal to (sin(theta) * axis), where
		 *     (theta > 0.0) and (theta < pi).
		 * -> 'theta' equals (acos(dp)).
		 */
		return acos(dp);

	} else if (tp < 0.0) {

		/*
		 * This indicates:
		 * -> 'xp' is non-zero, and in the opposite direction
		 *     to 'axis'.
		 * -> 'xp' is equal to (sin(theta) * axis), where
		 *     (theta < 0.0) and (theta > -pi).
		 * -> 'theta' equals (-acos(dp)).
		 */
		return -acos(dp);

	} else {

		/*
		 * This indicates:
		 * -> 'xp' is of zero length.
		 * -> 'orig' is either parallel to or antiparallel with
		 *     'next'.
		 * -> 'theta' is either 0 or pi.
		 * -> 'theta' equals (acos(dp)).
		 */
		return acos(dp);
	}
}


namespace {

	GPlatesMaths::PointOnSphere
	rotate_point_about_axis(
	 const GPlatesMaths::PointOnSphere &p,
	 const GPlatesMaths::UnitVector3D &rot_axis,
	 const GPlatesMaths::real_t &rot_angle) {

		using namespace GPlatesMaths;

		Rotation r = Rotation::create(rot_axis, rot_angle);
		return PointOnSphere(r * p);
	}

}


GPlatesMaths::PointOnSphere
GPlatesMaths::GridOnSphere::resolve(index_t x, index_t y) const {

	/*
	 * NOTE: that the order of rotation is important!
	 *
	 * Different great-circles of longitude have different normals,
	 * and thus different rotations associated with them, while all
	 * the small-circles of latitude share the same normal, and thus
	 * the same rotation.
	 *
	 * For this reason, the rotation about the axis of the great-circle
	 * of longitude must occur first -- if the rotation about the axis
	 * of the small-circle of latitude occurs first, the axis of the
	 * great-circle will be pointing in the wrong direction.  This would
	 * result in the point being rotated OFF the sphere!
	 */

	/*
	 * Rotate the origin to the appropriate latitude (along the line
	 * of longitude, ie. about the axis of the great circle of longitude).
	 */
	PointOnSphere rot_orig =
	 rotate_point_about_axis(_origin, _line_of_lon.axis_vector(),
	  y * _delta_along_lon);

	/*
	 * Next, rotate the rotated-origin to the appropriate longitude
	 * (about the axis of the small circle of latitude).
	 */
	return
	 rotate_point_about_axis(rot_orig, _line_of_lat.axis_vector(),
	  x * _delta_along_lat);
}


void
GPlatesMaths::GridOnSphere::AssertInvariantHolds() const {

	/*
	 * Firstly, ensure the great circle and small circle are perpendicular.
	 */
	if ( ! perpendicular(_line_of_lon.normal(), _line_of_lat.normal())) {

		// not perpendicular => oh no!
		std::ostringstream oss;
		oss << "Grid composed of non-perpendicular\n"
		 << "great circle (normal: " << _line_of_lon.normal() << ")\n"
		 << "and small circle (normal: " << _line_of_lat.normal()
		 << ").";
		throw ViolatedClassInvariantException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}

	/*
	 * Next, ensure the 'origin' still lies on both the great circle
	 * and small circle (which, since the two circles are perpendicular,
	 * implies that it must still lie on one of the two points of
	 * intersection).
	 */
	if ( ! _line_of_lon.contains(_origin)) {

		std::ostringstream oss;
		oss << "Grid origin " << _origin << " does not lie on\n"
		 << "great circle (normal: " << _line_of_lon.normal() << ").";
		throw ViolatedClassInvariantException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
	if ( ! _line_of_lat.contains(_origin)) {

		std::ostringstream oss;
		oss << "Grid origin " << _origin << " does not lie on\n"
		 << "small circle (normal: " << _line_of_lat.normal() << ").";
		throw ViolatedClassInvariantException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
}
