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

#include "LineData.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

LineData::LineData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw,
	const std::string &first_header_line,
	const std::string &second_header_line,
	const Attributes_t& attrs, 
	const GPlatesMaths::PolylineOnSphere& line)
	: DrawableData(dt, id, tw, first_header_line, second_header_line,
	   attrs), _line(line)
{  }


void
LineData::Draw() {

	GPlatesState::Layout::InsertLineDataPos(this, _line);
}


void
LineData::RotateAndDraw(const GPlatesMaths::FiniteRotation &rot) {

	GPlatesMaths::PolylineOnSphere rot_line = (rot * _line);
	GPlatesState::Layout::InsertLineDataPos(this, rot_line);
}


GPlatesMaths::real_t
GPlatesGeo::LineData::proximity(
 const GPlatesMaths::PointOnSphere &pos) const {

	/* 
	 * XXX This is only a stub calculation.
	 */

	using namespace GPlatesMaths;

	/*
	 * For each great circle arc...
	 */
	PolylineOnSphere::const_iterator 
	 i = _line.begin(), end = _line.end();

	for ( ; i != end; ++i) {
		
		/*
		 * Project pos onto *i.
		 */
		
		/*
		 * If proj(pos) is outside *i, return 
		 *   min(dist(pos, i->start_point), dist(pos, i->end_point))
		 */

		/* Else (proj(pos) is inside *i), return
		 *   dist(pos, proj(pos))
		 */
	}
	
	return dot(pos.position_vector(), _line.begin()->start_point().position_vector());
}
