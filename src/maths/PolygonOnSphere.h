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
#include <algorithm> 
#include <utility>  // std::pair
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "GeometryOnSphere.h"
#include "GreatCircleArc.h"
#include "PolygonOrientation.h"
#include "PolylineOnSphere.h"

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
	 * Internally, this is stored as a sequence of GreatCircleArc for the polygon exterior and optional
	 * sequences for interior rings (holes). You can iterate over this sequence of GreatCircleArc in the usual manner
	 * using the const_iterators returned by the functions @a begin and
	 * @a end.
	 *
	 * You can also iterate over the @em vertices of the polygon using the
	 * vertex_const_iterators returned by the functions @a exterior_ring_vertex_begin and
	 * @a exterior_ring_vertex_end.  For instance, to copy all the vertices of a polygon
	 * into a list of PointOnSphere, you would use the code snippet:
	 * @code
	 * std::list< PointOnSphere > the_list(polygon.exterior_ring_vertex_begin(),
	 *                                     polygon.exterior_ring_vertex_end());
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
	 * @a exterior_begin and @a exterior_end will return these 4 segments.)
	 * If you subsequently iterate through the exterior vertices of this polygon,
	 * you will get the same sequence of points back again: A, B, C, D.
	 *
	 * Note that PolygonOnSphere does *not* have mutators (non-const member functions
	 * which enable the modification of the class internals).
	 */
	class PolygonOnSphere:
			public GeometryOnSphere
	{
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PolygonOnSphere>.
		 *
		 * Note that this typedef is indeed meant to be private.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PolygonOnSphere> non_null_ptr_type;

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere> non_null_ptr_to_const_type;


		/**
		 * The type of the sequence of great circle arcs that form a closed ring.
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
		typedef std::vector<GreatCircleArc> ring_type;

		/**
		 * The type used to const-iterate over the sequence of arcs in a closed ring.
		 */
		typedef ring_type::const_iterator ring_const_iterator;


		/**
		 * Typedef for a sequence of rings.
		 */
		typedef std::vector<ring_type> ring_sequence_type;

		/**
		 * Typedef for a const iterator over @a ring_sequence_type.
		 */
		typedef ring_sequence_type::const_iterator ring_sequence_const_iterator;


		/**
		 * This class enables const_iteration over vertices in an exterior or interior ring of PolygonOnSphere.
		 *
		 * An instance of this class @em actually iterates over the sequence of GreatCircleArc by which
		 * a PolygonOnSphere is implemented, but it pretends it's iterating over a sequence of PointOnSphere.
		 */
		class RingVertexConstIterator :
				public boost::iterator_adaptor<
						RingVertexConstIterator,
						ring_const_iterator,
						const PointOnSphere>
		{
		public:

			/**
			 * Default-construct a vertex iterator (mandated by iterator interface).
			 */
			RingVertexConstIterator() :
				iterator_adaptor_(ring_const_iterator())
			{  }

			/**
			 * Construct a vertex iterator from a ring iterator.
			 */
			explicit
			RingVertexConstIterator(
					const ring_const_iterator &ring_const_iter) :
				iterator_adaptor_(ring_const_iter)
			{  }

		private:

			const PointOnSphere &
			dereference() const
			{
				return base_reference()->start_point();
			}

			// Give access to boost::iterator_adaptor.
			friend class boost::iterator_core_access;
		};

		/**
		 * The type used to const_iterate over the vertices in a ring.
		 */
		typedef RingVertexConstIterator ring_vertex_const_iterator;


		/**
		 * The type used to const_iterate over the vertices in a ring as if it was a polyline.
		 *
		 * This means the last vertex iterated over is the end vertex of the last segment in the ring
		 * (which is also the first vertex in the ring).
		 */
		typedef PolylineOnSphere::VertexConstIterator<ring_const_iterator> polyline_vertex_const_iterator;


		/**
		 * This class enables const_iteration over *all* arcs of PolygonOnSphere.
		 *
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 */
		class ConstIterator :
				public boost::iterator_facade<
						ConstIterator,
						const GreatCircleArc,
						// Keep the iterator as "random access" so that std::advance can do fast indexing...
						std::random_access_iterator_tag>
		{
		public:

			/**
			 * Create the "begin" iterator over *all* arcs in @a polygon.
			 */
			static
			ConstIterator
			create_begin(
					const PolygonOnSphere &polygon)
			{
				return ConstIterator(polygon, 0/*exterior ring id*/, polygon.d_exterior_ring.begin());
			}

			/**
			 * Create the "end" iterator over *all* arcs in @a polygon.
			 */
			static
			ConstIterator
			create_end(
					const PolygonOnSphere &polygon)
			{
				return ConstIterator(
						polygon,
						polygon.d_interior_rings.size(), // Id of last ring (either exterior, or interior if any).
						// "End" ring is either the exterior ring (if no interior rings), or the last interior ring.
						// And "end" ring iterator is the end of that ring.
						(polygon.d_interior_rings.empty()
								? polygon.d_exterior_ring
								: polygon.d_interior_rings.back()
								).end());
			}


			/**
			 * Default-construct an iterator (mandated by iterator interface).
			 */
			ConstIterator() :
				d_polygon_ptr(NULL),
				d_current_ring_id(0),
				d_current_ring_ptr(NULL),
				d_current_ring_iter(ring_const_iterator())
			{  }

		private:

			const PolygonOnSphere *d_polygon_ptr;

			/**
			 * Ring identifier.
			 *
			 * 0 is the exterior ring and 1, 2, ... are the interior rings.
			 */
			unsigned int d_current_ring_id;

			/**
			 * The current ring (associated with @a d_current_ring_id).
			 */
			const ring_type *d_current_ring_ptr;

			/**
			 * Current iterator into the current ring.
			 */
			ring_const_iterator d_current_ring_iter;


			/**
			 * Construct a ConstIterator instance to iterate over *all* arcs of @a polygon.
			 *
			 * Should only be invoked by @a create_begin and @a create_end.
			 */
			ConstIterator(
					const PolygonOnSphere &polygon,
					unsigned int current_ring_id,
					const ring_const_iterator &current_ring_iter) :
				d_polygon_ptr(&polygon),
				d_current_ring_id(current_ring_id),
				d_current_ring_ptr(&(
					current_ring_id == 0
						? polygon.d_exterior_ring
						: polygon.d_interior_rings[current_ring_id - 1])),
				d_current_ring_iter(current_ring_iter)
			{  }

			/**
			 * Iterator dereference - for boost::iterator_facade.
			 *
			 * Returns the currently-pointed-at GreatCircleArc.
			 */
			const GreatCircleArc &
			dereference() const;

			/**
			 * Iterator increment - for boost::iterator_facade.
			 */
			void
			increment();

			/**
			 * Iterator decrement - for boost::iterator_facade.
			 */
			void
			decrement();

			/**
			 * Iterator equality comparison - for boost::iterator_facade.
			 */
			bool
			equal(
					const ConstIterator &other) const;

			/**
			 * Iterator advancement - for boost::iterator_facade.
			 */
			void
			advance(
					ConstIterator::difference_type n);

			/**
			 * Distance between two iterators - for boost::iterator_facade.
			 */
			ConstIterator::difference_type
			distance_to(
					const ConstIterator &other) const;


			// Give access to boost::iterator_adaptor.
			friend class boost::iterator_core_access;
		};

		/**
		 * The type used to const_iterate over *all* arcs.
		 *
		 * Iteration starts with the exterior ring and continues with interior rings, in that order.
		 */
		typedef ConstIterator const_iterator;


		/**
		 * This class enables const_iteration over vertices in *all* arcs of PolygonOnSphere (exterior and interior).
		 *
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 *
		 * An instance of this class @em actually iterates over the sequence of GreatCircleArc by which
		 * a PolygonOnSphere is implemented, but it pretends it's iterating over a sequence of PointOnSphere.
		 */
		class VertexConstIterator :
				public boost::iterator_adaptor<
						VertexConstIterator,
						const_iterator,
						const PointOnSphere>
		{
		public:

			/**
			 * Default-construct a vertex iterator (mandated by iterator interface).
			 */
			VertexConstIterator() :
				iterator_adaptor_(const_iterator())
			{  }

			/**
			 * Construct a vertex iterator from a segment iterator.
			 */
			explicit
			VertexConstIterator(
					const const_iterator &const_iter) :
				iterator_adaptor_(const_iter)
			{  }

		private:

			const PointOnSphere &
			dereference() const
			{
				return base_reference()->start_point();
			}

			// Give access to boost::iterator_adaptor.
			friend class boost::iterator_core_access;
		};

		/**
		 * The type used to const_iterate over vertices in *all* arcs of PolygonOnSphere (exterior and interior).
		 */
		typedef VertexConstIterator vertex_const_iterator;


		/**
		 * Typedef for the bounding tree of great circle arcs within a ring.
		 */
		typedef PolyGreatCircleArcBoundingTree<ring_const_iterator, true/*RequireRandomAccessIterator*/> ring_bounding_tree_type;

		/**
		 * Typedef for the bounding tree of *all* great circle arcs in polygon.
		 *
		 * NOTE: This means all segments in the exterior and interior rings.
		 */
		typedef PolyGreatCircleArcBoundingTree<const_iterator, true/*RequireRandomAccessIterator*/> bounding_tree_type;


		/**
		 * The possible return values from the construction-parameter validation function
		 * @a evaluate_construction_parameter_validity.
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
		 * (eg, your collection of exterior points in the range
		 * [@a exterior_begin, @a exterior_end) ).
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
		 * The half-open range [@a exterior_begin, @a exterior_end) should contain PointOnSphere objects.
		 *
		 * If @a check_distinct_points is 'true' then the sequence of points
		 * is validated for insufficient *distinct* points, otherwise it is validated
		 * for insufficient points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 */
		template <typename PointForwardIter>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				PointForwardIter exterior_begin,
				PointForwardIter exterior_end,
				bool check_distinct_points = false);

		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * @a exterior_points should be a sequential STL container (list, vector, ...) of PointOnSphere.
		 */
		template <typename PointCollectionType>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const PointCollectionType &exterior_points,
				bool check_distinct_points = false)
		{
			return evaluate_construction_parameter_validity(
					exterior_points.begin(), exterior_points.end(), check_distinct_points);
		}


		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * Each interior ring in the range @a interior_rings_begin / @a interior_rings_end should be
		 * a sequential STL container (list, vector, ...) of PointOnSphere.
		 */
		template <typename PointForwardIter, typename PointCollectionForwardIter>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				PointForwardIter exterior_begin,
				PointForwardIter exterior_end,
				PointCollectionForwardIter interior_rings_begin,
				PointCollectionForwardIter interior_rings_end,
				bool check_distinct_points = false);

		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * @a exterior_points should be a sequential STL container (list, vector, ...) of PointOnSphere.
		 *
		 * Each interior ring in the range @a interior_rings_begin / @a interior_rings_end should be
		 * a sequential STL container (list, vector, ...) of PointOnSphere.
		 */
		template <typename PointCollectionType, typename PointCollectionForwardIter>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const PointCollectionType &exterior_points,
				PointCollectionForwardIter interior_rings_begin,
				PointCollectionForwardIter interior_rings_end,
				bool check_distinct_points = false)
		{
			return evaluate_construction_parameter_validity(
					exterior_points.begin(), exterior_points.end(),
					interior_rings_begin, interior_rings_end,
					check_distinct_points);
		}


		/**
		 * Create a new PolygonOnSphere instance on the heap from the sequence of exterior points
		 * in the range @a exterior_begin / @a exterior_end.
		 *
		 * If @a check_distinct_points is 'true' then the sequence of points
		 * is validated for insufficient *distinct* points, otherwise it is validated
		 * for insufficient points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (ie, any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 * This flag was added to prevent exceptions being thrown when reconstructing very small polygons
		 * containing only a few points (eg, a polygon with 4 points might contain 3 distinct points when
		 * it's loaded from a file but, due to numerical precision, contain only 2 distinct points after
		 * it is rotated to a new polygon thus raising an exception when one it not really needed
		 * or desired - because the polygon was good enough to load into GPlates therefore any
		 * rotation of it should also be successful).
		 *
		 * @throws InvalidPointsForPolygonConstructionError if there are insufficient points in the polygon exterior.
		 * If @a check_distinct_points is true then the number of *distinct* points is counted,
		 * otherwise the number of *indistinct* points (ie, the total number of points) is counted.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointForwardIter>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				PointForwardIter exterior_begin,
				PointForwardIter exterior_end,
				bool check_distinct_points = false);

		/**
		 * Create a new PolygonOnSphere instance on the heap from a sequence of exterior points in @a exterior_points.
		 *
		 * @a exterior_points should be a sequential STL container (list, vector, ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointCollectionType>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				const PointCollectionType &exterior_points,
				bool check_distinct_points = false)
		{
			return create_on_heap(exterior_points.begin(), exterior_points.end(), check_distinct_points);
		}


		/**
		 * Create a new PolygonOnSphere instance on the heap from the sequence of exterior points
		 * in the range @a exterior_begin / @a exterior_end and a sequence of interior rings
		 * (where each ring is a sequence of points).
		 *
		 * Each interior ring in the range @a interior_rings_begin / @a interior_rings_end should be
		 * a sequential STL container (list, vector, ...) of PointOnSphere.
		 *
		 * If @a check_distinct_points is 'true' then the exterior ring and interior rings are each
		 * validated for insufficient *distinct* points, otherwise they are validated for insufficient
		 * points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (ie, any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 * This flag was added to prevent exceptions being thrown when reconstructing very small polygons
		 * containing only a few points (eg, a polygon with 4 points might contain 3 distinct points when
		 * it's loaded from a file but, due to numerical precision, contain only 2 distinct points after
		 * it is rotated to a new polygon thus raising an exception when one it not really needed
		 * or desired - because the polygon was good enough to load into GPlates therefore any
		 * rotation of it should also be successful).
		 *
		 * @throws InvalidPointsForPolygonConstructionError if there are insufficient points in the
		 * exterior ring or any interior rings.
		 * If @a check_distinct_points is true then the number of *distinct* points is counted,
		 * otherwise the number of *indistinct* points (ie, the total number of points) is counted.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointForwardIter, typename PointCollectionForwardIter>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				PointForwardIter exterior_begin,
				PointForwardIter exterior_end,
				PointCollectionForwardIter interior_rings_begin,
				PointCollectionForwardIter interior_rings_end,
				bool check_distinct_points = false);

		/**
		 * Create a new PolygonOnSphere instance on the heap from a sequence of exterior points in
		 * @a exterior_points and a sequence of interior rings (where each ring is a sequence of points).
		 *
		 * @a exterior_points should be a sequential STL container (list, vector, ...) of PointOnSphere.
		 *
		 * Each interior ring in the range @a interior_rings_begin / @a interior_rings_end should be
		 * a sequential STL container (list, vector, ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointCollectionType, typename PointCollectionForwardIter>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				const PointCollectionType &exterior_points,
				PointCollectionForwardIter interior_rings_begin,
				PointCollectionForwardIter interior_rings_end,
				bool check_distinct_points = false)
		{
			return create_on_heap(
					exterior_points.begin(), exterior_points.end(),
					interior_rings_begin, interior_rings_end,
					check_distinct_points);
		}

		/**
		 * Create a new PolygonOnSphere instance on the heap from a sequence of exterior points in
		 * @a exterior_points and a sequence of interior rings in @a interior_rings.
		 *
		 * @a exterior_points should be a sequential STL container (list, vector, ...) of PointOnSphere.
		 *
		 * @a interior_rings should be a sequential STL container (list, vector, ...) of
		 * sequential STL containers (list, vector, ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointCollectionType, typename RingCollectionType>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				const PointCollectionType &exterior_points,
				const RingCollectionType &interior_rings,
				bool check_distinct_points = false)
		{
			return create_on_heap(
					exterior_points.begin(), exterior_points.end(),
					interior_rings.begin(), interior_rings.end(),
					check_distinct_points);
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
			return GeometryOnSphere::non_null_ptr_to_const_type(new PolygonOnSphere(*this));
		}


		/**
		 * Clone this PolygonOnSphere instance, to create a duplicate instance on the heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const non_null_ptr_to_const_type
		clone_as_polygon() const
		{
			return non_null_ptr_to_const_type(new PolygonOnSphere(*this));
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
		 * Return the 'begin' const_iterator over *all* GreatCircleArc of this polygon.
		 *
		 * NOTE: This means all segments in the exterior and interior rings.
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 */
		const_iterator
		begin() const
		{
			return const_iterator::create_begin(*this);
		}

		/**
		 * Return the 'end' const_iterator over *all* GreatCircleArc of this polygon.
		 *
		 * NOTE: This means all segments in the exterior and interior rings.
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 */
		const_iterator
		end() const
		{
			return const_iterator::create_end(*this);
		}

		/**
		 * Return the const_iterator in this polygon at the specified segment index (which can be in
		 * an exterior or interior ring), so can iterate over GreatCircleArc starting at that segment.
		 *
		 * NOTE: This can iterate over *all* segments in exterior and interior rings.
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 *
		 * @a segment_index can be one past the last segment, corresponding to @a end.
		 */
		const_iterator
		segment_iterator(
				unsigned int segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					segment_index <= number_of_segments(),
					GPLATES_ASSERTION_SOURCE);

			const_iterator segment_iter = begin();
			// This should be fast since iterator type is random access...
			std::advance(segment_iter, segment_index);

			return segment_iter;
		}

		/**
		 * Return the number of segments in this polygon.
		 *
		 * NOTE: This includes all segments in the exterior and interior rings.
		 */
		unsigned int
		number_of_segments() const
		{
			// Exterior ring.
			unsigned int num_segments = d_exterior_ring.size();

			// Interior rings.
			ring_sequence_const_iterator ring_seq_iter = d_interior_rings.begin();
			ring_sequence_const_iterator ring_seq_end = d_interior_rings.end();
			for ( ; ring_seq_iter != ring_seq_end; ++ring_seq_iter)
			{
				num_segments += ring_seq_iter->size();
			}

			return num_segments;
		}

		/**
		 * Return the segment in this polygon at the specified index.
		 *
		 * NOTE: @a segment_index can reference an exterior or interior ring segment, and as such
		 * can be greater than or equal to @a number_of_segments_in_exterior_ring.
		 */
		const GreatCircleArc &
		get_segment(
				unsigned int segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					segment_index < number_of_segments(),
					GPLATES_ASSERTION_SOURCE);

			const_iterator segment_iter = begin();
			// This should be fast since iterator type is random access...
			std::advance(segment_iter, segment_index);

			return *segment_iter;
		}


		/**
		 * Return the 'begin' vertex_const_iterator over *all* vertices of this polygon.
		 *
		 * NOTE: This means all vertices in the exterior and interior rings.
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 */
		vertex_const_iterator
		vertex_begin() const
		{
			return vertex_const_iterator(begin());
		}

		/**
		 * Return the 'end' vertex_const_iterator over *all* vertices of this polygon.
		 *
		 * NOTE: This means all vertices in the exterior and interior rings.
		 * Iteration starts with the exterior ring and continues with the interior rings, in that order.
		 */
		vertex_const_iterator
		vertex_end() const
		{
			return vertex_const_iterator(end());
		}

		/**
		 * Return the vertex_const_iterator in this polygon at the specified vertex index
		 * (which can be in an exterior or interior ring).
		 *
		 * NOTE: @a vertex_index can reference an exterior or interior ring vertex, and as such
		 * can be greater than or equal to @a number_of_vertices_in_exterior_ring.
		 *
		 * @a vertex_index can be one past the last vertex, corresponding to @a vertex_end.
		 */
		vertex_const_iterator
		vertex_iterator(
				unsigned int vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					vertex_index <= number_of_vertices(),
					GPLATES_ASSERTION_SOURCE);

			vertex_const_iterator vertex_iter = vertex_begin();
			// This should be fast since iterator type is random access...
			std::advance(vertex_iter, vertex_index);

			return vertex_iter;
		}

		/**
		 * Return the number of vertices in this polygon.
		 *
		 * NOTE: This includes all vertices in the exterior and interior rings.
		 */
		unsigned int
		number_of_vertices() const
		{
			return number_of_segments();
		}

		/**
		 * Return the vertex in this polygon at the specified index.
		 *
		 * NOTE: @a vertex_index can reference an exterior or interior ring vertex, and as such
		 * can be greater than or equal to @a number_of_vertices_in_exterior_ring.
		 */
		const PointOnSphere &
		get_vertex(
				unsigned int vertex_index) const
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
		 * Return the 'begin' ring_const_iterator over the sequence of GreatCircleArc
		 * which defines the exterior of this polygon.
		 */
		ring_const_iterator
		exterior_ring_begin() const
		{
			return d_exterior_ring.begin();
		}

		/**
		 * Return the 'end' ring_const_iterator over the sequence of GreatCircleArc
		 * which defines the exterior of this polygon.
		 */
		ring_const_iterator
		exterior_ring_end() const
		{
			return d_exterior_ring.end();
		}

		/**
		 * Return the ring_const_iterator at the specified segment index in the exterior ring.
		 *
		 * @a exterior_segment_index can be one past the last segment, corresponding to @a exterior_ring_end.
		 */
		ring_const_iterator
		exterior_ring_iterator(
				unsigned int exterior_segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					exterior_segment_index <= number_of_segments_in_exterior_ring(),
					GPLATES_ASSERTION_SOURCE);

			ring_const_iterator exterior_segment_iter = exterior_ring_begin();
			// This should be fast since iterator type is random access...
			std::advance(exterior_segment_iter, exterior_segment_index);

			return exterior_segment_iter;
		}

		/**
		 * Return the exterior ring of this polygon.
		 */
		const ring_type &
		exterior_ring() const
		{
			return d_exterior_ring;
		}

		/**
		 * Return the number of segments in the exterior ring in this polygon.
		 */
		unsigned int
		number_of_segments_in_exterior_ring() const
		{
			return d_exterior_ring.size();
		}

		/**
		 * Return the exterior segment in this polygon at the specified index.
		 */
		const GreatCircleArc &
		get_exterior_ring_segment(
				unsigned int exterior_segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					exterior_segment_index < number_of_segments_in_exterior_ring(),
					GPLATES_ASSERTION_SOURCE);

			return d_exterior_ring[exterior_segment_index];
		}


		/**
		 * Return the 'begin' ring_vertex_const_iterator over the exterior vertices of this polygon.
		 */
		ring_vertex_const_iterator
		exterior_ring_vertex_begin() const
		{
			return ring_vertex_const_iterator(d_exterior_ring.begin());
		}

		/**
		 * Return the 'end' ring_vertex_const_iterator over the exterior vertices of this polygon.
		 */
		ring_vertex_const_iterator
		exterior_ring_vertex_end() const
		{
			return ring_vertex_const_iterator(d_exterior_ring.end());
		}

		/**
		 * Return the ring_vertex_const_iterator at the specified vertex index in the exterior ring.
		 *
		 * @a exterior_vertex_index can be one past the last vertex, corresponding to @a exterior_ring_vertex_end.
		 */
		ring_vertex_const_iterator
		exterior_ring_vertex_iterator(
				unsigned int exterior_vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					exterior_vertex_index <= number_of_vertices_in_exterior_ring(),
					GPLATES_ASSERTION_SOURCE);

			ring_vertex_const_iterator exterior_vertex_iter = exterior_ring_vertex_begin();
			// This should be fast since iterator type is random access...
			std::advance(exterior_vertex_iter, exterior_vertex_index);

			return exterior_vertex_iter;
		}

		/**
		 * Return the number of vertices in the exterior ring in this polygon.
		 */
		unsigned int
		number_of_vertices_in_exterior_ring() const
		{
			return number_of_segments_in_exterior_ring();
		}

		/**
		 * Return the exterior vertex in this polygon at the specified index.
		 */
		const PointOnSphere &
		get_exterior_ring_vertex(
				unsigned int exterior_vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					exterior_vertex_index < number_of_vertices_in_exterior_ring(),
					GPLATES_ASSERTION_SOURCE);

			ring_vertex_const_iterator exterior_vertex_iter = exterior_ring_vertex_begin();
			// This should be fast since iterator type is random access...
			std::advance(exterior_vertex_iter, exterior_vertex_index);

			return *exterior_vertex_iter;
		}

		/**
		 * Return the first exterior vertex in this polygon.
		 *
		 * This is the first point specified as an exterior point in the polygon.
		 */
		const PointOnSphere &
		first_exterior_ring_vertex() const
		{
			const GreatCircleArc &first_exterior_gca = *(exterior_ring_begin());
			return first_exterior_gca.start_point();
		}

		/**
		 * Return the last exterior vertex in this polygon.
		 *
		 * This is the last point specified as an exterior point in the polygon.  It is presumably
		 * different to the start-point.  (FIXME: We should ensure this at creation time.)
		 */
		const PointOnSphere &
		last_exterior_ring_vertex() const
		{
			const GreatCircleArc &last_exterior_gca = *(--(exterior_ring_end()));
			return last_exterior_gca.end_point();
		}


		/**
		 * Return the 'begin' polyline_vertex_const_iterator over the exterior ring as if it was a polyline.
		 *
		 * NOTE: This iterates over one extra vertex compared to ring_vertex_const_iterator since
		 * it treats the sequence of GreatCircleArc in the ring as a polyline and hence the last
		 * vertex is the end point of the last ring segment (which is also the first vertex of ring).
		 */
		polyline_vertex_const_iterator
		exterior_polyline_vertex_begin() const
		{
			return polyline_vertex_const_iterator::create_begin(d_exterior_ring.begin());
		}

		/**
		 * Return the 'end' polyline_vertex_const_iterator over the exterior ring as if it was a polyline.
		 *
		 * NOTE: This iterates over one extra vertex compared to ring_vertex_const_iterator since
		 * it treats the sequence of GreatCircleArc in the ring as a polyline and hence the last
		 * vertex is the end point of the last ring segment (which is also the first vertex of ring).
		 */
		polyline_vertex_const_iterator
		exterior_polyline_vertex_end() const
		{
			return polyline_vertex_const_iterator::create_end(d_exterior_ring.begin(), d_exterior_ring.end());
		}

		/**
		 * Return the polyline_vertex_const_iterator at the specified vertex index in the exterior ring
		 * behaving as if it was a polyline.
		 *
		 * @a exterior_vertex_index can be one past the last vertex, corresponding to @a exterior_polyline_vertex_end.
		 *
		 * NOTE: This iterates over one extra vertex compared to ring_vertex_const_iterator since
		 * it treats the sequence of GreatCircleArc in the ring as a polyline and hence the last
		 * vertex is the end point of the last ring segment (which is also the first vertex of ring).
		 */
		polyline_vertex_const_iterator
		exterior_polyline_vertex_iterator(
				unsigned int exterior_vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					exterior_vertex_index <= number_of_vertices_in_exterior_polyline(),
					GPLATES_ASSERTION_SOURCE);

			polyline_vertex_const_iterator exterior_vertex_iter = exterior_polyline_vertex_begin();
			// This should be fast since iterator type is random access...
			std::advance(exterior_vertex_iter, exterior_vertex_index);

			return exterior_vertex_iter;
		}

		/**
		 * Return the number of vertices in the exterior ring as if it was a polyline.
		 *
		 * NOTE: There is one extra vertex compared to the exterior ring since we treat the sequence
		 * of GreatCircleArc in the ring as a polyline and hence the last vertex is the end point of the
		 * last ring segment (which is also the first vertex of ring).
		 */
		unsigned int
		number_of_vertices_in_exterior_polyline() const
		{
			return number_of_vertices_in_exterior_ring() + 1;
		}


		/**
		 * Return the "begin" const iterator over the interior rings of this polygon.
		 *
		 * Each interior ring has type @a ring_type.
		 */
		ring_sequence_const_iterator
		interior_rings_begin() const
		{
			return d_interior_rings.begin();
		}

		/**
		 * Return the "end" const iterator over the interior rings of this polygon.
		 *
		 * Each interior ring has type @a ring_type.
		 */
		ring_sequence_const_iterator
		interior_rings_end() const
		{
			return d_interior_rings.end();
		}
		
		/**
		 * Return the sequence of interior rings of this polygon.
		 *
		 * Each interior ring has type @a ring_type.
		 */
		const ring_sequence_type &
		interior_rings() const
		{
			return d_interior_rings;
		}

		/**
		 * Return the number of interior rings in this polygon.
		 */
		unsigned int
		number_of_interior_rings() const
		{
			return d_interior_rings.size();
		}

		/**
		 * Return the 'begin' ring_const_iterator over the sequence of GreatCircleArc
		 * which defines the interior ring of this polygon at the specified interior ring index.
		 */
		ring_const_iterator
		interior_ring_begin(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			return d_interior_rings[interior_ring_index].begin();
		}

		/**
		 * Return the 'end' ring_const_iterator over the sequence of GreatCircleArc
		 * which defines the interior ring of this polygon at the specified interior ring index.
		 */
		ring_const_iterator
		interior_ring_end(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			return d_interior_rings[interior_ring_index].end();
		}

		/**
		 * Return the ring_const_iterator at the specified segment index in the specified interior ring.
		 *
		 * @a segment_index can be one past the last segment, corresponding to @a interior_ring_end.
		 */
		ring_const_iterator
		interior_ring_iterator(
				unsigned int interior_ring_index,
				unsigned int segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			const ring_type &interior_ring = d_interior_rings[interior_ring_index];

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					segment_index <= interior_ring.size(),
					GPLATES_ASSERTION_SOURCE);

			ring_const_iterator interior_segment_iter = interior_ring.begin();
			// This should be fast since iterator type is random access...
			std::advance(interior_segment_iter, segment_index);

			return interior_segment_iter;
		}

		/**
		 * Return the number of segments in the interior ring in this polygon at the specified interior ring index.
		 */
		unsigned int
		number_of_segments_in_interior_ring(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			return d_interior_rings[interior_ring_index].size();
		}

		/**
		 * Return the segment of the interior ring in this polygon at the specified segment index
		 * in the specified interior ring index.
		 */
		const GreatCircleArc &
		get_interior_ring_segment(
				unsigned int interior_ring_index,
				unsigned int segment_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			const ring_type &interior_ring = d_interior_rings[interior_ring_index];

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					segment_index < interior_ring.size(),
					GPLATES_ASSERTION_SOURCE);

			return interior_ring[segment_index];
		}


		/**
		 * Return the 'begin' ring_vertex_const_iterator over the vertices of the interior ring
		 * of this polygon at the specified interior ring index.
		 */
		ring_vertex_const_iterator
		interior_ring_vertex_begin(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);
			return ring_vertex_const_iterator(d_interior_rings[interior_ring_index].begin());
		}

		/**
		 * Return the 'end' ring_vertex_const_iterator over the vertices of the interior ring
		 * of this polygon at the specified interior ring index.
		 */
		ring_vertex_const_iterator
		interior_ring_vertex_end(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);
			return ring_vertex_const_iterator(d_interior_rings[interior_ring_index].end());
		}

		/**
		 * Return the ring_vertex_const_iterator at the specified vertex index in the specified interior ring.
		 *
		 * @a vertex_index can be one past the last vertex, corresponding to @a interior_ring_vertex_end.
		 */
		ring_vertex_const_iterator
		interior_ring_vertex_iterator(
				unsigned int interior_ring_index,
				unsigned int vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			const ring_type &interior_ring = d_interior_rings[interior_ring_index];

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					// A ring contains same number of vertices and segments,
					// so we can compare with number of segments in ring...
					vertex_index <= interior_ring.size(),
					GPLATES_ASSERTION_SOURCE);

			ring_vertex_const_iterator interior_vertex_iter(interior_ring.begin());
			// This should be fast since iterator type is random access...
			std::advance(interior_vertex_iter, vertex_index);

			return interior_vertex_iter;
		}

		/**
		 * Return the number of vertices in the interior ring in this polygon
		 * at the specified interior ring index.
		 */
		unsigned int
		number_of_vertices_in_interior_ring(
				unsigned int interior_ring_index) const
		{
			return number_of_segments_in_interior_ring(interior_ring_index);
		}

		/**
		 * Return the vertex of the interior ring in this polygon at the specified vertex index
		 * in the specified interior ring index.
		 */
		const PointOnSphere &
		get_interior_ring_vertex(
				unsigned int interior_ring_index,
				unsigned int vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			const ring_type &interior_ring = d_interior_rings[interior_ring_index];

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					// A ring contains same number of vertices and segments,
					// so we can compare with number of segments in ring...
					vertex_index < interior_ring.size(),
					GPLATES_ASSERTION_SOURCE);

			ring_vertex_const_iterator interior_vertex_iter(interior_ring.begin());
			// This should be fast since iterator type is random access...
			std::advance(interior_vertex_iter, vertex_index);

			return *interior_vertex_iter;
		}

		/**
		 * Return the first vertex in the interior ring in this polygon at the specified interior ring index.
		 */
		const PointOnSphere &
		first_interior_ring_vertex(
				unsigned int interior_ring_index) const
		{
			const GreatCircleArc &first_interior_gca = *(interior_ring_begin(interior_ring_index));
			return first_interior_gca.start_point();
		}

		/**
		 * Return the last vertex in the interior ring in this polygon at the specified interior ring index.
		 *
		 * This is the last point specified as an interior point in the polygon at the specified interior ring index.
		 * It is presumably different to the start-point.  (FIXME: We should ensure this at creation time.)
		 */
		const PointOnSphere &
		last_interior_ring_vertex(
				unsigned int interior_ring_index) const
		{
			const GreatCircleArc &last_interior_gca = *(--(interior_ring_end(interior_ring_index)));
			return last_interior_gca.end_point();
		}


		/**
		 * Return the 'begin' polyline_vertex_const_iterator over an interior ring as if it was a polyline.
		 *
		 * NOTE: This iterates over one extra vertex compared to ring_vertex_const_iterator since
		 * it treats the sequence of GreatCircleArc in the ring as a polyline and hence the last
		 * vertex is the end point of the last ring segment (which is also the first vertex of ring).
		 */
		polyline_vertex_const_iterator
		interior_polyline_vertex_begin(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);
			return polyline_vertex_const_iterator::create_begin(d_interior_rings[interior_ring_index].begin());
		}

		/**
		 * Return the 'end' polyline_vertex_const_iterator over an interior ring as if it was a polyline.
		 *
		 * NOTE: This iterates over one extra vertex compared to ring_vertex_const_iterator since
		 * it treats the sequence of GreatCircleArc in the ring as a polyline and hence the last
		 * vertex is the end point of the last ring segment (which is also the first vertex of ring).
		 */
		polyline_vertex_const_iterator
		interior_polyline_vertex_end(
				unsigned int interior_ring_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);
			return polyline_vertex_const_iterator::create_end(
					d_interior_rings[interior_ring_index].begin(),
					d_interior_rings[interior_ring_index].end());
		}

		/**
		 * Return the polyline_vertex_const_iterator at the specified vertex index in an interior ring
		 * behaving as if it was a polyline.
		 *
		 * @a vertex_index can be one past the last vertex, corresponding to @a interior_polyline_vertex_end.
		 *
		 * NOTE: This iterates over one extra vertex compared to ring_vertex_const_iterator since
		 * it treats the sequence of GreatCircleArc in the ring as a polyline and hence the last
		 * vertex is the end point of the last ring segment (which is also the first vertex of ring).
		 */
		polyline_vertex_const_iterator
		interior_polyline_vertex_iterator(
				unsigned int interior_ring_index,
				unsigned int vertex_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					interior_ring_index < number_of_interior_rings(),
					GPLATES_ASSERTION_SOURCE);

			const ring_type &interior_ring = d_interior_rings[interior_ring_index];

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					// A polyline contains one extra vertex compared to number of ring segments/vertices...
					vertex_index <= interior_ring.size() + 1,
					GPLATES_ASSERTION_SOURCE);

			polyline_vertex_const_iterator interior_vertex_iter =
					polyline_vertex_const_iterator::create_begin(d_interior_rings[interior_ring_index].begin());
			// This should be fast since iterator type is random access...
			std::advance(interior_vertex_iter, vertex_index);

			return interior_vertex_iter;
		}

		/**
		 * Return the number of vertices in an interior ring as if it was a polyline.
		 *
		 * NOTE: There is one extra vertex compared to the interior ring since we treat the sequence
		 * of GreatCircleArc in the ring as a polyline and hence the last vertex is the end point of the
		 * last ring segment (which is also the first vertex of ring).
		 */
		unsigned int
		number_of_vertices_in_interior_polyline(
				unsigned int interior_ring_index) const
		{
			return number_of_vertices_in_interior_ring(interior_ring_index) + 1;
		}


		/**
		 * Evaluate whether @a test_point is "close" to this polygon.
		 *
		 * Note: This function tests whether @a test_point is "close" to the polygon @em outline
		 * (where the outline includes the exterior ring and any interior rings). To test if a point
		 * is inside this polygon use @a is_point_in_polygon instead.
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
			return d_exterior_ring == other.d_exterior_ring &&
					d_interior_rings == other.d_interior_rings;
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
		 * Returns the total arc-length of the exterior ring and interior rings (each ring is a
		 * sequences of @a GreatCirclArc.
		 *
		 * The result is in radians and represents the distance on the unit radius sphere.
		 *
		 * The result is cached on first call.
		 */
		real_t
		get_arc_length() const;

		/**
		 * Returns the arc-length of the exterior ring sequence of @a GreatCirclArc which
		 * defines the exterior of this polygon.
		 *
		 * The result is in radians and represents the distance on the unit radius sphere.
		 *
		 * The result is cached on first call.
		 */
		const real_t &
		get_exterior_ring_arc_length() const;

		/**
		 * Returns the arc-length of an interior ring sequence of @a GreatCirclArc for the
		 * specified interior ring index.
		 *
		 * The result is in radians and represents the distance on the unit radius sphere.
		 *
		 * The result is cached on first call.
		 */
		const real_t &
		get_interior_ring_arc_length(
				unsigned int interior_ring_index) const;


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
		 *
		 * If @a use_point_on_polygon_threshold is true then the point is considered *on* the outline
		 * (and hence classified as *inside* the polygon) if it is within an (extremely small) threshold
		 * distance from the polygon's outline. An example where this is useful is avoiding a point
		 * exactly on the dateline not getting included by a polygon that has an edge exactly aligned
		 * with the dateline.
		 * So usually this should be true (the default) and only turned off for some specific cases
		 * (such as the polyline intersections code where a point extremely close to a polygon,
		 * but still outside it, should not be considered inside it).
		 *
		 * The point-in-polygon test is done by counting the number of polygon edges crossed
		 * (in the exterior ring and any interior rings) from the polygon's antipodal centroid to
		 * the point being tested. This means that if an interior ring intersects the exterior ring
		 * then it is possible for a test point, that is outside the exterior ring but inside the
		 * interior ring, to pass the point-in-polygon test. This might not be desirable but it is
		 * similar to how filled polygons are currently rendered and so at least there is some
		 * consistency there (and reconstructed rasters use the point-in-polygon test when generating
		 * a polygon mesh so it's consistent too).
		 */
		bool
		is_point_in_polygon(
				const PointOnSphere &point,
				PointInPolygonSpeedAndMemory speed_and_memory = ADAPTIVE,
				bool use_point_on_polygon_threshold = true) const;


		/**
		 * Returns the centroid of the *edges* of the exterior ring of this polygon
		 * (see @a Centroid::calculate_outline_centroid).
		 *
		 * This does not include the edges of the interior rings and hence is the same as
		 * calling @a get_outline_centroid with 'use_interior_rings' set to false.
		 *
		 * This centroid is useful for the centre of a small circle that bounds this polygon.
		 * Since the interior rings are (usually) inside the exterior ring they're not really needed
		 * for this sort of thing (hence @a get_outline_centroid is not really needed). The interior
		 * rings are more important @a get_interior_centroid which looks at the interior fill region.
		 *
		 * The result is cached on first call.
		 */
		const UnitVector3D &
		get_boundary_centroid() const;


		/**
		 * Returns the centroid of the *edges* of this polygon
		 * (see @a Centroid::calculate_outline_centroid).
		 *
		 * If @a use_interior_rings is true (the default) then the edges of the interior rings are
		 * included. If it is false then this is the same as calling @a get_boundary_centroid.
		 *
		 * The result is cached on first call.
		 */
		const UnitVector3D &
		get_outline_centroid(
				bool use_interior_rings = true) const;


		/**
		 * Returns the centroid of the *interior* of this polygon (see @a Centroid::calculate_interior_centroid).
		 *
		 * This centroid is useful when the centroid should be closer to the centre-of-mass of
		 * the polygon interior.
		 *
		 * This centroid can be considered a centre-of-mass type of centroid since the calculated
		 * centroid is weighted according to the area coverage of the interior region.
		 * For example a bottom-heavy pear-shaped polygon will have an interior centroid closer
		 * to the bottom whereas the @a get_boundary_centroid or @a get_outline_centroid will
		 * be closer to the middle of the pear.
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
		 * The inner bounding small circle contains no exterior ring or interior ring arc segments.
		 *
		 * Note that since the centre of the small circle is @a get_boundary_centroid it could lie
		 * inside or outside the exterior ring or inside/outside any interior rings depending on the
		 * shape of the rings and the location of the centroid. In other words the inner bounding
		 * small circle has nothing to do with being inside the polygon exterior or interior rings.
		 *
		 * The result is cached on first call.
		 */
		const InnerOuterBoundingSmallCircle &
		get_inner_outer_bounding_small_circle() const;


		/**
		 * Returns the small circle bounding tree over *all* great circle arc segments.
		 *
		 * NOTE: This means all segments in the exterior and interior rings.
		 *
		 * The result is cached on first call.
		 */
		const bounding_tree_type &
		get_bounding_tree() const;

		/**
		 * Returns the exterior ring small circle bounding tree over the great circle arc segments
		 * of the exterior ring of this polygon.
		 *
		 * The result is cached on first call.
		 */
		const ring_bounding_tree_type &
		get_exterior_ring_bounding_tree() const;

		/**
		 * Returns the interior ring small circle bounding tree at the specified interior ring index.
		 *
		 * This is the bounding tree over the great circle arc segments of the specified interior ring.
		 *
		 * The result is cached on first call.
		 */
		const ring_bounding_tree_type &
		get_interior_ring_bounding_tree(
				unsigned int interior_ring_index) const;

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


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		PolygonOnSphere &
		operator=(
				const PolygonOnSphere &other);


		/**
		 * Evaluate the validity of the points for use in the creation of a ring.
		 */
		template <typename PointForwardIter>
		static
		ConstructionParameterValidity
		evaluate_ring_validity(
				PointForwardIter begin,
				PointForwardIter end,
				bool check_distinct_points);


		/**
		 * Evaluate the validity of the points @a p1 and @a p2 for use
		 * in the creation of a polygon line-segment.
		 */
		static
		ConstructionParameterValidity
		evaluate_segment_endpoint_validity(
				const PointOnSphere &p1,
				const PointOnSphere &p2);


		/**
		 * Generate a sequence of polygon segments from the exterior points in the range
		 * @a exterior_begin / @a exterior_end.
		 *
		 * These are used to define the vertices of an exterior ring which is then swapped with the exterior
		 * ring in the polygon @a polygon, discarding any exterior ring which may have been there before.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointForwardIter>
		static
		void
		generate_rings_and_swap(
				PolygonOnSphere &polygon,
				PointForwardIter exterior_begin,
				PointForwardIter exterior_end,
				bool check_distinct_points);


		/**
		 * Generate a sequence of polygon segments from the exterior points in the range
		 * @a exterior_begin / @a exterior_end and a sequence of interior rings in the range
		 * @a interior_rings_begin / @a interior_rings_end (where each ring is a sequence of points).
		 *
		 * These are used to define the vertices of an exterior ring and interior rings which are then swapped
		 * with the rings in the polygon @a polygon, discarding any rings which may have been there before.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename PointForwardIter, typename PointCollectionForwardIter>
		static
		void
		generate_rings_and_swap(
				PolygonOnSphere &polygon,
				PointForwardIter exterior_begin,
				PointForwardIter exterior_end,
				PointCollectionForwardIter interior_rings_begin,
				PointCollectionForwardIter interior_rings_end,
				bool check_distinct_points);


		/**
		 * Generate a ring from a sequence of points.
		 */
		template <typename PointForwardIter>
		static
		void
		generate_ring(
				ring_type &ring,
	 			PointForwardIter begin,
				PointForwardIter end);


		/**
		 * This is the minimum number of (distinct) collection points to be passed into the
		 * 'create_on_heap' function to enable creation of a closed, well-defined polygon.
		 */
		static const unsigned s_min_num_collection_points;


		/**
		 * The exterior ring of this polygon.
		 */
		ring_type d_exterior_ring;


		/**
		 * The interior rings of this polygon (if any).
		 *
		 * If any interior rings are present then the polygon is a donut polygon (a polygon with holes).
		 */
		ring_sequence_type d_interior_rings;

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
	template <typename PointForwardIter>
	PolygonOnSphere::ConstructionParameterValidity
	PolygonOnSphere::evaluate_construction_parameter_validity(
			PointForwardIter exterior_begin,
			PointForwardIter exterior_end,
			bool check_distinct_points)
	{
		return evaluate_ring_validity(exterior_begin, exterior_end, check_distinct_points);
	}


	template <typename PointForwardIter, typename PointCollectionForwardIter>
	PolygonOnSphere::ConstructionParameterValidity
	PolygonOnSphere::evaluate_construction_parameter_validity(
			PointForwardIter exterior_begin,
			PointForwardIter exterior_end,
			PointCollectionForwardIter interior_rings_begin,
			PointCollectionForwardIter interior_rings_end,
			bool check_distinct_points)
	{
		const ConstructionParameterValidity exterior_validity =
				evaluate_ring_validity(exterior_begin, exterior_end, check_distinct_points);
		if (exterior_validity != VALID)
		{
			return exterior_validity;
		}

		for (PointCollectionForwardIter interior_rings_iter = interior_rings_begin;
			interior_rings_iter != interior_rings_end;
			++interior_rings_iter)
		{
			const ConstructionParameterValidity interior_validity =
					evaluate_ring_validity(
							interior_rings_iter->begin(),
							interior_rings_iter->end(),
							check_distinct_points);
			if (interior_validity != VALID)
			{
				return interior_validity;
			}
		}

		return VALID;
	}


	template <typename PointForwardIter>
	PolygonOnSphere::ConstructionParameterValidity
	PolygonOnSphere::evaluate_ring_validity(
			PointForwardIter begin,
			PointForwardIter end,
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
			if (std::distance(begin, end) >= 2)
			{
				// It's valid (and worth-while) to invoke the 'front' and 'back' accessors
				// of the container.
				const PointOnSphere &first = *begin;
				PointForwardIter last_iter = end;
				--last_iter;
				const PointOnSphere &last = *last_iter;
				if (first == last)
				{
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
		PointForwardIter prev;
		PointForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++)
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v = evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly
			// enabled by "-Wall"), will help to ensure that no cases are missed.
			switch (v)
			{
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
			switch (v)
			{
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
				return v;
			}
		}

		// If we got this far, we couldn't find anything wrong with the
		// construction parameters.
		return VALID;
	}


	template <typename PointForwardIter>
	const PolygonOnSphere::non_null_ptr_to_const_type
	PolygonOnSphere::create_on_heap(
			PointForwardIter exterior_begin,
			PointForwardIter exterior_end,
			bool check_distinct_points)
	{
		non_null_ptr_type ptr(new PolygonOnSphere());
		generate_rings_and_swap(*ptr, exterior_begin, exterior_end, check_distinct_points);
		return ptr;
	}


	template <typename PointForwardIter, typename PointCollectionForwardIter>
	const PolygonOnSphere::non_null_ptr_to_const_type
	PolygonOnSphere::create_on_heap(
			PointForwardIter exterior_begin,
			PointForwardIter exterior_end,
			PointCollectionForwardIter interior_rings_begin,
			PointCollectionForwardIter interior_rings_end,
			bool check_distinct_points)
	{
		non_null_ptr_type ptr(new PolygonOnSphere());
		generate_rings_and_swap(
				*ptr,
				exterior_begin, exterior_end,
				interior_rings_begin, interior_rings_end,
				check_distinct_points);
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


	template <typename PointForwardIter>
	void
	PolygonOnSphere::generate_rings_and_swap(
			PolygonOnSphere &polygon,
	 		PointForwardIter exterior_begin,
			PointForwardIter exterior_end,
			bool check_distinct_points)
	{
		const ConstructionParameterValidity v =
				evaluate_construction_parameter_validity(
						exterior_begin,
						exterior_end,
						check_distinct_points);
		if (v != VALID)
		{
			throw InvalidPointsForPolygonConstructionError(GPLATES_EXCEPTION_SOURCE, v);
		}

		// Make it easier to provide strong exception safety by appending the new segments
		// to a temporary sequence (rather than putting them directly into 'd_exterior_ring').
		ring_type exterior;
		generate_ring(exterior, exterior_begin, exterior_end);
		polygon.d_exterior_ring.swap(exterior);
	}


	template <typename PointForwardIter, typename PointCollectionForwardIter>
	void
	PolygonOnSphere::generate_rings_and_swap(
			PolygonOnSphere &polygon,
	 		PointForwardIter exterior_begin,
			PointForwardIter exterior_end,
			PointCollectionForwardIter interior_rings_begin,
			PointCollectionForwardIter interior_rings_end,
			bool check_distinct_points)
	{
		const ConstructionParameterValidity exterior_validity =
				evaluate_construction_parameter_validity(
						exterior_begin,
						exterior_end,
						check_distinct_points);
		if (exterior_validity != VALID)
		{
			throw InvalidPointsForPolygonConstructionError(GPLATES_EXCEPTION_SOURCE, exterior_validity);
		}

		// Make it easier to provide strong exception safety by appending to temporary rings
		// and then swapping them into 'polygon'.

		ring_type exterior;
		generate_ring(exterior, exterior_begin, exterior_end);

		ring_sequence_type interiors;
		interiors.resize(std::distance(interior_rings_begin, interior_rings_end));

		unsigned int interior_index = 0;
		for (PointCollectionForwardIter interior_rings_iter = interior_rings_begin;
			interior_rings_iter != interior_rings_end;
			++interior_rings_iter, ++interior_index)
		{
			const ConstructionParameterValidity interior_validity =
					evaluate_construction_parameter_validity(
							interior_rings_iter->begin(),
							interior_rings_iter->end(),
							check_distinct_points);
			if (interior_validity != VALID)
			{
				throw InvalidPointsForPolygonConstructionError(GPLATES_EXCEPTION_SOURCE, interior_validity);
			}

			generate_ring(
					interiors[interior_index],
					interior_rings_iter->begin(),
					interior_rings_iter->end());
		}

		polygon.d_exterior_ring.swap(exterior);
		polygon.d_interior_rings.swap(interiors);
	}


	template <typename PointForwardIter>
	void
	PolygonOnSphere::generate_ring(
			ring_type &ring,
	 		PointForwardIter begin,
			PointForwardIter end)
	{
		// Observe that the number of points used to define a ring (which will become
		// the number of vertices in the ring) is also the number of segments in the ring.
		ring.reserve(std::distance(begin, end));

		// This for-loop is identical to the corresponding code in PolylineOnSphere.
		PointForwardIter prev;
		PointForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++)
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;
			ring.push_back(GreatCircleArc::create(p1, p2));
		}
		// Now, an additional step, for the last->first point wrap-around.
		iter = begin;
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;
			ring.push_back(GreatCircleArc::create(p1, p2));
		}
	}
}

#endif  // GPLATES_MATHS_POLYGONONSPHERE_H
