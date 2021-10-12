/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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
#include <vector>
#include <boost/bind.hpp>
#include <boost/none.hpp>

#include "ViewFeatureGeometriesWidgetPopulator.h"

#include "app-logic/ReconstructionGeometryUtils.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"
#include "model/Reconstruction.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/ReconstructedFeatureGeometryFinder.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"

#include "maths/LatLonPointConversions.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "utils/UnicodeStringUtils.h"
//#include "utils/Profile.h"


namespace
{
	/**
	 * Enum used to select coordinate columns.
	 */
	namespace CoordinatePeriods
	{
		enum CoordinatePeriod { PRESENT = 1, RECONSTRUCTED = 2 };
	}
	
	/**
	 * A sequence of item handles used in the TreeWidgetBuilder interface.
	 */
	typedef std::vector<GPlatesGui::TreeWidgetBuilder::item_handle_type> item_handle_seq_type;

	/**
	 * Create a top-level QTreeWidgetItem but don't add as a top-level item yet.
	 * This is because it won't get added unless it's from a geometric property value.
	 */
	GPlatesGui::TreeWidgetBuilder::item_handle_type
	make_top_level_item_for_property(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			const GPlatesModel::PropertyName &name)
	{
		QStringList fields;
		fields.push_back(GPlatesUtils::make_qstring_from_icu_string(name.build_aliased_name()));
		fields.push_back(QString());
		fields.push_back(QString());

		return tree_widget_builder.create_item(fields);
	}
	
	/**
	 * Ensures the given coordinate_widgets list has a minimum number of
	 * blank QTreeWidgetItems suitable for populating with coordinates.
	 */
	void
	fill_coordinates_with_blank_items(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			item_handle_seq_type &coordinate_widgets,
			GPlatesMaths::PolylineOnSphere::size_type new_size)
	{
		static QLocale locale;
		for (unsigned int i = coordinate_widgets.size(); i < new_size; ++i) {
			QString point_id(QObject::tr("#"));
			point_id.append(locale.toString(i));
			point_id.append(QObject::tr(" (lat ; lon)"));
		
			const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
					tree_widget_builder.create_item();

			tree_widget_builder.get_qtree_widget_item(item_handle)->setText(0, point_id);

			coordinate_widgets.push_back(item_handle);
		}
	}
	
	/**
	 * Iterates over the vertices of the polygon, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_polygon(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			item_handle_seq_type &coordinate_widgets,
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon,
			CoordinatePeriods::CoordinatePeriod period)
	{
		// This whole polygon function is essentially a copy of the line_string function. 
		static QLocale locale;
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = polygon->vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon->vertex_end();
		
		// Ensure we have enough blank QTreeWidgetItems in the list to populate.
		fill_coordinates_with_blank_items(
				tree_widget_builder, coordinate_widgets, polygon->number_of_vertices());
		
		// Then fill in the appropriate column.
		for (unsigned point_index = 0; iter != end; ++iter, ++point_index) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
			QString lat = locale.toString(llp.latitude());
			QString lon = locale.toString(llp.longitude());
			QString point;
			point.append(lat);
			point.append(QObject::tr(" ; "));
			point.append(lon);
	
			tree_widget_builder.get_qtree_widget_item(
					coordinate_widgets[point_index])->setText(period, point);
		}
	}

	/**
	 * Iterates over the vertices of the multipoint, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_multi_point(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			item_handle_seq_type &coordinate_widgets,
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point,
			CoordinatePeriods::CoordinatePeriod period)
	{
		// This whole multi_point function is essentially a copy of the line_string function. 
		static QLocale locale;
		GPlatesMaths::MultiPointOnSphere::const_iterator iter = multi_point->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator end = multi_point->end();
		
		// Ensure we have enough blank QTreeWidgetItems in the list to populate.
		fill_coordinates_with_blank_items(
				tree_widget_builder, coordinate_widgets, multi_point->number_of_points());
		
		// Then fill in the appropriate column.
		for (unsigned point_index = 0; iter != end; ++iter, ++point_index) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
			QString lat = locale.toString(llp.latitude());
			QString lon = locale.toString(llp.longitude());
			QString point;
			point.append(lat);
			point.append(QObject::tr(" ; "));
			point.append(lon);
	
			tree_widget_builder.get_qtree_widget_item(
					coordinate_widgets[point_index])->setText(period, point);
		}
	}

	/**
	 * Iterates over the vertices of the polyline, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_polyline(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			item_handle_seq_type &coordinate_widgets,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline,
			CoordinatePeriods::CoordinatePeriod period)
	{
		//PROFILE_FUNC();

		static QLocale locale;
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline->vertex_end();
		
		// Ensure we have enough blank QTreeWidgetItems in the list to populate.
		fill_coordinates_with_blank_items(
				tree_widget_builder, coordinate_widgets, polyline->number_of_vertices());
		
		// Then fill in the appropriate column.
		for (unsigned point_index = 0; iter != end; ++iter, ++point_index) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
			QString lat = locale.toString(llp.latitude());
			QString lon = locale.toString(llp.longitude());
			QString point;
			point.append(lat);
			point.append(QObject::tr(" ; "));
			point.append(lon);
	
			tree_widget_builder.get_qtree_widget_item(
					coordinate_widgets[point_index])->setText(period, point);
		}
	}

	/**
	 * Iterates over the vertices of the point, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_point(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			item_handle_seq_type &coordinate_widgets,
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
			CoordinatePeriods::CoordinatePeriod period)
	{
		static QLocale locale;
		// Ensure we have a blank QTreeWidgetItem in the list to populate.
		fill_coordinates_with_blank_items(
				tree_widget_builder, coordinate_widgets, 1);
		
		// Then fill in the appropriate column.
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*point_on_sphere);
		QString lat = locale.toString(llp.latitude());
		QString lon = locale.toString(llp.longitude());
		QString point;
		point.append(lat);
		point.append(QObject::tr(" ; "));
		point.append(lon);

		tree_widget_builder.get_qtree_widget_item(
					coordinate_widgets[0])->setText(period, point);
	}

}


void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::populate(
		GPlatesModel::FeatureHandle::weak_ref &feature,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type focused_rg)
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
		GPlatesModel::FeatureHandle::properties_iterator focused_geometry_property;
		if (GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
				focused_rg, focused_geometry_property))
		{
			d_focused_geometry = focused_geometry_property;
		}
	}
	visit_feature(feature);

	// Now that we've accumulated the tree widget item hierarchy we can
	// add the hierarchy to Qt efficiently by adding all children of each
	// tree widget item in one call using a QList.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
}


bool
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// Iterate over the Reconstruction and grab the reconstructed geometry that originates
	// from the given feature.
	populate_rfg_geometries_for_feature(feature_handle);

	// Visit the properties.
	return true;
}


void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::finalise_post_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// Now add any geometric properties we were interested in (and delete the others)
	property_info_vector_const_iterator it = d_property_info_vector.begin();
	property_info_vector_const_iterator end = d_property_info_vector.end();
	for ( ; it != end; ++it) {
		if (it->is_geometric_property) {
			// This is a geometric property and we want to add it to the widget.
			add_top_level_item(d_tree_widget_builder, it->item_handle);
		}
		// Else the memory used by this top-level tree widget item will be released by
		// our TreeWidgetBuilder when the tree widget item hierarchy is transferred
		// to the QTreeWidget.
	}
}


bool
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::initialise_pre_property_values(
		GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	// Create a top-level item for this property and remember it - do not add it just yet.
	PropertyInfo info;
	info.is_geometric_property = false;
	info.item_handle = make_top_level_item_for_property(
			d_tree_widget_builder, top_level_property_inline.property_name());
	d_property_info_vector.push_back(info);

	// If the current property is the focused geometry then scroll to it
	// so the user can see it.
	if (d_focused_geometry == current_top_level_propiter())
	{
		// Call QTreeWidget::scrollToItem() passing the current item, but do it later
		// when the item is attached to the QTreeWidget otherwise it will have no effect.
		d_tree_widget_builder.add_function(
				info.item_handle,
				boost::bind(
						&QTreeWidget::scrollToItem,
						_2, // will be the QTreeWidget that the tree widget builder uses
						_1, // will be the QTreeWidgetItem we're attaching this function to
						QAbstractItemView::EnsureVisible));
	}

	// Set up stack for building the children of this item.
	d_tree_widget_builder.push_current_item(info.item_handle);

	// Visit the properties.
	return true;
}


void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::finalise_post_property_values(
		GPlatesModel::TopLevelPropertyInline &)
{
	d_tree_widget_builder.pop_current_item();
}


void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	//PROFILE_FUNC();

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

	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;
	
	// First, add a branch for the type of geometry.
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:LineString"));

	// Call QTreeWidgetItem::setExpanded(true) on 'item_handle', but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	d_tree_widget_builder.add_function(
			item_handle,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// Now, prepare the coords in present-day and reconstructed time.
	item_handle_seq_type coordinate_widgets;
	// The present-day polyline.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type present_day_polyline =
			gml_line_string.polyline();
	populate_coordinates_from_polyline(d_tree_widget_builder,
			coordinate_widgets, present_day_polyline,
			CoordinatePeriods::PRESENT);

	// The reconstructed polyline, which may not be available.
	boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
			get_reconstructed_geometry_for_property(*current_top_level_propiter());
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
			populate_coordinates_from_polyline(d_tree_widget_builder,
					coordinate_widgets,
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type(
							recon_polyline,
							GPlatesUtils::NullIntrusivePointerHandler()),
					CoordinatePeriods::RECONSTRUCTED);
		}
	}

	// Add the coordinates to the tree!
	add_children(d_tree_widget_builder,
			item_handle, coordinate_widgets.begin(), coordinate_widgets.end());
}

void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
// This whole gml_multi_point function is essentially a copy of the gml_line_string function. 

	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;
	
	// First, add a branch for the type of geometry.
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:MultiPoint"));

	// Call QTreeWidgetItem::setExpanded(true) on 'item_handle', but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	d_tree_widget_builder.add_function(
			item_handle,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// Now, prepare the coords in present-day and reconstructed time.
	item_handle_seq_type coordinate_widgets;
	// The present-day polyline.

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type present_day_multi_point =
			gml_multi_point.multipoint();
	populate_coordinates_from_multi_point(d_tree_widget_builder,
			coordinate_widgets, present_day_multi_point,
			CoordinatePeriods::PRESENT);

	// The reconstructed polyline, which may not be available.
	boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
			get_reconstructed_geometry_for_property(*current_top_level_propiter());
	if (recon_geometry) {
		// We use a dynamic cast here (despite the fact that dynamic casts are
		// generally considered bad form) because we only care about one specific
		// derivation.  There's no "if ... else if ..." chain, so I think it's not
		// super-bad form.  (The "if ... else if ..." chain would imply that we
		// should be using polymorphism -- specifically, the double-dispatch of the
		// Visitor pattern -- rather than updating the "if ... else if ..." chain
		// each time a new derivation is added.)
		const GPlatesMaths::MultiPointOnSphere *recon_multi_point =
				dynamic_cast<const GPlatesMaths::MultiPointOnSphere *>(recon_geometry->get());
		if (recon_multi_point) {
			populate_coordinates_from_multi_point(d_tree_widget_builder,
					coordinate_widgets,
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type(
							recon_multi_point,
							GPlatesUtils::NullIntrusivePointerHandler()),
					CoordinatePeriods::RECONSTRUCTED);
		}
	}
	
	// Add the coordinates to the tree!
	add_children(d_tree_widget_builder,
			item_handle, coordinate_widgets.begin(), coordinate_widgets.end());
}

void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
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

	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;

	// FIXME:  Ensure that 'gml_orientable_curve.base_curve()' is not NULL.
	add_child_then_visit_value(QObject::tr("gml:OrientableCurve"), QString(),
			*gml_orientable_curve.base_curve());
}


void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;

	// First, add a branch for the type of geometry.
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:Point"));

	// Call QTreeWidgetItem::setExpanded(true) on 'item_handle', but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	d_tree_widget_builder.add_function(
			item_handle,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// Now, prepare the coords in present-day and reconstructed time.
	item_handle_seq_type coordinate_widgets;
	// The present-day point.
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type present_day_point =
			gml_point.point();
	populate_coordinates_from_point(d_tree_widget_builder,
			coordinate_widgets, present_day_point,
			CoordinatePeriods::PRESENT);

	// The reconstructed point, which may not be available.
	boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
			get_reconstructed_geometry_for_property(*current_top_level_propiter());
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
			populate_coordinates_from_point(d_tree_widget_builder,
					coordinate_widgets,
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type(
							recon_point,
							GPlatesUtils::NullIntrusivePointerHandler()),
					CoordinatePeriods::RECONSTRUCTED);
		}
	}
	
	// Add the coordinates to the tree!
	add_children(d_tree_widget_builder, item_handle,
			coordinate_widgets.begin(), coordinate_widgets.end());
}

void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// Like the multi_point function, I've more or less just copied the line_string function here. 

	// Call QTreeWidgetItem::setExpanded(true) on the current item, but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	add_function_to_current_item(d_tree_widget_builder,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;
	
	// First, add a branch for the type of geometry.
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:Polygon"));

	// Call QTreeWidgetItem::setExpanded(true) on 'item_handle', but do it later
	// when the item is attached to the QTreeWidget otherwise it will have no effect.
	d_tree_widget_builder.add_function(
			item_handle,
			boost::bind(
					&QTreeWidgetItem::setExpanded,
					_1, // will be the QTreeWidgetItem we're attaching this function to
					true));

	d_tree_widget_builder.push_current_item(item_handle);

	// Exterior ring. 
	const GPlatesGui::TreeWidgetBuilder::item_handle_type exterior_item_handle =
			add_child_to_current_item(d_tree_widget_builder, QObject::tr("gml:exterior"));

	d_tree_widget_builder.push_current_item(exterior_item_handle);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr =
			gml_polygon.exterior();

	write_polygon_ring(polygon_ptr);

	d_tree_widget_builder.pop_current_item();

	// Now handle any internal rings.
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator iter = gml_polygon.interiors_begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();

	for (unsigned ring_number = 1; iter != end ; ++iter, ++ring_number)
	{
		QString interior;
		interior.append(QObject::tr("gml:interior"));
		interior.append(QObject::tr(" #"));
		interior.append(ring_number);

		const GPlatesGui::TreeWidgetBuilder::item_handle_type interior_item_handle =
				add_child_to_current_item(d_tree_widget_builder, interior);

		d_tree_widget_builder.push_current_item(interior_item_handle);

		write_polygon_ring(*iter);

		d_tree_widget_builder.pop_current_item();
	}

	d_tree_widget_builder.pop_current_item();
}

void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}




void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::populate_rfg_geometries_for_feature(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// Iterate through the RFGs (belonging to 'd_reconstruction_ptr')
	// that are observing 'feature_handle'.
	GPlatesModel::ReconstructedFeatureGeometryFinder rfgFinder(d_reconstruction_ptr);
	rfgFinder.find_rfgs_of_feature(&feature_handle);

	GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator rfgIter;
	for (rfgIter = rfgFinder.found_rfgs_begin();
		rfgIter != rfgFinder.found_rfgs_end();
		++rfgIter)
	{
		GPlatesModel::ReconstructedFeatureGeometry *rfg = rfgIter->get();

		ReconstructedGeometryInfo info(rfg->property(), rfg->geometry());
		d_rfg_geometries.push_back(info);
	}
}


boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::get_reconstructed_geometry_for_property(
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


void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::add_child_then_visit_value(
		const QString &name,
		const QString &value,
		GPlatesModel::PropertyValue &property_value_to_visit)
{
	const GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle =
			add_child_to_current_item(d_tree_widget_builder, name, value);

	d_tree_widget_builder.push_current_item(item_handle);
	property_value_to_visit.accept_visitor(*this);
	d_tree_widget_builder.pop_current_item();
}

void
GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator::write_polygon_ring(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr)
{
	// Now, prepare the coords in present-day and reconstructed time.
	item_handle_seq_type coordinate_widgets;

	populate_coordinates_from_polygon(d_tree_widget_builder,
			coordinate_widgets, polygon_ptr,
			CoordinatePeriods::PRESENT);

	// The reconstructed polyline, which may not be available. And test current_top_level_propiter(),
	// because someone might attempt to call us without invoking visit_feature_handle.
	if (current_top_level_propiter()) {
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
				get_reconstructed_geometry_for_property(*current_top_level_propiter());
		if (recon_geometry) {
			// We use a dynamic cast here (despite the fact that dynamic casts are
			// generally considered bad form) because we only care about one specific
			// derivation.  There's no "if ... else if ..." chain, so I think it's not
			// super-bad form.  (The "if ... else if ..." chain would imply that we
			// should be using polymorphism -- specifically, the double-dispatch of the
			// Visitor pattern -- rather than updating the "if ... else if ..." chain
			// each time a new derivation is added.)
			const GPlatesMaths::PolygonOnSphere *recon_polygon =
					dynamic_cast<const GPlatesMaths::PolygonOnSphere *>(recon_geometry->get());
			if (recon_polygon) {
				populate_coordinates_from_polygon(d_tree_widget_builder,
						coordinate_widgets,
						GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type(
								recon_polygon,
								GPlatesUtils::NullIntrusivePointerHandler()),
						CoordinatePeriods::RECONSTRUCTED);
			}
		}
	}
	
	// Add the coordinates to the tree!
	add_children_to_current_item(d_tree_widget_builder,
			coordinate_widgets.begin(), coordinate_widgets.end());
}
