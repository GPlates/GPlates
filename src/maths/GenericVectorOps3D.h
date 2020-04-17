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

#ifndef GPLATES_MATHS_GENERICVECTOROPS3D_H
#define GPLATES_MATHS_GENERICVECTOROPS3D_H

#include "types.h"  /* real_t */

namespace GPlatesMaths {

	namespace GenericVectorOps3D {

		template< typename V1, typename V2 >
		inline
		const real_t
		dot(
		 const V1 &v1,
		 const V2 &v2) {

			// Removing temporary variables and using double instead of real_t generates
			// significantly more efficient assembly code (in MSVC2005 reduces from
			// 41 instructions to 12 and allows the code to be inlined saving the call overhead).
			return
				v1.x().dval() * v2.x().dval() +
				v1.y().dval() * v2.y().dval() +
				v1.z().dval() * v2.z().dval();
		}


		template< typename V >
		inline
		const V
		negate(
		 const V &v) {

			return V(-v.x().dval(), -v.y().dval(), -v.z().dval());
		}


		template< typename V1, typename V2 >
		inline
		bool
		perpendicular(
		 const V1 &v1,
		 const V2 &v2) {

			return (abs(dot(v1, v2)) <= 0.0);
		}


		template< typename R >
		struct ReturnType {

			template< typename V1, typename V2 >
			static inline
			const R
			cross(
			 const V1 &v1,
			 const V2 &v2) {

				// Removing temporary variables and using double instead of real_t generates
				// significantly more efficient assembly code (in MSVC2005 reduces from
				// 89 instructions to 29).
				return R(
				 v1.y().dval() * v2.z().dval() - v1.z().dval() * v2.y().dval(),
				 v1.z().dval() * v2.x().dval() - v1.x().dval() * v2.z().dval(),
				 v1.x().dval() * v2.y().dval() - v1.y().dval() * v2.x().dval());
			}


			template< typename V >
			static inline
			const R
			scale(
			 const real_t s,
			 const V v) {

				return R(s.dval() * v.x().dval(), s.dval() * v.y().dval(), s.dval() * v.z().dval());
			}

		};

	}

}

#endif  // GPLATES_MATHS_GENERICVECTOROPS3D_H
