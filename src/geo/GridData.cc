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

#include "DrawableData.h"
#include "GridData.h"
#include "maths/Basis.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

GridData::GridData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw, const Attributes_t& attrs, 
	const GPlatesMaths::PointOnSphere& origin,
	const GPlatesMaths::Basis& basis)
	: DrawableData(dt, id, tw, attrs), _origin(origin), _basis(basis)
{  }


void
GridData::Draw() const {
	// TODO
	//GPlatesState::Layout::InsertLineDataPos(this, _line);
}


void
GridData::RotateAndDraw(const GPlatesMaths::FiniteRotation &rot) const {
	// TODO
	//GPlatesMaths::PolyLineOnSphere rot_line = (rot * _line);
	//GPlatesState::Layout::InsertLineDataPos(this, rot_line);
}
