/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

			real_t x_dot = v1.x() * v2.x();
			real_t y_dot = v1.y() * v2.y();
			real_t z_dot = v1.z() * v2.z();

			return (x_dot + y_dot + z_dot);
		}


		template< typename V >
		inline
		const V
		negate(
		 const V &v) {

			return V(-v.x(), -v.y(), -v.z());
		}


		template< typename V1, typename V2 >
		inline
		bool
		perpendicular(
		 const V1 &v1,
		 const V2 &v2) {

			real_t dot_prod = dot(v1, v2);
			return (abs(dot_prod) <= 0.0);
		}


		template< typename R >
		struct ReturnType {

			template< typename V1, typename V2 >
			static inline
			const R
			cross(
			 const V1 &v1,
			 const V2 &v2) {

				real_t
				 x_comp = v1.y() * v2.z() - v1.z() * v2.y(),
				 y_comp = v1.z() * v2.x() - v1.x() * v2.z(),
				 z_comp = v1.x() * v2.y() - v1.y() * v2.x();

				return R(x_comp, y_comp, z_comp);
			}


			template< typename V >
			static inline
			const R
			scale(
			 const real_t s,
			 const V v) {

				return R(s * v.x(), s * v.y(), s * v.z());
			}

		};

	}

}

#endif  // GPLATES_MATHS_GENERICVECTOROPS3D_H
