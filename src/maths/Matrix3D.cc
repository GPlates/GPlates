/* $Id$ */

/**
 * \file 
 * No file specific comments.
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
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#include <cmath>  /* std::{cos,sin}() */
#include "Matrix3D.h"

using namespace GPlatesMaths;

Matrix3D
Matrix3D::CreateRotationMatrix(const PointOnSphere& E, const real_t& phi)
{
	Matrix3D m;

	// ...
	
	return m;
}

inline Matrix3D
Matrix3D::CreateRotationMatrix(const Rotation& rot)
{ 
	return CreateRotationMatrix(rot.GetPole(), rot.GetAngle());
}

