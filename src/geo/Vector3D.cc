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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include "Vector3D.h"

using namespace GPlatesGeo;

inline
Vector3D::Vector3D(const fpdata_t& x, const fpdata_t& y, const fpdata_t& z)
	: GeneralisedData(), _x(x), _y(y), _z(z) 
{ }

inline void
Vector3D::PrintOut(std::ostream& os) const 
{
	os << GetX() << " " << GetY() << " " << GetZ();
}
