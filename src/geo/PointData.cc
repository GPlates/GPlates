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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include "PointData.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

PointData::PointData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw, const Attributes_t& attrs, 
	const GPlatesMaths::PointOnSphere& point)
	: DrawableData(dt, id, tw, attrs), _point(point)
{ }


void
PointData::Draw() const {

	GPlatesState::Layout::InsertPointDataPos(this, _point);
}


void
PointData::RotateAndDraw(const GPlatesMaths::FiniteRotation &rot) const {

	GPlatesMaths::PointOnSphere rot_point = (rot * _point);
	GPlatesState::Layout::InsertPointDataPos(this, rot_point);
}
