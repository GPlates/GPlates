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

#include <algorithm>
#include <iterator>
#include <map>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/numeric/conversion/cast.hpp>

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

#include "TopologyTools.h"

#include "ChooseCanvasTool.h"
#include "FeatureFocus.h"

#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/TopologyInternalUtils.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/TopologySectionsFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/FeatureTableModel.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/GeometryOnSphere.h"
#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPoint.h"
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

#include "presentation/ViewState.h"

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
#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/TopologyToolsWidget.h"
#include "qt-widgets/ViewportWindow.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryUtil.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesGui::TopologyTools::TopologyTools(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool):
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_feature_focus_ptr(&view_state.get_feature_focus()),
	d_reconstruct_ptr(&view_state.get_reconstruct()),
	d_viewport_window_ptr(&viewport_window),
	d_choose_canvas_tool(&choose_canvas_tool)
{
	// set up the drawing
	create_child_rendered_layers();

	// Set pointer to TopologySectionsContainer
	d_topology_sections_container_ptr = &( d_viewport_window_ptr->topology_sections_container() );


	// set the internal state flags
	d_is_active = false;
	d_in_edit = false;
}


void
GPlatesGui::TopologyTools::activate( CanvasToolMode mode )
{
	//
	// Set the mode and active state first.
	//

	// Set the mode 
	d_mode = mode;

	// set the widget state
	d_is_active = true;

	//
	// Connect to signals second.
	//

	// 
	// Connect to signals from Topology Sections Container
	connect_to_topology_sections_container_signals( true );

	// Connect to focus signals from Feature Focus
	connect_to_focus_signals( true );

	// Connect to recon time changes
	QObject::connect(
		d_reconstruct_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::Reconstruct &, bool, bool)),
		this,
		SLOT(handle_reconstruction()));


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

		// Check the focused feature topology type
 		static const QString topology_boundary_type_name("TopologicalClosedPlateBoundary");
 		static const QString topology_network_type_name("TopologicalNetwork");
		const QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
			(d_feature_focus_ptr->focused_feature())->handle_data().feature_type().get_name() );

		if ( feature_type_name == topology_boundary_type_name )
		{ 
			d_topology_type = GPlatesGlobal::PLATEPOLYGON;
		}
		else if ( feature_type_name == topology_network_type_name )
		{
			d_topology_type = GPlatesGlobal::NETWORK;
		}
		else
		{
			d_topology_type = GPlatesGlobal::UNKNOWN_TOPOLOGY;
		}

		// Load the topology into the Topology Sections Table 
 		initialise_focused_topology();

		// Set the num_sections in the TopologyToolsWidget
		d_topology_tools_widget_ptr->display_number_of_sections(
			d_topology_sections_container_ptr->size() );

		// Load the topology into the Topology Widget
		d_topology_tools_widget_ptr->display_topology(
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_reconstruction_geometry() );

		// NOTE: this will NOT trigger a set_focus signal with NULL ref ; 
		// NOTE: the focus connection is below 
		d_feature_focus_ptr->unset_focus();
		// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
		d_viewport_window_ptr->feature_table_model().clear();

		// Flip the ViewportWindow to the Topology Sections Table
		d_viewport_window_ptr->choose_topology_sections_table();

		// Flip the TopologyToolsWidget to the Toplogy Tab
		d_topology_tools_widget_ptr->choose_topology_tab();
}


void
GPlatesGui::TopologyTools::deactivate()
{
	// Unset any focused feature
	if ( d_feature_focus_ptr->is_valid() )
	{
		d_feature_focus_ptr->unset_focus();
	}

	// Flip the ViewportWindow to the Clicked Geometry Table
	d_viewport_window_ptr->choose_clicked_geometry_table();

	// Clear out all old data.
	// NOTE: We should be connected to the topology sections container
	//       signals for this to work properly.
	clear_widgets_and_data();

	//
	// Disconnect signals last.
	//

	// Connect to focus signals from Feature Focus.
	connect_to_focus_signals( false );

	// Connect to signals from Topology Sections Container.
	connect_to_topology_sections_container_signals( false );

	// Disconnect to recon time changes.
	QObject::disconnect(
		d_reconstruct_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::Reconstruct &, bool, bool)),
		this,
		SLOT(handle_reconstruction()));

	// Reset internal state - the very last thing we should do.
	d_is_active = false;
}


void
GPlatesGui::TopologyTools::clear_widgets_and_data()
{
	// clear the tables
	d_viewport_window_ptr->feature_table_model().clear();

	// Clear the TopologySectionsTable.
	// NOTE: This will generate a signal that will call our 'react_cleared()'
	// method which clear out our internal section sequence and redraw.
	d_topology_sections_container_ptr->clear();

	// Set the topology feature ref to NULL
	d_topology_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
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
				GPlatesGui::FeatureFocus &)),
			this,
			SLOT( set_focus(
				GPlatesGui::FeatureFocus &)));
	
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focused_feature_modified(
				GPlatesGui::FeatureFocus &)),
			this,
			SLOT( display_feature_focus_modified(
				GPlatesGui::FeatureFocus &)));
	} 
	else 
	{
		// Un-Subscribe to focus events. 
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focus_changed(
				GPlatesGui::FeatureFocus &)),
			this, 
			0);
	
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focused_feature_modified(
				GPlatesGui::FeatureFocus &)),
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
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::const_iterator,
				GPlatesGui::TopologySectionsContainer::const_iterator) ),
		this,
			SLOT( react_entries_inserted(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::const_iterator,
				GPlatesGui::TopologySectionsContainer::const_iterator) )
		);

		QObject::connect(
			d_topology_sections_container_ptr,
			SIGNAL( entry_modified(
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entry_modified(
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


void
GPlatesGui::TopologyTools::set_click_point(double lat, double lon)
{
	const GPlatesMaths::PointOnSphere &reconstructed_click_point =
			GPlatesMaths::make_point_on_sphere(
					GPlatesMaths::LatLonPoint(lat, lon));

	// NOTE: This relies on the focused feature being set before we are called -
	// this is currently taken care of by the BuildTopology and EditTopology classes.
	d_click_point.set_focus(
			d_feature_focus_ptr->focused_feature(),
			reconstructed_click_point,
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree());

	draw_click_point();
}


void
GPlatesGui::TopologyTools::handle_reconstruction()
{
#ifdef DEBUG
std::cout << "GPlatesGui::TopologyTools::handle_reconstruction() " << std::endl;
#endif

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

		const GPlatesPropertyValues::GeoTimeInstant recon_time(
				d_reconstruct_ptr->get_current_reconstruction_time());

		if ( ! time_period->contains( recon_time ) )
		{
			// Clear all the layers
			draw_all_layers_clear();
			return;
		}
	}

	// Update the click point.
	// NOTE: This is necessary since the user might click on a feature, then
	// animate the reconstruction time (moving the focused feature away from the
	// original click point) and then add the focused feature to the topology thus
	// giving it a click point that is not on the feature like it was originally).
	d_click_point.update_reconstructed_click_point(
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree());
	draw_click_point();

	// Update all topology sections and redraw.
	update_and_redraw_topology(0, d_section_info_seq.size());

	// re-display feature focus
	display_feature( 
		d_feature_focus_ptr->focused_feature(),
		d_feature_focus_ptr->associated_geometry_property() );
}


void
GPlatesGui::TopologyTools::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	if (! d_is_active ) { return; }

	// clear or paint the focused geom 
	draw_focused_geometry();

	const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_focus.focused_feature();

	// do nothing with a null ref
	if ( ! feature_ref.is_valid() )
	{
		// Reset the click point - it represents where the user clicked on a feature
		// and since the feature is now unfocused we should remove it.
		d_click_point.unset_focus();
		draw_click_point();

		return;
	}


	//
	// The following check for "TopologicalClosedPlateBoundary" is not needed
	// anymore since only ReconstructedFeatureGeometry's and not
	// ResolvedTopologicalBoundary's are added to the clicked feature table
	// when using the topology tools.
	// However we'll keep it here just in case.
	//
	// Check feature type via qstrings 
 	static const QString topology_boundary_type_name ("TopologicalClosedPlateBoundary");
 	static const QString topology_network_type_name ("TopologicalNetwork");
	QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		feature_ref->handle_data().feature_type().get_name() );

	if ( ( feature_type_name == topology_boundary_type_name ) ||
		( feature_type_name == topology_network_type_name ) )
	{
		// NOTE: this will trigger a set_focus signal with NULL ref
		d_feature_focus_ptr->unset_focus(); 

// NOTE: Not clearing feature table because there might be
// other non-TopologicalClosedPlateBoundary features in the table that the user can
// select. Without this commented out there would have been intermittent cases where the
// a valid feature was not getting highlighted (focused) - this would have happened when
// the user clicked on two features, for example, where one is a TopologicalClosedPlateBoundary
// and the other not - if the TopologicalClosedPlateBoundary was closest to the click point
// it would get the feature focus and this code here would then clear the table preventing
// the user from selecting the non-TopologicalClosedPlateBoundary feature.
#if 0
		// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
		d_viewport_window_ptr->feature_table_model().clear();
#endif
		return;
	} 
	

	// Flip tab
	d_topology_tools_widget_ptr->choose_section_tab();

	// display this feature ; or unset focus if it is a topology
	display_feature( 
		d_feature_focus_ptr->focused_feature(),
		d_feature_focus_ptr->associated_geometry_property());

	return;
}


void
GPlatesGui::TopologyTools::display_feature_focus_modified(
		GPlatesGui::FeatureFocus &feature_focus)
{
	display_feature(
			feature_focus.focused_feature(),
			feature_focus.associated_geometry_property());
}


void
GPlatesGui::TopologyTools::display_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::children_iterator &properties_iter)
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

	if ( properties_iter.is_valid() ) { 
		qDebug() << "associated_geometry_property valid " ;
	} else { 
		qDebug() << "associated_geometry_property invalid " ; 
	}
#endif

	//
	// Check feature type via qstrings 
	//
 	static const QString topology_boundary_type_name ("TopologicalClosedPlateBoundary");
 	static const QString topology_network_type_name ("TopologicalNetwork");
	const QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		feature_ref->handle_data().feature_type().get_name() );

	if ( ( feature_type_name == topology_boundary_type_name ) ||
		( feature_type_name == topology_network_type_name ) )
	{
		// Only focus TopologicalClosedPlateBoundary types upon activate() calls
		return;
	} 
	else // non-topology feature type selected 
	{
		// Flip Topology Widget to Topology Sections Tab
		d_topology_tools_widget_ptr->choose_section_tab();

		// check if the feature is in the topology 
		int i = find_topological_section_index(feature_ref, properties_iter);

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
GPlatesGui::TopologyTools::find_topological_section_index(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::children_iterator &properties_iter)
{
	if ( !(feature_ref.is_valid() && properties_iter.is_valid()) )
	{
		// Return not found if either feature reference or property iterator is invalid.
		return -1;
	}

	GPlatesGui::TopologySectionsContainer::const_iterator sections_iter = 
		d_topology_sections_container_ptr->begin();
	const GPlatesGui::TopologySectionsContainer::const_iterator sections_end = 
		d_topology_sections_container_ptr->end();

	for (int section_index = 0;
		sections_iter != sections_end;
		++sections_iter, ++section_index)
	{
		// See if the feature reference and geometry property iterator matches.
		if (feature_ref == sections_iter->get_feature_ref() &&
			properties_iter == sections_iter->get_geometry_property())
		{
			return section_index;
		}
	}

	// Feature id not found.
	return -1;
}


void
GPlatesGui::TopologyTools::initialise_focused_topology()
{
	// set the topology feature ref
	d_topology_feature_ref = d_feature_focus_ptr->focused_feature();

	// Create a new TopologySectionsFinder to fill a topology sections container
	// table row for each topology section found.
	GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder;

	// Visit the topology feature.
	topo_sections_finder.visit_feature(d_topology_feature_ref);

	// NOTE: This will generate a signal that will call our 'react_cleared()'
	// method which clear out our internal section sequence and redraw.
	d_topology_sections_container_ptr->clear();

	// Iterate over the table rows found in the TopologySectionsFinder and
	// insert them into the topology sections container.
	// NOTE: This will generate a signal that will call our 'react_entries_inserted()'
	// method which will handle the building of our topology data structures.
	d_topology_sections_container_ptr->insert(
			topo_sections_finder.found_rows_begin(),
			topo_sections_finder.found_rows_end());

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesGui::TopologyTools::handle_shift_left_click(
	const GPlatesMaths::PointOnSphere &click_pos_on_globe,
	const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
	bool is_on_globe)
{
	// Check if the focused feature is valid
	if ( !d_feature_focus_ptr->is_valid() )
	{
		// no feature focused ; just return
		return;
	}

	// Check if the focused feature is a topology
	static const QString topology_boundary_type_name("TopologicalClosedPlateBoundary");
	static const QString topology_network_type_name("TopologicalNetwork");
	const QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		(d_feature_focus_ptr->focused_feature())->handle_data().feature_type().get_name() );
	if ( feature_type_name == topology_boundary_type_name ||
		feature_type_name == topology_network_type_name ) 
	{
		return;
	}

	// check if the focused feature is in the topology
	const int section_index = find_topological_section_index(
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());

	if (section_index < 0)
	{
		return;
	}

	SectionInfo &section_info = d_section_info_seq[section_index];

	// Set the unrotated user click point in the table row.
	section_info.d_table_row.set_present_day_click_point(
			d_click_point.d_present_day_click_point);

	// Update the row in the topology sections container.
	// NOTE: This will generate a signal that will call our 'react_entry_modified()' method.
	d_topology_sections_container_ptr->update_at(
			section_index,
			section_info.d_table_row);

	// flip the tab
	d_topology_tools_widget_ptr->choose_topology_tab();
}


void
GPlatesGui::TopologyTools::react_cleared()
{
	if (! d_is_active) { return; }

	// Remove all our internal sections.
	d_section_info_seq.clear();

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	// This is a bit dodgy since there are no sections -
	// but 'update_and_redraw_topology()' is designed to work in this case and
	// it does clear our topology state.
	update_and_redraw_topology(0, 0);
}


void
GPlatesGui::TopologyTools::react_insertion_point_moved(
	GPlatesGui::TopologySectionsContainer::size_type new_index)
{
	if (! d_is_active) { return; }

	// WARNING: We don't do anything with this slot because when a table row
	// is inserted into the topology sections container it will emit two signals.
	// The first being the 'insertion_point_moved' signal and then second being
	// the 'entries_inserted' signal. However the second signal is where we
	// update our internal 'd_section_info_seq' sequence to stay in sync with
	// the topology sections container. So in this method we cannot assume that the
	// two are in sync yet and hence we cannot really do anything here.
	// However, the 'insertion_point_moved' signal usually happens with the
	// 'entries_inserted' signal so we can do anything needed in our
	// 'react_entries_inserted' slot instead.

	// However, sometimes the insertion point is moved when new table rows are not
	// being inserted (eg, the user has moved it via the sections table GUI).
	// We can detect this by seeing if our internal sections sequence is in sync
	// with the topology sections container.
	// Besides, if they are out of sync then we shouldn't be doing anything anyway.
	if (d_section_info_seq.size() == d_topology_sections_container_ptr->size())
	{
		draw_insertion_neighbors();
	}
}


void
GPlatesGui::TopologyTools::react_entry_removed(
	GPlatesGui::TopologySectionsContainer::size_type deleted_index)
{
	if (! d_is_active) { return; }

	// Make sure delete index is not out of range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			deleted_index < d_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);
	// Remove from our internal section sequence.
	d_section_info_seq.erase(d_section_info_seq.begin() + deleted_index);

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology(deleted_index, 0);
}


void
GPlatesGui::TopologyTools::react_entries_inserted(
		GPlatesGui::TopologySectionsContainer::size_type inserted_index,
		GPlatesGui::TopologySectionsContainer::size_type quantity,
		GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
		GPlatesGui::TopologySectionsContainer::const_iterator inserted_end)
{
	if (! d_is_active) { return; }

	// Iterate over the table rows inserted in the topology sections container and
	// also insert them into our internal section sequence structure.
	using boost::lambda::_1;
	std::transform(
			inserted_begin,
			inserted_end,
			std::inserter(
					d_section_info_seq,
					d_section_info_seq.begin() + inserted_index),
			// Calls the SectionInfo constructor with a TableRow as the argument...
			boost::lambda::bind(
					boost::lambda::constructor<SectionInfo>(),
					_1));

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology(inserted_index, quantity);
}


void
GPlatesGui::TopologyTools::react_entry_modified(
	GPlatesGui::TopologySectionsContainer::size_type modified_section_index)
{
	if (! d_is_active) { return; }

	// Our section sequence should be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Copy the table row into our own internal sequence.
	d_section_info_seq[modified_section_index].d_table_row =
			d_topology_sections_container_ptr->at(modified_section_index);

	// Update topology and redraw.
	update_and_redraw_topology(modified_section_index, 1);
}


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
	const int check_index = find_topological_section_index(
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());
	if (check_index != -1 )	
	{
		return;
	}

	// Get the current insertion point 
	const section_info_seq_type::size_type insert_index =
			d_topology_sections_container_ptr->insertion_point();

	// Flip to Topology Sections Table
	d_viewport_window_ptr->change_tab( 2 );

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
	// We exclude features with a ResolvedTopologicalBoundary because those features are
	// themselves topological boundaries and we're trying to build a topological boundary
	// from ordinary features.
	const GPlatesModel::ReconstructedFeatureGeometry *rfg_ptr = NULL;
	if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
			rg_ptr, rfg_ptr))
	{
		return;
	}

	// The table row to insert into the topology sections container.
	//
	// This user click point is the last user click intercepted by us.
	// It is where the user clicked on the feature we are now adding (rotated
	// to present day).
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(
			rfg_ptr->property(),
			d_click_point.d_present_day_click_point);

	// Insert the row.
	// NOTE: This will generate a signal that will call our 'react_entries_inserted()'
	// method which will handle the building of our topology data structures.
	d_topology_sections_container_ptr->insert( table_row );

	// See if the inserted section should be reversed.
	// This is done after everything has been updated because we need the intersection
	// clipped topology sections that neighbour the inserted section when determining
	// if we should reverse the current section or not.
	if (should_reverse_section(insert_index))
	{
		GPlatesGui::TopologySectionsContainer::TableRow reversed_table_row =
				d_section_info_seq[insert_index].d_table_row;

		// Flip the reverse flag for the inserted section.
		reversed_table_row.set_reverse(!reversed_table_row.get_reverse());

		// Update the topology sections container.
		// NOTE: This will generate a signal that will call our 'react_entry_modified()'
		// method which will handle any changes made.
		d_topology_sections_container_ptr->update_at(insert_index, reversed_table_row);
	}

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();
}


void
GPlatesGui::TopologyTools::handle_remove_all_sections()
{
	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();

	// Remove all sections from the topology sections container.
	// NOTE: This will generate a signal that will call our 'react_cleared()'
	// method which clear out our internal section sequence and redraw.
	d_topology_sections_container_ptr->clear();
}


void
GPlatesGui::TopologyTools::handle_apply()
{
	if ( ! d_topology_feature_ref.is_valid() )
	{
		// no topology feature ref exists
		return;
	}

	// Convert the current topology state to a 'gpml:boundary' property value
	// and attach it to the topology feature reference.
	convert_topology_to_boundary_feature_property(d_topology_feature_ref);

	// Now that we're finished building/editing the topology switch to the
	// tool used to choose a feature - this will allow the user to select
	// another topology for editing or do something else altogether.
	d_choose_canvas_tool->choose_click_geometry_tool();
}


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
}


void
GPlatesGui::TopologyTools::draw_all_layers()
{
	// draw all the layers
	draw_topology_geometry();
	draw_segments();
	draw_end_points();
	draw_intersection_points();
	draw_insertion_neighbors();
	
	// FIXME: this tends to produce too much clutter
	draw_click_points();
}


void
GPlatesGui::TopologyTools::draw_topology_geometry()
{
	d_topology_geometry_layer_ptr->clear_rendered_geometries();


	if ( d_topology_type == GPlatesGlobal::NETWORK )
	{
		// FIXME: eventually we will want the network drawn here too, 
		// but for now, don't draw the network, just let the Resolver do it
	}
	else
	{
		if (d_topology_geometry_opt_ptr) 
		{
			// draw the single plate polygon 
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
	}


	const GPlatesGui::Colour &point_colour = GPlatesGui::Colour::Colour(0.75, 0.75, 0.75, 1.0);

	// Loop over the topology vertices.
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator topology_point_iter =
			d_topology_vertices.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator topology_point_end =
			d_topology_vertices.end();
	for ( ; topology_point_iter != topology_point_end ; ++topology_point_iter)
	{
		const GPlatesMaths::PointOnSphere &point = *topology_point_iter;

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
				point,
				point_colour,
				GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

		d_topology_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}


void
GPlatesGui::TopologyTools::draw_insertion_neighbors()
{
	d_insertion_neighbors_layer_ptr->clear_rendered_geometries();

	// Our internal section sequence should be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	if (d_section_info_seq.empty())
	{
		return;
	}

	// Get the current insertion point in the topology sections container.
	const GPlatesGui::TopologySectionsContainer::size_type insertion_point = 
			d_topology_sections_container_ptr->insertion_point();

	// Index of the geometry just before the insertion point.
	const GPlatesGui::TopologySectionsContainer::size_type prev_index =
			get_prev_section_index(insertion_point);

	// Index of the geometry after the insertion point.
	// This is actually the geometry at the insertion point index since we haven't
	// inserted any geometry yet.
	const GPlatesGui::TopologySectionsContainer::size_type next_index = insertion_point;


	if (d_section_info_seq[prev_index].d_section_geometry_unreversed) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*d_section_info_seq[prev_index].d_section_geometry_unreversed,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}

	if (d_section_info_seq[next_index].d_section_geometry_unreversed) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*d_section_info_seq[next_index].d_section_geometry_unreversed,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}


void
GPlatesGui::TopologyTools::draw_focused_geometry()
{
	d_focused_feature_layer_ptr->clear_rendered_geometries();

	// always check weak refs
	if ( ! d_feature_focus_ptr->is_valid() ) { return; }

	if ( d_feature_focus_ptr->associated_reconstruction_geometry() )
	{
		// Check if the focused feature is not already in the topology.
		// If it is then we won't highlight it since we don't want the user
		// to think they can select it again.
		if (find_topological_section_index(
				d_feature_focus_ptr->focused_feature(),
				d_feature_focus_ptr->associated_geometry_property())
			>= 0)
		{
			// Focused feature is already in our topology.
			return;
		}

		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

		GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				d_feature_focus_ptr->associated_reconstruction_geometry()->geometry(),
				colour,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

		d_focused_feature_layer_ptr->add_rendered_geometry(rendered_geometry);

		// Get the start and end points of the focused feature's geometry.
		// Since the geometry is in focus but has not been added to the topology
		// there is no information about whether to reverse its points so we don't.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				focus_feature_end_points =
					GPlatesUtils::GeometryUtil::get_geometry_end_points(
							*d_feature_focus_ptr->associated_reconstruction_geometry()->geometry());

		// draw the focused end_points
		draw_focused_geometry_end_points(
				focus_feature_end_points.first,
				focus_feature_end_points.second);
	}
}


void
GPlatesGui::TopologyTools::draw_focused_geometry_end_points(
		const GPlatesMaths::PointOnSphere &start_point,
		const GPlatesMaths::PointOnSphere &end_point)
{
	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry start_point_rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			start_point,
			colour,
			GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

	// Add to layer.
	d_focused_feature_layer_ptr->add_rendered_geometry(start_point_rendered_geometry);

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry end_point_rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			end_point,
			colour,
			GPlatesViewOperations::GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

	// Add to layer.
	d_focused_feature_layer_ptr->add_rendered_geometry(end_point_rendered_geometry);
}


void
GPlatesGui::TopologyTools::draw_segments()
{
	d_segments_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_grey();

	// Iterate over the sections and draw the subsegment geometry of each.
	section_info_seq_type::const_iterator section_info_iter = d_section_info_seq.begin();
	section_info_seq_type::const_iterator section_info_end = d_section_info_seq.end();
	for ( ; section_info_iter != section_info_end; ++section_info_iter)
	{
		if (section_info_iter->d_subsegment_geometry_unreversed)
		{
			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					*section_info_iter->d_subsegment_geometry_unreversed,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

			// Add to layer.
			d_segments_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
}


void
GPlatesGui::TopologyTools::draw_end_points()
{
	d_end_points_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &end_points_colour = GPlatesGui::Colour::get_grey();

	// Iterate over the sections and draw the start and end point of each segment.
	section_info_seq_type::const_iterator section_info_iter = d_section_info_seq.begin();
	section_info_seq_type::const_iterator section_info_end = d_section_info_seq.end();
	for ( ; section_info_iter != section_info_end; ++section_info_iter)
	{
		if (section_info_iter->d_section_start_point)
		{
			const GPlatesMaths::PointOnSphere &segment_start =
					*section_info_iter->d_section_start_point;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry start_point_rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					segment_start,
					end_points_colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(start_point_rendered_geometry);
		}

		if (section_info_iter->d_section_end_point)
		{
			const GPlatesMaths::PointOnSphere &segment_end =
					*section_info_iter->d_section_end_point;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry end_point_rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					segment_end,
					end_points_colour,
					GPlatesViewOperations::GeometryOperationParameters::REGULAR_POINT_SIZE_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(end_point_rendered_geometry);
		}
	}
}


void
GPlatesGui::TopologyTools::draw_intersection_points()
{
	d_intersection_points_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &intersection_points_colour = GPlatesGui::Colour::get_grey();

	// Iterate over the sections and draw the start intersection of each segment.
	// Since the start intersection of one segment is the same as the end intersection
	// of the previous segment we do not need to draw the end intersection points.
	section_info_seq_type::const_iterator section_info_iter = d_section_info_seq.begin();
	section_info_seq_type::const_iterator section_info_end = d_section_info_seq.end();
	for ( ; section_info_iter != section_info_end; ++section_info_iter)
	{
		if (!section_info_iter->d_intersection_point_with_prev)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere &segment_start_intersection =
				*section_info_iter->d_intersection_point_with_prev;

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry intersection_point_rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
				segment_start_intersection,
				intersection_points_colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT);

		// Add to layer.
		d_intersection_points_layer_ptr->add_rendered_geometry(intersection_point_rendered_geometry);
	}
}


void
GPlatesGui::TopologyTools::draw_click_point()
{
	d_click_point_layer_ptr->clear_rendered_geometries();

	// Make sure click point has been set - it should be.
	if (!d_click_point.d_reconstructed_click_point)
	{
		return;
	}

	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_olive();

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			*d_click_point.d_reconstructed_click_point,
			colour,
			GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT);

	d_click_point_layer_ptr->add_rendered_geometry(rendered_geometry);
}


void
GPlatesGui::TopologyTools::draw_click_points()
{
	d_click_points_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

	// Iterate over the sections and the reconstructed click points in each section that has one.
	section_info_seq_type::const_iterator section_info_iter = d_section_info_seq.begin();
	section_info_seq_type::const_iterator section_info_end = d_section_info_seq.end();
	for ( ; section_info_iter != section_info_end; ++section_info_iter)
	{
		if (section_info_iter->d_reconstructed_click_point)
		{
			const GPlatesMaths::PointOnSphere &reconstructed_click_point =
					*section_info_iter->d_reconstructed_click_point;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry click_point_rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					reconstructed_click_point,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

			// Add to layer.
			d_click_points_layer_ptr->add_rendered_geometry(click_point_rendered_geometry);
		}
	}
}


void
GPlatesGui::TopologyTools::update_and_redraw_topology(
		const section_info_seq_type::size_type first_modified_section_index,
		const section_info_seq_type::size_type num_sections)
{
	section_info_seq_type::size_type section_count;
	section_info_seq_type::size_type section_index;

	//
	// First iterate through the modified sections and reconstruct them so that
	// we have up-to-date reconstructed section geometries.
	//

	// We need to reconstruct the modified sections plus the two sections next to the
	// start and end section of the modified range of sections.
	// Actually, for the extra two sections, we really only need to reset the
	// subsegment geometry to the full section geometry in preparation for intersections
	// but it's easier just to reconstruct them which also takes care of that.
	const section_info_seq_type::size_type start_reconstruct_index =
			get_prev_section_index(first_modified_section_index);

	// The most number of reconstructions we can have is the number of sections.
	section_info_seq_type::size_type num_reconstructions = num_sections + 2;
	if (num_reconstructions >= d_section_info_seq.size())
	{
		num_reconstructions = d_section_info_seq.size();
	}

	// Iterate over the sections and reconstruct them.
	for (section_count = 0, section_index = start_reconstruct_index;
		section_count < num_reconstructions;
		++section_count, ++section_index)
	{
		// Test for index wrap around.
		if (section_index == d_section_info_seq.size())
		{
			section_index = 0;
		}

		SectionInfo &section_info = d_section_info_seq[section_index];

		// Clear all data members in the section (except the table row).
		// This prepares the section for reconstruction *and* intersection.
		section_info.reset();

		// Initialise the data members that deal with reconstructions.
		// The remaining will be taken care of if there are intersections
		// with neighbouring sections.
		section_info.reconstruct_section_info_from_table_row(
				d_reconstruct_ptr->get_current_reconstruction());
	}

	//
	// Next iterate through the potential intersections that can affect the
	// modified sections.
	//

	// We need to recalculate the intersections for the two sections next to the
	// start and end section of the modified range of sections.
	// This is because the intersections may have changed and hence the subsegments
	// of these two boundary sections need to be recalculated even though those
	// two sections were not modified.
	// This sounds like it should be '+2' intersections but it's '+3' because
	// 'n' sections have 'n+1' endpoints (and hence potential intersections).
	const section_info_seq_type::size_type start_intersection_index =
			get_prev_section_index(first_modified_section_index);

	// Are we processing all intersections in the topology (polygon).
	bool processing_all_intersections = false;

	// The most number of intersections we can have is the number of sections since
	// the sections form a cycle (polygon).
	section_info_seq_type::size_type num_intersections = num_sections + 3;
	if (num_intersections >= d_section_info_seq.size())
	{
		num_intersections = d_section_info_seq.size();
		processing_all_intersections = true;
	}

	// Iterate over the intersections and process them.
	for (section_count = 0, section_index = start_intersection_index;
		section_count < num_intersections;
		++section_count, ++section_index)
	{
		// Test for index wrap around.
		if (section_index == d_section_info_seq.size())
		{
			section_index = 0;
		}

		// The convention is to process the intersection at the start of a section.
		// We could have chosen the end (would've also been fine) - but we chose the start.
		// This potentially intersects the start of 'section_index' and the end
		// of 'section_index - 1'.

		const section_info_seq_type::size_type prev_section_index =
				get_prev_section_index(section_index);
		const section_info_seq_type::size_type next_section_index = section_index;

		// If we are processing all intersections in the topology then none
		// of the sections have already been clipped and they'll all need to be.
		//
		// If we are *not* processing all intersections in the topology then
		// there will be two sections that hang off the range of intersections
		// that do not need to be clipped - these are the previous section of
		// the first intersection and the next section of the last intersection.
		bool prev_section_already_clipped = false;
		bool next_section_already_clipped = false;
		if (!processing_all_intersections)
		{
			prev_section_already_clipped = (section_count == 0);
			next_section_already_clipped = (section_count == num_intersections - 1);
		}

		process_intersection(
				prev_section_index,
				next_section_index,
				prev_section_already_clipped,
				next_section_already_clipped);
	}

	// Now that we've updated all the topology subsegments we can
	// create the full set of topology vertices for display.
	update_topology_vertices();

	draw_all_layers();

	// Set the num_sections in the TopologyToolsWidget
	d_topology_tools_widget_ptr->display_number_of_sections(
		d_topology_sections_container_ptr->size() );
}


void
GPlatesGui::TopologyTools::process_intersection(
		const section_info_seq_type::size_type first_section_index,
		const section_info_seq_type::size_type second_section_index,
		bool first_section_already_clipped,
		bool second_section_already_clipped)
{
	// Make sure the second section follows the first section.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			second_section_index == get_next_section_index(first_section_index),
			GPLATES_ASSERTION_SOURCE);

	// If there's only one section we don't want to intersect it with itself.
	if (first_section_index == second_section_index)
	{
		return;
	}

	SectionInfo &first_section_info = d_section_info_seq[first_section_index];
	SectionInfo &second_section_info = d_section_info_seq[second_section_index];

	// If either section has no geometry then return since we cannot intersect.
	if (!first_section_info.d_section_geometry_unreversed ||
		!second_section_info.d_section_geometry_unreversed)
	{
		// No need for warning message - one was already provided when the section was added.
		return;
	}

	// If one of the sections has already been intersected (at both ends) then its
	// subsegment geometry is already clipped. The other section still needs to
	// do intersection tests and it needs to test against the full, unclipped
	// section geometry of its opposing section (otherwise it might not detect
	// the intersection - because it just barely touches the already clipped subsegment
	// of the opposing section). Remember that all subsegment geometry starts out as
	// the full, unclipped section geometry and gradually gets cut down by the
	// intersections until it accurately represents the topology polygon boundary.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_geometry =
			first_section_already_clipped
					? *first_section_info.d_section_geometry_unreversed
					: *first_section_info.d_subsegment_geometry_unreversed;
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_geometry =
			second_section_already_clipped
					? *second_section_info.d_section_geometry_unreversed
					: *second_section_info.d_subsegment_geometry_unreversed;

	// Attempt to get the two geometries as polylines if they are intersectable -
	// that is if neither geometry is a point or a multi-point.
	boost::optional<
		std::pair<
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > polylines_for_intersection =
				GPlatesAppLogic::TopologyInternalUtils::get_polylines_for_intersection(
					first_section_geometry,
					second_section_geometry);

	// If the geometries are not intersectable then just return since we cannot intersect.
	if (!polylines_for_intersection)
	{
		// This is not an error or warning condition - this can happen when
		// one or both of the sections is a point.
		return;
	}

	// If either section has no click point then ignore the intersection and
	// provide a warning to the user.
	if (!first_section_info.d_reconstructed_click_point ||
		!second_section_info.d_reconstructed_click_point)
	{
		if (!first_section_info.d_reconstructed_click_point)
		{
			qDebug() << "WARNING: ======================================";
			qDebug() << "WARNING: no click point for feature at Topology Section Table index:" << first_section_index;
			qDebug() << "WARNING: Unable to process intersections of this feature with neighbors without a click point.";
			qDebug() << "WARNING: If this line intersects others, then";
			qDebug() << "WARNING: use shift-click to give it a new click point.";
		}
		if (!second_section_info.d_reconstructed_click_point)
		{
			qDebug() << "WARNING: ======================================";
			qDebug() << "WARNING: no click point for feature at Topology Section Table index:" << second_section_index;
			qDebug() << "WARNING: Unable to process intersections of this feature with neighbors without a click point.";
			qDebug() << "WARNING: If this line intersects others, then";
			qDebug() << "WARNING: use shift-click to give it a new click point.";
		}

		return;
	}

	// Extract the subsegment polylines.
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &
			first_section_polyline_unreversed = polylines_for_intersection->first;
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &
			second_section_polyline_unreversed = polylines_for_intersection->second;

	// Intersect the first section with the second section and find the intersected
	// segments that are closest to the respective rotated click points.
	boost::tuple<
			boost::optional<GPlatesMaths::PointOnSphere>/*intersection point*/, 
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type/*first_section_closest_segment*/,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type/*second_section_closest_segment*/>
		closest_segments =
			GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections(
					// Potentially clipped subsegment polyline from first section...
					first_section_polyline_unreversed,
					*first_section_info.d_reconstructed_click_point,
					// Potentially clipped subsegment polyline from second section...
					second_section_polyline_unreversed,
					*second_section_info.d_reconstructed_click_point);

	// If a respective section has already been intersected (at both ends) then we don't
	// need to initialise it. If we were to initialise it then we might overwrite the
	// already clipped subsegment with a partially clipped subsegment (say at one end only)
	// but the other end would never get clipped - which is the main reason for this
	// variable (that is so we can minimise the number of intersection calculations needed
	// when a subset of sections are modified by the user or inserted, removed).
	if (!first_section_already_clipped)
	{
		// Was there an intersection?
		first_section_info.d_intersection_point_with_next = boost::get<0>(closest_segments);
		// Copy the possibly clipped segment back onto itself - this shortens
		// the subsegment for this intersection - another intersection (at the other end
		// of the segment) is possibly needed later on to shorten it some more.
		first_section_info.d_subsegment_geometry_unreversed = boost::get<1>(closest_segments);
	}
	if (!second_section_already_clipped)
	{
		// Was there an intersection?
		second_section_info.d_intersection_point_with_prev = boost::get<0>(closest_segments);
		// Copy the possibly clipped segment back onto itself - this shortens
		// the subsegment for this intersection - another intersection (at the other end
		// of the segment) is possibly needed later on to shorten it some more.
		second_section_info.d_subsegment_geometry_unreversed = boost::get<2>(closest_segments);
	}
}


GPlatesGui::TopologyTools::section_info_seq_type::size_type
GPlatesGui::TopologyTools::get_prev_section_index(
		section_info_seq_type::size_type section_index) const
{
	section_info_seq_type::size_type prev_section_index = section_index;

	if (prev_section_index == 0)
	{
		prev_section_index = d_section_info_seq.size();
	}
	--prev_section_index;

	return prev_section_index;
}


GPlatesGui::TopologyTools::section_info_seq_type::size_type
GPlatesGui::TopologyTools::get_next_section_index(
		section_info_seq_type::size_type section_index) const
{
	section_info_seq_type::size_type next_section_index = section_index;

	++next_section_index;
	if (next_section_index == d_section_info_seq.size())
	{
		next_section_index = 0;
	}

	return next_section_index;
}


void
GPlatesGui::TopologyTools::handle_create_new_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// Set the d_topology_feature_ref to the newly created feature 
	d_topology_feature_ref = feature_ref;
}


namespace
{
	/**
	 * Removes the first property named @a property_name from the feature if there currently is one.
	 *
	 * Returns true if property was found and removed.
	 */
	bool
	remove_boundary_property_from_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const GPlatesModel::PropertyName &property_name)
	{
		GPlatesModel::FeatureHandle::children_iterator iter = feature_ref->children_begin();
		GPlatesModel::FeatureHandle::children_iterator end = feature_ref->children_end();
		// loop over properties
		for ( ; iter != end; ++iter) 
		{
			// double check for validity and nullness
			if(! iter.is_valid() ){ continue; }
			if( *iter == NULL )   { continue; }  
			// FIXME: previous edits to the feature leave property pointers NULL

			// passed all checks, make the name and test
			GPlatesModel::PropertyName test_name = (*iter)->property_name();

			if ( test_name == property_name )
			{
				// Delete the old boundary 
				GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
				feature_ref->remove_child(iter, transaction);
				transaction.commit();
				// FIXME: this seems to create NULL pointers in the properties collection
				// see FIXME note above to check for NULL? 
				// Or is this to be expected?
				return true;
			}
		} // loop over properties

		return false;
	}


	/**
	 * Create the boundary property value from the topological sections.
	 */
	GPlatesModel::PropertyValue::non_null_ptr_type
	create_boundary_property(
			const std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &topological_sections,
			const GPlatesModel::FeatureHandle::weak_ref &topology_feature_ref)
	{
		//
		// Create the "boundary" property for the topology feature.
		//

		// create the TopologicalPolygon
		GPlatesModel::PropertyValue::non_null_ptr_type topo_poly_value =
			GPlatesPropertyValues::GpmlTopologicalPolygon::create(topological_sections);

		static const GPlatesPropertyValues::TemplateTypeParameterType topo_poly_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");

		// Create the ConstantValue
		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
			GPlatesPropertyValues::GpmlConstantValue::create(topo_poly_value, topo_poly_type);

		// Get the time period for the feature's validTime prop
		// FIXME: (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");

		GPlatesPropertyValues::GmlTimePeriod *time_period;

		GPlatesFeatureVisitors::get_property_value(
			topology_feature_ref, valid_time_property_name, time_period);

		GPlatesUtils::non_null_intrusive_ptr<
			GPlatesPropertyValues::GmlTimePeriod, 
			GPlatesUtils::NullIntrusivePointerHandler> ttpp(
				time_period,
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
		return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(time_windows, topo_poly_type);
	}
}


void
GPlatesGui::TopologyTools::convert_topology_to_boundary_feature_property(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	// double check for non existant features
	if ( ! feature_ref.is_valid() ) { return; }

	// We're interested in the "boundary" property.
	static const GPlatesModel::PropertyName boundary_prop_name = 
			GPlatesModel::PropertyName::create_gpml("boundary");

	//
	// Remove the "boundary" property from the topology feature if there currently is one.
	// After this we'll add a new "boundary" property.
	//
	remove_boundary_property_from_feature(feature_ref, boundary_prop_name);

	//
	// Iterate over our sections and create a vector of GpmlTopologicalSection objects.
	//
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
			topological_sections;
	create_topological_sections(topological_sections);

	//
	// Create the boundary property value from the topological sections.
	//
	GPlatesModel::PropertyValue::non_null_ptr_type boundary_property_value =
			create_boundary_property(topological_sections, feature_ref);

	//
	// Add the gpml:boundary property to the topology feature.
	//
	
	GPlatesModel::ModelUtils::append_property_value_to_feature(
		boundary_property_value,
		boundary_prop_name,
		feature_ref);

	// Set the ball rolling again ...
	d_reconstruct_ptr->reconstruct(); 
}


void
GPlatesGui::TopologyTools::create_topological_sections(
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &topological_sections)
{
	//
	// Iterate over our sections and create a vector of GpmlTopologicalSection objects.
	//

	for (section_info_seq_type::size_type section_index = 0;
		section_index < d_section_info_seq.size();
		++section_index)
	{
		const SectionInfo &section_info = d_section_info_seq[section_index];

		// Is there an intersection with the previous section?
		boost::optional<GPlatesModel::FeatureHandle::children_iterator> prev_intersection;
		if (section_info.d_intersection_point_with_prev)
		{
			// Get the previous section info.
			const SectionInfo &prev_section_info =
					d_section_info_seq[get_prev_section_index(section_index)];

			// Set the previous intersecting geometry.
			prev_intersection = prev_section_info.d_table_row.get_geometry_property();
		}

		// Is there an intersection with the next section?
		boost::optional<GPlatesModel::FeatureHandle::children_iterator> next_intersection;
		if (section_info.d_intersection_point_with_next)
		{
			// Get the next section info.
			const SectionInfo &next_section_info =
					d_section_info_seq[get_next_section_index(section_index)];

			// Set the next intersecting geometry.
			next_intersection = next_section_info.d_table_row.get_geometry_property();
		}

		// Create the GpmlTopologicalSection property value for the current section.
		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
				topological_section =
						GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_section(
								section_info.d_table_row.get_geometry_property(),
								section_info.d_table_row.get_reverse(),
								prev_intersection,
								next_intersection,
								section_info.d_table_row.get_present_day_click_point());
		if (!topological_section)
		{
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: TopologyTools::create_topological_sections():";
			qDebug() << "ERROR: failed to create GpmlTopologicalSection!";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";
			continue;
		}

		// Add the GpmlTopologicalSection pointer to the working vector
		topological_sections.push_back(*topological_section);
	}
}


void
GPlatesGui::TopologyTools::show_numbers()
{
	qDebug() << "############################################################"; 
	qDebug() << "TopologyTools::show_numbers: "; 
	qDebug() << "d_topology_sections_container_ptr->size() = " << d_topology_sections_container_ptr->size(); 

	//
	// Show details about d_feature_focus_ptr
	//
	if ( d_feature_focus_ptr->is_valid() ) 
	{
		qDebug() << "d_feature_focus_ptr = " << GPlatesUtils::make_qstring_from_icu_string( 
			d_feature_focus_ptr->focused_feature()->handle_data().feature_id().get() );

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
		qDebug() << "d_topology_feature_ref = " << GPlatesUtils::make_qstring_from_icu_string( d_topology_feature_ref->handle_data().feature_id().get() );
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
GPlatesGui::TopologyTools::update_topology_vertices()
{
	// FIXME: Only handles the unbroken line and single-ring cases.

	d_topology_vertices.clear();

	// Iterate over the subsegments and append their points to the sequence
	// of topology vertices.
	section_info_seq_type::const_iterator section_info_iter = d_section_info_seq.begin();
	section_info_seq_type::const_iterator section_info_end = d_section_info_seq.end();
	for ( ; section_info_iter != section_info_end; ++section_info_iter)
	{
		const SectionInfo &section_info = *section_info_iter;

		// If there's no geometry then continue to the next section.
		if (!section_info.d_subsegment_geometry_unreversed)
		{
			continue;
		}

		// Get the vertices from the possibly clipped section geometry
		// and add them to the list of topology vertices.
		GPlatesUtils::GeometryUtil::get_geometry_points(
				*section_info.d_subsegment_geometry_unreversed.get(),
				d_topology_vertices,
				section_info.d_table_row.get_reverse());
	}

	// There's no guarantee that adjacent points in the table aren't identical.
	const std::vector<GPlatesMaths::PointOnSphere>::size_type num_topology_points =
			GPlatesMaths::count_distinct_adjacent_points(d_topology_vertices);

	// FIXME: I think... we need some way to add data() to the 'header' QTWIs, so that
	// we can immediately discover which bits are supposed to be polygon exteriors etc.
	// Then the function calculate_label_for_item could do all our 'tagging' of
	// geometry parts, and -this- function wouldn't need to duplicate the logic.
	// FIXME 2: We should have a 'try {  } catch {  }' block to catch any exceptions
	// thrown during the instantiation of the geometries.
	d_topology_geometry_opt_ptr = boost::none;

	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	if (num_topology_points == 0) 
	{
		validity = GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
		d_topology_geometry_opt_ptr = boost::none;
	} 
	else if (num_topology_points == 1) 
	{
		d_topology_geometry_opt_ptr = 
			GPlatesUtils::create_point_on_sphere(d_topology_vertices, validity);
	} 
	else if (num_topology_points == 2) 
	{
		d_topology_geometry_opt_ptr = 
			GPlatesUtils::create_polyline_on_sphere(d_topology_vertices, validity);
	} 
	else if (num_topology_points == 3 && d_topology_vertices.front() == d_topology_vertices.back()) 
	{
		d_topology_geometry_opt_ptr = 
			GPlatesUtils::create_polyline_on_sphere(d_topology_vertices, validity);
	} 
	else 
	{
		d_topology_geometry_opt_ptr = 
			GPlatesUtils::create_polygon_on_sphere(d_topology_vertices, validity);
	}
}


bool
GPlatesGui::TopologyTools::should_reverse_section(
		const section_info_seq_type::size_type section_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			section_index < d_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	// If there's less than two sections in topology then we have no
	// way to determine if the section should be reversed.
	if (d_section_info_seq.size() < 2)
	{
		return false;
	}

	const SectionInfo &section_info = d_section_info_seq[section_index];

	// Get the previous section.
	const SectionInfo &prev_section_info =
			d_section_info_seq[get_prev_section_index(section_index)];

	// Get the next section.
	const SectionInfo &next_section_info =
			d_section_info_seq[get_next_section_index(section_index)];

	GPlatesMaths::real_t arc_distance = 0;
	GPlatesMaths::real_t reversed_arc_distance = 0;

	if (!section_info.d_subsegment_geometry_unreversed)
	{
		// There are no vertices in the current subsegment so nothing to do.
		return false;
	}

	// Get the start and end points of the current subsegment.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			current_subsegment_geometry_end_points =
				GPlatesUtils::GeometryUtil::get_geometry_end_points(
						*section_info.d_subsegment_geometry_unreversed.get(),
						section_info.d_table_row.get_reverse());

	const GPlatesMaths::PointOnSphere &curr_section_head =
			current_subsegment_geometry_end_points.first;
	const GPlatesMaths::PointOnSphere &curr_section_tail =
			current_subsegment_geometry_end_points.second;

	// If there are vertices in the previous section.
	if (prev_section_info.d_subsegment_geometry_unreversed)
	{
		// Get the start and end points of the previous subsegment.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				prev_subsegment_geometry_end_points =
					GPlatesUtils::GeometryUtil::get_geometry_end_points(
							*prev_section_info.d_subsegment_geometry_unreversed.get(),
							prev_section_info.d_table_row.get_reverse());
		const GPlatesMaths::PointOnSphere &prev_section_tail =
				prev_subsegment_geometry_end_points.second;

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
	if (next_section_info.d_subsegment_geometry_unreversed)
	{
		// Get the start and end points of the previous subsegment.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				next_subsegment_geometry_end_points =
					GPlatesUtils::GeometryUtil::get_geometry_end_points(
							*next_section_info.d_subsegment_geometry_unreversed.get(),
							next_section_info.d_table_row.get_reverse());
		const GPlatesMaths::PointOnSphere &next_section_head =
				next_subsegment_geometry_end_points.first;

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


void
GPlatesGui::TopologyTools::ClickPoint::update_reconstructed_click_point(
		GPlatesModel::ReconstructionTree &reconstruction_tree)
{
	//
	// Compute a new reconstructed click point from the present day click point.
	//

	if (!d_present_day_click_point)
	{
		d_reconstructed_click_point = boost::none;
		return;
	}

	// Get the rotation used to rotate the present-day click point.
	const boost::optional<GPlatesMaths::FiniteRotation> fwd_rot =
			GPlatesAppLogic::TopologyInternalUtils::get_finite_rotation(
					d_clicked_feature_ref,
					reconstruction_tree);
	if (!fwd_rot)
	{
		// NOTE: no rotation so just set the rotated click point to
		// the present day click point.
		// FIXME: Perhaps we should be setting it to boost::none ?
		d_reconstructed_click_point = d_present_day_click_point;
		return;
	}

	// Reconstruct the point. 
	d_reconstructed_click_point = *fwd_rot * *d_present_day_click_point;
}


void
GPlatesGui::TopologyTools::ClickPoint::calc_present_day_click_point(
		GPlatesModel::ReconstructionTree &reconstruction_tree)
{
	//
	// Compute the present day user click point from the reconstructed click point.
	//

	if (!d_reconstructed_click_point)
	{
		d_present_day_click_point = boost::none;
		return;
	}

	// Get the rotation used to rotate the present-day click point.
	const boost::optional<GPlatesMaths::FiniteRotation> fwd_rot =
			GPlatesAppLogic::TopologyInternalUtils::get_finite_rotation(
					d_clicked_feature_ref,
					reconstruction_tree);
	if (!fwd_rot)
	{
		// NOTE: no rotation so just set the present day click point to
		// the rotated click point.
		// FIXME: Perhaps we should be setting it to boost::none ?
		d_present_day_click_point = d_reconstructed_click_point;
		return;
	}

	// Get the reverse rotation.
	const GPlatesMaths::FiniteRotation rev_rot = GPlatesMaths::get_reverse(*fwd_rot);

	// Un-reconstruct the point. 
	d_present_day_click_point = rev_rot * *d_reconstructed_click_point;
}


void
GPlatesGui::TopologyTools::SectionInfo::reconstruct_section_info_from_table_row(
		GPlatesModel::Reconstruction &reconstruction)
{
	// Find the RFG, in the current Reconstruction, for the current topological section.
	boost::optional<GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type> section_rfg =
			GPlatesAppLogic::TopologyInternalUtils::find_reconstructed_feature_geometry(
					d_table_row.get_geometry_property(),
					&reconstruction);

	if (!section_rfg)
	{
		qDebug() << "ERROR: ======================================";
		qDebug() << "ERROR: reconstruct_section_info_from_table_row():";
		qDebug() << "ERROR: No RFG found for feature_id =";
		qDebug() <<
			GPlatesUtils::make_qstring_from_icu_string( d_table_row.get_feature_id().get() );
		qDebug() << "ERROR: Unable to obtain feature (and its geometry, or vertices)";
		qDebug() << "ERROR: ======================================";

		// FIXME: what else to do?
		return;
	}

	// Get the geometry on sphere from the RFG.
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry_unreversed =
			(*section_rfg)->geometry();

	// The section geometry is always the whole unclipped section geometry.
	// This shouldn't change when we do neighbouring section intersection processing.
	d_section_geometry_unreversed = section_geometry_unreversed;

	// Also initially set the subsegment geometry to the whole unclipped section geometry.
	// This might get shorter if it gets clipped/intersected when we do neighbouring
	// section intersection processing.
	d_subsegment_geometry_unreversed = section_geometry_unreversed;

	// Get the start and end points of the current section's geometry.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			section_geometry_end_points =
				GPlatesUtils::GeometryUtil::get_geometry_end_points(
						*section_geometry_unreversed,
						d_table_row.get_reverse());

	// Set the section start and end points.
	d_section_start_point = section_geometry_end_points.first;
	d_section_end_point = section_geometry_end_points.second;

	// Reconstruct the click point if there is one.
	// Otherwise just use the present day click point.
	d_reconstructed_click_point = d_table_row.get_present_day_click_point();
	if (d_table_row.get_present_day_click_point())
	{
		// Get the rotation used to rotate this section's reference point.
		// NOTE: we use the section itself as the reference feature rather than the
		// feature stored in the GpmlTopologicalIntersection.
		const boost::optional<GPlatesMaths::FiniteRotation> fwd_rot =
				GPlatesAppLogic::TopologyInternalUtils::get_finite_rotation(
						d_table_row.get_feature_ref(),
						reconstruction.reconstruction_tree());

		if (fwd_rot)
		{
			d_reconstructed_click_point =
					*fwd_rot * *d_table_row.get_present_day_click_point();
		}
	}
}


void
GPlatesGui::TopologyTools::SectionInfo::reset()
{
	d_section_geometry_unreversed = boost::none;
	d_subsegment_geometry_unreversed = boost::none;
	d_section_start_point = boost::none;
	d_section_end_point = boost::none;
	d_reconstructed_click_point = boost::none;
	d_intersection_point_with_prev = boost::none;
	d_intersection_point_with_next = boost::none;
}
