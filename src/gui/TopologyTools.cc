/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2008 , 2009 California Institute of Technology 
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

//#define DEBUG
//#define DEBUG1

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
#include <QMessageBox>

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "TopologyTools.h"

#include "FeatureFocus.h"
#include "ProximityTests.h"

#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/TopologySectionsFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

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

#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ModelUtils.h"
#include "model/ReconstructedFeatureGeometryFinder.h"
#include "model/ReconstructionGeometry.h"
#include "model/DummyTransactionHandle.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"

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

#include "qt-widgets/CreateFeatureDialog.h"
#include "qt-widgets/ExportCoordinatesDialog.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "presentation/ViewState.h"


// 
// Constructor
// 
GPlatesGui::TopologyTools::TopologyTools(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window):
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_feature_focus_ptr(&view_state.get_feature_focus()),
	d_reconstruct_ptr(&view_state.get_reconstruct()),
	d_viewport_window_ptr(&viewport_window)
{
	// set up the drawing
	create_child_rendered_layers();

	// Set pointer to TopologySectionsContainer
	d_topology_sections_container_ptr = &( d_viewport_window_ptr->topology_sections_container() );


	// set the internal state flags
	d_is_active = false;
	d_in_edit = false;

	d_visit_to_check_type = false;
	d_visit_to_create_properties = false;
	d_visit_to_get_focus_end_points = false;
}

void
GPlatesGui::TopologyTools::activate( CanvasToolMode mode )
{
	// Set the mode 
	d_mode = mode;

	// NOTE: should this be in the constructor ?
	// Set the pointer to the Topology Tools Widget 
	d_topology_tools_widget_ptr = 
		&( d_viewport_window_ptr->task_panel_ptr()->topology_tools_widget() );


	if ( d_mode == BUILD )
	{
		activate_build_mode();
	}
	else 
	{
		activate_edit_mode();
	}

	// process the table
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// Draw the topology
	draw_topology_geometry();


	// 
	// Report errors
	//
	if ( ! d_warning.isEmpty() )
	{
		// Add some helpful hints:
		d_warning.append(tr("\n"));
		d_warning.append(tr("Please check the Topology Sections table:\n"));
		d_warning.append(tr("\n"));
		d_warning.append(tr("a red row indicates either:\n"));
		d_warning.append(tr("- the feature is missing from the loaded data\n"));
		d_warning.append(tr("- the feature is loaded more than once\n"));
		d_warning.append(tr("\n"));
		d_warning.append(tr("a yellow row indicates either:\n"));
		d_warning.append(tr("- the feature has a missing geometry property\n"));
		d_warning.append(tr("- the feature is being used outside its lifetime\n"));
		d_warning.append(tr("\n"));

		QMessageBox::warning(
			d_topology_tools_widget_ptr, 
			tr("Error Building Topology"), 
			d_warning,
			QMessageBox::Ok, 
			QMessageBox::Ok);
	}

	// Connect to focus signals from Feature Focus
	connect_to_focus_signals( true );

	// Connect to signals from Topology Sections Container
	connect_to_topology_sections_container_signals( true );

	// Connect to recon time changes
	QObject::connect(
		d_reconstruct_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::Reconstruct &, bool, bool)),
		this,
		SLOT(handle_reconstruction()));

	// set the widget state
	d_is_active = true;

	d_viewport_window_ptr->status_message(QObject::tr(
 		"Click on a features to Add or Remove them from the Topology."
 		" Ctrl+drag to reorient the globe."));

	return;
}

void
GPlatesGui::TopologyTools::activate_build_mode()
{
	// place holder if we need it in the future ...
}

void
GPlatesGui::TopologyTools::activate_edit_mode()
{
		// set the topology feature ref
		d_topology_feature_ref = d_feature_focus_ptr->focused_feature();

		// Load the topology into the Topology Sections Table 
 		display_topology(
			d_feature_focus_ptr->focused_feature(), d_feature_focus_ptr->associated_reconstruction_geometry() );

		// Load the topology into the Topology Widget
		d_topology_tools_widget_ptr->display_topology(
			d_feature_focus_ptr->focused_feature(), d_feature_focus_ptr->associated_reconstruction_geometry() );

		// NOTE: this will NOT trigger a set_focus signal with NULL ref ; 
		// NOTE: the focus connection is below 
		d_feature_focus_ptr->unset_focus();
		d_feature_focus_head_points.clear();
		d_feature_focus_tail_points.clear();
		// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
		d_viewport_window_ptr->feature_table_model().clear();

		// Flip the ViewportWindow to the Topology Sections Table
		d_viewport_window_ptr->choose_topology_sections_table();

		// Flip the TopologyToolsWidget to the Toplogy Tab
		d_topology_tools_widget_ptr->choose_topology_tab();

		// check 

		// report 

}

void
GPlatesGui::TopologyTools::deactivate()
{
	// reset internal state
	d_is_active = false;

	// Unset any focused feature
	if ( d_feature_focus_ptr->is_valid() )
	{
		d_feature_focus_ptr->unset_focus();
	}

	// clear all the drawing layers
	draw_all_layers_clear();

	// Flip the ViewportWindow to the Clicked Geometry Table
	d_viewport_window_ptr->choose_clicked_geometry_table();

	// Connect to focus signals from Feature Focus
	connect_to_focus_signals( false );

	// Connect to signals from Topology Sections Container
	connect_to_topology_sections_container_signals( false );

	// connect to recon time changes
	QObject::disconnect(
		d_reconstruct_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::Reconstruct &, bool, bool)),
		this,
		SLOT(handle_reconstruction()));

	// Clear out all old data
	clear_widgets_and_data();
}

//
// Clear the widgets, tables, d_section_ vectors, derrived vectors, topology references
//
void
GPlatesGui::TopologyTools::clear_widgets_and_data()
{
	// clear the tables
	d_viewport_window_ptr->feature_table_model().clear();

	// loop over TopologySectionsTable 
	int i = d_topology_sections_container_ptr->size(); 
	--i; // decrement to last index
	for ( ; i > -1 ; --i )
	{
		d_topology_sections_container_ptr->remove_at( i );
	}
	
	// clear the vertex list
	d_topology_vertices.clear();
	d_section_ranges_into_topology_vertices.clear();
	d_tmp_index_vertex_list.clear();

	// clear the working lists
	d_head_end_points.clear();
	d_tail_end_points.clear();
	d_intersection_points.clear();
	d_segments.clear();

	// Set the topology feature ref to NULL
	d_topology_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
	d_topology_feature_rfg = NULL;

	// unset the d_topology_geometry_opt_ptr
	d_topology_geometry_opt_ptr = boost::none;

	// clear the drawing layers
	draw_all_layers_clear();

	// unset the focus 
	d_feature_focus_ptr->unset_focus(); 
}
	

void
GPlatesGui::TopologyTools::connect_to_focus_signals(bool state)
{
	if (state) 
	{
		// Subscribe to focus events. 
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this,
			SLOT( set_focus(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));
	
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focused_feature_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this,
			SLOT( display_feature_focus_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));
	} 
	else 
	{
		// Un-Subscribe to focus events. 
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this, 
			0);
	
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focused_feature_modified(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			this, 
			0);
	}
}


void
GPlatesGui::TopologyTools::connect_to_topology_sections_container_signals(
	bool state)
{
	if (state) 
	{
		QObject::connect(
			d_topology_sections_container_ptr,
			SIGNAL( cleared() ),
			this,
			SLOT( react_cleared() )
		);

		QObject::connect(
			d_topology_sections_container_ptr,
			SIGNAL( insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_topology_sections_container_ptr,
			SIGNAL( entry_removed(
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entry_removed(
				GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_topology_sections_container_ptr,
			SIGNAL( entries_inserted(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entries_inserted(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_topology_sections_container_ptr,
			SIGNAL( entries_modified(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entries_modified(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type) )
		);
	} 
	else 
	{
		// Disconnect this receiver from all signals from TopologySectionsContainer:
		QObject::disconnect(
			d_topology_sections_container_ptr,
			0,
			this,
			0
		);
	}
}

void
GPlatesGui::TopologyTools::create_child_rendered_layers()
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

	// click point of the current mouse position
	d_click_point_layer_ptr =
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

	// insert neighbors
	d_insertion_neighbors_layer_ptr =
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
	d_insertion_neighbors_layer_ptr->set_active();
	d_segments_layer_ptr->set_active();
	d_intersection_points_layer_ptr->set_active();
	d_click_point_layer_ptr->set_active();
	d_click_points_layer_ptr->set_active();
	d_end_points_layer_ptr->set_active();
}




// ===========================================================================================
// 
// Functions called from Canvas Tool or Vieportwindow code 
//

void
GPlatesGui::TopologyTools::set_click_point(double lat, double lon)
{
	d_click_point_lat = lat;
	d_click_point_lon = lon;

	draw_click_point();
}


//
// reconstruction signals get sent here from a connect in ViewportWindow.cc
//
void
GPlatesGui::TopologyTools::handle_reconstruction()
{
	if (! d_is_active) { return; }

 	// Check to make sure the topology feature is defined for this new time
	if ( d_topology_feature_ref.is_valid() )
	{
		// Get the time period for the d_topology_feature_ref's validTime prop
		// FIXME: (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");

		const GPlatesPropertyValues::GmlTimePeriod *time_period;

		GPlatesFeatureVisitors::get_property_value(
			d_topology_feature_ref, valid_time_property_name, time_period);

		const GPlatesPropertyValues::GeoTimeInstant recon_time( d_reconstruct_ptr->get_current_reconstruction_time() );

		if ( ! time_period->contains( recon_time ) )
		{
			// Clear all the layers
			draw_all_layers_clear();
			return;
		}
	}

	//
	// Continue with the update
	//
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// re-display feature focus
	display_feature( 
		d_feature_focus_ptr->focused_feature(), d_feature_focus_ptr->associated_reconstruction_geometry() );

}

//
// focus_changed signals get sent here from a connect in ViewportWindow.cc
//
void
GPlatesGui::TopologyTools::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type /*associated_rg*/)
{
	if (! d_is_active ) { 
		return; }

	// clear or paint the focused geom 
	draw_focused_geometry();

	// do nothing with a null ref
	if ( ! feature_ref.is_valid() ) { 
		return; }


	// Check feature type via qstrings 
 	QString topology_type_name ("TopologicalClosedPlateBoundary");
	QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		feature_ref->feature_type().get_name() );

	if ( feature_type_name == topology_type_name )
	{
		// NOTE: this will trigger a set_focus signal with NULL ref
		d_feature_focus_ptr->unset_focus(); 
		// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
		d_viewport_window_ptr->feature_table_model().clear();
		return;
	} 
	

	// Flip tab
	d_topology_tools_widget_ptr->choose_section_tab();

	// display this feature ; or unset focus if it is a topology
	display_feature( 
		d_feature_focus_ptr->focused_feature(), d_feature_focus_ptr->associated_reconstruction_geometry() );

	return;
}


//
void
GPlatesGui::TopologyTools::display_feature_focus_modified(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type associated_rg)
{
	display_feature( feature_ref, associated_rg );
}

//
// Display the clicked feature data in the widgets and on the globe
//
void
GPlatesGui::TopologyTools::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type associated_rg)
{
	if (! d_is_active) { return; }

	// always check weak_refs!
	if ( ! feature_ref.is_valid() ) { return; }

#ifdef DEBUG
qDebug() << "TopologyTools::display_feature() = ";
qDebug() << "d_feature_focus_ptr = " << GPlatesUtils::make_qstring_from_icu_string( d_feature_focus_ptr->focused_feature()->feature_id().get() );

	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name;
	if ( GPlatesFeatureVisitors::get_property_value(feature_ref, name_property_name, name) )
	{
		qDebug() << "name = " << GPlatesUtils::make_qstring( name->value() );
	}

	// feature id
	GPlatesModel::FeatureId id = feature_ref->feature_id();
	qDebug() << "id = " << GPlatesUtils::make_qstring_from_icu_string( id.get() );

	if ( associated_rg ) { 
		qDebug() << "associated_rg = okay " ;
	} else { 
		qDebug() << "associated_rg = NULL " ; 
	}
#endif

	//
	// Check feature type via qstrings 
	//
 	QString topology_type_name ("TopologicalClosedPlateBoundary");
	QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		feature_ref->feature_type().get_name() );

	if ( feature_type_name == topology_type_name )
	{
		// Only focus TopologicalClosedPlateBoundary types upon activate() calls
		return;
	} 
	else // non-topology feature type selected 
	{
		// Flip Topology Widget to Topology Sections Tab
		d_topology_tools_widget_ptr->choose_section_tab();

		// check if the feature is in the topology 
		int i = find_feature_in_topology( feature_ref );

		if ( i != -1 )
		{
			// Flip to the Topology Sections Table
			d_viewport_window_ptr->choose_topology_sections_table();

			// Pretend we clicked in that row
		 	TopologySectionsContainer::size_type index = 
				static_cast<TopologySectionsContainer::size_type>( i );

			d_topology_sections_container_ptr->set_focus_feature_at_index( index );
			return;
		}

		// else, not found on boundary 

		// Flip to the Clicked Features tab
		d_viewport_window_ptr->choose_clicked_geometry_table();
	}
}

int 
GPlatesGui::TopologyTools::find_feature_in_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref )
{
	// test if feature is in the section ids vector
	GPlatesModel::FeatureId test_id = feature_ref->feature_id();

	GPlatesGui::TopologySectionsContainer::const_iterator iter = 
		d_topology_sections_container_ptr->begin();

	int i = 0;
	for ( ; iter != d_topology_sections_container_ptr->end(); ++iter)
	{
		if ( test_id == iter->d_feature_id )
		{
			return i;
		}
		++i;
	}
	// feature id not found
	return -1;
}

//
// display the topology in the sections table and on the widget 
//
void
GPlatesGui::TopologyTools::display_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type /*associated_rg*/)
{
	// Clear the d_section_ vectors
	d_section_ptrs.clear();
	d_section_ids.clear();
	d_section_click_points.clear();
	d_section_reverse_flags.clear();

	// FIXME: should we create one of these visitors upon activate() and just reuse it ?
	// Create a new TopologySectionsFinder to fill d_section_ vectors
	GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder( 
		d_section_ptrs, d_section_ids, d_section_click_points, d_section_reverse_flags);

	// Visit the feature_ref, filling d_section_ vectors with data
	topo_sections_finder.visit_feature(feature_ref);

	// just to be safe, disconnect listening to feature focus while changing Section Table
	connect_to_focus_signals( false );
	connect_to_topology_sections_container_signals( false );

	// Clear the sections_table
	d_topology_sections_container_ptr->clear();

	// iterate over the table rows found in the TopologySectionsFinder
	GPlatesGui::TopologySectionsContainer::iterator table_row_itr;
	table_row_itr = topo_sections_finder.found_rows_begin();

    // loop over found rows
    for ( ; table_row_itr != topo_sections_finder.found_rows_end() ; ++table_row_itr)
    {
		// insert a row into the table
		d_topology_sections_container_ptr->insert( *table_row_itr );

		// fill the topology_sections vector
		d_topology_sections.push_back( *table_row_itr );
    }
 
	// Set the num_sections in the TopologyToolsWidget
	d_topology_tools_widget_ptr->display_number_of_sections(
		d_topology_sections_container_ptr->size() );
}



void
GPlatesGui::TopologyTools::handle_shift_left_click(
	const GPlatesMaths::PointOnSphere &click_pos_on_globe,
	const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
	bool is_on_globe)
{
	// Check if the focused feature is valid
	if ( d_feature_focus_ptr->is_valid() )
	{
		// Check if the focused feature is a topology
 		QString topology_type_name("TopologicalClosedPlateBoundary");
		QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
			(d_feature_focus_ptr->focused_feature())->feature_type().get_name() );

		if ( feature_type_name == topology_type_name ) 
		{
			return;
		}
	}
	else
	{
		// no feature focused ; just return
		return;
	}

	// check if the focused feature is in the topology
	int index = find_feature_in_topology( d_feature_focus_ptr->focused_feature() );

	if ( index != -1 )
	{
		GPlatesGui::TopologySectionsContainer::TableRow table_row;
		table_row = d_topology_sections_container_ptr->at(index);

		//
		// Compute and set the user click point 
		//

		// Get a feature handle for the id
		// get the plate id for the feature
		static const GPlatesModel::PropertyName plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
			table_row.d_feature_ref, plate_id_property_name, recon_plate_id ) )
		{
			// The feature has a reconstruction plate ID.

			// first, get the user click point into a point on sphere 
			GPlatesMaths::PointOnSphere user_point = 
				GPlatesMaths::make_point_on_sphere(
					GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon)
				);

			// Get the recon tree from the view state ptr
			GPlatesModel::Reconstruction &recon = d_reconstruct_ptr->get_current_reconstruction();
			///d_viewport_window_ptr->reconstruction();
			GPlatesModel::ReconstructionTree &recon_tree = recon.reconstruction_tree();	

			// get the forward rotation for this plate id
			const GPlatesMaths::FiniteRotation &fwd_rot = 
				recon_tree.get_composed_absolute_rotation( recon_plate_id->value() ).first;

			// get the reverse rotation
			const GPlatesMaths::FiniteRotation rev_rot = GPlatesMaths::get_reverse( fwd_rot );

			// un-reconstruct the point 
			const GPlatesMaths::PointOnSphere un_rotated_point = 
				GPlatesMaths::operator*( rev_rot, user_point );

			// set the click point coordinates:
			table_row.d_click_point =
				GPlatesMaths::make_lat_lon_point( un_rotated_point );

#if 0
			//
			// FIXME this is a TEST to use the click point as raw data with no rotations
			//
			table_row.d_click_point =
				GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon);
#endif
		}
		else
		{
			// NOTE: no rotation 
			// set the click point from canvas ; 
			table_row.d_click_point =
 				GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon);
 			// FIXME: else - what to do? 
		}

		// Update the row
		d_topology_sections_container_ptr->update_at(index, table_row);

		// flip the tab
		d_topology_tools_widget_ptr->choose_topology_tab();

		return;

	}

	// end of check on index 
	return;
}

//
// slots for signals from TopologySectionsContainer
//
void
GPlatesGui::TopologyTools::react_cleared()
{
	if (! d_is_active) { return; }
	// not used  ; keep in for potential expansion or refactoring
}

void
GPlatesGui::TopologyTools::react_insertion_point_moved(
	GPlatesGui::TopologySectionsContainer::size_type new_index)
{
	if (! d_is_active) { return; }

	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;
}

    
void
GPlatesGui::TopologyTools::react_entry_removed(
	GPlatesGui::TopologySectionsContainer::size_type deleted_index)
{
	if (! d_is_active) { return; }

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// append the new boundary
	append_boundary_to_feature( d_topology_feature_ref );
}


void
GPlatesGui::TopologyTools::react_entries_inserted(
	GPlatesGui::TopologySectionsContainer::size_type inserted_index,
	GPlatesGui::TopologySectionsContainer::size_type quantity)
{
	if (! d_is_active) { return; }

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// append the new boundary
	append_boundary_to_feature( d_topology_feature_ref );
}

//
//
//
void
GPlatesGui::TopologyTools::react_entries_modified(
	GPlatesGui::TopologySectionsContainer::size_type modified_index_begin,
	GPlatesGui::TopologySectionsContainer::size_type modified_index_end)
{
	if (! d_is_active) { return; }

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// append the new boundary
	append_boundary_to_feature( d_topology_feature_ref );
}


// 
// Button Handlers  and support functions 
// 

void
GPlatesGui::TopologyTools::handle_add_feature()
{
	// adjust the mode
	d_in_edit = true;

	// only allow adding of focused features
	if ( ! d_feature_focus_ptr->is_valid() ) 
	{ 
		return; 
	}

	// Double check that the feature is not already in the topology
	int check_index = find_feature_in_topology( d_feature_focus_ptr->focused_feature() );
	if (check_index != -1 )	
	{
		return;
	}
	// else

	// Get the current insertion point 
	int insert_index = d_topology_sections_container_ptr->insertion_point();

	// insert the feature into the boundary
	handle_insert_feature( insert_index );
}

void
GPlatesGui::TopologyTools::handle_insert_feature(int insert_index)
{
	// Flip to Topology Sections Table
	d_viewport_window_ptr->change_tab( 2 );

	// Disconnect to FeatureFocus and TopologySectionsTable signals while changing Table
	connect_to_topology_sections_container_signals( false );
	connect_to_focus_signals( false );

	// TableRow struct to insert into TopologySectionsTable
	GPlatesGui::TopologySectionsContainer::TableRow table_row;

	// Pointer to the Clicked Features table
	GPlatesGui::FeatureTableModel &clicked_table = d_viewport_window_ptr->feature_table_model();

	// Table index of clicked feature
	int click_index = clicked_table.current_index().row();

	// Get the feature id from the RG
	const GPlatesModel::ReconstructionGeometry *rg_ptr = 
		( clicked_table.geometry_sequence().begin() + click_index )->get();

	// only insert features with a valid RG
	if (! rg_ptr ) { return ; }

	// Only insert features that have a ReconstructedFeatureGeometry.
	// We exclude features with a ResolvedTopologicalGeometry because those features are
	// themselves topological boundaries and we're trying to build a topological boundary
	// from ordinary features.
	const GPlatesModel::ReconstructedFeatureGeometry *rfg_ptr = NULL;
	if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
			rg_ptr, rfg_ptr))
	{
		return;
	}

	//const GPlatesModel::FeatureId id = rfg_ptr->feature_handle_ptr()->feature_id();

	// Set the raw data for this row 
	table_row.d_feature_id = rfg_ptr->feature_handle_ptr()->feature_id();

	// set the raw data for this row 
	table_row.d_feature_ref = rfg_ptr->feature_handle_ptr()->reference();

	//
	// Compute and set the user click point 
	//

	// Get a feature handle for the id
	// get the plate id for the feature
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

	if ( GPlatesFeatureVisitors::get_property_value(
		table_row.d_feature_ref, plate_id_property_name, recon_plate_id ) )
	{
		// The feature has a reconstruction plate ID.

		// first, get the user click point into a point on sphere 
		GPlatesMaths::PointOnSphere user_point = 
			GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon)
			);

		// Get the recon tree from the view state ptr
		GPlatesModel::Reconstruction &recon = d_reconstruct_ptr->get_current_reconstruction();
		GPlatesModel::ReconstructionTree &recon_tree = recon.reconstruction_tree();	

		// get the forward rotation for this plate id
		const GPlatesMaths::FiniteRotation &fwd_rot = 
			recon_tree.get_composed_absolute_rotation( recon_plate_id->value() ).first;

		// get the reverse rotation
		const GPlatesMaths::FiniteRotation rev_rot = GPlatesMaths::get_reverse( fwd_rot );

		// un-reconstruct the point 
		const GPlatesMaths::PointOnSphere un_rotated_point = 
			GPlatesMaths::operator*( rev_rot, user_point );

		// set the click point coordinates:
		table_row.d_click_point =
			GPlatesMaths::make_lat_lon_point( un_rotated_point );

#if 0
		//
		// FIXME this is a TEST to use the click point as raw data with no rotations
		//
		table_row.d_click_point = 
			GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon);
#endif
	}
	else
	{
		// NOTE: no rotation 
		// set the click point from canvas ; 
		table_row.d_click_point = 
			GPlatesMaths::LatLonPoint(d_click_point_lat, d_click_point_lon);
		// FIXME: else - what to do? 
	}


	// Set the reverse to a default ; see below for the auto update
	table_row.d_reverse = false;

	// visit the RFG's geom to check type and set d_tmp_property_name
	d_visit_to_check_type = true;
	rfg_ptr->geometry()->accept_visitor(*this);

	const GPlatesModel::PropertyName property_name =
		GPlatesModel::PropertyName::create_gpml( d_tmp_property_name );

	// NOTE: This loop was copied from 
	// NOTE: GPlatesFeatureVisitors::TopologySectionsFinder::find_properties_iterator
	// NOTE: See the comments there.

	// NOTE: it is the row.d_geometry_property_opt  that determines
	// the color of the row in the table: YELLOW for missing geom's 

	// Iterate through the top level properties; look for the first name that matches
	GPlatesModel::FeatureHandle::properties_iterator it =
			table_row.d_feature_ref->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end =
			table_row.d_feature_ref->properties_end();

	for ( ; it != end; ++it)
	{
		if (it.is_valid() && (*it)->property_name() == property_name)
		{
			// the table row will be drawn normally
			table_row.d_geometry_property_opt = it;
			break; // out of for loop 
		}
		else
		{
			// the table row will be drawn in yellow 
			table_row.d_geometry_property_opt = boost::none;
		}
	}

	// Insert the row
	d_topology_sections_container_ptr->insert( table_row );

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	// set flags for visit from update_geom()
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// See if the inserted section should be reversed.
	// This is done after 'update_geometry()' because we need the intersection
	// clipped topology sections that neighbour the inserted section.
	if (should_reverse_section(insert_index))
	{
		// Flip the reverse flag for the inserted section.
		table_row.d_reverse = !table_row.d_reverse;

		// Update the topology sections container.
		d_topology_sections_container_ptr->update_at(insert_index, table_row);

		//
		// Since we are not connected to the topology sections container signals
		// we need to manually regenerate the topology sections.
		//

		d_feature_focus_head_points.clear();
		d_feature_focus_tail_points.clear();

		// set flags for visit from update_geom()
		d_visit_to_check_type = false;
		d_visit_to_create_properties = true;
		update_geometry();
		d_visit_to_create_properties = false;
	}

	// d_topology_feature_ref exists, simple append the boundary
	append_boundary_to_feature( d_topology_feature_ref );

	// reconnect listening to focus signals from Topology Sections table
	connect_to_topology_sections_container_signals( true );
	connect_to_focus_signals( true );
}


void
GPlatesGui::TopologyTools::handle_remove_feature()
{
	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	// Flip to Topology Sections Table
	d_viewport_window_ptr->change_tab( 2 );

	// process the sections vectors 
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// d_topology_feature_ref exists, simple append the boundary
	append_boundary_to_feature( d_topology_feature_ref );

	return;
}

void
GPlatesGui::TopologyTools::handle_remove_all_sections()
{
	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	// disconnect from signals 
	connect_to_topology_sections_container_signals(false);
	
	// loop over table
	int i = d_topology_sections_container_ptr->size(); 
	--i; // decrement to last index
	for ( ; i > -1 ; --i )
	{
		d_topology_sections_container_ptr->remove_at( i );
	}

	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	// set the new boundary
	append_boundary_to_feature( d_topology_feature_ref );

	// reconnect listening to focus signals from Topology Sections table
	connect_to_topology_sections_container_signals(true);

	return;
}

void
GPlatesGui::TopologyTools::handle_apply()
{
	// do update ; make sure to create properties
	d_visit_to_check_type = false;
	d_visit_to_create_properties = true;
	update_geometry();
	d_visit_to_create_properties = false;

	if ( ! d_topology_feature_ref.is_valid() )
	{
		// no topology feature ref exisits
		return;
	}

	// else, a d_topology_feature_ref exists, simple append the boundary
	append_boundary_to_feature( d_topology_feature_ref );

	// deacticate the TopologyToolsWidget; NOTE: will call our deactivate();
	d_topology_tools_widget_ptr->deactivate();
}


// ===========================================================================================
//
// Visitors for basic geometry types
//
void
GPlatesGui::TopologyTools::visit_multi_point_on_sphere(
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
GPlatesGui::TopologyTools::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{ 
	// set type only
	if (d_visit_to_check_type) {
		d_tmp_feature_type = GPlatesGlobal::POINT_FEATURE;
		d_tmp_property_name = "position";
		d_tmp_value_type = "Point";
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
		// single points just go in head list
		d_feature_focus_head_points.push_back( *point_on_sphere );
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

	// Update the Topology Sections Container

	// get the current row
	GPlatesGui::TopologySectionsContainer::TableRow row =
		d_topology_sections_container_ptr->at( d_tmp_index );

	// update the section pointer
	row.d_section_ptr = gtp_ptr;

	// update the row
	connect_to_topology_sections_container_signals( false );
	d_topology_sections_container_ptr->update_at( d_tmp_index, row );
	connect_to_topology_sections_container_signals( true );
}

void
GPlatesGui::TopologyTools::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	// set type only
	if (d_visit_to_check_type) {
		d_tmp_feature_type = GPlatesGlobal::POLYGON_FEATURE;
		d_tmp_property_name = "outlineOf";
		d_tmp_value_type = "LinearRing";
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
		d_feature_focus_head_points.push_back( *(polygon_on_sphere->vertex_begin()) );
		d_feature_focus_tail_points.push_back( *(--polygon_on_sphere->vertex_end()) );
		return;
	}

	// set the global flag for intersection processing 
	d_tmp_process_intersections = true;

	// Write out each point of the polygon.
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = 
		polygon_on_sphere->vertex_begin();

	GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = 
		polygon_on_sphere->vertex_end();

	// create a list of this polygon's vertices
	std::vector<GPlatesMaths::PointOnSphere> polygon_vertices;
	polygon_vertices.clear();
	
	for ( ; iter != end; ++iter) 
	{
		polygon_vertices.push_back( *iter );
	}

	// check for reverse flag
	if (d_tmp_index_use_reverse) 
	{
		d_tmp_index_vertex_list.insert( d_tmp_index_vertex_list.end(), 
			polygon_vertices.rbegin(), polygon_vertices.rend() );

		// set the head and tail end_points
		d_head_end_points.push_back( *(--polygon_on_sphere->vertex_end()) );
		d_tail_end_points.push_back( *(polygon_on_sphere->vertex_begin()) );
	}
	else 
	{
		d_tmp_index_vertex_list.insert( d_tmp_index_vertex_list.end(), 
			polygon_vertices.begin(), polygon_vertices.end() );

		// set the head and tail end_points
		d_head_end_points.push_back( *(polygon_on_sphere->vertex_begin()) );
		d_tail_end_points.push_back( *(--polygon_on_sphere->vertex_end()) );
	}

	// return early if properties are not needed
	if (! d_visit_to_create_properties) { return; }


	// Set the d_tmp vars to create a sourceGeometry property delegate 
	d_tmp_property_name = "outlineOf";
	d_tmp_value_type = "LinearRing";

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

	// Update the Topology Sections Container

	// get the current row
	GPlatesGui::TopologySectionsContainer::TableRow row =
		d_topology_sections_container_ptr->at( d_tmp_index );

	// update the section pointer
	row.d_section_ptr = gtls_ptr;

	// update the row
	connect_to_topology_sections_container_signals( false );
	d_topology_sections_container_ptr->update_at( d_tmp_index, row );
	connect_to_topology_sections_container_signals( true );
}

void
GPlatesGui::TopologyTools::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{  

	// set type only
	if (d_visit_to_check_type) {
		d_tmp_feature_type = GPlatesGlobal::LINE_FEATURE;
		d_tmp_property_name = "centerLineOf";
		d_tmp_value_type = "LineString";
		return;
	}

	// get end points only
	if (d_visit_to_get_focus_end_points) 
	{
		d_feature_focus_head_points.push_back( *(polyline_on_sphere->vertex_begin()) );
		d_feature_focus_tail_points.push_back( *(--polyline_on_sphere->vertex_end()) );
		return;
	}

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

	// Update the Topology Sections Container

	// get the current row
	GPlatesGui::TopologySectionsContainer::TableRow row =
		d_topology_sections_container_ptr->at( d_tmp_index );

	// update the section pointer
	row.d_section_ptr = gtls_ptr;

	// update the row
	connect_to_topology_sections_container_signals( false );
	d_topology_sections_container_ptr->update_at( d_tmp_index, row );
	connect_to_topology_sections_container_signals( true );
}


// ===========================================================================================
//
// draw_ functions
//
void
GPlatesGui::TopologyTools::draw_all_layers_clear()
{
	// clear all layers
	d_topology_geometry_layer_ptr->clear_rendered_geometries();
	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_insertion_neighbors_layer_ptr->clear_rendered_geometries();
	d_segments_layer_ptr->clear_rendered_geometries();
	d_end_points_layer_ptr->clear_rendered_geometries();
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_click_point_layer_ptr->clear_rendered_geometries();
	d_click_points_layer_ptr->clear_rendered_geometries();

	d_viewport_window_ptr->globe_canvas().update_canvas();
}

void
GPlatesGui::TopologyTools::draw_all_layers()
{
	// draw all the layers
	draw_topology_geometry();
	draw_focused_geometry();
	draw_segments();
	draw_end_points();
	draw_intersection_points();
	draw_insertion_neighbors();
	
	// FIXME: this tends to produce too much clutter
	draw_click_points();

	d_viewport_window_ptr->globe_canvas().update_canvas();
}


void
GPlatesGui::TopologyTools::draw_topology_geometry()
{
	d_topology_geometry_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

	if (d_topology_geometry_opt_ptr) 
	{
		// light grey
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::Colour(0.75, 0.75, 0.75, 1.0);

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*d_topology_geometry_opt_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DIGITISATION_LINE_WIDTH_HINT);

		d_topology_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	// loop over vertices
	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;
	itr = d_topology_vertices.begin();
	end = d_topology_vertices.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::Colour(0.75, 0.75, 0.75, 1.0);

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

			d_topology_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}

void
GPlatesGui::TopologyTools::draw_insertion_neighbors()
{
	d_insertion_neighbors_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

	if ( d_feature_before_insert_opt_ptr ) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*d_feature_before_insert_opt_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	
	if ( d_feature_after_insert_opt_ptr ) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*d_feature_after_insert_opt_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}

void
GPlatesGui::TopologyTools::draw_focused_geometry()
{
	d_focused_feature_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

	// always check weak refs
	if ( ! d_feature_focus_ptr->is_valid() ) { return; }

	if ( d_feature_focus_ptr->associated_reconstruction_geometry() )
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

		GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				d_feature_focus_ptr->associated_reconstruction_geometry()->geometry(),
				colour,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

		d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);

		// visit to get end_points
		d_feature_focus_head_points.clear();
		d_feature_focus_tail_points.clear();
		d_visit_to_get_focus_end_points = true;
		d_feature_focus_ptr->associated_reconstruction_geometry()->geometry()->accept_visitor(*this);
		d_visit_to_get_focus_end_points = false;

		// draw the focused end_points
		draw_focused_geometry_end_points();
	}
}


void
GPlatesGui::TopologyTools::draw_focused_geometry_end_points()
{
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
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					geom_on_sphere_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

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
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

			// Add to layer.
			d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
}

void
GPlatesGui::TopologyTools::draw_segments()
{
	d_segments_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

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
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

			d_segments_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}

void
GPlatesGui::TopologyTools::draw_end_points()
{
	d_end_points_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

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
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_grey();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

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
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_grey();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::REGULAR_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}

void
GPlatesGui::TopologyTools::draw_intersection_points()
{
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

	// loop over click points 
	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;
	itr = d_intersection_points.begin();
	end = d_intersection_points.end();
	for ( ; itr != end ; ++itr)
	{
		// get a geom
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_grey();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

			d_intersection_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}



void
GPlatesGui::TopologyTools::draw_click_point()
{
	d_click_point_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

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
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				pos_ptr,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_click_point_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}

void
GPlatesGui::TopologyTools::draw_click_points()
{
	d_click_points_layer_ptr->clear_rendered_geometries();
	d_viewport_window_ptr->globe_canvas().update_canvas();

	// loop over click points 
	std::vector<GPlatesMaths::PointOnSphere>::iterator itr, end;
	itr = d_click_points.begin();
	end = d_click_points.end();
	for ( ; itr != end ; ++itr)
	{
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type pos_ptr = 
			itr->clone_as_geometry();

		if (pos_ptr) 
		{
			const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pos_ptr,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

			d_click_points_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}

	// update the canvas 
	d_viewport_window_ptr->globe_canvas().update_canvas();
}



// ===========================================================================================
// ===========================================================================================
//
// Updater function for processing d_section_ vectors into geom and boundary prop
//

void
GPlatesGui::TopologyTools::update_geometry()
{
	// clear layers
	draw_all_layers_clear();

	// these will be filled by create_sections_from_sections_table()
	d_section_ptrs.clear();
	d_topology_vertices.clear();
	d_section_ranges_into_topology_vertices.clear();
	d_head_end_points.clear();
	d_tail_end_points.clear();
	d_intersection_points.clear();
	d_segments.clear();

	d_click_points.clear();

	// FIXME: do we need these here?
	d_feature_focus_head_points.clear();
	d_feature_focus_tail_points.clear();

	// loop over the Sections Table to check if all enties are valid
	check_sections_table();

	// loop over Sections Table to fill d_topology_vertices
	create_sections_from_sections_table();

	// Set the num_sections in the TopologyToolsWidget
	d_topology_tools_widget_ptr->display_number_of_sections(
		d_topology_sections_container_ptr->size() );

	// create the temp geom.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	// Set the d_topology_geometry_opt_ptr to the newly created geom;
	create_geometry_from_vertex_list( d_topology_vertices, validity);

	draw_all_layers();

	return;
}




///
///
///
void
GPlatesGui::TopologyTools::fill_topology_sections_from_section_table()
{
	// just to be safe, turn off connection to feature focus while changing Section Table
	connect_to_topology_sections_container_signals( false );
	connect_to_focus_signals( false );

	// Clear the old data
	d_topology_sections.clear();

	// read the table
	GPlatesGui::TopologySectionsContainer::const_iterator iter = 
		d_topology_sections_container_ptr->begin();

	for ( ; iter != d_topology_sections_container_ptr->end(); ++iter)
	{
		d_topology_sections.push_back( *iter );
	}
	
	// reconnect listening to focus signals from Topology Sections table
	connect_to_topology_sections_container_signals( true );
	connect_to_focus_signals( true );

	return;
}


///
/// Loop over the Topology Section entries and double check everything 
///

//
// FIXME: some of this can
//
void
GPlatesGui::TopologyTools::check_sections_table()
{
	// a list of the particular indices to remove 
	std::vector<int> indexes_with_errors;

	// clear the warning string 
	d_warning = tr("");

	// update the table 
	d_topology_sections_container_ptr->update();

	// get the size of the table
	d_tmp_sections_size = d_topology_sections_container_ptr->size();

	// super short cut for empty table
	if ( d_tmp_sections_size == 0 ) { 
		return; 
	}

	// Compute indices for insertion neighbors
	GPlatesGui::TopologySectionsContainer::size_type insertion_point = 
		d_topology_sections_container_ptr->insertion_point();

	d_feature_before_insert_index = --( insertion_point );
	d_feature_after_insert_index = ++( insertion_point );

	// wrap around
	if ( insertion_point == 0 ) {
		d_feature_before_insert_index = d_topology_sections_container_ptr->size();
	}
	// wrap around
	if ( insertion_point == d_topology_sections_container_ptr->size() ) {
		d_feature_after_insert_index = 0;
	}

	// Loop over each geom in the Sections Table
	d_tmp_index = 0;
	for ( ; d_tmp_index != d_tmp_sections_size ; ++d_tmp_index )
	{
		// clear the tmp index list
		d_tmp_index_vertex_list.clear();

		// Set the fid for the tmp_index section 
		d_tmp_index_fid = 
			( d_topology_sections_container_ptr->at( d_tmp_index ) ).d_feature_id;


		// set the tmp reverse flag to this feature's flag
		d_tmp_index_use_reverse =
			 ( d_topology_sections_container_ptr->at( d_tmp_index ) ).d_reverse;

		// Get a vector of FeatureHandle weak_refs for this FeatureId
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::iterator ref_itr;

		d_tmp_index_fid.find_back_ref_targets( append_as_weak_refs( back_refs ) );

		// Double check back_refs
		if ( back_refs.size() == 0 )
		{
			// FIXME: feak out? 
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: check_sections_table():";
			qDebug() << "ERROR: No feature found for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() );
			qDebug() << "ERROR: Unable to obtain feature (and its geometry, or vertices)";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";

			// create a warning message 
			d_warning.append( tr("Missing feature reference:\n") );
			d_warning.append( tr("id = ") );
			d_warning.append(
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() ) );
			d_warning.append( tr("\n") );
			d_warning.append( tr("\n") );

			// FIXME: what else to do?

			// add this entry to the remove list 
			indexes_with_errors.push_back( d_tmp_index );

			// no change to vertex_list
			continue; // to next element in the Sections Table
		}

		if ( back_refs.size() > 1)
		{
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: check_sections_table():";
			qDebug() << "ERROR: " << back_refs.size() << " features found for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() );
			qDebug() << "ERROR: Unable to determine feature";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";

			static const GPlatesModel::PropertyName prop_name = 
				GPlatesModel::PropertyName::create_gml("name");

			const GPlatesPropertyValues::XsString *name;

			// create a warning message 
			d_warning.append( tr("Multiple feature references for:\n") );
			d_warning.append( tr("id = ") );
			d_warning.append(
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() ) );
			d_warning.append( tr("\n") );
			d_warning.append( tr("The feature names found are:\n") );

			// Loop over the refs picking up the various names
			for ( ref_itr = back_refs.begin(); ref_itr != back_refs.end(); ++ref_itr)
			{
				if ( GPlatesFeatureVisitors::get_property_value(*ref_itr, prop_name, name) )
				{
					d_warning.append( tr("name = '") );
					d_warning.append( 
						GPlatesUtils::make_qstring(name->value()) );
					d_warning.append( tr("'\n") );
				}
				else 
				{
					d_warning.append("MISSSING NAME PROPERTY");
				}
			}
			
			d_warning.append(tr("\n") );

			// FIXME: what else to do?

			// add this entry to the remove list 
			indexes_with_errors.push_back( d_tmp_index );

			// no change to vertex_list
			continue; // to next element in the Sections Table
		}

		// else , get the first ref on the list
		GPlatesModel::FeatureHandle::weak_ref feature_ref = back_refs.front();
		


		// Get the RFG for this feature 
		GPlatesModel::ReconstructedFeatureGeometryFinder finder( 
			&( d_reconstruct_ptr->get_current_reconstruction() ) );

		finder.find_rfgs_of_feature( feature_ref );

		GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator find_iter;
		find_iter = finder.found_rfgs_begin();

		// Keep track of the vertices belonging to the current potentially clipped section.
		// We'll use this later to determine if a newly inserted/added section should
		// automatically have its reverse flag changed.
		clipped_section_vertex_range_type clipped_section_vertex_range(
				d_topology_vertices.size(), d_topology_vertices.size());

		// Double check RFGs
		if ( find_iter != finder.found_rfgs_end() )
		{
			// Get the geometry on sphere from the RFG
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type gos_ptr =
				(*find_iter)->geometry();

			if (gos_ptr) 
			{
				// visit the geoms.  :
				// fill additional d_tmp_index_ vars 
				// fill d_head_end_points d_tail_end_points 
				// set d_tmp_process_intersections

				// visit will call :
				// d_topology_sections_container_ptr->update_at( d_tmp_index, row )
				// to set GpmlTopologicalSection for that row

				d_visit_to_check_type = false;
				d_visit_to_create_properties = true;
				gos_ptr->accept_visitor(*this);


				// Get the GpmlTopologicalSection ptr and add it to the working vector
				if ( d_topology_sections_container_ptr->at( d_tmp_index ).d_section_ptr )
				{
					GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type gts_ptr =
					(d_topology_sections_container_ptr->at( d_tmp_index ).d_section_ptr).get();
					
					d_section_ptrs.push_back( gts_ptr );
				}
				else
				{
					qDebug() << "ERROR: ======================================";
					qDebug() << "ERROR: check_sections_table():";
					qDebug() << "ERROR: no GpmlTopologicalSection found in Table!";
					qDebug() << "ERROR: continue to next element in Sections Table";
					qDebug() << "ERROR: ======================================";

					// add this entry to the remove list 
					indexes_with_errors.push_back( d_tmp_index );

					//
					// Create warning message
					//
					d_warning.append( tr("\n") );
					d_warning.append(tr("Missing GpmlTopologicalSection for:\n") );
					d_warning.append( tr("id = ") );
					d_warning.append(
						GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() ) );
					d_warning.append( tr("\n") );
					d_warning.append( tr("name = ") );
					static const GPlatesModel::PropertyName prop_name = 
						GPlatesModel::PropertyName::create_gml("name");
					const GPlatesPropertyValues::XsString *name;
					if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop_name, name) )
					{
						d_warning.append( tr("'") );
						d_warning.append( GPlatesUtils::make_qstring( name->value() ) );
						d_warning.append( tr("'\n") );
					}
					else
					{
						d_warning.append( tr("NO name property found") );
					}
					d_warning.append( tr("\n") );
		
					//
					// Adjust d_geometry_property_opt
					//
					// get the current row
					GPlatesGui::TopologySectionsContainer::TableRow row =
						d_topology_sections_container_ptr->at( d_tmp_index );
		
					// update the d_geometry_property_opt to be 'False'
					row.d_geometry_property_opt = boost::none;
		
					// update the row
					connect_to_topology_sections_container_signals( false );
					d_topology_sections_container_ptr->update_at( d_tmp_index, row );
				connect_to_topology_sections_container_signals( true );
	
					continue;
				}
			}
			else
			{
				// else no GOS for RFG 
				qDebug() << "ERROR: ======================================";
				qDebug() << "ERROR: check_sections_table():";
				qDebug() << "ERROR: no geometry on sphere ptr!";
				qDebug() << "ERROR: continue to next element in Sections Table";
				qDebug() << "ERROR: ======================================";
				// FIXME: what to do here??!

				// add this entry to the remove list 
				indexes_with_errors.push_back( d_tmp_index );

				//
				// Create warning message
				//
				d_warning.append( tr("\n") );
				d_warning.append(tr("Missing Geometry On Sphere for:\n") );
				d_warning.append( tr("id = ") );
				d_warning.append(
					GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() ) );
				d_warning.append( tr("\n") );
				d_warning.append( tr("name = ") );
				static const GPlatesModel::PropertyName prop_name = 
					GPlatesModel::PropertyName::create_gml("name");
				const GPlatesPropertyValues::XsString *name;
				if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop_name, name) )
				{
					d_warning.append( tr("'") );
					d_warning.append( GPlatesUtils::make_qstring( name->value() ) );
					d_warning.append( tr("'\n") );
				}
				else
				{
					d_warning.append( tr("NO name property found") );
				}
				d_warning.append( tr("\n") );
	
				//
				// Adjust d_geometry_property_opt
				//
				// get the current row
				GPlatesGui::TopologySectionsContainer::TableRow row =
					d_topology_sections_container_ptr->at( d_tmp_index );
	
				// update the d_geometry_property_opt to be 'False'
				row.d_geometry_property_opt = boost::none;
	
				// update the row
				connect_to_topology_sections_container_signals( false );
				d_topology_sections_container_ptr->update_at( d_tmp_index, row );
				connect_to_topology_sections_container_signals( true );
	
				continue; // to next element in the Sections Table
			}
		} 
		else
		{
			// else no RFG found for feature 
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: check_sections_table():";
			qDebug() << "ERROR: no RFG!";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";
			// FIXME: what to do here??!

			// add this entry to the remove list 
			indexes_with_errors.push_back( d_tmp_index );

			//
			// Create warning message
			//
			d_warning.append( tr("\n") );
			d_warning.append(tr("Missing Reconstructed Feature Geometry for:\n") );
			d_warning.append( tr("id = ") );
			d_warning.append(
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() ) );
			d_warning.append( tr("\n") );
			d_warning.append( tr("name = ") );
			static const GPlatesModel::PropertyName prop_name = 
				GPlatesModel::PropertyName::create_gml("name");
			const GPlatesPropertyValues::XsString *name;
			if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop_name, name) )
			{
				d_warning.append( tr("'") );
				d_warning.append( GPlatesUtils::make_qstring( name->value() ) );
				d_warning.append( tr("'\n") );
			}
			else
			{
				d_warning.append( tr("NO name property found") );
			}
			d_warning.append( tr("\n") );

			//
			// Adjust d_geometry_property_opt
			//
			// get the current row
			GPlatesGui::TopologySectionsContainer::TableRow row =
				d_topology_sections_container_ptr->at( d_tmp_index );

			// update the d_geometry_property_opt to be 'False'
			row.d_geometry_property_opt = boost::none;

			// update the row
			connect_to_topology_sections_container_signals( false );
			d_topology_sections_container_ptr->update_at( d_tmp_index, row );
			connect_to_topology_sections_container_signals( true );

			continue; // to next element in the Sections Table
		}

	}
}

///
/// Loop over the Topology Section entries and fill the working lists
///
void
GPlatesGui::TopologyTools::create_sections_from_sections_table()
{
	// clear out the old vectors, since the calls to accept_visitor will re-populate them
	d_section_ptrs.clear();
	d_topology_vertices.clear();
	d_section_ranges_into_topology_vertices.clear();

	// clear out old ptrs 
	d_feature_before_insert_opt_ptr = boost::none;
	d_feature_after_insert_opt_ptr = boost::none;

	// get the size of the table
	d_tmp_sections_size = d_topology_sections_container_ptr->size();

	// super short cut for empty table
	if ( d_tmp_sections_size == 0 ) { 
		return; 
	}

	// Compute indices for insertion neighbors
	GPlatesGui::TopologySectionsContainer::size_type insertion_point = 
		d_topology_sections_container_ptr->insertion_point();

	d_feature_before_insert_index = --( insertion_point );
	d_feature_after_insert_index = ++( insertion_point );

	// wrap around
	if ( insertion_point == 0 ) {
		d_feature_before_insert_index = d_topology_sections_container_ptr->size();
	}
	// wrap around
	if ( insertion_point == d_topology_sections_container_ptr->size() ) {
		d_feature_after_insert_index = 0;
	}

	// Loop over each geom in the Sections Table
	d_tmp_index = 0;
	for ( ; d_tmp_index != d_tmp_sections_size ; ++d_tmp_index )
	{
		// clear the tmp index list
		d_tmp_index_vertex_list.clear();

		// Set the fid for the tmp_index section 
		d_tmp_index_fid = 
			( d_topology_sections_container_ptr->at( d_tmp_index ) ).d_feature_id;

		// set the tmp reverse flag to this feature's flag
		d_tmp_index_use_reverse =
			 ( d_topology_sections_container_ptr->at( d_tmp_index ) ).d_reverse;

		// Get a vector of FeatureHandle weak_refs for this FeatureId
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;
		d_tmp_index_fid.find_back_ref_targets( append_as_weak_refs( back_refs ) );

		// Double check back_refs
		if ( back_refs.size() == 0 )
		{
			// FIXME: feak out? 
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: create_sections_from_sections_table():";
			qDebug() << "ERROR: No feature found for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() );
			qDebug() << "ERROR: Unable to obtain feature (and its geometry, or vertices)";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";
			// FIXME: what else to do?
			// no change to vertex_list
			continue; // to next element in the Sections Table
		}

		if ( back_refs.size() > 1)
		{
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: create_sections_from_sections_table():";
			qDebug() << "ERROR: " << back_refs.size() << " features found for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( d_tmp_index_fid.get() );
			qDebug() << "ERROR: Unable to determine feature";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";
			// FIXME: what else to do?
			// no change to vertex_list
			continue; // to next element in the Sections Table
		}

		// else , get the first ref on the list
		GPlatesModel::FeatureHandle::weak_ref feature_ref = back_refs.front();
		
		// Get the RFG for this feature 
		GPlatesModel::ReconstructedFeatureGeometryFinder finder( 
				&d_reconstruct_ptr->get_current_reconstruction());

		finder.find_rfgs_of_feature( feature_ref );

		GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator find_iter;
		find_iter = finder.found_rfgs_begin();

		// Keep track of the vertices belonging to the current potentially clipped section.
		// We'll use this later to determine if a newly inserted/added section should
		// automatically have its reverse flag changed.
		clipped_section_vertex_range_type clipped_section_vertex_range(
				d_topology_vertices.size(), d_topology_vertices.size());

		// Double check RFGs
		if ( find_iter != finder.found_rfgs_end() )
		{
			// Get the geometry on sphere from the RFG
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type gos_ptr =
				(*find_iter)->geometry();

			if (gos_ptr) 
			{
				// visit the geoms.  :
				// fill additional d_tmp_index_ vars 
				// fill d_head_end_points d_tail_end_points 
				// set d_tmp_process_intersections

				// visit will call :
				// d_topology_sections_container_ptr->update_at( d_tmp_index, row )
				// to set GpmlTopologicalSection for that row

				d_visit_to_check_type = false;
				d_visit_to_create_properties = true;
				gos_ptr->accept_visitor(*this);


				// Get the GpmlTopologicalSection ptr and add it to the working vector
				if ( d_topology_sections_container_ptr->at( d_tmp_index ).d_section_ptr )
				{
					GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type gts_ptr =
					(d_topology_sections_container_ptr->at( d_tmp_index ).d_section_ptr).get();
					
					d_section_ptrs.push_back( gts_ptr );
				}
				else
				{
					qDebug() << "ERROR: ======================================";
					qDebug() << "ERROR: create_sections_from_sections_table():";
					qDebug() << "ERROR: no GpmlTopologicalSection found in Table!";
					qDebug() << "ERROR: continue to next element in Sections Table";
					qDebug() << "ERROR: ======================================";
					continue;
				}

				// Check this index for before/after_insert_index 
				if ( d_tmp_index == static_cast<int>(d_feature_before_insert_index) ) {
					// set this geom as feature_before_insert_opt_ptr 
					d_feature_before_insert_opt_ptr = (*find_iter)->geometry(); 
				}

				// Check this index for before/after_insert_index 
				if ( d_tmp_index == static_cast<int>(d_feature_after_insert_index) ) {
					// set this geom as feature_before_insert_opt_ptr 
					d_feature_after_insert_opt_ptr = (*find_iter)->geometry(); 
				}
	
				// short-cut for single item boundary
				if ( d_tmp_sections_size == 1 ) {
					d_tmp_process_intersections = false;
				}

				// Mark the beginning of the current potentially clipped section
				// in the topology vertices.
				clipped_section_vertex_range.second = d_topology_vertices.size();

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

				// Mark the end of the current potentially clipped section
				// in the topology vertices.
				clipped_section_vertex_range.second = d_topology_vertices.size();
			}
			else
			{
				// else no GOS for RFG 
				qDebug() << "ERROR: ======================================";
				qDebug() << "ERROR: create_sections_from_sections_table():";
				qDebug() << "ERROR: no geometry on sphere ptr!";
				qDebug() << "ERROR: continue to next element in Sections Table";
				qDebug() << "ERROR: ======================================";
				// FIXME: what to do here??!
				continue; // to next element in the Sections Table
			}
		} 
		else
		{
			// else no RFG found for feature 
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: create_sections_from_sections_table():";
			qDebug() << "ERROR: no RFG!";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";
			// FIXME: what to do here??!
			continue; // to next element in the Sections Table
		}

		// Store the range of topology vertices belonging to the current
		// potentially clipped section.
		// We do this even if no vertices were added because we need to have an entry for
		// each section.
		d_section_ranges_into_topology_vertices.push_back(clipped_section_vertex_range);
	}
}

void
GPlatesGui::TopologyTools::process_intersections()
{
	//
	// Double check for empty click_point for this feature 
	//
	if ( ! d_topology_sections_container_ptr->at( d_tmp_index ).d_click_point )
	{	
		qDebug() << "WARNING: ======================================";
		qDebug() << "WARNING: no click point for feature at Topology Section Table index:" << d_tmp_index;
		qDebug() << "WARNING: Unable to process intersections of this feature with neighbors without a click point.";
		qDebug() << "WARNING: If this line intersects others, then";
		qDebug() << "WARNING: remove the feature, and re-insert it from a click on the globe.";
		return;
	}

	// Get the click point ptr for this feature
	// it is set above in create_sections_from_sections_table

	GPlatesMaths::PointOnSphere click_pos = GPlatesMaths::make_point_on_sphere(
		(d_topology_sections_container_ptr->at( d_tmp_index ).d_click_point).get() 
	);

	d_click_point_ptr = &click_pos;


	// This is used below to create the reference property GPlatesPropertyValues::GmlPoint
	// in the present day coords (as stored in the section table )
	const GPlatesMaths::PointOnSphere const_pos(click_pos); 

	// rotate the click_point with the src feature 
	// before the call to compute_intersection()
	
	// Get a feature handle for the id
	// get the plate id for the feature
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

	// FIXME: this is moved outside the following 'if' statement to stop
	// 'd_click_point_ptr' referencing a local variable inside the 'if' statement
	// - but it is only used when the 'if' statement is 'true'.
	GPlatesMaths::PointOnSphere rotated_point(
			// NOTE: arbitrary unit vector because will either get unused or overwritten
			GPlatesMaths::UnitVector3D(1,0,0));

	if ( GPlatesFeatureVisitors::get_property_value(
			d_topology_sections_container_ptr->at( d_tmp_index ).d_feature_ref, 
			plate_id_property_name, 
			recon_plate_id ) )
	{
		// The feature has a reconstruction plate ID.

		// Get the recon tree from the view state ptr
		GPlatesModel::Reconstruction &recon = d_reconstruct_ptr->get_current_reconstruction();
		GPlatesModel::ReconstructionTree &recon_tree = recon.reconstruction_tree();	

		// get the forward rotation for this plate id
		const GPlatesMaths::FiniteRotation &fwd_rot = 
			recon_tree.get_composed_absolute_rotation( recon_plate_id->value() ).first;

#if 0
// TEST : 
		// get the reverse rotation
		const GPlatesMaths::FiniteRotation rev_rot = GPlatesMaths::get_reverse( fwd_rot );

		// un-reconstruct the point 
		const GPlatesMaths::PointOnSphere rotated_point = 
			GPlatesMaths::operator*( rev_rot, click_pos );
#endif

		// reconstruct the point 
		//
		// NOTE: moved declaration of 'rotated_point' outside this 'if' statement
		// to stop 'd_click_point_ptr' referencing a local variable inside this 'if' statement
		// - but it is only used when the 'if' statement is 'true'.
#if 0
		const GPlatesMaths::PointOnSphere rotated_point = 
			GPlatesMaths::operator*( fwd_rot, click_pos );
#else
		rotated_point = 
			GPlatesMaths::operator*( fwd_rot, click_pos );
#endif


		// reset the d_click_point_ptr to the rotated point
		d_click_point_ptr = &rotated_point;
	}


	// Add in the click point to the list
	d_click_points.push_back( *d_click_point_ptr );

	//
	// Index math to close the loop of boundary sections
	// The current feature, INDEX, will be tested against both PREV and NEXT below
	//
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

	// reset intersection variables
	d_num_intersections_with_prev = 0;
	d_num_intersections_with_next = 0;

	// Get a polyline for this INDEX feature

	// NOTE: the d_tmp_index segment may have had its d_tmp_index_vertex_list reversed, 
	// so use that list of points_on_sphere, rather than the geom from Sections Table.

	// tmp_for_prev_polyline is intersected with the PREV feature 
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type tmp_for_prev_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( d_tmp_index_vertex_list );

	// 
	// Access the Topology Sections Container for PREV boundary section
	//
	GPlatesModel::FeatureHandle::weak_ref prev_feature_ref;

	// access the topology sections table
	prev_feature_ref = 
		( d_topology_sections_container_ptr->at( d_tmp_prev_index ) ).d_feature_ref;

	if ( prev_feature_ref.is_valid() )
	{
		// Get the RFG for this feature 
		GPlatesModel::ReconstructedFeatureGeometryFinder finder( 
			&d_reconstruct_ptr->get_current_reconstruction());

		finder.find_rfgs_of_feature( prev_feature_ref );

		GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator find_iter;
		find_iter = finder.found_rfgs_begin();

		// Double check RFGs
		if ( find_iter != finder.found_rfgs_end() )
		{
			// Get the geometry on sphere from the RFG
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type prev_gos_ptr =
				(*find_iter)->geometry();

			if (prev_gos_ptr) 
			{
				d_visit_to_check_type = true;
				prev_gos_ptr->accept_visitor(*this);
				d_visit_to_check_type = false;

				// Only compute intersections for LINE_FEAURE
				if (d_tmp_feature_type == GPlatesGlobal::LINE_FEATURE ) 
				{
					// else process the geom as a LINE
					const GPlatesMaths::PolylineOnSphere *prev_polyline = 
						dynamic_cast<const GPlatesMaths::PolylineOnSphere *>( prev_gos_ptr.get() );

					// check if INDEX and PREV polylines intersect
					compute_intersection(
						tmp_for_prev_polyline.get(),
						prev_polyline,
						GPlatesGui::TopologyTools::INTERSECT_PREV);
				}
			}
			else
			{
				// else no GOS for RFG 
				qDebug() << "ERROR: ===================================================";
				qDebug() << "ERROR: GPlatesGui::TopologyTools::process_intersections():";
				qDebug() << "ERROR: no geometry on sphere ptr for PREV";
				d_num_intersections_with_prev = 0;
				// FIXME: what to do here??!
				
			}
		} 
		else
		{
			// else no RFG found for feature 
			qDebug() << "ERROR: ===================================================";
			qDebug() << "ERROR: GPlatesGui::TopologyTools::process_intersections():";
			qDebug() << "ERROR: no RFG for PREV";
			// FIXME: what to do here??!
			d_num_intersections_with_prev = 0;
		}
	}
	else
	{
		// else prev_feature_ref not valid
		qDebug() << "ERROR: ===================================================";
		qDebug() << "EDDOR: GPlatesGui::TopologyTools::process_intersections():";
		qDebug() << "ERROR: prev_feature_ref not valid!";
		// FIXME: what to do here??!
		d_num_intersections_with_prev = 0;
	}


	//
	// if INDEX and PREV intersect, then create the startIntersection property value
	//
	if ( (d_num_intersections_with_prev != 0) && d_visit_to_create_properties )
	{
		const GPlatesModel::FeatureId prev_fid = prev_feature_ref->feature_id();

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

		// get a GpmlTopologicalSection nnptr from the Continer
		GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type gts_ptr =
			(d_topology_sections_container_ptr->at( d_tmp_index ).d_section_ptr).get();

		// Set the start instersection
		GPlatesPropertyValues::GpmlTopologicalLineSection* gtls_ptr =
			dynamic_cast<GPlatesPropertyValues::GpmlTopologicalLineSection*>( 
				gts_ptr.get() );

		gtls_ptr->set_start_intersection( start_ti );
	}

	//
	// NOTE: d_tmp_index_vertex_list may have been changed by PREV, create another polyline 
	//

	// tmp_for_next_polyline is intersected with the NEXT feature 
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type tmp_for_next_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( d_tmp_index_vertex_list );

	//
	// Access the Topology Sections Container for NEXT boundary section
	//
	GPlatesModel::FeatureHandle::weak_ref next_feature_ref;
	next_feature_ref = ( d_topology_sections_container_ptr->at( d_tmp_next_index ) ).d_feature_ref;

	if (next_feature_ref.is_valid() )
	{
		// Get the RFG for this feature 
		GPlatesModel::ReconstructedFeatureGeometryFinder next_finder( 
			&d_reconstruct_ptr->get_current_reconstruction());

		next_finder.find_rfgs_of_feature( next_feature_ref );

		GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator next_find_iter;
		next_find_iter = next_finder.found_rfgs_begin();

		// Double check RFGs
		if ( next_find_iter != next_finder.found_rfgs_end() )
		{
			// Get the geometry on sphere from the RFG
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type next_gos_ptr =
				(*next_find_iter)->geometry();

			if (next_gos_ptr) 
			{
				d_visit_to_check_type = true;
				next_gos_ptr->accept_visitor(*this);
				d_visit_to_check_type = false;

				// Only compute intersections for LINE_FEAURE
				if (d_tmp_feature_type == GPlatesGlobal::LINE_FEATURE ) 
				{	
					// else process the geom as a LINE
					const GPlatesMaths::PolylineOnSphere *next_polyline = 
						dynamic_cast<const GPlatesMaths::PolylineOnSphere *>( next_gos_ptr.get() );

					// check if INDEX and NEXT polylines intersect
					compute_intersection(
						tmp_for_next_polyline.get(),
						next_polyline,
						GPlatesGui::TopologyTools::INTERSECT_NEXT);
				}
			}
			else
			{
				// else no GOS for RFG 
				qDebug() << "ERROR: ===================================================";
				qDebug() << "ERROR: GPlatesGui::TopologyTools::process_intersections():";
				qDebug() << "ERROR: no geometry on sphere ptr for NEXT";
				// FIXME: what to do here??!
				d_num_intersections_with_next = 0;
			}
		} 
		else
		{
			// else no RFG found for feature 
			qDebug() << "ERROR: ===================================================";
			qDebug() << "ERROR: GPlatesGui::TopologyTools::process_intersections():";
			qDebug() << "ERROR: no RFG for NEXT";
			// FIXME: what to do here??!
			d_num_intersections_with_next = 0;
		}
	}
	else
	{
		// else prev_feature_ref not valid
		qDebug() << "ERROR: ===================================================";
		qDebug() << "EDDOR: GPlatesGui::TopologyTools::process_intersections():";
		qDebug() << "ERROR: NEXT next_feature_ref not valid!";
		// FIXME: what to do here??!
		d_num_intersections_with_next = 0;
	}

	//
	// if INDEX and NEXT intersect, then create the endIntersection property value
	//
	if ( (d_num_intersections_with_next != 0) && d_visit_to_create_properties )
	{
		const GPlatesModel::FeatureId next_fid = next_feature_ref->feature_id();

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

		// get a GpmlTopologicalSection nnptr from the Topology Sections Continer
		GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type gts_ptr =
			(d_topology_sections_container_ptr->at( d_tmp_index ).d_section_ptr).get();

		// Set the end instersection
		GPlatesPropertyValues::GpmlTopologicalLineSection* gtls_ptr =
			dynamic_cast<GPlatesPropertyValues::GpmlTopologicalLineSection*>(
				gts_ptr.get() );

		gtls_ptr->set_end_intersection( end_ti );
	}
}

void
GPlatesGui::TopologyTools::compute_intersection(
	const GPlatesMaths::PolylineOnSphere* node1_polyline,
	const GPlatesMaths::PolylineOnSphere* node2_polyline,
	GPlatesGui::TopologyTools::NeighborRelation relation)
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


	// switch on relation enum to set node1's member data
	switch ( relation )
	{
		case GPlatesGui::TopologyTools::INTERSECT_PREV :
			d_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesGui::TopologyTools::INTERSECT_NEXT:
			d_num_intersections_with_next = num_intersect;
			break;

		case GPlatesGui::TopologyTools::NONE :
		case GPlatesGui::TopologyTools::OTHER :
		default :
			// something bad happened freak out
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
qDebug() << "TopologyTools::compute_intersection:: HEAD: verts:";
for ( ; iter != end; ++iter) {
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
qDebug() << "llp=" << llp.latitude() << "," << llp.longitude();
}

iter = parts.first.back()->vertex_begin();
end = parts.first.back()->vertex_end();
qDebug() << "TopologyTools::compute_intersection:: TAIL: verts:";
for ( ; iter != end; ++iter) {
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
qDebug() << "llp=" << llp.latitude() << "," << llp.longitude();
}
#endif

		// now check which element of parts.first is closest to d_click_point_ptr:

		// FIXME: is there a global setting for PROXIMITY?
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
			qDebug() << "GPlatesGui::TopologyTools::compute_intersection(): "
			<< "WARN: click point not close to anything!"
			<< "WARN: Unable to set boundary feature intersection flags!";
			return;
		}

#ifdef DEBUG1
qDebug() << "TopologyTools::compute_intersection: "
<< "closeness_head=" << closeness_head.dval() << " and " 
<< "closeness_tail=" << closeness_tail.dval();
GPlatesMaths::LatLonPoint llp_pff = GPlatesMaths::make_lat_lon_point( *(parts.first.front()->vertex_begin()) );
GPlatesMaths::LatLonPoint llp_pfb = GPlatesMaths::make_lat_lon_point( *(parts.first.back()->vertex_begin()) );
#endif

		// now compare the closeness values to set relation
		if ( closeness_head > closeness_tail )
		{
#ifdef DEBUG1
qDebug() << "TopologyTools::compute_intersection: " << "use HEAD";
#endif
			d_closeness = closeness_head;

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesGui::TopologyTools::INTERSECT_PREV :
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					// save intersection point
					d_intersection_points.push_back( *(parts.first.front()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "TopologyTools::compute_intersection: llp_pff =" << llp_pff.latitude() << "," << llp_pff.longitude();
#endif
					break;
	
				case GPlatesGui::TopologyTools::INTERSECT_NEXT:
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.front()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "TopologyTools::compute_intersection: llp_pff =" << llp_pff.latitude() << "," << llp_pff.longitude();
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
qDebug() << "TopologyTools::compute_intersection: " << "use TAIL" ;
#endif

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesGui::TopologyTools::INTERSECT_PREV :
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.back()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "TopologyTools::compute_intersection: llp=" << llp_pfb.latitude() << "," << llp_pfb.longitude();
#endif

					break;
	
				case GPlatesGui::TopologyTools::INTERSECT_NEXT:
					d_tmp_index_vertex_list.clear();
					copy(
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( d_tmp_index_vertex_list )
					);
					d_intersection_points.push_back( *(parts.first.back()->vertex_begin()) );
#ifdef DEBUG1
qDebug() << "TopologyTools::compute_intersection: llp=" << llp_pfb.latitude() << "," << llp_pfb.longitude();
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
		qDebug() << "TopologyTools::compute_intersection: "
		<< "WARN: num_intersect=" << num_intersect
		<< "WARN: Unable to set boundary feature intersection relations!"
		<< "WARN: Make sure boundary feature's only intersect once.";
	}

}

void
GPlatesGui::TopologyTools::handle_create_new_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// Set the d_topology_feature_ref to the newly created feature 
	d_topology_feature_ref = feature_ref;
}


void
GPlatesGui::TopologyTools::append_boundary_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// double check for non existant features
	if ( ! feature_ref.is_valid() ) { return; }

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

		if ( test_name == boundary_prop_name )
		{
			// Delete the old boundary 
			GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
			feature_ref->remove_top_level_property(iter, transaction);
			transaction.commit();
			// FIXME: this seems to create NULL pointers in the properties collection
			// see FIXME note above to check for NULL? 
			// Or is this to be expected?
			break;
		}
	} // loop over properties

	
	//
	// d_section_ptrs has been filled by a call to create_sections_from_sections_table()
	//

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
		feature_ref, valid_time_property_name, time_period);

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
	d_reconstruct_ptr->reconstruct(); 
}



void
GPlatesGui::TopologyTools::show_numbers()
{
	qDebug() << "############################################################"; 
	qDebug() << "TopologyTools::show_numbers: "; 
	qDebug() << "d_topology_sections_container_ptr->size() = " << d_topology_sections_container_ptr->size(); 
	qDebug() << "d_topology_sections.size()                = " << d_topology_sections.size(); 

	qDebug() << "d_topology_vertices.size()    = " << d_topology_vertices.size(); 
	qDebug() << "d_tmp_index_vertex_list.size()= " << d_tmp_index_vertex_list.size();
	qDebug() << "d_head_end_points.size()      = " << d_head_end_points.size(); 
	qDebug() << "d_tail_end_points.size()      = " << d_tail_end_points.size(); 
	qDebug() << "d_intersection_points.size()  = " << d_intersection_points.size(); 
	qDebug() << "d_segments.size()             = " << d_segments.size(); 
	qDebug() << "d_section_click_points.size() = " << d_section_click_points.size(); 
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
			d_feature_focus_ptr->focused_feature(), name_property_name, name) )
		{
			qDebug() << "d_feature_focus_ptr name = " << GPlatesUtils::make_qstring(name->value());
		}
		else 
		{
			qDebug() << "d_feature_focus_ptr = INVALID";
		}
	}

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
			d_topology_feature_ref, name_property_name, name) )
		{
			qDebug() << "d_topology_feature_ref name = " << GPlatesUtils::make_qstring( name->value() );
		} 
		else 
		{
			qDebug() << "d_topology_feature_ref = INVALID";
		}
	}
	qDebug() << "############################################################"; 

}

void
GPlatesGui::TopologyTools::create_geometry_from_vertex_list(
	std::vector<GPlatesMaths::PointOnSphere> &points,
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
{
	// FIXME: Only handles the unbroken line and single-ring cases.

	// There's no guarantee that adjacent points in the table aren't identical.
	std::vector<GPlatesMaths::PointOnSphere>::size_type num_points =
			GPlatesMaths::count_distinct_adjacent_points(points);

	// FIXME: I think... we need some way to add data() to the 'header' QTWIs, so that
	// we can immediately discover which bits are supposed to be polygon exteriors etc.
	// Then the function calculate_label_for_item could do all our 'tagging' of
	// geometry parts, and -this- function wouldn't need to duplicate the logic.
	// FIXME 2: We should have a 'try {  } catch {  }' block to catch any exceptions
	// thrown during the instantiation of the geometries.
	// This will become a proper 'try {  } catch {  } block' when we get around to it.
	try
	{
			d_topology_geometry_opt_ptr = boost::none;

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

	} catch (...) {
		throw;
	}
}


bool
GPlatesGui::TopologyTools::should_reverse_section(
		int curr_section_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			curr_section_index < boost::numeric_cast<int>(d_section_ranges_into_topology_vertices.size()),
			GPLATES_ASSERTION_SOURCE);

	// If there's less than two sections in topology then we have no
	// way to determine if the section should be reversed.
	if (d_section_ranges_into_topology_vertices.size() < 2)
	{
		return false;
	}

	// Get index of previous section.
	int prev_section_index = curr_section_index - 1;
	if (prev_section_index < 0)
	{
		prev_section_index = d_section_ranges_into_topology_vertices.size() - 1;
	}

	// Get index of next section.
	int next_section_index = curr_section_index + 1;
	if (next_section_index ==
			boost::numeric_cast<int>(d_section_ranges_into_topology_vertices.size()))
	{
		next_section_index = 0;
	}

	// Range of previous section.
	const clipped_section_vertex_range_type &prev_section_vertex_range =
			d_section_ranges_into_topology_vertices[prev_section_index];

	// Range of current section.
	const clipped_section_vertex_range_type &curr_section_vertex_range =
			d_section_ranges_into_topology_vertices[curr_section_index];

	// Range of next section.
	const clipped_section_vertex_range_type &next_section_vertex_range =
			d_section_ranges_into_topology_vertices[next_section_index];

	GPlatesMaths::real_t arc_distance = 0;
	GPlatesMaths::real_t reversed_arc_distance = 0;

	if (curr_section_vertex_range.first == curr_section_vertex_range.second)
	{
		// There are no vertices in the current section so nothing to do.
		return false;
	}

	const GPlatesMaths::PointOnSphere &curr_section_head =
			d_topology_vertices[curr_section_vertex_range.first];
	const GPlatesMaths::PointOnSphere &curr_section_tail =
			d_topology_vertices[curr_section_vertex_range.second - 1];

	// If there are vertices in the previous section.
	if (prev_section_vertex_range.first != prev_section_vertex_range.second)
	{
		const GPlatesMaths::PointOnSphere &prev_section_tail =
				d_topology_vertices[prev_section_vertex_range.second - 1];

		// Calculate angle between tail of previous section and head of current section.
		arc_distance += acos(dot(
				prev_section_tail.position_vector(),
				curr_section_head.position_vector()));

		// Same but we're reversing the current section's head and tail.
		reversed_arc_distance += acos(dot(
				prev_section_tail.position_vector(),
				curr_section_tail.position_vector()));
	}

	// If there are vertices in the next section.
	if (next_section_vertex_range.first != next_section_vertex_range.second)
	{
		const GPlatesMaths::PointOnSphere &next_section_head =
				d_topology_vertices[next_section_vertex_range.first];

		// Calculate angle between tail of current section and head of next section.
		arc_distance += acos(dot(
				curr_section_tail.position_vector(),
				next_section_head.position_vector()));

		// Same but we're reversing the current section's head and tail.
		reversed_arc_distance += acos(dot(
				curr_section_head.position_vector(),
				next_section_head.position_vector()));
	}

	// If the distance is smaller when the current section is reversed then we should
	// reverse it.
	// NOTE: if both are zero then GPlatesMath::real_t::operator<() will return false
	// which is what we want.
	return reversed_arc_distance < arc_distance;
}
