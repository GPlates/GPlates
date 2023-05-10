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

#ifndef GPLATES_MATHS_POINTONSPHERE_H
#define GPLATES_MATHS_POINTONSPHERE_H

#include <iosfwd>
#include <iterator>  // std::distance
#include <QDebug>
#include <QTextStream>

#include "GeometryOnSphere.h"
#include "UnitVector3D.h"
#include "LatLonPoint.h"
#include "TrailingLatLonCoordinateException.h"
#include "InvalidLatLonCoordinateException.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"

#include "utils/QtStreamable.h"


namespace GPlatesMaths
{
	class GreatCircleArc;
	class PointGeometryOnSphere;

	/** 
	 * Represents a point on the surface of a sphere. 
	 *
	 * This is represented internally as a 3D unit vector.  Fun fact:
	 * There is a one-to-one (bijective) correspondence between the set of
	 * all points on the surface of the sphere and the set of all 3D unit
	 * vectors.
	 *
	 * As long as the invariant of the unit vector is maintained, the point
	 * will definitely lie on the surface of the sphere.
	 *
	 * NOTE: This does *NOT* inherit from @a GeometryOnSphere (see @a PointGeometryOnSphere below for that).
	 *       This reduces the size of @a PointOnSphere from 40 to 24 bytes and consequently reduces
	 *       the size of multipoints, polylines and polygons. This is actually the main reason for
	 *       the separation into two classes.
	 */
	class PointOnSphere :
		// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
		public GPlatesUtils::QtStreamable<PointOnSphere>
	{
	public:

		/**
		 * This is the North Pole (latitude \f$ 90^\circ \f$).
		 */
		static const PointOnSphere north_pole;

		/**
		 * This is the South Pole (latitude \f$ -90^\circ \f$).
		 */
		static const PointOnSphere south_pole;


		/**
		 * Create a new PointOnSphere instance from the unit vector @a position_vector_.
		 *
		 * Note that, in contrast to variable-length geometries like PolylineOnSphere and
		 * PolygonOnSphere, a PointOnSphere has an internal representation which is fixed
		 * in size and known in advance (and relatively small, in fact -- only 3 'double's),
		 * so memory allocation is not needed when a PointOnSphere instance is copied.
		 */
		explicit 
		PointOnSphere(
				const UnitVector3D &position_vector_) :
			d_position_vector(position_vector_)
		{  }


		const UnitVector3D &
		position_vector() const
		{
			return d_position_vector;
		}


		/**
		 * Evaluate whether @a test_point is "close" to this
		 * point.
		 *
		 * The measure of what is "close" is provided by
		 * @a closeness_inclusion_threshold.
		 *
		 * If @a test_point is "close", the function will
		 * calculate exactly @em how close, and store that
		 * value in @a closeness.
		 */
		bool
		is_close_to(
				const PointOnSphere &test_point,
				const real_t &closeness_inclusion_threshold,
				real_t &closeness) const;


		/**
		 * Evaluate whether this point lies on @a gca.
		 */
		bool
		lies_on_gca(
				const GreatCircleArc &gca) const;


		/**
		 * Test for a proximity hit.
		 *
		 * If there is a hit, the pointer returned will be a pointer to extra information
		 * about the hit -- for example, the specific vertex (point) or segment (GCA) of a
		 * polyline which was hit.
		 *
		 * Otherwise (ie, if there was not a hit), the pointer returned will be null.
		 */
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const;

		/**
		 * Test for a proximity hit, but only on the vertices of the geometry.                                                                     
		 */
		ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const ProximityCriteria &criteria) const;


		/**
		 * Copy this point into a @a PointGeometryOnSphere instance and return that as its base class @a GeometryOnSphere.
		 *
		 * This is useful when a point needs to be returned as a @a GeometryOnSphere (note that
		 * @a PointOnSphere does *not* inherit @a GeometryOnSphere, only @a PointGeometryOnSphere inherits it).
		 */
		GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry_on_sphere() const;


		/**
		 * Copy this point into a @a PointGeometryOnSphere instance.
		 *
		 * This is useful when a point needs to be returned as a @a PointGeometryOnSphere.
		 */
		GPlatesUtils::non_null_intrusive_ptr<const PointGeometryOnSphere>  // Same as PointGeometryOnSphere::non_null_ptr_to_const_type
		get_point_geometry_on_sphere() const;

	private:

		/**
		 * This is the 3-D unit-vector which defines the position of this point.
		 */
		UnitVector3D d_position_vector;

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<PointOnSphere> &point_on_sphere);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/** 
	 * A derivation of @a GeometryOnSphere that wraps a @a PointOnSphere.
	 *
	 * In most uses cases only @a PointOnSphere should be needed (which uses less memory, 24 bytes versus 40 bytes),
	 * however @a PointGeometryOnSphere will be needed when accessing a point via the @a GeometryOnSphere interface.
	 */
	class PointGeometryOnSphere:
			public GeometryOnSphere,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<PointGeometryOnSphere>
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const PointGeometryOnSphere>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PointGeometryOnSphere> non_null_ptr_to_const_type;


		/**
		 * Create a new PointGeometryOnSphere instance on the heap from the PointOnSphere @a position_.
		 */
		static
		const non_null_ptr_to_const_type
		create(
				const PointOnSphere &position_)
		{
			return non_null_ptr_to_const_type(new PointGeometryOnSphere(position_));
		}


		/**
		 * Inherited from @a GeometryOnSphere.
		 */
		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const
		{
			return position().test_proximity(criteria);
		}


		/**
		 * Inherited from @a GeometryOnSphere.
		 */
		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
			const ProximityCriteria &criteria) const
		{
			return position().test_vertex_proximity(criteria);
		}

		/**
		 * Accept a ConstGeometryOnSphereVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstGeometryOnSphereVisitor &visitor) const;


		/**
		 * Return the wrapped @a PointOnSphere.
		 */
		const PointOnSphere &
		position() const
		{
			return d_position;
		}


		/**
		 * Return this instance as a non-null pointer.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

	private:

		/**
		 * Construct a PointGeometryOnSphere instance from a PointOnSphere.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the static 'create' function.
		 *
		 * It should also initialise the ref-count to zero.
		 */
		explicit 
		PointGeometryOnSphere(
				const PointOnSphere &position_):
			GeometryOnSphere(),
			d_position(position_)
		{  }


		/**
		 * The wrapped point-on-sphere position.
		 */
		PointOnSphere d_position;

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<PointGeometryOnSphere> &point_geometry_on_sphere);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Return the point antipodal to @a p on the sphere.
	 */
	inline
	const PointOnSphere
	get_antipodal_point(
			const PointOnSphere &p)
	{
		return PointOnSphere( -p.position_vector());
	}


	/**
	 * Calculate the "closeness" of the points @a p1 and @a p2 on the
	 * surface of the sphere.
	 *
	 * The "closeness" of two points is defined by the vector dot-product
	 * of the unit-vectors of the points.  What this means in practical
	 * terms is that the "closeness" of two points will be a value in the
	 * range [-1.0, 1.0], with a value of 1.0 signifying coincident points,
	 * and a value of -1.0 signifying antipodal points.
	 *
	 * To determine which of two points is closer to a given test-point,
	 * you would use a code snippet similar to the following:
	 * @code
	 * real_t c1 = calculate_closeness(point1, test_point);
	 * real_t c2 = calculate_closeness(point2, test_point);
	 * 
	 * if (c1 > c2) {
	 *
	 *     // point1 is closer to test_point.
	 *
	 * } else if (c2 > c1) {
	 *
	 *     // point2 is closer to test_point.
	 *
	 * } else {
	 *
	 *     // The points are equidistant from test_point.
	 * }
	 * @endcode
	 *
	 * Note that this measure of "closeness" cannot be used to construct a
	 * valid metric (alas).
	 */
	inline
	const real_t
	calculate_closeness(
			const PointOnSphere &p1,
			const PointOnSphere &p2)
	{
		return dot(p1.position_vector(), p2.position_vector());
	}
	

	/**
	 * Calculate the distance between the points @a p1 and @a p2 on the
	 * surface of the sphere of radius @a radius_of_sphere.
	 *
	 * Consider the circular disc that contains the two points and the centre
	 * of the sphere. The distance between the points is taken to be the length
	 * of the shortest arc between the two points on the circumference of that
	 * disc.
	 */
	const real_t
	calculate_distance_on_surface_of_sphere(
			const PointOnSphere &p1,
			const PointOnSphere &p2,
			real_t radius_of_sphere);


	/**
	 * Return whether the points @a p1 and @a p2 are coincident.
	 */
	inline
	bool
	points_are_coincident(
			const PointOnSphere &p1,
			const PointOnSphere &p2)
	{
		return p1.position_vector() == p2.position_vector();
	}


	/**
	 * Count the number of distinct adjacent points in the sequence @a point_seq in
	 * the range @a point_seq_begin / @a point_seq_end
	 * (which is assumed to be a sequence of PointOnSphere).
	 */
	template<typename PointForwardIter>
	unsigned
	count_distinct_adjacent_points(
			PointForwardIter point_seq_begin,
			PointForwardIter point_seq_end);

	/**
	 * Count the number of distinct adjacent points in the sequence @a point_seq of type S
	 * (which is assumed to be a sequence of PointOnSphere).
	 */
	template<typename S>
	unsigned
	count_distinct_adjacent_points(
			const S &point_seq)
	{
		return count_distinct_adjacent_points(point_seq.begin(), point_seq.end());
	}


	/**
	 * Populate the supplied (presumably empty) destination sequence @a dest_seq of type D
	 * (which is assumed to be a sequence of type PointOnSphere) from the source sequence
	 * range @a source_seq_begin / @a source_seq_end (which is assumed to be a sequence of double).
	 *
	 * The source sequence is assumed to contain doubles in the order used by GML in a
	 * "gml:posList" property in a "gml:LineString" geometry -- that is, each consecutive pair
	 * of doubles represents the (longitude, latitude) of a point: lon, lat, lon, lat, ...
	 *
	 * Note that this is the reverse of the usual (latitude, longitude) representation used in
	 * GPlates.
	 *
	 * It is presumed that the destination sequence is empty -- or its contents unimportant --
	 * since its contents, if any, will be swapped out into a temporary sequence and discarded
	 * at the end of the function.
	 *
	 * This function is strongly exception-safe and exception-neutral.
	 */
	template<typename PointForwardIter, typename D>
	void
	populate_point_on_sphere_sequence(
			D &dest_seq,
			PointForwardIter source_seq_begin,
			PointForwardIter source_seq_end);

	/**
	 * Populate the supplied (presumably empty) destination sequence @a dest_seq of type D
	 * (which is assumed to be a sequence of type PointOnSphere) from the source sequence
	 * @a source_seq of type S (which is assumed to be a sequence of double).
	 *
	 * The source sequence is assumed to contain doubles in the order used by GML in a
	 * "gml:posList" property in a "gml:LineString" geometry -- that is, each consecutive pair
	 * of doubles represents the (longitude, latitude) of a point: lon, lat, lon, lat, ...
	 *
	 * Note that this is the reverse of the usual (latitude, longitude) representation used in
	 * GPlates.
	 *
	 * It is presumed that the destination sequence is empty -- or its contents unimportant --
	 * since its contents, if any, will be swapped out into a temporary sequence and discarded
	 * at the end of the function.
	 *
	 * This function is strongly exception-safe and exception-neutral.
	 */
	template<typename S, typename D>
	void
	populate_point_on_sphere_sequence(
			D &dest_seq,
			const S &source_seq)
	{
		return populate_point_on_sphere_sequence(
				dest_seq, source_seq.begin(), source_seq.end());
	}


	std::ostream &
	operator<<(
			std::ostream &os,
			const PointOnSphere &p);


	std::ostream &
	operator<<(
			std::ostream &os,
			const PointGeometryOnSphere &p);


	inline
	bool
	operator==(
			const PointOnSphere &p1,
			const PointOnSphere &p2)
	{
		return p1.position_vector() == p2.position_vector();
	}

	inline
	bool
	operator!=(
			const PointOnSphere &p1,
			const PointOnSphere &p2)
	{
		return p1.position_vector() != p2.position_vector();
	}


	inline
	bool
	operator==(
			const PointGeometryOnSphere &p1,
			const PointGeometryOnSphere &p2)
	{
		return p1.position() == p2.position();
	}

	inline
	bool
	operator!=(
			const PointGeometryOnSphere &p1,
			const PointGeometryOnSphere &p2)
	{
		return p1.position() != p2.position();
	}


	/**
	 * Enables PointOnSphere to be used as a key in a 'std::map'.
	 *
	 * For example, std::map<PointOnSphere, mapped_type, PointOnSphereMapPredicate>.
	 *
	 *
	 * NOTE: If two points are close enough together then they will correspond to the same map entry
	 * since the map's equivalence relation...
	 *
	 *   !compare(point1, point2) && !compare(point2, point1)
	 *
	 * ...will be true (due to the use of an epsilon in the comparison).
	 *
	 * This may not be desirable if you want 'point1' and 'point2' to have separate map entries.
	 *
	 *
	 * You might then be tempted to write a slightly different map predicate that does exact comparisons
	 * of the floating-point x/y/z coordinates using ('<' and '>' operators in the predicate) instead
	 * of using an epsilon. And this will probably work fine when the points used to look up the map
	 * are the exact same points added to the map as keys. However note that the compiler can do
	 * things like comparing one 80-bit double (in a floating-point register) with a 64-bit double (in memory)
	 * that was truncated from a 80-bit register when written to memory. Google
	 * "[29.18] Why is cos(x) != cos(y) even though x == y? (Or sine or tangent or log or just about any other floating point computation)"
	 * for more details on this. However this is extremely unlikely in our example above (using the same
	 * points to look up the map as used to create the map) because when a point is first created it is
	 * unlikely that an untruncated version of that point (eg, x/y/z cached in 80-bit registers) will get
	 * compiled in during the map creation but a truncated version of that point compiled in during the
	 * map look up (or vice versa). Still it's probably not in the realm of impossibility.
	 */
	class PointOnSphereMapPredicate
	{
	public:
		bool
		operator()(
				const PointOnSphere &lhs,
				const PointOnSphere &rhs) const;
	};
}


template<typename PointForwardIter>
unsigned
GPlatesMaths::count_distinct_adjacent_points(
		PointForwardIter point_seq_begin,
		PointForwardIter point_seq_end)
{
	PointForwardIter iter = point_seq_begin;
	PointForwardIter end = point_seq_end;

	if (iter == end) {
		// The container is empty.
		return 0;
	}
	// else, the container is not empty.
	PointOnSphere recent = *iter;
	unsigned num_distinct_adjacent_points = 1;
	for (++iter; iter != end; ++iter) {
		if (*iter != recent) {
			++num_distinct_adjacent_points;
			recent = *iter;
		}
	}
	return num_distinct_adjacent_points;
}


template<typename PointForwardIter, typename D>
void
GPlatesMaths::populate_point_on_sphere_sequence(
		D &dest_seq,
		PointForwardIter source_seq_begin,
		PointForwardIter source_seq_end)
{
	D tmp_seq;

	// First, verify that the source is of an even length -- otherwise, there will be a
	// trailing coordinate which cannot be paired.
	if ((std::distance(source_seq_begin, source_seq_end) % 2) != 0) {
		// Uh oh, it's an odd-length collection.
		// Note that, since the length of the sequence is odd, the length must be
		// greater than zero, so there must be at least one element.
		PointForwardIter back_iter = source_seq_end;
		--back_iter;
		throw TrailingLatLonCoordinateException(GPLATES_EXCEPTION_SOURCE,
				*back_iter,
				std::distance(source_seq_begin, source_seq_end));
	}
	PointForwardIter iter = source_seq_begin;
	PointForwardIter end = source_seq_end;
	for (unsigned coord_index = 0; iter != end; ++iter, ++coord_index) {
		double lon = *iter;
		if ( ! LatLonPoint::is_valid_longitude(lon)) {
			throw InvalidLatLonCoordinateException(GPLATES_EXCEPTION_SOURCE,
					lon,
					InvalidLatLonCoordinateException::LongitudeCoord,
					coord_index);
		}
		++iter;
		double lat = *iter;
		if ( ! LatLonPoint::is_valid_latitude(lat)) {
			throw InvalidLatLonCoordinateException(GPLATES_EXCEPTION_SOURCE,
					lat,
					InvalidLatLonCoordinateException::LatitudeCoord,
					coord_index);
		}
		LatLonPoint llp(lat, lon);
		tmp_seq.push_back(make_point_on_sphere(llp));
	}

	dest_seq.swap(tmp_seq);
}

#endif  // GPLATES_MATHS_POINTONSPHERE_H
