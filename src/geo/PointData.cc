/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
