/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GEOMETRYONSPHERE_H
#define GPLATES_MATHS_GEOMETRYONSPHERE_H

#include "ProximityHitDetail.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesMaths
{
	class ProximityCriteria;
	class ConstGeometryOnSphereVisitor;


	/**
	 * This class is the abstract base of all geometries on the sphere.
	 *
	 * It provides pure virtual function declarations for cloning and accepting visitors.  It
	 * also provides the functions to be used by boost::intrusive_ptr for reference-counting.
	 */
	class GeometryOnSphere
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * Construct a GeometryOnSphere instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		GeometryOnSphere():
			d_ref_count(0)
		{  }

		/**
		 * Construct a GeometryOnSphere instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 *
		 * This ctor should only be invoked by the @a clone_as_geometry member function
		 * (pure virtual in this class; defined in derived classes), which will create a
		 * duplicate instance and return a new intrusive_ptr reference to the new
		 * duplicate.  Since initially the only reference to the new duplicate will be the
		 * one returned by the @a clone_as_geometry function, *before* the new
		 * intrusive_ptr is created, the ref-count of the new GeometryOnSphere instance
		 * should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		GeometryOnSphere(
				const GeometryOnSphere &other) :
			d_ref_count(0)
		{  }

		virtual
		~GeometryOnSphere()
		{  }

		/**
		 * Create a duplicate of this GeometryOnSphere instance.
		 */
		virtual
		const non_null_ptr_to_const_type
		clone_as_geometry() const = 0;

		/**
		 * Test for a proximity hit.
		 *
		 * If there is a hit, the pointer returned will be a pointer to extra information
		 * about the hit -- for example, the specific vertex (point) or segment (GCA) of a
		 * polyline which was hit.
		 *
		 * Otherwise (ie, if there was not a hit), the pointer returned will be null.
		 */
		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const = 0;

		/**
		 * Accept a ConstGeometryOnSphereVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstGeometryOnSphereVisitor &visitor) const = 0;

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

		/**
		 * Access the reference-count of this instance.
		 *
		 * This function is intended for use by @a PointOnSphere::get_non_null_pointer.
		 */
		ref_count_type
		ref_count() const
		{
			return d_ref_count;
		}

	private:
		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor
		// 'clone_as_geometry' (which will in turn use the copy-constructor); all
		// "assignment" should really only be assignment of one intrusive_ptr to another.
		GeometryOnSphere &
		operator=(
				const GeometryOnSphere &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const GeometryOnSphere *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const GeometryOnSphere *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MATHS_GEOMETRYONSPHERE_H
