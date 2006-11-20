/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006 The University of Sydney, Australia
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

#include <list>
#include <iterator>  /* iterator, bidirectional_iterator_tag */
#include <utility>  /* pair */
#include <memory>  /* auto_ptr */
#include "GreatCircleArc.h"
#include "InvalidPolygonException.h"

namespace GPlatesMaths {

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
	 * You can create a polygon by invoking either the static member
	 * function @a PolygonOnSphere::create or the static member function
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
	 * this sequence to the @a PolygonOnSphere::create function, it will
	 * create a polygon composed of 4 segments: A->B, B->C, C->D and D->A. 
	 * If you subsequently iterate through the vertices of this polygon,
	 * you will get the same sequence of points back again: A, B, C, D.
	 */
	class PolygonOnSphere {

	 public:

		/**
		 * The type of the sequence of great circle arcs.
		 */
		typedef std::list< GreatCircleArc > seq_type;


		/**
		 * The type used to const_iterate over the sequence of arcs.
		 */
		typedef seq_type::const_iterator const_iterator;


		/**
		 * This class enables const_iteration over the vertices of a
		 * PolygonOnSphere.
		 *
		 * An instance of this class @em actually iterates over the
		 * sequence of GreatCircleArc by which a PolygonOnSphere is
		 * implemented, but it pretends it's iterating over a sequence
		 * of PointOnSphere.
		 *
		 * It is assumed that the sequence of GreatCircleArc over
		 * which this iterator is iterating will always contain at
		 * least three elements (and thus, at least three vertices). 
		 * This assumption should be fulfilled by the PolygonOnSphere
		 * invariant.
		 */
		class VertexConstIterator: public std::iterator<
		 std::bidirectional_iterator_tag, PointOnSphere > {
		
			typedef PolygonOnSphere::const_iterator
			 gca_const_iterator;

		  public:

			/**
			 * Create the "begin" vertex const_iterator for @a poly.
			 */
			static
			VertexConstIterator
			create_begin(
			 const PolygonOnSphere &poly);


			/**
			 * Create the "end" vertex const_iterator for @a poly.
			 */
			static
			VertexConstIterator
			create_end(
			 const PolygonOnSphere &poly);


			/**
			 * Default-construct a vertex iterator.
			 *
			 * A default-constructed iterator will be
			 * uninitialised.  (I don't @em like providing a
			 * constructor which leaves an object in an
			 * uninitialised state, but the presence of a default
			 * constructor is mandated by the
			 * bidirectional_iterator interface.)
			 *
			 * If you attempt to dereference an uninitialised
			 * iterator or access the members of a PointOnSphere
			 * through an uninitialised iterator you will get a
			 * GPlatesGlobal::UninitialisedIteratorException thrown
			 * back at you.  (Hey, it's better than a segfault...)
			 *
			 * The following operations are OK for an uninitialised
			 * iterator:
			 *  - comparison for equality to another iterator;
			 *  - comparison for inequality to another iterator;
			 *  - being assigned-to by another iterator;
			 *  - increment;
			 *  - decrement.
			 */
			VertexConstIterator() :
			 d_poly_ptr(NULL), 
			 d_curr_gca() {  }


			/**
			 * Copy-construct a vertex iterator.
			 */
			VertexConstIterator(
			 const VertexConstIterator &other) :
			 d_poly_ptr(other.d_poly_ptr),
			 d_curr_gca(other.d_curr_gca) {  }


			/**
			 * Return the gca_const_iterator which points to the
			 * current GreatCircleArc.
			 */
			gca_const_iterator
			curr_gca() const {

				return d_curr_gca;
			}


			/**
			 * Assign @a other to this.
			 */
			VertexConstIterator &
			operator=(
			 const VertexConstIterator &other) {

				d_poly_ptr = other.d_poly_ptr;
				d_curr_gca = other.d_curr_gca;

				return *this;
			}


			/**
			 * Dereference this iterator to obtain the
			 * currently-pointed-at PointOnSphere.
			 */
			const PointOnSphere &
			operator*() const {

				return current_point();
			}


			/**
			 * Access a member of the PointOnSphere which is
			 * currently being pointed-at by this iterator.
			 */
			const PointOnSphere *
			operator->() const {

				return &(current_point());
			}


			/**
			 * Pre-increment this iterator.
			 */
			VertexConstIterator &
			operator++() {

				increment();

				return *this;
			}


			/**
			 * Post-increment this iterator.
			 */
			const VertexConstIterator
			operator++(int) {

				VertexConstIterator old = *this;
				increment();

				return old;
			}


			/**
			 * Pre-decrement this iterator.
			 */
			VertexConstIterator &
			operator--() {

				decrement();

				return *this;
			}


			/**
			 * Post-decrement this iterator.
			 */
			const VertexConstIterator
			operator--(int) {

				VertexConstIterator old = *this;
				decrement();

				return old;
			}

		  private:

			/**
			 * Gee, why do you think this is private?
			 */
			VertexConstIterator(
			 const PolygonOnSphere &poly,
			 gca_const_iterator curr_gca_) :
			 d_poly_ptr(&poly),
			 d_curr_gca(curr_gca_) {  }


			/**
			 * This function performs the magic which is used to
			 * obtain the currently-pointed-at PointOnSphere.
			 */
			const PointOnSphere &
			current_point() const;


			/**
			 * This function performs the magic which is used to
			 * increment this iterator.
			 */
			void
			increment();


			/**
			 * This function performs the magic which is used to
			 * decrement this iterator.
			 */
			void
			decrement();

			const PolygonOnSphere *d_poly_ptr;

			gca_const_iterator d_curr_gca;

		};


		/**
		 * The type used to const_iterate over the vertices.
		 */
		typedef VertexConstIterator vertex_const_iterator;


		/**
		 * The possible return values from the construction-parameter
		 * validation functions
		 * @a evaluate_construction_parameter_validity and
		 * @a evaluate_segment_endpoint_validity.
		 */
		enum ConstructionParameterValidity {

			VALID,
			INVALID_INSUFFICIENT_DISTINCT_POINTS,
			INVALID_DUPLICATE_SEGMENT_ENDPOINTS,
			INVALID_ANTIPODAL_SEGMENT_ENDPOINTS
		};


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
		 * qualifias as valid parameters (no duplicate or antipodal
		 * adjacent points; at least three distinct points in the
		 * collection -- nothing particularly unreasonable) but the
		 * creation functions are fairly unsympathetic if your
		 * parameters @em do turn out to be invalid.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * @a invalid_points is a return-parameter; if the
		 * construction-parameters are found to be invalid due to
		 * duplicate or antipodal adjacent points, the value of this
		 * return-parameter will be set to the pair of const_iterators
		 * of @a coll which point to the guilty points.  If no adjacent
		 * points are found to be duplicate or antipodal, this
		 * parameter will not be modified.
		 *
		 * The optional argument @a should_silently_drop_dups controls
		 * whether or not duplicate adjacent points should silently be
		 * dropped instead of causing an exception to be thrown.  (Dup
		 * adjacent points are a not-uncommon occurrence when reading
		 * PLATES data files.  All Hail PLATES!)
		 */
		template< typename C >
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
		 const C &coll,
		 std::pair< typename C::const_iterator, typename
		  C::const_iterator > &invalid_points,
		 bool should_silently_drop_dups = true);


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
		 * Create a new PolygonOnSphere instance from the sequence of
		 * points @a coll.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template< typename C >
		static
		PolygonOnSphere
		create(
		 const C &coll);


		/**
		 * Create a new PolygonOnSphere instance on the heap from the
		 * sequence of points @a coll, and return an auto_ptr which
		 * points to the newly-created instance.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template< typename C >
		static
		std::auto_ptr< PolygonOnSphere >
		create_on_heap(
		 const C &coll);


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polygon.
		 */
		const_iterator
		begin() const {
			
			return d_seq.begin();
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polygon.
		 */
		const_iterator
		end() const {
			
			return d_seq.end();
		}


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * vertices of this polygon.
		 */
		vertex_const_iterator
		vertex_begin() const {

			return vertex_const_iterator::create_begin(*this);
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * vertices of this polygon.
		 */
		vertex_const_iterator
		vertex_end() const {

			return vertex_const_iterator::create_end(*this);
		}


		/**
		 * Swap the contents of this polygon with @a other.
		 *
		 * This function does not throw.
		 */
		void
		swap(
		 PolygonOnSphere &other) {

			d_seq.swap(other.d_seq);
		}


		/**
		 * Evaluate whether @a test_point is "close" to this polygon.
		 *
		 * Note: This function currently only tests whether
		 * @a test_point is "close" to the polygon @em boundary.
		 *
		 * The measure of what is "close" is provided by
		 * @a closeness_inclusion_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in @a closeness.
		 *
		 * The value of @a latitude_exclusion_threshold should be equal
		 * to \f$\sqrt{1 - {t_c}^2}\f$ (where \f$t_c\f$ is the
		 * closeness inclusion threshold).  This parameter is designed
		 * to enable a quick elimination of "no-hopers" (test-points
		 * which can easily be determined to have no chance of being
		 * "close"), leaving only plausible test-points to proceed to
		 * the more expensive proximity tests.  If you imagine a
		 * line-segment of this polygon as an arc along the equator,
		 * then there will be a threshold latitude above and below the
		 * equator, beyond which there is no chance of a test-point
		 * being "close" to that segment.
		 *
		 * For more information, read the comment before
		 * @a GPlatesState::Layout::find_close_data.
		 */
		bool
		is_close_to(
		 const PointOnSphere &test_point,
		 const real_t &closeness_inclusion_threshold,
		 const real_t &latitude_exclusion_threshold,
		 real_t &closeness) const;

	 private:

		/**
		 * The default constructor should never be invoked directly by
		 * client code; only through the static 'create' functions.
		 */
		PolygonOnSphere() {  }


		/**
		 * Appends a copy of @a g to the end.
		 */
		void
		push_back(
		 const GreatCircleArc &g) {

			d_seq.push_back(g);
		}


		/**
		 * Populate the empty polygon @a poly with the collection of 
		 * points @a coll, using the points to define vertices of the
		 * polygon.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template< typename C >
		static
		void
		populate(
		 PolygonOnSphere &poly,
		 const C &coll);


		/**
		 * Attempt to create a line-segment defined by the points @a p1
		 * and @a p2; append it to @a seq.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		static
		void
		create_segment_and_append_to_seq(
		 seq_type &seq,
		 const PointOnSphere &p1,
		 const PointOnSphere &p2,
		 bool should_silently_drop_dups = true);


		/**
		 * The minimum number of (distinct) collection points to be
		 * passed into a 'create' function to enable creation of a
		 * closed, well-defined polygon.
		 */
		static const unsigned MIN_NUM_COLLECTION_POINTS;

		seq_type d_seq;

	};


	inline
	PolygonOnSphere::VertexConstIterator
	PolygonOnSphere::VertexConstIterator::create_begin(
	 const PolygonOnSphere &poly) {

		return VertexConstIterator(poly, poly.begin());
	}
	

	inline
	PolygonOnSphere::VertexConstIterator
	PolygonOnSphere::VertexConstIterator::create_end(
	 const PolygonOnSphere &poly) {

		return VertexConstIterator(poly, poly.end());
	}


	inline
	void
	PolygonOnSphere::VertexConstIterator::increment() {

		++d_curr_gca;
	}


	inline
	void
	PolygonOnSphere::VertexConstIterator::decrement() {

		--d_curr_gca;
	}


	/**
	 * Compare @a i1 and @a i2 for equality.
	 */
	inline
	bool
	operator==(
	 const PolygonOnSphere::VertexConstIterator &i1,
	 const PolygonOnSphere::VertexConstIterator &i2) {

		return (i1.curr_gca() == i2.curr_gca());
	}


	/**
	 * Compare @a i1 and @a i2 for inequality.
	 */
	inline
	bool
	operator!=(
	 const PolygonOnSphere::VertexConstIterator &i1,
	 const PolygonOnSphere::VertexConstIterator &i2) {

		return (i1.curr_gca() != i2.curr_gca());
	}


	template< typename C >
	PolygonOnSphere::ConstructionParameterValidity
	PolygonOnSphere::evaluate_construction_parameter_validity(
	 const C &coll,
	 std::pair< typename C::const_iterator, typename
	  C::const_iterator > &invalid_points,
	 bool should_silently_drop_dups) {

		typename C::size_type num_points = coll.size();
		if (num_points < MIN_NUM_COLLECTION_POINTS) {

			// The collection does not contain enough points to
			// create a closed, well-defined polygon.
			return INVALID_INSUFFICIENT_DISTINCT_POINTS;
		}

		// This for-loop is identical to the corresponding code in
		// PolylineOnSphere.
		typename C::const_iterator
		 prev,
		 iter = coll.begin(),
		 end = coll.end();
		for (prev = iter++ ; iter != end; prev = iter++) {

			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v =
			 evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's
			// "-Wswitch" option (implicitly enabled by "-Wall"),
			// will help to ensure that no cases are missed.
			switch (v) {

			 case VALID:

				// Keep looping.
				break;

			 case INVALID_INSUFFICIENT_DISTINCT_POINTS:

				// This value shouldn't be returned.
				// FIXME:  Can this be checked at compile-time?
				// (Perhaps with use of template magic, to
				// avoid the need to check at run-time and
				// throw an exception if the assertion fails.)
				break;

			 case INVALID_DUPLICATE_SEGMENT_ENDPOINTS:

				if (should_silently_drop_dups) {

					// You heard the man:  We should
					// silently drop duplicates.  But we
					// still need to keep track of the
					// number of (usable) points.
					--num_points;

				} else {

					invalid_points.first = prev;
					invalid_points.second = iter;

					return v;
				}
				// Keep looping.
				break;

			 case INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:

				invalid_points.first = prev;
				invalid_points.second = iter;

				return v;
			}
		}

		// Now, an additional check, for the last->first point
		// wrap-around.
		iter = coll.begin();
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v =
			 evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's
			// "-Wswitch" option (implicitly enabled by "-Wall"),
			// will help to ensure that no cases are missed.
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

			 case INVALID_DUPLICATE_SEGMENT_ENDPOINTS:

				if (should_silently_drop_dups) {

					// You heard the man:  We should
					// silently drop duplicates.  But we
					// still need to keep track of the
					// number of (usable) points.
					--num_points;

				} else {

					invalid_points.first = prev;
					invalid_points.second = iter;

					return v;
				}
				// Exit the switch-statement.
				break;

			 case INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:

				invalid_points.first = prev;
				invalid_points.second = iter;

				return v;
			}
		}

		// Check the number of (usable) points again, now that we've
		// abjusted for duplicates.
		if (num_points < MIN_NUM_COLLECTION_POINTS) {

			return INVALID_INSUFFICIENT_DISTINCT_POINTS;
		}

		// If we got this far, we couldn't find anything wrong with the
		// construction parameters.
		return VALID;
	}


	template< typename C >
	PolygonOnSphere
	PolygonOnSphere::create(
	 const C &coll) {

		PolygonOnSphere p;
		populate(p, coll);
		return p;
	}


	template< typename C >
	std::auto_ptr< PolygonOnSphere >
	PolygonOnSphere::create_on_heap(
	 const C &coll) {

		std::auto_ptr< PolygonOnSphere > ptr(new PolygonOnSphere());
		populate(*ptr, coll);
		return ptr;
	}


	template< typename C >
	void
	PolygonOnSphere::populate(
	 PolygonOnSphere &poly,
	 const C &coll) {

		if (coll.size() < MIN_NUM_COLLECTION_POINTS) {

			// The collection does not contain enough points to
			// create a closed, well-defined polygon.
			// FIXME:  I don't like throwing in a header-file.
			throw InvalidPolygonException("Attempted to create a "
			 "polygon from an insufficient number (ie, less than "
			 "3) of endpoints.");
		}

		// There's no real need to put all the new line-segments into a
		// temporary sequence (rather than putting them directly into
		// 'd_seq'), but doing it this way might make the code logic a
		// little easier to follow.
		seq_type tmp_seq;

		// This for-loop is identical to the corresponding code in
		// PolylineOnSphere.
		typename C::const_iterator
		 prev,
		 iter = coll.begin(),
		 end = coll.end();
		for (prev = iter++ ; iter != end; prev = iter++) {

			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			create_segment_and_append_to_seq(tmp_seq, p1, p2);
		}
		// Now, an additional step, for the last->first point
		// wrap-around.
		iter = coll.begin();
		{
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			create_segment_and_append_to_seq(tmp_seq, p1, p2);
		}

		if (tmp_seq.size() < MIN_NUM_COLLECTION_POINTS) {

			// Not enough line-segments were created to create a
			// closed, well-defined polygon.
			// FIXME:  I don't like throwing in a header-file.
			throw InvalidPolygonException("Attempted to create a "
			 "polygon from an insufficient number (ie, less than "
			 "3) of distinct endpoints.");
		}
		poly.d_seq.splice(poly.d_seq.end(), tmp_seq);
	}

}

#endif  // GPLATES_MATHS_POLYGONONSPHERE_H
