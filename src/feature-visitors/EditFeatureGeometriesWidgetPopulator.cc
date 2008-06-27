/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include <QDebug>
#include <QList>
#include <boost/none.hpp>

#include "EditFeatureGeometriesWidgetPopulator.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"
#include "model/Reconstruction.h"
#include "model/ReconstructedFeatureGeometry.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"

#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "utils/UnicodeStringUtils.h"


namespace
{
	/**
	 * Enum used to select coordinate columns.
	 */
	namespace CoordinatePeriods
	{
		enum CoordinatePeriod { PRESENT = 1, RECONSTRUCTED = 2 };
	}
	
	QTreeWidgetItem *
	make_top_level_item_for_property(
			const GPlatesModel::PropertyName &name)
	{
		QStringList fields;
		fields.push_back(GPlatesUtils::make_qstring_from_icu_string(name.build_aliased_name()));
		fields.push_back(QString());
		fields.push_back(QString());

		QTreeWidgetItem *item = new QTreeWidgetItem(fields);
		return item;
	}
	
	/**
	 * Ensures the given coordinate_widgets list has a minimum number of
	 * blank QTreeWidgetItems suitable for populating with coordinates.
	 */
	void
	fill_coordinates_with_blank_items(
			QList<QTreeWidgetItem *> &coordinate_widgets,
			GPlatesMaths::PolylineOnSphere::size_type new_size)
	{
		static QLocale locale;
		for (unsigned int i = coordinate_widgets.size(); i < new_size; ++i) {
			QString point_id(QObject::tr("#"));
			point_id.append(locale.toString(i));
			point_id.append(QObject::tr(" (lat ; lon)"));
		
			coordinate_widgets.append(new QTreeWidgetItem());
			coordinate_widgets.last()->setText(0, point_id);
		}
	}
	
	/**
	 * Iterates over the vertices of the polyline, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_polyline(
			QList<QTreeWidgetItem *> &coordinate_widgets,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline,
			CoordinatePeriods::CoordinatePeriod period)
	{
		static QLocale locale;
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline->vertex_end();
		
		// Ensure we have enough blank QTreeWidgetItems in the list to populate.
		fill_coordinates_with_blank_items(coordinate_widgets, polyline->number_of_vertices());
		
		// Then fill in the appropriate column.
		for (unsigned point_index = 0; iter != end; ++iter, ++point_index) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
			QString lat = locale.toString(llp.latitude());
			QString lon = locale.toString(llp.longitude());
			QString point;
			point.append(lat);
			point.append(QObject::tr(" ; "));
			point.append(lon);
	
			coordinate_widgets[point_index]->setText(period, point);
		}
	}

	/**
	 * Iterates over the vertices of the point, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_point(
			QList<QTreeWidgetItem *> &coordinate_widgets,
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
			CoordinatePeriods::CoordinatePeriod period)
	{
		static QLocale locale;
		// Ensure we have a blank QTreeWidgetItem in the list to populate.
		fill_coordinates_with_blank_items(coordinate_widgets, 1);
		
		// Then fill in the appropriate column.
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*point_on_sphere);
		QString lat = locale.toString(llp.latitude());
		QString lon = locale.toString(llp.longitude());
		QString point;
		point.append(lat);
		point.append(QObject::tr(" ; "));
		point.append(lon);

		coordinate_widgets[0]->setText(period, point);
	}

}


void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_tree_widget_ptr->clear();
	
	// Iterate over the Reconstruction and grab the reconstructed geometry that originates
	// from the given feature.
	populate_rfg_geometries_for_feature(feature_handle);

	// Now visit each of the properties in turn, populating d_property_info_vector
	// with QTreeWidgetItems suitable for display.
	visit_feature_properties(feature_handle);
	
	// Now add any geometric properties we were interested in (and delete the others)
	property_info_vector_const_iterator it = d_property_info_vector.begin();
	property_info_vector_const_iterator end = d_property_info_vector.end();
	for ( ; it != end; ++it) {
		if (it->item != NULL) {
			if (it->is_geometric_property) {
				// This is a geometric property and we want to add it to the widget.
				d_tree_widget_ptr->addTopLevelItem(it->item);
			} else {
				// This should hopefully delete any children of the QTreeWidgetItem,
				// although I don't believe we are adding any in this case.
				delete it->item;
			}
		}
	}
	// It turns out that doing it this way - manipulating QTreeWidgetItems that do not belong
	// to a parent QTreeWidget yet - means that we cannot set expanded/collapsed status until
	// after everything is added.
	d_tree_widget_ptr->expandAll();
}


// FIXME: I was going to implement this d_last_property_visited optional in {Const,}FeatureVisitor
// directly, since there's several visitors that currently rely on a similar member and override this
// visit_feature_properties function.
// However, I ran into problems with the forward-declaration of FeatureHandle. I am not high level
// enough to make those changes just yet.
void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			d_last_property_visited = iter;
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	// Create a top-level item for this property and remember it - do not add it just yet.
	PropertyInfo info;
	info.is_geometric_property = false;
	info.item = make_top_level_item_for_property(inline_property_container.property_name());
	d_property_info_vector.push_back(info);

	// Set up stack for building the children of this item.
	d_tree_widget_item_stack.clear();
	d_tree_widget_item_stack.push_back(info.item);

	visit_property_values(inline_property_container);
}




void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	// Fetch the parent QTreeWidgetItem we will be adding to from the stack.
	QTreeWidgetItem *parent_item = d_tree_widget_item_stack.back();
	parent_item->setExpanded(true);
	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;
	
	// First, add a branch for the type of geometry.
	QTreeWidgetItem *geom_type_item = add_child(QObject::tr("gml:LineString"), QString());
	d_tree_widget_item_stack.push_back(geom_type_item);
	geom_type_item->setExpanded(true);

	// Now, prepare the coords in present-day and reconstructed time.
	QList<QTreeWidgetItem *> coordinate_widgets;
	// The present-day polyline.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type present_day_polyline =
			gml_line_string.polyline();
	populate_coordinates_from_polyline(coordinate_widgets, present_day_polyline,
			CoordinatePeriods::PRESENT);

	// The reconstructed polyline, which may not be available. And test d_last_property_visited,
	// because someone might attempt to call us without invoking visit_feature_handle.
	if (d_last_property_visited) {
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
				get_reconstructed_geometry_for_property(*d_last_property_visited);
		if (recon_geometry) {
			// We use a dynamic cast here (despite the fact that dynamic casts are
			// generally considered bad form) because we only care about one specific
			// derivation.  There's no "if ... else if ..." chain, so I think it's not
			// super-bad form.  (The "if ... else if ..." chain would imply that we
			// should be using polymorphism -- specifically, the double-dispatch of the
			// Visitor pattern -- rather than updating the "if ... else if ..." chain
			// each time a new derivation is added.)
			const GPlatesMaths::PolylineOnSphere *recon_polyline =
					dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(recon_geometry->get());
			if (recon_polyline) {
				populate_coordinates_from_polyline(coordinate_widgets,
						GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type(*recon_polyline),
						CoordinatePeriods::RECONSTRUCTED);
			}
		}
	}
	
	// Add the coordinates to the tree!
	d_tree_widget_item_stack.back()->addChildren(coordinate_widgets);	

	d_tree_widget_item_stack.pop_back();
}


void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	// Fetch the parent QTreeWidgetItem we will be adding to from the stack.
	QTreeWidgetItem *parent_item = d_tree_widget_item_stack.back();
	parent_item->setExpanded(true);
	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;

	// FIXME:  Ensure that 'gml_orientable_curve.base_curve()' is not NULL.
	add_child_then_visit_value(QObject::tr("gml:OrientableCurve"), QString(),
			*gml_orientable_curve.base_curve());
}


void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	// Fetch the parent QTreeWidgetItem we will be adding to from the stack.
	QTreeWidgetItem *parent_item = d_tree_widget_item_stack.back();
	parent_item->setExpanded(true);
	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;

	// First, add a branch for the type of geometry.
	QTreeWidgetItem *geom_type_item = add_child(QObject::tr("gml:Point"), QString());
	d_tree_widget_item_stack.push_back(geom_type_item);
	geom_type_item->setExpanded(true);

	// Now, prepare the coords in present-day and reconstructed time.
	QList<QTreeWidgetItem *> coordinate_widgets;
	// The present-day point.
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type present_day_point =
			gml_point.point();
	populate_coordinates_from_point(coordinate_widgets, present_day_point,
			CoordinatePeriods::PRESENT);

	// The reconstructed point, which may not be available. And test d_last_property_visited,
	// because someone might attempt to call us without invoking visit_feature_handle.
	if (d_last_property_visited) {
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
				get_reconstructed_geometry_for_property(*d_last_property_visited);
		if (recon_geometry) {
			// We use a dynamic cast here (despite the fact that dynamic casts are
			// generally considered bad form) because we only care about one specific
			// derivation.  There's no "if ... else if ..." chain, so I think it's not
			// super-bad form.  (The "if ... else if ..." chain would imply that we
			// should be using polymorphism -- specifically, the double-dispatch of the
			// Visitor pattern -- rather than updating the "if ... else if ..." chain
			// each time a new derivation is added.)
			const GPlatesMaths::PointOnSphere *recon_point =
					dynamic_cast<const GPlatesMaths::PointOnSphere *>(recon_geometry->get());
			if (recon_point) {
				populate_coordinates_from_point(coordinate_widgets,
						GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type(*recon_point),
						CoordinatePeriods::RECONSTRUCTED);
			}
		}
	}
	
	// Add the coordinates to the tree!
	d_tree_widget_item_stack.back()->addChildren(coordinate_widgets);	

	d_tree_widget_item_stack.pop_back();
}



void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}




void
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::populate_rfg_geometries_for_feature(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	std::vector<GPlatesModel::ReconstructedFeatureGeometry>::const_iterator it =
			d_reconstruction_ptr->geometries().begin();
	std::vector<GPlatesModel::ReconstructedFeatureGeometry>::const_iterator end =
			d_reconstruction_ptr->geometries().end();
	for ( ; it != end; ++it) {
		if (it->feature_ref().references(feature_handle)) {
			ReconstructedGeometryInfo info(it->property(), it->geometry());
			d_rfg_geometries.push_back(info);
		}
	}
}


boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::get_reconstructed_geometry_for_property(
		const GPlatesModel::FeatureHandle::properties_iterator property)
{
	geometries_for_property_const_iterator it = d_rfg_geometries.begin();
	geometries_for_property_const_iterator end = d_rfg_geometries.end();
	for ( ; it != end; ++it) {
		if (it->d_property == property) {
			return it->d_geometry;
		}
	}
	return boost::none;
}


QTreeWidgetItem *
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::add_child(
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
GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator::add_child_then_visit_value(
		const QString &name,
		const QString &value,
		GPlatesModel::PropertyValue &property_value_to_visit)
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
