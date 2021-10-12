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

#ifndef GPLATES_MATHS_CONSTGEOMETRYONSPHEREVISITOR_H
#define GPLATES_MATHS_CONSTGEOMETRYONSPHEREVISITOR_H

#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "SmallCircle.h"


namespace GPlatesMaths
{
	/**
	 * This class defines an abstract interface for a Visitor to visit const geometries on the
	 * sphere.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 *  - The "visit" functions explicitly include the name of the target type in the function
	 *    name, to avoid the problem of name hiding in derived classes.  (If you declare *any*
	 *    functions named 'f' in a derived class, the declaration will hide *all* functions
	 *    named 'f' in the base class; if all the "visit" functions were simply called 'visit'
	 *    and you wanted to override *any* of them in a derived class, you would have to
	 *    override them all.)
	 */
	class ConstGeometryOnSphereVisitor
	{
	public:

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~ConstGeometryOnSphereVisitor() = 0;

		// Please keep these geometries ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_multi_point_on_sphere(
				MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polygon_on_sphere(
				PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{  }


	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ConstGeometryOnSphereVisitor &
		operator=(
				const ConstGeometryOnSphereVisitor &);

	};


	inline
	ConstGeometryOnSphereVisitor::~ConstGeometryOnSphereVisitor()
	{  }

}

#endif  // GPLATES_MATHS_CONSTGEOMETRYONSPHEREVISITOR_H
