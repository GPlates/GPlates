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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <sstream>
#include "DirVector3D.h"
#include "ViolatedDirVectorInvariantException.h"


using namespace GPlatesMaths;

void
DirVector3D::AssertInvariantHolds() const {

	/**
	 * On a Pentium IV processor, this FP comparison should cost about
	 * 7 clock cycles.
	 *
	 * Obviously, if the comparison returns false, performance will go
	 * right out the window.
	 */
	if (_mag <= 0.0) {

		// invariant has been violated
		std::ostringstream oss("DirVector3D has magnitude ");
		oss << _mag;
		throw ViolatedDirVectorInvariantException(oss.str().c_str());
	}
}
