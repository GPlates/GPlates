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

#include "GeometryOnSphere.h"
#include "UnitVector3D.h"
#include "LatLonPointConversions.h"
#include "TrailingLatLonCoordinateException.h"
#include "InvalidLatLonCoordinateException.h"


namespace GPlatesMaths
{
	class GreatCircleArc;

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
	 */
	class PointOnSphere:
			public GeometryOnSphere
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
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;


		/**
		 * Create a new PointOnSphere instance on the heap from the unit vector
		 * @a position_vector_.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				const UnitVector3D &position_vector_);


		/**
		 * Clone this PointOnSphere instance, to create a duplicate instance on the
		 * heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const GeometryOnSphere::non_null_ptr_to_const_type
		clone_as_geometry() const
		{
			GeometryOnSphere::non_null_ptr_to_const_type dup(
					new PointOnSphere(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Clone this PointOnSphere instance, to create a duplicate instance on the
		 * heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const non_null_ptr_to_const_type
		clone_as_point() const
		{
			non_null_ptr_to_const_type dup(
					new PointOnSphere(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Get a non-null pointer to a const PointOnSphere which points to this instance
		 * (or a clone of this instance).
		 *
		 * (Since geometries are treated as immutable literals in GPlates, a geometry can
		 * never be modified through a pointer, so there is no reason why it would be
		 * inappropriate to return a pointer to a clone of this instance rather than a
		 * pointer to this instance.)
		 *
		 * This function will behave correctly regardless of whether this instance is on
		 * the stack or the heap.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer() const;


		/**
		 * Create a new PointOnSphere instance from the unit vector @a position_vector_.
		 *
		 * Note that, in contrast to variable-length geometries like PolylineOnSphere and
		 * PolygonOnSphere, a PointOnSphere has an internal representation which is fixed
		 * in size and known in advance (and relatively small, in fact -- only 3 'double's
		 * and a single 'long'), so memory allocation is not needed when a PointOnSphere
		 * instance is copied.  For this reason, it's not such a tragedy if PointOnSphere
		 * instances are instantiated on the stack and copied into containers by value.
		 */
		explicit 
		PointOnSphere(
				const UnitVector3D &position_vector_):
			GeometryOnSphere(),
			d_position_vector(position_vector_)
		{  }


		/**
		 * Create a copy-constructed PointOnSphere instance on the stack.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * copy-constructor would, except that it should initialise the ref-count to zero.
		 *
		 * Note that, in contrast to variable-length geometries like PolylineOnSphere and
		 * PolygonOnSphere, a PointOnSphere has an internal representation which is fixed
		 * in size and known in advance (and relatively small, in fact -- only 3 'double's
		 * and a single 'long'), so memory allocation is not needed when a PointOnSphere
		 * instance is copied.  For this reason, it's not such a tragedy if PointOnSphere
		 * instances are instantiated on the stack and copied into containers by value.
		 */
		PointOnSphere(
				const PointOnSphere &other):
			GeometryOnSphere(),
			d_position_vector(other.d_position_vector)
		{  }


		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const;


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
		 * Copy-assign the value of @a other to this.
		 *
		 * This function has a nothrow exception-safety guarantee.
		 *
		 * This copy-assignment operator should act exactly the same as the default
		 * (auto-generated) copy-assignment operator would, except that it should
		 * not assign the ref-count of @a other to this.
		 */
		PointOnSphere &
		operator=(
				const PointOnSphere &other)
		{
			d_position_vector = other.d_position_vector;
			return *this;
		}


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
		 *
		 * For more information, read the comment before
		 * @a GPlatesGui::ProximityTests::find_close_rfgs.
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

	private:

		/**
		 * This is the 3-D unit-vector which defines the position of this point.
		 */
		UnitVector3D d_position_vector;

	};


	inline
	const PointOnSphere::non_null_ptr_to_const_type
	PointOnSphere::create_on_heap(
			const UnitVector3D &position_vector_)
	{
		non_null_ptr_to_const_type ptr(
				new PointOnSphere(position_vector_),
				GPlatesUtils::NullIntrusivePointerHandler());
		return ptr;
	}


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


	inline
	bool
	operator==(
			const PointOnSphere &p1,
			const PointOnSphere &p2)
	{
		return p1.position_vector() == p2.position_vector();
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
	inline
	const real_t
	calculate_distance_on_surface_of_sphere(
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


	inline
	bool
	operator!=(
			const PointOnSphere &p1,
			const PointOnSphere &p2)
	{
		return p1.position_vector() != p2.position_vector();
	}


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
	template<typename ForwardIter>
	unsigned
	count_distinct_adjacent_points(
			ForwardIter point_seq_begin,
			ForwardIter point_seq_end);

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
	template<typename ForwardIter, typename D>
	void
	populate_point_on_sphere_sequence(
			D &dest_seq,
			ForwardIter source_seq_begin,
			ForwardIter source_seq_end);

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


	/**
	 * This routine exports the Python wrapper class and associated functionality
	 */
	void export_PointOnSphere();
}


template<typename ForwardIter>
unsigned
GPlatesMaths::count_distinct_adjacent_points(
		ForwardIter point_seq_begin,
		ForwardIter point_seq_end)
{
	ForwardIter iter = point_seq_begin;
	ForwardIter end = point_seq_end;

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


template<typename ForwardIter, typename D>
void
GPlatesMaths::populate_point_on_sphere_sequence(
		D &dest_seq,
		ForwardIter source_seq_begin,
		ForwardIter source_seq_end)
{
	D tmp_seq;

	// First, verify that the source is of an even length -- otherwise, there will be a
	// trailing coordinate which cannot be paired.
	if ((std::distance(source_seq_begin, source_seq_end) % 2) != 0) {
		// Uh oh, it's an odd-length collection.
		// Note that, since the length of the sequence is odd, the length must be
		// greater than zero, so there must be at least one element.
		ForwardIter back_iter = source_seq_end;
		--back_iter;
		throw TrailingLatLonCoordinateException(GPLATES_EXCEPTION_SOURCE,
				*back_iter,
				std::distance(source_seq_begin, source_seq_end));
	}
	ForwardIter iter = source_seq_begin;
	ForwardIter end = source_seq_end;
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
