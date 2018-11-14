/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "PolyLineOnSphere.h")
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
 *  (under the name "PolylineOnSphere.h")
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

#ifndef GPLATES_MATHS_POLYLINEONSPHERE_H
#define GPLATES_MATHS_POLYLINEONSPHERE_H

#include <algorithm>
#include <iterator>  // std::iterator, std::bidirectional_iterator_tag, std::distance
#include <utility>  // std::pair
#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "AngularExtent.h"
#include "GeometryOnSphere.h"
#include "GreatCircleArc.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	// Forward declarations.
	namespace PolylineOnSphereImpl
	{
		struct CachedCalculations;
	}
	class BoundingSmallCircle;

	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	class PolyGreatCircleArcBoundingTree;


	/** 
	 * Represents a polyline on the surface of a sphere. 
	 *
	 * Internally, this is stored as a sequence of GreatCircleArc.  You
	 * can iterate over this sequence of GreatCircleArc in the usual manner
	 * using the const_iterators returned by the functions @a begin and
	 * @a end.
	 *
	 * You can also iterate over the @em vertices of the polyline using the
	 * vertex_const_iterators returned by the functions @a vertex_begin and
	 * @a vertex_end.  For instance, to copy all the vertices of a polyline
	 * into a list of PointOnSphere, you would use the code snippet:
	 * @code
	 * std::list< PointOnSphere > the_list(polyline.vertex_begin(),
	 *                                     polyline.vertex_end());
	 * @endcode
	 *
	 * You can create a polyline by invoking the static member function
	 * @a PolylineOnSphere::create_on_heap, passing it a sequential STL
	 * container (list, vector, ...) of PointOnSphere to define the
	 * vertices of the polyline.  The sequence of points must contain at
	 * least two distinct elements, enabling the creation of a polyline
	 * composed of at least one well-defined segment.  The requirements
	 * upon the sequence of points are described in greater detail in the
	 * comment for the static member function
	 * @a PolylineOnSphere::evaluate_construction_parameter_validity.
	 *
	 * Say you have a sequence of PointOnSphere: [A, B, C, D].  If you pass
	 * this sequence to the @a PolylineOnSphere::create_on_heap function, it
	 * will create a polyline composed of 3 segments: A->B, B->C, C->D.  If
	 * you subsequently iterate through the vertices of this polyline, you
	 * will get the same sequence of points back again: A, B, C, D.
	 *
	 * Note that PolylineOnSphere does *not* have mutators (non-const member functions
	 * which enable the modification of the class internals).
	 */
	class PolylineOnSphere:
			public GeometryOnSphere
	{
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PolylineOnSphere>.
		 * 
		 * Note that this typedef is indeed meant to be private.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PolylineOnSphere> non_null_ptr_type;

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere> non_null_ptr_to_const_type;


		/**
		 * The type of the sequence of great circle arcs.
		 *
		 * Implementation detail:  We are using 'std::vector' as the
		 * sequence type (rather than, say, 'std::list') to provide a
		 * speed-up in memory-allocation (we use 'std::vector::reserve'
		 * at creation time to avoid expensive reallocations as arcs
		 * are appended one-by-one; after that, because the contents of
		 * the sequence are never altered, the size of the vector will
		 * never change), a speed-up in iteration (for what it's worth,
		 * a pointer-increment rather than a 'node = node->next'-style
		 * operation) and a decrease (hopefully) in memory-usage (by
		 * avoiding a whole bunch of unnecessary links).  (We should,
		 * however, be able to get away without relying upon the
		 * "random-access"ness of vector iterators; forward iterators
		 * should be enough.)
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


		/**
		 * This class enables const_iteration over the vertices of a sequence of GreatCircleArc.
		 *
		 * The number of vertices iterated is one larger than the number of GreatCircleArc.
		 *
		 * An instance of this class @em actually iterates over a sequence of GreatCircleArc,
		 * but pretends it's iterating over a sequence of PointOnSphere by additionally keeping track
		 * of whether it's pointing at the "start-point" or "end-point" of the current GreatCircleArc.
		 *
		 * It is assumed that the sequence of GreatCircleArc over which this iterator is iterating
		 * will always contain at least one element (and thus, at least two vertices).
		 * This assumption should be fulfilled by the PolylineOnSphere invariant and
		 * PolygonOnSphere ring invariant.
		 */
		template <typename GreatCircleArcConstIteratorType>
		class VertexConstIterator :
				public boost::iterator_facade<
						VertexConstIterator<GreatCircleArcConstIteratorType>,
						const PointOnSphere,
						// Keep the iterator as "random access" so that std::advance can do fast indexing...
						std::random_access_iterator_tag>
		{
			enum StartOrEnd
			{
				START,
				END
			};

			typedef GreatCircleArcConstIteratorType gca_const_iterator;

		public:

			/**
			 * Create the "begin" vertex iterator for @a poly.
			 *
			 * Note that it's intentional that the instance returned is non-const: If
			 * the instance were const, it would not be possible to write an expression
			 * like '++(polyline.vertex_begin())' to access the second vertex of the
			 * polyline.
			 */
			static
			VertexConstIterator
			create_begin(
					gca_const_iterator begin)
			{
				return VertexConstIterator(begin, begin, START);
			}


			/**
			 * Create the "end" vertex iterator for @a poly.
			 *
			 * Note that it's intentional that the instance returned is non-const: If
			 * the instance were const, it would not be possible to write an expression
			 * like '--(polyline.vertex_end())' to access the last vertex of the
			 * polyline.
			 */
			static
			VertexConstIterator
			create_end(
					gca_const_iterator begin,
					gca_const_iterator end)
			{
				return VertexConstIterator(begin, end, END);
			}


			/**
			 * Default-construct a vertex iterator (mandated by the iterator interface).
			 *
			 * A default-constructed iterator will be uninitialised.
			 *
			 * If you attempt to dereference an uninitialised iterator then the behaviour will
			 * depend on the underlying standard library iterators (specifically for std::vector).
			 */
			VertexConstIterator():
				d_begin_gca(), 
				d_curr_gca(), 
				d_gca_start_or_end(END)
			{  }

		private:

			/**
			 * Construct a VertexConstIterator instance to iterate over the vertices of @a poly.
			 *
			 * The current position of the iterator is described by @a curr_gca_ and @a gca_start_or_end_.
			 * Note that not all combinations of @a curr_gca_ and @a gca_start_or_end_ are valid.
			 *
			 * This constructor should only be invoked by @a create_begin and @a create_end.
			 */
			VertexConstIterator(
					gca_const_iterator begin_gca_,
					gca_const_iterator curr_gca_,
					StartOrEnd gca_start_or_end_) :
				d_begin_gca(begin_gca_),
				d_curr_gca(curr_gca_),
				d_gca_start_or_end(gca_start_or_end_)
			{  }

			/**
			 * Iterator dereference - for boost::iterator_facade.
			 *
			 * This function performs the magic which is used to
			 * obtain the currently-pointed-at PointOnSphere.
			 */
			const PointOnSphere &
			dereference() const
			{
				if (d_curr_gca == d_begin_gca &&
					d_gca_start_or_end == START)
				{
					return d_curr_gca->start_point();
				}
				else
				{
					return d_curr_gca->end_point();
				}
			}

			/**
			 * Iterator increment - for boost::iterator_facade.
			 *
			 * This function performs the magic which is used to increment this iterator.
			 */
			void
			increment()
			{
				if (d_curr_gca == d_begin_gca &&
					d_gca_start_or_end == START)
				{
					d_gca_start_or_end = END;
				}
				else
				{
					++d_curr_gca;
				}
			}

			/**
			 * Iterator decrement - for boost::iterator_facade.
			 *
			 * This function performs the magic which is used to decrement this iterator.
			 */
			void
			decrement()
			{
				if (d_curr_gca == d_begin_gca &&
					d_gca_start_or_end == END)
				{
					d_gca_start_or_end = START;
				}
				else
				{
					--d_curr_gca;
				}
			}

			/**
			 * Iterator equality comparison - for boost::iterator_facade.
			 */
			bool
			equal(
					const VertexConstIterator &other) const
			{
				return d_curr_gca == other.d_curr_gca &&
						d_gca_start_or_end == other.d_gca_start_or_end;
			}

			/**
			 * Iterator advancement - for boost::iterator_facade.
			 */
			void
			advance(
					typename VertexConstIterator::difference_type n)
			{
				if (n > 0)
				{
					if (d_curr_gca == d_begin_gca &&
						d_gca_start_or_end == START)
					{
						// Advance by one.
 						d_gca_start_or_end = END;
 						--n;

						// Advance any remaining amount.
						if (n > 0)
						{
							std::advance(d_curr_gca, n);
						}
					}
					else
					{
						std::advance(d_curr_gca, n);
					}
				}
				else if (n < 0)
				{
					if (d_curr_gca == d_begin_gca &&
						d_gca_start_or_end == END)
					{
						// Advance by minus one.
						d_gca_start_or_end = START;
						++n;

						// Advance any remaining amount.
						//
						// Actually this shouldn't be able to happen since we're already at the beginning
						// of the sequence, but we'll advance as requested.
						//
						// TODO: Should we check and throw exception or assert ?
						// The MSVC 'std' library only checks iterators in debug builds.
						if (n < 0)
						{
							std::advance(d_curr_gca, n);
						}
					}
					else
					{
						std::advance(d_curr_gca, n);
					}
				}
			}

			/**
			 * Distance between two iterators - for boost::iterator_facade.
			 */
			typename VertexConstIterator::difference_type
			distance_to(
					const VertexConstIterator &other) const
			{
				typename VertexConstIterator::difference_type difference = std::distance(d_curr_gca, other.d_curr_gca);

				// Make adjustments if either, or both, iterators reference the first point in the sequence.
				if (d_curr_gca == d_begin_gca &&
					d_gca_start_or_end == START)
				{
					++difference;
				}
				if (other.d_curr_gca == other.d_begin_gca &&
					other.d_gca_start_or_end == START)
				{
					--difference;
				}

				return difference;
			}


			// Give access to boost::iterator_facade.
			friend class boost::iterator_core_access;


			/**
			 * Begin iterator of the sequence of GreatCircleArc being iterated over.
			 */
			gca_const_iterator d_begin_gca;

			/**
			 * This points to the current GreatCircleArc in the sequence.
			 */
			gca_const_iterator d_curr_gca;

			/**
			 * This keeps track of whether this iterator is pointing at the "start-point" or
			 * "end-point" of the current GreatCircleArc.
			 */
			StartOrEnd d_gca_start_or_end;
		};


		/**
		 * The type used to const_iterate over the vertices.
		 */
		typedef VertexConstIterator<const_iterator> vertex_const_iterator;


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
		 * construct a polyline instance from a given set of parameters
		 * (ie, your collection of points in the range @a begin / @a end).
		 *
		 * If you pass this function what turns out to be invalid
		 * construction-parameters, it will politely return an error
		 * diagnostic.  If you were to pass these same invalid
		 * parameters to the creation functions down below, you would
		 * get an exception thrown back at you.
		 *
		 * It's not terribly difficult to obtain a collection which
		 * qualifias as valid parameters (no antipodal adjacent points;
		 * at least two distinct points in the collection -- nothing
		 * particularly unreasonable) but the creation functions are
		 * fairly unsympathetic if your parameters @em do turn out to
		 * be invalid.
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
				PointForwardIter begin,
				PointForwardIter end,
				bool check_distinct_points = false);

		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 */
		template <typename C>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const C &coll,
				bool check_distinct_points = false)
		{
			return evaluate_construction_parameter_validity(coll.begin(), coll.end(), check_distinct_points);
		}


		/**
		 * Create a new PolylineOnSphere instance on the heap from the sequence of points
		 * delimited by the forward iterators @a begin and @a end and return
		 * an intrusive_ptr which points to the newly-created instance.
		 *
		 * If @a check_distinct_points is 'true' then the sequence of points
		 * is validated for insufficient *distinct* points, otherwise it is validated
		 * for insufficient points (regardless of whether they are distinct or not).
		 * Distinct points are points that are separated by an epsilon distance (ie, any
		 * points within epsilon distance from each other are counted as one point).
		 * The default is to validate for insufficient *indistinct* points.
		 * This flag was added to prevent exceptions being thrown when reconstructing very small polylines
		 * containing only a few points (eg, a polyline with 4 points might contain 2 distinct points when
		 * it's loaded from a file but, due to numerical precision, contain only 1 distinct point after
		 * it is rotated to a new polyline thus raising an exception when one it not really needed
		 * or desired - because the polyline was good enough to load into GPlates therefore any
		 * rotation of it should also be successful).
		 *
		 * @throws InvalidPointsForPolygonConstructionError if there are insufficient points for the polyline.
		 * If @a check_distinct_points is true then the number of *distinct* points is counted,
		 * otherwise the number of *indistinct* points (ie, the total number of points) is counted.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template<typename PointForwardIter>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				PointForwardIter begin,
				PointForwardIter end,
				bool check_distinct_points = false);

		/**
		 * Create a new PolylineOnSphere instance on the heap from the sequence of points
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
		~PolylineOnSphere();


		/**
		 * Clone this PolylineOnSphere instance, to create a duplicate instance on the
		 * heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const GeometryOnSphere::non_null_ptr_to_const_type
		clone_as_geometry() const
		{
			return GeometryOnSphere::non_null_ptr_to_const_type(new PolylineOnSphere(*this));
		}


		/**
		 * Clone this PolylineOnSphere instance, to create a duplicate instance on the
		 * heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const non_null_ptr_to_const_type
		clone_as_polyline() const
		{
			return non_null_ptr_to_const_type(new PolylineOnSphere(*this));
		}


		/**
		 * Get a non-null pointer to a const PolylineOnSphere which points to this instance
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
		 * Return the "begin" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polyline.
		 */
		const_iterator
		begin() const
		{
			return d_seq.begin();
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polyline.
		 */
		const_iterator
		end() const
		{
			return d_seq.end();
		}


		/**
		 * Return the const_iterator in this polyline at the specified segment index,
		 * so can iterate over GreatCircleArc starting at that segment.
		 *
		 * @a segment_index can be one past the last segment, corresponding to @a end.
		 */
		const_iterator
		segment_iterator(
				size_type segment_index) const
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
		 * Return the number of segments in this polyline.
		 */
		size_type
		number_of_segments() const
		{
			return d_seq.size();
		}


		/**
		 * Return the segment in this polyline at the specified index.
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
		 * Return the "begin" vertex_const_iterator to iterate over the vertices of this polyline.
		 *
		 * Note that it's intentional that the instance returned is non-const: If the
		 * instance were const, it would not be possible to write an expression like
		 * '++(polyline.vertex_begin())' to access the second vertex of the polyline.
		 */
		vertex_const_iterator
		vertex_begin() const
		{
			return vertex_const_iterator::create_begin(begin());
		}


		/**
		 * Return the "end" vertex_const_iterator to iterate over the vertices of this polyline.
		 *
		 * Note that it's intentional that the instance returned is non-const: If the
		 * instance were const, it would not be possible to write an expression like
		 * '--(polyline.vertex_begin())' to access the last vertex of the polyline.
		 */
		vertex_const_iterator
		vertex_end() const
		{
			return vertex_const_iterator::create_end(begin(), end());
		}


		/**
		 * Return the vertex_const_iterator in this polyline at the specified vertex index.
		 *
		 * @a vertex_index can be one past the last vertex, corresponding to @a vertex_end.
		 */
		vertex_const_iterator
		vertex_iterator(
				size_type vertex_index) const
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
		 * Return the number of vertices in this polyline.
		 */
		size_type
		number_of_vertices() const
		{
			return d_seq.size() + 1;
		}


		/**
		 * Return the vertex in this polyline at the specified index.
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
		 * Return the start-point of this polyline.
		 */
		const PointOnSphere &
		start_point() const
		{
			const GreatCircleArc &first_gca = *(begin());
			return first_gca.start_point();
		}


		/**
		 * Return the end-point of this polyline.
		 */
		const PointOnSphere &
		end_point() const
		{
			const GreatCircleArc &last_gca = *(--(end()));
			return last_gca.end_point();
		}


		/**
		 * Evaluate whether @a test_point is "close" to this polyline.
		 *
		 * The measure of what is "close" is provided by @a closeness_angular_extent_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in @a closeness and
		 * return the closest point on the PolylineOnSphere.
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
				const PolylineOnSphere &other) const
		{
			return d_seq == other.d_seq;
		}

		/**
		 * Inequality operator.
		 */
		bool
		operator!=(
				const PolylineOnSphere &other) const
		{
			return !operator==(other);
		}


		//
		// The following are cached calculations on the geometry data.
		//


		/**
		 * Returns the total arc-length of the sequence of @a GreatCirclArc which defines this polyline.
		 *
		 * The result is in radians and represents the distance on the unit radius sphere.
		 *
		 * The result is cached on first call.
		 */
		const real_t &
		get_arc_length() const;


		/**
		 * Returns the centroid of the edges of this polyline (see @a Centroid::calculate_outline_centroid).
		 *
		 * The result is cached on first call.
		 */
		const UnitVector3D &
		get_centroid() const;


		/**
		 * Returns the small circle that bounds this polyline - the small circle centre
		 * is the same as calculated by @a get_centroid.
		 *
		 * The result is cached on first call.
		 */
		const BoundingSmallCircle &
		get_bounding_small_circle() const;


		/**
		 * Returns the small circle bounding tree over of great circle arc segments of this polyline.
		 *
		 * The result is cached on first call.
		 */
		const bounding_tree_type &
		get_bounding_tree() const;

	private:

		/**
		 * Create an empty PolylineOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of a polyline without any vertices.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the static 'create_on_heap' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * default-constructor would, except that it should initialise the ref-count to
		 * zero.
		 */
		PolylineOnSphere();


		/**
		 * Create a copy-constructed PolylineOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the 'clone_as_geometry' or 'clone_as_polyline' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * copy-constructor would, except that it should initialise the ref-count to zero.
		 */
		PolylineOnSphere(
				const PolylineOnSphere &other);


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		PolylineOnSphere &
		operator=(
				const PolylineOnSphere &other);


		/**
		 * Evaluate the validity of the points @a p1 and @a p2 for use
		 * in the creation of a polyline line-segment.
		 */
		static
		ConstructionParameterValidity
		evaluate_segment_endpoint_validity(
				const PointOnSphere &p1,
				const PointOnSphere &p2);


		/**
		 * Generate a sequence of polyline segments from the sequence
		 * of points in the range @a begin / @a end, using the points to
		 * define the endpoints and vertices of the segments, then swap this
		 * new sequence of segments into the polyline @a poly, discarding
		 * any sequence of segments which may have been there before.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template<typename PointForwardIter>
		static
		void
		generate_segments_and_swap(
				PolylineOnSphere &poly,
				PointForwardIter begin,
				PointForwardIter end,
				bool check_distinct_points);


		/**
		 * This is the minimum number of (distinct) collection points to be passed into the
		 * 'create_on_heap' function to enable creation of a closed, well-defined polyline.
		 */
		static const unsigned s_min_num_collection_points;


		/**
		 * This is the sequence of polyline segments.
		 */
		seq_type d_seq;

		/**
		 * Useful calculations on the polyline data.
		 *
		 * These calculations are stored directly with the geometry instead of associating
		 * them at a higher level since it's then much easier to query the same geometry
		 * at various places throughout the code (and reuse results of previous queries).
		 * This is made easier by the fact that the geometry data itself is immutable.
		 *
		 * This pointer is NULL until the first calculation is requested.
		 */
		mutable boost::intrusive_ptr<PolylineOnSphereImpl::CachedCalculations> d_cached_calculations;
	};


	/**
	 * Subdivides each segment (great circle arc) of a polyline and returns tessellated polyline.
	 *
	 * Each pair of adjacent points in the tessellated polyline will have a maximum angular extent of
	 * @a max_angular_extent radians.
	 *
	 * Note that those arcs (of the original polyline) already subtending an angle less than
	 * @a max_angular_extent radians will not be tessellated.
	 *
	 * Note that the distance between adjacent points in the tessellated polyline will not be *uniform*.
	 * This is because each arc in the original polyline is tessellated to the nearest integer number
	 * of points and hence each original arc will have a slightly different tessellation angle.
	 */
	PolylineOnSphere::non_null_ptr_to_const_type
	tessellate(
			const PolylineOnSphere &polyline,
			const real_t &max_angular_extent);


	/**
	 * Determine whether the two polylines @a poly1 and @a poly2 are
	 * equivalent when the directedness of the polyline segments is taken
	 * into account.
	 */
	bool
	polylines_are_directed_equivalent(
			const PolylineOnSphere &poly1,
			const PolylineOnSphere &poly2);


	/**
	 * Determine whether the two polylines @a poly1 and @a poly2 are
	 * equivalent when the directedness of the polyline segments is
	 * ignored.
	 *
	 * By this test, a polyline whose segments are [A, B, C, D] would be
	 * equivalent to polylines [A, B, C, D] or [D', C', B', A'], where 
	 * A' is the reverse of A, B' the reverse of B, and so on.
	 */
	bool
	polylines_are_undirected_equivalent(
			const PolylineOnSphere &poly1,
			const PolylineOnSphere &poly2);


	/**
	 * Concatenates multiple polylines into a single polyline by joining
	 * the tail of one polyline to the head of the next, etc.
	 */
	template <typename PolylineForwardIter>
	PolylineOnSphere::non_null_ptr_to_const_type
	concatenate_polylines(
			PolylineForwardIter begin,
			PolylineForwardIter end);


	template<typename PointForwardIter>
	PolylineOnSphere::ConstructionParameterValidity
	PolylineOnSphere::evaluate_construction_parameter_validity(
			PointForwardIter begin,
			PointForwardIter end,
			bool check_distinct_points)
	{
		const unsigned num_points =
				check_distinct_points
				? count_distinct_adjacent_points(begin, end)
				: std::distance(begin, end);
		if (num_points < s_min_num_collection_points)
		{
			// The collection does not contain enough distinct (or indistinct) points to
			// create even one line-segment.
			return INVALID_INSUFFICIENT_DISTINCT_POINTS;
		}

		PointForwardIter prev;
		PointForwardIter iter = begin;
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

				return v;
			}
		}

		// If we got this far, we couldn't find anything wrong with the
		// construction parameters.
		return VALID;
	}

	template<typename PointForwardIter>
	const PolylineOnSphere::non_null_ptr_to_const_type
	PolylineOnSphere::create_on_heap(
			PointForwardIter begin,
			PointForwardIter end,
			bool check_distinct_points)
	{
		non_null_ptr_type ptr(new PolylineOnSphere());
		generate_segments_and_swap(*ptr, begin, end, check_distinct_points);
		return ptr;
	}


	/**
	 * The exception thrown when an attempt is made to create a polyline using invalid points.
	 */
	class InvalidPointsForPolylineConstructionError:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		/**
		 * Instantiate the exception.
		 *
		 * @param cpv is the polyline's construction parameter validity value, which
		 * presumably describes why the points are invalid.
		 */
		InvalidPointsForPolylineConstructionError(
				const GPlatesUtils::CallStack::Trace &exception_source,
				PolylineOnSphere::ConstructionParameterValidity cpv) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_cpv(cpv),
			d_filename(exception_source.get_filename()),
			d_line_num(exception_source.get_line_num())
		{  }

		~InvalidPointsForPolylineConstructionError() throw(){}

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "InvalidPointsForPolylineConstructionError";
		}

		// FIXME: This would be better as a 'const std::string'.
		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		PolylineOnSphere::ConstructionParameterValidity d_cpv;
		const char *d_filename;
		int d_line_num;
	};


	template<typename PointForwardIter>
	void
	PolylineOnSphere::generate_segments_and_swap(
			PolylineOnSphere &poly,
			PointForwardIter begin,
			PointForwardIter end,
			bool check_distinct_points)
	{
		// NOTE: We ignore determination of insufficient distinct points if we are *not*
		// throwing an exception for it.
		ConstructionParameterValidity v =
				evaluate_construction_parameter_validity(
						begin,
						end,
						check_distinct_points);
		if (v != VALID)
		{
			throw InvalidPointsForPolylineConstructionError(GPLATES_EXCEPTION_SOURCE, v);
		}

		// Make it easier to provide strong exception safety by appending the new segments
		// to a temporary sequence (rather than putting them directly into 'd_seq').
		seq_type tmp_seq;
		// Observe that the number of points used to define a polyline (which will become
		// the number of vertices in the polyline, counting the begin-point and end-point
		// of the polyline as vertices) is one greater than the number of segments in the
		// polyline.
		const seq_type::size_type num_points = std::distance(begin, end);
		tmp_seq.reserve(num_points - 1);

		PointForwardIter prev;
		PointForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++)
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;
			tmp_seq.push_back(GreatCircleArc::create(p1, p2));
		}
		poly.d_seq.swap(tmp_seq);
	}


	template <typename PolylineForwardIter>
	PolylineOnSphere::non_null_ptr_to_const_type
	concatenate_polylines(
			PolylineForwardIter polylines_begin,
			PolylineForwardIter polylines_end)
	{
		unsigned int num_total_points = 0;

		// Calculate total number of points in all polylines.
		PolylineForwardIter polylines_iter = polylines_begin;
		for ( ; polylines_iter != polylines_end; ++polylines_iter)
		{
			num_total_points += (*polylines_iter)->number_of_vertices();
		}

		// Add all polyline points to a single point vector.
		std::vector<PointOnSphere> all_points;
		all_points.reserve(num_total_points);
		for (polylines_iter = polylines_begin ; polylines_iter != polylines_end; ++polylines_iter)
		{
			all_points.insert(all_points.end(),
					(*polylines_iter)->vertex_begin(),
					(*polylines_iter)->vertex_end());
		}

		// Create the final concatenated polyline.
		return PolylineOnSphere::create_on_heap(all_points.begin(), all_points.end());
	}
}

#endif  // GPLATES_MATHS_POLYLINEONSPHERE_H
