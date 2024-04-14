/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
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
#include <cstddef> // For std::size_t
#include <iterator>
#include <limits>
#include <map>

#include <boost/foreach.hpp>
#include <boost/integer_traits.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <QtDebug>
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

#include "ChooseCanvasToolUndoCommand.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/ReconstructedFeatureGeometryFinder.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedTopologicalGeometry.h"
#include "app-logic/TopologyInternalUtils.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/GeometryTypeFinder.h"
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
#include "maths/Real.h"

#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "presentation/ReconstructionGeometryRenderer.h"
#include "presentation/ViewState.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/StructuralType.h"

#include "qt-widgets/CreateFeatureDialog.h"
#include "qt-widgets/ExportCoordinatesDialog.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/SearchResultsDockWidget.h"
#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/TopologyToolsWidget.h"
#include "qt-widgets/ViewportWindow.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"

namespace
{
	//! Returns the wrapped previous index before @@a index in @@a sequence.
	template <class VectorSequenceType>
	typename VectorSequenceType::size_type
	get_prev_index(
			typename VectorSequenceType::size_type sequence_index,
			const VectorSequenceType &sequence)
	{
		typename VectorSequenceType::size_type prev_sequence_index = sequence_index;

		if (prev_sequence_index == 0)
		{
			prev_sequence_index = sequence.size();
		}
		--prev_sequence_index;

		return prev_sequence_index;
	}


	//! Returns the wrapped next index after @@a index in @@a sequence.
	template <class VectorSequenceType>
	typename VectorSequenceType::size_type
	get_next_index(
			typename VectorSequenceType::size_type sequence_index,
			const VectorSequenceType &sequence)
	{
		typename VectorSequenceType::size_type next_sequence_index = sequence_index;

		++next_sequence_index;
		if (next_sequence_index >= sequence.size())
		{
			next_sequence_index = 0;
		}

		return next_sequence_index;
	}

	/**
	 * Create a topological line property value from the topological sections.
	 */
	GPlatesModel::PropertyValue::non_null_ptr_type
	create_topological_line_property_value(
			const std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &topological_sections)
	{
		// Create the property value.
		return GPlatesPropertyValues::GpmlTopologicalLine::create(
				topological_sections.begin(),
				topological_sections.end());
	}

	/**
	 * Create a topological polygon property value from the topological sections.
	 */
	GPlatesModel::PropertyValue::non_null_ptr_type
	create_topological_polygon_property_value(
			const std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &topological_sections)
	{
		// Create the property value.
		return GPlatesPropertyValues::GpmlTopologicalPolygon::create(
				topological_sections.begin(),
				topological_sections.end());
	}

	/**
	 * Create a topological network property value from the topological boundary sections and interior geometries.
	 */
	GPlatesModel::PropertyValue::non_null_ptr_type
	create_topological_network_property_value(
			const std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &topological_boundary_sections,
			const std::vector<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> &topological_interiors)
	{
		if (topological_interiors.empty())
		{
			// Create the property value.
			return GPlatesPropertyValues::GpmlTopologicalNetwork::create(
					topological_boundary_sections.begin(),
					topological_boundary_sections.end());
		}

		return GPlatesPropertyValues::GpmlTopologicalNetwork::create(
				topological_boundary_sections.begin(),
				topological_boundary_sections.end(),
				topological_interiors.begin(),
				topological_interiors.end());
	}
}

GPlatesGui::TopologyTools::TopologyTools(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window):
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_rendered_geometry_parameters(view_state.get_rendered_geometry_parameters()),
	d_feature_focus_ptr(&view_state.get_feature_focus()),
	d_application_state_ptr(&view_state.get_application_state()),
	d_viewport_window_ptr(&viewport_window)
{
	// set up the drawing
	create_child_rendered_layers();

	// Set pointer to TopologySectionsContainer
	d_boundary_sections_container_ptr = &(view_state.get_topology_boundary_sections_container());
	d_interior_sections_container_ptr = &(view_state.get_topology_interior_sections_container());

	// set the internal state flags
	d_is_active = false;
}


void
GPlatesGui::TopologyTools::activate_build_mode(
		GPlatesAppLogic::TopologyGeometry::Type topology_geometry_type)
{
	// First perform activation tasks common to both build and edit tools.
	// A time period of boost::none implies distant past to distant future.
	activate(topology_geometry_type, boost::none);

	// unset focus 
	unset_focus();

	// synchronize the tools, container, widget, and table; default use the boundary
	// note: will call handle_sections_combobox_index_changed() , which  will call update_and_redraw()
	synchronize_seq_num( 0 );
}


void
GPlatesGui::TopologyTools::activate_edit_mode(
		GPlatesAppLogic::TopologyGeometry::Type topology_geometry_type,
		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> topology_time_period)
{
	// First perform activation tasks common to both build and edit tools.
	activate(topology_geometry_type, topology_time_period);

	// Load the topology into the Topology Sections Table 
	initialise_topological_sections_from_edit_topology_focused_feature();

	// unset focus 
	unset_focus();

	// Flip the ViewportWindow to the Topology Sections Table
	d_viewport_window_ptr->search_results_dock_widget().choose_topology_sections_table();

	// Flip the TopologyToolsWidget to the Toplogy Tab
	d_topology_tools_widget_ptr->choose_topology_tab();

	// update the table with the boundary
	d_boundary_sections_container_ptr->update_table_from_container();

	// synchronize the tools, container, widget, and table; default use the boundary
	// note: will call handle_sections_combobox_index_changed() , which  will call update_and_redraw()
	synchronize_seq_num( 0 );
}


void
GPlatesGui::TopologyTools::activate(
		GPlatesAppLogic::TopologyGeometry::Type topology_geometry_type,
		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> topology_time_period)
{
	//
	// Set the topology geometry type, time period and active state first.
	//

	// Set the time period of the topology being built or edited.
	d_topology_time_period = topology_time_period;

	// Set the topology geometry type being built or edited.
	d_topology_geometry_type = topology_geometry_type;

	// set the widget state
	d_is_active = true;

	//
	// Connect to signals
	//

	// 
	// Connect to signals from Topology Sections Container
	connect_to_boundary_sections_container_signals( true );
	connect_to_interior_sections_container_signals( true );

	// Connect to focus signals from Feature Focus
	connect_to_focus_signals( true );

	// Connect to recon time changes
	QObject::connect(
		d_application_state_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
		this,
		SLOT(handle_reconstruction()));

	// Re-draw when the render geometry parameters change.
	QObject::connect(
			&d_rendered_geometry_parameters,
			SIGNAL(parameters_changed(GPlatesViewOperations::RenderedGeometryParameters &)),
			this,
			SLOT(draw_all_layers()));

	// Set the pointer to the Topology Tools Widget 
	d_topology_tools_widget_ptr = 
		&( d_viewport_window_ptr->task_panel_ptr()->topology_tools_widget() );

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
}


void
GPlatesGui::TopologyTools::deactivate()
{
	// Unset any focused feature
	if ( d_feature_focus_ptr->is_valid() )
	{
		// unset focus 
		unset_focus();
	}

	// Flip the ViewportWindow to the Clicked Geometry Table
	d_viewport_window_ptr->search_results_dock_widget().choose_clicked_geometry_table();

	// Clear out all old data.
	// NOTE: We should be connected to the topology sections container
	//       signals for this to work properly.
	clear_widgets_and_data();

	//
	// Disconnect signals last.
	//

	// Disconnect from focus signals from Feature Focus.
	connect_to_focus_signals( false );

	// Disconnect from signals from Topology Sections Container.
	connect_to_boundary_sections_container_signals( false );
	connect_to_interior_sections_container_signals( false );

	// Disconnect to recon time changes.
	QObject::disconnect(
		d_application_state_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
		this,
		SLOT(handle_reconstruction()));

	// Disconnect from render geometry parameter changes.
	QObject::disconnect(
			&d_rendered_geometry_parameters,
			SIGNAL(parameters_changed(GPlatesViewOperations::RenderedGeometryParameters &)),
			this,
			SLOT(draw_all_layers()));

	// Reset internal state - the very last thing we should do.
	d_is_active = false;
	d_topology_geometry_type = boost::none;
	d_topology_time_period = boost::none;
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesGui::TopologyTools::create_topological_geometry_property()
{
	// Iterate over all sections and create vectors of GpmlTopologicalSection objects.
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> topological_sections;

	// Create the GpmlTopologicalSection objects.
	create_topological_sections(topological_sections);

	// Make sure we have enough topological sections for the topology geometry type.
	if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
	{
		// Need at least one topological section for a line topology.
		if (topological_sections.size() >= 1)
		{
			return create_topological_line_property_value(topological_sections);
		}
	}
	else if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY)
	{
		// Need at least one topological section for a boundary topology.
		// Only need one section because it could be a static polygon (which is already a boundary) or
		// it could be a line section that should be treated like a polygon (ie, first/last vertex joined).
		if (topological_sections.size() >= 1)
		{
			return create_topological_polygon_property_value(topological_sections);
		}
	}
	else if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
	{
		// Need at least one topological section for a network boundary topology.
		// Only need one section because it could be a static polygon (which is already a boundary) or
		// it could be a line section that should be treated like a polygon (ie, first/last vertex joined).
		if (topological_sections.size() >= 1)
		{
			// Create the topological interiors (if any).
			std::vector<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> topological_interiors;
			create_topological_interiors(topological_interiors);

			return create_topological_network_property_value(topological_sections, topological_interiors);
		}
	}

	return boost::none;
}


bool
GPlatesGui::TopologyTools::has_topological_sections() const
{
	return !d_boundary_section_info_seq.empty() ||
		!d_interior_section_info_seq.empty();
}


void
GPlatesGui::TopologyTools::clear_widgets_and_data()
{
	// clear the tables
	d_viewport_window_ptr->get_view_state().get_feature_table_model().clear();

	// Clear the TopologySectionsContainer.
	// NOTE: This will generate a signal that will call our 'react_cleared_FOO()'
	// method which clear out our internal section sequence and redraw.
	d_boundary_sections_container_ptr->clear();
	d_interior_sections_container_ptr->clear();
}


void
GPlatesGui::TopologyTools::connect_to_focus_signals(
		bool state)
{
	if (state) 
	{
		// Subscribe to focus events. 
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focus_changed( GPlatesGui::FeatureFocus &)),
			this,
			SLOT( set_focus( GPlatesGui::FeatureFocus &)));
	
		QObject::connect(
			d_feature_focus_ptr,
			SIGNAL( focused_feature_modified( GPlatesGui::FeatureFocus &)),
			this,
			SLOT( display_feature_focus_modified( GPlatesGui::FeatureFocus &)));
	} 
	else 
	{
		// Un-Subscribe to focus events. 
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focus_changed( GPlatesGui::FeatureFocus &)),
			this, 
			0);
	
		QObject::disconnect(
			d_feature_focus_ptr, 
			SIGNAL( focused_feature_modified( GPlatesGui::FeatureFocus &)),
			this, 
			0);
	}
}



// Boundary
void
GPlatesGui::TopologyTools::connect_to_boundary_sections_container_signals(
		bool state)
{
	if (state) 
	{
		QObject::connect(
			d_boundary_sections_container_ptr,
			SIGNAL( cleared() ),
			this,
			SLOT( react_cleared_boundary() )
		);

		QObject::connect(
			d_boundary_sections_container_ptr,
			SIGNAL( insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_boundary_sections_container_ptr,
			SIGNAL( entry_removed( GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entry_removed_boundary( GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_boundary_sections_container_ptr,
			SIGNAL( entries_inserted(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::const_iterator,
				GPlatesGui::TopologySectionsContainer::const_iterator) ),
			this,
			SLOT( react_entries_inserted_boundary(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::const_iterator,
				GPlatesGui::TopologySectionsContainer::const_iterator) )
		);

		QObject::connect(
			d_boundary_sections_container_ptr,
			SIGNAL( entry_modified( GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entry_modified_boundary( GPlatesGui::TopologySectionsContainer::size_type) )
		);
	} 
	else 
	{
		// Disconnect this receiver from all signals from TopologySectionsContainer:
		QObject::disconnect( d_boundary_sections_container_ptr, 0, this, 0);
	}
}

// Interior
void
GPlatesGui::TopologyTools::connect_to_interior_sections_container_signals(
		bool state)
{
	if (state) 
	{
		QObject::connect(
			d_interior_sections_container_ptr,
			SIGNAL( cleared() ),
			this,
			SLOT( react_cleared_interior() )
		);

		QObject::connect(
			d_interior_sections_container_ptr,
			SIGNAL( insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_insertion_point_moved(
				GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_interior_sections_container_ptr,
			SIGNAL( entry_removed( GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entry_removed_interior( GPlatesGui::TopologySectionsContainer::size_type) )
		);

		QObject::connect(
			d_interior_sections_container_ptr,
			SIGNAL( entries_inserted(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::const_iterator,
				GPlatesGui::TopologySectionsContainer::const_iterator) ),
			this,
			SLOT( react_entries_inserted_interior(
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::size_type,
				GPlatesGui::TopologySectionsContainer::const_iterator,
				GPlatesGui::TopologySectionsContainer::const_iterator) )
		);

		QObject::connect(
			d_interior_sections_container_ptr,
			SIGNAL( entry_modified( GPlatesGui::TopologySectionsContainer::size_type) ),
			this,
			SLOT( react_entry_modified_interior( GPlatesGui::TopologySectionsContainer::size_type) )
		);
	} 
	else 
	{
		// Disconnect this receiver from all signals from TopologySectionsContainer:
		QObject::disconnect( d_interior_sections_container_ptr, 0, this, 0);
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

	// the topology vertices are drawn on the bottom layer
	d_topology_geometry_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

	// the segments resulting from intersections of line data come next
	d_segments_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

	// points where line data intersects and cuts the src geometry
	d_intersection_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

	// NOTE: the boundary and interior layers are above vertices, segments, and intersections
	// to help highlight the different lists of sections 

	// boundary polygon
	d_boundary_geometry_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

	// interior segments 
	d_interior_geometry_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

#if 0
// NOTE: this was just getting confusing so, remove it.
	// head and tail points of src geometry 
	d_end_points_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);
#endif

	// insert neighbors
	d_insertion_neighbors_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

	// Put the focus layer on top
	d_focused_feature_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.

	// Activate layers
	d_topology_geometry_layer_ptr->set_active();
	d_segments_layer_ptr->set_active();
	d_intersection_points_layer_ptr->set_active();
	d_boundary_geometry_layer_ptr->set_active();
	d_interior_geometry_layer_ptr->set_active();
	d_insertion_neighbors_layer_ptr->set_active();
#if 0
	d_end_points_layer_ptr->set_active();
#endif
	d_focused_feature_layer_ptr->set_active();
}

void
GPlatesGui::TopologyTools::handle_reconstruction()
{
#ifdef DEBUG
std::cout << "GPlatesGui::TopologyTools::handle_reconstruction() " << std::endl;
#endif

	if (! d_is_active) { return; }

	// Handle the case where the user has unloaded a feature collection(s) containing
	// features (sections) referenced by the topology being built/edited.
	// We are handling this in this method since an unloaded file will trigger a reconstruction.
	handle_unloaded_sections();

	// Update all topology sections and redraw.
	update_and_redraw_topology();

	// re-display feature focus
	display_focused_feature();

	// Update topology sections tables.
	// We do this because topological section features that don't exist at the current reconstruction
	// time are displayed in a different row colour.
	d_boundary_sections_container_ptr->update_table_from_container();
	d_interior_sections_container_ptr->update_table_from_container();
}


void
GPlatesGui::TopologyTools::handle_unloaded_sections()
{
	//
	// Iterate through the section list and see if any sections have a feature id
	// that we cannot locate (because feature was unloaded).
	// If any were found then clear the entire section list - the user will
	// have to starting building/editing the topology from scratch (after they reload
	// the features).
	//
	section_info_seq_type::size_type section_index;

	// process boundary
	for (section_index = 0; section_index < d_boundary_section_info_seq.size(); ++section_index)
	{
		const SectionInfo &section_info = d_boundary_section_info_seq[section_index];

		if (!GPlatesAppLogic::TopologyInternalUtils::resolve_feature_id(
				section_info.d_table_row.get_feature_id()).is_valid())
		{
			// Clear the sections container - this will trigger our 'react_cleared_boundary' slot.
			d_boundary_sections_container_ptr->clear();
			break; // out of loop; 
		}
	}

	// process interior
	for (section_index = 0; section_index < d_interior_section_info_seq.size(); ++section_index)
	{
		const SectionInfo &section_info = d_interior_section_info_seq[section_index];

		if (!GPlatesAppLogic::TopologyInternalUtils::resolve_feature_id(
				section_info.d_table_row.get_feature_id()).is_valid())
		{
			// Clear the sections container - this will trigger our 'react_cleared_interior' slot.
			d_interior_sections_container_ptr->clear();
			break; // out of loop; 
		}
	}

	return;
}

void
GPlatesGui::TopologyTools::unset_focus()
{
	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();

	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->get_view_state().get_feature_table_model().clear();
}


void
GPlatesGui::TopologyTools::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	if (! d_is_active ) { return; }

	// If we can insert the focused feature (or remove it) then switch to the section tab
	// so the user can add/remove the focused feature to/from the topology.
	if (can_insert_focused_feature_into_topology() ||
		can_remove_focused_feature_from_topology())
	{
		// Draw focused geometry.
		draw_focused_geometry(true);
		d_topology_tools_widget_ptr->choose_section_tab();
	}
	else
	{
		// Clear focused geometry - we don't want the user to think they
		// can add it as a section to the topology..
		draw_focused_geometry(false);
		d_topology_tools_widget_ptr->choose_topology_tab();
	}

	// display this feature ; or unset focus if it is a topology
	display_focused_feature();

#if 0 // FIXME: Do we still need this ? ...
	// display_focused_feature() may have called synch, 
	// so make sure to set the tab back to sections
	d_topology_tools_widget_ptr->choose_section_tab();
#endif
}


void
GPlatesGui::TopologyTools::display_feature_focus_modified(
		GPlatesGui::FeatureFocus &/*feature_focus*/)
{
	display_focused_feature();
}


void
GPlatesGui::TopologyTools::display_focused_feature()
{
	if (! d_is_active) { return; }

	// always check weak_refs!
	if ( ! d_feature_focus_ptr->focused_feature().is_valid() ) { return; }

	// check if the feature is in the topology 
	int found_sequence_num; // which sequce of sections the feature was found in
	std::vector<int> found_indices; // indices in the sequence

	find_topological_section_indices(
			found_sequence_num,
			found_indices,
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());

	if ( found_indices.empty() ) 
	{ 
		// focused feature not found in topology ... return
		return; 
	}

	// focused feature was found in topology ... check where and highlight it 

	// Set the ViewportWindow to the Topology Sections Table
	d_viewport_window_ptr->search_results_dock_widget().choose_topology_sections_table();

	// Pretend we clicked in the table row corresponding to the first occurrence
	// of the focused feature geometry in the topology sections lists
	// (note that the geometry can occur multiple times in the topology).
 	const TopologySectionsContainer::size_type container_section_index = 
			static_cast<TopologySectionsContainer::size_type>( found_indices[0]);

	// check for synchronize 
	if ( found_sequence_num != d_topology_tools_widget_ptr->get_sections_combobox_index() )
	{
		// synchronize the tools, container, widget, and table
		synchronize_seq_num( found_sequence_num );
	}
	// else, just continue 

	// Connection between the focus and the Table via the containers
	if ( found_sequence_num == 0)
	{
		d_boundary_sections_container_ptr->set_focus_feature_at_index( container_section_index);
	}
	else if ( found_sequence_num == 1)
	{
		d_interior_sections_container_ptr->set_focus_feature_at_index( container_section_index);
	}
	else
	{
		// focus feature not found in any sequence? something bad happened
		return;
	}

	// Make sure to draw the focused feature
	// FIXME: there is some bug in set_focus() and
	// can_insert_focused_feature_into_topology() above
	// such that the last item on the table does not pass all the checks,
	// but still gets focused? , so draw it here
	draw_focused_geometry(true);
}


void
GPlatesGui::TopologyTools::find_topological_section_indices(
		int &found_sequence_num,
		std::vector<int> &found_indices,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &properties_iter)
{
	if ( !(feature_ref.is_valid() && properties_iter.is_still_valid() ) )
	{
		// Return empty if either feature reference or property iterator is invalid.
		found_sequence_num = -1;
		return;
	}

	// Check boundary sections
	GPlatesGui::TopologySectionsContainer::const_iterator sections_iter = 
		d_boundary_sections_container_ptr->begin();
	GPlatesGui::TopologySectionsContainer::const_iterator sections_end = 
		d_boundary_sections_container_ptr->end();

	for (int section_index = 0; sections_iter != sections_end; ++sections_iter, ++section_index)
	{
		// See if the feature reference and geometry property iterator matches.
		if (feature_ref == sections_iter->get_feature_ref() &&
			properties_iter == sections_iter->get_geometry_property())
		{
			found_indices.push_back(section_index);
		}

	}

	// return if the focused feature was found 
	if (! found_indices.empty() )
	{
		found_sequence_num = 0;
		return;
	}

	// 
	// Check interior sections
	GPlatesGui::TopologySectionsContainer::const_iterator i_sections_iter = 
		d_interior_sections_container_ptr->begin();
	GPlatesGui::TopologySectionsContainer::const_iterator i_sections_end = 
		d_interior_sections_container_ptr->end();

	for (int i_section_index=0; i_sections_iter != i_sections_end; ++i_sections_iter, ++i_section_index)
	{
		// See if the feature reference and geometry property iterator matches.
		if (feature_ref == i_sections_iter->get_feature_ref() &&
			properties_iter == i_sections_iter->get_geometry_property())
		{
			found_indices.push_back(i_section_index);
		}
	}

	// return if the focused feature was found 
	if (! found_indices.empty() )
	{
		found_sequence_num = 1;
		return;
	}

	// Feature id not found.
	found_sequence_num = -1;
	return;
}


bool
GPlatesGui::TopologyTools::can_insert_focused_feature_into_topology()
{
	// Can't insert focused feature if it's not valid.
	if (!d_feature_focus_ptr->focused_feature().is_valid())
	{
		return false;
	}

    // Can't insert focused feature if it does not have an associated recon geom
	if ( !d_feature_focus_ptr->associated_reconstruction_geometry() )
	{
		return false;
	}
	const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type focused_recon_geom =
			d_feature_focus_ptr->associated_reconstruction_geometry().get();

	// Only insert features that are accepted by the topology geometry type being built/edited.
	// The BuildTopology and EditTopology tools filter for us so we shouldn't have to test this but we will anyway.
	if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
	{
		if (!GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_line_topological_section(focused_recon_geom))
		{
			return false;
		}
	}
	else if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY)
	{
		if (!GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_boundary_topological_section(focused_recon_geom))
		{
			return false;
		}
	}
	else if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
	{
		if (!GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_network_topological_section(focused_recon_geom))
		{
			return false;
		}
	}
	else // Shouldn't get here...
	{
		return false;
	}

#if 0
	// Currently we cannot handle a feature that contains more than one geometry property.
	// This is because a topology geometry is referenced using a property delegate and
	// this means a feature id and a property name. So to uniquely reference a geometry
	// the property name must be unique within the feature. We could detect if the property
	// name is unique and allow adding of the section if it is. However we cannot guarantee
	// that the property name will remain unique for the following reasons:
	// - the user could save to shapefile, edit feature in ArcGIS and load back into GPlates
	//   and currently the shapefile reader/writer does not preserve the property names
	//   (I believe it uses the generic GML names when loading into GPlates),
	// - even if the above were fixed the user could still save to shapefile, edit feature
	//   in ArcGIS, then save it out to shapefile but if the feature crosses the dateline
	//   then ArcGIS will cut the feature into two geometries (so now it's a multi-geom
	//   feature) and when that gets loaded back into GPlates there will be two geometries
	//   in a single feature that have the same property name because presumably the property
	//   name would be stored as a shapefile attributes and shapefile attributes are shared
	//   by all geometries in a shapefile feature.
	//
	// The general solution to this is to use some kind of property id in the property delegate
	// that uniquely identifies a property instead of using a property name.
	GPlatesFeatureVisitors::GeometryTypeFinder geometry_type_finder;
	geometry_type_finder.visit_feature(d_feature_focus_ptr->focused_feature());

// NOTE: MULTIPLE GEOM FIXME
	if (geometry_type_finder.has_found_multiple_geometries_of_the_same_type())
	{
		qWarning()
			<< "Cannot add feature to topology because it has multiple geometries - "
			<< "feature_id=";
		qWarning() << GPlatesUtils::make_qstring_from_icu_string(
				d_feature_focus_ptr->focused_feature()->feature_id().get());

		return false;
	}
#endif

	// Check if the focused feature is already in the topology.
	int found_sequence_num; // which sequce of sections the feature was found in
	std::vector<int> found_indices; // indices in the sequence

	find_topological_section_indices(
			found_sequence_num,
			found_indices,
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());

	// If it's not then we can insert it.
	if ( found_indices.empty() )
	{
		return true;
	}

	//
	// The focused feature is already in the topology.
	//

	// If the focused feature is in the topology *interior* then return false since it
	// can only be added once to the interior.
	// FIXME: We really should be using enums instead of 0 or 1 for the sequence numbers and
	// call it SequenceType is better.
	if (found_sequence_num == 1)
	{
		return false;
	}

	// It is possible to have the focused feature geometry occur more than
	// once in the topology *boundary* under certain conditions:
	// - it must be separated from itself by other geometry sections, or
	// - there must be no contiguous sequence of more than two of the focused
	//   feature geometry *and* it can only be inserted before itself, not after.
	//
	// The reason for the first condition is a geometry can contribute to
	// more than one segment of the plate polygon boundary.
	// The reason for the second condition is the user could build a topology
	// using geometry A as the first section, geometry B as the second section,
	// geometry A as the third section and geometry C as the fourth section.
	// But before they add geometry C we have a configuration where geometry A
	// is next to itself (since it's the first and last section).
	// And it can only be inserted before itself and not after because it doesn't
	// make sense to insert the same geometry twice in a row - it really only makes
	// sense in the above example where geometry A is the first and last section
	// in the topology (ie, when adding geometry A as the last section it is effectively
	// being inserted before the first section).
	//

	// For two or less sections in the topology it doesn't make sense for them
	// to be the same geometry - the user has no reason to do this.
	if (d_boundary_section_info_seq.size() <= 2)
	{
		return false;
	}

	// Get the current insertion point 
	const section_info_seq_type::size_type insert_section_index =
			d_boundary_sections_container_ptr->insertion_point();

	// Make sure that we don't create a contiguous sequence of three sections with the
	// same geometry when we insert the focused feature geometry.
	if (std::find(found_indices.begin(), found_indices.end(),
			static_cast<int>(get_prev_index(insert_section_index, d_boundary_section_info_seq)))
				!= found_indices.end())
	{
		// Cannot insert same geometry after itself.
		return false;
	}
	else if (std::find(found_indices.begin(), found_indices.end(), 
				static_cast<int>(insert_section_index)) != found_indices.end())
	{
		if (std::find(found_indices.begin(), found_indices.end(),
				static_cast<int>(get_next_index(insert_section_index, d_boundary_section_info_seq)))
					!= found_indices.end())
		{
			// We'd be inserting in front of two sections with same geometry - not allowed.
			return false;
		}
	}

	return true;
}


bool
GPlatesGui::TopologyTools::can_remove_focused_feature_from_topology()
{
	// Can't remove focused feature if it's not valid.
	if (!d_feature_focus_ptr->focused_feature().is_valid())
	{
		return false;
	}

    // Can't remove focused feature if it does not have an associated recon geom
	if ( !d_feature_focus_ptr->associated_reconstruction_geometry() )
	{
		return false;
	}

	// Check if the focused feature is in the topology.
	int found_sequence_num; // which sequence of sections the feature was found in
	std::vector<int> found_indices; // indices in the sequence

	find_topological_section_indices(
			found_sequence_num,
			found_indices, 
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());

	// If feature is currently in the topology then we can remove it.
	return !found_indices.empty();
}


void
GPlatesGui::TopologyTools::initialise_topological_sections_from_edit_topology_focused_feature()
{
	// Get the edit topology feature ref.
	const GPlatesModel::FeatureHandle::weak_ref edit_topology_feature_ref =
			d_feature_focus_ptr->focused_feature();

	// Create a new TopologySectionsFinder to fill a topology sections container
	// table row for each topology section found.
	GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder;

	// Visit the topology feature.
	topo_sections_finder.visit_feature(edit_topology_feature_ref);

	// NOTE: This will generate a signal that will call our 'react_cleared_FOO()'
	// method which clear out our internal section sequence and redraw.
	d_boundary_sections_container_ptr->clear();
	d_interior_sections_container_ptr->clear();

	//
	// initialize Boundary Sections
	//

	// NOTE: This will generate a signal that will call our 'react_entries_insert()'
	// method which will handle the building of our topology data structures.
	d_boundary_sections_container_ptr->insert(
			topo_sections_finder.boundary_sections_begin(),
			topo_sections_finder.boundary_sections_end());

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_boundary_section_info_seq.size() == d_boundary_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	//
	// Initialize with interior list
	//

	// NOTE: This will generate a signal that will call our 'react_entries_insert()'
	// method which will handle the building of our topology data structures.
	d_interior_sections_container_ptr->insert(
			topo_sections_finder.interior_sections_begin(),
			topo_sections_finder.interior_sections_end());

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_interior_section_info_seq.size() == d_interior_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);
}


//
// boundary react functions
//



void
GPlatesGui::TopologyTools::react_cleared_boundary()
{
	if (! d_is_active) { return; }

	d_boundary_section_info_seq.clear();

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_boundary_section_info_seq.size() == d_boundary_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology();
}


void
GPlatesGui::TopologyTools::react_entry_removed_boundary(
		GPlatesGui::TopologySectionsContainer::size_type deleted_index)
{
	if (! d_is_active) { return; }

	// Make sure delete index is not out of range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			deleted_index < d_boundary_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	// Remove from our internal section sequence.
	d_boundary_section_info_seq.erase(d_boundary_section_info_seq.begin() + deleted_index);

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_boundary_section_info_seq.size() == d_boundary_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// unset focus 
	unset_focus();

	// Update topology and redraw.
	update_and_redraw_topology();
}

void
GPlatesGui::TopologyTools::react_entries_inserted_boundary(
		GPlatesGui::TopologySectionsContainer::size_type inserted_index,
		GPlatesGui::TopologySectionsContainer::size_type quantity,
		GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
		GPlatesGui::TopologySectionsContainer::const_iterator inserted_end)
{
	if (! d_is_active) { return; }

	// Iterate over the table rows inserted in the topology sections container and
	// also insert them into our internal section sequence structure.
	std::transform(
			inserted_begin,
			inserted_end,
			std::inserter(
					d_boundary_section_info_seq,
					d_boundary_section_info_seq.begin() + inserted_index),
			// Calls the SectionInfo constructor with a TableRow as the argument...
			boost::lambda::bind(
					boost::lambda::constructor<SectionInfo>(),
					boost::lambda::_1));

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_boundary_section_info_seq.size() == d_boundary_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology();
}


void
GPlatesGui::TopologyTools::react_entry_modified_boundary(
		GPlatesGui::TopologySectionsContainer::size_type modified_section_index)
{
	if (! d_is_active) { return; }

	// Our section sequence should be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_boundary_section_info_seq.size() == d_boundary_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Copy the table row into our own internal sequence.
	d_boundary_section_info_seq[modified_section_index].d_table_row =
			d_boundary_sections_container_ptr->at(modified_section_index);

	// Update topology and redraw.
	update_and_redraw_topology();
}

//
// interior react functions
//

void
GPlatesGui::TopologyTools::react_cleared_interior()
{
	if (! d_is_active) { return; }

	d_interior_section_info_seq.clear();

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_interior_section_info_seq.size() == d_interior_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology();
}



void
GPlatesGui::TopologyTools::react_insertion_point_moved(
		GPlatesGui::TopologySectionsContainer::size_type new_index)
{
	// this check prevents the draw before the tables are filled upon init
	if ( 
		( d_boundary_section_info_seq.size() == d_boundary_sections_container_ptr->size() ) || 
		( d_interior_section_info_seq.size() == d_interior_sections_container_ptr->size() ) )
	{ 
		draw_insertion_neighbors();
	}
}


void
GPlatesGui::TopologyTools::react_entry_removed_interior(
		GPlatesGui::TopologySectionsContainer::size_type deleted_index)
{
	if (! d_is_active) { return; }

	// Make sure delete index is not out of range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			deleted_index < d_interior_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	// Remove from our internal section sequence.
	d_interior_section_info_seq.erase(d_interior_section_info_seq.begin() + deleted_index);

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_interior_section_info_seq.size() == d_interior_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// unset focus 
	unset_focus();

	// Update topology and redraw.
	update_and_redraw_topology();
}

void
GPlatesGui::TopologyTools::react_entries_inserted_interior(
		GPlatesGui::TopologySectionsContainer::size_type inserted_index,
		GPlatesGui::TopologySectionsContainer::size_type quantity,
		GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
		GPlatesGui::TopologySectionsContainer::const_iterator inserted_end)
{
	if (! d_is_active) { return; }

	// Iterate over the table rows inserted in the topology sections container and
	// also insert them into our internal section sequence structure.
	std::transform(
			inserted_begin,
			inserted_end,
			std::inserter(
					d_interior_section_info_seq,
					d_interior_section_info_seq.begin() + inserted_index),
			// Calls the SectionInfo constructor with a TableRow as the argument...
			boost::lambda::bind(
					boost::lambda::constructor<SectionInfo>(),
					boost::lambda::_1));

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_interior_section_info_seq.size() == d_interior_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology();
}


void
GPlatesGui::TopologyTools::react_entry_modified_interior(
		GPlatesGui::TopologySectionsContainer::size_type modified_section_index)
{
	if (! d_is_active) { return; }

	// Our section sequence should be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_interior_section_info_seq.size() == d_interior_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Copy the table row into our own internal sequence.
	d_interior_section_info_seq[modified_section_index].d_table_row =
			d_interior_sections_container_ptr->at(modified_section_index);

	// Update topology and redraw.
	update_and_redraw_topology();
}


// 
// Widget handle functions 
//

void
GPlatesGui::TopologyTools::handle_add_section()
{
	// only allow adding of focused features
	if ( ! d_feature_focus_ptr->is_valid() ) 
	{ 
		return;
	}

	// Check if we can insert the focused feature into the topology.
	// This also checks that the focused reconstruction geometry is the correct type
	// for the topology geometry being built/edited.
	if (!can_insert_focused_feature_into_topology())
	{
		return;
	}

	// synchronize the tools, container, widget, and table
	synchronize_seq_num( 0 );

	// Flip the ViewportWindow to the Topology Sections Table
	d_viewport_window_ptr->search_results_dock_widget().choose_topology_sections_table();

	// Check for ReconstructionGeometry
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type rg_ptr =
		d_feature_focus_ptr->associated_reconstruction_geometry();
	if (!rg_ptr)
	{
		return;
	}

	boost::optional<GPlatesModel::FeatureHandle::iterator> focused_geometry_property =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(rg_ptr);
	if (!focused_geometry_property)
	{
		return;
	}

	// The table row to insert into the topology sections container.
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(focused_geometry_property.get());

	// Insert the row.
	// NOTE: This will generate a signal that will call our 'react_entries_inserted_boundary()'
	// method which will handle the building of our topology data structures.
	d_boundary_sections_container_ptr->insert( table_row );

	// unset focus 
	unset_focus();
}


void
GPlatesGui::TopologyTools::handle_add_interior()
{
	// only allow adding of focused features
	if ( ! d_feature_focus_ptr->is_valid() ) 
	{ 
		return;
	}

	// Check if we can insert the focused feature into the topology.
	// This also checks that the focused reconstruction geometry is the correct type
	// for the topology geometry being built/edited.
	if (!can_insert_focused_feature_into_topology())
	{
		return;
	}

	// synchronize the tools, container, widget, and table
	synchronize_seq_num( 1 );
	
	// Flip the ViewportWindow to the Topology Sections Table
	d_viewport_window_ptr->search_results_dock_widget().choose_topology_sections_table();

	// Check for ReconstructionGeometry
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type rg_ptr =
		d_feature_focus_ptr->associated_reconstruction_geometry();
	if (!rg_ptr)
	{
		return;
	}

	boost::optional<GPlatesModel::FeatureHandle::iterator> focused_geometry_property =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(rg_ptr);
	if (!focused_geometry_property)
	{
		return;
	}

	// The table row to insert into the topology sections container.
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(focused_geometry_property.get());

	// Insert the row.
	// NOTE: This will generate a signal that will call our 'react_entries_inserted_interior()'
	// method which will handle the building of our topology data structures.
	d_interior_sections_container_ptr->insert( table_row );

	// unset focus 
	unset_focus();
}


void
GPlatesGui::TopologyTools::handle_remove_section()
{
	// only allow adding of focused features
	if ( ! d_feature_focus_ptr->is_valid() ) 
	{ 
		return;
	}

	// check if the feature is in the topology 
	int found_sequence_num; // which sequce of sections the feature was found in
	std::vector<int> found_indices; // indices in the sequence

	find_topological_section_indices(
			found_sequence_num,
			found_indices, 
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());
	
	// If focused feature found in topology boundary...
	if ( !found_indices.empty() )	
	{
		int check_index = found_indices[0];

		if ( found_sequence_num == 0)
		{
			// remove it from the boundary containter 
			d_boundary_sections_container_ptr->remove_at( check_index );
			// NOTE : this will then trigger  our react_entry_removed_boundary() 
			// so no further action should be needed here ...
		}
		else if ( found_sequence_num == 1)
		{
			// remove it from the interior containter 
			d_interior_sections_container_ptr->remove_at( check_index );
			// NOTE : this will then trigger  our react_entry_removed_interior() 
			// so no further action should be needed here ...
		}
	}
}


void
GPlatesGui::TopologyTools::synchronize_seq_num(
		int i)
{
	// Adjust the combobox if necessary
	if ( d_topology_tools_widget_ptr->get_sections_combobox_index() != i )
	{
		// flip the combobox ; will trigger handle_sections_combobox_index_changed()
		d_topology_tools_widget_ptr->set_sections_combobox_index( i );
	}
	else
	{
		// just set the operating sequence
		handle_sections_combobox_index_changed( i );
	}
	
	// update seq_num
	d_seq_num = i;
}


void
GPlatesGui::TopologyTools::handle_sections_combobox_index_changed(
		int i)
{
	QString s("Topology Sections"); // default

	// which sequnce of sections
	if (i == 0)
	{
		// set interal state 
		d_seq_num = 0;

		if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
		{
			s = QString("Topology Sections");
		}
		else // topology boundaries and networks...
		{
			s = QString("Topology Boundary Sections");
		}
		// set the container ptr in the sections table
		d_boundary_sections_container_ptr->set_container_ptr_in_table(d_boundary_sections_container_ptr);
		// update the table contnent 
		d_boundary_sections_container_ptr->update_table_from_container();
	}
	else if ( i == 1)
	{
		// set interal state 
		d_seq_num = 1;

		s = QString("Topology Interior Sections");
		// set the container ptr in the sections table
		d_interior_sections_container_ptr->set_container_ptr_in_table(d_interior_sections_container_ptr);
		// update the table contnent 
		d_interior_sections_container_ptr->update_table_from_container();
	}

	// change the text on the table tab to match the current seq
	d_viewport_window_ptr->search_results_dock_widget().set_topology_sections_table_tab_text( s );

	// Update topology and redraw.; changing the combo box changes the color scheme 
	update_and_redraw_topology();
}


void
GPlatesGui::TopologyTools::handle_clear()
{
	// unset focus 
	unset_focus();

	// Remove all sections from the topology sections container.
	// NOTE: This will generate a signal that will call our 'react_cleared_FOO()' functions
	// which clear out our internal section sequence and redraw.

	( d_viewport_window_ptr->get_view_state().get_topology_boundary_sections_container() ).clear();
	( d_viewport_window_ptr->get_view_state().get_topology_interior_sections_container() ).clear();
}


void
GPlatesGui::TopologyTools::draw_all_layers_clear()
{
	// clear all layers
	d_topology_geometry_layer_ptr->clear_rendered_geometries();
	d_segments_layer_ptr->clear_rendered_geometries();
	d_intersection_points_layer_ptr->clear_rendered_geometries();
	d_boundary_geometry_layer_ptr->clear_rendered_geometries();
	d_interior_geometry_layer_ptr->clear_rendered_geometries();
	d_insertion_neighbors_layer_ptr->clear_rendered_geometries();
#if 0
	d_end_points_layer_ptr->clear_rendered_geometries();
#endif
	d_focused_feature_layer_ptr->clear_rendered_geometries();
}


void
GPlatesGui::TopologyTools::draw_all_layers()
{
	// If the topology feature (only in edit mode) does not exist
	// at the current reconstruction time then don't draw it.
	// All its sections will still exist though.
	if (!does_topology_exist_at_current_recon_time())
	{
		draw_all_layers_clear();
		return;
	}

	// draw all the layers
	draw_topology_geometry();
	draw_boundary_geometry();
	draw_interior_geometry();

	draw_segments();
	draw_intersection_points();
	draw_insertion_neighbors();

	// draw_end_points();

#if 0 
	// Don't draw the section click points anymore since they are not used to determine
	// the boundary subsegment anymore (reduces visual clutter).
	draw_click_points();
#endif

	// Draw focused geometry.
	draw_focused_geometry(
			can_insert_focused_feature_into_topology() ||
			can_remove_focused_feature_from_topology());
}


void
GPlatesGui::TopologyTools::draw_topology_geometry()
{
	d_topology_geometry_layer_ptr->clear_rendered_geometries();

	// FIXME: we might want the network drawn here too, but for now, 
	// don't draw the network, just let the Resolver do it

	const GPlatesGui::Colour &point_colour = GPlatesGui::Colour(0.75, 0.75, 0.75, 1.0);

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
GPlatesGui::TopologyTools::draw_boundary_geometry()
{
	d_boundary_geometry_layer_ptr->clear_rendered_geometries();

	// super short cut
	if (!d_boundary_geometry_opt_ptr) { return; }

	// Render the active topology section sequence in contrast to the topology verts (and network) 
	GPlatesGui::Colour colour = d_rendered_geometry_parameters.get_topology_tool_topological_sections_colour();
	float point_size_hint =
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_point_size_hint();
    float line_width_hint =
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_line_width_hint();

	// check if boundary is the active sequnce
	if ( d_topology_tools_widget_ptr->get_sections_combobox_index() != 0 ) // 0 = boundary list
	{
		// Render the non-active topology sections sequence muted 
		colour = GPlatesGui::Colour(0.75f, 0.75f, 0.75f, 1.0f); // light grey
		point_size_hint = GPlatesViewOperations::RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT;
    	line_width_hint = GPlatesViewOperations::RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT;
	}

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
			*d_boundary_geometry_opt_ptr,
			colour,
			point_size_hint,
			line_width_hint);

	d_boundary_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
}

void
GPlatesGui::TopologyTools::draw_interior_geometry()
{
	d_interior_geometry_layer_ptr->clear_rendered_geometries();

	// super short cut for empty list
	if ( d_interior_geometry_opt_ptrs.empty() ) { return; }

	// Render the active topology section sequence in contrast to topology verts 
	GPlatesGui::Colour colour = d_rendered_geometry_parameters.get_topology_tool_topological_sections_colour();
	float point_size_hint =
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_point_size_hint();
    float line_width_hint =
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_line_width_hint();

	// check if interior is the active sequence
	if ( d_topology_tools_widget_ptr->get_sections_combobox_index() != 1 ) // 1 = interior list
	{
		// Render the non-active topology sections sequence muted 
		colour = GPlatesGui::Colour(0.75, 0.75, 0.75, 1.0); // light grey
		point_size_hint = GPlatesViewOperations::RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT;
    	line_width_hint = GPlatesViewOperations::RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT;
	}

	// draw all the interior sections
	std::vector<boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> >::iterator itr;
	std::vector<boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> >::iterator end;
	itr = d_interior_geometry_opt_ptrs.begin();
	end = d_interior_geometry_opt_ptrs.end();

	for ( ; itr != end; ++itr )
	{
		if ( itr->get() )
		{
			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					itr->get(),
					colour,
					point_size_hint,
					line_width_hint);

			d_interior_geometry_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
}

void
GPlatesGui::TopologyTools::draw_focused_geometry(
		bool draw)
{
	d_focused_feature_layer_ptr->clear_rendered_geometries();

	// Return early if we need to clear the rendered focused feature.
	if (!draw)
	{
		return;
	}

	// always check weak refs
	if ( ! d_feature_focus_ptr->is_valid() ) { return; }

	if ( !d_feature_focus_ptr->associated_reconstruction_geometry() )
	{
		return;
	}
	const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type focused_recon_geom =
			d_feature_focus_ptr->associated_reconstruction_geometry().get();

	// There are restrictions on the type of topological section depending on the topology geometry
	// type currently being built/edited.
	// But this has already been handled both by classes BuildTopology/EditTopology and by the
	// member function 'can_insert_focused_feature_into_topology()'.
	// So there's no need to apply the restrictions here.

	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> focused_geometry;

	// See if focused reconstruction geometry is an RFG.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> focused_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry *>(focused_recon_geom);
	if (focused_rfg)
	{
		// Get the geometry on sphere from the RFG.
		focused_geometry = focused_rfg.get()->reconstructed_geometry();
	}

	// See if focused reconstruction geometry is an RTG.
	boost::optional<const GPlatesAppLogic::ResolvedTopologicalGeometry *> focused_rtg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ResolvedTopologicalGeometry *>(focused_recon_geom);
	if (focused_rtg)
	{
		focused_geometry = focused_rtg.get()->resolved_topology_geometry();
	}

	if (!focused_geometry)
	{
		return;
	}

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_style_params(
			d_rendered_geometry_parameters);
	render_style_params.reconstruction_line_width_hint =
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_line_width_hint();
	render_style_params.reconstruction_point_size_hint =
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_point_size_hint();

	// This creates the RenderedGeometry's from the ReconstructionGeometry's.
	GPlatesPresentation::ReconstructionGeometryRenderer reconstruction_geometry_renderer(
			render_style_params,
			d_viewport_window_ptr->get_view_state().get_render_settings(),
			d_application_state_ptr->get_current_topological_sections(),
			d_application_state_ptr->get_current_reconstruction().get_all_resolved_topological_shared_sub_segments(),
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_colour(),
			boost::none,
			boost::none);

	reconstruction_geometry_renderer.begin_render(*d_focused_feature_layer_ptr);

	focused_recon_geom->accept_visitor(reconstruction_geometry_renderer);

	reconstruction_geometry_renderer.end_render();

	// Get the start and end points of the focused feature's geometry.
	// Since the geometry is in focus but has not been added to the topology
	// there is no information about whether to reverse its points so we don't.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			focus_feature_end_points =
				GPlatesAppLogic::GeometryUtils::get_geometry_exterior_end_points(
						*focused_geometry.get());

	//
	// Draw the focused end_points.
	//

	const GPlatesMaths::PointOnSphere &focus_feature_start_point = focus_feature_end_points.first;
	const GPlatesMaths::PointOnSphere &focus_feature_end_point = focus_feature_end_points.second;
	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

	//
	// NOTE: Both end points are drawn the same size now since we don't need
	// to show the user which direction the geometry is because the user does not
	// need to reverse any geometry any more (it's handled automatically now).
	//

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry start_point_rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			focus_feature_start_point,
			colour,
			GPlatesViewOperations::GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

	// Add to layer.
	d_focused_feature_layer_ptr->add_rendered_geometry(start_point_rendered_geometry);

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry end_point_rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			focus_feature_end_point,
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

	//
	// Draw the boundary subsegment geometry rather than the entire section geometry.
	// This helps see the topology polygon more clearly and visuals helps the user
	// see if there's some weird polygon boundary that runs to the section endpoint
	// rather than the section intersection point.
	//

	// Iterate over the visible sections and draw the subsegment geometry of each.
	visible_section_seq_type::const_iterator visible_section_iter = 
		d_visible_boundary_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = 
		d_visible_boundary_section_seq.end();
	for ( ; visible_section_iter != visible_section_end; ++visible_section_iter)
	{
		if (visible_section_iter->d_final_boundary_segment_unreversed_geom)
		{
			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					*visible_section_iter->d_final_boundary_segment_unreversed_geom,
					colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

			// Add to layer.
			d_segments_layer_ptr->add_rendered_geometry(rendered_geometry);
		}
	}
}


#if 0
void
GPlatesGui::TopologyTools::draw_end_points()
{
	d_end_points_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &end_points_colour = GPlatesGui::Colour::get_grey();

	// Iterate over the visible sections and draw the start and end point of each segment.
	visible_section_seq_type::const_iterator visible_section_iter = 
		d_visible_boundary_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = 
		d_visible_boundary_section_seq.end();
	for ( ; visible_section_iter != visible_section_end; ++visible_section_iter)
	{
		if (visible_section_iter->d_section_start_point)
		{
			const GPlatesMaths::PointOnSphere &segment_start =
					*visible_section_iter->d_section_start_point;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry start_point_rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					segment_start,
					end_points_colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT);
					//GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(start_point_rendered_geometry);
		}

		if (visible_section_iter->d_section_end_point)
		{
			const GPlatesMaths::PointOnSphere &segment_end =
					*visible_section_iter->d_section_end_point;

			// Create rendered geometry.
			const GPlatesViewOperations::RenderedGeometry end_point_rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					segment_end,
					end_points_colour,
					GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT);
					//GPlatesViewOperations::GeometryOperationParameters::REGULAR_POINT_SIZE_HINT);

			// Add to layer.
			d_end_points_layer_ptr->add_rendered_geometry(end_point_rendered_geometry);
		}
	}
}
#endif


void
GPlatesGui::TopologyTools::draw_intersection_points()
{
	d_intersection_points_layer_ptr->clear_rendered_geometries();

	//const GPlatesGui::Colour &intersection_points_colour = GPlatesGui::Colour::get_grey();
	const GPlatesGui::Colour &intersection_points_colour = GPlatesGui::Colour::get_black();

	// Iterate over the visible sections and draw the start intersection of each segment.
	// Since the start intersection of one segment is the same as the end intersection
	// of the previous segment we do not need to draw the end intersection points.
	visible_section_seq_type::const_iterator visible_section_iter = 
		d_visible_boundary_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = 
		d_visible_boundary_section_seq.end();
	for ( ; visible_section_iter != visible_section_end; ++visible_section_iter)
	{
		if (!visible_section_iter->d_intersection_point_with_prev)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere &segment_start_intersection =
				*visible_section_iter->d_intersection_point_with_prev;

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
GPlatesGui::TopologyTools::draw_insertion_neighbors()
{
	d_insertion_neighbors_layer_ptr->clear_rendered_geometries();

	// local working vars
	section_info_seq_type local_section_info_seq;
	local_section_info_seq.clear();

	visible_section_seq_type local_visible_section_seq;
	local_visible_section_seq.clear();

	GPlatesGui::TopologySectionsContainer *topology_sections_container_ptr;

	// choose which sequence intersection neighbors to draw
	if (d_seq_num == 0)
	{
		local_section_info_seq = d_boundary_section_info_seq;
		local_visible_section_seq = d_visible_boundary_section_seq;

		topology_sections_container_ptr = 
			&(d_viewport_window_ptr->get_view_state().get_topology_boundary_sections_container());

	}
	else if ( d_seq_num == 1)
	{
		local_section_info_seq = d_interior_section_info_seq;
		local_visible_section_seq = d_visible_interior_section_seq;

		topology_sections_container_ptr = 
			&(d_viewport_window_ptr->get_view_state().get_topology_interior_sections_container());
	}
	else
		return;

	// If no visible sections then nothing to draw.
	if ( local_section_info_seq.empty() )
	{
		return;
	}

	// Get the current insertion point in the topology sections container.
	const GPlatesGui::TopologySectionsContainer::size_type insertion_point = 
		topology_sections_container_ptr->insertion_point();

	// Index of the geometry just before the insertion point.
	GPlatesGui::TopologySectionsContainer::size_type prev_neighbor_index =
		get_prev_index(insertion_point, local_section_info_seq);

	// Index of the geometry after the insertion point.
	// This is actually the geometry at the insertion point index 
	GPlatesGui::TopologySectionsContainer::size_type next_neighbor_index = 0;
	if ( insertion_point == local_section_info_seq.size() )
	{
		next_neighbor_index = 0;
	} else {
		next_neighbor_index = insertion_point;
	}

#if 0
qDebug() << "local_section_info_seq.size = " << local_section_info_seq.size();
qDebug() << "local_visible_section_seq.size = " << local_visible_section_seq.size();
qDebug() << "";
qDebug() << "prev index = " << prev_neighbor_index;
qDebug() << "insert idx = " << insertion_point;
qDebug() << "next index = " << next_neighbor_index;
qDebug() << "";
#endif


	// See if the section before the insertion point is currently visible by searching
	// for its section index in the visible sections.
	boost::optional<visible_section_seq_type::const_iterator> prev_visible_section_iter;
	for (section_info_seq_type::size_type count_p = 0;
		count_p < local_section_info_seq.size();
		++count_p, prev_neighbor_index = get_prev_index(prev_neighbor_index, local_section_info_seq))
	{
		if ( d_seq_num == 0 ) {
			prev_visible_section_iter = is_section_visible_boundary(prev_neighbor_index);
		} else if (d_seq_num == 1) {
			prev_visible_section_iter = is_section_visible_interior(prev_neighbor_index);
		}

		if (prev_visible_section_iter)
		{
			break;
		}
	}

	if (prev_visible_section_iter &&
			prev_visible_section_iter.get()->d_final_boundary_segment_unreversed_geom) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_blue();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*prev_visible_section_iter.get()->d_section_geometry_unreversed,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::TOPOLOGY_TOOL_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}


	// See if the section at/after the insertion point is currently visible by searching
	// for its section index in the visible sections.
	boost::optional<visible_section_seq_type::const_iterator> next_visible_section_iter;
	for (section_info_seq_type::size_type count_n = 0;
		count_n < local_section_info_seq.size();
		++count_n, next_neighbor_index = get_next_index(next_neighbor_index, local_section_info_seq))
	{
		if (d_seq_num == 0) {
			next_visible_section_iter = is_section_visible_boundary(next_neighbor_index);
		} else if (d_seq_num == 1) {
			next_visible_section_iter = is_section_visible_interior(next_neighbor_index);
		}

		if (next_visible_section_iter)
		{
			break;
		}
	}

	if (next_visible_section_iter &&
			next_visible_section_iter.get()->d_final_boundary_segment_unreversed_geom) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_green();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*next_visible_section_iter.get()->d_section_geometry_unreversed,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::TOPOLOGY_TOOL_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}

void
GPlatesGui::TopologyTools::update_and_redraw_topology()
{
	PROFILE_FUNC();
	
	// 
	// update Boundary
	//

	//
	// First iterate through the sections and reconstruct the currently visible
	// ones (ie, the ones whose age range contains the current reconstruction time).
	//
	reconstruct_boundary_sections();

	//
	// Next iterate through the potential intersections of the visible sections.
	//

	if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
	{
		process_resolved_line_topological_section_intersections();
	}
	else // topological boundary or network (boundary)...
	{
		process_resolved_boundary_topological_section_intersections();
	}

	//
	// Next iterate over all the visible sections and determine if any need to be reversed by
	// minimizing the distance of any rubber-banding.
	// Reversal is only determined for those visible sections that don't intersect *both*
	// adjacent visible sections because they are the visible sections that need rubber-banding.
	// Hence this needs to be done after intersection processing.
	//
	determine_boundary_segment_reversals();

	//
	// Next iterate over all the visible sections and assign the boundary subsegments that
	// will form the boundary of the plate polygon.
	//
	assign_boundary_segments();

	// Set the number of boundary sections in the TopologyToolsWidget
	d_topology_tools_widget_ptr->display_number_of_sections_boundary(
		d_boundary_sections_container_ptr->size() );

	// 
	// Interior ; see above for similar comments regarding these processing steps
	//
	reconstruct_interior_sections();

	// NOTE: Do not process any intersections, or reverse code on the interior sections

	assign_interior_segments();

	d_topology_tools_widget_ptr->display_number_of_sections_interior(
		d_interior_sections_container_ptr->size() );

	//
	// finalize total Topology 
	//

	// Now that we've updated all the topology boundary and interior subsegments 
	// we can create the sets of vertices and geoms for display.
	update_topology_vertices();

	draw_all_layers();
}


bool
GPlatesGui::TopologyTools::does_topology_exist_at_current_recon_time() const
{
 	// Check if the topology feature being built or edited is defined for the current reconstruction time.
	// The topology time period is non-null when in edit mode and null in build mode.
	if (d_topology_time_period)
	{
		return d_topology_time_period.get()->contains(
				GPlatesPropertyValues::GeoTimeInstant(
						d_application_state_ptr->get_current_reconstruction_time()));
	}

	// Else a time period of boost::none implies all time (distant past to distant future).
	return true;
}


void
GPlatesGui::TopologyTools::reconstruct_boundary_sections()
{
	// Clear the list of currently visible sections.
	// We're going to repopulate it.
	d_visible_boundary_section_seq.clear();

	std::vector<GPlatesAppLogic::ReconstructHandle::type> topological_section_reconstruct_handles;

	// Generate RFGs for all active ReconstructLayer's.
	// This is needed because we are about to search the topological section features for
	// their RFGs and if we don't generate the RFGs then they might not be found.
	//
	// The RFGs - we'll keep them active until we've finished searching for RFGs below.
	std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
	GPlatesAppLogic::LayerProxyUtils::get_reconstructed_feature_geometries(
			reconstructed_feature_geometries,
			topological_section_reconstruct_handles,
			d_application_state_ptr->get_current_reconstruction(),
			// Filter out 'topology' reconstructed feature geometries. These cannot supply topological
			// sections because they use topology layers thus creating a cyclic dependency. This also
			// increases performance by avoiding lengthy reconstructions of features using topologies...
			false/*include_topology_reconstructed_feature_geometries*/);

	// Also, if building a resolved topological boundary or network, then generate the
	// resolved topological lines since they can be used as topological sections.
	if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY ||
		d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
	{
		std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> resolved_topological_lines;
		GPlatesAppLogic::LayerProxyUtils::get_resolved_topological_lines(
				resolved_topological_lines,
				topological_section_reconstruct_handles,
				d_application_state_ptr->get_current_reconstruction());
	}

	const section_info_seq_type::size_type num_sections = d_boundary_section_info_seq.size();

	section_info_seq_type::size_type section_index;
	for (section_index = 0; section_index < num_sections; ++section_index)
	{
		SectionInfo &section_info = d_boundary_section_info_seq[section_index];

		// Create a structure that we can use to do intersection processing and
		// rendering with.
		//
		// Returns false if section is not currently visible because it's age range
		// does not contain the current reconstruction time.
		const boost::optional<VisibleSection> visible_section =
				section_info.reconstruct_section_info_from_table_row(
						section_index,
						d_application_state_ptr->get_current_reconstruction_time(),
						section_info.d_table_row.get_reverse(),
						topological_section_reconstruct_handles);

		if (!visible_section)
		{
			continue;
		}

		// Add to the list of currently visible sections - these are the sections
		// that will be intersection processed.
		d_visible_boundary_section_seq.push_back(*visible_section);
	}
}

void
GPlatesGui::TopologyTools::reconstruct_interior_sections()
{
	// Clear the list of currently visible sections.
	// We're going to repopulate it.
	d_visible_interior_section_seq.clear();

	// Generate RFGs for all active ReconstructLayer's.
	// This is needed because we are about to search the topological section features for
	// their RFGs and if we don't generate the RFGs then they might not be found.
	//
	// The RFGs - we'll keep them active until we've finished searching for RFGs below.
	std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
	std::vector<GPlatesAppLogic::ReconstructHandle::type> reconstruct_handles;
	GPlatesAppLogic::LayerProxyUtils::get_reconstructed_feature_geometries(
			reconstructed_feature_geometries,
			reconstruct_handles,
			d_application_state_ptr->get_current_reconstruction(),
			// Filter out 'topology' reconstructed feature geometries. These cannot supply topological
			// sections because they use topology layers thus creating a cyclic dependency. This also
			// increases performance by avoiding lengthy reconstructions of features using topologies...
			false/*include_topology_reconstructed_feature_geometries*/);

	const section_info_seq_type::size_type num_sections = d_interior_section_info_seq.size();

	section_info_seq_type::size_type section_index;
	for (section_index = 0; section_index < num_sections; ++section_index)
	{
		SectionInfo &section_info = d_interior_section_info_seq[section_index];

		// Create a structure that we can use to do intersection processing and
		// rendering with.
		//
		// Returns false if section is not currently visible because it's age range
		// does not contain the current reconstruction time.
		const boost::optional<VisibleSection> visible_section =
				section_info.reconstruct_section_info_from_table_row(
						section_index,
						d_application_state_ptr->get_current_reconstruction_time(),
						section_info.d_table_row.get_reverse(),
						reconstruct_handles);

		if (!visible_section)
		{
			continue;
		}

		// Add to the list of currently visible sections - these are the sections
		// that will be intersection processed.
		d_visible_interior_section_seq.push_back(*visible_section);
	}
}


void
GPlatesGui::TopologyTools::process_resolved_line_topological_section_intersections()
{
	const visible_section_seq_type::size_type num_visible_sections = 
		d_visible_boundary_section_seq.size();

	// If there's only one section then don't try to intersect it with itself.
	if (num_visible_sections < 2)
	{
		return;
	}

	// Topological *lines* don't form a closed loop of sections so we don't handle wraparound.
	// For the first section there is no previous section so there's no intersection to process.
	// So we skip the first section and start at index 1.
	// The first section will still get intersected when the second section is visited.
	visible_section_seq_type::size_type visible_section_index;
	for (visible_section_index = 1;
		visible_section_index < num_visible_sections;
		++visible_section_index)
	{
		// The convention is to process the intersection at the start of a visible section.
		// We could have chosen the end (would've also been fine) - but we chose the start.
		// This potentially intersects the start of 'visible_section_index' and the end
		// of 'visible_section_index - 1'.

		const visible_section_seq_type::size_type prev_visible_section_index =
				get_prev_index(visible_section_index, d_visible_boundary_section_seq);

		const visible_section_seq_type::size_type next_visible_section_index =
				visible_section_index;

		process_topological_section_intersection(
				prev_visible_section_index,
				next_visible_section_index);
	}
}


void
GPlatesGui::TopologyTools::process_resolved_boundary_topological_section_intersections()
{
	const visible_section_seq_type::size_type num_visible_sections = 
		d_visible_boundary_section_seq.size();

	// If there's only one section then don't try to intersect it with itself.
	if (num_visible_sections < 2)
	{
		return;
	}

	// Special case treatment when there are exactly two sections.
	// In this case the two sections can intersect twice to form a closed polygon.
	// This is the only case where two adjacent sections are allowed to intersect twice.
	if (num_visible_sections == 2)
	{
		process_resolved_boundary_two_topological_sections_intersections();
		return;
	}

	visible_section_seq_type::size_type visible_section_index;
	for (visible_section_index = 0;
		visible_section_index < num_visible_sections;
		++visible_section_index)
	{
		// The convention is to process the intersection at the start of a visible section.
		// We could have chosen the end (would've also been fine) - but we chose the start.
		// This potentially intersects the start of 'visible_section_index' and the end
		// of 'visible_section_index - 1'.

		const visible_section_seq_type::size_type prev_visible_section_index =
				get_prev_index(visible_section_index, d_visible_boundary_section_seq);

		const visible_section_seq_type::size_type next_visible_section_index =
				visible_section_index;

		process_topological_section_intersection(
				prev_visible_section_index,
				next_visible_section_index);
	}
}


void
GPlatesGui::TopologyTools::process_resolved_boundary_two_topological_sections_intersections()
{
	// If we got here then we have exactly two visible sections in total.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_visible_boundary_section_seq.size() == 2,
			GPLATES_ASSERTION_SOURCE);

	//
	// We treat two visible sections as a special case because if they intersect each other
	// exactly twice then we can form a closed polygon from them.
	// NOTE: This only applies to topological *boundaries* (not topological *lines*).
	// NOTE: For three or more sections we don't allow multiple intersections
	// between two adjacent sections.
	//

	VisibleSection &first_visible_section = d_visible_boundary_section_seq[0];
	VisibleSection &second_visible_section = d_visible_boundary_section_seq[1];

	//
	// Process the actual intersection(s).
	//

	typedef boost::optional<
			boost::tuple<
					// First intersection
					GPlatesMaths::PointOnSphere,
					// Optional second intersection
					boost::optional<GPlatesMaths::PointOnSphere> > >
							intersected_segments_type;

	const intersected_segments_type intersected_segments =
			second_visible_section.d_intersection_results->
					intersect_with_previous_section_allowing_two_intersections(
							first_visible_section.d_intersection_results);

	if (!intersected_segments)
	{
		return;
	}

	// Intersection point(s).
	const GPlatesMaths::PointOnSphere &first_intersection =
			boost::get<0>(*intersected_segments);
	const boost::optional<GPlatesMaths::PointOnSphere> &second_intersection =
			boost::get<1>(*intersected_segments);

	// Store the possible intersection point(s) with each section.
	first_visible_section.d_intersection_point_with_prev = first_intersection;
	second_visible_section.d_intersection_point_with_next = first_intersection;
	if (second_intersection)
	{
		first_visible_section.d_intersection_point_with_next = *second_intersection;
		second_visible_section.d_intersection_point_with_prev = *second_intersection;
	}
}


void
GPlatesGui::TopologyTools::process_topological_section_intersection(
		const visible_section_seq_type::size_type first_visible_section_index,
		const visible_section_seq_type::size_type second_visible_section_index)
{
	// Make sure the second visible section follows the first section.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			second_visible_section_index == get_next_index(
					first_visible_section_index, d_visible_boundary_section_seq),
			GPLATES_ASSERTION_SOURCE);

	// If there's only one section we don't want to intersect it with itself.
	if (first_visible_section_index == second_visible_section_index)
	{
		return;
	}

	VisibleSection &first_visible_section = 
		d_visible_boundary_section_seq[first_visible_section_index];
	VisibleSection &second_visible_section = 
		d_visible_boundary_section_seq[second_visible_section_index];

	// If both sections refer to the same geometry then don't intersect.
	// This can happen when the same geometry is added more than once to the topology
	// when it forms different parts of a plate polygon boundary - normally there
	// are other geometries in between but when building topologies it's possible to
	// add the geometry as first section, then add another geometry as second section,
	// then add the first geometry again as the third section and then add another
	// geometry as the fourth section - before the fourth section is added the
	// first and third sections are adjacent and they are the same geometry.
	if (first_visible_section.d_section_geometry_unreversed->get() ==
			second_visible_section.d_section_geometry_unreversed->get())
	{
		return;
	}

	//
	// Process the actual intersection.
	//
	const boost::optional<GPlatesMaths::PointOnSphere> intersection =
			second_visible_section.d_intersection_results->intersect_with_previous_section(
					first_visible_section.d_intersection_results);

	// Store the possible intersection point with each section.
	// Was there an intersection?
	first_visible_section.d_intersection_point_with_next = intersection;
	// Was there an intersection?
	second_visible_section.d_intersection_point_with_prev = intersection;
}


void
GPlatesGui::TopologyTools::determine_boundary_segment_reversals()
{
	PROFILE_FUNC();

	// No need to determine reversal if there's only one visible segment.
	if (d_visible_boundary_section_seq.size() < 2)
	{
		return;
	}

	// Let others know of the change to the reverse flag, that we make below, but
	// we don't want to know about it because otherwise a 'react_entry_modified_*()' will get called.
	// We're automatically determining the reverse flag (and updating the topology section containers)
	// rather than having the user specify the reversal flags for us (like older versions of GPlates)
	// so we don't need to know when the topology section containers have been updated.
	// NOTE: This provides a dramatic speedup when the user uses the topology tools with lots of
	// section (even anything above five sections starts to become noticeable).
	struct AutoReconnectToTopologySectionsContainers
	{
		AutoReconnectToTopologySectionsContainers(
				TopologyTools &topology_tools_) :
			topology_tools(topology_tools_)
		{
			topology_tools.connect_to_boundary_sections_container_signals( false );
			topology_tools.connect_to_interior_sections_container_signals( false );
		}

		~AutoReconnectToTopologySectionsContainers()
		{
			topology_tools.connect_to_boundary_sections_container_signals( true );
			topology_tools.connect_to_interior_sections_container_signals( true );
		}

		TopologyTools &topology_tools;

	} auto_reconnect_topology_sections_containers(*this);

	// Find any contiguous sequences of visible sections that require rubber-banding
	// (ie, don't intersect each other) and that are anchored at the start and end
	// of each sequence by an intersection.
	// Each of these sequences can be optimised separately (ie, test different combinations
	// of reverse flags to see which minimised the rubber-banding distance).
	// The rubber banding distance is the distance from the tail of one section to the
	// head of the next section - added up over the visible sections in a sequence.
	const std::vector< std::vector<section_info_seq_type::size_type> > reverse_visible_section_subsets =
			find_reverse_section_subsets();

	// Iterate over the section sequences.
	std::size_t reverse_section_subset_index;
	for (reverse_section_subset_index = 0;
		reverse_section_subset_index < reverse_visible_section_subsets.size();
		++reverse_section_subset_index)
	{
		const std::vector<visible_section_seq_type::size_type> &reverse_visible_section_subset =
				reverse_visible_section_subsets[reverse_section_subset_index];

		// Get a vector of flags that determine whether to flip the reverse flag of
		// each visible section or not.
		const std::vector<bool> flip_reverse_order_seq =
				find_flip_reverse_order_flags(reverse_visible_section_subset);

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				flip_reverse_order_seq.size() == reverse_visible_section_subset.size(),
				GPLATES_ASSERTION_SOURCE);

		// Now that we've calculated which reverse flags needed to be flipped
		// we can go and flip them.
		std::size_t reverse_section_indices_index;
		for (reverse_section_indices_index = 0;
			reverse_section_indices_index < reverse_visible_section_subset.size();
			++reverse_section_indices_index)
		{
			if (flip_reverse_order_seq[reverse_section_indices_index])
			{
				const visible_section_seq_type::size_type reverse_visible_section_index =
						reverse_visible_section_subset[reverse_section_indices_index];

				// NOTE: Need to call the overload of 'flip_reverse_flag' that accepts a
				// 'VisibleSection' instead of a section index because there can be a mismatch
				// between visible section indices and regular section indices - for example,
				// if a section is not visible.
				VisibleSection &reverse_visible_section =
						d_visible_boundary_section_seq[reverse_visible_section_index];

				flip_reverse_flag(reverse_visible_section);
			}
		}
	}

	//
	// Special case handling for topological *lines* for the first and last visible sections when they
	// intersect their neighbour.
	//
	determine_topological_line_intersecting_end_section_reversals();
}


const std::vector< std::vector<GPlatesGui::TopologyTools::visible_section_seq_type::size_type> > 
GPlatesGui::TopologyTools::find_reverse_section_subsets()
{
	typedef std::vector< std::vector<visible_section_seq_type::size_type> >
			reverse_section_subsets_type;

	const visible_section_seq_type::size_type num_visible_sections = 
		d_visible_boundary_section_seq.size();

	// The first section of the first subset.
	boost::optional<visible_section_seq_type::size_type> index_of_first_section_of_first_subset;

	visible_section_seq_type::size_type visible_section_index;

	bool found_a_valid_section = false;
	bool all_sections_intersect_their_neighbours = true;

	// Iterate over the visible sections to find the first section that only intersects with
	// it's previous section - this is so we have a starting point for our calculations.
	// Also determines if all sections intersect (both) their neighbours.
	for (visible_section_index = 0;
		visible_section_index < num_visible_sections;
		++visible_section_index)
	{
		VisibleSection &visible_section_info = d_visible_boundary_section_seq[visible_section_index];

		found_a_valid_section = true;

		// Determine, if not already, the first section of the first subset.
		if (!index_of_first_section_of_first_subset)
		{
			if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE &&
				visible_section_index == 0)
			{
				// For topological *lines* the first visible section can start a subset if it does not
				// intersect its next neighbour (and it cannot intersect its previous neighbour).
				// This is because there's no circular wraparound for topological lines.
				if (visible_section_info.d_intersection_results->does_not_intersect_previous_or_next_section())
				{
					index_of_first_section_of_first_subset = visible_section_index;
				}
			}
			else if (visible_section_info.d_intersection_results->only_intersects_previous_section())
			{
				index_of_first_section_of_first_subset = visible_section_index;
			}
		}

		// See if the current section intersects (both) its neighbours.
		// For topological lines there's no circular wraparound so the first and last sections
		// are treated differently.
		if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE &&
			visible_section_index == 0)
		{
			// First section in topological line can only intersect with the next section.
			if (!visible_section_info.d_intersection_results->only_intersects_next_section())
			{
				all_sections_intersect_their_neighbours = false;
			}
		}
		else if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE &&
			visible_section_index == num_visible_sections - 1)
		{
			// Last section in topological line can only intersect with the previous section.
			if (!visible_section_info.d_intersection_results->only_intersects_previous_section())
			{
				all_sections_intersect_their_neighbours = false;
			}
		}
		// Interior sections treated same for topological lines and boundaries...
		else if (!visible_section_info.d_intersection_results->intersects_previous_and_next_sections())
		{
			all_sections_intersect_their_neighbours = false;
		}
	}

	// It's possible that no valid sections were found - this can happen when
	// the features referenced by the topology are unloaded by the user.
	if (!found_a_valid_section)
	{
		// No valid features found so we shouldn't proceed.
		// Return empty list.
		return reverse_section_subsets_type();
	}

	// If all visible sections intersect their neighbouring sections then the reversal
	// flags are already determined so we don't have to do anything.
	if (all_sections_intersect_their_neighbours)
	{
		// Return empty list.
		return reverse_section_subsets_type();
	}

	reverse_section_subsets_type reverse_section_subsets;

	if (!index_of_first_section_of_first_subset)
	{
		// If we get here then none of the sections intersect each other (for topological boundaries).
		// So create a single subset that contains all valid sections.
		reverse_section_subsets.resize(1);
		for (visible_section_index = 0;
			visible_section_index < num_visible_sections;
			++visible_section_index)
		{
			reverse_section_subsets.back().push_back(visible_section_index);
		}

		// Don't continue if only one valid section since reversal determination is not needed.
		if (reverse_section_subsets[0].size() < 2)
		{
			// Return empty list.
			return reverse_section_subsets_type();
		}

		return reverse_section_subsets;
	}
	// else there are intersecting sections...

	visible_section_seq_type::size_type section_count;

	// Iterate over the visible sections starting at our new starting position and record
	// contiguous sequences of sections that need reversal processing.
	for (section_count = 0, visible_section_index = index_of_first_section_of_first_subset.get();
		section_count < num_visible_sections;
		++section_count, ++visible_section_index)
	{
		// Test for index wrap around.
		if (visible_section_index == d_visible_boundary_section_seq.size())
		{
			// For topological lines there is no continuation (of rubber-banding) from last to first section.
			// So just break out of the loop.
			if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
			{
				break;
			}

			// For topological boundaries we wraparound and continuing searching.
			visible_section_index = 0;
		}

		VisibleSection &visible_section_info = d_visible_boundary_section_seq[visible_section_index];

		// If start of a subset...
		if (visible_section_index == index_of_first_section_of_first_subset.get() ||
			visible_section_info.d_intersection_results->only_intersects_previous_section())
		{
			// For topological lines we don't start a new subset if it's the last section because
			// if we did then we'd have a subset with only one section in it and there needs to be
			// at least two sections per subset.
			if (d_topology_geometry_type != GPlatesAppLogic::TopologyGeometry::LINE ||
				visible_section_index < num_visible_sections - 1)
			{
				// Start a new subset.
				reverse_section_subsets.push_back(
						std::vector<section_info_seq_type::size_type>());
			}
		}

		// This works for both topological boundaries and lines (even for the last visible section).
		if (!visible_section_info.d_intersection_results->intersects_previous_and_next_sections())
		{
			// Add section to the current subset.
			reverse_section_subsets.back().push_back(visible_section_index);
		}
	}

	return reverse_section_subsets;
}


std::vector<bool>
GPlatesGui::TopologyTools::find_flip_reverse_order_flags(
		const std::vector<visible_section_seq_type::size_type> &reverse_section_subset)
{
	const visible_section_seq_type::size_type parent_visible_section_index =
			reverse_section_subset[0];

	// The parentheses around max prevent windows max macro from stuffing
	// numeric_limits' max.
	double min_length = (std::numeric_limits<double>::max)();
	std::vector<bool> min_length_reverse_order_seq;

	// Try reversing and not-reversing the first section in the sequence.
	for (int flip_parent_reverse_flag = 0;
		flip_parent_reverse_flag < 2;
		++flip_parent_reverse_flag)
	{
		// The parent geometry end points for the current parent reversal flag.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				parent_geometry_start_and_endpoints = get_boundary_sub_segment_end_points(
						parent_visible_section_index, flip_parent_reverse_flag, false/*include_rubber_band_points*/);
		const GPlatesMaths::PointOnSphere &parent_geometry_head =
				parent_geometry_start_and_endpoints.first;
		const GPlatesMaths::PointOnSphere &parent_geometry_tail =
				parent_geometry_start_and_endpoints.second;

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				reverse_section_subset.size() >= 2,
				GPLATES_ASSERTION_SOURCE);

		const visible_section_seq_type::size_type child_depth = 1;
		const visible_section_seq_type::size_type child_visible_section_index =
				reverse_section_subset[child_depth];

		// The child geometry end points if its geometry is flipped.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				child_section_start_and_endpoints_flipped = get_boundary_sub_segment_end_points(
						child_visible_section_index, true, false/*include_rubber_band_points*/);
		const GPlatesMaths::PointOnSphere &child_section_flipped_geometry_head =
				child_section_start_and_endpoints_flipped.first;
		const GPlatesMaths::PointOnSphere &child_section_flipped_geometry_tail =
				child_section_start_and_endpoints_flipped.second;


		// The child geometry end points if its geometry is not flipped.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				child_section_start_and_endpoints_not_flipped = get_boundary_sub_segment_end_points(
						child_visible_section_index, false, false/*include_rubber_band_points*/);
		const GPlatesMaths::PointOnSphere &child_section_not_flipped_geometry_head =
				child_section_start_and_endpoints_not_flipped.first;
		const GPlatesMaths::PointOnSphere &child_section_not_flipped_geometry_tail =
				child_section_start_and_endpoints_not_flipped.second;

		// Recurse to find the two optimal sequences of reversal flags for each geometry
		// in the sequence (excluding the parent geometry).
		// These two sequences are based on whether the child geometry is flipped or not.
		double length_with_child_flipped;
		double length_with_child_not_flipped;
		std::vector<bool> reverse_order_seq_with_child_flipped;
		std::vector<bool> reverse_order_seq_with_child_not_flipped;
		find_reverse_order_subset_to_minimize_rubber_banding(
				length_with_child_flipped,
				length_with_child_not_flipped,
				reverse_order_seq_with_child_flipped,
				reverse_order_seq_with_child_not_flipped,
				reverse_section_subset,
				parent_geometry_head/*start_section_head*/,
				child_section_flipped_geometry_tail,
				child_section_not_flipped_geometry_tail,
				child_depth,
				reverse_section_subset.size()/*total_depth*/);

		// Choose which of the two optimal sequences of reversal flags to use based
		// on the current reversal orientation of the parent geometry.
		double length;
		std::vector<bool> reverse_order_seq;
		choose_child_reverse_order_subset(
				length,
				reverse_order_seq,
				length_with_child_flipped,
				length_with_child_not_flipped,
				reverse_order_seq_with_child_flipped,
				reverse_order_seq_with_child_not_flipped,
				parent_geometry_tail,
				child_section_flipped_geometry_head,
				child_section_not_flipped_geometry_head);

		// Add the reversal flag for the current parent orientation.
		reverse_order_seq.push_back(flip_parent_reverse_flag);

		// Keep track of the most optimal parent reversal.
		if (length < min_length)
		{
			min_length = length;
			min_length_reverse_order_seq = reverse_order_seq;
		}
	}

	// The flip reverse flags were added in the opposite order to
	// the section indices vector so we need to reverse them.
	std::reverse(min_length_reverse_order_seq.begin(), min_length_reverse_order_seq.end());

	return min_length_reverse_order_seq;
}


void
GPlatesGui::TopologyTools::find_reverse_order_subset_to_minimize_rubber_banding(
		double &length_with_current_flipped,
		double &length_with_current_not_flipped,
		std::vector<bool> &reverse_order_seq_with_current_flipped,
		std::vector<bool> &reverse_order_seq_with_current_not_flipped,
		const std::vector<visible_section_seq_type::size_type> &reverse_section_subset,
		const GPlatesMaths::PointOnSphere &start_section_head,
		const GPlatesMaths::PointOnSphere &current_section_flipped_geometry_tail,
		const GPlatesMaths::PointOnSphere &current_section_not_flipped_geometry_tail,
		visible_section_seq_type::size_type current_depth,
		visible_section_seq_type::size_type total_depth)
{
	// Start handling our child node straight away.
	++current_depth;

	if (current_depth >= total_depth)
	{
		// We've reached the end of the sequence.
		if (d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
		{
			// For topological lines return set both lengths to zero.
			//
			// NOTE: We bias the flipped section by a very small amount to avoid causing sections
			// to be flipped when the section geometry is a point (instead of a line) and hence
			// doesn't need flipping. If we don't bias then numerical tolerances can cause some points
			// to be flipped and each flip is currently very expensive since it causes the topological
			// section table GUI to be updated. The small bias (1e-8) corresponds to less than one metre
			// on the Earth's surface and should not affect the flipping decisions for *line* sections.
			length_with_current_flipped = 1e-8 + 0;
			length_with_current_not_flipped = 0;
		}
		else // topological boundary ...
		{
			// For topological boundaries and networks (boundary) we need to complete the circular
			// loop back to the first section's head (when none of the sections intersect).
			// This is only needed when all sections don't intersect.
			// However, when there's one section intersection then that intersection point
			// is a fixed anchor point in the sense that all segments start or end there.
			// Similar thing when more than one intersection.
			// For these cases (non-zero intersections) we don't need to complete the circular loop
			// but we execute the following code anyway because the distance added is the same
			// constant for all reversal paths (in the sequence of sections) so it doesn't
			// affect comparison of the path distances.

			// NOTE: We bias the flipped section by a very small amount to avoid causing sections
			// to be flipped when the section geometry is a point (instead of a line) and hence
			// doesn't need flipping. If we don't bias then numerical tolerances can cause some points
			// to be flipped and each flip is currently very expensive since it causes the topological
			// section table GUI to be updated. The small bias (1e-8) corresponds to less than one metre
			// on the Earth's surface and should not affect the flipping decisions for *line* sections.
			length_with_current_flipped = 1e-8 + acos(dot(
						current_section_flipped_geometry_tail.position_vector(),
						start_section_head.position_vector())).dval();

			length_with_current_not_flipped = acos(dot(
						current_section_not_flipped_geometry_tail.position_vector(),
						start_section_head.position_vector())).dval();
		}

		return;
	}

	const visible_section_seq_type::size_type next_visible_section_index =
			reverse_section_subset[current_depth];

	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			next_section_start_and_endpoints_flipped = get_boundary_sub_segment_end_points(
					next_visible_section_index, true, false/*include_rubber_band_points*/);

	const GPlatesMaths::PointOnSphere &next_section_flipped_geometry_head =
			next_section_start_and_endpoints_flipped.first;
	const GPlatesMaths::PointOnSphere &next_section_flipped_geometry_tail =
			next_section_start_and_endpoints_flipped.second;


	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			next_section_start_and_endpoints_not_flipped = get_boundary_sub_segment_end_points(
					next_visible_section_index, false, false/*include_rubber_band_points*/);

	const GPlatesMaths::PointOnSphere &next_section_not_flipped_geometry_head =
			next_section_start_and_endpoints_not_flipped.first;
	const GPlatesMaths::PointOnSphere &next_section_not_flipped_geometry_tail =
			next_section_start_and_endpoints_not_flipped.second;


	// Recurse to find the two optimal sequences of reversal flags for each geometry
	// in the sequence (excluding the current geometry).
	// These two sequences are based on whether the next geometry is flipped or not.
	double length_with_next_flipped;
	double length_with_next_not_flipped;
	std::vector<bool> reverse_order_seq_with_next_flipped;
	std::vector<bool> reverse_order_seq_with_next_not_flipped;
	find_reverse_order_subset_to_minimize_rubber_banding(
			length_with_next_flipped,
			length_with_next_not_flipped,
			reverse_order_seq_with_next_flipped,
			reverse_order_seq_with_next_not_flipped,
			reverse_section_subset,
			start_section_head,
			next_section_flipped_geometry_tail,
			next_section_not_flipped_geometry_tail,
			current_depth,
			total_depth);

	// Choose which of the two optimal sequences of reversal flags to use based
	// on flipping the current geometry.
	choose_child_reverse_order_subset(
			length_with_current_flipped,
			reverse_order_seq_with_current_flipped,
			length_with_next_flipped,
			length_with_next_not_flipped,
			reverse_order_seq_with_next_flipped,
			reverse_order_seq_with_next_not_flipped,
			current_section_flipped_geometry_tail,
			next_section_flipped_geometry_head,
			next_section_not_flipped_geometry_head);

	// Choose which of the two optimal sequences of reversal flags to use based
	// on not flipping the current geometry.
	choose_child_reverse_order_subset(
			length_with_current_not_flipped,
			reverse_order_seq_with_current_not_flipped,
			length_with_next_flipped,
			length_with_next_not_flipped,
			reverse_order_seq_with_next_flipped,
			reverse_order_seq_with_next_not_flipped,
			current_section_not_flipped_geometry_tail,
			next_section_flipped_geometry_head,
			next_section_not_flipped_geometry_head);
}


void
GPlatesGui::TopologyTools::choose_child_reverse_order_subset(
		double &length,
		std::vector<bool> &reverse_order_seq,
		const double &length_with_next_flipped,
		const double &length_with_next_not_flipped,
		const std::vector<bool> &reverse_order_seq_with_next_flipped,
		const std::vector<bool> &reverse_order_seq_with_next_not_flipped,
		const GPlatesMaths::PointOnSphere &current_section_geometry_tail,
		const GPlatesMaths::PointOnSphere &next_section_flipped_geometry_head,
		const GPlatesMaths::PointOnSphere &next_section_not_flipped_geometry_head)
{
	// Calculate angle between tail of current section and head of next section (flipped).
	//
	// NOTE: We bias the flipped section by a very small amount to avoid causing sections
	// to be flipped when the section geometry is a point (instead of a line) and hence
	// doesn't need flipping. If we don't bias then numerical tolerances can cause some points
	// to be flipped and each flip is currently very expensive since it causes the topological
	// section table GUI to be updated. The small bias (1e-8) corresponds to less than one metre
	// on the Earth's surface and should not affect the flipping decisions for *line* sections.
	const double length_current_with_next_flipped =
			1e-8 + length_with_next_flipped +
			acos(dot(
					current_section_geometry_tail.position_vector(),
					next_section_flipped_geometry_head.position_vector())).dval();

	// Calculate angle between tail of current section and head of next section (not flipped).
	const double length_current_with_next_not_flipped =
			length_with_next_not_flipped +
			acos(dot(
					current_section_geometry_tail.position_vector(),
					next_section_not_flipped_geometry_head.position_vector())).dval();

	if (length_current_with_next_not_flipped < length_current_with_next_flipped)
	{
		length = length_current_with_next_not_flipped;

		reverse_order_seq.reserve(reverse_order_seq_with_next_not_flipped.size() + 1);
		reverse_order_seq.insert(
				reverse_order_seq.end(),
				reverse_order_seq_with_next_not_flipped.begin(),
				reverse_order_seq_with_next_not_flipped.end());
		reverse_order_seq.push_back(false/*next not flipped*/);
	}
	else
	{
		length = length_current_with_next_flipped;

		reverse_order_seq.reserve(reverse_order_seq_with_next_flipped.size() + 1);
		reverse_order_seq.insert(
				reverse_order_seq.end(),
				reverse_order_seq_with_next_flipped.begin(),
				reverse_order_seq_with_next_flipped.end());
		reverse_order_seq.push_back(true/*next flipped*/);
	}
}


void
GPlatesGui::TopologyTools::determine_topological_line_intersecting_end_section_reversals()
{
	//
	// Special case handling for topological *lines* for the first and last visible sections when they
	// intersect their neighbour.
	//
	// This case is not handled by any section subset sequences above.
	//
	// If the first (or last) visible section intersects its sole neighbour then we need a way to
	// determine whether to use its head or tail segment.
	// Currently we just pick the longest segment as that is most likely what the user expects.
	//

	if (d_topology_geometry_type != GPlatesAppLogic::TopologyGeometry::LINE)
	{
		return;
	}

	// No need to determine reversal if there's only one visible segment.
	if (d_visible_boundary_section_seq.size() < 2)
	{
		return;
	}

	// Determine the first visible section intersects its neighbour (can only intersect its next section).
	VisibleSection &first_visible_section = d_visible_boundary_section_seq.front();
	if (first_visible_section.d_intersection_results->only_intersects_next_section())
	{
		// The geometry end points if its geometry is flipped.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				geometry_start_and_endpoints_flipped = get_boundary_sub_segment_end_points(
						0, true, false/*include_rubber_band_points*/);

		// The geometry end points if its geometry is *not* flipped.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				geometry_start_and_endpoints_not_flipped = get_boundary_sub_segment_end_points(
						0, false, false/*include_rubber_band_points*/);

		// Pick the reversal that generates the longest segment.
		if (dot(geometry_start_and_endpoints_flipped.first.position_vector(),
			geometry_start_and_endpoints_flipped.second.position_vector()) <
			dot(geometry_start_and_endpoints_not_flipped.first.position_vector(),
			geometry_start_and_endpoints_not_flipped.second.position_vector()))
		{
			flip_reverse_flag(first_visible_section);
		}
	}

	// Determine the last visible section intersects its neighbour (can only intersect its previous section).
	VisibleSection &last_visible_section = d_visible_boundary_section_seq.back();
	if (last_visible_section.d_intersection_results->only_intersects_previous_section())
	{
		// The geometry end points if its geometry is flipped.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				geometry_start_and_endpoints_flipped = get_boundary_sub_segment_end_points(
						d_visible_boundary_section_seq.size() - 1, true, false/*include_rubber_band_points*/);

		// The geometry end points if its geometry is *not* flipped.
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				geometry_start_and_endpoints_not_flipped = get_boundary_sub_segment_end_points(
						d_visible_boundary_section_seq.size() - 1, false, false/*include_rubber_band_points*/);

		// Pick the reversal that generates the longest segment.
		if (dot(geometry_start_and_endpoints_flipped.first.position_vector(),
			geometry_start_and_endpoints_flipped.second.position_vector()) <
			dot(geometry_start_and_endpoints_not_flipped.first.position_vector(),
			geometry_start_and_endpoints_not_flipped.second.position_vector()))
		{
			flip_reverse_flag(last_visible_section);
		}
	}
}


void
GPlatesGui::TopologyTools::assign_boundary_segments()
{
	const visible_section_seq_type::size_type num_visible_sections = 
		d_visible_boundary_section_seq.size();

	visible_section_seq_type::size_type visible_section_index;
	for (visible_section_index = 0;
		visible_section_index < num_visible_sections;
		++visible_section_index)
	{
		// Get the partitioned segment that will contribute to the plate boundary.
		assign_boundary_segment(visible_section_index);
	}
}


void
GPlatesGui::TopologyTools::assign_boundary_segment(
		const visible_section_seq_type::size_type visible_section_index)
{
	VisibleSection &visible_section = d_visible_boundary_section_seq[visible_section_index];

	// Get the boundary segment for the current section.
	visible_section.d_final_boundary_segment_unreversed_geom =
			visible_section.d_intersection_results->get_sub_segment_geometry();

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours.
	const bool new_reverse_flag = visible_section.d_intersection_results->get_reverse_flag();
	set_boundary_section_reverse_flag(visible_section, new_reverse_flag);

	//
	// Now that we've determined whether to reverse the section or not we can
	// now generate the segment end-points.
	//
	// Get the start and end points of the current section's geometry.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			section_geometry_end_points =
				GPlatesAppLogic::GeometryUtils::get_geometry_exterior_end_points(
						**visible_section.d_section_geometry_unreversed,
						get_boundary_section_info(visible_section).d_table_row.get_reverse());
	// Set the section start and end points.
	visible_section.d_section_start_point = section_geometry_end_points.first;
	visible_section.d_section_end_point = section_geometry_end_points.second;
}


void
GPlatesGui::TopologyTools::assign_interior_segments()
{
	const visible_section_seq_type::size_type num_visible_sections = 
		d_visible_interior_section_seq.size();

	visible_section_seq_type::size_type visible_section_index;
	for (visible_section_index = 0;
		visible_section_index < num_visible_sections;
		++visible_section_index)
	{
		// Get the partitioned segment that will contribute to the plate boundary.
		assign_interior_segment(visible_section_index);
	}
}


void
GPlatesGui::TopologyTools::assign_interior_segment(
		const visible_section_seq_type::size_type visible_section_index)
{
	VisibleSection &visible_section = d_visible_interior_section_seq[visible_section_index];

	// Get the boundary segment for the current section.
	visible_section.d_final_boundary_segment_unreversed_geom =
			visible_section.d_intersection_results->get_sub_segment_geometry();

#if 0
//
// NOTE: don't worry about reverse flags for interior sections; keep this here for reference
//
	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	const bool new_reverse_flag =
			visible_section.d_intersection_results->get_reverse_flag(reverse_flag);
	set_reverse_flag(visible_section.d_section_info_index, new_reverse_flag);
#endif

	//
	// Now that we've determined whether to reverse the section or not we can
	// now generate the segment end-points.
	//
	// Get the start and end points of the current section's geometry.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			section_geometry_end_points =
				GPlatesAppLogic::GeometryUtils::get_geometry_exterior_end_points(
						**visible_section.d_section_geometry_unreversed,
						get_interior_section_info(visible_section).d_table_row.get_reverse());

	// Set the section start and end points.
	visible_section.d_section_start_point = section_geometry_end_points.first;
	visible_section.d_section_end_point = section_geometry_end_points.second;
}


const GPlatesGui::TopologyTools::SectionInfo &
GPlatesGui::TopologyTools::get_boundary_section_info(
		const VisibleSection &visible_section) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			visible_section.d_section_info_index < d_boundary_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_boundary_section_info_seq[visible_section.d_section_info_index];
}

	
GPlatesGui::TopologyTools::SectionInfo &
GPlatesGui::TopologyTools::get_boundary_section_info(
		const VisibleSection &visible_section)
{
	// Reuse const version.
	return const_cast<SectionInfo &>(
			static_cast<const TopologyTools *>(this)->get_boundary_section_info(visible_section));
}


	
const GPlatesGui::TopologyTools::SectionInfo &
GPlatesGui::TopologyTools::get_interior_section_info(
		const VisibleSection &visible_section) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			visible_section.d_section_info_index < d_interior_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_interior_section_info_seq[visible_section.d_section_info_index];
}

	
GPlatesGui::TopologyTools::SectionInfo &
GPlatesGui::TopologyTools::get_interior_section_info(
		const VisibleSection &visible_section)
{
	// Reuse const version.
	return const_cast<SectionInfo &>(
			static_cast<const TopologyTools *>(this)->get_interior_section_info(visible_section));
}




boost::optional<GPlatesGui::TopologyTools::visible_section_seq_type::const_iterator>
GPlatesGui::TopologyTools::is_section_visible_boundary(
		const section_info_seq_type::size_type section_index) const
{
	const visible_section_seq_type::const_iterator visible_section_iter =
			std::find_if(
					d_visible_boundary_section_seq.begin(),
					d_visible_boundary_section_seq.end(),
					boost::bind(&VisibleSection::d_section_info_index, boost::placeholders::_1) ==
						boost::cref(section_index));

	if (visible_section_iter == d_visible_boundary_section_seq.end())
	{
		return boost::none;
	}

	return visible_section_iter;
}

boost::optional<GPlatesGui::TopologyTools::visible_section_seq_type::const_iterator>
GPlatesGui::TopologyTools::is_section_visible_interior(
		const section_info_seq_type::size_type section_index) const
{
	const visible_section_seq_type::const_iterator visible_section_iter =
			std::find_if(
					d_visible_interior_section_seq.begin(),
					d_visible_interior_section_seq.end(),
					boost::bind(&VisibleSection::d_section_info_index, boost::placeholders::_1) ==
						boost::cref(section_index));

	if (visible_section_iter == d_visible_interior_section_seq.end())
	{
		return boost::none;
	}

	return visible_section_iter;
}


void
GPlatesGui::TopologyTools::set_boundary_section_reverse_flag(
		VisibleSection &visible_section_info,
		const bool new_reverse_flag)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			visible_section_info.d_section_info_index < d_boundary_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	SectionInfo &section_info = d_boundary_section_info_seq[visible_section_info.d_section_info_index];

	// An optimisation that only updates the topology sections container if
	// the reverse flag has changed - this is because the topology sections table GUI
	// is very expensive to update and this can be visible as jerkiness when animating
	// the reconstruction time.
	if (new_reverse_flag != section_info.d_table_row.get_reverse())
	{
		flip_reverse_flag(visible_section_info);
	}
}


void
GPlatesGui::TopologyTools::flip_reverse_flag(
		VisibleSection &visible_section_info)
{
	const section_info_seq_type::size_type section_index = visible_section_info.d_section_info_index;

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			section_index < d_boundary_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	SectionInfo &section_info = d_boundary_section_info_seq[section_index];

	// Flip the reverse flag.
	const bool flipped_reverse_flag = !section_info.d_table_row.get_reverse();

	// Set it in the sections container and in the intersection results.
	// It's only used in the intersection results if section does not intersect both its neighbour sections.
	section_info.d_table_row.set_reverse(flipped_reverse_flag);
	visible_section_info.d_intersection_results->set_reverse_hint(flipped_reverse_flag);

	// Let others know of the change to the reverse flag.
	d_boundary_sections_container_ptr->update_at(section_index, section_info.d_table_row);
}


std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
GPlatesGui::TopologyTools::get_boundary_sub_segment_end_points(
		const visible_section_seq_type::size_type visible_section_index,
		const bool flip_reversal_flag,
		const bool include_rubber_band_points)
{
	VisibleSection &visible_section_info = d_visible_boundary_section_seq[visible_section_index];

	// Get the current reverse flag.
	const bool reverse_flag = get_boundary_section_info(visible_section_info).d_table_row.get_reverse();

	// Flip the current reverse flag if requested.
	if (flip_reversal_flag)
	{
		visible_section_info.d_intersection_results->set_reverse_hint(!reverse_flag);
	}

	// NOTE: We must query the intersection results for the boundary sub-segment
	// using the reverse hint instead of just getting the boundary sub-segment
	// directly from the SectionInfo because the reverse hint determines which
	// partitioned sub-segment to use when there is only one intersection.
	const std::pair<GPlatesMaths::PointOnSphere, GPlatesMaths::PointOnSphere> sub_segment_end_points =
			visible_section_info.d_intersection_results
					->get_reversed_sub_segment_end_points(include_rubber_band_points);

	// Reset reverse flag back to original.
	if (flip_reversal_flag)
	{
		visible_section_info.d_intersection_results->set_reverse_hint(reverse_flag);
	}

	return sub_segment_end_points;
}


void
GPlatesGui::TopologyTools::create_topological_sections(
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &topological_sections)
{
	// Iterate over our sections and create a vector of GpmlTopologicalSection objects.
	for (section_info_seq_type::size_type section_index = 0;
		section_index < d_boundary_section_info_seq.size();
		++section_index)
	{
		const SectionInfo &section_info = d_boundary_section_info_seq[section_index];

		// Create the GpmlTopologicalSection property value for the current section.
		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
				topological_section =
						GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_section(
								section_info.d_table_row.get_geometry_property(),
								section_info.d_table_row.get_reverse());

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
		topological_sections.push_back(topological_section.get());
	}
}

void
GPlatesGui::TopologyTools::create_topological_interiors(
		std::vector<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> &topological_interiors)
{
	// Iterate over our interior (sections) and create a vector of GpmlTopologicalNetwork::Interior objects.
	//
	// FIXME: The topological *interiors* are not actually sections (because they don't intersect
	// each other) so we need to have a *non-section* list of interiors.
	for (section_info_seq_type::size_type interior_section_index = 0;
		interior_section_index < d_interior_section_info_seq.size();
		++interior_section_index)
	{
		const SectionInfo &interior_section_info = d_interior_section_info_seq[interior_section_index];

		// Create the GpmlTopologicalNetwork::Interior for the current interior.
		boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
				topological_interior =
						GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_network_interior(
								interior_section_info.d_table_row.get_geometry_property());

		if (!topological_interior)
		{
			qDebug() << "ERROR: ======================================";
			qDebug() << "ERROR: TopologyTools::create_topological_interiors():";
			qDebug() << "ERROR: failed to create GpmlTopologicalNetwork::Interior!";
			qDebug() << "ERROR: continue to next element in Sections Table";
			qDebug() << "ERROR: ======================================";
			continue;
		}

		// Add the GpmlTopologicalNetwork::Interior to the working vector
		topological_interiors.push_back(topological_interior.get());
	}
}


void
GPlatesGui::TopologyTools::show_numbers()
{
	qDebug() << "############################################################"; 
	qDebug() << "TopologyTools::show_numbers: "; 
	qDebug() << "d_boundary_sections_container size() = " << d_boundary_sections_container_ptr->size(); 
	qDebug() << "d_interior_sections_container size() = " << d_interior_sections_container_ptr->size(); 

	//
	// Show details about d_feature_focus_ptr
	//
	if ( d_feature_focus_ptr->is_valid() ) 
	{
		qDebug() << "d_feature_focus_ptr = " << GPlatesUtils::make_qstring_from_icu_string( 
			d_feature_focus_ptr->focused_feature()->feature_id().get() );

		static const GPlatesModel::PropertyName name_property_name = 
			GPlatesModel::PropertyName::create_gml("name");

		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
						d_feature_focus_ptr->focused_feature(), name_property_name);
		if (name)
		{
			qDebug() << "d_feature_focus_ptr name = " << GPlatesUtils::make_qstring(name.get()->value());
		}
		else 
		{
			qDebug() << "d_feature_focus_ptr = INVALID";
		}
	}

	qDebug() << "############################################################"; 
}

void
GPlatesGui::TopologyTools::update_topology_vertices()
{
	// clear the list of all vertices
	d_topology_vertices.clear();

	// fill the d_topology_vertices with boundary vertices
	// build the boundary geom
	update_boundary_vertices();

	// fill the d_topology_vertices with interior vertices
	// build the interior geoms
	update_interior_vertices();
}



void
GPlatesGui::TopologyTools::update_boundary_vertices()
{
	// Iterate over the visible subsegments and append their points to the sequence
	// of topology vertices.
	visible_section_seq_type::const_iterator visible_section_iter = 
		d_visible_boundary_section_seq.begin();

	visible_section_seq_type::const_iterator visible_section_end = 
		d_visible_boundary_section_seq.end();

	for ( ; visible_section_iter != visible_section_end; ++visible_section_iter)
	{
		const VisibleSection &visible_section = *visible_section_iter;

		// Get the vertices from the possibly clipped section geometry
		// and add them to the list of topology vertices.
		GPlatesAppLogic::GeometryUtils::get_geometry_exterior_points(
				*visible_section.d_final_boundary_segment_unreversed_geom.get(),
				d_topology_vertices,
				get_boundary_section_info(visible_section).d_table_row.get_reverse());
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
	d_boundary_geometry_opt_ptr = boost::none;

	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	if (num_topology_points == 0) 
	{
		validity = GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
		d_boundary_geometry_opt_ptr = boost::none;
	} 
	else if (num_topology_points == 1) 
	{
		d_boundary_geometry_opt_ptr = 
			GPlatesUtils::create_point_geometry_on_sphere(d_topology_vertices, validity);
	} 
	else if (num_topology_points == 2 ||
		d_topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE) 
	{
		d_boundary_geometry_opt_ptr = 
			GPlatesUtils::create_polyline_on_sphere(d_topology_vertices, validity);
	} 
	else if (num_topology_points == 3 && d_topology_vertices.front() == d_topology_vertices.back()) 
	{
		d_boundary_geometry_opt_ptr = 
			GPlatesUtils::create_polyline_on_sphere(d_topology_vertices, validity);
	} 
	else 
	{
		d_boundary_geometry_opt_ptr = 
			GPlatesUtils::create_polygon_on_sphere(d_topology_vertices, validity);
	}
}

void
GPlatesGui::TopologyTools::update_interior_vertices()
{
	// tmp list of vertices for this section
	std::vector<GPlatesMaths::PointOnSphere> section_vertices;

	// Iterate over the visible subsegments and append their points to the sequence
	// of topology vertices.
	visible_section_seq_type::const_iterator visible_section_iter = 
		d_visible_interior_section_seq.begin();

	visible_section_seq_type::const_iterator visible_section_end = 
		d_visible_interior_section_seq.end();

	// clear out old pointers
	d_interior_geometry_opt_ptrs.clear();

	for ( ; visible_section_iter != visible_section_end; ++visible_section_iter)
	{
		const VisibleSection &visible_section = *visible_section_iter;

		// Get the vertices from the possibly clipped section geometry
		// and add them to the list of topology vertices.
		GPlatesAppLogic::GeometryUtils::get_geometry_exterior_points(
				*visible_section.d_final_boundary_segment_unreversed_geom.get(),
				d_topology_vertices,
				get_interior_section_info(visible_section).d_table_row.get_reverse());

		// Get the vertices for just this section
		section_vertices.clear();

		GPlatesAppLogic::GeometryUtils::get_geometry_exterior_points(
				*visible_section.d_final_boundary_segment_unreversed_geom.get(),
				section_vertices,
				get_interior_section_info(visible_section).d_table_row.get_reverse());

		// There's no guarantee that adjacent points in the table aren't identical.
		const std::vector<GPlatesMaths::PointOnSphere>::size_type num_points =
				GPlatesMaths::count_distinct_adjacent_points(section_vertices);
	
		// FIXME: I think... we need some way to add data() to the 'header' QTWIs, so that
		// we can immediately discover which bits are supposed to be polygon exteriors etc.
		// Then the function calculate_label_for_item could do all our 'tagging' of
		// geometry parts, and -this- function wouldn't need to duplicate the logic.
		// FIXME 2: We should have a 'try {  } catch {  }' block to catch any exceptions
		// thrown during the instantiation of the geometries.
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr = boost::none;
	
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;
	
		if (num_points == 0) 
		{
			validity = GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
			geometry_opt_ptr = boost::none;
		} 
		else if (num_points == 1) 
		{
			geometry_opt_ptr = 
				GPlatesUtils::create_point_geometry_on_sphere(section_vertices, validity);
		} 
		else if (num_points == 2) 
		{
			geometry_opt_ptr = 
				GPlatesUtils::create_polyline_on_sphere(section_vertices, validity);
		} 
		else if (num_points == 3 && section_vertices.front() == section_vertices.back()) 
		{
			geometry_opt_ptr = 
				GPlatesUtils::create_polyline_on_sphere(section_vertices, validity);
		} 
		else 
		{
			// FIXME: how to differentiate between polyline and polygon? 
			geometry_opt_ptr = 
				GPlatesUtils::create_polyline_on_sphere(section_vertices, validity);
		}

		// append this geometry to the interior list
		d_interior_geometry_opt_ptrs.push_back( geometry_opt_ptr );

	} // end of loop over interior sections

}


boost::optional<GPlatesGui::TopologyTools::VisibleSection>
GPlatesGui::TopologyTools::SectionInfo::reconstruct_section_info_from_table_row(
		std::size_t section_index,
		const double &reconstruction_time,
		bool reverse_hint,
		const std::vector<GPlatesAppLogic::ReconstructHandle::type> &reconstruct_handles) const
{
	// Find the RFG, in the current Reconstruction, for the current topological section.
	boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> section_rg =
			GPlatesAppLogic::TopologyInternalUtils::find_topological_reconstruction_geometry(
					d_table_row.get_geometry_property(),
					reconstruction_time,
					reconstruct_handles);

	// If no RG was found then either:
	// - the feature id could not be resolved (feature not loaded or loaded twice), or
	// - the referenced feature's age range does not include the current reconstruction time.
	// The first condition has already reported an error and the second condition means
	// the current section does not partake in the plate boundary for the current reconstruction time.
	if (!section_rg)
	{
		return boost::none;
	}

	// There are restrictions on the type of topological section depending on the topology geometry
	// type currently being built/edited.
	// But this has already been handled both by classes BuildTopology/EditTopology and by the
	// member function 'can_insert_focused_feature_into_topology()'.
	// So there's no need to apply the restrictions here.

	// See if topological section is an RFG.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> section_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry *>(section_rg.get());
	if (section_rfg)
	{
		// Get the geometry on sphere from the RFG.
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry_unreversed =
				section_rfg.get()->reconstructed_geometry();

		return VisibleSection(section_rg.get(), section_geometry_unreversed, reverse_hint, section_index);
	}

	// See if topological section is an RTG.
	boost::optional<const GPlatesAppLogic::ResolvedTopologicalGeometry *> section_rtg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ResolvedTopologicalGeometry *>(section_rg.get());
	if (section_rtg)
	{
		// Get the geometry on sphere from the RTG.
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry_unreversed =
				section_rtg.get()->resolved_topology_geometry();

		return VisibleSection(section_rg.get(), section_geometry_unreversed, reverse_hint, section_index);
	}

	return boost::none;
}


