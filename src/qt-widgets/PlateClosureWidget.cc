
/**
 * \file 
 * $Revision$
 * $Date$ 
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

#define DEBUG

#include <map>

#include <QtGlobal>
#include <QHeaderView>
#include <QTreeWidget>
#include <QUndoStack>
#include <QToolButton>
#include <QMessageBox>
#include <QLocale>
#include <QDebug>
#include <QString>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "PlateClosureWidget.h"

#include "ViewportWindow.h"
#include "ExportCoordinatesDialog.h"
#include "CreateFeatureDialog.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/GeometryOnSphere.h"
#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPointConversions.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"
#include "maths/Real.h"

#include "gui/FeatureFocus.h"
#include "gui/ProximityTests.h"

#include "model/FeatureHandle.h"
#include "model/ReconstructionGeometry.h"
#include "model/ModelUtils.h"
#include "model/DummyTransactionHandle.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"

#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"
#include "feature-visitors/TopologySectionsFinder.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/TemplateTypeParameterType.h"

#include "view-operations/RenderedGeometryParameters.h"

namespace
{
	/**
	 * Borrowed from FeatureTableModel.cc.
	 */
	QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.time_position().is_real()) {
			return locale.toString(time_instant.time_position().value());
		} else if (time_instant.time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}

	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;
	
	
	/**
	 * Determines what fragment of geometry the top-level tree widget item
	 * would become, given the current configuration of the PlateClosureWidget
	 * and the position and number of children in this top-level item.
	 */
	QString
	calculate_label_for_item(
			GPlatesQtWidgets::PlateClosureWidget::GeometryType target_geom_type,
			int position,
			QTreeWidgetItem *item)
	{
		QString label;
		// Pick a sensible default.
		switch (target_geom_type)
		{
		default:
				label = QObject::tr("<Error: unknown GeometryType>");
				break;
		case GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON:
				label = "gml:LineString";
				break;
		case GPlatesQtWidgets::PlateClosureWidget::DEFORMINGPLATE:
				label = "gml:MultiPoint";
				break;
		}
		
		// Override that default for particular edge cases.
		int children = item->childCount();
		if (children == 0) {
			label = "";
		} else if (children == 1) {
			label = "gml:Point";
		} else if (children == 2 && target_geom_type == GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON) {
			label = "gml:LineString";
		}
		// FIXME:  We need to handle the situation in which the user wants to digitise a
		// polygon, and there are 3 distinct adjacent points, but the first and last points
		// are equal.  (This should result in a gml:LineString.)
		
		// PlateClosure Polygon gives special meaning to the first entry.
		if (target_geom_type == GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON) {
			if (position == 0) {
				label = QObject::tr("exterior: %1").arg(label);
			} else {
				label = QObject::tr("interior: %1").arg(label);
			}
		}
		
		return label;
	}


	/**
	 * Creates 'appropriate' geometry given the available 
	 * Examines QTreeWidgetItems, the number of points available, and the user's
	 * intentions. 
	 * to call upon the appropriate anonymous namespace geometry creation function.
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GPlatesUtils::GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	geometry_opt_ptr_type
	create_geometry_from_vertex_list(
			std::vector<GPlatesMaths::PointOnSphere> &points,
			GPlatesQtWidgets::PlateClosureWidget::GeometryType target_geom_type,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
	{
		// FIXME: Only handles the unbroken line and single-ring cases.

		// There's no guarantee that adjacent points in the table aren't identical.
		std::vector<GPlatesMaths::PointOnSphere>::size_type num_points =
				GPlatesMaths::count_distinct_adjacent_points(points);

std::cout << "create_geometry_from_vertex_list: size =" << num_points << std::endl;

#ifdef DEBUG1
std::vector<GPlatesMaths::PointOnSphere>::iterator itr;
for ( itr = points.begin() ; itr != points.end(); ++itr)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*itr);
	std::cout << "create_geometry_from_vertex_list: llp=" << llp << std::endl;
}
#endif
	

		// FIXME: I think... we need some way to add data() to the 'header' QTWIs, so that
		// we can immediately discover which bits are supposed to be polygon exteriors etc.
		// Then the function calculate_label_for_item could do all our 'tagging' of
		// geometry parts, and -this- function wouldn't need to duplicate the logic.

		// FIXME 2: We should have a 'try {  } catch {  }' block to catch any exceptions
		// thrown during the instantiation of the geometries.

		// This will become a proper 'try {  } catch {  } block' when we get around to it.
		try
		{
			switch (target_geom_type)
			{
			default:
				// FIXME: Exception.
				qDebug() << "Unknown geometry type, not implemented yet!";
				return boost::none;

			case GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON:
				if (num_points == 0) {
					validity = GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
					return boost::none;
				} else if (num_points == 1) {
					return GPlatesUtils::create_point_on_sphere(points, validity);
				} else if (num_points == 2) {
					return GPlatesUtils::create_polyline_on_sphere(points, validity);
				} else if (num_points == 3 && points.front() == points.back()) {
					return GPlatesUtils::create_polyline_on_sphere(points, validity);
				} else {
					return GPlatesUtils::create_polygon_on_sphere(points, validity);
				}
				break;
			}
			// Should never reach here.
		} catch (...) {
			throw;
		}
		return boost::none;
	}
}



// 
// Constructor
// 
GPlatesQtWidgets::PlateClosureWidget::PlateClosureWidget(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface &model_interface,
		ViewportWindow &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_rendered_geom_factory(&rendered_geom_factory),
	d_feature_focus_ptr(&feature_focus),
	d_model_interface(&model_interface),
	d_view_state_ptr(&view_state_),
	d_create_feature_dialog(
		new CreateFeatureDialog(model_interface, view_state_, this) ),
	d_geometry_type(GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON),
	d_geometry_opt_ptr(boost::none)
{
	setupUi(this);

	create_child_rendered_layers();

	// set the internal state
	d_visit_to_check_type = false;
	d_visit_to_create_properties = false;
	d_visit_to_get_focus_end_points = false;

	// clear the lineedit widgets
	clear_widgets();

//WWWW
	// Set the widget states
	button_new_topology->setEnabled(true);
	button_edit_topology->setEnabled(false);
	label_type->setEnabled(false);
	lineedit_type->setEnabled(false);
	label_name->setEnabled(false);
	lineedit_name->setEnabled(false);
	label_plate_id->setEnabled(false);
	lineedit_plate_id->setEnabled(false);
	label_coordinates->setEnabled(false);
	label_first->setEnabled(false);
	label_last->setEnabled(false);
	lineedit_first->setEnabled(false);
	lineedit_last->setEnabled(false);
	lineedit_use_reverse->setEnabled(false);
	button_use_reverse->setEnabled(false);
	button_append_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(false);
	button_cancel->setEnabled(true);

	// Subscribe to focus events. We can discard the FeatureFocus reference afterwards.
	QObject::connect(&feature_focus,
		SIGNAL( focus_changed(
			GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT( display_feature(
			GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));

	QObject::connect(&feature_focus,
		SIGNAL( focused_feature_modified(
			GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT( display_feature_focus_modified(
			GPlatesModel::FeatureHandle::weak_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));


	// New Topology button to start a new topology
	QObject::connect(button_new_topology, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_new_topology()));	
	
	// Edit Topology button to load the feature focus atopology
	QObject::connect(button_edit_topology, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_edit_topology()));	
	


	// Use Coordinates in Reverse
	QObject::connect(button_use_reverse, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_use_coordinates_in_reverse()));
	
	// Choose Feature button 
	QObject::connect(button_append_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_append_feature()));
	
	// Remove Feature button 
	QObject::connect(button_remove_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_remove_feature()));
	
	QObject::connect(button_insert_after, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_insert_after()));
	
	QObject::connect(button_insert_before, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_insert_before()));
	
	// Clear button to clear points from table and start over.
	QObject::connect(button_clear_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_clear()));
	
	// New Topology button to open the Create Feature dialog if needed
	QObject::connect(button_apply, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_apply()));	
	
	// Cancel button to cancel the process 
	QObject::connect(button_cancel, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_cancel()));	
	
	// Get everything else ready that may need to be set up more than once.
	initialise_geometry(PLATEPOLYGON);
}

void
GPlatesQtWidgets::PlateClosureWidget::create_child_rendered_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layers to draw geometries.
	// NOTE: create bottom to top 
	d_temporary_geometry_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_focused_feature_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_segments_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_intersection_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_click_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_end_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.

	// Activate layers
	d_temporary_geometry_layer_ptr->set_active();
	d_focused_feature_layer_ptr->set_active();
	d_segments_layer_ptr->set_active();
	d_intersection_points_layer_ptr->set_active();
	d_click_points_layer_ptr->set_active();
	d_end_points_layer_ptr->set_active();
}

void
GPlatesQtWidgets::PlateClosureWidget::initialise_geometry(
		GeometryType geom_type)
{
	clear_widgets();
	d_use_reverse = false;
	d_tmp_index_use_reverse = false;
	d_geometry_type = geom_type;

	d_tmp_feature_type = GPlatesGlobal::UNKNOWN_FEATURE;
}


void
GPlatesQtWidgets::PlateClosureWidget::change_geometry_type(
		GeometryType geom_type)
{
	if (geom_type == d_geometry_type) {
		// Convert from one type of desired geometry to the exact same type.
		// i.e. do nothing.
		return;
	}
}


void
GPlatesQtWidgets::PlateClosureWidget::set_click_point(double lat, double lon)
{
	d_click_point_lat = lat;
	d_click_point_lon = lon;

	draw_click_point();
}


// Clear all the widgets
void
GPlatesQtWidgets::PlateClosureWidget::clear_widgets()
{
	lineedit_type->clear();
	lineedit_name->clear();
	lineedit_plate_id->clear();
	lineedit_first->clear();
	lineedit_last->clear();
	lineedit_use_reverse->clear();
	lineedit_num_sections->clear();
}

// Fill some of the widgets 
void
GPlatesQtWidgets::PlateClosureWidget::fill_widgets(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
	// Populate the widget from the FeatureHandle:
	
	// Feature Type.
	lineedit_type->setText(GPlatesUtils::make_qstring_from_icu_string(
			feature_ref->feature_type().build_aliased_name()) );

	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");
	GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
	string_finder.visit_feature_handle(*feature_ref);
	if (string_finder.found_strings_begin() != string_finder.found_strings_end()) 
	{
		// The feature has one or more name properties. Use the first one for now.
		GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name = 
				*string_finder.found_strings_begin();
		
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
	}

	// Plate ID.
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
	plate_id_finder.visit_feature_handle(*feature_ref);
	if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) 
	{
		// The feature has a reconstruction plate ID.
		GPlatesModel::integer_plate_id_type recon_plate_id =
				*plate_id_finder.found_plate_ids_begin();
		
		lineedit_plate_id->setText(QString::number(recon_plate_id));
	}
	
	// create a dummy tree
	// use it and the populator to get coords
	QTreeWidget *tree_geometry = new QTreeWidget(this);
	tree_geometry->hide();
	GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator populator(
		d_view_state_ptr->reconstruction(), *tree_geometry);
	populator.visit_feature_handle(*feature_ref);
	d_first_coord = populator.get_first_coordinate();
	d_last_coord = populator.get_last_coordinate();
	lineedit_first->setText(d_first_coord);
	lineedit_last->setText(d_last_coord);
	
	// clean up
	delete tree_geometry;
}


// ===========================================================================================
// 
// Functions to display different features
//

//
void
GPlatesQtWidgets::PlateClosureWidget::display_feature_focus_modified(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{

#ifdef DEBUG
std::cout << std::endl;
std::cout << "PlateClosureWidget::display_feature_focus_modified:" << std::endl;
#endif
	display_feature( feature_ref, associated_rfg);
}

// Display the clicked feature data in the widgets 
void
GPlatesQtWidgets::PlateClosureWidget::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{

#ifdef DEBUG
std::cout << std::endl;
std::cout << "PlateClosureWidget::display_feature:" << std::endl;
#endif

	// Clear the widget fields
	clear_widgets();

	// Set widget states
	button_new_topology->setEnabled(false);
	button_edit_topology->setEnabled(false);
	label_type->setEnabled(false);
	lineedit_type->setEnabled(false);
	label_name->setEnabled(false);
	lineedit_name->setEnabled(false);
	label_plate_id->setEnabled(false);
	lineedit_plate_id->setEnabled(false);
	label_coordinates->setEnabled(false);
	label_first->setEnabled(false);
	label_last->setEnabled(false);
	lineedit_first->setEnabled(false);
	lineedit_last->setEnabled(false);
	lineedit_use_reverse->setEnabled(false);
	button_use_reverse->setEnabled(false);
	button_append_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
#if 0
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);
#endif


	// Clear all the layers
	draw_all_layers_clear();

	// draw the current click point
	draw_click_point();


	//
	// Determine what to do with the focused feature
	//

	// always check your weak_refs!
	if ( ! feature_ref.is_valid() ) 
	{
std::cout << "PlateClosureWidget::display_feature: invalid ref" << std::endl;
		// if d_topology_reference has been set, then update_geometry
		if ( d_topology_feature_ref.is_valid() )
		{
			// process the sections table
			d_visit_to_create_properties = true;
			update_geometry();
			d_visit_to_create_properties = false;
		}
		
		// else, reset the references
		d_geometry_opt_ptr = boost::none;
		d_feature_focus_rfg = NULL;

		return;
	}

	// set the focused ref
	d_focused_feature_ref = feature_ref;
qDebug() << "feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );

	// set the focused geom
	if ( associated_rfg ) 
	{
qDebug() << "associated_rfg = okay " ;
		d_feature_focus_rfg = associated_rfg;
	} else {
qDebug() << "associated_rfg = NULL " ;
		d_feature_focus_rfg = NULL;
	}

	// draw the geom and its end_points
	draw_focused_geometry();

	//
	// Check feature type via qstrings 
	//
 	QString topology_type_name ("TopologicalClosedPlateBoundary");
	QString feature_name = GPlatesUtils::make_qstring_from_icu_string(
		feature_ref->feature_type().get_name() );

	if ( feature_name == topology_type_name )
	{
		if ( d_topology_feature_ref.is_valid() )
		{
			return; // a topology ref has been set ; don't do anything 
		}
		// else, change widget state
		button_new_topology->setEnabled(false);
		button_edit_topology->setEnabled(true);
	} 
	else // non-topology feature type selected 
	{
		// test if feature is already in the section table
		GPlatesModel::FeatureId test_id = feature_ref->feature_id();
qDebug() << "test_id = " << GPlatesUtils::make_qstring_from_icu_string( test_id.get() );

		std::vector<GPlatesModel::FeatureId>::iterator section_itr = d_section_ids.begin();
		std::vector<GPlatesModel::FeatureId>::iterator section_end = d_section_ids.end();
		int i = 0;
		for ( ; section_itr != section_end; ++section_itr, ++i)
		{
			GPlatesModel::FeatureId	section_id = *section_itr;
#ifdef DEBUG
qDebug() << "i = " << i << "; section_id = " << GPlatesUtils::make_qstring_from_icu_string( section_id.get() );
#endif 
			if ( test_id == section_id)
			{
				display_feature_on_boundary( feature_ref, associated_rfg );
				// set focus data 
				d_focused_index = i;
				return;
			}
		}
		// test_id not found on boundary 
		display_feature_not_on_boundary( feature_ref, associated_rfg );
		return;
	}
	return;
}


//
// display the topology in the sections table and on the widget 
//
void
GPlatesQtWidgets::PlateClosureWidget::display_feature_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
#ifdef DEBUG
qDebug() << "PlateClosureWidget::display_feature_topology()";
#endif

	// Set the widget states
	button_new_topology->setEnabled(false);
	button_edit_topology->setEnabled(false);
	label_type->setEnabled(true);
	lineedit_type->setEnabled(true);
	label_name->setEnabled(true);
	lineedit_name->setEnabled(true);
	label_plate_id->setEnabled(true);
	lineedit_plate_id->setEnabled(true);
	label_coordinates->setEnabled(false);
	label_first->setEnabled(false);
	label_last->setEnabled(false);
	lineedit_first->setEnabled(false);
	lineedit_last->setEnabled(false);
	lineedit_use_reverse->setEnabled(false);
	button_use_reverse->setEnabled(false);
	button_append_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(true);
	lineedit_num_sections->setEnabled(true);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);

	// Clear the sections_table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();
	sections_table.clear();

	// Clear the working lists
	d_section_ptrs.clear();
	d_section_ids.clear();
	d_section_click_points.clear();
	d_section_reverse_flags.clear();
	// NOTE: The call to update_geometry(); below updates the other working lists

	// Create a new TopologySectionsFinder with d_section_ vectors
	GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder( 
		d_section_ptrs, d_section_ids, d_section_click_points, d_section_reverse_flags);

	// Visit the feture ref, filling d_section_ vectors with data
	feature_ref->accept_visitor( topo_sections_finder );

	// Get a map of FeatureId to ReconstructionGeometry pointers for reconstruction
	typedef std::map<
		GPlatesModel::FeatureId,
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type > 
			id_to_rg_map_type;

	typedef std::map<
		GPlatesModel::FeatureId,
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type >::iterator 	
			id_to_rg_map_iterator_type;

	id_to_rg_map_type rg_map;
	id_to_rg_map_iterator_type rg_itr, rg_end;
	
	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator geom_itr =
		d_view_state_ptr->reconstruction().geometries().begin();
	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator geom_end =
		d_view_state_ptr->reconstruction().geometries().end();
	for ( ; geom_itr != geom_end; ++geom_itr)
	{
		GPlatesModel::ReconstructionGeometry *rg = geom_itr->get();
		GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
		if ( ! rfg) {
			continue;
		}
		rg_map.insert( 
			std::make_pair( rfg->feature_ref()->feature_id(), *geom_itr ) );
	}

	//
	// Find this topology's ReconstructionGeometry data and insert into sections table
	//
	std::vector<GPlatesModel::FeatureId>::iterator section_itr = d_section_ids.begin();
	std::vector<GPlatesModel::FeatureId>::iterator section_end = d_section_ids.end();
	for ( ; section_itr != section_end; ++section_itr)
	{
		GPlatesModel::FeatureId	section_id = *section_itr;

		// try to find the id in the map
		id_to_rg_map_iterator_type find_iter = rg_map.find( section_id );

		if ( find_iter != rg_map.end() )
		{
			sections_table.begin_insert_features(0, 0);
			sections_table.geometry_sequence().push_back( find_iter->second );
			sections_table.end_insert_features();
		}
	}

	// update the d_section_* vectors from the sections table
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// fill the widgets with feature data
	fill_widgets( feature_ref, associated_rfg );

	// Set the num_sections
	lineedit_num_sections->setText(QString::number( d_section_ptrs.size() ));
}


//
// 
//
void
GPlatesQtWidgets::PlateClosureWidget::display_feature_on_boundary(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
#ifdef DEBUG
qDebug() << "PlateClosureWidget::display_feature_on_boundary()";
#endif

	// always double check your weak_refs!
	if ( ! feature_ref.is_valid() ) {
		return;
	} 

	// set the focus refs
	d_focused_feature_ref = feature_ref;
	d_feature_focus_rfg = associated_rfg;

	// fill the widgets with feature data
	fill_widgets( feature_ref, associated_rfg );

	// set the the lineedit_use_reverse
	bool r = d_section_reverse_flags.at( d_focused_index );
	if (r) { lineedit_use_reverse->setText( tr("yes") ); }
	if (!r) { lineedit_use_reverse->setText( tr("no") ); }

	// Set the widget states
	button_new_topology->setEnabled(false);
	button_edit_topology->setEnabled(false);
	label_type->setEnabled(true);
	lineedit_type->setEnabled(true);
	label_name->setEnabled(true);
	lineedit_name->setEnabled(true);
	label_plate_id->setEnabled(true);
	lineedit_plate_id->setEnabled(true);
	label_coordinates->setEnabled(true);
	label_first->setEnabled(true);
	label_last->setEnabled(true);
	lineedit_first->setEnabled(true);
	lineedit_last->setEnabled(true);
	lineedit_use_reverse->setEnabled(true);
	button_use_reverse->setEnabled(true);
	button_append_feature->setEnabled(true);
	button_remove_feature->setEnabled(true);
	button_insert_before->setEnabled(true);
	button_insert_after->setEnabled(true);
	button_clear_feature->setEnabled(true);
	label_num_sections->setEnabled(true);
	lineedit_num_sections->setEnabled(true);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);

	// update the d_section_* vectors from the sections table
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// draw the layers
	draw_all_layers();
}


void
GPlatesQtWidgets::PlateClosureWidget::display_feature_not_on_boundary(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
#ifdef DEBUG
qDebug() << "PlateClosureWidget::display_feature_not_on_boundary()";
#endif
	// always double check your weak_refs!
	if ( ! feature_ref.is_valid() ) {
		return;
	} 

	// set the focus refs
	d_focused_feature_ref = feature_ref;
	d_feature_focus_rfg = associated_rfg;

	// fill the widgets with feature data
	fill_widgets( feature_ref, associated_rfg );

	// Set the widget states
	button_new_topology->setEnabled(true);
	button_edit_topology->setEnabled(false);
	label_type->setEnabled(true);
	lineedit_type->setEnabled(true);
	label_name->setEnabled(true);
	lineedit_name->setEnabled(true);
	label_plate_id->setEnabled(true);
	lineedit_plate_id->setEnabled(true);
	label_coordinates->setEnabled(true);
	label_first->setEnabled(true);
	label_last->setEnabled(true);
	lineedit_first->setEnabled(true);
	lineedit_last->setEnabled(true);
	lineedit_use_reverse->setEnabled(false);
	button_use_reverse->setEnabled(false);
	button_append_feature->setEnabled(true);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(true);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(false);
	button_cancel->setEnabled(true);

	// draw the layers
	draw_all_layers();
}




// 
// Button Handlers  and support functions 
// 
void
GPlatesQtWidgets::PlateClosureWidget::handle_new_topology()
{
	// adjust the widgets
	button_new_topology->setEnabled(false);
	button_edit_topology->setEnabled(false);

	// display the focused feature with intent to start a new boundary 
	GPlatesQtWidgets::PlateClosureWidget::display_feature_not_on_boundary(
		d_focused_feature_ref, d_feature_focus_rfg );

 	d_view_state_ptr->status_message( QObject::tr(
 		"Click on features to create the boundary; "
 		"use Apply to create new Topology; "
 		"use Ctrl+drag to re-orient the globe.")
	);
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_edit_topology()
{
	// set the feature ref 
	d_topology_feature_ref = d_focused_feature_ref;

	// set the feature focus to the topology feature
	d_feature_focus_ptr->set_focus( d_topology_feature_ref );

	// flip to the Topology Sections table
	d_view_state_ptr->change_tab( 2 );

	// display the focused feature with intent to edit boundary
	GPlatesQtWidgets::PlateClosureWidget::display_feature_topology(
		d_focused_feature_ref, d_feature_focus_rfg );

 	d_view_state_ptr->status_message( QObject::tr(
 		"Click on features to create the boundary; "
 		"use Apply to create new Topology; "
 		"use Ctrl+drag to re-orient the globe.")
	);
}


void
GPlatesQtWidgets::PlateClosureWidget::handle_use_coordinates_in_reverse()
{
#ifdef DEBUG
std::cout << "handle_use_coordinates_in_reverse" << std::endl;
#endif

	// pointers to the tables
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	GPlatesGui::FeatureTableModel &clicked_table = 
		d_view_state_ptr->feature_table_model();

	//
	// Determine which feature to reverse
	//

	// Clicked Table is the current tab
	// so we just want to reverse the display 
	if ( d_view_state_ptr->get_tab() == 0)
	{
		if ( clicked_table.current_index().isValid() )
		{
			// int index = clicked_table.current_index().row();
#ifdef DEBUG
std::cout << "use rever; tab 0 ; is valid" << std::endl;
#endif
			// just set the widget's flag
			d_use_reverse = !d_use_reverse;

			if ( d_use_reverse ) {
				lineedit_first->setText( d_last_coord );
				lineedit_last->setText( d_first_coord );
			} else {
				lineedit_first->setText( d_first_coord );
				lineedit_last->setText( d_last_coord );
			}
		}
	}

	// Sections Table is the current tab 
	if ( d_view_state_ptr->get_tab() == 2 )
	{
#ifdef DEBUG
std::cout << "use rever; tab 2" << std::endl;
std::cout << "use rever; tab 2; row=" << sections_table.current_index().row() << std::endl;
#endif

		if ( sections_table.current_index().isValid() )
		{
			int index = sections_table.current_index().row();

			// re-set the flag in the vector
			d_section_reverse_flags.at( index ) = !d_section_reverse_flags.at( index );

#ifdef DEBUG
std::cout << "use rever; tab 2 ; size=" << d_section_reverse_flags.size() << std::endl;
std::cout << "use rever; tab 2 ; is valid; index=" << index << "; use=" << d_section_reverse_flags.at( index ) << std::endl;
#endif
			if ( d_section_reverse_flags.at(index) ) {
				lineedit_first->setText( d_last_coord );
				lineedit_last->setText( d_first_coord );
				lineedit_use_reverse->setText( tr("yes") );
				
			} else {
				lineedit_first->setText( d_first_coord );
				lineedit_last->setText( d_last_coord );
				lineedit_use_reverse->setText( tr("no") );
			}

/////
			// Append the new boundary 
			append_boundary_to_feature( d_topology_feature_ref );

			// un-highlight the sections table row for this feature
			d_view_state_ptr->highlight_sections_table_row( index, false );
// ZZZZ
			return;
		}
	}
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_append_feature()
{
	// flip tab to sections table
	d_view_state_ptr->change_tab( 2 );

	// pointers to the tables
	GPlatesGui::FeatureTableModel &clicked_table = 
		d_view_state_ptr->feature_table_model();

	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	// transfer data to sections table
	int click_index = clicked_table.current_index().row();
	sections_table.begin_insert_features(0, 0);
	sections_table.geometry_sequence().push_back(
		clicked_table.geometry_sequence().at(click_index) );
	sections_table.end_insert_features();

	// append the current flag
	d_section_reverse_flags.push_back(d_use_reverse);

	// reset the current flag
	d_use_reverse = false;

	// append the current click_point
	d_section_click_points.push_back( std::make_pair( d_click_point_lat, d_click_point_lon ) );

	// set flag for visit from update_geom()
	d_visit_to_check_type = false;

	// process the sections table
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
	// NOTE: this undoes the connection to the clicked table
	d_feature_focus_ptr->unset_focus();

	// clear the "Clicked" table
	d_view_state_ptr->feature_table_model().clear();
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_remove_feature()
{
	// flip tab to Sections Table
	d_view_state_ptr->change_tab( 2 );

	// pointer to the table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	if ( sections_table.current_index().isValid() )
	{
		// get current selected index
		int index = sections_table.current_index().row();

		// erase that element from the Sections Table
		sections_table.begin_remove_features(index, index);
		sections_table.geometry_sequence().erase(
			sections_table.geometry_sequence().begin() + index);
		sections_table.end_remove_features();

		// remove the current click_point and reverse flags
		d_section_click_points.erase( d_section_click_points.begin() + index );
		d_section_reverse_flags.erase( d_section_reverse_flags.begin() + index );

		// clear out the widgets
		clear_widgets();

		// process the sections table 
		update_geometry();
// ZZZZ
		append_boundary_to_feature( d_topology_feature_ref );
	}

}

void
GPlatesQtWidgets::PlateClosureWidget::handle_insert_after()
{

}

void
GPlatesQtWidgets::PlateClosureWidget::handle_insert_before()
{

}


void
GPlatesQtWidgets::PlateClosureWidget::handle_clear()
{
	// clear the "Clicked" table
	d_view_state_ptr->feature_table_model().clear();

	// clear the widgets
	clear_widgets();

	// clear the focus data 
	d_focused_feature_layer_ptr->clear_rendered_geometries();

	// FIXME: should we unset focus?
}


void
GPlatesQtWidgets::PlateClosureWidget::handle_apply()
{
	// check if d_topology_feature_ref is set 

	// pointer to the Sections table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	// check for an empty sections table
	if ( sections_table.geometry_sequence().empty() ) 
	{
		QMessageBox::warning(this, 
			tr("No boundary sections are selected for this feature"),
			tr("There are no valid boundray sements to use for creating this feature."),
			QMessageBox::Ok);
		return;
	}

	// do one final update ; create properties this time
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// no topology feature ref exisits , so fire up the feature creation dialog
	if ( ! d_topology_feature_ref.is_valid() )
	{
		// tell the dialog that we are creating a topological feature
		d_create_feature_dialog->set_topological();

		bool success = d_create_feature_dialog->display();

		if ( ! success) {
			// The user cancelled the creation process. 
			// Return early and do not reset the widget.
			return;
		}

		// else, the feature was created by the dialog
		// and, append_boundary should have been called
	}

	// else, a d_topology_feature_ref exists, so clean up and focus on this feature 

	// Clear the widgets
	handle_clear();
	// FIXME: check handle_cancel for other 'end of operation' type things to do here ...

	// NOTE: this undoes the connection to highlight_sections_table 
	// d_feature_focus_ptr->unset_focus(); // will call display_feature() 
	d_feature_focus_ptr->set_focus( d_topology_feature_ref ); // will call display_feature()
}

void
GPlatesQtWidgets::PlateClosureWidget::handle_cancel()
{
	// clear the widgets
	handle_clear();

	// Set the widget states
	button_new_topology->setEnabled(true);
	button_edit_topology->setEnabled(false);
	label_type->setEnabled(false);
	lineedit_type->setEnabled(false);
	label_name->setEnabled(false);
	lineedit_name->setEnabled(false);
	label_plate_id->setEnabled(false);
	lineedit_plate_id->setEnabled(false);
	label_coordinates->setEnabled(false);
	label_first->setEnabled(false);
	label_last->setEnabled(false);
	lineedit_first->setEnabled(false);
	lineedit_last->setEnabled(false);
	lineedit_use_reverse->setEnabled(false);
	button_use_reverse->setEnabled(false);
	button_append_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(false);
	button_cancel->setEnabled(true);

	// clear the tables
	d_view_state_ptr->sections_feature_table_model().clear();
	d_view_state_ptr->feature_table_model().clear();

	// flip tab to clicked table
	d_view_state_ptr->change_tab( 0 );

	// empty the vertex list
	d_vertex_list.clear();
	d_tmp_index_vertex_list.clear();

	// empty the d_section_FOO vectors
	d_section_ptrs.clear();
	d_section_ids.clear();
	d_section_click_points.clear();
	d_section_reverse_flags.clear();

	// clear the working lists
	d_head_end_points.clear();
	d_tail_end_points.clear();
	d_intersection_points.clear();
	d_segments.clear();
	d_insert_segments.clear();

	// unset the references 
	d_focused_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	d_feature_focus_rfg = NULL;

	// unset the topology ref
	d_topology_feature_ref = GPlatesModel::FeatureHandle::weak_ref();

	// unset the d_geometry_opt_ptr
	d_geometry_opt_ptr = boost::none;

	// clear the drawing layers
	draw_all_layers_clear();
	d_view_state_ptr->globe_canvas().update_canvas();

	// reset widget defaults
	initialise_geometry(PLATEPOLYGON);

// FIXME: remove
	show_numbers();

	// NOTE: this undoes the connection to highlight_sections_table 
	if ( d_feature_focus_ptr->is_valid() ) {
		d_feature_focus_ptr->unset_focus(); // will call display_feature() 
	}

}



// ===========================================================================================
//
// Visitors for base geometry types
//

void
GPlatesQtWidgets::PlateClosureWidget::visit_multi_point_on_sphere(
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	// set type only
	if (d_visit_to_check_type){
		d_tmp_feature_type = GPlatesGlobal::MULTIPOINT_FEATURE;
		return;
	}

	// set the global flag for intersection processing 
	d_tmp_process_intersections = false;

	// simply append the points to the working list
	GPlatesMaths::MultiPointOnSphere::const_iterator itr;
	GPlatesMaths::MultiPointOnSphere::const_iterator beg = multi_point_on_sphere->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multi_point_on_sphere->end();
	for ( itr = beg ; itr != end ; ++itr)
	{
		d_tmp_index_vertex_list.push_back( *itr );
	}

	// return early if properties are not needed
	if (! d_visit_to_create_properties) { return; }

	// FIXME:
	// loop again and create a set of sourceGeometry property delegates 
}

void
GPlatesQtWidgets::PlateClosureWidget::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{ 
	// set type only
	if (d_visit_to_check_type) {
		d_tmp_feature_type = GPlatesGlobal::POINT_FEATURE;
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
		// single points just go in head list
		d_head_end_points.push_back( *point_on_sphere );
		return;
	}

	// set the global flag for intersection processing 
	d_tmp_process_intersections = false;

	// simply append the point to the working list
	d_tmp_index_vertex_list.push_back( *point_on_sphere );


	// return early if properties are not needed
	if (! d_visit_to_create_properties) { return; }

	// set the d_tmp vars to create a sourceGeometry property delegate 
	d_tmp_property_name = "position";
	d_tmp_value_type = "Point";

	const GPlatesModel::FeatureId fid(d_tmp_index_fid);

	const GPlatesModel::PropertyName prop_name =
		GPlatesModel::PropertyName::create_gpml( d_tmp_property_name);

	const GPlatesPropertyValues::TemplateTypeParameterType value_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gml( d_tmp_value_type );

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr = 
		GPlatesPropertyValues::GpmlPropertyDelegate::create( 
			fid,
			prop_name,
			value_type
		);
			
	// create a GpmlTopologicalPoint from the delegate
	GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type gtp_ptr =
		GPlatesPropertyValues::GpmlTopologicalPoint::create(
			pd_ptr);

	// Fill the vector of GpmlTopologicalSection::non_null_ptr_type 
	d_section_ptrs.push_back( gtp_ptr );
}

void
GPlatesQtWidgets::PlateClosureWidget::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	// set type only
	if (d_visit_to_check_type) {
		d_tmp_feature_type = GPlatesGlobal::POLYGON_FEATURE;
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
		return;
	}

	// return early if properties are not needed
	if (! d_visit_to_create_properties) 
	{ 
		return; 
	}

}

void
GPlatesQtWidgets::PlateClosureWidget::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{  
// std::cout << "GPlatesQtWidgets::PlateClosureWidget::visit_polyline_on_sphere()" << std::endl;

	// set type only
	if (d_visit_to_check_type) {
		d_tmp_feature_type = GPlatesGlobal::LINE_FEATURE;
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
		d_focus_head_end_points.push_back( *(polyline_on_sphere->vertex_begin()) );
		d_focus_tail_end_points.push_back( *(--polyline_on_sphere->vertex_end()) );
		return;
	}

	// set the global flag for intersection processing 
	d_tmp_process_intersections = true;

	// Write out each point of the polyline.
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = 
		polyline_on_sphere->vertex_begin();

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = 
		polyline_on_sphere->vertex_end();

	std::vector<GPlatesMaths::PointOnSphere> polyline_vertices;
	polyline_vertices.clear();
	
	for ( ; iter != end; ++iter) 
	{
		polyline_vertices.push_back( *iter );
	}

	// set the head and tail end_points
	d_head_end_points.push_back( *(polyline_on_sphere->vertex_begin()) );
	d_tail_end_points.push_back( *(--polyline_on_sphere->vertex_end()) );

	// check for reverse flag
	if (d_tmp_index_use_reverse) 
	{
		d_tmp_index_vertex_list.insert( d_tmp_index_vertex_list.end(), 
			polyline_vertices.rbegin(), polyline_vertices.rend() );
	}
	else 
	{
		d_tmp_index_vertex_list.insert( d_tmp_index_vertex_list.end(), 
			polyline_vertices.begin(), polyline_vertices.end() );
	}

	// return early if properties are not needed
	if (! d_visit_to_create_properties) { return; }

	// Set the d_tmp vars to create a sourceGeometry property delegate 
	d_tmp_property_name = "centerLineOf";
	d_tmp_value_type = "LineString";

	const GPlatesModel::FeatureId fid(d_tmp_index_fid);

	const GPlatesModel::PropertyName prop_name =
		GPlatesModel::PropertyName::create_gpml(d_tmp_property_name);

	const GPlatesPropertyValues::TemplateTypeParameterType value_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gml( d_tmp_value_type );

	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr = 
		GPlatesPropertyValues::GpmlPropertyDelegate::create( 
			fid,
			prop_name,
			value_type
		);

	// create a GpmlTopologicalLineSection from the delegate
	GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type gtls_ptr =
		GPlatesPropertyValues::GpmlTopologicalLineSection::create(
			pd_ptr,
			boost::none,
			boost::none,
			d_tmp_index_use_reverse);

	// Fill the vector of GpmlTopologicalSection::non_null_ptr_type 
	d_section_ptrs.push_back( gtls_ptr );
}


// ===========================================================================================
//
// Updater function for processing sections table into geom and boundary prop
//

void
GPlatesQtWidgets::PlateClosureWidget::update_geometry()
{
std::cout << "GPlatesQtWidgets::PlateClosureWidget::update_geometry()" << std::endl;

	// clear most of  layers
	d_temporary_geometry_layer_ptr->clear_rendered_geometries();
	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_segments_layer_ptr->clear_rendered_geometries();
	d_end_points_layer_ptr->clear_rendered_geometries();
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_click_points_layer_ptr->clear_rendered_geometries();

	d_view_state_ptr->globe_canvas().update_canvas();

	// clear some of the working lists

	// DO NOT clear() these two ! used by create_sections_from_sections_table
	// d_section_reverse_flags.clear()
	// d_section_click_points.clear();

	// all these get set by create_sections_from_sections_table
	d_section_ids.clear();
	d_vertex_list.clear();
	d_section_ptrs.clear();
	d_head_end_points.clear();
	d_tail_end_points.clear();
	d_intersection_points.clear();
	d_segments.clear();
	d_insert_segments.clear();
	d_focus_head_end_points.clear();
	d_focus_tail_end_points.clear();

	// loop over Sections Table to fill d_vertex_list
	create_sections_from_sections_table();

	// Set the num_sections
	lineedit_num_sections->setText(QString::number( d_section_ptrs.size() ));

	// create the temp geom.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;
	geometry_opt_ptr_type geometry_opt_ptr = 
		create_geometry_from_vertex_list( d_vertex_list, d_geometry_type, validity);

	// Set the d_geometry_opt_ptr to the newly created geom;
	d_geometry_opt_ptr = geometry_opt_ptr;

	draw_all_layers();
}

// ===========================================================================================
//
// draw_ functions
//
void
GPlatesQtWidgets::PlateClosureWidget::draw_all_layers_clear()
{
std::cout << "GPlatesQtWidgets::PlateClosureWidget::draw_all_layers_clear()" << std::endl;
	// clear all layers
	d_temporary_geometry_layer_ptr->clear_rendered_geometries();
	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_segments_layer_ptr->clear_rendered_geometries();
	d_end_points_layer_ptr->clear_rendered_geometries();
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_click_points_layer_ptr->clear_rendered_geometries();

	d_view_state_ptr->globe_canvas().update_canvas();
}

void
GPlatesQtWidgets::PlateClosureWidget::draw_all_layers()
{
std::cout << "GPlatesQtWidgets::PlateClosureWidget::draw_all_layers()" << std::endl;
	// draw all the layers
	draw_temporary_geometry();
	draw_focused_geometry();
	draw_segments();
	draw_end_points();
	draw_intersection_points();
	draw_click_points();
	draw_click_point();

	d_view_state_ptr->globe_canvas().update_canvas();

	show_numbers();
}


void
GPlatesQtWidgets::PlateClosureWidget::draw_temporary_geometry()
{
std::cout << "GPlatesQtWidgets::PlateClosureWidget::draw_temporary_geometry()" << std::endl;

	d_temporary_geometry_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	if (d_geometry_opt_ptr) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::WHITE;

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
				*d_geometry_opt_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

		d_temporary_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}


void
GPlatesQtWidgets::PlateClosureWidget::draw_focused_geometry()
{
	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	if (d_feature_focus_rfg)
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::WHITE;

		GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
				d_feature_focus_rfg->geometry(),
				colour,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

		d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);

		// visit to get end_points
		d_focus_head_end_points.clear();
		d_focus_tail_end_points.clear();
		d_visit_to_get_focus_end_points = true;
		d_feature_focus_rfg->geometry()->accept_visitor(*this);
		d_visit_to_get_focus_end_points = false;

		// draw the focused end_points
		draw_focused_geometry_end_points();
	}
	d_view_state_ptr->globe_canvas().update_canvas();
}


void
GPlatesQtWidgets::PlateClosureWidget::draw_focused_geometry_end_points()
{
	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;

	// draw head points
	itr = d_focus_head_end_points.begin();
	end = d_focus_head_end_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::BLACK;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			// Add to layer.
			d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// draw tail end_points
	itr = d_focus_tail_end_points.begin();
	end = d_focus_tail_end_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::WHITE;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			// Add to layer.
			d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
}

void
GPlatesQtWidgets::PlateClosureWidget::draw_segments()
{
	d_segments_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>::iterator itr, end;
	itr = d_segments.begin();
	end = d_segments.end();
	for ( ; itr != end ; ++itr)
	{
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->get()->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::GREY;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			d_segments_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}

void
GPlatesQtWidgets::PlateClosureWidget::draw_end_points()
{
	d_end_points_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;

	// draw head points
	itr = d_head_end_points.begin();
	end = d_head_end_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::BLACK;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// draw tail end_points
	itr = d_tail_end_points.begin();
	end = d_tail_end_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::GREY;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::REGULAR_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}

void
GPlatesQtWidgets::PlateClosureWidget::draw_intersection_points()
{
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	// loop over click points 
	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;
	itr = d_intersection_points.begin();
	end = d_intersection_points.end();
	for ( ; itr != end ; ++itr)
	{
		// make a point from the coordinates
		//GPlatesMaths::PointOnSphere click_pos = GPlatesMaths::make_point_on_sphere(
		//	GPlatesMaths::LatLonPoint( itr->first, itr->second ) );

		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::WHITE;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			d_intersection_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}



void
GPlatesQtWidgets::PlateClosureWidget::draw_click_point()
{
	d_click_points_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	// make a point from the coordinates
	GPlatesMaths::PointOnSphere click_pos = GPlatesMaths::make_point_on_sphere(
		GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon) );

	// get a geom
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
		click_pos.clone_as_geometry();

	if (pos_ptr) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::GREY;

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
				pos_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

		d_click_points_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}

void
GPlatesQtWidgets::PlateClosureWidget::draw_click_points()
{
	d_click_points_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	// loop over click points 
	std::vector<std::pair<double, double> >::iterator itr, end;
	itr = d_section_click_points.begin();
	end = d_section_click_points.end();
	for ( ; itr != end ; ++itr)
	{
		// make a point from the coordinates
		GPlatesMaths::PointOnSphere click_pos = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint( itr->first, itr->second ) );

		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			click_pos.clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::BLACK;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			d_click_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}



// ===========================================================================================
// ===========================================================================================
// ===========================================================================================
// ===========================================================================================



void
GPlatesQtWidgets::PlateClosureWidget::create_sections_from_sections_table()
{
	// clear the working lists
	d_vertex_list.clear();
	d_section_ptrs.clear();
	d_section_ids.clear();
	// but DO NOT clear() d_section_reverse_flags !  since it is used below 

	// access the sections table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	//
	// super short cut for empty table
	//
	if ( sections_table.geometry_sequence().empty() ) { 
		return; 
	}

	// get the size of the table
	d_tmp_sections_size = sections_table.geometry_sequence().size();

	// loop over each geom in the Sections Table
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator iter;
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator end;

	iter = sections_table.geometry_sequence().begin();
	end = sections_table.geometry_sequence().end();

	// re-set the global d_tmp_index to zero for the start of the list;
	d_tmp_index = 0;

	for ( ; iter != end ; ++iter)
	{
		GPlatesModel::ReconstructionGeometry *rg = iter->get();

		GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);

		// Set the fid for the tmp_index section 
		d_tmp_index_fid = rfg->feature_ref()->feature_id();

		// fill the section ids vector
		d_section_ids.push_back( d_tmp_index_fid );

		// set the tmp reverse flag to this feature's flag
		d_tmp_index_use_reverse = d_section_reverse_flags.at(d_tmp_index);

#ifdef DEBUG1
std::cout << "create_sections_from_sections_table: d_tmp_index = " << d_tmp_index << std::endl;
static const GPlatesModel::PropertyName name_property_name =
	GPlatesModel::PropertyName::create_gml("name");
GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
string_finder.visit_feature_handle( *(rfg->feature_ref()) );
if (string_finder.found_strings_begin() != string_finder.found_strings_end()) 
{
	GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name =
		 *string_finder.found_strings_begin();
	qDebug() << "create_sections_from_sections_table: name=" << GPlatesUtils::make_qstring( name->value() );
}
std::cout << "create_sections_from_sections_table: d_use_rev = " << d_tmp_index_use_reverse 
<< std::endl;
#endif

		// clear the tmp index list
		d_tmp_index_vertex_list.clear();

		// visit the geoms.  :
		// fill additional d_tmp_index_ vars 
		// fill d_head_end_points d_tail_end_points 
		d_visit_to_check_type = false;
		(*iter)->geometry()->accept_visitor(*this);

		// FIXME: DO WE NEED THIS? d_tmp_process_intersections = false; // Works without
		// re-set the check intersections flag for a sigle item on the list
		if ( d_tmp_sections_size == 1 ) {
			d_tmp_process_intersections = false;
		}

		//
		// Check for intersection
		//
		if ( d_tmp_process_intersections )
		{
			process_intersections();

			// save this segment
			// make a polyline
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type pos_ptr = 
				GPlatesMaths::PolylineOnSphere::create_on_heap( d_tmp_index_vertex_list );

			d_segments.push_back( pos_ptr );

			// d_tmp_index_vertex_list may have been modified by process_intersections()
			d_vertex_list.insert( d_vertex_list.end(), 
				d_tmp_index_vertex_list.begin(), d_tmp_index_vertex_list.end() );
		}
		else
		{
			// simply insert tmp items on the list
			d_vertex_list.insert( d_vertex_list.end(), 
				d_tmp_index_vertex_list.begin(), d_tmp_index_vertex_list.end() );
		}

		// update counter d_tmp_index
		++d_tmp_index;
	}
}

void
GPlatesQtWidgets::PlateClosureWidget::process_intersections()
{
	// access the sections table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	// set the tmp click point to d_tmp_index feture's click point
	d_click_point_lat = d_section_click_points.at(d_tmp_index).first;
	d_click_point_lon = d_section_click_points.at(d_tmp_index).second;

	GPlatesMaths::PointOnSphere click_pos = GPlatesMaths::make_point_on_sphere(
		GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon) );

	d_click_point_ptr = &click_pos;

	const GPlatesMaths::PointOnSphere const_pos(click_pos); 

	// index math to close the loop of sections
	if ( d_tmp_index == (d_tmp_sections_size - 1) ) 
	{
		d_tmp_next_index = 0;
		d_tmp_prev_index = d_tmp_index - 1;
	}
	else if ( d_tmp_index == 0 )
	{
		d_tmp_next_index = d_tmp_index + 1;
		d_tmp_prev_index = d_tmp_sections_size - 1;
	}
	else
	{
		d_tmp_next_index = d_tmp_index + 1;
		d_tmp_prev_index = d_tmp_index - 1;
	}

#ifdef DEBUG1
std::cout << "PlateClosureWidget::process_intersections: "
<< "d_click_point_lat = " << d_click_point_lat << "; " 
<< "d_click_point_lon = " << d_click_point_lon << "; " 
<< std::endl;
std::cout << "PlateClosureWidget::process_intersections: "
<< "d_prev_index = " << d_tmp_prev_index << "; " 
<< "d_tmp_index = " << d_tmp_index << "; " 
<< "d_next_index = " << d_tmp_next_index << "; " 
<< std::endl;
#endif

	// reset intersection variables
	d_num_intersections_with_prev = 0;
	d_num_intersections_with_next = 0;

	//
	// check for startIntersection
	//
	// NOTE: the d_tmp_index segment may have had its d_tmp_index_vertex_list reversed, 
	// so use that list of points_on_sphere, rather than the geom from Sections Table.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type tmp_for_prev_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( d_tmp_index_vertex_list );

	// Access the Sections Table for the PREV item's geom
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator prev;
	prev = sections_table.geometry_sequence().begin() + d_tmp_prev_index;
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type prev_gos = (*prev)->geometry();

	// Set the d_tmp_feature_type by visiting the PREV geom
	d_visit_to_check_type = true;
	prev_gos->accept_visitor(*this);
	d_visit_to_check_type = false;

	// No need to process intersections with POINT features 
	if (d_tmp_feature_type == GPlatesGlobal::POINT_FEATURE ) {
		return;
	}

	// else process the geom as a LINE
	const GPlatesMaths::PolylineOnSphere *prev_polyline = 
		dynamic_cast<const GPlatesMaths::PolylineOnSphere *>( prev_gos.get() );

	// check if INDEX and PREV polylines intersect
	compute_intersection(
		tmp_for_prev_polyline.get(),
		prev_polyline,
		GPlatesQtWidgets::PlateClosureWidget::INTERSECT_PREV);

	// if they do, then create the startIntersection property value
	if ( d_visit_to_create_properties && (d_num_intersections_with_prev != 0) )
	{
		// if there was an intersection, create a startIntersection property value
		GPlatesModel::ReconstructionGeometry *prev_rg = prev->get();

		GPlatesModel::ReconstructedFeatureGeometry *prev_rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(prev_rg);

		// intersection_geometry
		const GPlatesModel::FeatureId prev_fid = prev_rfg->feature_ref()->feature_id();

		const GPlatesModel::PropertyName prop_name1 =
			GPlatesModel::PropertyName::create_gpml("centerLineOf");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type1 =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString" );

		// create the intersectionGeometry property delegate
		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type geom_delegte = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				prev_fid,
				prop_name1,
				value_type1
			);

		// reference_point
		 GPlatesPropertyValues::GmlPoint::non_null_ptr_type ref_point =
			GPlatesPropertyValues::GmlPoint::create( const_pos );

		// reference_point_plate_id
		const GPlatesModel::FeatureId index_fid(d_tmp_index_fid);

		const GPlatesModel::PropertyName prop_name2 =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type2 =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("PlateId" );

		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type plate_id_delegate = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				index_fid,
				prop_name2,
				value_type2
			);

		// Create the start GpmlTopologicalIntersection
		GPlatesPropertyValues::GpmlTopologicalIntersection start_ti(
			geom_delegte,
			ref_point,
			plate_id_delegate);
			
		// Set the start instersection
		GPlatesPropertyValues::GpmlTopologicalLineSection* gtls_ptr =
			dynamic_cast<GPlatesPropertyValues::GpmlTopologicalLineSection*>(
				d_section_ptrs.at( d_tmp_index ).get() );

		gtls_ptr->set_start_intersection( start_ti );
	}


	//
	// Since d_tmp_index_vertex_list may have been changed by PREV, create another polyline 
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type tmp_for_next_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( d_tmp_index_vertex_list );

	//
	// access the Sections Table for the NEXT item
	//
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator next;
	next = sections_table.geometry_sequence().begin() + d_tmp_next_index;
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type next_gos = (*next)->geometry();

	// Set the d_tmp_feature_type by visiting the NEXT geom
	d_visit_to_check_type = true;
	next_gos->accept_visitor(*this);
	d_visit_to_check_type = false;

	// No need to process intersections with POINT features 
	if (d_tmp_feature_type == GPlatesGlobal::POINT_FEATURE ) {
		return;
	}

	// else process the geom as LINE
	const GPlatesMaths::PolylineOnSphere *next_polyline = 
		dynamic_cast<const GPlatesMaths::PolylineOnSphere *>( next_gos.get() );

	// check if INDEX and NEXT polylines intersect
	compute_intersection(
		tmp_for_next_polyline.get(),
		next_polyline,
		GPlatesQtWidgets::PlateClosureWidget::INTERSECT_NEXT);

	// if they do, then create the endIntersection property value
	if ( d_visit_to_create_properties && (d_num_intersections_with_next != 0) )
	{
		// if there was an intersection, create a endIntersection property value
		GPlatesModel::ReconstructionGeometry *rg = next->get();

		GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);

		// intersection_geometry
		const GPlatesModel::FeatureId next_fid = rfg->feature_ref()->feature_id();

		const GPlatesModel::PropertyName prop_name1 =
			GPlatesModel::PropertyName::create_gpml("centerLineOf");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type1 =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString" );

		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type geom_delegte = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				next_fid,
				prop_name1,
				value_type1
			);

		// reference_point
		 GPlatesPropertyValues::GmlPoint::non_null_ptr_type ref_point =
			GPlatesPropertyValues::GmlPoint::create( const_pos );

		// reference_point_plate_id
		const GPlatesModel::FeatureId index_fid(d_tmp_index_fid);

		const GPlatesModel::PropertyName prop_name2 =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type2 =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("PlateId" );

		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type plate_id_delegate = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				index_fid,
				prop_name2,
				value_type2
			);

		// Create the end GpmlTopologicalIntersection
		GPlatesPropertyValues::GpmlTopologicalIntersection end_ti(
			geom_delegte,
			ref_point,
			plate_id_delegate);
			
		// Set the end instersection
		GPlatesPropertyValues::GpmlTopologicalLineSection* gtls_ptr =
			dynamic_cast<GPlatesPropertyValues::GpmlTopologicalLineSection*>(
				d_section_ptrs.at( d_tmp_index ).get() );

		gtls_ptr->set_end_intersection( end_ti );
	}

}

void
GPlatesQtWidgets::PlateClosureWidget::compute_intersection(
	const GPlatesMaths::PolylineOnSphere* node1_polyline,
	const GPlatesMaths::PolylineOnSphere* node2_polyline,
	GPlatesQtWidgets::PlateClosureWidget::NeighborRelation relation)
{
	// variables to save results of intersection
	std::list<GPlatesMaths::PointOnSphere> intersection_points;
	std::list<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> partitioned_lines;

	int num_intersect = 0;

	num_intersect = GPlatesMaths::PolylineIntersections::partition_intersecting_polylines(
		*node1_polyline,
		*node2_polyline,
		intersection_points,
		partitioned_lines);

#ifdef DEBUG1
std::cout << std::endl
<< "PlateClosureWidget::compute_intersection: " << "num_intersect = " << num_intersect 
<< std::endl;
#endif


	// switch on relation enum to set node1's member data
	switch ( relation )
	{
		case GPlatesQtWidgets::PlateClosureWidget::INTERSECT_PREV :
#ifdef DEBUG1
std::cout << "PlateClosureWidget::compute_intersection: INTERSECT_PREV: " << std::endl;
#endif
			d_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesQtWidgets::PlateClosureWidget::INTERSECT_NEXT:
#ifdef DEBUG1
std::cout << "PlateClosureWidget::compute_intersection: INTERSECT_NEXT: " << std::endl;
#endif
			d_num_intersections_with_next = num_intersect;
			break;

		case GPlatesQtWidgets::PlateClosureWidget::NONE :
		case GPlatesQtWidgets::PlateClosureWidget::OTHER :
		default :
			// somthing bad happened freak out
			break;
	}

	if ( num_intersect == 0 )
	{
		// no change to d_tmp_index_vertex_list
		return;
	}
	else if ( num_intersect == 1)
	{
		// pair of polyline lists from intersection
		std::pair<
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		> parts;

		// unambiguously identify partitioned lines:
		//
		// parts.first.front is the head of node1_polyline
		// parts.first.back is the tail of node1_polyline
		// parts.second.front is the head of node2_polyline
		// parts.second.back is the tail of node2_polyline
		//
		parts = GPlatesMaths::PolylineIntersections::identify_partitioned_polylines(
			*node1_polyline,
			*node2_polyline,
			intersection_points,
			partitioned_lines);


#ifdef DEBUG1
GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter, end;
iter = parts.first.front()->vertex_begin();
end = parts.first.front()->vertex_end();
std::cout << "PlateClosureWidget::compute_intersection:: HEAD: verts:" << std::endl;
for ( ; iter != end; ++iter) {
std::cout << "llp=" << GPlatesMaths::make_lat_lon_point(*iter) << std::endl;
}

iter = parts.first.back()->vertex_begin();
end = parts.first.back()->vertex_end();
std::cout << "PlateClosureWidget::compute_intersection:: TAIL: verts:" << std::endl;
for ( ; iter != end; ++iter) {
std::cout << "llp=" << GPlatesMaths::make_lat_lon_point(*iter) << std::endl;
}
#endif

		// now check which element of parts.first
		// is closest to click_point:

// FIXME :
// we should first rotate the click point with the plate id of intersection_geometry_fid 
// before calling is_close_to()

// PROXIMITY
		GPlatesMaths::real_t closeness_inclusion_threshold = 0.9;
		const GPlatesMaths::real_t cit_sqrd =
			closeness_inclusion_threshold * closeness_inclusion_threshold;
		const GPlatesMaths::real_t latitude_exclusion_threshold = sqrt(1.0 - cit_sqrd);

		// these get filled by calls to is_close_to()
		bool click_close_to_head;
		bool click_close_to_tail;
		GPlatesMaths::real_t closeness_head;
		GPlatesMaths::real_t closeness_tail;

		// set head closeness
		click_close_to_head = parts.first.front()->is_close_to(
			*d_click_point_ptr,
			closeness_inclusion_threshold,
			latitude_exclusion_threshold,
			closeness_head);

		// set tail closeness
		click_close_to_tail = parts.first.back()->is_close_to(
			*d_click_point_ptr,
			closeness_inclusion_threshold,
			latitude_exclusion_threshold,
			closeness_tail);


		// Make sure that the click point is close to something!
		if ( !click_close_to_head && !click_close_to_tail ) 
		{
			// FIXME : freak out!
			std::cerr << "PlateClosureWidget::compute_intersection: " << std::endl
			<< "WARN: click point not close to anything!" << std::endl
			<< "WARN: Unable to set boundary feature intersection flags!" 
			<< std::endl << std::endl;
			return;
		}

#ifdef DEBUG1
std::cout << "PlateClosureWidget::compute_intersection: "
<< "closeness_head=" << closeness_head << " and " 
<< "closeness_tail=" << closeness_tail << std::endl;
#endif

		// now compare the closeness values to set relation
		if ( closeness_head > closeness_tail )
		{
#ifdef DEBUG1
qDebug() << "PlateClosureWidget::compute_intersection: " << "use HEAD";
#endif
			d_closeness = closeness_head;

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesQtWidgets::PlateClosureWidget::INTERSECT_PREV :
					//d_use_head_from_intersect_prev = true;
					//d_use_tail_from_intersect_prev = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					// save intersection point
					d_intersection_points.push_back( *(parts.first.front()->vertex_begin()) );
					break;
	
				case GPlatesQtWidgets::PlateClosureWidget::INTERSECT_NEXT:
					//d_use_head_from_intersect_next = true;
					//d_use_tail_from_intersect_next = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.front()->vertex_begin()) );
					break;

				default:
					break;
			}
			return; // node1's relation has been set
		} 
		else if ( closeness_tail > closeness_head )
		{
			d_closeness = closeness_tail;
#ifdef DEBUG1
qDebug() << "PlateClosureWidget::compute_intersection: " << "use TAIL" ;
#endif

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesQtWidgets::PlateClosureWidget::INTERSECT_PREV :
					//d_use_tail_from_intersect_prev = true;
					//d_use_head_from_intersect_prev = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.back()->vertex_begin()) );

					break;
	
				case GPlatesQtWidgets::PlateClosureWidget::INTERSECT_NEXT:
					//d_use_tail_from_intersect_next = true;
					//d_use_head_from_intersect_next = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.back()->vertex_begin()) );
					break;

				default:
					break;
			}
			return; // node1's relation has been set
		} 

	} // end of else if ( num_intersect == 1 )
	else 
	{
		// num_intersect must be 2 or greater oh no!
		std::cerr << "PlateClosureWidget::compute_intersection: " << std::endl
		<< "WARN: num_intersect=" << num_intersect << std::endl 
		<< "WARN: Unable to set boundary feature intersection relations!" << std::endl
		<< "WARN: Make sure boundary feature's only intersect once." << std::endl 
		<< std::endl
		<< std::endl;
	}

}


void
GPlatesQtWidgets::PlateClosureWidget::append_boundary_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{

qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
qDebug() << "PlateClosureWidget::append_boundary_value_to_feature() feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );

static const GPlatesModel::PropertyName name_property_name =
	GPlatesModel::PropertyName::create_gml("name");
GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
string_finder.visit_feature_handle( *feature_ref );
if (string_finder.found_strings_begin() != string_finder.found_strings_end()) 
{
	GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name =
		 *string_finder.found_strings_begin();
	qDebug() << "PlateClosureWidget::append_boundary_value_to_feature: name=" 
	<< GPlatesUtils::make_qstring( name->value() );
}

	// do one final update ; create properties this time
	d_visit_to_create_properties = true;

	// process the sections table into d_section_ptrs
	update_geometry();

	// find the prop to remove
	GPlatesModel::PropertyName boundary_prop_name = 
		GPlatesModel::PropertyName::create_gpml("boundary");

	GPlatesModel::FeatureHandle::properties_iterator iter = feature_ref->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_ref->properties_end();
	// loop over properties
	for ( ; iter != end; ++iter) 
	{
		// double check for validity and nullness
		if(!iter.is_valid()){ continue; }
		if(*iter == NULL)   { continue; }  
		// FIXME: previous edits to the feature leave property pointers NULL

		// passed all checks, make the name and test
		GPlatesModel::PropertyName test_name = (*iter)->property_name();

qDebug() << "name = " << GPlatesUtils::make_qstring_from_icu_string(test_name.get_name());

		if ( test_name == boundary_prop_name )
		{
qDebug() << "call remove_property_container on = " << GPlatesUtils::make_qstring_from_icu_string(test_name.get_name());
			// Delete the old boundary 
			GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
			feature_ref->remove_property_container(iter, transaction);
			transaction.commit();
			// FIXME: this seems to create NULL pointers in the properties collection
			// see FIXME note above to check for NULL? 
			// Or is this to be expected?

			// FIXME: do this?
			// d_feature_focus_ptr->announce_modification_of_focused_feature();
			break;
		}
	} // loop over properties

	// create the TopologicalPolygon
	GPlatesModel::PropertyValue::non_null_ptr_type topo_poly_value =
		GPlatesPropertyValues::GpmlTopologicalPolygon::create(d_section_ptrs);

	const GPlatesPropertyValues::TemplateTypeParameterType topo_poly_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");

	// create the ConstantValue
	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
		GPlatesPropertyValues::GpmlConstantValue::create(topo_poly_value, topo_poly_type);

	// get the time period for the feature
	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
	time_period_finder.visit_feature_handle( *feature_ref );

	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period = 
		*time_period_finder.found_time_periods_begin();

	//GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type *tp = 
	GPlatesPropertyValues::GmlTimePeriod* tp = 
	const_cast<GPlatesPropertyValues::GmlTimePeriod *>( time_period.get() );

	// GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type ttpp =
	GPlatesUtils::non_null_intrusive_ptr<
		GPlatesPropertyValues::GmlTimePeriod, 
		GPlatesUtils::NullIntrusivePointerHandler> ttpp(
				tp,
				GPlatesUtils::NullIntrusivePointerHandler()
		);


	// create the TimeWindow
	GPlatesPropertyValues::GpmlTimeWindow tw = GPlatesPropertyValues::GpmlTimeWindow(
			constant_value, 
			ttpp,
			topo_poly_type);

	// use the time window
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;

	time_windows.push_back(tw);

	// create the PiecewiseAggregation
	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type aggregation =
		GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, topo_poly_type);
	
	// Add a gpml:boundary Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
		aggregation,
		GPlatesModel::PropertyName::create_gpml("boundary"),
		feature_ref);

	// Set the ball rolling again ...
	d_view_state_ptr->reconstruct();
}

// FIXME : save this code for ref on how to create things

#if 0
		const GPlatesModel::FeatureId fid = rfg->feature_ref()->feature_id();
			
		const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml("gpml:centerLineOf");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("gml:LineString");
			
		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				fid,
				prop_name,
				value_type
			);
				
		//d_source_geometry_property_delegate_ptrs.push_back( pd );
#endif

#if 0
//
// FIXME: save this code for generic example of how to 
// Create a sourceGeometry property delegate 
//
		const GPlatesModel::FeatureId fid(d_tmp_index_fid);
		const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml( d_tmp_property_name);
		const GPlatesPropertyValues::TemplateTypeParameterType value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml( d_tmp_value_type );

		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd = 
			GPlatesPropertyValues::GpmlPropertyDelegate::create( 
				fid,
				prop_name,
				value_type
			);
#endif

void
GPlatesQtWidgets::PlateClosureWidget::show_numbers()
{
std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl; 
std::cout << "show_numbers: " << std::endl; 
std::cout << "d_section_ptrs.size()         = " << d_section_ptrs.size() << std::endl; 
std::cout << "d_section_ids.size()          = " << d_section_ids.size() << std::endl; 
std::cout << "d_section_click_points.size() = " << d_section_click_points.size() <<std::endl;
std::cout << "d_section_reverse_flags.size()= " << d_section_reverse_flags.size() <<std::endl;
std::cout << "d_vertex_list.size()          = " << d_vertex_list.size() << std::endl; 
std::cout << "d_tmp_index_vertex_list.size()= " << d_tmp_index_vertex_list.size() << std::endl;
std::cout << "d_head_end_points.size()      = " << d_head_end_points.size() << std::endl; 
std::cout << "d_tail_end_points.size()      = " << d_tail_end_points.size() << std::endl; 
std::cout << "d_intersection_points.size()  = " << d_intersection_points.size() << std::endl; 
std::cout << "d_segments.size()             = " << d_segments.size() << std::endl; 
std::cout << "d_insert_segments.size()      = " << d_insert_segments.size() << std::endl; 
std::cout << "d_focus_head_end_points.size()= " << d_focus_head_end_points.size() << std::endl;
std::cout << "d_focus_tail_end_points.size()= " << d_focus_tail_end_points.size() << std::endl;
std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl; 
}

//
// END
//
