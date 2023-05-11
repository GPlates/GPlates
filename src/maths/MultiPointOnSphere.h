/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_MULTIPOINTONSPHERE_H
#define GPLATES_MATHS_MULTIPOINTONSPHERE_H

#include <vector>
#include <algorithm>
#include <iterator>   // std::distance
#include <boost/intrusive_ptr.hpp>

#include "GeometryOnSphere.h"
#include "PointOnSphere.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesMaths
{
	// Forward declarations.
	namespace MultiPointOnSphereImpl
	{
		struct CachedCalculations;
	}
	class BoundingSmallCircle;


	/** 
	 * Represents a multi-point on the surface of a sphere. 
	 *
	 * Internally, this is stored as a container of PointOnSphere.  You
	 * can iterate over this sequence of PointOnSphere in the usual manner
	 * using the const_iterators returned by the functions @a begin and
	 * @a end.
	 *
	 * You can create a multi-point by invoking the static member function
	 * @a MultiPointOnSphere::create, passing it a sequential STL
	 * container (list, vector, ...) of PointOnSphere to define the points
	 * in the multi-point.  The sequence of points must contain at least one
	 * element, enabling the creation of a multi-point composed of at least
	 * one point.
	 *
	 * Note that MultiPointOnSphere does *not* have mutators (non-const member
	 * functions which enable the modification of the class internals).
	 */
	class MultiPointOnSphere:
			public GeometryOnSphere
	{
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MultiPointOnSphere>.
		 * 
		 * Note that this typedef is indeed meant to be private.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MultiPointOnSphere> non_null_ptr_type;

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere> non_null_ptr_to_const_type;


		/**
		 * The type of the container of points.
		 */
		typedef std::vector<PointOnSphere> point_container_type;


		/**
		 * The type used to const_iterate over the container of points.
		 */
		typedef point_container_type::const_iterator const_iterator;


		/**
		 * The possible return values from the construction-parameter
		 * validation function
		 * @a evaluate_construction_parameter_validity.
		 */
		enum ConstructionParameterValidity
		{
			VALID,
			INVALID_INSUFFICIENT_POINTS
		};


		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * What this actually means in plain(er) English is that you
		 * can use this function to check whether you would be able to
		 * construct a multi-point instance from a given set of parameters
		 * (ie, your collection of points in the range @a begin / @a end).
		 *
		 * If you pass this function what turns out to be invalid
		 * construction-parameters, it will politely return an error
		 * diagnostic.  If you were to pass these same invalid
		 * parameters to the creation functions down below, you would
		 * get an exception thrown back at you.
		 *
		 * It's not terribly difficult to obtain a collection which
		 * qualifies as valid parameters (at least one point in the
		 * collection -- nothing particularly unreasonable) but the
		 * creation functions are fairly unsympathetic if your
		 * parameters @em do turn out to be invalid.
		 */
		template<typename ForwardIterPointOnSphere>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				ForwardIterPointOnSphere begin,
				ForwardIterPointOnSphere end);


		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 */
		template<typename C>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const C &coll)
		{
			return evaluate_construction_parameter_validity(coll.begin(), coll.end());
		}


		/**
		 * Create a new MultiPointOnSphere instance on the heap from the sequence of points
		 * in the range @a begin / @a end, and return an intrusive_ptr which points to the
		 * newly-created instance.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template <typename ForwardIterPointOnSphere>
		static
		const non_null_ptr_to_const_type
		create(
				ForwardIterPointOnSphere begin,
				ForwardIterPointOnSphere end);


		/**
		 * Create a new MultiPointOnSphere instance on the heap from the sequence of points
		 * @a coll, and return an intrusive_ptr which points to the newly-created instance.
		 *
		 * @a coll should be an STL container (list, vector, ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template<typename C>
		static
		const non_null_ptr_to_const_type
		create(
				const C &coll)
		{
			return create(coll.begin(), coll.end());
		}


		virtual
		~MultiPointOnSphere();


		/**
		 * Return this instance as a non-null pointer.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}


		/**
		 * Get the collection as a vector 
		 */ 
		point_container_type
		collection() const
		{
			return d_points;
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
		 * container of points which defines this multi-point.
		 */
		const_iterator
		begin() const
		{
			return d_points.begin();
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * container of points which defines this multi-point.
		 */
		const_iterator
		end() const
		{
			return d_points.end();
		}


		/**
		 * Return the number of points in this multi-point.
		 */
		unsigned int
		number_of_points() const
		{
			return d_points.size();
		}


		/**
		 * Return the point in this multi-point at the specified index.
		 */
		const PointOnSphere &
		get_point(
				unsigned int point_index) const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					point_index < number_of_points(),
					GPLATES_ASSERTION_SOURCE);

			return d_points[point_index];
		}


		/**
		 * Return the start-point of this multi-point.
		 */
		const PointOnSphere &
		start_point() const
		{
			// It is an invariant of this class that it contains at least one point.
			return d_points.front();
		}


		/**
		 * Return the end-point of this multi-point.
		 */
		const PointOnSphere &
		end_point() const
		{
			// It is an invariant of this class that it contains at least one point.
			return d_points.back();
		}


		/**
		 * Evaluate whether @a test_point is "close" to this multi-point.
		 *
		 * The measure of what is "close" is provided by
		 * @a closeness_inclusion_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in @a closeness and
		 * return the closest point on the MultiPointOnSphere.
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		is_close_to(
				const PointOnSphere &test_point,
				const real_t &closeness_inclusion_threshold,
				real_t &closeness) const;


		/**
		 * Equality operator compares points in order.
		 */
		bool
		operator==(
				const MultiPointOnSphere &other) const
		{
			return d_points == other.d_points;
		}

		/**
		 * Inequality operator.
		 */
		bool
		operator!=(
				const MultiPointOnSphere &other) const
		{
			return !operator==(other);
		}


		//
		// The following are cached calculations on the geometry data.
		//

		/**
		 * Returns the sum of the points in this multipoint (normalised).
		 *
		 * The result is cached on first call.
		 */
		const UnitVector3D &
		get_centroid() const;


		/**
		 * Returns the small circle that bounds this multipoint - the small circle centre
		 * is the same as calculated by @a get_centroid.
		 *
		 * The result is cached on first call.
		 */
		const BoundingSmallCircle &
		get_bounding_small_circle() const;

	private:

		/**
		 * Create an empty MultiPointOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of a multi-point without any vertices.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the static 'create' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * default-constructor would, except that it should initialise the ref-count to
		 * zero.
		 */
		MultiPointOnSphere();


		/**
		 * This is the minimum number of collection points to be passed into the
		 * 'create' function to enable creation of a multi-point.
		 */
		static const unsigned s_min_num_collection_points;


		/**
		 * This is the collection of points.
		 */
		point_container_type d_points;

		/**
		 * Useful calculations on the multipoint data.
		 *
		 * These calculations are stored directly with the geometry instead of associating
		 * them at a higher level since it's then much easier to query the same geometry
		 * at various places throughout the code (and reuse results of previous queries).
		 * This is made easier by the fact that the geometry data itself is immutable.
		 *
		 * This pointer is NULL until the first calculation is requested.
		 */
		mutable boost::intrusive_ptr<MultiPointOnSphereImpl::CachedCalculations> d_cached_calculations;

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Determine whether the two multi-points @a mp1 and @a mp2 are
	 * equivalent when the ordering of the points is taken into account.
	 */
	bool
	multi_points_are_ordered_equivalent(
			const MultiPointOnSphere &mp1,
			const MultiPointOnSphere &mp2);


	template<typename ForwardIterPointOnSphere>
	MultiPointOnSphere::ConstructionParameterValidity
	MultiPointOnSphere::evaluate_construction_parameter_validity(
			ForwardIterPointOnSphere begin,
			ForwardIterPointOnSphere end)
	{
		if (begin == end) {
			return INVALID_INSUFFICIENT_POINTS;
		} else {
			return VALID;
		}
	}


	/**
	 * The exception thrown when an attempt is made to create a multi-point using insufficient
	 * points.
	 */
	class InsufficientPointsForMultiPointConstructionError:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		/**
		 * Instantiate the exception.
		 */
		InsufficientPointsForMultiPointConstructionError(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_filename(exception_source.get_filename()),
			d_line_num(exception_source.get_line_num())
		{  }

		~InsufficientPointsForMultiPointConstructionError() throw(){}
	
	protected:
		virtual
		const char *
		exception_name() const
		{
			return "InsufficientPointsForMultiPointConstructionError";
		}

	private:
		const char *d_filename;
		int d_line_num;
	};


	template<typename ForwardIterPointOnSphere>
	const MultiPointOnSphere::non_null_ptr_to_const_type
	MultiPointOnSphere::create(
			ForwardIterPointOnSphere begin,
			ForwardIterPointOnSphere end)
	{
		ConstructionParameterValidity v = evaluate_construction_parameter_validity(begin, end);
		if (v != VALID) {
			throw InsufficientPointsForMultiPointConstructionError(GPLATES_EXCEPTION_SOURCE);
		}

		non_null_ptr_type ptr(new MultiPointOnSphere());
		ptr->d_points.reserve(std::distance(begin, end));
		ptr->d_points.assign(begin, end);

		return ptr;
	}
}

#endif  // GPLATES_MATHS_MULTIPOINTONSPHERE_H
