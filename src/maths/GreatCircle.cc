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

#include <sstream>
#include "GreatCircle.h"
#include "UnitVector3D.h"
#include "IndeterminateResultException.h"


using namespace GPlatesMaths;

GreatCircle::GreatCircle (const UnitVector3D &v1, const UnitVector3D &v2)
{
	if (dot (v1, v2) == 0.0) {
		std::ostringstream oss;
		oss << "Attempted to calculate a great-circle from identical "
			"endpoint " << v1 << ".";
		throw IndeterminateResultException (oss.str ().c_str ());
	}

	_normal = cross (Vector3D (v1), Vector3D (v2));
}
