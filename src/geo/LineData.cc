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

#include "LineData.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

LineData::LineData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw, const Attributes_t& attrs, 
	const GPlatesMaths::PolyLineOnSphere& line)
	: DataOnSphere(dt, id, tw, attrs), _line(line)
{  }


void
LineData::Rotate(const GPlatesMaths::FiniteRotation &rot) const {

	GPlatesMaths::PolyLineOnSphere plos = (rot * GetPolyLineOnSphere());
	GPlatesState::Layout::InsertLineDataPos(this, plos);
}
