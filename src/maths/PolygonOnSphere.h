/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POLYGONONSPHERE_H
#define GPLATES_MATHS_POLYGONONSPHERE_H

#include <cstddef>  // For std::size_t
#include <vector>
#include <algorithm>  // std::swap
#include <utility>  // std::pair
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "GeometryOnSphere.h"
#include "GreatCircleArc.h"
#include "PolygonOrientation.h"

#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	// Forward declarations.
	namespace PolygonOnSphereImpl
	{
		struct CachedCalculations;
	}
	class BoundingSmallCircle;
	class InnerOuterBoundingSmallCircle;

	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	class PolyGreatCircleArcBoundingTree;


	/** 
	 * Represents a polygon on the surface of a sphere. 
	 *
	 * Internally, this is stored as a sequence of GreatCircleArc.  You
	 * can iterate over this sequence of GreatCircleArc in the usual manner
	 * using the const_iterators returned by the functions @a begin and
	 * @a end.
	 *
	 * You can also iterate over the @em vertices of the polygon using the
	 * vertex_const_iterators returned by the functions @a vertex_begin and
	 * @a vertex_end.  For instance, to copy all the vertices of a polygon
	 * into a list of PointOnSphere, you would use the code snippet:
	 * @code
	 * std::list< PointOnSphere > the_list(polygon.vertex_begin(),
	 *                                     polygon.vertex_end());
	 * @endcode
	 *
	 * You can create a polygon by invoking the static member function
	 * @a PolygonOnSphere::create_on_heap, passing it a sequential STL
	 * container (list, vector, ...) of PointOnSphere to define the
	 * vertices of the polygon.  The sequence of points must contain at
	 * least three distinct elements, enabling the creation of a polygon
	 * composed of at least three well-defined segments.  The requirements
	 * upon the sequence of points are described in greater detail in the
	 * comment for the static member function
	 * @a PolygonOnSphere::evaluate_construction_parameter_validity.
	 *
	 * Say you have a sequence of PointOnSphere: [A, B, C, D].  If you pass
	 * this sequence to the @a PolygonOnSphere::create_on_heap function, it
	 * will create a polygon composed of 4 segments: A->B, B->C, C->D and D->A. 
	 * (Iterating through the arcs of the polygon using the member functions
	 * @a begin and @a end will return these 4 segments.)
	 * If you subsequently iterate through the vertices of this polygon,
	 * you will get the same sequence of points back again: A, B, C, D.
	 *
	 * Note that PolygonOnSphere does have mutators (non-const member functions
	 * which enable the modification of the class internals), in particular
	 * the copy-assignment operator.
	 */
	class PolygonOnSphere:
			public GeometryOnSphere
	{
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<PolygonOnSphere,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 *
		 * Note that this typedef is indeed meant to be private.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PolygonOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;


		/**
		 * The type of the sequence of great circle arcs.
		 *
		 * Implementation detail:  We are using 'std::vector' as the sequence type (rather
		 * than, say, 'std::list') to provide a speed-up in memory-allocation (we use
		 * 'std::vector::reserve' at creation time to avoid expensive reallocations as arcs
		 * are appended one-by-one; after that, because the contents of the sequence are
		 * never altered, the size of the vector will never change), a speed-up in
		 * iteration (for what it's worth, a pointer-increment rather than a
		 * 'node = node->next'-style operation) and a decrease (hopefully) in memory-usage
		 * (by avoiding a whole bunch of unnecessary links).
		 *
		 * (We should, however, be able to get away without relying upon the
		 * "random-access"ness of vector iterators; forward iterators should be enough.)
		 */
		typedef std::vector<GreatCircleArc> seq_type;


		/**
		 * The type used to const_iterate over the sequence of arcs.
		 */
		typedef seq_type::const_iterator const_iterator;


		/**
		 * The type used to describe collection sizes.
		 */
		typedef seq_type::size_type size_type;


		/**
		 * Typedef for the bounding tree of great circle arcs in a polyline.
		 */
		typedef PolyGreatCircleArcBoundingTree<const_iterator, true/*RequireRandomAccessIterator*/>
				bounding_tree_type;

	private:

		/**
		 * The type of a function that returns the start point of a GCA.
		 */
		typedef boost::function< const GPlatesMaths::PointOnSphere & (
				const GreatCircleArc &) > gca_start_point_fn_type;

	public:

		/**
		 * The type used to const_iterate over the vertices.
		 */
		typedef boost::transform_iterator<gca_start_point_fn_type, const_iterator> vertex_const_iterator;
		typedef vertex_const_iterator VertexConstIterator;

		/**
		 * The possible return values from the construction-parameter
		 * validation functions
		 * @a evaluate_construction_parameter_validity and
		 * @a evaluate_segment_endpoint_validity.
		 */
		enum ConstructionParameterValidity
		{
			VALID,
			INVALID_INSUFFICIENT_DISTINCT_POINTS,
			INVALID_ANTIPODAL_SEGMENT_ENDPOINTS
		};


		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * What this actually means in plain(er) English is that you
		 * can use this function to check whether you would be able to
		 * construct a polygon instance from a given set of parameters
		 * (ie, your collection of points in the range [@a begin, @a end) ).
		 *
		 * If you pass this function what turns out to be invalid
		 * construction-parameters, it will politely return an error
		 * diagnostic.  If you were to pass these same invalid
		 * parameters to the creation functions down below, you would
		 * get an exception thrown back at you.
		 *
		 * It's not terribly difficult to obtain a collection which
		 * qualifies as as valid parameters (no antipodal adjacent points;
		 * at least three distinct points in the collection -- nothing
		 * particularly unreasonable) but the creation functions are
		 * fairly unsympathetic if your parameters @em do turn out to
		 * be invalid.
		 *
		 * The half-open range [@a begin, @a end) should contain
		 * PointOnSphere objects.
		 *
		 * @a invalid_points is a return-parameter; if the
		 * construction-parameters are found to be invalid due to
		 * antipodal adjacent points, the value of this return-parameter
		 * will be set to the pair of iterators of type ForwardIter which
		 * point to the guilty points.  If no adjacent points are found
		 * to be antipodal, this parameter will not be modified.
		 *
		 * If @a check_distinct_points is 'true' then the sequence of points
		 * is validated for insufficient *distinct* points, otherwise it is validated
		 * for insufficient points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 */
		template<typename ForwardIter>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				ForwardIter begin,
				ForwardIter end,
				std::pair<ForwardIter, ForwardIter> &invalid_points,
				bool check_distinct_points = false);

		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * What this actually means in plain(er) English is that you
		 * can use this function to check whether you would be able to
		 * construct a polygon instance from a given set of parameters
		 * (ie, your collection of points in @a coll).
		 *
		 * If you pass this function what turns out to be invalid
		 * construction-parameters, it will politely return an error
		 * diagnostic.  If you were to pass these same invalid
		 * parameters to the creation functions down below, you would
		 * get an exception thrown back at you.
		 *
		 * It's not terribly difficult to obtain a collection which
		 * qualifies as as valid parameters (no antipodal adjacent points;
		 * at least three distinct points in the collection -- nothing
		 * particularly unreasonable) but the creation functions are
		 * fairly unsympathetic if your parameters @em do turn out to
		 * be invalid.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * @a invalid_points is a return-parameter; if the
		 * construction-parameters are found to be invalid due to
		 * antipodal adjacent points, the value of this return-parameter
		 * will be set to the pair of const_iterators of @a coll which
		 * point to the guilty points.  If no adjacent points are found
		 * to be antipodal, this parameter will not be modified.
		 *
		 * If @a check_distinct_points is 'true' then the sequence of points
		 * is validated for insufficient *distinct* points, otherwise it is validated
		 * for insufficient points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 */
		template<typename C>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const C &coll,
				std::pair<typename C::const_iterator, typename C::const_iterator> &invalid_points,
				bool check_distinct_points = false)
		{
			return evaluate_construction_parameter_validity(
					coll.begin(), coll.end(), invalid_points, check_distinct_points);
		}


		/**
		 * Evaluate the validity of the points @a p1 and @a p2 for use
		 * in the creation of a polygon line-segment.
		 *
		 * You won't ever @em need to call this function
		 * (@a evaluate_construction_parameter_validity will do all the
		 * calling for you), but it's here in case you ever, you know,
		 * @em want to...
		 */
		static
		ConstructionParameterValidity
		evaluate_segment_endpoint_validity(
				const PointOnSphere &p1,
				const PointOnSphere &p2);


		/**
		 * Create a new PolygonOnSphere instance on the heap from the sequence of points
		 * in the range @a begin / @a end, and return an intrusive_ptr which points to
		 * the newly-created instance.
		 *
		 * If @a check_distinct_points is 'true' then the sequence of points
		 * is validated for insufficient *distinct* points, otherwise it is validated
		 * for insufficient points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (ie, any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 *
		 * This flag is 'false' by default but should be set to 'true' whenever data is loaded
		 * into GPlates (ie, at any input to GPlates such as file IO). This flag was added to
		 * prevent exceptions being thrown when reconstructing very small polygons containing only
		 * a few points (eg, a polygon with 4 points might contain 3 distinct points when it's
		 * loaded from a file but, due to numerical precision, contain only 2 distinct points after
		 * it is rotated to a new polygon thus raising an exception when one it not really needed
		 * or desired - because the polygon was good enough to load into GPlates therefore any
		 * rotation of it should also be successful).
		 *
		 * @throws InvalidPointsForPolygonConstructionError if there are insufficient points for the polygon.
		 * If @a check_distinct_points is true then the number of *distinct* points is counted,
		 * otherwise the number of *indistinct* points (ie, the total number of points) is counted.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template<typename ForwardIter>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				ForwardIter begin,
				ForwardIter end,
				bool check_distinct_points = false);

		/**
		 * Create a new PolygonOnSphere instance on the heap from the sequence of points
		 * @a coll, and return an intrusive_ptr which points to the newly-created instance.
		 *
		 * @a coll should be a sequential STL container (list, vector, ...) of
		 * PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template<typename C>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				const C &coll,
				bool check_distinct_points = false)
		{
			return create_on_heap(coll.begin(), coll.end(), check_distinct_points);
		}


		virtual
		~PolygonOnSphere();


		/**
		 * Clone this PolygonOnSphere instance, to create a duplicate instance on the heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const GeometryOnSphere::non_null_ptr_to_const_type
		clone_as_geometry() const
		{
			GeometryOnSphere::non_null_ptr_to_const_type dup(
					new PolygonOnSphere(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Clone this PolygonOnSphere instance, to create a duplicate instance on the heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const non_null_ptr_to_const_type
		clone_as_polygon() const
		{
			non_null_ptr_to_const_type dup(
					new PolygonOnSphere(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Get a non-null pointer to a const PolygonOnSphere which points to this instance
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
		get_non_null_pointer() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}


		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const;


		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
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
		 * This function is strongly exception-safe and exception-neutral.
		 *
		 * This copy-assignment operator should act exactly the same as the default
		 * (auto-generated) copy-assignment operator would, except that it should not
		 * assign the ref-count of @a other to this.
		 */
		PolygonOnSphere &
		operator=(
				const PolygonOnSphere &other)
		{
			// Use the copy+swap idiom to enable strong exception safety.
			PolygonOnSphere dup(other);
			this->swap(dup);
			return *this;
		}


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polygon.
		 */
		const_iterator
		begin() const
		{
			return d_seq.begin();
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polygon.
		 */
		const_iterator
		end() const
		{
			return d_seq.end();
		}


		/**
		 * Return the number of segments in this polygon.
		 */
		size_type
		number_of_segments() const
		{
			return d_seq.size();
		}


		/**
		 * Return the segment in this polygon at the specified index.
		 */
		const GreatCircleArc &
		get_segment(
				size_type segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					segment_index < number_of_segments(),
					GPLATES_ASSERTION_SOURCE);

			return d_seq[segment_index];
		}


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * vertices of this polygon.
		 */
		vertex_const_iterator
		vertex_begin() const
		{
			return boost::make_transform_iterator(
					d_seq.begin(),
					gca_start_point_fn_type(
						boost::bind(&GreatCircleArc::start_point, _1)));
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * vertices of this polygon.
		 */
		vertex_const_iterator
		vertex_end() const
		{
			return boost::make_transform_iterator(
					d_seq.end(),
					gca_start_point_fn_type(
						boost::bind(&GreatCircleArc::start_point, _1)));
		}


		/**
		 * Return the number of vertices in this polygon.
		 */
		size_type
		number_of_vertices() const
		{
			return d_seq.size();
		}


		/**
		 * Return the vertex in this polygon at the specified index.
		 */
		const PointOnSphere &
		get_vertex(
				size_type vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					vertex_index < number_of_vertices(),
					GPLATES_ASSERTION_SOURCE);

			vertex_const_iterator vertex_iter = vertex_begin();
			// This should be fast since iterator type is random access...
			std::advance(vertex_iter, vertex_index);

			return *vertex_iter;
		}


		/**
		 * Return the first vertex in this polygon.
		 *
		 * This is the first point specified as a point in the polygon.
		 */
		const PointOnSphere &
		first_vertex() const
		{
			const GreatCircleArc &first_gca = *(begin());
			return first_gca.start_point();
		}


		/**
		 * Return the last vertex in this polygon.
		 *
		 * This is the last point specified as a point in the polygon.  It is presumably
		 * different to the start-point.  (FIXME: We should ensure this at creation time.)
		 */
		const PointOnSphere &
		last_vertex() const
		{
			const GreatCircleArc &last_gca = *(--(end()));
			return last_gca.end_point();
		}


		/**
		 * Swap the contents of this polygon with @a other.
		 *
		 * This function does not throw.
		 */
		void
		swap(
				PolygonOnSphere &other)
		{
			// Obviously, we should not swap the ref-counts of the instances.
			d_seq.swap(other.d_seq);
			d_cached_calculations.swap(other.d_cached_calculations);
		}


		/**
		 * Evaluate whether @a test_point is "close" to this polygon.
		 *
		 * Note: This function currently only tests whether
		 * @a test_point is "close" to the polygon @em boundary.
		 *
		 * The measure of what is "close" is provided by @a closeness_angular_extent_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in @a closeness and
		 * return the closest point on the PolygonOnSphere.
		 */
		boost::optional<PointOnSphere>
		is_close_to(
				const PointOnSphere &test_point,
				const AngularExtent &closeness_angular_extent_threshold,
				real_t &closeness) const;


		/**
		 * Equality operator compares great circle arc subsegments.
		 */
		bool
		operator==(
				const PolygonOnSphere &other) const
		{
			return d_seq == other.d_seq;
		}

		/**
		 * Inequality operator.
		 */
		bool
		operator!=(
				const PolygonOnSphere &other) const
		{
			return !operator==(other);
		}


		//
		// The following are cached calculations on the geometry data.
		//


		/**
		 * Returns the total arc-length of the sequence of @a GreatCirclArc which defines this polygon.
		 *
		 * The result is in radians and represents the distance on the unit radius sphere.
		 *
		 * The result is cached on first call.
		 */
		const real_t &
		get_arc_length() const;


		/**
		 * Returns the area of this polygon.
		 *
		 * See 'SphericalArea::calculate_polygon_area' for details.
		 *
		 * The result is cached on first call.
		 */
		real_t
		get_area() const;

		/**
		 * Returns the signed area of this polygon.
		 *
		 * See 'SphericalArea::calculate_polygon_signed_area' for details.
		 *
		 * The result is cached on first call.
		 */
		const real_t &
		get_signed_area() const;


		/**
		 * Returns the orientation of this polygon.
		 *
		 * See 'PolygonOrientation::calculate_polygon_orientation' for details.
		 *
		 * The result is cached on first call.
		 */
		PolygonOrientation::Orientation
		get_orientation() const;


		/**
		 * Determines the speed versus memory trade-off of point-in-polygon tests.
		 *
		 * NOTE: The set up cost is a once-off cost that happens in the first call to
		 * @a is_point_in_polygon or if you increase the speed.
		 *
		 * See PointInPolygon for more details. But in summary...
		 *
		 * Use:
		 * LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE              0 < N < 4     points tested per polygon,
		 * MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE   4 < N < 200   points tested per polygon,
		 * HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE         N > 200       points tested per polygon.
		 *
		 * Or just use ADAPTIVE to progressively switch through the above stages as the number
		 * of calls to @a is_point_in_polygon increases, eventually ending up at
		 * HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE if enough calls are made.
		 */
		enum PointInPolygonSpeedAndMemory
		{
			ADAPTIVE = 0,
			LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE = 1,
			MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE = 2,
			HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE = 3
		};

		/**
		 * Tests whether the specified point is inside this polygon.
		 *
		 * The default @a speed_and_memory is adaptive.
		 *
		 * @a speed_and_memory determines how fast the point-in-polygon test should be
		 * and how much memory it uses.
		 *
		 * NOTE: The set up cost is a once-off cost that happens in the first call to
		 * @a is_point_in_polygon, or if you increase the speed.
		 *
		 * You can increase the speed but you cannot reduce it - this is because it takes
		 * longer to set up for the higher speed tests and reducing back to lower speeds
		 * would effectively remove any advantages gained.
		 */
		bool
		is_point_in_polygon(
				const PointOnSphere &point,
				PointInPolygonSpeedAndMemory speed_and_memory = ADAPTIVE) const;


		/**
		 * Returns the centroid of the *edges* of this polygon (see @a Centroid::calculate_outline_centroid).
		 *
		 * This centroid is useful for the centre of a small circle that bounds this polygon.
		 *
		 * The result is cached on first call.
		 */
		const UnitVector3D &
		get_boundary_centroid() const;


		/**
		 * Returns the centroid of the *interior* of this polygon (see @a Centroid::calculate_outline_centroid).
		 *
		 * This centroid is useful when the centroid should be closer to the centre-of-mass of
		 * the polygon interior.
		 *
		 * The result is cached on first call.
		 */
		const UnitVector3D &
		get_interior_centroid() const;


		/**
		 * Returns the small circle that bounds this polygon - the small circle centre
		 * is the same as calculated by @a get_boundary_centroid.
		 *
		 * The result is cached on first call.
		 */
		const BoundingSmallCircle &
		get_bounding_small_circle() const;


		/**
		 * Returns the inner and outer small circle bounds of this polygon - the small circle centre
		 * is the same as calculated by @a get_boundary_centroid.
		 *
		 * The result is cached on first call.
		 */
		const InnerOuterBoundingSmallCircle &
		get_inner_outer_bounding_small_circle() const;


		/**
		 * Returns the small circle bounding tree over of great circle arc segments of this polygon.
		 *
		 * The result is cached on first call.
		 */
		const bounding_tree_type &
		get_bounding_tree() const;

	private:

		/**
		 * Create an empty PolygonOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of a polygon without any vertices.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the static 'create_on_heap' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * default-constructor would, except that it should initialise the ref-count to
		 * zero.
		 */
		PolygonOnSphere();


		/**
		 * Create a copy-constructed PolygonOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the 'clone_as_geometry' or 'clone_as_polygon' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * copy-constructor would, except that it should initialise the ref-count to zero.
		 */
		PolygonOnSphere(
				const PolygonOnSphere &other);


		/**
		 * Generate a sequence of polygon segments from the collection
		 * of points in the range @a begin / @a end, using the points to
		 * define the vertices of the segments, then swap this new sequence
		 * of segments into the polygon @a poly, discarding any sequence of
		 * segments which may have been there before.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template<typename ForwardIter>
		static
		void
		generate_segments_and_swap(
				PolygonOnSphere &poly,
				ForwardIter begin,
				ForwardIter end,
				bool check_distinct_points);


		/**
		 * This is the minimum number of (distinct) collection points to be passed into the
		 * 'create_on_heap' function to enable creation of a closed, well-defined polygon.
		 */
		static const unsigned s_min_num_collection_points;


		/**
		 * This is the sequence of polygon segments.
		 */
		seq_type d_seq;

		/**
		 * Useful calculations on the polygon data.
		 *
		 * These calculations are stored directly with the geometry instead of associating
		 * them at a higher level since it's then much easier to query the same geometry
		 * at various places throughout the code (and reuse results of previous queries).
		 * This is made easier by the fact that the geometry data itself is immutable.
		 *
		 * This pointer is NULL until the first calculation is requested.
		 */
		mutable boost::intrusive_ptr<PolygonOnSphereImpl::CachedCalculations> d_cached_calculations;
	};


	/**
	 * Subdivides each segment (great circle arc) of a polygon and returns tessellated polygon.
	 *
	 * Each pair of adjacent points in the tessellated polygon will have a maximum angular extent of
	 * @a max_angular_extent radians.
	 *
	 * Note that those arcs (of the original polygon) already subtending an angle less than
	 * @a max_angular_extent radians will not be tessellated.
	 *
	 * Note that the distance between adjacent points in the tessellated polygon will not be *uniform*.
	 * This is because each arc in the original polygon is tessellated to the nearest integer number
	 * of points and hence each original arc will have a slightly different tessellation angle.
	 */
	PolygonOnSphere::non_null_ptr_to_const_type
	tessellate(
			const PolygonOnSphere &polygon,
			const real_t &max_angular_extent);
}

//
// Implementation
//

namespace GPlatesMaths
{
	template<typename ForwardIter>
	PolygonOnSphere::ConstructionParameterValidity
	PolygonOnSphere::evaluate_construction_parameter_validity(
			ForwardIter begin,
			ForwardIter end,
			std::pair<ForwardIter, ForwardIter> &invalid_points,
			bool check_distinct_points)
	{
		unsigned int num_points = 0;
		if (check_distinct_points)
		{
			num_points = count_distinct_adjacent_points(begin, end);
			// Don't forget that the polygon "wraps around" from the last point to the first.
			// This 'count_distinct_adjacent_points' doesn't consider the first and last points
			// of the sequence to be adjacent, but we do.  Hence, if the first and last points
			// aren't distinct, that means there's one less "distinct adjacent point".
			if (std::distance(begin, end) >= 2) {
				// It's valid (and worth-while) to invoke the 'front' and 'back' accessors
				// of the container.
				const PointOnSphere &first = *begin;
				ForwardIter last_iter = end;
				--last_iter;
				const PointOnSphere &last = *last_iter;
				if (first == last) {
					// A-HA!
					--num_points;
				}
			}
		}
		else
		{
			num_points = std::distance(begin, end);
		}

		if (num_points < s_min_num_collection_points)
		{
			// The collection does not contain enough distinct points to create a
			// closed, well-defined polygon.
			return INVALID_INSUFFICIENT_DISTINCT_POINTS;
		}

		// This for-loop is identical to the corresponding code in PolylineOnSphere.
		ForwardIter prev;
		ForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++) {
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v = evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly
			// enabled by "-Wall"), will help to ensure that no cases are missed.
			switch (v) {

			case VALID:

				// Keep looping.
				break;

			case INVALID_INSUFFICIENT_DISTINCT_POINTS:

				// This value should never be returned, since it's not related to
				// segments.
				//
				// FIXME:  This sucks.  We should separate "global" errors (like
				// "insufficient distinct points") from "segment" errors (like
				// "antipodal segment endpoints").
				break;

			case INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:

				invalid_points.first = prev;
				invalid_points.second = iter;
				return v;
			}
		}

		// Now, an additional check, for the last->first point wrap-around.
		iter = begin;
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v = evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly
			// enabled by "-Wall"), will help to ensure that no cases are missed.
			switch (v) {

			case VALID:

				// Exit the switch-statement.
				break;

			case INVALID_INSUFFICIENT_DISTINCT_POINTS:

				// This value shouldn't be returned.
				// FIXME:  Can this be checked at compile-time?
				// (Perhaps with use of template magic, to
				// avoid the need to check at run-time and
				// throw an exception if the assertion fails.)
				break;

			case INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:

				invalid_points.first = prev;
				invalid_points.second = iter;
				return v;
			}
		}

		// If we got this far, we couldn't find anything wrong with the
		// construction parameters.
		return VALID;
	}


	template<typename ForwardIter>
	const PolygonOnSphere::non_null_ptr_to_const_type
	PolygonOnSphere::create_on_heap(
			ForwardIter begin,
			ForwardIter end,
			bool check_distinct_points)
	{
		PolygonOnSphere::non_null_ptr_type ptr(
				new PolygonOnSphere(),
				GPlatesUtils::NullIntrusivePointerHandler());
		generate_segments_and_swap(*ptr, begin, end, check_distinct_points);
		return ptr;
	}


	/**
	 * The exception thrown when an attempt is made to create a polygon using invalid points.
	 */
	class InvalidPointsForPolygonConstructionError:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		/**
		 * Instantiate the exception.
		 *
		 * @param cpv is the polygon's construction parameter validity value, which
		 * presumably describes why the points are invalid.
		 */
		InvalidPointsForPolygonConstructionError(
				const GPlatesUtils::CallStack::Trace &exception_source,
				PolygonOnSphere::ConstructionParameterValidity cpv) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_cpv(cpv),
			d_filename(exception_source.get_filename()),
			d_line_num(exception_source.get_line_num())
		{  }

		virtual
		~InvalidPointsForPolygonConstructionError() throw()
		{  }

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "InvalidPointsForPolygonConstructionError";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		PolygonOnSphere::ConstructionParameterValidity d_cpv;
		const char *d_filename;
		int d_line_num;
	};


	template<typename ForwardIter>
	void
	PolygonOnSphere::generate_segments_and_swap(
			PolygonOnSphere &poly,
	 		ForwardIter begin,
			ForwardIter end,
			bool check_distinct_points)
	{
		std::pair<ForwardIter, ForwardIter> invalid_points;
		ConstructionParameterValidity v =
				evaluate_construction_parameter_validity(
						begin, end,
						invalid_points,
						check_distinct_points);
		if (v != VALID)
		{
			throw InvalidPointsForPolygonConstructionError(GPLATES_EXCEPTION_SOURCE, v);
		}

		// Make it easier to provide strong exception safety by appending the new segments
		// to a temporary sequence (rather than putting them directly into 'd_seq').
		seq_type tmp_seq;
		// Observe that the number of points used to define a polygon (which will become
		// the number of vertices in the polygon) is also the number of segments in the
		// polygon.
		tmp_seq.reserve(std::distance(begin, end));

		// This for-loop is identical to the corresponding code in PolylineOnSphere.
		ForwardIter prev;
		ForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++)
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;
			tmp_seq.push_back(GreatCircleArc::create(p1, p2));
		}
		// Now, an additional step, for the last->first point wrap-around.
		iter = begin;
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;
			tmp_seq.push_back(GreatCircleArc::create(p1, p2));
		}
		poly.d_seq.swap(tmp_seq);
	}
}

namespace std
{
	/**
	 * This is a template specialisation of the standard function @swap.
	 *
	 * See Josuttis, section 4.4.2, "Swapping Two Values", for more information.
	 */
	template<>
	inline
	void
	swap<GPlatesMaths::PolygonOnSphere>(
			GPlatesMaths::PolygonOnSphere &p1,
			GPlatesMaths::PolygonOnSphere &p2)
	{
		p1.swap(p2);
	}
}

#endif  // GPLATES_MATHS_POLYGONONSPHERE_H
