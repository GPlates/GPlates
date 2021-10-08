/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include <QLocale>

#include "QueryFeaturePropertiesDialogPopulator.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlStrikeSlipEnumeration.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "utils/UnicodeStringUtils.h"


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_tree_widget_ptr->clear();

	{
		QStringList fields;
		fields.push_back(QObject::tr("gpml:identity"));
		fields.push_back(GPlatesUtils::make_qstring(feature_handle.feature_id()));
		d_tree_widget_ptr->addTopLevelItem(new QTreeWidgetItem(d_tree_widget_ptr, fields));
	}
	{
		QStringList fields;
		fields.push_back(QObject::tr("gpml:revision"));
		fields.push_back(GPlatesUtils::make_qstring(feature_handle.revision_id()));
		d_tree_widget_ptr->addTopLevelItem(new QTreeWidgetItem(d_tree_widget_ptr, fields));
	}

	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	QStringList fields;
	fields.push_back(GPlatesUtils::make_qstring(inline_property_container.property_name()));
	fields.push_back(QString());
	// FIXME:  This next line could result in memory leaks.
	QTreeWidgetItem *item = new QTreeWidgetItem(d_tree_widget_ptr, fields);
	d_tree_widget_ptr->addTopLevelItem(item);

	d_tree_widget_item_stack.clear();
	d_tree_widget_item_stack.push_back(item);

#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output,
			inline_property_container.property_name().get(),
			inline_property_container.xml_attributes().begin(),
			inline_property_container.xml_attributes().end());
#endif

	visit_property_values(inline_property_container);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	// First, add a branch for the "gml:posList".
	d_tree_widget_item_stack.back()->setExpanded(true);

	QTreeWidgetItem *pos_list_item = add_child(QObject::tr("gml:posList"), QString());
	d_tree_widget_item_stack.push_back(pos_list_item);

	// Now, hang the coords (in (lon, lat) format, since that is how GML does things) off the
	// "gml:posList" branch.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.polyline();

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_ptr->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_ptr->vertex_end();

	for (unsigned point_number = 1; iter != end; ++iter, ++point_number) {
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);

		QLocale locale;

		QString point_id(QObject::tr("#"));
		point_id.append(locale.toString(point_number));
		point_id.append(QObject::tr(" (lat ; lon)"));

		QString lat = locale.toString(llp.latitude());
		QString lon = locale.toString(llp.longitude());
		QString point;
		point.append(lat);
		point.append(QObject::tr(" ; "));
		point.append(lon);

		add_child(point_id, point);
	}

	d_tree_widget_item_stack.pop_back();
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	d_tree_widget_item_stack.back()->setExpanded(true);

	// FIXME:  Ensure that 'gml_orientable_curve.base_curve()' is not NULL.
	add_child_then_visit_value(QObject::tr("gml:baseCurve"), QString(),
			*gml_orientable_curve.base_curve());
#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:OrientableCurve",
			gml_orientable_curve.xml_attributes().begin(),
			gml_orientable_curve.xml_attributes().end());
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:baseCurve");
#endif
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{
#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:Point");
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:pos");

	const GPlatesMaths::PointOnSphere &pos = *gml_point.point();
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

	d_output.write_line_of_decimal_duple_content(llp.longitude().dval(), llp.latitude().dval());
#endif

	// First, add a branch for the "gml:posList".
	d_tree_widget_item_stack.back()->setExpanded(true);

	QTreeWidgetItem *pos_list_item = add_child(QObject::tr("gml:position"), QString());
	d_tree_widget_item_stack.push_back(pos_list_item);

	// Now, hang the coords (in (lon, lat) format, since that is how GML does things) off the
	// "gml:posList" branch.

	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*(gml_point.point()));
#if 0
	GPlatesMaths::LatLonPoint llp =
			GPlatesMaths::LatLonPointConversions::convertPointOnSphereToLatLonPoint(*(gml_point.point()));
#endif
	QLocale locale;

	QString point_id(QObject::tr("#"));
	
	point_id.append(QObject::tr(" (lat ; lon)"));
			
	QString lat = locale.toString(llp.latitude());
	QString lon = locale.toString(llp.longitude());
	QString point;
	point.append(lat);
	point.append(QObject::tr(" ; "));
	point.append(lon);

	add_child(point_id, point);

	d_tree_widget_item_stack.pop_back();
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	static const int which_column = 1;
	QLocale locale;
	QString qstring;

	const GPlatesPropertyValues::GeoTimeInstant &time_position = gml_time_instant.time_position();
	if (time_position.is_real()) {
		qstring = locale.toString(time_position.value());
	} else if (time_position.is_distant_past()) {
		qstring = QObject::tr("distant past");
	} else if (time_position.is_distant_future()) {
		qstring = QObject::tr("distant future");
	}

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
#if 0
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
#endif
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	d_tree_widget_item_stack.back()->setExpanded(true);

	// FIXME:  Ensure that 'gml_time_period.begin()' and 'gml_time_period.end()' are not NULL.
	add_child_then_visit_value(QObject::tr("gml:begin"), QString(), *gml_time_period.begin());
	add_child_then_visit_value(QObject::tr("gml:end"), QString(), *gml_time_period.end());
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
{
#if 0
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
#endif
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{
#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:FiniteRotationSlerp");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_finite_rotation_slerp.value_type().get());
	}
#endif
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:IrregularSampling");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:timeSamples");
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
				gpml_irregular_sampling.time_samples().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
				gpml_irregular_sampling.time_samples().end();
		for ( ; iter != end; ++iter) {
			iter->accept_visitor(*this);
		}
	}
	// The interpolation function is optional.
	if (gpml_irregular_sampling.interpolation_function() != NULL) {
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:interpolationFunction");
		gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_irregular_sampling.value_type().get());
	}
#endif
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const int which_column = 1;
	QString qstring = QString::number(gpml_plate_id.value());

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
{
#if 0
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
		d_output.write_line_of_string_content(gpml_time_sample.value_type().get());
	}
#endif
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_tree_widget_item_stack.back()->setExpanded(true);

	QLocale locale;
	const GPlatesPropertyValues::GpmlOldPlatesHeader &header = gpml_old_plates_header;
	add_child(QObject::tr("gpml:regionNumber"), locale.toString(header.region_number()));
	add_child(QObject::tr("gpml:referenceNumber"), QString::number(header.reference_number()));
	add_child(QObject::tr("gpml:stringNumber"), QString::number(header.string_number()));
	add_child(QObject::tr("gpml:geographicDescription"),
			GPlatesUtils::make_qstring_from_icu_string(header.geographic_description()));
	add_child(QObject::tr("gpml:plateIdNumber"), QString::number(header.plate_id_number()));
	add_child(QObject::tr("gpml:ageOfAppearance"), locale.toString(header.age_of_appearance()));
	add_child(QObject::tr("gpml:ageOfDisappearance"), locale.toString(header.age_of_disappearance()));
	add_child(QObject::tr("gpml:dataTypeCode"),
			GPlatesUtils::make_qstring_from_icu_string(header.data_type_code()));
	add_child(QObject::tr("gpml:dataTypeCodeNumber"), QString::number(header.data_type_code_number()));
	add_child(QObject::tr("gpml:dataTypeCodeNumberAdditional"),
			GPlatesUtils::make_qstring_from_icu_string(header.data_type_code_number_additional()));
	add_child(QObject::tr("gpml:conjugatePlateIdNumber"), QString::number(header.conjugate_plate_id_number()));
	add_child(QObject::tr("gpml:colourCode"), QString::number(header.colour_code()));
	add_child(QObject::tr("gpml:numberOfPoints"), QString::number(header.number_of_points()));
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_gpml_strike_slip_enumeration(
		const GPlatesPropertyValues::GpmlStrikeSlipEnumeration &strike_slip_enumeration)
{
	static const int which_column = 1;
	QString qstring = GPlatesUtils::make_qstring_from_icu_string(strike_slip_enumeration.value().get());

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	static const int which_column = 1;
	QString qstring = QVariant(xs_boolean.value()).toString();

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_xs_double(
	const GPlatesPropertyValues::XsDouble& xs_double)
{
	static const int which_column = 1;

	QLocale locale;

	QString qstring = locale.toString(xs_double.value());

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_xs_integer(
	const GPlatesPropertyValues::XsInteger& xs_integer)
{
	static const int which_column = 1;

	QLocale locale;

	QString qstring = locale.toString(xs_integer.value());

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	static const int which_column = 1;
	QString qstring = GPlatesUtils::make_qstring(xs_string.value());

	// This assumes that the stack is non-empty.
	d_tree_widget_item_stack.back()->setText(which_column, qstring);
}


QTreeWidgetItem *
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::add_child(
		const QString &name,
		const QString &value)
{
	QStringList fields;
	fields.push_back(name);
	fields.push_back(value);

	// FIXME:  This next line could result in memory leaks.
	QTreeWidgetItem *item = new QTreeWidgetItem(d_tree_widget_item_stack.back(), fields);
	d_tree_widget_item_stack.back()->addChild(item);

	return item;
}


QTreeWidgetItem *
GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator::add_child_then_visit_value(
		const QString &name,
		const QString &value,
		const GPlatesModel::PropertyValue &property_value_to_visit)
{
	QStringList fields;
	fields.push_back(name);
	fields.push_back(value);

	// FIXME:  This next line could result in memory leaks.
	QTreeWidgetItem *item = new QTreeWidgetItem(d_tree_widget_item_stack.back(), fields);
	d_tree_widget_item_stack.back()->addChild(item);

	d_tree_widget_item_stack.push_back(item);
	property_value_to_visit.accept_visitor(*this);
	d_tree_widget_item_stack.pop_back();

	return item;
}
