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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_OPERATIONSONSPHERE_H_
#define _GPLATES_MATHS_OPERATIONSONSPHERE_H_

#include "PointOnSphere.h"
#include "UnitVector3D.h"

namespace GPlatesMaths
{
	namespace OperationsOnSphere
	{
		UnitVector3D convertLatLongToUnitVector(const real_t& latitude,
		 const real_t& longitude);
	}
}

#endif  // _GPLATES_MATHS_OPERATIONSONSPHERE_H_
