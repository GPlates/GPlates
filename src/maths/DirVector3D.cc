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
 *
 */

#include <sstream>
#include "DirVector3D.h"
#include "UnitVector3D.h"
#include "ViolatedDirVectorInvariantException.h"


GPlatesMaths::UnitVector3D
GPlatesMaths::DirVector3D::normalise() const {

	real_t scale = 1 / magnitude();
	return UnitVector3D(scale * x(), scale * y(), scale * z());
}


void
GPlatesMaths::DirVector3D::AssertInvariant() const {

	/**
	 * On a Pentium IV processor, this FP comparison should cost about
	 * 7 clock cycles.
	 *
	 * Obviously, if the comparison returns false, performance will go
	 * right out the window.
	 */
	if (_mag <= 0.0) {

		// invariant has been violated
		std::ostringstream oss;
		oss << "DirVector3D has magnitude " << _mag << ".";
		throw ViolatedDirVectorInvariantException(oss.str().c_str());
	}
}
