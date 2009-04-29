/**
 * \file 
 * $Revision: 4821 $
 * $Date: 2009-02-13 18:54:19 -0800 (Fri, 13 Feb 2009) $ 
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

// #define DEBUG
// #define DEBUG1

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

#include "BuildTopologyWidget.h"

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
#include "model/ReconstructedFeatureGeometryFinder.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/TopologySectionsFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"

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
#include "property-values/XsString.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


// 
// Constructor
// 
GPlatesQtWidgets::BuildTopologyWidget::BuildTopologyWidget(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface &model_interface,
		ViewportWindow &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_feature_focus_ptr(&feature_focus),
	d_model_interface(&model_interface),
	d_view_state_ptr(&view_state_),
	d_create_feature_dialog(
		new CreateFeatureDialog(
			model_interface, view_state_, 
			GPlatesQtWidgets::CreateFeatureDialog::TOPOLOGICAL, this) ),
	d_geometry_type(GPlatesQtWidgets::BuildTopologyWidget::PLATEPOLYGON),
	d_topology_geometry_opt_ptr(boost::none)
{
	setupUi(this);

	create_child_rendered_layers();

	// set the internal state flags
	d_is_active = false;
	d_in_edit = false;

	d_visit_to_check_type = false;
	d_visit_to_create_properties = false;
	d_visit_to_get_focus_end_points = false;

	d_insert_index = -1;

	// clear all the lineedit widgets
	clear_widgets();

	// Set the widget states
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
	checkbox_reverse->setEnabled(false);
	checkbox_reverse->setChecked(false);
	button_add_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);


	// Use Coordinates in Reverse
	QObject::connect(checkbox_reverse, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_reverse()));
	
	// Choose Feature button 
	QObject::connect(button_add_feature, 
		SIGNAL(clicked()),
		this, 
		SLOT(handle_add_feature()));
	
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
GPlatesQtWidgets::BuildTopologyWidget::activate()
{
	d_is_active = true;

#if 0
// FIXME: some users want the creation process to start here, upon tool activation

	// show the CreateFeature dialog
	bool success = d_create_feature_dialog->display();

	if ( ! success) {
		// The user cancelled the creation process. 
		handle_cancel();
	}

	// else the widget is up and basic feature info is being created.
#endif
}

void
GPlatesQtWidgets::BuildTopologyWidget::deactivate()
{
	d_is_active = false;
}

void
GPlatesQtWidgets::BuildTopologyWidget::connect_to_focus_signals(bool state)
{
	if (state) 
	{
		// Subscribe to focus events. 
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this,
			SLOT( set_focus(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
	
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focused_feature_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this,
			SLOT( set_focus(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
	
	} 
	else 
	{
		// Un-Subscribe to focus events. 
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this, 
			0);
	
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focused_feature_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this, 
			0);
	}
}


void
GPlatesQtWidgets::BuildTopologyWidget::create_child_rendered_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layers to draw geometries.

	// the topology is drawn on the bottom layer
	d_topology_geometry_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// the segments resulting from intersections of line data come next
	d_segments_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// points where line data intersects and cuts the src geometry
	d_intersection_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// click points of the boundary feature data 
	d_click_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// head and tail points of src geometry 
	d_end_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// Put the focus layer on top
	d_focused_feature_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.

	// Activate layers
	d_topology_geometry_layer_ptr->set_active();
	d_focused_feature_layer_ptr->set_active();
	d_segments_layer_ptr->set_active();
	d_intersection_points_layer_ptr->set_active();
	d_click_points_layer_ptr->set_active();
	d_end_points_layer_ptr->set_active();
}

void
GPlatesQtWidgets::BuildTopologyWidget::initialise_geometry(
		GeometryType geom_type)
{
	clear_widgets();
	d_use_reverse = false;
	d_tmp_index_use_reverse = false;
	d_geometry_type = geom_type;

	d_tmp_feature_type = GPlatesGlobal::UNKNOWN_FEATURE;
}



void
GPlatesQtWidgets::BuildTopologyWidget::clear_widgets()
{
	lineedit_type->clear();
	lineedit_name->clear();
	lineedit_plate_id->clear();
	lineedit_first->clear();
	lineedit_last->clear();
	lineedit_num_sections->clear();
}

// Fill some of the widgets 
void
GPlatesQtWidgets::BuildTopologyWidget::fill_widgets(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
	// Populate the widget from the FeatureHandle:
	
	// Feature Type.
	lineedit_type->setText(GPlatesUtils::make_qstring_from_icu_string(
			feature_ref->feature_type().build_aliased_name()) );

	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name;
	
	if ( GPlatesFeatureVisitors::get_property_value(
			*feature_ref, name_property_name, name) )
	{
		// The feature has one or more name properties. Use the first one for now.
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
	}

	// Plate ID.
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	const GPlatesPropertyValues::GpmlPlateId *plate_id;

	if ( GPlatesFeatureVisitors::get_property_value( 
			*feature_ref, plate_id_property_name, plate_id ) )
	{
		// The feature has a plate ID of the desired kind.
		// The feature has a reconstruction plate ID.
		lineedit_plate_id->setText( QString::number( plate_id->value() ) );
	}
	
	if ( feature_ref.is_valid() ) 
	{
		// create a dummy tree
		// use it and the populator to get coords
		QTreeWidget *tree_geometry = new QTreeWidget(this);
		tree_geometry->hide();

		GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator populator(
			d_view_state_ptr->reconstruction(), *tree_geometry);

		populator.populate(*feature_ref, associated_rfg);

		d_first_coord = populator.get_first_coordinate();
		d_last_coord = populator.get_last_coordinate();

		lineedit_first->setText(d_first_coord);

		lineedit_last->setText(d_last_coord);
	
		// clean up
		delete tree_geometry;
	}
}


// ===========================================================================================
// 
// Functions called from Canvas Tool or Vieportwindow code 
//

void
GPlatesQtWidgets::BuildTopologyWidget::set_click_point(double lat, double lon)
{
	d_click_point_lat = lat;
	d_click_point_lon = lon;

	draw_click_point();
}


//
// reconstruction signals get sent here from a connect in ViewportWindow.cc
//
void
GPlatesQtWidgets::BuildTopologyWidget::handle_reconstruction_time_change(
		double new_time)
{
	if (! d_is_active) { return; }

	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	display_feature( 
		d_feature_focus_ptr->focused_feature(), d_feature_focus_ptr->associated_rfg() );

}

//
// focus_changed signals get sent here from a connect in ViewportWindow.cc
//
void
GPlatesQtWidgets::BuildTopologyWidget::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
	if (! d_is_active ) { return; }
qDebug() << "BuildTopologyWidget::set_focus():";

	if ( ! associated_rfg) {
qDebug() << "BuildTopologyWidget::set_focus(): no rfg";
	}

	// reset the index 
	if ( ! feature_ref.is_valid() )
	{
		d_section_feature_focus_index = -1;
	}

	// draw the focused geometry 
	draw_focused_geometry(); 


	// adjust widgets
	display_feature( 
		d_feature_focus_ptr->focused_feature(), d_feature_focus_ptr->associated_rfg() );

	show_numbers();
	
qDebug() << "BuildTopologyWidget::set_focus(): END";

	return;
}


//
void
GPlatesQtWidgets::BuildTopologyWidget::display_feature_focus_modified(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
qDebug() << "BuildTopologyWidget::display_feature_focus_modified:";
	display_feature( feature_ref, associated_rfg);
}

//
// Display the clicked feature data in the widgets and on the globe
//
void
GPlatesQtWidgets::BuildTopologyWidget::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
qDebug() << "BuildTopologyWidget::display_feature:";

	if (! d_is_active) { return; }

	// Clear the widget fields
	clear_widgets();

	// Set widget states
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
	checkbox_reverse->setEnabled(false);
	checkbox_reverse->setChecked(false);
	button_add_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);


		// clear the section table, and d_section_ vectors
		if (! d_in_edit )
		{
			// clear the Section table
			d_view_state_ptr->sections_feature_table_model().clear();
		
			// clear the vertex list
			d_topology_vertices.clear();
			d_tmp_index_vertex_list.clear();

			// clear the d_section_FOO vectors
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

			// Set the topology feature ref to NULL
			d_topology_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
			d_topology_feature_rfg = NULL;

			// unset the d_topology_geometry_opt_ptr
			d_topology_geometry_opt_ptr = boost::none;

			// clear the drawing layers
			draw_all_layers_clear();

			// draw the focus
			draw_focused_geometry();
		}


	//
	// Determine what to do with the focused feature
	//

	// always check your weak_refs!
	if ( ! feature_ref.is_valid() ) 
	{
qDebug() << "BuildTopologyWidget::display_feature: invalid ref";
		// do nothing with a null ref
		return;
	}

	// else, check what kind of feature it is:

	qDebug() << "d_feature_focus_ptr = " << GPlatesUtils::make_qstring_from_icu_string( d_feature_focus_ptr->focused_feature()->feature_id().get() );

	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name;

	if ( GPlatesFeatureVisitors::get_property_value(*feature_ref, name_property_name, name) )
	{
		qDebug() << "name = " << GPlatesUtils::make_qstring( name->value() );
	}

	if ( associated_rfg ) 
	{ 
		qDebug() << "associated_rfg = okay " ;
	} 
	else 
	{ 
		qDebug() << "associated_rfg = NULL " ; 
	}


	//
	// Check feature type via qstrings 
	//
 	QString topology_type_name ("TopologicalClosedPlateBoundary");
	QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		feature_ref->feature_type().get_name() );

	if ( feature_type_name == topology_type_name )
	{
		// a topology ref has been set ; don't display this topology feature 
		if ( d_topology_feature_ref.is_valid() ) {
			return; 
		}

		// d_section_ids is not empty ; don't display this topology feature
		if ( d_section_ids.size() != 0 ) {
			return;
		}

		// else the widget is ready to show an existing topology
		display_feature_topology( feature_ref, associated_rfg );

		return;
	} 
	else // non-topology feature type selected 
	{
		//
		// test if feature is already in the section vectors
		//

		// test feature id
		GPlatesModel::FeatureId test_id = feature_ref->feature_id();
qDebug() << "test_id = " << GPlatesUtils::make_qstring_from_icu_string( test_id.get() );

		// check if the feature is in the topology 
		int index = find_feature_in_topology( feature_ref );
		if ( index != -1 )
		{
			d_section_feature_focus_index = index;

			display_feature_on_boundary( feature_ref, associated_rfg );
qDebug() << "BuildTopologyWidget::display_feature: END";
			return;
		}

		// else, test_id not found on boundary 
		display_feature_off_boundary( feature_ref, associated_rfg );
		return;
	}
	return;
}

int 
GPlatesQtWidgets::BuildTopologyWidget::find_feature_in_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref )
{
qDebug() << "BuildTopologyWidget::find_feature_in_topology()";
	// test if feature is in the section ids vector
	GPlatesModel::FeatureId test_id = feature_ref->feature_id();
qDebug() << "test_id = " << GPlatesUtils::make_qstring_from_icu_string( test_id.get() );

	std::vector<GPlatesModel::FeatureId>::iterator section_itr = d_section_ids.begin();
	std::vector<GPlatesModel::FeatureId>::iterator section_end = d_section_ids.end();
	int index = 0;
	for ( ; section_itr != section_end; ++section_itr, ++index)
	{
		GPlatesModel::FeatureId	section_id = *section_itr;
qDebug() << "index = " << index << "; section_id = " << GPlatesUtils::make_qstring_from_icu_string( section_id.get() );
		if ( test_id == section_id)
		{
			return index;
		}
	}

	// feature id not found
	return -1;
}

//
// display the topology in the sections table and on the widget 
//
void
GPlatesQtWidgets::BuildTopologyWidget::display_feature_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
#if 0
qDebug() << "BuildTopologyWidget::display_feature_topology():";

	// Set the widget states
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
	checkbox_reverse->setEnabled(false);
	checkbox_reverse->setChecked(false);
	button_add_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(true);
	lineedit_num_sections->setEnabled(true);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);

	// Clear the d_section_ vectors
	d_section_ptrs.clear();
	d_section_ids.clear();
	d_section_click_points.clear();
	d_section_reverse_flags.clear();

	// Create a new TopologySectionsFinder to fill d_section_ vectors
	GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder( 
		d_section_ptrs, d_section_ids, d_section_click_points, d_section_reverse_flags);

	// Visit the feature_ref, filling d_section_ vectors with data
	feature_ref->accept_visitor( topo_sections_finder );

	// Get a pointer to the Topology Section Table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	// just to be safe, disconnect listening to feature focus while changing Section Table
	connect_to_focus_signals( false );

	// Clear the sections_table
	sections_table.clear();

	// Find this topology's ReconstructionGeometry data and insert into sections table
	std::vector<GPlatesModel::FeatureId>::iterator section_itr = d_section_ids.begin();
	std::vector<GPlatesModel::FeatureId>::iterator section_end = d_section_ids.end();
	//int index = 0;
	for ( ; section_itr != section_end; ++section_itr)
	{
		GPlatesModel::FeatureId	section_id = *section_itr;

		// try to find this section's feature id in the reconstruction's rg map
		GPlatesModel::Reconstruction::id_to_rfg_map_type::iterator find_iter, map_end;
		map_end = d_view_state_ptr->reconstruction().id_to_rfg_map()->end();
		find_iter = d_view_state_ptr->reconstruction().id_to_rfg_map()->find( section_id) ;

		if ( find_iter != map_end )
		{
			sections_table.sequence_about_to_be_changed();
			sections_table.begin_insert_features(0, 0);
			sections_table.geometry_sequence().push_back( find_iter->second );
			sections_table.end_insert_features();
			sections_table.sequence_changed();
		}
	}

	// reconnect listening to focus signals from Topology Sections table
	connect_to_focus_signals( true );

	// fill the widgets with feature data
	fill_widgets( feature_ref, associated_rfg );

	// Set the num_sections
	lineedit_num_sections->setText(QString::number( d_section_ids.size() ));

qDebug() << "BuildTopologyWidget::display_feature_topology() END";
#endif
}



//
// 
//
void
GPlatesQtWidgets::BuildTopologyWidget::display_feature_on_boundary(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
qDebug() << "-------------------------------------------------";
qDebug() << "BuildTopologyWidget::display_feature_on_boundary()";
qDebug() << "-------------------------------------------------";

	// always double check your weak_refs!
	if ( ! feature_ref.is_valid() ) {
qDebug() << "BuildTopologyWidget::display_feature_on_boundary() invalid ref";
		return;
	} 

	// Flip to the Topology Sections Table
	d_view_state_ptr->change_tab( 2 );

	// fill the widgets with feature data
	fill_widgets( feature_ref, associated_rfg );

	// set the the checkbox_reverse
	bool r = d_section_reverse_flags.at( d_section_feature_focus_index );

	// Set the widget states
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
	checkbox_reverse->setEnabled(true);
	checkbox_reverse->setChecked( r );
	button_add_feature->setEnabled(false);
	button_remove_feature->setEnabled(true);
	button_insert_before->setEnabled(true);
	button_insert_after->setEnabled(true);
	button_clear_feature->setEnabled(true);
	label_num_sections->setEnabled(true);
	lineedit_num_sections->setEnabled(true);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);


	// light 'em up!
	connect_to_focus_signals( false );
	d_view_state_ptr->highlight_sections_table_row( d_section_feature_focus_index, true );
	connect_to_focus_signals( true );

}


void
GPlatesQtWidgets::BuildTopologyWidget::display_feature_off_boundary(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
qDebug() << "-----------------------------------------------------";
qDebug() << "BuildTopologyWidget::display_feature_off_boundary()";
qDebug() << "-----------------------------------------------------";
	// always double check your weak_refs!
	if ( ! feature_ref.is_valid() ) {
		return;
	} 

	// Flip to the Topology Sections Table
	d_view_state_ptr->change_tab( 0 );

	// fill the widgets with feature data
	fill_widgets( feature_ref, associated_rfg );

	// Set the widget states
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
	checkbox_reverse->setEnabled(false);
	checkbox_reverse->setChecked(false);
	button_add_feature->setEnabled(true);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(true);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);
}


void
GPlatesQtWidgets::BuildTopologyWidget::handle_shift_left_click(
	const GPlatesMaths::PointOnSphere &click_pos_on_globe,
	const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
	bool is_on_globe)
{
	// Check if the focused feature is a topology
 	QString topology_type_name("TopologicalClosedPlateBoundary");
	QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		(d_feature_focus_ptr->focused_feature())->feature_type().get_name() );
	if ( feature_type_name == topology_type_name ) 
	{
		return;
	}

	// check if the focused feature is in the topology
	int index = find_feature_in_topology( d_feature_focus_ptr->focused_feature() );
	if ( index != -1 )
	{
		d_section_feature_focus_index = index;

		// remove the focused feature
		handle_remove_feature();
		return;
	}

	// else, add the focused feature 
	handle_add_feature();
	return;
}


// 
// Button Handlers  and support functions 
// 

void
GPlatesQtWidgets::BuildTopologyWidget::handle_reverse()
{
qDebug() << "BuildTopologyWidget::handle_reverse";

	// bool is_checked = checkbox_reverse->isChecked();

	// find 
	int index = find_feature_in_topology( d_feature_focus_ptr->focused_feature() );

	if (index == -1) 
	{ 
		return; 
	}

	// else, flip the flag
	d_section_reverse_flags.at(index) = ! d_section_reverse_flags.at(index);

	// adjust the widget 
	checkbox_reverse->setChecked( d_section_reverse_flags.at(index) ); 


#if 0
qDebug() << "BuildTopologyWidget::handle_reverse() call d_feature_focus_ptr->unset_focus();";
	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
#endif

#if 0
qDebug() << "BuildTopologyWidget::handle_reverse() call d_view_state_ptr->feature_table_model().clear()";
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_view_state_ptr->feature_table_model().clear();
#endif

qDebug() << "BuildTopologyWidget::handle_reverse() call update_geom";
	// set flags for visit from update_geom()
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

qDebug() << "BuildTopologyWidget::handle_reverse() END";

	// reset to default state
	checkbox_reverse->setChecked( false );
}

void
GPlatesQtWidgets::BuildTopologyWidget::handle_add_feature()
{
	// adjust the mode
	d_in_edit = true;

	// if this is the first feature on an empty boundary, 
	// then 'click' handle_new_topology button automatically to set other widgets
	if ( d_section_ids.size() == 0 ) {
		// handle_new_topology();
	}

	// set the insert index to add the feature to the end of the boundary
	int index = d_section_ids.size();

	// if this is an 'insert before' or 'insert after' operation, 
	// then reset the index to the correct spot 
	if ( d_insert_index != -1 )
	{
		index = d_insert_index; 
	}

	// insert the feature into the boundary
	handle_insert_feature( index );

	return;
}

void
GPlatesQtWidgets::BuildTopologyWidget::handle_insert_feature(int index)
{
qDebug() << "BuildTopologyWidget::handle_insert_feature()";
qDebug() << "index = " << index;

	// Flip to Topology Sections Table
	d_view_state_ptr->change_tab( 2 );

	// pointers to the Clicked Features table
	GPlatesGui::FeatureTableModel &clicked_table = 
		d_view_state_ptr->feature_table_model();

	// table index of clicked feature
	int click_index = clicked_table.current_index().row();

	// get the feature id 
	GPlatesModel::ReconstructionGeometry *rg_ptr = 
		( clicked_table.geometry_sequence().begin() + click_index )->get();
	GPlatesModel::ReconstructedFeatureGeometry *rfg_ptr =
		dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg_ptr);
	const GPlatesModel::FeatureId id = rfg_ptr->feature_handle_ptr()->feature_id();

	// insert the feature id
	d_section_ids.insert( d_section_ids.begin() + index, id );

	// insert the default reverse flag
	d_section_reverse_flags.insert( d_section_reverse_flags.begin() + index, false );

	// insert the current click_point
	d_section_click_points.insert( d_section_click_points.begin() + index,  
			std::make_pair( d_click_point_lat, d_click_point_lon ) );

	// reset the d_insert_ variables before the update call 
	d_insert_index = -1;
	d_insert_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	d_insert_feature_rfg = NULL;

qDebug() << "BuildTopologyWidget::handle_insert_feature() call d_feature_focus_ptr->unset_focus();";

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
qDebug() << "BuildTopologyWidget::handle_insert_feature() call d_view_state_ptr->feature_table_model().clear()";
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_view_state_ptr->feature_table_model().clear();


qDebug() << "BuildTopologyWidget::handle_insert_feature() call update_geom";
	// set flags for visit from update_geom()
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

qDebug() << "BuildTopologyWidget::handle_insert_feature() END";

	// reset the add button
	button_add_feature->setEnabled(false);
}


void
GPlatesQtWidgets::BuildTopologyWidget::handle_remove_feature()
{
qDebug() << "BuildTopologyWidget::handle_remove_feature()";

	int index = d_section_feature_focus_index;

qDebug() << "index = " << index;

	// remove elements from the d_section_ vectors 
	d_section_ids.erase( d_section_ids.begin() + index );
	d_section_ptrs.erase( d_section_ptrs.begin() + index );
	d_section_click_points.erase( d_section_click_points.begin() + index );
	d_section_reverse_flags.erase( d_section_reverse_flags.begin() + index );

	// clear out the widgets
	clear_widgets();

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();

	// Flip to Topology Sections Table
	d_view_state_ptr->change_tab( 2 );

	// process the sections vectors 
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

qDebug() << "BuildTopologyWidget::handle_remove_feature() END";
}

void
GPlatesQtWidgets::BuildTopologyWidget::handle_insert_after()
{
qDebug() << "BuildTopologyWidget::handle_insert_after()";

	// set the d_insert_feature_ref 
	d_insert_feature_ref = d_feature_focus_ptr->focused_feature();

	// find the index 
	d_insert_index = find_feature_in_topology( d_feature_focus_ptr->focused_feature() );
	d_insert_index += 1;

qDebug() << "d_insert_index = " << d_insert_index;

	// unset the focus 
	d_feature_focus_ptr->unset_focus(); // will call display_feature() with NULL ref
	d_feature_focus_head_points.clear(); 
	d_feature_focus_tail_points.clear();
}

void
GPlatesQtWidgets::BuildTopologyWidget::handle_insert_before()
{
qDebug() << "BuildTopologyWidget::handle_insert_before()";

	// set the d_insert_feature_ref 
	d_insert_feature_ref = d_feature_focus_ptr->focused_feature();

	// set the insert index
	d_insert_index = find_feature_in_topology( d_feature_focus_ptr->focused_feature() );

qDebug() << "d_insert_index = " << d_insert_index;

	// unset the focus 
	d_feature_focus_ptr->unset_focus(); // will call display_feature() with NULL ref
	d_feature_focus_head_points.clear(); 
	d_feature_focus_tail_points.clear(); 
}


void
GPlatesQtWidgets::BuildTopologyWidget::handle_clear()
{
qDebug() << "BuildTopologyWidget::handle_clear()";

	// clear the "Clicked" table
	d_view_state_ptr->feature_table_model().clear();

	// clear the widgets
	clear_widgets();

	// clear the focus data 
	d_focused_feature_layer_ptr->clear_rendered_geometries();

	// unset the focus 
	d_feature_focus_ptr->unset_focus(); // will call display_feature() with NULL ref
}


void
GPlatesQtWidgets::BuildTopologyWidget::handle_apply()
{
qDebug() << "BuildTopologyWidget::handle_apply()";

	// check for an empty section vectors
	if ( d_section_ids.empty() ) 
	{
		QMessageBox::warning(this, 
			tr("No boundary sections are defined for this feature"),
			tr("There are no valid boundray sections to use for creating this feature."),
			QMessageBox::Ok);
		return;
	}

	// do one final update ; make sure to create properties
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// no topology feature ref exisits, so fire up the feature creation dialog
	if ( ! d_topology_feature_ref.is_valid() )
	{
		// show the dialog
		bool success = d_create_feature_dialog->display();

		if ( ! success) {
			// The user cancelled the creation process. 
			// Return early and do not reset the widget.
			return;
		}

		// else, the feature was created by the dialog
		// and, append_boundary should have been called
		// from a signal/slot set up in ViewportWindow.cc

		d_feature_focus_ptr->unset_focus();

		// call handle cancel to reset widget and data
		handle_cancel(); 

		qDebug() << "BuildTopologyWidget::handle_apply() END";
		return;
	}

	// else, a d_topology_feature_ref exists, simple append the boundary
	append_boundary_to_feature( d_topology_feature_ref );

	//
	// Clear the widgets, tables, d_section_ vectors, derrived vectors, topology references
	//

	// Clear the widgets
	handle_clear();

	// clear the tables
	d_view_state_ptr->sections_feature_table_model().clear();
	d_view_state_ptr->feature_table_model().clear();

	// clear the vertex list
	d_topology_vertices.clear();
	d_tmp_index_vertex_list.clear();

	// clear the d_section_FOO vectors
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

	// Set the topology feature ref to NULL
	d_topology_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	d_topology_feature_rfg = NULL;

	// unset the d_topology_geometry_opt_ptr
	d_topology_geometry_opt_ptr = boost::none;

	// clear the drawing layers
	draw_all_layers_clear();

	// reset widget defaults
	initialise_geometry(PLATEPOLYGON);

	// unset the focus 
	d_feature_focus_ptr->unset_focus(); 

qDebug() << "BuildTopologyWidget::handle_apply() END";
}

void
GPlatesQtWidgets::BuildTopologyWidget::handle_cancel()
{
qDebug() << "BuildTopologyWidget::handle_cancel()";

	// adjust widget mode
	d_in_edit = false;

	// Set the widget states back to defaults
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
	checkbox_reverse->setEnabled(false);
	checkbox_reverse->setChecked(false);
	button_add_feature->setEnabled(false);
	button_remove_feature->setEnabled(false);
	button_insert_before->setEnabled(false);
	button_insert_after->setEnabled(false);
	button_clear_feature->setEnabled(false);
	label_num_sections->setEnabled(false);
	lineedit_num_sections->setEnabled(false);
	button_apply->setEnabled(true);
	button_cancel->setEnabled(true);

	//
	// Clear the widgets, tables, d_section_ vectors, derrived vectors, topology references
	//

	// Clear the widgets
	handle_clear();

	// clear the tables
	d_view_state_ptr->sections_feature_table_model().clear();
	d_view_state_ptr->feature_table_model().clear();

	// clear the vertex list
	d_topology_vertices.clear();
	d_tmp_index_vertex_list.clear();

	// clear the d_section_FOO vectors
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

	// Set the topology feature ref to NULL
	d_topology_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	d_topology_feature_rfg = NULL;

	// unset the d_topology_geometry_opt_ptr
	d_topology_geometry_opt_ptr = boost::none;

	// clear the drawing layers
	draw_all_layers_clear();

	// reset widget defaults
	initialise_geometry(PLATEPOLYGON);

	// unset the focus 
	d_feature_focus_ptr->unset_focus(); 

qDebug() << "BuildTopologyWidget::handle_cancel() END";
}


// ===========================================================================================
//
// Visitors for basic geometry types
//
void
GPlatesQtWidgets::BuildTopologyWidget::visit_multi_point_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{ 
	// set type only
	if (d_visit_to_check_type) {
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_point_on_sphere(): TYPE";
#endif
		d_tmp_feature_type = GPlatesGlobal::POINT_FEATURE;
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_point_on_sphere(): END_POINTS";
#endif
		// single points just go in head list
		d_feature_focus_head_points.push_back( *point_on_sphere );
		return;
	}

#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_point_on_sphere(): VERTS";
#endif
	// set the global flag for intersection processing 
	d_tmp_process_intersections = false;

	// simply append the point to the working list
	d_tmp_index_vertex_list.push_back( *point_on_sphere );


	// return early if properties are not needed
	if (! d_visit_to_create_properties) { return; }

#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_point_on_sphere(): PROPS";
#endif

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
GPlatesQtWidgets::BuildTopologyWidget::visit_polygon_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{  

	// set type only
	if (d_visit_to_check_type) {
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_polyline_on_sphere(): TYPE";
#endif
		d_tmp_feature_type = GPlatesGlobal::LINE_FEATURE;
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_polyline_on_sphere(): END_POINTS";
#endif
		d_feature_focus_head_points.push_back( *(polyline_on_sphere->vertex_begin()) );
		d_feature_focus_tail_points.push_back( *(--polyline_on_sphere->vertex_end()) );
		return;
	}

#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_polyline_on_sphere(): VERTS";
#endif

	// set the global flag for intersection processing 
	d_tmp_process_intersections = true;

	// Write out each point of the polyline.
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = 
		polyline_on_sphere->vertex_begin();

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = 
		polyline_on_sphere->vertex_end();

	// create a list of this polyline's vertices
	std::vector<GPlatesMaths::PointOnSphere> polyline_vertices;
	polyline_vertices.clear();
	
	for ( ; iter != end; ++iter) 
	{
		polyline_vertices.push_back( *iter );
	}

	// check for reverse flag
	if (d_tmp_index_use_reverse) 
	{
		d_tmp_index_vertex_list.insert( d_tmp_index_vertex_list.end(), 
			polyline_vertices.rbegin(), polyline_vertices.rend() );

		// set the head and tail end_points
		d_head_end_points.push_back( *(--polyline_on_sphere->vertex_end()) );
		d_tail_end_points.push_back( *(polyline_on_sphere->vertex_begin()) );
	}
	else 
	{
		d_tmp_index_vertex_list.insert( d_tmp_index_vertex_list.end(), 
			polyline_vertices.begin(), polyline_vertices.end() );

		// set the head and tail end_points
		d_head_end_points.push_back( *(polyline_on_sphere->vertex_begin()) );
		d_tail_end_points.push_back( *(--polyline_on_sphere->vertex_end()) );
	}

	// return early if properties are not needed
	if (! d_visit_to_create_properties) { return; }

#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::visit_polyline_on_sphere(): PROPS";
#endif

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
// draw_ functions
//
void
GPlatesQtWidgets::BuildTopologyWidget::draw_all_layers_clear()
{
qDebug() << "BuildTopologyWidget::draw_all_layers_clear()";

	// clear all layers
	d_topology_geometry_layer_ptr->clear_rendered_geometries();
	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_segments_layer_ptr->clear_rendered_geometries();
	d_end_points_layer_ptr->clear_rendered_geometries();
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_click_points_layer_ptr->clear_rendered_geometries();

	d_view_state_ptr->globe_canvas().update_canvas();
}

void
GPlatesQtWidgets::BuildTopologyWidget::draw_all_layers()
{
qDebug() << "BuildTopologyWidget::draw_all_layers()";
	// draw all the layers
	draw_topology_geometry();
	draw_focused_geometry();
	draw_segments();
	draw_end_points();
	draw_intersection_points();
	draw_click_points();

	d_view_state_ptr->globe_canvas().update_canvas();
}


void
GPlatesQtWidgets::BuildTopologyWidget::draw_topology_geometry()
{
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::draw_topology_geometry()";
#endif
	d_topology_geometry_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	if (d_topology_geometry_opt_ptr) 
	{
		// light grey
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::Colour(0.75, 0.75, 0.75, 1.0);

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::create_rendered_geometry_on_sphere(
				*d_topology_geometry_opt_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

		d_topology_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	// update the canvas 
	d_view_state_ptr->globe_canvas().update_canvas();
}


void
GPlatesQtWidgets::BuildTopologyWidget::draw_focused_geometry()
{
qDebug() << "BuildTopologyWidget::draw_focused_geometry()";

	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_view_state_ptr->globe_canvas().update_canvas();

	if ( d_feature_focus_ptr->associated_rfg() )
	{
qDebug() << "BuildTopologyWidget::draw_focused_geometry() RFG okay";

		// Delay any notification of changes to the rendered geometry collection
		GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::create_rendered_geometry_on_sphere(
				d_feature_focus_ptr->associated_rfg()->geometry(),
				colour,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

		d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);

		// visit to get end_points
		d_feature_focus_head_points.clear();
		d_feature_focus_tail_points.clear();
		d_visit_to_get_focus_end_points = true;
		d_feature_focus_ptr->associated_rfg()->geometry()->accept_visitor(*this);
		d_visit_to_get_focus_end_points = false;

		// draw the focused end_points
		draw_focused_geometry_end_points();
	}


	//
	// if an insert spot has been selected, draw that feature in black
	//
	if ( d_insert_feature_ref.is_valid() )
	{
qDebug() << "BuildTopologyWidget::draw_focused_geometry() d_insert_feature_ref.is_valid()";

		// access the current RFG for this feature 
		GPlatesModel::Reconstruction *recon_ptr;
		recon_ptr = &( d_view_state_ptr->reconstruction() );

		GPlatesModel::ReconstructedFeatureGeometryFinder finder(
			&( d_view_state_ptr->reconstruction() ) );

		finder.find_rfgs_of_feature( d_insert_feature_ref );

		GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator 
			find_iter = finder.found_rfgs_begin();

		// get the geometry on sphere from the RFG
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type gos_ptr = 
			(*find_iter)->geometry();	

		if (gos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

			GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
					gos_ptr,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

			d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);

			// visit to get end_points
			d_feature_focus_head_points.clear();
			d_feature_focus_tail_points.clear();

			d_visit_to_get_focus_end_points = true;
			gos_ptr->accept_visitor(*this);
			d_visit_to_get_focus_end_points = false;

			// draw the focused end_points
			draw_focused_geometry_end_points();

			d_view_state_ptr->globe_canvas().update_canvas();
		}
	}
qDebug() << "BuildTopologyWidget::draw_focused_geometry() END";
}


void
GPlatesQtWidgets::BuildTopologyWidget::draw_focused_geometry_end_points()
{
qDebug() << "BuildTopologyWidget::draw_focused_geometry_end_points()";

	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;

	// draw head points
	itr = d_feature_focus_head_points.begin();
	end = d_feature_focus_head_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom_on_sphere_ptr = 
			itr->clone_as_geometry();

		if (geom_on_sphere_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
					geom_on_sphere_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFUALT_LINE_WIDTH_HINT);

			// Add to layer.
			d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// draw tail end_points
	itr = d_feature_focus_tail_points.begin();
	end = d_feature_focus_tail_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::draw_segments()
{
qDebug() << "BuildTopologyWidget::draw_segments()";

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
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_grey();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::draw_end_points()
{
qDebug() << "BuildTopologyWidget::draw_end_points()";

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
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::draw_intersection_points()
{
qDebug() << "BuildTopologyWidget::draw_intersection_points()";

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
		// GPlatesMaths::LatLonPoint( itr->first, itr->second ) );

#ifdef DEBUG1
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*itr);
qDebug() << "draw_intersection_points d_intersection_points: llp=" << llp.latitude() << "," << llp.longitude();
#endif

		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_grey();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::draw_click_point()
{
qDebug() << "BuildTopologyWidget::draw_click_point()";

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
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_olive();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
GPlatesQtWidgets::BuildTopologyWidget::draw_click_points()
{
qDebug() << "BuildTopologyWidget::draw_click_points()";

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
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_olive();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
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
//
// Updater function for processing d_section_ vectors into geom and boundary prop
//

void
GPlatesQtWidgets::BuildTopologyWidget::update_geometry()
{
qDebug() << "BuildTopologyWidget::update_geometry()";

	// clear layers
	draw_all_layers_clear();

	// new d_section_ptrs will be created by create_sections_from_sections_table
	d_section_ptrs.clear();

	// these will be filled by create_sections_from_sections_table()
	d_topology_vertices.clear();
	d_head_end_points.clear();
	d_tail_end_points.clear();
	d_intersection_points.clear();
	d_segments.clear();
	d_insert_segments.clear();

	// FIXME: do we need these here?
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();

	// loop over Section Vectors to fill section table
	fill_section_table_from_section_ids();

	// loop over Sections Table to fill d_topology_vertices
	create_sections_from_sections_table();

	// Set the num_sections widget
	lineedit_num_sections->setText(QString::number( d_section_ptrs.size() ));

	// create the temp geom.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	// Set the d_topology_geometry_opt_ptr to the newly created geom;
	create_geometry_from_vertex_list( d_topology_vertices, d_geometry_type, validity);

	draw_all_layers();

	return;
}


///
/// 
///
void
GPlatesQtWidgets::BuildTopologyWidget::fill_section_table_from_section_ids()
{
qDebug() << "BuildTopologyWidget::fill_section_table_from_section_ids()";

	// Pointer to the Topology Section Table
	GPlatesGui::FeatureTableModel &sections_table = 
		d_view_state_ptr->sections_feature_table_model();

	// just to be safe, turn off connection to feature focus while changing Section Table
	connect_to_focus_signals( false );

	// Clear the old data
qDebug() << "BuildTopologyWidget::fill_section_table_from_section_ids() sections_table.clear";
	sections_table.clear();

	//
	// Find this feature's section's ReconstructionGeometry data to insert into sections table
	//
	int first = 0;
	int last = 0;
	std::vector<GPlatesModel::FeatureId>::iterator section_itr = d_section_ids.begin();
	std::vector<GPlatesModel::FeatureId>::iterator section_end = d_section_ids.end();

	for ( ; section_itr != section_end; ++section_itr)
	{
qDebug() << "first = " << first << "; last = " << last;

		GPlatesModel::FeatureId	section_id = *section_itr;

#if 0
		// try to find this section's feature id in the reconstruction's rg map
		GPlatesModel::Reconstruction::id_to_rfg_map_type::iterator find_iter, map_end;
		map_end = d_view_state_ptr->reconstruction().id_to_rfg_map()->end();
		find_iter = d_view_state_ptr->reconstruction().id_to_rfg_map()->find( section_id) ;
		
		if ( find_iter != map_end )
		{
			sections_table.sequence_about_to_be_changed();
			sections_table.begin_insert_features(first, last);
			sections_table.geometry_sequence().push_back( find_iter->second );
			sections_table.end_insert_features();
			sections_table.sequence_changed();
			first += 1;
			last += 1;
		}
#endif

	}

	// reconnect listening to focus signals from Topology Sections table
	connect_to_focus_signals( true );

	return;
}

///
/// Loop over the Topology Section entries and fill the working lists
///
void
GPlatesQtWidgets::BuildTopologyWidget::create_sections_from_sections_table()
{
qDebug() << "BuildTopologyWidget::create_sections_from_sections_table()";

	// clear out the old vectors, since the calls to accept_visitor will re-populate them
	d_section_ptrs.clear();
	d_topology_vertices.clear();

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
		d_tmp_index_fid = rfg->feature_handle_ptr()->feature_id();

		// set the tmp reverse flag to this feature's flag
		d_tmp_index_use_reverse = d_section_reverse_flags.at(d_tmp_index);

#ifdef DEBUG1
qDebug() << "d_tmp_index = " << d_tmp_index;

static const GPlatesModel::PropertyName name_property_name =
	GPlatesModel::PropertyName::create_gml("name");
const GPlatesPropertyValues::XsString *name;
if ( GPlatesFeatureVisitors::get_property_value(
	*(rfg->feature_handle_ptr()), name_property_name, name) )
{
	qDebug() << "name = " << GPlatesUtils::make_qstring( name->value() );
}
qDebug() << "d_use_rev = " << d_tmp_index_use_reverse;
#endif

		// clear the tmp index list
		d_tmp_index_vertex_list.clear();

		// visit the geoms.  :
		// fill additional d_tmp_index_ vars 
		// fill d_head_end_points d_tail_end_points 
		// set d_tmp_process_intersections
		d_visit_to_check_type = false;
		d_visit_to_create_properties = true;
		(*iter)->geometry()->accept_visitor(*this);

		// short-cut for single item boundary
		if ( d_tmp_sections_size == 1 ) {
			d_tmp_process_intersections = false;
		}

		//
		// Check for intersection
		//
		if ( d_tmp_process_intersections )
		{
			process_intersections();

			// d_tmp_index_vertex_list may have been modified by process_intersections()
			d_topology_vertices.insert( d_topology_vertices.end(), 
				d_tmp_index_vertex_list.begin(), d_tmp_index_vertex_list.end() );

			// save this segment make a polyline
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type pos_ptr = 
				GPlatesMaths::PolylineOnSphere::create_on_heap( d_tmp_index_vertex_list );
			d_segments.push_back( pos_ptr );
		}
		else
		{
			// simply insert tmp items on the list
			d_topology_vertices.insert( d_topology_vertices.end(), 
				d_tmp_index_vertex_list.begin(), d_tmp_index_vertex_list.end() );
		}

		// update counter d_tmp_index
		++d_tmp_index;
	}
}

void
GPlatesQtWidgets::BuildTopologyWidget::process_intersections()
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
qDebug() << "BuildTopologyWidget::process_intersections: "
<< "d_click_point_lat = " << d_click_point_lat << "; " 
<< "d_click_point_lon = " << d_click_point_lon << "; ";
qDebug() << "BuildTopologyWidget::process_intersections: "
<< "d_prev_index = " << d_tmp_prev_index << "; " 
<< "d_tmp_index = " << d_tmp_index << "; " 
<< "d_next_index = " << d_tmp_next_index << "; ";
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
		GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_PREV);

	// if they do, then create the startIntersection property value
	if ( d_visit_to_create_properties && (d_num_intersections_with_prev != 0) )
	{
		// if there was an intersection, create a startIntersection property value
		GPlatesModel::ReconstructionGeometry *prev_rg = prev->get();

		GPlatesModel::ReconstructedFeatureGeometry *prev_rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(prev_rg);

		// intersection_geometry
		const GPlatesModel::FeatureId prev_fid = prev_rfg->feature_handle_ptr()->feature_id();

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
		// this sets the reference point's recond plate = the intersection geom's plate id
		const GPlatesModel::FeatureId index_fid( prev_fid );

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
		GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_NEXT);

	// if they do, then create the endIntersection property value
	if ( d_visit_to_create_properties && (d_num_intersections_with_next != 0) )
	{
		// if there was an intersection, create a endIntersection property value
		GPlatesModel::ReconstructionGeometry *next_rg = next->get();

		GPlatesModel::ReconstructedFeatureGeometry *next_rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(next_rg);

		// intersection_geometry
		const GPlatesModel::FeatureId next_fid = next_rfg->feature_handle_ptr()->feature_id();

		const GPlatesModel::PropertyName prop_name1 =
			GPlatesModel::PropertyName::create_gpml("centerLineOf");

		const GPlatesPropertyValues::TemplateTypeParameterType value_type1 =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString" );

		// create the intersectionGeometry property delegate
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
		// this sets the reference point's recond plate = the intersection geom's plate id
		const GPlatesModel::FeatureId index_fid( next_fid );

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
GPlatesQtWidgets::BuildTopologyWidget::compute_intersection(
	const GPlatesMaths::PolylineOnSphere* node1_polyline,
	const GPlatesMaths::PolylineOnSphere* node2_polyline,
	GPlatesQtWidgets::BuildTopologyWidget::NeighborRelation relation)
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
qDebug() << "BuildTopologyWidget::compute_intersection: " << "num_intersect = " << num_intersect;
#endif


	// switch on relation enum to set node1's member data
	switch ( relation )
	{
		case GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_PREV :
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: INTERSECT_PREV: ";
#endif
			d_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_NEXT:
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: INTERSECT_NEXT: ";
#endif
			d_num_intersections_with_next = num_intersect;
			break;

		case GPlatesQtWidgets::BuildTopologyWidget::NONE :
		case GPlatesQtWidgets::BuildTopologyWidget::OTHER :
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
qDebug() << "BuildTopologyWidget::compute_intersection:: HEAD: verts:";
for ( ; iter != end; ++iter) {
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
qDebug() << "llp=" << llp.latitude() << "," << llp.longitude();
}

iter = parts.first.back()->vertex_begin();
end = parts.first.back()->vertex_end();
qDebug() << "BuildTopologyWidget::compute_intersection:: TAIL: verts:";
for ( ; iter != end; ++iter) {
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
qDebug() << "llp=" << llp.latitude() << "," << llp.longitude();
}
#endif

		// now check which element of parts.first
		// is closest to click_point:

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
			qDebug() << "BuildTopologyWidget::compute_intersection: "
			<< "WARN: click point not close to anything!"
			<< "WARN: Unable to set boundary feature intersection flags!";
			return;
		}

#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: "
<< "closeness_head=" << closeness_head.dval() << " and " 
<< "closeness_tail=" << closeness_tail.dval();
GPlatesMaths::LatLonPoint llp_pff = GPlatesMaths::make_lat_lon_point( *(parts.first.front()->vertex_begin()) );
GPlatesMaths::LatLonPoint llp_pfb = GPlatesMaths::make_lat_lon_point( *(parts.first.back()->vertex_begin()) );
#endif

		// now compare the closeness values to set relation
		if ( closeness_head > closeness_tail )
		{
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: " << "use HEAD";
#endif
			d_closeness = closeness_head;

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_PREV :
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
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: llp_pff =" << llp_pff.latitude() << "," << llp_pff.longitude();
#endif
					break;
	
				case GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_NEXT:
					//d_use_head_from_intersect_next = true;
					//d_use_tail_from_intersect_next = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.front()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: llp_pff =" << llp_pff.latitude() << "," << llp_pff.longitude();
#endif
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
qDebug() << "BuildTopologyWidget::compute_intersection: " << "use TAIL" ;
#endif

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_PREV :
					//d_use_tail_from_intersect_prev = true;
					//d_use_head_from_intersect_prev = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.back()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: llp=" << llp_pfb.latitude() << "," << llp_pfb.longitude();
#endif

					break;
	
				case GPlatesQtWidgets::BuildTopologyWidget::INTERSECT_NEXT:
					//d_use_tail_from_intersect_next = true;
					//d_use_head_from_intersect_next = false;
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.back()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "BuildTopologyWidget::compute_intersection: llp=" << llp_pfb.latitude() << "," << llp_pfb.longitude();
#endif
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
		qDebug() << "BuildTopologyWidget::compute_intersection: "
		<< "WARN: num_intersect=" << num_intersect
		<< "WARN: Unable to set boundary feature intersection relations!"
		<< "WARN: Make sure boundary feature's only intersect once.";
	}

}


void
GPlatesQtWidgets::BuildTopologyWidget::append_boundary_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{

qDebug() << "BuildTopologyWidget::append_boundary_value_to_feature() feature_ref = " 
<< GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );

static const GPlatesModel::PropertyName name_property_name =
	GPlatesModel::PropertyName::create_gml("name");

const GPlatesPropertyValues::XsString *name;

if ( GPlatesFeatureVisitors::get_property_value( *feature_ref, name_property_name, name) )
{
qDebug() << "name = " << GPlatesUtils::make_qstring( name->value() );
}


	// do an update ; create properties this time
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;

	// process the d_section_ vectors into the sections table 
	// process the Sections Table into d_section_ptrs
	update_geometry(); 

	// find the old prop to remove
	GPlatesModel::PropertyName boundary_prop_name = 
		GPlatesModel::PropertyName::create_gpml("boundary");

	GPlatesModel::FeatureHandle::properties_iterator iter = feature_ref->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_ref->properties_end();
	// loop over properties
	for ( ; iter != end; ++iter) 
	{
		// double check for validity and nullness
		if(! iter.is_valid() ){ continue; }
		if( *iter == NULL )   { continue; }  
		// FIXME: previous edits to the feature leave property pointers NULL

		// passed all checks, make the name and test
		GPlatesModel::PropertyName test_name = (*iter)->property_name();

qDebug() << "name = " << GPlatesUtils::make_qstring_from_icu_string(test_name.get_name());

		if ( test_name == boundary_prop_name )
		{
qDebug() << "call remove_property_container on = " << GPlatesUtils::make_qstring_from_icu_string(test_name.get_name());
			// Delete the old boundary 
			GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
			// FIXME: WAS :
			//feature_ref->remove_property_container(iter, transaction);
			// NOW:
			feature_ref->remove_top_level_property(iter, transaction);
			transaction.commit();
			// FIXME: this seems to create NULL pointers in the properties collection
			// see FIXME note above to check for NULL? 
			// Or is this to be expected?

			// FIXME: do this?
			// d_feature_focus_ptr->announce_modification_of_focused_feature();
			// only if the focus is the topo feature 

			break;
		}
	} // loop over properties

	// create the TopologicalPolygon
	GPlatesModel::PropertyValue::non_null_ptr_type topo_poly_value =
		GPlatesPropertyValues::GpmlTopologicalPolygon::create(d_section_ptrs);

	const GPlatesPropertyValues::TemplateTypeParameterType topo_poly_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");

	// Create the ConstantValue
	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
		GPlatesPropertyValues::GpmlConstantValue::create(topo_poly_value, topo_poly_type);

	// Get the time period for the feature's validTime prop
	// FIXME: (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	const GPlatesPropertyValues::GmlTimePeriod *time_period;

	GPlatesFeatureVisitors::get_property_value( 
		*feature_ref, valid_time_property_name, time_period);

	// Casting time details
	GPlatesPropertyValues::GmlTimePeriod* tp = 
	const_cast<GPlatesPropertyValues::GmlTimePeriod *>( time_period );

	GPlatesUtils::non_null_intrusive_ptr<
		GPlatesPropertyValues::GmlTimePeriod, 
		GPlatesUtils::NullIntrusivePointerHandler> ttpp(
			tp,
			GPlatesUtils::NullIntrusivePointerHandler()
		);

	// Create the TimeWindow
	GPlatesPropertyValues::GpmlTimeWindow tw = GPlatesPropertyValues::GpmlTimeWindow(
			constant_value, 
			ttpp,
			topo_poly_type);

	// Use the time window
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;

	time_windows.push_back(tw);

	// Create the PiecewiseAggregation
	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type aggregation =
		GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, topo_poly_type);
	
	// Add a gpml:boundary Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
		aggregation,
		GPlatesModel::PropertyName::create_gpml("boundary"),
		feature_ref);

	// Set the ball rolling again ...
	d_view_state_ptr->reconstruct(); 

qDebug() << "BuildTopologyWidget::append_boundary_value_to_feature() END";
}


void
GPlatesQtWidgets::BuildTopologyWidget::fill_section_vectors_from_feature_ref(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
qDebug() << "BuildTopologyWidget::fill_sections_section_vectors_from_feature_ref()";
	if ( ! feature_ref.is_valid() ) 
	{
		return;
	}

	// Clear the working lists
	d_section_ptrs.clear();
	d_section_ids.clear();
	d_section_click_points.clear();
	d_section_reverse_flags.clear();

	// NOTE: call to update_geometry(); updates the other working lists

	// Create a new TopologySectionsFinder to fill d_section_ vectors
	GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder( 
		d_section_ptrs, d_section_ids, d_section_click_points, d_section_reverse_flags);

	// Visit the topology_feature ref, filling d_section_ vectors with data
	feature_ref->accept_visitor( topo_sections_finder );
}



void
GPlatesQtWidgets::BuildTopologyWidget::show_numbers()
{
	qDebug() << "############################################################"; 
	qDebug() << "show_numbers: "; 
	qDebug() << "d_section_ptrs.size()         = " << d_section_ptrs.size(); 
	qDebug() << "d_section_ids.size()          = " << d_section_ids.size(); 
	qDebug() << "d_section_click_points.size() = " << d_section_click_points.size();
	qDebug() << "d_section_reverse_flags.size()= " << d_section_reverse_flags.size();
	qDebug() << "d_topology_vertices.size()    = " << d_topology_vertices.size(); 
	qDebug() << "d_tmp_index_vertex_list.size()= " << d_tmp_index_vertex_list.size();
	qDebug() << "d_head_end_points.size()      = " << d_head_end_points.size(); 
	qDebug() << "d_tail_end_points.size()      = " << d_tail_end_points.size(); 
	qDebug() << "d_intersection_points.size()  = " << d_intersection_points.size(); 
	qDebug() << "d_segments.size()             = " << d_segments.size(); 
	qDebug() << "d_insert_segments.size()      = " << d_insert_segments.size(); 
	qDebug() << "d_feature_focus_head_points.size()= " << d_feature_focus_head_points.size();
	qDebug() << "d_feature_focus_tail_points.size()= " << d_feature_focus_tail_points.size();

	//
	// Show details about d_feature_focus_ptr
	//
	if ( d_feature_focus_ptr->is_valid() ) 
	{
		qDebug() << "d_feature_focus_ptr = " << GPlatesUtils::make_qstring_from_icu_string( 
			d_feature_focus_ptr->focused_feature()->feature_id().get() );

		static const GPlatesModel::PropertyName name_property_name = 
			GPlatesModel::PropertyName::create_gml("name");

		const GPlatesPropertyValues::XsString *name;
		if ( GPlatesFeatureVisitors::get_property_value(
			*d_feature_focus_ptr->focused_feature(), name_property_name, name) )
		{
			qDebug() << "d_feature_focus_ptr name = " << GPlatesUtils::make_qstring(name->value());
		}
		else 
		{
			qDebug() << "d_feature_focus_ptr = INVALID";
		}
	}

	qDebug() << "d_section_feature_focus_index = " << d_section_feature_focus_index;

	//
	// show details about d_topology_feature_ref
	//
	if ( d_topology_feature_ref.is_valid() ) 
	{
		qDebug() << "d_topology_feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( d_topology_feature_ref->feature_id().get() );
		static const GPlatesModel::PropertyName name_property_name = 
			GPlatesModel::PropertyName::create_gml("name");

		const GPlatesPropertyValues::XsString *name;

		if ( GPlatesFeatureVisitors::get_property_value(
			*d_topology_feature_ref, name_property_name, name) )
		{
			qDebug() << "d_topology_feature_ref name = " << GPlatesUtils::make_qstring( name->value() );
		} 
		else 
		{
			qDebug() << "d_topology_feature_ref = INVALID";
		}
	}


	//
	// show sections details
	//
	std::vector<GPlatesModel::FeatureId>::iterator section_itr = d_section_ids.begin();
	std::vector<GPlatesModel::FeatureId>::iterator section_end = d_section_ids.end();
	int index = 0;
	for ( ; section_itr != section_end; ++section_itr, ++index)
	{
		GPlatesModel::FeatureId	section_id = *section_itr;
		qDebug() << "index = " << index 
			<< "; section_id = " 
			<< GPlatesUtils::make_qstring_from_icu_string( section_id.get() );
	}


	// 
	// show d_insert_feature_ref
	//
	if ( d_insert_feature_ref.is_valid() ) 
	{
		qDebug() << "d_insert_feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( d_insert_feature_ref->feature_id().get() );

		static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

		const GPlatesPropertyValues::XsString *name;

		if ( GPlatesFeatureVisitors::get_property_value(
			*d_insert_feature_ref, name_property_name, name) )
		{
			qDebug() << "name = " << GPlatesUtils::make_qstring( name->value() );
		} 
		else 
		{
			qDebug() << "d_insert_feature_ref = INVALID";
		}
	}

	qDebug() << "############################################################"; 

}

void
GPlatesQtWidgets::BuildTopologyWidget::create_geometry_from_vertex_list(
	std::vector<GPlatesMaths::PointOnSphere> &points,
	GPlatesQtWidgets::BuildTopologyWidget::GeometryType target_geom_type,
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
{
	// FIXME: Only handles the unbroken line and single-ring cases.

	// There's no guarantee that adjacent points in the table aren't identical.
	std::vector<GPlatesMaths::PointOnSphere>::size_type num_points =
			GPlatesMaths::count_distinct_adjacent_points(points);

#ifdef DEBUG1
qDebug() << "create_geometry_from_vertex_list: size =" << num_points;
std::vector<GPlatesMaths::PointOnSphere>::iterator itr;
for ( itr = points.begin() ; itr != points.end(); ++itr)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*itr);
	qDebug() << "create_geometry_from_vertex_list: llp=" << llp.latitude() << "," << llp.longitude();
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
			d_topology_geometry_opt_ptr = boost::none;

		case GPlatesQtWidgets::BuildTopologyWidget::PLATEPOLYGON:
			if (num_points == 0) 
			{
				validity = GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
				d_topology_geometry_opt_ptr = boost::none;
			} 
			else if (num_points == 1) 
			{
				d_topology_geometry_opt_ptr = 
					GPlatesUtils::create_point_on_sphere(points, validity);
			} 
			else if (num_points == 2) 
			{
				d_topology_geometry_opt_ptr = 
					GPlatesUtils::create_polyline_on_sphere(points, validity);
			} 
			else if (num_points == 3 && points.front() == points.back()) 
			{
				d_topology_geometry_opt_ptr = 
					GPlatesUtils::create_polyline_on_sphere(points, validity);
			} 
			else 
			{
				d_topology_geometry_opt_ptr = 
					GPlatesUtils::create_polygon_on_sphere(points, validity);
			}
			break;
		}
		// Should never reach here.
	} catch (...) {
		throw;
	}
	d_topology_geometry_opt_ptr = boost::none;
}

//
// END
//
