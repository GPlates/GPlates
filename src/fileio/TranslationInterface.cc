/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include "TranslationInterface.h"
#include "geo/DataGroup.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "maths/OperationsOnSphere.h"

using namespace GPlatesGeo;

TranslationInterface::TranslationInterface()
{
	_data = new DataGroup(NO_DATATYPE, NO_ROTATIONGROUP,
	 NO_TIMEWINDOW, NO_ATTRIBUTES);
}


static void
SetDataFromAttributes(
 const TranslationInterface::Attributes& attr,
 GPlatesGeo::GeologicalData::DataType_t& data_type,
 GPlatesGeo::GeologicalData::RotationGroupId_t& rid,
 GPlatesGeo::GeologicalData::TimeWindow& time_window,
 GPlatesGeo::GeologicalData::Attributes& attributes)
{
	TranslationInterface::Attributes::const_iterator iter = attr.Begin();
	for ( ; iter != attr.End(); ++iter)
	{
	}
}


void
TranslationInterface::RegisterLatLonPointData(
 const LatLonPoint& point,
 const Attributes&  attributes)
{
	// Will throw an InvalidLatLonException if point is invalid.
	// Implicit conversion from fpdata_t to real_t.
	GPlatesMaths::LatLonPoint llp(point.GetLatitude(),
	                              point.GetLongitude());
	GPlatesMaths::PointOnSphere pos = GPlatesMaths::
	 OperationsOnSphere::convertLatLonPointToPointOnSphere(llp);

	GPlatesGeo::GeologicalData::DataType_t        data_type;
	GPlatesGeo::GeologicalData::RotationGroupId_t rotation_group;
	GPlatesGeo::GeologicalData::TimeWindow        time_window;
	GPlatesGeo::GeologicalData::Attributes_t      attrs;

	// Throw InvalidAttributeValue
	SetDataFromAttributes(attributes,
	 data_type, rotation_group, time_window, attrs);

	PointData* pd = new PointData(
	 data_type, rotation_group, time_window, attrs, pos);

	_data->AddChild(pd);
}


