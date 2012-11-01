/* $Id: GetValueFromPropertyVisitor.h 11519 2011-05-12 05:45:47Z mchin $ */

/**
 * \file 
 * $Revision: 11519 $
 * $Date: 2011-05-12 15:45:47 +1000 (Thu, 12 May 2011) $
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <iostream>
#include "GetValueFromPropertyVisitor.h"
#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlGridEnvelope.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlScalarField3DFile.h"
#include "property-values/GpmlStringList.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


void
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gpml_constant_value(
		gpml_constant_value_type &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gpml_plate_id(
		gpml_plate_id_type &gpml_plate_id)
{
	d_data.push_back(QString().setNum(gpml_plate_id.value()));
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_time_period(
		gml_time_period_type &gml_time_period)
{
	d_data.push_back(to_qstring(gml_time_period));
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_enumeration(
		enumeration_type &enumeration)
{
	d_data.push_back(to_qstring(enumeration));
}

QString
GPlatesDataMining::GetValueFromPropertyVisitor::to_qstring(
		const GPlatesModel::PropertyValue& data)
{
	std::stringstream ss;
	data.print_to(ss);
	return QString(ss.str().c_str());
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_line_string(
		gml_line_string_type &gml_line_string)
{
	d_data.push_back(to_qstring(gml_line_string));
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_multi_point(
		gml_multi_point_type &gml_multi_point)
{
	d_data.push_back(to_qstring(gml_multi_point));
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_orientable_curve(
			gml_orientable_curve_type &gml_orientable_curve)
{
	d_data.push_back(to_qstring(gml_orientable_curve));
}

void
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_point(
			gml_point_type &gml_point)
{
	std::stringstream ss;
	ss << gml_point.point_in_lat_lon();
	d_data.push_back(QString(ss.str().c_str()));
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_polygon(
			gml_polygon_type &gml_polygon)
{
	d_data.push_back(to_qstring(gml_polygon));
}

void 
GPlatesDataMining::GetValueFromPropertyVisitor::visit_gml_time_instant(
		gml_time_instant_type &gml_time_instant)
{
	d_data.push_back(to_qstring(gml_time_instant));
}




