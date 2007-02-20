/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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
#include <boost/intrusive_ptr.hpp>
#include "UnitVector3D.h"


namespace GPlatesMaths {

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
	class PointOnSphere {

		public:

			/**
			 * The type used for the reference-count.
			 */
			typedef long ref_count_type;


			/**
			 * Create a new PointOnSphere instance on the heap from the unit vector
			 * @a position_vector_.
			 *
			 * This function is strongly exception-safe and exception-neutral.
			 */
			static
			const boost::intrusive_ptr<PointOnSphere>
			create_on_heap(
					const UnitVector3D &position_vector_);


			/**
			 * Clone this PointOnSphere instance, to create a duplicate instance on the
			 * heap.
			 *
			 * This function is strongly exception-safe and exception-neutral.
			 */
			const boost::intrusive_ptr<PointOnSphere>
			clone_on_heap() const
			{
				boost::intrusive_ptr<PointOnSphere> dup(new PointOnSphere(*this));
				return dup;
			}


			/**
			 * Create a new PointOnSphere instance from the unit vector
			 * @a position_vector_.
			 *
			 * FIXME:  We should really prohibit construction of PointOnSphere
			 * instances on the stack, insisting that they are instead created on the
			 * heap, referenced by intrusive_ptr.  However, there is a substantial
			 * amount of existing code which creates PointOnSphere instances on the
			 * stack, and changing all that code to use intrusive_ptr would take too
			 * long to justify right now.  But we should do it properly some day...
			 *
			 * Trac ticket: http://trac.gplates.org/ticket/3
			 */
			explicit 
			PointOnSphere(
					const UnitVector3D &position_vector_):
				d_ref_count(0),
				d_position_vector(position_vector_)
			{  }


			/**
			 * Create a copy-constructed PointOnSphere instance on the stack.
			 *
			 * This constructor should act exactly the same as the default
			 * (auto-generated) copy-constructor would, except that it should
			 * initialise the ref-count to zero.
			 *
			 * FIXME:  We should really prohibit construction of PointOnSphere
			 * instances on the stack, insisting that they are instead created on the
			 * heap, referenced by intrusive_ptr.  However, there is a substantial
			 * amount of existing code which creates PointOnSphere instances on the
			 * stack, and changing all that code to use intrusive_ptr would take too
			 * long to justify right now.  But we should do it properly some day...
			 *
			 * Trac ticket: http://trac.gplates.org/ticket/3
			 */
			PointOnSphere(
					const PointOnSphere &other):
				d_ref_count(0),
				d_position_vector(other.d_position_vector)
			{  }


			/**
			 * Copy-assign the value of @a other to this.
			 *
			 * This function has a nothrow exception-safety guarantee.
			 *
			 * This copy-assignment operator should act exactly the same as the default
			 * (auto-generated) copy-assignment operator would, except that it should
			 * not assign the ref-count of @a other to this.
			 *
			 * It makes sense to define this operator, because: there are already going
			 * to be instances of PointOnSphere on the stack (see the "FIXME" comment
			 * in the copy-constructor above); there's no polymorphism involved, so
			 * there's no concern about polymorphic slicing; and there's no
			 * class-behavioural or efficiency-related reason why instances of this
			 * class should not be copy-assignable.
			 */
			PointOnSphere &
			operator=(
					const PointOnSphere &other)
			{
				d_position_vector = other.d_position_vector;
				return *this;
			}


			const UnitVector3D &
			position_vector() const {
				
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
			 * @a GPlatesState::Layout::find_close_data.
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
			 * Increment the reference-count of this instance.
			 */
			void
			increment_ref_count() const {
				++d_ref_count;
			}


			/**
			 * Decrement the reference-count of this instance.
			 */
			ref_count_type
			decrement_ref_count() const {
				return --d_ref_count;
			}

		private:

			/**
			 * This is the reference-count used by boost::intrusive_ptr.
			 *
			 * It is declared "mutable", because it is to be modified by
			 * 'increment_ref_count' and 'decrement_ref_count', which are const member
			 * functions.  They are const member functions because they do not modify
			 * the "abstract state" of the instance; the reference-count is really only
			 * memory-management book-keeping.
			 */
			mutable ref_count_type d_ref_count;


			/**
			 * This is the 3-D unit-vector which defines the position of this point.
			 */
			UnitVector3D d_position_vector;

	};


	inline
	const boost::intrusive_ptr<PointOnSphere>
	PointOnSphere::create_on_heap(
			const UnitVector3D &position_vector_) {
		boost::intrusive_ptr<PointOnSphere> ptr(new PointOnSphere(position_vector_));
		return ptr;
	}


	/**
	 * Return the point antipodal to @a p on the sphere.
	 */
	inline
	const PointOnSphere
	get_antipodal_point(
			const PointOnSphere &p) {

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
			const PointOnSphere &p2) {

		return dot(p1.position_vector(), p2.position_vector());
	}


	inline
	bool
	operator==(
			const PointOnSphere &p1,
			const PointOnSphere &p2) {

		return (p1.position_vector() == p2.position_vector());
	}


	inline
	bool
	operator!=(
			const PointOnSphere &p1,
			const PointOnSphere &p2) {

		return (p1.position_vector() != p2.position_vector());
	}


	/**
	 * Return whether the points @a p1 and @a p2 are coincident.
	 */
	inline
	bool
	points_are_coincident(
			const PointOnSphere &p1,
			const PointOnSphere &p2) {

		return (p1.position_vector() == p2.position_vector());
	}


	std::ostream &
	operator<<(
			std::ostream &os,
			const PointOnSphere &p);


	inline
	void
	intrusive_ptr_add_ref(
			const PointOnSphere *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const PointOnSphere *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MATHS_POINTONSPHERE_H
