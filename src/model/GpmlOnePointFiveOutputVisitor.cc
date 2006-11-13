/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "GpmlOnePointFiveOutputVisitor.h"
#include "model/FeatureHandle.h"
#include "model/FeatureRevision.h"
#include "model/GmlLineString.h"
#include "model/GmlOrientableCurve.h"
#include "model/GmlTimeInstant.h"
#include "model/GmlTimePeriod.h"
#include "model/GpmlConstantValue.h"
#include "model/GpmlPlateId.h"
#include "model/SingleValuedPropertyContainer.h"
#include "model/XsString.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle) {

	// If a feature handle doesn't contain a revision, we pretend the handle doesn't exist.
	if (feature_handle.current_revision() == NULL) {
		return;
	}

	XmlOutputInterface::ElementPairStackFrame f1(d_output, feature_handle.feature_type().get());
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:identity");
		d_output.write_line_of_string_content(feature_handle.feature_id().get());
	}
	feature_handle.current_revision()->accept_visitor(*this);
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_feature_revision(
		const GPlatesModel::FeatureRevision &feature_revision) {

	{
		XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:revision");
		d_output.write_line_of_string_content(feature_revision.revision_id().get());
	}

	// Now visit each of the properties in turn.
	std::vector<boost::intrusive_ptr<GPlatesModel::PropertyContainer> >::const_iterator iter =
			feature_revision.properties().begin();
	std::vector<boost::intrusive_ptr<GPlatesModel::PropertyContainer> >::const_iterator end =
			feature_revision.properties().end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_line_string(
		const GPlatesModel::GmlLineString &gml_line_string) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:LineString");
	static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > pos_list_xml_attrs;
	if (pos_list_xml_attrs.empty()) {
		GPlatesModel::XmlAttributeName attr_name("dimension");
		GPlatesModel::XmlAttributeValue attr_value("2");
		pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
	}
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:posList",
			pos_list_xml_attrs.begin(),
			pos_list_xml_attrs.end());

	// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
	// were to create an iterator which performed this transformation for us automatically, but
	// (i) that's probably not the most efficient use of our time right now; (ii) it's file
	// I/O, it's slow anyway; and (iii) we can cut it down to a single memory allocation if we
	// reserve the size of the vector in advance.
	boost::intrusive_ptr<const GPlatesMaths::PolylineOnSphere> polyline_ptr = gml_line_string.polyline();
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
		GPlatesMaths::LatLonPoint llp =
				GPlatesMaths::LatLonPointConversions::convertPointOnSphereToLatLonPoint(*iter);

		pos_list.push_back(llp.latitude().dval());
		pos_list.push_back(llp.longitude().dval());
	}
	d_output.write_line_of_decimal_content(pos_list.begin(), pos_list.end());
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_orientable_curve(
		const GPlatesModel::GmlOrientableCurve &gml_orientable_curve) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:OrientableCurve",
			gml_orientable_curve.xml_attributes().begin(),
			gml_orientable_curve.xml_attributes().end());
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:baseCurve");
	// FIXME:  Should we throw an exception if this value is NULL?
	if (gml_orientable_curve.base_curve() != NULL) {
		gml_orientable_curve.base_curve()->accept_visitor(*this);
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_time_instant(
		const GPlatesModel::GmlTimeInstant &gml_time_instant) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:TimeInstant");
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:timePosition",
			gml_time_instant.time_position_xml_attributes().begin(),
			gml_time_instant.time_position_xml_attributes().end());

	const GPlatesModel::GeoTimeInstant &time_position = gml_time_instant.time_position();
	if (time_position.is_real()) {
		d_output.write_line_of_decimal_content(time_position.value());
	} else if (time_position.is_distant_past()) {
		d_output.write_line_of_string_content("http://gplates.org/times/distantPast");
	} else if (time_position.is_distant_future()) {
		d_output.write_line_of_string_content("http://gplates.org/times/distantFuture");
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gml_time_period(
		const GPlatesModel::GmlTimePeriod &gml_time_period) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:TimePeriod");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:begin");
		// FIXME:  Should we throw an exception if this value is NULL?
		if (gml_time_period.begin() != NULL) {
			gml_time_period.begin()->accept_visitor(*this);
		}
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:end");
		// FIXME:  Should we throw an exception if this value is NULL?
		if (gml_time_period.end() != NULL) {
			gml_time_period.end()->accept_visitor(*this);
		}
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_constant_value(
		const GPlatesModel::GpmlConstantValue &gpml_constant_value) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:ConstantValue");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:value");
		// FIXME:  Should we throw an exception if this value is NULL?
		if (gpml_constant_value.value() != NULL) {
			gpml_constant_value.value()->accept_visitor(*this);
		}
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_plate_id(
		const GPlatesModel::GpmlPlateId &gpml_plate_id) {
	d_output.write_line_of_integer_content(gpml_plate_id.value());
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_single_valued_property_container(
		const GPlatesModel::SingleValuedPropertyContainer &single_valued_property_container) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, single_valued_property_container.property_name().get(),
			single_valued_property_container.xml_attributes().begin(),
			single_valued_property_container.xml_attributes().end());

	// FIXME:  Should we bother checking whether the value is optional?
	if (single_valued_property_container.value() != NULL) {
		single_valued_property_container.value()->accept_visitor(*this);
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_xs_string(
		const GPlatesModel::XsString &xs_string) {
	d_output.write_line_of_string_content(xs_string.value().get());
}
