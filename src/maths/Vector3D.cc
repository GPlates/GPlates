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

#include "Vector3D.h"


using namespace GPlatesMaths;


Vector3D
GPlatesMaths::cross(Vector3D v1, Vector3D v2) {

	real_t x_comp = v1.y() * v2.z() - v1.z() * v2.y();
	real_t y_comp = v1.z() * v2.x() - v1.x() * v2.z();
	real_t z_comp = v1.x() * v2.y() - v1.y() * v2.x();

	return Vector3D(x_comp, y_comp, z_comp);
}
