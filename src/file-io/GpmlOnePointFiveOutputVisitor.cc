/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#include "GpmlOnePointFiveOutputVisitor.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/XsString.h"

#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPoint.h"


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle) {

	XmlOutputInterface::ElementPairStackFrame f1(d_output, feature_handle.feature_type().get_name());
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:identity");
		d_output.write_line_of_string_content(feature_handle.feature_id().get());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f3(d_output, "gpml:revision");
		d_output.write_line_of_string_content(feature_handle.revision_id().get());
	}

	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_top_level_property_inline(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output,
			top_level_property_inline.property_name().get_name(),
			top_level_property_inline.xml_attributes().begin(),
			top_level_property_inline.xml_attributes().end());

	visit_property_values(top_level_property_inline);
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:LineString");
	static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > pos_list_xml_attrs;
	if (pos_list_xml_attrs.empty()) {
		GPlatesModel::XmlAttributeName attr_name = 
			GPlatesModel::XmlAttributeName::create_gml("dimension");
		GPlatesModel::XmlAttributeValue attr_value("2");
		pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
	}
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:posList",
			pos_list_xml_attrs.begin(),
			pos_list_xml_attrs.end());

	// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
	// were to create an iterator which performed the following transformation for us
	// automatically, but (i) that's probably not the most efficient use of our time right now;
	// (ii) it's file I/O, it's slow anyway; and (iii) we can cut it down to a single memory
	// allocation if we reserve the size of the vector in advance.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.polyline();
	std::vector<double> pos_list;
	// Reserve enough space for the coordinates, to avoid the need to reallocate.
	//
	// number of coords = 
	//   (one for each segment start-point, plus one for the final end-point
	//   (all other end-points are the start-point of the next segment, so are not counted)),
	//   times two, since each point is a (lat, lon) duple.
	pos_list.reserve((polyline_ptr->number_of_segments() + 1) * 2);

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_ptr->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_ptr->vertex_end();
	for ( ; iter != end; ++iter) {
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);

		pos_list.push_back(llp.longitude());
		pos_list.push_back(llp.latitude());
	}
	d_output.write_line_of_multi_decimal_content(pos_list.begin(), pos_list.end());

	// Don't forget to clear the vector when we're done with it!
	pos_list.clear();
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:OrientableCurve",
			gml_orientable_curve.xml_attributes().begin(),
			gml_orientable_curve.xml_attributes().end());
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:baseCurve");
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:Point");
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:pos");

	const GPlatesMaths::PointOnSphere &pos = *gml_point.point();
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);
	d_output.write_line_of_decimal_duple_content(llp.longitude(), llp.latitude());
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:TimeInstant");
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:timePosition",
			gml_time_instant.time_position_xml_attributes().begin(),
			gml_time_instant.time_position_xml_attributes().end());

	const GPlatesPropertyValues::GeoTimeInstant &time_position = gml_time_instant.time_position();
	if (time_position.is_real()) {
		d_output.write_line_of_single_decimal_content(time_position.value());
	} else if (time_position.is_distant_past()) {
		d_output.write_line_of_string_content("http://gplates.org/times/distantPast");
	} else if (time_position.is_distant_future()) {
		d_output.write_line_of_string_content("http://gplates.org/times/distantFuture");
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:TimePeriod");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:begin");
		gml_time_period.begin()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:end");
		gml_time_period.end()->accept_visitor(*this);
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:ConstantValue");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:value");
		gpml_constant_value.value()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_constant_value.value_type().get_name());
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation) {
	if (gpml_finite_rotation.is_zero_rotation()) {
		d_output.write_empty_element("gpml:ZeroFiniteRotation");
	} else {
		XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:AxisAngleFiniteRotation");
		{
			XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:eulerPole");
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
					::GPlatesPropertyValues::calculate_euler_pole(gpml_finite_rotation);
			visit_gml_point(*gml_point);
		}
		{
			XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:angle");
			GPlatesMaths::real_t angle_in_radians =
					::GPlatesPropertyValues::calculate_angle(gpml_finite_rotation);
			double angle_in_degrees =
					::GPlatesMaths::degreesToRadians(angle_in_radians).dval();
			d_output.write_line_of_single_decimal_content(angle_in_degrees);
		}
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:FiniteRotationSlerp");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_finite_rotation_slerp.value_type().get_name());
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:IrregularSampling");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:timeSamples");
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
				gpml_irregular_sampling.time_samples().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
				gpml_irregular_sampling.time_samples().end();
		for ( ; iter != end; ++iter) {
			write_gpml_time_sample(*iter);
		}
	}
	// The interpolation function is optional.
	if (gpml_irregular_sampling.interpolation_function() != NULL) {
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:interpolationFunction");
		gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_irregular_sampling.value_type().get_name());
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id) {
	d_output.write_line_of_single_integer_content(gpml_plate_id.value());
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::write_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:TimeSample");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:value");
		gpml_time_sample.value()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:validTime");
		gpml_time_sample.valid_time()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:description");
		// The description is optional.
		if (gpml_time_sample.description() != NULL) {
			gpml_time_sample.description()->accept_visitor(*this);
		}
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_time_sample.value_type().get_name());
	}
}

void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:OldPlatesHeader");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:regionNumber");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.region_number());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:referenceNumber");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.reference_number());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:stringNumber");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.string_number());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:geographicDescription");
 		d_output.write_line_of_string_content(gpml_old_plates_header.geographic_description());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:plateIdNumber");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.plate_id_number());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:ageOfAppearance");
 		d_output.write_line_of_single_decimal_content(gpml_old_plates_header.age_of_appearance());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:ageOfDisappearance");
 		d_output.write_line_of_single_decimal_content(gpml_old_plates_header.age_of_disappearance());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:dataTypeCode");
 		d_output.write_line_of_string_content(gpml_old_plates_header.data_type_code());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:dataTypeCodeNumber");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.data_type_code_number());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:dataTypeCodeNumberAdditional");
 		d_output.write_line_of_string_content(gpml_old_plates_header.data_type_code_number_additional());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:conjugatePlateIdNumber");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.conjugate_plate_id_number());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:colourCode");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.colour_code());
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:numberOfPoints");
 		d_output.write_line_of_single_integer_content(gpml_old_plates_header.number_of_points());
	}
}

void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string) {
	d_output.write_line_of_string_content(xs_string.value().get());
}
