/* $Id$ */

/**
 * @file 
 * Contains the definition of various function objects which may be used as
 * predicates to test for various kinds of polyline equivalence.
 *
 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular, section
 * 8.2.4 "User-Defined Function Objects for Function Adapters", for more
 * information about @a std::unary_function.
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

#ifndef GPLATES_MATHS_POLYLINEEQUIVALENCEPREDICATES_H
#define GPLATES_MATHS_POLYLINEEQUIVALENCEPREDICATES_H

#include <algorithm>  /* unary_function */
#include "PolylineOnSphere.h"

namespace GPlatesMaths {

	/**
	 * This class instantiates to a function object which determines
	 * whether a polyline is equivalent to another polyline when the
	 * directedness of the polyline segments is taken into account.
	 *
	 * This version of the class stores a @em reference to the polyline
	 * instance instead of making an internal copy.  Beware of the lifetime
	 * of the polyline instance!
	 */
	class PolylineIsDirectedEquivalentRef:
	 public std::unary_function< PolylineOnSphere, bool > {

	 public:

		PolylineIsDirectedEquivalentRef(
		 const PolylineOnSphere &poly) :
		 d_poly_ptr(&poly) {  }

		bool
		operator()(
		 const PolylineOnSphere &other_poly) const {

			return
			 polylines_are_directed_equivalent(*d_poly_ptr,
							   other_poly);
		}

	 private:

		const PolylineOnSphere *d_poly_ptr;

	};


	/**
	 * This class instantiates to a function object which determines
	 * whether a polyline is equivalent to another polyline when the
	 * directedness of the polyline segments is ignored.
	 *
	 * This version of the class stores a @em reference to the polyline
	 * instance instead of making an internal copy.  Beware of the lifetime
	 * of the polyline instance!
	 */
	class PolylineIsUndirectedEquivalentRef:
	 public std::unary_function< PolylineOnSphere, bool > {

	 public:

		PolylineIsUndirectedEquivalentRef(
		 const PolylineOnSphere &poly) :
		 d_poly_ptr(&poly) {  }

		bool
		operator()(
		 const PolylineOnSphere &other_poly) const {

			return
			 polylines_are_undirected_equivalent(*d_poly_ptr,
							     other_poly);
		}

	 private:

		const PolylineOnSphere *d_poly_ptr;

	};

}

#endif  // GPLATES_MATHS_POLYLINEEQUIVALENCEPREDICATES_H
