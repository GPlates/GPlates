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

#ifndef _GPLATES_MATHS_GREATCIRCLE_H_
#define _GPLATES_MATHS_GREATCIRCLE_H_

#include "types.h"
#include "UnitVector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A great-circle on the surface of a sphere.
	 */
	class GreatCircle
	{
		public:
			GreatCircle (UnitVector3D &normal_vec)
				: _normal (normal_vec) { }
			GreatCircle (const UnitVector3D &v1, const UnitVector3D &v2);

			UnitVector3D normal () const { return _normal; }

		private:
			UnitVector3D _normal;
	};
}

#endif  // _GPLATES_MATHS_GREATCIRCLE_H_
