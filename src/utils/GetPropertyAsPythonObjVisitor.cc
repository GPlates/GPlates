/* $Id: GetPropertyAsPythonObjVisitor.h 11519 2011-05-12 05:45:47Z mchin $ */

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

#include "GetPropertyAsPythonObjVisitor.h"

#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlDataBlock.h"
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
#include "property-values/GpmlStringList.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#if !defined(GPLATES_NO_PYTHON)


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_data_block(
		gml_data_block_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_plate_id(
		gpml_plate_id_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_time_period(
		gml_time_period_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_enumeration(
		enumeration_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

QString
GPlatesUtils::GetPropertyAsPythonObjVisitor::to_qstring(
		const GPlatesModel::PropertyValue& data)
{
	std::stringstream ss;
	data.print_to(ss);
	return QString(ss.str().c_str());
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_line_string(
		gml_line_string_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_multi_point(
		gml_multi_point_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_orientable_curve(
			gml_orientable_curve_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_point(
			gml_point_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_polygon(
			gml_polygon_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gml_time_instant(
		gml_time_instant_type &v)
{
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_feature_reference(
		gpml_feature_reference_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_feature_snapshot_reference(
		gpml_feature_snapshot_reference_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_finite_rotation(
		gpml_finite_rotation_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_finite_rotation_slerp(
		gpml_finite_rotation_slerp_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_hot_spot_trail_mark(
		gpml_hot_spot_trail_mark_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_irregular_sampling(
		gpml_irregular_sampling_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_key_value_dictionary(
		gpml_key_value_dictionary_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_measure(
		gpml_measure_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_old_plates_header(
		gpml_old_plates_header_type &v) 
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_piecewise_aggregation(
		gpml_piecewise_aggregation_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_polarity_chron_id(
		gpml_polarity_chron_id_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_property_delegate(
		gpml_property_delegate_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_revision_id(
		gpml_revision_id_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_topological_polygon(
		gpml_topological_polygon_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}
		

void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_topological_line_section(
		gpml_topological_line_section_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_topological_intersection(
		gpml_topological_intersection_type &v)
{ 
	//TODO:

	// it is not a property value?
	d_val = bp::str("Not implement yet -- gpml_topological_intersection");

// 	d_val =	
// 		GPlatesApi::PythonUtils::qstring_to_python_string(
// 				property_value_to_qstring(v));
}


void
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_gpml_topological_point(
		gpml_topological_point_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}


void 
GPlatesUtils::GetPropertyAsPythonObjVisitor::visit_uninterpreted_property_value(
		uninterpreted_property_value_type &v)
{ 
	//TODO:
	d_val =	
		GPlatesApi::PythonUtils::qstring_to_python_string(
				property_value_to_qstring(v));
}

#endif //GPLATES_NO_PYTHON




