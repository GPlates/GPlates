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

#include "PointData.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

PointData::PointData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw,
	const std::string &first_header_line,
	const std::string &second_header_line,
	const Attributes_t& attrs, 
	const GPlatesMaths::PointOnSphere& point)
	: DrawableData(dt, id, tw, first_header_line, second_header_line,
	   attrs), _point(point)
{ }


void
PointData::Draw() {

	GPlatesState::Layout::InsertPointDataPos(this, _point);
}


void
PointData::RotateAndDraw(const GPlatesMaths::FiniteRotation &rot) {

	GPlatesMaths::PointOnSphere rot_point = (rot * _point);
	GPlatesState::Layout::InsertPointDataPos(this, rot_point);
}
