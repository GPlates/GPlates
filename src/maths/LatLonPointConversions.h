/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004 The University of Sydney, Australia
 *  (under the name "OperationsOnSphere.h")
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "LatLonPointConversions.h")
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

#ifndef GPLATES_MATHS_LATLONPOINTCONVERSIONS_H
#define GPLATES_MATHS_LATLONPOINTCONVERSIONS_H

#include <iosfwd>
#include <list>

#include "PointOnSphere.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths {

	class LatLonPoint {

		public:

			/**
			 * Make a point in the standard spherical coordinate
			 * system.
			 * @throws InvalidLatLonException when
			 *   @p isValidLat( @a lat ) returns false or
			 *   @p isValidLon( @a lon ) returns false.
			 */
			LatLonPoint(
			 const real_t &lat,
			 const real_t &lon);

			/**
			 * Return whether a given value is a valid latitude.
			 *
			 * GPlates uses the range [-90.0, 90.0].
			 */
			static
			bool
			isValidLat(
			 const real_t &val);

			/**
			 * Return whether a given value is a valid longitude.
			 *
			 * GPlates uses the half-open range (-180.0, 180.0] for
			 * output, but accepts [-360.0, 360.0] as input.
			 */
			static
			bool
			isValidLon(
			 const real_t &val);

			real_t
			latitude() const {
				
				return m_lat;
			}

			real_t
			longitude() const {
				
				return m_lon;
			}

		private:

			real_t m_lat, m_lon;  // in degrees

	};


	inline
	bool
	operator==(
	 const LatLonPoint &p1,
	 const LatLonPoint &p2) {

		return ((p1.latitude() == p2.latitude()) &&
		        (p1.longitude() == p2.longitude()));
	}


	std::ostream &
	operator<<(
	 std::ostream &os,
	 const LatLonPoint &p);


	namespace LatLonPointConversions {

		const PointOnSphere
		convertLatLonPointToPointOnSphere(
		 const LatLonPoint &);

		const LatLonPoint 
		convertPointOnSphereToLatLonPoint(
		 const PointOnSphere &);
						
		/**
		 * The list must contain at least TWO LatLonPoints.
		 * No two successive LatLonPoints may be equivalent.
		 *
		 * @throws InvalidPolylineException if @a llpl contains less 
		 *   than two LatLonPoints.
		 *
		 * This function is strongly exception-safe and exception
		 * neutral.
		 *
		 * If a pair of identical adjacent points is found, the second
		 * will be silently elided; this occurs sometimes when parsing
		 * otherwise-valid PLATES "line data" files.
		 */
		const PolylineOnSphere
		convertLatLonPointListToPolylineOnSphere(
		 const std::list< LatLonPoint > &);

		/**
		 * Populate the supplied (presumably empty) sequence of
		 * LatLonPoints using the supplied PolylineOnSphere.
		 *
		 * It is presumed that the sequence of LatLonPoints is empty --
		 * or its contents unimportant -- since its contents, if any,
		 * will be swapped out into a temporary sequence and discarded
		 * at the end of the function.
		 *
		 * This function is strongly exception-safe.  *sigh*  If only
		 * the same could be said for the rest of this crap (and yes,
		 * I wrote most of it).
		 */
		template< typename S >
		void
		populate_lat_lon_point_sequence(
		 S &sequence,
		 const PolylineOnSphere &polyline);

	}

	/// The north pole (latitude \f$ 90^\circ \f$)
	static const PointOnSphere NorthPole =
	 LatLonPointConversions::convertLatLonPointToPointOnSphere(
	  LatLonPoint(90.0, 0.0));

	/// The south pole (latitude \f$ -90^\circ \f$)
	static const PointOnSphere SouthPole =
	 LatLonPointConversions::convertLatLonPointToPointOnSphere(
	  LatLonPoint(-90.0, 0.0));

}


template< typename S >
void
GPlatesMaths::LatLonPointConversions::populate_lat_lon_point_sequence(
 S &sequence,
 const PolylineOnSphere &polyline) {

	S tmp_seq;

	PolylineOnSphere::const_iterator
	 iter = polyline.begin(),
	 end = polyline.end();

	if (iter == end) {

		// This Polyline contains no segments.
		// FIXME: Should we, uh, like, COMPLAIN about this?...
		// It's probably invalid...
		return;
	}

	// The first LatLonPoint in the list will be the start-point of the
	// first GreatCircleArc...
	tmp_seq.push_back(
	 convertPointOnSphereToLatLonPoint(iter->start_point()));

	for ( ; iter != end; ++iter) {

		// ... and all the rest of the LatLonPoints will be end-points.
		tmp_seq.push_back(
		 convertPointOnSphereToLatLonPoint(iter->end_point()));
	}

	// OK, if we made it to here without throwing an exception, we're safe.
	// Let's swap the contents into the target sequence.
	sequence.swap(tmp_seq);  // This won't throw.
}

#endif  // GPLATES_MATHS_LATLONPOINTCONVERSIONS_H
