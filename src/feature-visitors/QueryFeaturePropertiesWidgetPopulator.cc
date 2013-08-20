/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2010, 2012 Geological Survey of Norway
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

#include <boost/bind.hpp>
#include <QLocale>

#include "QueryFeaturePropertiesWidgetPopulator.h"

#include "app-logic/ReconstructionGeometryUtils.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/Enumeration.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlStringList.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "utils/UnicodeStringUtils.h"
//#include "utils/Profile.h"


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::populate(
		GPlatesModel::FeatureHandle::const_weak_ref &feature,
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_rg)
{
	d_tree_widget_ptr->clear();

	d_tree_widget_builder.reset();

	// Visit the feature handle.
	if (focused_rg)
	{
		// The focused geometry property will be expanded but the others won't.
		// This serves two purposes:
		//   1) highlights to the user which geometry (of the feature) is in focus.
		//   2) serves a dramatic optimisation for large number of geometries in feature.
		boost::optional<GPlatesModel::FeatureHandle::iterator> focused_geometry_property =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
						focused_rg);
		if (focused_geometry_property)
		{
			d_focused_geometry = focused_geometry_property.get();
		}
	}
	visit_feature(feature);

	// Now that we've accumulated the tree widget item hierarchy we can
	// add the hierarchy to Qt efficiently by adding all children of each
	// tree widget item in one call using a QList.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
}


bool
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	const QString name = convert_qualified_xml_name_to_qstring(top_level_property_inline.get_property_name());

	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, name);

#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output,
			top_level_property_inline.property_name().get(),
			top_level_property_inline.xml_attributes().begin(),
			top_level_property_inline.xml_attributes().end());
#endif

	// If the current property is the focused geometry then scroll to it
	// so the user can see it.
	if (d_focused_geometry == current_top_level_propiter())
	{
		// Call QTreeWidget::scrollToItem() passing the current item, but do it later
		// when the item is attached to the QTreeWidget otherwise it will have no effect.
		d_tree_widget_builder.add_function(
				item_handle,
				boost::bind(
						&QTreeWidget::scrollToItem,
						_2, // will be the QTreeWidget that the tree widget builder uses
						_1, // will be the QTreeWidgetItem we're attaching this function to
						QAbstractItemView::EnsureVisible));
	}

	d_tree_widget_builder.push_current_item(item_handle);

	// Visit the properties.
	return true;
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::finalise_post_property_values(
		const GPlatesModel::TopLevelPropertyInline &)
{
	d_tree_widget_builder.pop_current_item();
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	static const int which_column = 1;
	QString qstring = GPlatesUtils::make_qstring_from_icu_string(enumeration.get_value().get());

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	// The focused geometry property will be expanded but the others won't.
	// This serves two purposes:
	//   1) highlights to the user which geometry (of the feature) is in focus.
	//   2) serves a dramatic optimisation for large number of geometries in feature.
	if (d_focused_geometry == current_top_level_propiter())
	{
		// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
		// when the item is attached to the QTreeWidget otherwise it will have no effect.
		add_function_to_current_item(d_tree_widget_builder,
				boost::bind(
						&QTreeWidgetItem::setExpanded,
						_1, // will be the QTreeWidgetItem we're attaching this function to
						true));
	}

	// First, add a branch for the "gml:posList".
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:posList"));

	// Now, hang the coords (in (lon, lat) format, since that is how GML does things) off the
	// "gml:posList" branch.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.get_polyline();

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
		
		add_child(d_tree_widget_builder, item_handle, point_id, point);
	}
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	// FIXME: Check if the following is the appropriate form of output.
	// Do we want a multi-point to look more like a polyline here, or do 
	// we want to wrap each (lat,lon) pair inside "gml:pos", which is inside a "gml:pointMember"?
	// I'm going to implement the latter here. 
	//
	// I'm following the nested structure that you get in an exported gpml file, except I'm missing
	// out the "gpml:value / gpml:ConstantValue" part, to be consistent with other geometries. 
	//
	// So I'll have something like this:
	// - unclassifiedGeometry (this is taken care of outside this function)
	//   - gml:MultiPoint # 1
	//     - gml:pos
	//       - (lat;lon)			<lat>;<lon>
	//	 - gml:MultiPoint # 2
	//     - gml:pos
	//       - (lat;lon)			<lat>;<lon>
	//
	// 

	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type 
		multi_point_ptr = gml_multi_point.get_multipoint();

	GPlatesMaths::MultiPointOnSphere::const_iterator iter = multi_point_ptr->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multi_point_ptr->end();

	for (unsigned point_number = 1; iter != end ; ++iter, ++point_number)
	{


		QLocale locale;

		QString point_member(QObject::tr("gml:pointMember "));
		point_member.append(QObject::tr("#"));
		point_member.append(locale.toString(point_number));

		const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
				add_child_to_current_item(d_tree_widget_builder, point_member);

		d_tree_widget_builder.push_current_item(item_handle);

		write_multipoint_member(*iter);

		d_tree_widget_builder.pop_current_item();

	}
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	// The focused geometry property will be expanded but the others won't.
	// This serves two purposes:
	//   1) highlights to the user which geometry (of the feature) is in focus.
	//   2) serves a dramatic optimisation for large number of geometries in feature.
	if (d_focused_geometry == current_top_level_propiter())
	{
		// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
		// when the item is attached to the QTreeWidget otherwise it will have no effect.
		add_function_to_current_item(d_tree_widget_builder,
				boost::bind(
						&QTreeWidgetItem::setExpanded,
						_1, // will be the QTreeWidgetItem we're attaching this function to
						true));
	}

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
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{
#if 0
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gml:Point");
	XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:pos");

	const GPlatesMaths::PointOnSphere &pos = *gml_point.point();
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

	d_output.write_line_of_decimal_duple_content(llp.longitude().dval(), llp.latitude().dval());
#endif

	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// First, add a branch for the "gml:posList".
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:position"));

	// Now, hang the coords (in (lon, lat) format, since that is how GML does things) off the
	// "gml:posList" branch.

	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*(gml_point.get_point()));
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

	add_child(d_tree_widget_builder, item_handle, point_id, point);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// FIXME: Check if the following is the appropriate form of output.
	// I'm going to export this in the same form as a gml:polygon appears in an 
	// exported gpml file, excluding the "gpml:value / gpml:ConstantValue" terms, to 
	// be consistent with other geometries.
	//
	// So we'll have
	// - gml:exterior 
	// 	- gml:posList
	//		- #1 (lat;lon)		<lat> ; <lon>
	//		- #2 (lat;lon)		<lat> ; <lon>
	// - gml:interior # 1
	//   - gml:posList
	//		- #1 (lat;lon)		<lat> ; <lon>
	//		- #2 (lat;lon)		<lat> ; <lon>	
	//

	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// First, add a branch for the "gml:exterior".
	const GPlatesGui::TreeWidgetBuilder::item_handle_type exterior_item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:exterior"));

	d_tree_widget_builder.push_current_item(exterior_item_handle);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr =
			gml_polygon.get_exterior();

	write_polygon_ring(polygon_ptr);

	d_tree_widget_builder.pop_current_item();

	// Now handle any internal rings.
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator iter = gml_polygon.get_interiors().begin();
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator end = gml_polygon.get_interiors().end();

	for (unsigned ring_number = 1; iter != end ; ++iter, ++ring_number)
	{
		QString interior;
		interior.append(QObject::tr("gml:interior"));
		interior.append(QObject::tr(" #"));
		interior.append(QString().setNum(ring_number));

		const GPlatesGui::TreeWidgetBuilder::item_handle_type interior_item_handle =
				add_child_to_current_item(d_tree_widget_builder, interior);

		d_tree_widget_builder.push_current_item(interior_item_handle);

		write_polygon_ring(*iter);

		d_tree_widget_builder.pop_current_item();
	}
	
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	static const int which_column = 1;
	QLocale locale;
	QString qstring;

	const GPlatesPropertyValues::GeoTimeInstant &time_position = gml_time_instant.get_time_position();
	if (time_position.is_real()) {
		qstring = locale.toString(time_position.value());
	} else if (time_position.is_distant_past()) {
		qstring = QObject::tr("distant past");
	} else if (time_position.is_distant_future()) {
		qstring = QObject::tr("distant future");
	}

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
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
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// FIXME:  Ensure that 'gml_time_period.begin()' and 'gml_time_period.end()' are not NULL.
	add_child_then_visit_value(QObject::tr("gml:begin"), QString(), *gml_time_period.begin());
	add_child_then_visit_value(QObject::tr("gml:end"), QString(), *gml_time_period.end());
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.get_value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_finite_rotation(
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
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_finite_rotation_slerp(
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
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_irregular_sampling(
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
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// FIXME:  Should that be "gpml:element" rather than "gpml:elements" in the KeyValueDictionary?
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:elements"));
	d_tree_widget_builder.push_current_item(item_handle);

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
		iter = gpml_key_value_dictionary.get_elements().begin(),
		end = gpml_key_value_dictionary.get_elements().end();
	for ( ; iter != end; ++iter) {
		add_gpml_key_value_dictionary_element(*iter);
	}
	d_tree_widget_builder.pop_current_item();
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const int which_column = 1;
	QString qstring = QString::number(gpml_plate_id.get_value());

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_measure(
	const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	static const int which_column = 1;
	QString qstring = QString::number(gpml_measure.get_quantity());

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}

#if 0
void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_time_sample(
		GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
{
	XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:TimeSample");
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:value");
		gpml_time_sample.get_value()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:validTime");
		gpml_time_sample.get_valid_time()->accept_visitor(*this);
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gml:description");
		// The description is optional.
		if (gpml_time_sample.get_description() != NULL) {
			gpml_time_sample.get_description()->accept_visitor(*this);
		}
	}
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:valueType");
		d_output.write_line_of_string_content(gpml_time_sample.get_value_type().get());
	}
}
#endif

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	QLocale locale;
	const GPlatesPropertyValues::GpmlOldPlatesHeader &header = gpml_old_plates_header;
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:regionNumber"), locale.toString(header.get_region_number()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:referenceNumber"), QString::number(header.get_reference_number()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:stringNumber"), QString::number(header.get_string_number()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:geographicDescription"),
			GPlatesUtils::make_qstring_from_icu_string(header.get_geographic_description()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:plateIdNumber"), QString::number(header.get_plate_id_number()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:ageOfAppearance"), locale.toString(header.get_age_of_appearance()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:ageOfDisappearance"), locale.toString(header.get_age_of_disappearance()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:dataTypeCode"),
			GPlatesUtils::make_qstring_from_icu_string(header.get_data_type_code()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:dataTypeCodeNumber"), QString::number(header.get_data_type_code_number()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:dataTypeCodeNumberAdditional"),
			GPlatesUtils::make_qstring_from_icu_string(header.get_data_type_code_number_additional()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:conjugatePlateIdNumber"), QString::number(header.get_conjugate_plate_id_number()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:colourCode"), QString::number(header.get_colour_code()));
	add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:numberOfPoints"), QString::number(header.get_number_of_points()));
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_string_list(
		const GPlatesPropertyValues::GpmlStringList &gpml_string_list)
{
	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// First, add a branch for the "gpml:element".
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:element"));
	d_tree_widget_builder.push_current_item(item_handle);

	const GPlatesPropertyValues::GpmlStringList::string_list_type &string_list =
			gpml_string_list.get_string_list();

	GPlatesPropertyValues::GpmlStringList::string_list_type::const_iterator iter = string_list.begin();
	GPlatesPropertyValues::GpmlStringList::string_list_type::const_iterator end = string_list.end();
	for (unsigned elem_number = 1; iter != end; ++iter, ++elem_number) {
		QLocale locale;
		QString elem_id(QObject::tr("#"));
		elem_id.append(locale.toString(elem_number));
		QString elem = GPlatesUtils::make_qstring_from_icu_string(iter->get());

		add_child(d_tree_widget_builder, item_handle, elem_id, elem);
	}
	d_tree_widget_builder.pop_current_item();
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_uninterpreted_property_value(
		const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val)
{
	static const int which_column = 1;
	const QString qstring("<uninterpreted>");

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	static const int which_column = 1;
	QString qstring = QVariant(xs_boolean.get_value()).toString();

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_xs_double(
	const GPlatesPropertyValues::XsDouble& xs_double)
{
	static const int which_column = 1;

	QLocale locale;

	QString qstring = locale.toString(xs_double.get_value());

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_xs_integer(
	const GPlatesPropertyValues::XsInteger& xs_integer)
{
	static const int which_column = 1;

	QLocale locale;

	QString qstring = locale.toString(xs_integer.get_value());

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	static const int which_column = 1;
	QString qstring = GPlatesUtils::make_qstring(xs_string.get_value());

	// This assumes that the stack is non-empty.
	get_current_qtree_widget_item(d_tree_widget_builder)->setText(which_column, qstring);
}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::add_child_then_visit_value(
		const QString &name,
		const QString &value,
		const GPlatesModel::PropertyValue &property_value_to_visit)
{
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, name, value);

	d_tree_widget_builder.push_current_item(item_handle);
	property_value_to_visit.accept_visitor(*this);
	d_tree_widget_builder.pop_current_item();
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::add_gpml_key_value_dictionary_element(
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
{

	QString key_string = GPlatesUtils::make_qstring_from_icu_string(element.key()->get_value().get());

	add_child_then_visit_value(key_string,QString(),*element.value());

}


void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::write_polygon_ring(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr)
{
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:posList"));

	// Now, hang the coords (in (lon, lat) format, since that is how GML does things) off the
	// "gml:posList" branch.

	GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = polygon_ptr->vertex_begin();
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon_ptr->vertex_end();

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

		add_child(d_tree_widget_builder, item_handle, point_id, point);
	}
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::write_multipoint_member(
	const GPlatesMaths::PointOnSphere &point)
{

		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);	
		QString gml_pos;
		gml_pos.append(QObject::tr("gml:pos"));

		const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
				add_child_to_current_item(d_tree_widget_builder, gml_pos);

		QString pos;
		pos.append(QObject::tr(" (lat ; lon)"));
	
		QLocale locale;

		QString lat = locale.toString(llp.latitude());
		QString lon = locale.toString(llp.longitude());
		QString point_string;
		point_string.append(lat);
		point_string.append(QObject::tr(" ; "));
		point_string.append(lon);

		add_child(d_tree_widget_builder, item_handle, pos, point_string);
}

void
GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator::visit_gpml_array(
		const GPlatesPropertyValues::GpmlArray &gpml_array)
{
	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
		boost::bind(
		&QTreeWidgetItem::setExpanded,
		_1, // will be the QTreeWidgetItem we're attaching this function to
		true));

	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
		add_child_to_current_item(d_tree_widget_builder, QObject::tr("gpml:members"));
	d_tree_widget_builder.push_current_item(item_handle);

	std::vector<GPlatesPropertyValues::GpmlArray::Member>::const_iterator 
		iter = gpml_array.get_members().begin(),
		end = gpml_array.get_members().end();
	for ( ; iter != end; ++iter) {
		iter->get_value()->accept_visitor(*this);
	}
	d_tree_widget_builder.pop_current_item();
}
