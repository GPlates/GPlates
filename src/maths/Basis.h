/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 *
 * Authors:
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_BASIS_H_
#define _GPLATES_MATHS_BASIS_H_

#include "DirVector3D.h"
//#include "types.h"  /* real_t */
//#include "ViolatedDirVectorInvariantException.h"

namespace GPlatesMaths
{
	/** 
	 * A two-dimensional basis.
	 * @invariant
	 *  - constituent vectors are perpendicular
	 */
	class Basis
	{
		public:
			/**
			 * Create a basis from the specified vectors.
			 */
			explicit 
			Basis(DirVector3D &vec1, DirVector3D &vec2)
				: _v1(vec1), _v2(vec2)
			{

				AssertInvariantHolds();
			}

		protected:
			/** 
			 * Assert the class invariant.
			 * @throw ViolatedUnitVectorInvariantException
			 *   if the invariant has been violated.
			 */
			void
			AssertInvariantHolds() const;

		private:
			DirVector3D &_v1, &_v2;
	};
}

#endif  // _GPLATES_MATHS_BASIS_H_
