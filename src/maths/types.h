/* $Id$ */

/**
 * \file 
 * This file contains typedefs that are used in various places in
 * the code; they are not tied to any particular class.
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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_TYPES_H_
#define _GPLATES_MATHS_TYPES_H_

#include <utility>  // std::pair
#include "geo/Vector2D.h"
#include "PointOnSphere.h"
#include "Real.h"

namespace GPlatesMaths
{
	/**
	 * A two-dimensional basis.
	 * This typedef is a placeholder for what will be a fully
	 * fleshed-out class in the end.
	 */
	typedef std::pair<GPlatesGeo::Vector2D, GPlatesGeo::Vector2D> Basis2D_t;

	/**
	 * A floating-point approximation to the field of reals.
	 */
	typedef Real real_t;

	/**
	 * A collection of points on the globe to be draw with line
	 * segments between them.
	 */
	typedef std::list<PointOnSphere> PolyLineOnSphere;
}

#endif  // _GPLATES_MATHS_TYPES_H_
