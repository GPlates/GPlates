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
#include "Basis.h"
#include "DirVector3D.h"
#include "ViolatedBasisInvariantException.h"
#include "types.h"


using namespace GPlatesMaths;

void
Basis::AssertInvariantHolds() const
{
	real_t dp = dot (_v1, _v2);

	if (dp != 0.0)
		throw ViolatedBasisInvariantException
				("Basis vectors are not perpendicular");
}
