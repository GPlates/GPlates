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

#include "ChooseCanvasTool.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometryFinder.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionFeatureProperties.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/TopologyInternalUtils.h"

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
#include "maths/PolylineIntersections.h"
#include "maths/Real.h"

#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ModelUtils.h"

#include "presentation/ReconstructionGeometryRenderer.h"
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
#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


namespace
{
	//! Returns the wrapped previous index before @a index in @a sequence.
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


	//! Returns the wrapped next index after @a index in @a sequence.
	template <class VectorSequenceType>
	typename VectorSequenceType::size_type
	get_next_index(
			typename VectorSequenceType::size_type sequence_index,
			const VectorSequenceType &sequence)
	{
		typename VectorSequenceType::size_type next_sequence_index = sequence_index;

		++next_sequence_index;
		if (next_sequence_index == sequence.size())
		{
			next_sequence_index = 0;
		}

		return next_sequence_index;
	}
}

GPlatesGui::TopologyTools::TopologyTools(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool):
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_feature_focus_ptr(&view_state.get_feature_focus()),
	d_application_state_ptr(&view_state.get_application_state()),
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
		d_application_state_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
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
	//
	// FIXME: Do this check based on feature properties rather than feature type.
	// So if something looks like a TCPB (because it has a topology polygon property)
	// then treat it like one. For this to happen we first need TopologicalNetwork to
	// use a property type different than TopologicalPolygon.
	//
	static const QString topology_boundary_type_name("TopologicalClosedPlateBoundary");
	static const QString topology_network_type_name("TopologicalNetwork");
	const QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		(d_feature_focus_ptr->focused_feature())->feature_type().get_name() );

	if ( feature_type_name == topology_boundary_type_name )
	{ 
		d_topology_type = GPlatesGlobal::PLATE_POLYGON;
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
		d_feature_focus_ptr->associated_reconstruction_geometry(),
		d_topology_type);

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

	// Disconnect from focus signals from Feature Focus.
	connect_to_focus_signals( false );

	// Disconnect from signals from Topology Sections Container.
	connect_to_topology_sections_container_signals( false );

	// Disconnect to recon time changes.
	QObject::disconnect(
		d_application_state_ptr,
		SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
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
	d_end_points_layer_ptr->set_active();
}

void
GPlatesGui::TopologyTools::handle_topology_type_changed(int index)
{
#ifdef DEBUG
std::cout << "GPlatesGui::TopologyTools::handle_topology_type_changed() " << std::endl;
#endif

	if (! d_is_active) { return; }

	if      ( index == 0 ) { d_topology_type = GPlatesGlobal::UNKNOWN_TOPOLOGY; }
	else if ( index == 1 ) { d_topology_type = GPlatesGlobal::PLATE_POLYGON; }
	else if ( index == 2 ) { d_topology_type = GPlatesGlobal::SLAB_POLYGON; }
	else if ( index == 3 ) { d_topology_type = GPlatesGlobal::DEFORMING_POLYGON; }
	else if ( index == 4 ) { d_topology_type = GPlatesGlobal::NETWORK; }
	else                   { d_topology_type = GPlatesGlobal::UNKNOWN_TOPOLOGY; }

	// update the topology
	update_and_redraw_topology();
}

void
GPlatesGui::TopologyTools::handle_mesh(double shape_factor, double max_edge)
{
#ifdef DEBUG
std::cout << "GPlatesGui::TopologyTools::handle_mesh(shape, max_edge)" << shape_factor << "," << max_edge << ";" << std::endl;
#endif
	if (! d_is_active) { return; }

	// Set the values
	d_shape_factor = shape_factor;
	d_max_edge = max_edge;

	// update the topology
	update_and_redraw_topology();
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
	for (section_index = 0; section_index < d_section_info_seq.size(); ++section_index)
	{
		const SectionInfo &section_info = d_section_info_seq[section_index];

		if (!GPlatesAppLogic::TopologyInternalUtils::resolve_feature_id(
				section_info.d_table_row.get_feature_id()).is_valid())
		{
			// Clear the sections container - this will trigger our 'react_cleared' slot.
			d_topology_sections_container_ptr->clear();
			return;
		}
	}
}


void
GPlatesGui::TopologyTools::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	if (! d_is_active ) { return; }

	// If we can insert the focused feature then switch to the section tab
	// so the user can add the focused feature to the topology.
	if (can_insert_focused_feature_into_topology())
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

#ifdef DEBUG
qDebug() << "TopologyTools::display_focused_feature() = ";
qDebug() << "d_feature_focus_ptr = " << GPlatesUtils::make_qstring_from_icu_string(
		d_feature_focus_ptr->focused_feature()->feature_id().get() );

	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name;
	if ( GPlatesFeatureVisitors::get_property_value(
			d_feature_focus_ptr->focused_feature(), name_property_name, name) )
	{
		qDebug() << "name = " << GPlatesUtils::make_qstring( name->value() );
	}

	// feature id
	GPlatesModel::FeatureId id = d_feature_focus_ptr->focused_feature()->feature_id();
	qDebug() << "id = " << GPlatesUtils::make_qstring_from_icu_string( id.get() );

	if ( d_feature_focus_ptr->associated_geometry_property().is_valid() ) { 
		qDebug() << "associated_geometry_property valid " ;
	} else { 
		qDebug() << "associated_geometry_property invalid " ; 
	}
#endif

	// check if the feature is in the topology 
	const std::vector<int> topological_section_indices = find_topological_section_indices(
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());

	// If focused feature found in topology boundary...
	if (!topological_section_indices.empty())
	{
		// Flip to the Topology Sections Table
		d_viewport_window_ptr->choose_topology_sections_table();

		// Pretend we clicked in the row corresponding to the first occurrence
		// of the focused feature geometry in the topology (note that the geometry
		// can occur multiple times in the topology).
	 	const TopologySectionsContainer::size_type container_section_index = 
				static_cast<TopologySectionsContainer::size_type>(
						topological_section_indices[0]);

		d_topology_sections_container_ptr->set_focus_feature_at_index(
				container_section_index);

		return;
	}

	// else, not found on boundary 

	// Flip to the Clicked Features tab
	d_viewport_window_ptr->choose_clicked_geometry_table();
}


std::vector<int>
GPlatesGui::TopologyTools::find_topological_section_indices(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &properties_iter) const
{
	std::vector<int> topological_section_indices;

	if ( !(feature_ref.is_valid() && properties_iter.is_still_valid() ) )
	{
		// Return not found if either feature reference or property iterator is invalid.
		return topological_section_indices;
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
			topological_section_indices.push_back(section_index);
		}
	}

	// Feature id not found.
	return topological_section_indices;
}


bool
GPlatesGui::TopologyTools::can_insert_focused_feature_into_topology() const
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

	// Only insert features that have a ReconstructedFeatureGeometry.
	// The BuildTopology and EditTopology tools filter out everything else
	// so we shouldn't have to test this but we will anyway.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> focused_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry>(
							d_feature_focus_ptr->associated_reconstruction_geometry());
	if (!focused_rfg)
	{
		return false;
	}

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

#if 0
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

	// See if the focused feature is already in the topology.
	const std::vector<int> topology_sections_indices = find_topological_section_indices(
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());

	// If it's not then we can add it.
	if (topology_sections_indices.empty())
	{
		return true;
	}

	//
	// The focused feature is already in the topology.
	// It is possible to have the focused feature geometry occur more than
	// once in the topology under certain conditions:
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
	if (d_section_info_seq.size() <= 2)
	{
		return false;
	}

	// Get the current insertion point 
	const section_info_seq_type::size_type insert_section_index =
			d_topology_sections_container_ptr->insertion_point();

	// Make sure that we don't create a contiguous sequence of three sections with the
	// same geometry when we insert the focused feature geometry.
	if (std::find(topology_sections_indices.begin(), topology_sections_indices.end(),
			static_cast<int>(get_prev_index(insert_section_index, d_section_info_seq)))
				!= topology_sections_indices.end())
	{
		// Cannot insert same geometry after itself.
		return false;
	}
	else if (std::find(topology_sections_indices.begin(), topology_sections_indices.end(), 
				static_cast<int>(insert_section_index)) != topology_sections_indices.end())
	{
		if (std::find(topology_sections_indices.begin(), topology_sections_indices.end(),
				static_cast<int>(get_next_index(insert_section_index, d_section_info_seq)))
					!= topology_sections_indices.end())
		{
			// We'd be inserting in front of two sections with same geometry - not allowed.
			return false;
		}
	}

	return true;
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
	update_and_redraw_topology();
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
	update_and_redraw_topology();
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
	std::transform(
			inserted_begin,
			inserted_end,
			std::inserter(
					d_section_info_seq,
					d_section_info_seq.begin() + inserted_index),
			// Calls the SectionInfo constructor with a TableRow as the argument...
			boost::lambda::bind(
					boost::lambda::constructor<SectionInfo>(),
					boost::lambda::_1));

	// Our internal section sequence should now be in sync with the topology sections container.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_section_info_seq.size() == d_topology_sections_container_ptr->size(),
			GPLATES_ASSERTION_SOURCE);

	// Update topology and redraw.
	update_and_redraw_topology();
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
	update_and_redraw_topology();
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

	// Check if we can insert the focused feature into the topology.
	if (!can_insert_focused_feature_into_topology())
	{
		return;
	}

	// Flip the ViewportWindow to the Topology Sections Table
	d_viewport_window_ptr->choose_topology_sections_table();

	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type rg_ptr =
			d_feature_focus_ptr->associated_reconstruction_geometry();

	// only insert features with a valid RG
	if (! rg_ptr ) { return ; }

	// Only insert features that have a ReconstructedFeatureGeometry.
	// We exclude features with a ResolvedTopologicalBoundary because those features are
	// themselves topological boundaries and we're trying to build a topological boundary
	// from ordinary features.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfg_ptr =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry>(rg_ptr);
	if (!rfg_ptr)
	{
		return;
	}

	// The table row to insert into the topology sections container.
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(
			rfg_ptr.get()->property());

	// Insert the row.
	// NOTE: This will generate a signal that will call our 'react_entries_inserted()'
	// method which will handle the building of our topology data structures.
	d_topology_sections_container_ptr->insert( table_row );

	// NOTE: this will trigger a set_focus signal with NULL ref
	d_feature_focus_ptr->unset_focus();
	// NOTE: the call to unset_focus does not clear the "Clicked" table, so do it here
	d_viewport_window_ptr->feature_table_model().clear();
}


void
GPlatesGui::TopologyTools::handle_remove_feature()
{
	// adjust the mode
	d_in_edit = true;

	// only allow adding of focused features
	if ( ! d_feature_focus_ptr->is_valid() ) 
	{ 
		return;
	}

	const std::vector<int> topological_section_indices = find_topological_section_indices(
			d_feature_focus_ptr->focused_feature(),
			d_feature_focus_ptr->associated_geometry_property());
	
	if ( !topological_section_indices.empty() )	
	{
		int check_index = topological_section_indices[0];
		// remove it from the containter 
		d_topology_sections_container_ptr->remove_at( check_index );

		// NOTE : this will then trigger  our react_entry_removed() 
		// so no further action should be needed here ...
	}
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

	// The topology feature has been modified so that it now behaves as a topological feature.
	// Create any new layers required so the topological feature can be processed.
	const boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference> file_ref =
			GPlatesAppLogic::get_file_reference_containing_feature(
				d_application_state_ptr->get_feature_collection_file_state(),
				d_topology_feature_ref);
	if(file_ref)
	{
		d_application_state_ptr->update_layers(*file_ref);
	}
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
	draw_segments();

	// Don't draw the end points anymore since they were used to give the user
	// an indication of the reversal direction of a segment so the user could change
	// the reversal flag if they wanted, but now that a segment's reversal flag is
	// automatically calculated in code we don't need this anymore (reduces visual clutter).
#if 0
	draw_end_points();
#endif

	draw_intersection_points();
	draw_insertion_neighbors();
	
	// Don't draw the section click points anymore since they are not used to determine
	// the boundary subsegment anymore (reduces visual clutter).
#if 0
	draw_click_points();
#endif
}


void
GPlatesGui::TopologyTools::draw_topology_geometry()
{
	d_topology_geometry_layer_ptr->clear_rendered_geometries();

#if 0
	if ( d_topology_type == GPlatesGlobal::NETWORK )
	{
		// FIXME: eventually we will want the network drawn here too, 
		// but for now, don't draw the network, just let the Resolver do it
	}
	else
#endif
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

	// If no visible sections then nothing to draw.
	if (d_visible_section_seq.empty())
	{
		return;
	}

	// Get the current insertion point in the topology sections container.
	const GPlatesGui::TopologySectionsContainer::size_type insertion_point = 
			d_topology_sections_container_ptr->insertion_point();

	// Index of the geometry just before the insertion point.
	GPlatesGui::TopologySectionsContainer::size_type prev_section_index =
			get_prev_index(insertion_point, d_section_info_seq);

	// Index of the geometry after the insertion point.
	// This is actually the geometry at the insertion point index since we haven't
	// inserted any geometry yet.
	GPlatesGui::TopologySectionsContainer::size_type next_section_index = insertion_point;

	// See if the section before the insertion point is currently visible by searching
	// for its section index in the visible sections.
	boost::optional<visible_section_seq_type::const_iterator> prev_visible_section_iter;
	for (section_info_seq_type::size_type section_count = 0;
		section_count < d_section_info_seq.size();
		++section_count, prev_section_index = get_prev_index(prev_section_index, d_section_info_seq))
	{
		prev_visible_section_iter = is_section_visible(prev_section_index);

		if (prev_visible_section_iter)
		{
			break;
		}
	}

	if (prev_visible_section_iter &&
			prev_visible_section_iter.get()->d_final_boundary_segment_unreversed_geom) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*prev_visible_section_iter.get()->d_section_geometry_unreversed,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
	}


	// See if the section at/after the insertion point is currently visible by searching
	// for its section index in the visible sections.
	boost::optional<visible_section_seq_type::const_iterator> next_visible_section_iter;
	for (section_info_seq_type::size_type section_count = 0;
		section_count < d_section_info_seq.size();
		++section_count, next_section_index = get_next_index(next_section_index, d_section_info_seq))
	{
		next_visible_section_iter = is_section_visible(next_section_index);

		if (next_visible_section_iter)
		{
			break;
		}
	}

	if (next_visible_section_iter &&
			next_visible_section_iter.get()->d_final_boundary_segment_unreversed_geom) 
	{
		const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_black();

		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				*next_visible_section_iter.get()->d_section_geometry_unreversed,
				colour,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_POINT_SIZE_HINT,
				GPlatesViewOperations::RenderedLayerParameters::DEFAULT_LINE_WIDTH_HINT);

		d_insertion_neighbors_layer_ptr->add_rendered_geometry(rendered_geometry);
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

	if ( !d_feature_focus_ptr->associated_reconstruction_geometry() ) { return; }

	// We only accept reconstructed feature geometry's.
	// The BuildTopology and EditTopology tools filter out everything else
	// so we shouldn't have to test this but we will anyway.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> focused_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry>(
							d_feature_focus_ptr->associated_reconstruction_geometry());
	if (!focused_rfg)
	{
		return;
	}

	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::StyleParams render_style_params;
	render_style_params.reconstruction_line_width_hint =
			GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT;
	render_style_params.reconstruction_point_size_hint =
			GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT;

	// This creates the RenderedGeometry's from the ReconstructionGeometry's.
	GPlatesPresentation::ReconstructionGeometryRenderer reconstruction_geometry_renderer(
			*d_focused_feature_layer_ptr,
			render_style_params,
			colour);
	focused_rfg.get()->accept_visitor(reconstruction_geometry_renderer);

	// Get the start and end points of the focused feature's geometry.
	// Since the geometry is in focus but has not been added to the topology
	// there is no information about whether to reverse its points so we don't.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			focus_feature_end_points =
				GPlatesAppLogic::GeometryUtils::get_geometry_end_points(
						*focused_rfg.get()->geometry());

	// draw the focused end_points
	draw_focused_geometry_end_points(
			focus_feature_end_points.first,
			focus_feature_end_points.second);
}


void
GPlatesGui::TopologyTools::draw_focused_geometry_end_points(
		const GPlatesMaths::PointOnSphere &start_point,
		const GPlatesMaths::PointOnSphere &end_point)
{
	const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

	//
	// NOTE: Both end points are drawn the same size now since we don't need
	// to show the user which direction the geometry is because the user does not
	// need to reverse any geometry any more (it's handled automatically now).
	//

	// Create rendered geometry.
	const GPlatesViewOperations::RenderedGeometry start_point_rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			start_point,
			colour,
			GPlatesViewOperations::GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

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

	//
	// Draw the boundary subsegment geometry rather than the entire section geometry.
	// This helps see the topology polygon more clearly and visuals helps the user
	// see if there's some weird polygon boundary that runs to the section endpoint
	// rather than the section intersection point.
	//

	// Iterate over the visible sections and draw the subsegment geometry of each.
	visible_section_seq_type::const_iterator visible_section_iter = d_visible_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = d_visible_section_seq.end();
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


void
GPlatesGui::TopologyTools::draw_end_points()
{
	d_end_points_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &end_points_colour = GPlatesGui::Colour::get_grey();

	// Iterate over the visible sections and draw the start and end point of each segment.
	visible_section_seq_type::const_iterator visible_section_iter = d_visible_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = d_visible_section_seq.end();
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
					GPlatesViewOperations::GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

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

	// Iterate over the visible sections and draw the start intersection of each segment.
	// Since the start intersection of one segment is the same as the end intersection
	// of the previous segment we do not need to draw the end intersection points.
	visible_section_seq_type::const_iterator visible_section_iter = d_visible_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = d_visible_section_seq.end();
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
GPlatesGui::TopologyTools::update_and_redraw_topology()
{
	PROFILE_FUNC();

	//
	// First iterate through the sections and reconstruct the currently visible
	// ones (ie, the ones whose age range contains the current reconstruction time).
	//
	reconstruct_sections();

	//
	// Next iterate through the potential intersections of the visible sections.
	//
	if (d_visible_section_seq.size() == 2)
	{
		// We treat two visible sections as a special case.
		process_two_section_intersections();
	}
	else if (d_visible_section_seq.size() > 2)
	{
		process_intersections();
	}
	// Else if there's only one visible section then don't try to intersect it with itself.

	//
	// Next iterate over all the visible sections and determine if any need to be reversed by
	// minimizing the distance of any rubber-banding.
	// Reversal is only determined for those visible sections that don't intersect *both*
	// adjacent visible sections because they are the visible sections that need rubber-banding.
	// Hence this needs to be done after intersection processing.
	//
	determine_segment_reversals();

	//
	// Next iterate over all the visible sections and assign the boundary subsegments that
	// will form the boundary of the plate polygon.
	//
	assign_boundary_segments();

	// Now that we've updated all the topology subsegments we can
	// create the full set of topology vertices for display.
	update_topology_vertices();

	draw_all_layers();

	// Set the num_sections in the TopologyToolsWidget
	d_topology_tools_widget_ptr->display_number_of_sections(
		d_topology_sections_container_ptr->size() );
}


bool
GPlatesGui::TopologyTools::does_topology_exist_at_current_recon_time() const
{
 	// Check to see if the topology feature is defined for the current reconstruction time.
	// Topology feature reference is non-null when in edit mode and null in build mode.
	if (!d_topology_feature_ref.is_valid())
	{
		// We must be in build mode and haven't yet created the topological polygon feature
		// so assume that it exists for all time.
		return true;
	}

	// See if topology feature exists for the current reconstruction time.
	GPlatesAppLogic::ReconstructionFeatureProperties topology_reconstruction_params(
			d_application_state_ptr->get_current_reconstruction_time());
	topology_reconstruction_params.visit_feature(d_topology_feature_ref);

	return topology_reconstruction_params.is_feature_defined_at_recon_time();
}


void
GPlatesGui::TopologyTools::reconstruct_sections()
{
	// Clear the list of currently visible sections.
	// We're going to repopulate it.
	d_visible_section_seq.clear();

	// Get the reconstruction tree output of the default reconstruction tree layer.
	// FIXME: Later on the user will be able to explicitly connect reconstruction tree layers
	// as input to other layers - when that happens we'll need to handle non-default trees too.
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			d_application_state_ptr->get_current_reconstruction().get_default_reconstruction_tree();

	const section_info_seq_type::size_type num_sections = d_section_info_seq.size();

	section_info_seq_type::size_type section_index;
	for (section_index = 0; section_index < num_sections; ++section_index)
	{
		SectionInfo &section_info = d_section_info_seq[section_index];

		// Create a structure that we can use to do intersection processing and
		// rendering with.
		//
		// Returns false if section is not currently visible because it's age range
		// does not contain the current reconstruction time.
		const boost::optional<VisibleSection> visible_section =
				section_info.reconstruct_section_info_from_table_row(
						section_index,
						*reconstruction_tree);

		if (!visible_section)
		{
			continue;
		}

		// Add to the list of currently visible sections - these are the sections
		// that will be intersection processed.
		d_visible_section_seq.push_back(*visible_section);
	}
}


void
GPlatesGui::TopologyTools::process_intersections()
{
	// If we got here then we have three or more sections in total.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_visible_section_seq.size() > 2,
			GPLATES_ASSERTION_SOURCE);

	const visible_section_seq_type::size_type num_visible_sections = d_visible_section_seq.size();

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
				get_prev_index(visible_section_index, d_visible_section_seq);
		const visible_section_seq_type::size_type next_visible_section_index =
				visible_section_index;

		process_intersection(
				prev_visible_section_index,
				next_visible_section_index);
	}
}


void
GPlatesGui::TopologyTools::process_intersection(
		const visible_section_seq_type::size_type first_visible_section_index,
		const visible_section_seq_type::size_type second_visible_section_index)
{
	// Make sure the second visible section follows the first section.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			second_visible_section_index == get_next_index(
					first_visible_section_index, d_visible_section_seq),
			GPLATES_ASSERTION_SOURCE);

	// If there's only one section we don't want to intersect it with itself.
	if (first_visible_section_index == second_visible_section_index)
	{
		return;
	}

	VisibleSection &first_visible_section = d_visible_section_seq[first_visible_section_index];
	VisibleSection &second_visible_section = d_visible_section_seq[second_visible_section_index];

	// If both sections refer to the same geometry then don't intersect.
	// This can happen when the same geometry is added more than once to the topology
	// when it forms different parts of the plate polygon boundary - normally there
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
			second_visible_section.d_intersection_results.intersect_with_previous_section(
					first_visible_section.d_intersection_results,
					get_section_info(first_visible_section).d_table_row.get_reverse());

	// Store the possible intersection point with each section.
	// Was there an intersection?
	first_visible_section.d_intersection_point_with_next = intersection;
	// Was there an intersection?
	second_visible_section.d_intersection_point_with_prev = intersection;
}


void
GPlatesGui::TopologyTools::process_two_section_intersections()
{
	// If we got here then we have exactly two visible sections in total.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_visible_section_seq.size() == 2,
			GPLATES_ASSERTION_SOURCE);

	//
	// We treat two visible sections as a special case because if they intersect each other
	// exactly twice then we can form a closed polygon from them.
	// NOTE: For three or more sections we don't allow multiple intersections
	// between two adjacent sections.
	//

	VisibleSection &first_visible_section = d_visible_section_seq[0];
	VisibleSection &second_visible_section = d_visible_section_seq[1];

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
			second_visible_section.d_intersection_results.
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
GPlatesGui::TopologyTools::determine_segment_reversals()
{
	// No need to determine reversal if there's only one visible segment.
	if (d_visible_section_seq.size() < 2)
	{
		return;
	}

	// Find any contiguous sequences of visible sections that require rubber-banding
	// (ie, don't intersect each other) and that are anchored at the start and end
	// of each sequence by an intersection.
	// Each of these sequences can be optimised separately (ie, test different combinations
	// of reverse flags to see which minimised the rubber-banding distance).
	// The rubber banding distance is the distance from the tail of one section to the
	// head of the next section - added up over the visible sections in a sequence.
	const std::vector< std::vector<section_info_seq_type::size_type> > reverse_section_subsets =
			find_reverse_section_subsets();

	// Iterate over the section sequences.
	std::size_t reverse_section_subset_index;
	for (reverse_section_subset_index = 0;
		reverse_section_subset_index < reverse_section_subsets.size();
		++reverse_section_subset_index)
	{
		const std::vector<visible_section_seq_type::size_type> &reverse_section_subset =
				reverse_section_subsets[reverse_section_subset_index];

		// Get a vector of flags that determine whether to flip the reverse flag of
		// each section or not.
		const std::vector<bool> flip_reverse_order_seq =
				find_flip_reverse_order_flags(reverse_section_subset);

		// Now that we've calculated which reverse flags needed to be flipped
		// we can go and flip them.
		std::size_t reverse_section_indices_index;
		for (reverse_section_indices_index = 0;
			reverse_section_indices_index < reverse_section_subset.size();
			++reverse_section_indices_index)
		{
			const section_info_seq_type::size_type reverse_section_index =
					reverse_section_subset[reverse_section_indices_index];

			if (flip_reverse_order_seq[reverse_section_indices_index])
			{
				flip_reverse_flag(reverse_section_index);
			}
		}
	}
}


const std::vector< std::vector<GPlatesGui::TopologyTools::visible_section_seq_type::size_type> > 
GPlatesGui::TopologyTools::find_reverse_section_subsets()
{
	typedef std::vector< std::vector<visible_section_seq_type::size_type> >
			reverse_section_subsets_type;

	const visible_section_seq_type::size_type num_visible_sections = d_visible_section_seq.size();

	visible_section_seq_type::size_type index_of_first_section_that_only_intersects_previous_section =
			boost::integer_traits<visible_section_seq_type::size_type>::const_max;

	visible_section_seq_type::size_type visible_section_index;

	bool found_a_valid_section = false;
	bool found_section_that_intersects_previous_and_next_sections = false;

	// Iterate over the visible sections to find the first section that only intersects with
	// it's previous section - this is so we have a starting point for our calculations.
	for (visible_section_index = 0;
		visible_section_index < num_visible_sections;
		++visible_section_index)
	{
		VisibleSection &visible_section_info = d_visible_section_seq[visible_section_index];

		found_a_valid_section = true;

		if (visible_section_info.d_intersection_results.only_intersects_previous_section())
		{
			index_of_first_section_that_only_intersects_previous_section = visible_section_index;
			break;
		}

		if (visible_section_info.d_intersection_results.intersects_previous_and_next_sections())
		{
			found_section_that_intersects_previous_and_next_sections = true;
		}
	}

	reverse_section_subsets_type reverse_section_subsets;

	// If all visible sections intersect both their neighbouring sections then the reversal
	// flags are already determined so we don't have to do anything.
	if (index_of_first_section_that_only_intersects_previous_section ==
			boost::integer_traits<visible_section_seq_type::size_type>::const_max)
	{
		if (found_section_that_intersects_previous_and_next_sections)
		{
			// Return empty list.
			return reverse_section_subsets_type();
		}

		// It's possible that no valid sections were found - this can happen when
		// the features referenced by the topology polygon are unloaded by the user.
		if (!found_a_valid_section)
		{
			// No valid features found so we shouldn't proceed lest we crash.
			// Return empty list.
			return reverse_section_subsets_type();
		}

		// If we get here then none of the sections intersect each other.
		// So create a single subset that contains all valid sections.
		reverse_section_subsets.resize(1);
		for (visible_section_index = 0;
			visible_section_index < num_visible_sections;
			++visible_section_index)
		{
			reverse_section_subsets.back().push_back(visible_section_index);
		}

		// Don't continue if only one valid section as code below requires at least two.
		if (reverse_section_subsets[0].size() < 2)
		{
			// Return empty list.
			return reverse_section_subsets_type();
		}
	}
	else // there are intersecting sections...
	{
		visible_section_seq_type::size_type section_count;

		// Iterate over the visible sections starting at our new starting position and record
		// contiguous sequences of sections that need reversal processing.
		for (section_count = 0, visible_section_index = index_of_first_section_that_only_intersects_previous_section;
			section_count < num_visible_sections;
			++section_count, ++visible_section_index)
		{
			// Test for index wrap around.
			if (visible_section_index == d_visible_section_seq.size())
			{
				visible_section_index = 0;
			}

			VisibleSection &visible_section_info = d_visible_section_seq[visible_section_index];

			if (visible_section_info.d_intersection_results.only_intersects_previous_section())
			{
				reverse_section_subsets.push_back(
						std::vector<section_info_seq_type::size_type>());
			}

			if (!visible_section_info.d_intersection_results.intersects_previous_and_next_sections())
			{
				reverse_section_subsets.back().push_back(visible_section_index);
			}
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
	bool min_length_flip_parent_reverse_flag = false;

	// Try reversing and not-reversing the first section in the sequence.
	for (int flip_parent_reverse_flag = 0;
		flip_parent_reverse_flag < 2;
		++flip_parent_reverse_flag)
	{
		std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
				geometry_start_and_endpoints = get_boundary_geometry_end_points(
						parent_visible_section_index, flip_parent_reverse_flag);
		const GPlatesMaths::PointOnSphere &parent_geometry_head =
				geometry_start_and_endpoints.first;
		const GPlatesMaths::PointOnSphere &parent_geometry_tail =
				geometry_start_and_endpoints.second;

		// Try reversing and not-reversing the second section in the sequence.
		for (int flip_child_reversal_flag = 0;
			flip_child_reversal_flag < 2;
			++flip_child_reversal_flag)
		{
			double length;
			// Recursively try all combinations of reversal flags.
			const std::vector<bool> reverse_order_seq =
					find_reverse_order_subset_to_minimize_rubber_banding(
							length,
							reverse_section_subset,
							flip_child_reversal_flag,
							parent_geometry_head,
							parent_geometry_tail,
							1/*current_depth*/,
							reverse_section_subset.size()/*total_depth*/);

			if (length < min_length)
			{
				min_length = length;
				min_length_reverse_order_seq = reverse_order_seq;
				min_length_flip_parent_reverse_flag = flip_parent_reverse_flag;
			}
		}
	}

	// Add the flip reverse flag for the parent section of recursive descent.
	min_length_reverse_order_seq.push_back(min_length_flip_parent_reverse_flag);

	// The flip reverse flags were added in the opposite order to
	// the section indices vector so we need to reverse them.
	std::reverse(min_length_reverse_order_seq.begin(), min_length_reverse_order_seq.end());

	return min_length_reverse_order_seq;
}


std::vector<bool>
GPlatesGui::TopologyTools::find_reverse_order_subset_to_minimize_rubber_banding(
		double &length,
		const std::vector<visible_section_seq_type::size_type> &reverse_section_subset,
		const bool flip_reversal_flag,
		const GPlatesMaths::PointOnSphere &start_section_head,
		const GPlatesMaths::PointOnSphere &previous_section_tail,
		visible_section_seq_type::size_type current_depth,
		visible_section_seq_type::size_type total_depth)
{
	const visible_section_seq_type::size_type visible_section_index =
			reverse_section_subset[current_depth];

	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			geometry_start_and_endpoints = get_boundary_geometry_end_points(
					visible_section_index, flip_reversal_flag);

	const GPlatesMaths::PointOnSphere &geometry_head = geometry_start_and_endpoints.first;
	const GPlatesMaths::PointOnSphere &geometry_tail = geometry_start_and_endpoints.second;

	// Calculate angle between tail of previous section and head of current section.
	length = acos(dot(
				previous_section_tail.position_vector(),
				geometry_head.position_vector())).dval();

	++current_depth;

	if (current_depth >= total_depth)
	{
		// Complete the circular loop back to the first section's head.
		// This is only needed when all sections don't intersect.
		// When one section intersect once then that intersection point is an
		// fixed anchor point in the sense that all segments start or end there.
		// Similar thing when more than one intersection.
		// For these cases (non-zero intersections) the distance added is the same
		// constant for all reversal paths so it doesn't affect comparison of
		// the path distances.
		length += acos(dot(
					geometry_tail.position_vector(),
					start_section_head.position_vector())).dval();

		return std::vector<bool>(1, flip_reversal_flag);
	}

	double length1;
	std::vector<bool> reverse_order_seq1 = find_reverse_order_subset_to_minimize_rubber_banding(
			length1,
			reverse_section_subset,
			false/*flip_reversal_flag*/,
			start_section_head,
			geometry_tail,
			current_depth,
			total_depth);

	double length2;
	std::vector<bool> reverse_order_seq2 = find_reverse_order_subset_to_minimize_rubber_banding(
			length2,
			reverse_section_subset,
			true/*flip_reversal_flag*/,
			start_section_head,
			geometry_tail,
			current_depth,
			total_depth);

	if (length1 < length2)
	{
		length += length1;
		reverse_order_seq1.push_back(flip_reversal_flag);
		return reverse_order_seq1;
	}

	length += length2;
	reverse_order_seq2.push_back(flip_reversal_flag);
	return reverse_order_seq2;
}


void
GPlatesGui::TopologyTools::assign_boundary_segments()
{
	const visible_section_seq_type::size_type num_visible_sections = d_visible_section_seq.size();

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
	VisibleSection &visible_section = d_visible_section_seq[visible_section_index];

	const bool reverse_flag = get_section_info(visible_section).d_table_row.get_reverse();

	// Get the boundary segment for the current section.
	// The reverse flag passed in is only used if the section did not intersect
	// both its neighbours.
	visible_section.d_final_boundary_segment_unreversed_geom =
			visible_section.d_intersection_results.get_unreversed_boundary_segment(reverse_flag);

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	const bool new_reverse_flag =
			visible_section.d_intersection_results.get_reverse_flag(reverse_flag);
	set_reverse_flag(visible_section.d_section_info_index, new_reverse_flag);

	//
	// Now that we've determined whether to reverse the section or not we can
	// now generate the segment end-points.
	//
	// Get the start and end points of the current section's geometry.
	std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
			section_geometry_end_points =
				GPlatesAppLogic::GeometryUtils::get_geometry_end_points(
						**visible_section.d_section_geometry_unreversed,
						get_section_info(visible_section).d_table_row.get_reverse());
	// Set the section start and end points.
	visible_section.d_section_start_point = section_geometry_end_points.first;
	visible_section.d_section_end_point = section_geometry_end_points.second;
}

	
const GPlatesGui::TopologyTools::SectionInfo &
GPlatesGui::TopologyTools::get_section_info(
		const VisibleSection &visible_section) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			visible_section.d_section_info_index < d_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_section_info_seq[visible_section.d_section_info_index];
}

	
GPlatesGui::TopologyTools::SectionInfo &
GPlatesGui::TopologyTools::get_section_info(
		const VisibleSection &visible_section)
{
	// Reuse const version.
	return const_cast<SectionInfo &>(
			static_cast<const TopologyTools *>(this)->get_section_info(visible_section));
}


boost::optional<GPlatesGui::TopologyTools::visible_section_seq_type::const_iterator>
GPlatesGui::TopologyTools::is_section_visible(
		const section_info_seq_type::size_type section_index) const
{
	const visible_section_seq_type::const_iterator visible_section_iter =
			std::find_if(
					d_visible_section_seq.begin(),
					d_visible_section_seq.end(),
					boost::bind(&VisibleSection::d_section_info_index, _1) ==
						boost::cref(section_index));

	if (visible_section_iter == d_visible_section_seq.end())
	{
		return boost::none;
	}

	return visible_section_iter;
}


void
GPlatesGui::TopologyTools::set_reverse_flag(
		const section_info_seq_type::size_type section_index,
		const bool new_reverse_flag)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			section_index < d_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	SectionInfo &section_info = d_section_info_seq[section_index];

	// An optimisation that only updates the topology sections container if
	// the reverse flag has changed - this is because the topology sections table GUI
	// is very expensive to update and this can be visible as jerkiness when animating
	// the reconstruction time.
	if (new_reverse_flag != section_info.d_table_row.get_reverse())
	{
		flip_reverse_flag(section_index);
	}
}


void
GPlatesGui::TopologyTools::flip_reverse_flag(
		const section_info_seq_type::size_type section_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			section_index < d_section_info_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	SectionInfo &section_info = d_section_info_seq[section_index];

	// Flip the reverse flag.
	section_info.d_table_row.set_reverse(
			!section_info.d_table_row.get_reverse());

	// Let others know of the change to the reverse flag but
	// we don't want to know about it because otherwise our 'react_entry_modified()'
	// will get called.
	// We've automatically determined the reverse flag rather than having
	// the user specify it for us.
	connect_to_topology_sections_container_signals(false);
	d_topology_sections_container_ptr->update_at(section_index, section_info.d_table_row);
	connect_to_topology_sections_container_signals(true);
}


std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
GPlatesGui::TopologyTools::get_boundary_geometry_end_points(
		const visible_section_seq_type::size_type visible_section_index,
		const bool flip_reversal_flag)
{
	VisibleSection &visible_section_info = d_visible_section_seq[visible_section_index];

	const bool reverse_order =
			get_section_info(visible_section_info).d_table_row.get_reverse()
					^ flip_reversal_flag;

	// NOTE: We must query the intersection results for the boundary segment
	// using the reverse flag instead of just getting the boundary segment
	// directly from the SectionInfo because the reverse flag determines which
	// partitioned segment to use when there is only one intersection.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			geometry = visible_section_info.d_intersection_results.
					get_unreversed_boundary_segment(reverse_order);

	// Return the start and end points of the current boundary subsegment.
	return GPlatesAppLogic::GeometryUtils::get_geometry_end_points(
			*geometry, reverse_order);
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
		GPlatesModel::FeatureHandle::iterator iter = feature_ref->begin();
		GPlatesModel::FeatureHandle::iterator end = feature_ref->end();
		// loop over properties
		for ( ; iter != end; ++iter) 
		{
			/*
			// double check for validity and nullness
			if(! iter.is_valid() ){ continue; }
			if( *iter == NULL )   { continue; }  
			*/
			// FIXME: previous edits to the feature leave property pointers NULL

			// passed all checks, make the name and test
			GPlatesModel::PropertyName test_name = (*iter)->property_name();

			if ( test_name == property_name )
			{
				// Delete the old boundary 
				// GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
				feature_ref->remove(iter);
				// transaction.commit();
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

		const GPlatesPropertyValues::GmlTimePeriod *time_period_ptr;
		GPlatesFeatureVisitors::get_property_value(
			topology_feature_ref, valid_time_property_name, time_period_ptr);

		const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type time_period =
				time_period_ptr->deep_clone();

		// Create the TimeWindow
		GPlatesPropertyValues::GpmlTimeWindow tw = GPlatesPropertyValues::GpmlTimeWindow(
				constant_value, 
				time_period,
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
	
	feature_ref->add(
			GPlatesModel::TopLevelPropertyInline::create(
				boundary_prop_name,
				boundary_property_value));

	// Set the ball rolling again ...
	d_application_state_ptr->reconstruct(); 
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

		//
		// We always create a start and end intersection even if a section doesn't intersect
		// at the current reconstruction time.
		// This is because it could intersect at a different reconstruction time.
		// Currently the TopologyBoundaryResolver tries to intersect all sections with
		// their neighbours (ignoring the GpmlTopologicalIntersection) anyway so it doesn't
		// really matter - but we'll do it anyway just to keep things consistent.
		//
		// FIXME: Remove start/end GpmlTopologicalIntersection from the GPGIM.
		//

		// Is there an intersection with the previous section?
		// We say yes always even if not intersecting at current reconstruction time.
		const boost::optional<GPlatesModel::FeatureHandle::iterator> prev_intersection =
				d_section_info_seq[get_prev_index(section_index, d_section_info_seq)]
						.d_table_row.get_geometry_property();

		// Is there an intersection with the next section?
		// We say yes always even if not intersecting at current reconstruction time.
		const boost::optional<GPlatesModel::FeatureHandle::iterator> next_intersection =
				d_section_info_seq[get_next_index(section_index, d_section_info_seq)]
						.d_table_row.get_geometry_property();

		// Create the GpmlTopologicalSection property value for the current section.
		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
				topological_section =
						GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_section(
								section_info.d_table_row.get_geometry_property(),
								section_info.d_table_row.get_reverse(),
								prev_intersection,
								next_intersection);
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
GPlatesGui::TopologyTools::update_topology_vertices()
{
	// FIXME: Only handles the unbroken line and single-ring cases.

	d_topology_vertices.clear();

	// Iterate over the visible subsegments and append their points to the sequence
	// of topology vertices.
	visible_section_seq_type::const_iterator visible_section_iter = d_visible_section_seq.begin();
	visible_section_seq_type::const_iterator visible_section_end = d_visible_section_seq.end();
	for ( ; visible_section_iter != visible_section_end; ++visible_section_iter)
	{
		const VisibleSection &visible_section = *visible_section_iter;

		// Get the vertices from the possibly clipped section geometry
		// and add them to the list of topology vertices.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*visible_section.d_final_boundary_segment_unreversed_geom.get(),
				d_topology_vertices,
				get_section_info(visible_section).d_table_row.get_reverse());
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


GPlatesGui::TopologyTools::VisibleSection::VisibleSection(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry_unreversed,
		const std::size_t section_info_index) :
	d_section_info_index(section_info_index),
	// The section geometry is always the whole unclipped section geometry.
	// This shouldn't change when we do neighbouring section intersection processing.
	d_section_geometry_unreversed(section_geometry_unreversed),
	d_intersection_results(section_geometry_unreversed)
{
}


boost::optional<GPlatesGui::TopologyTools::VisibleSection>
GPlatesGui::TopologyTools::SectionInfo::reconstruct_section_info_from_table_row(
		std::size_t section_index,
		const GPlatesAppLogic::ReconstructionTree &reconstruction_tree) const
{
	// Find the RFG, in the current Reconstruction, for the current topological section.
	boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> section_rfg =
			GPlatesAppLogic::TopologyInternalUtils::find_reconstructed_feature_geometry(
					d_table_row.get_geometry_property(),
					reconstruction_tree);

	// If no RFG was found then either:
	// - the feature id could not be resolved (feature not loaded or loaded twice), or
	// - the referenced feature's age range does not include the current reconstruction time.
	// The first condition has already reported an error and the second condition means
	// the current section does not partake in the plate boundary for the current reconstruction time.
	if (!section_rfg)
	{
		return boost::none;
	}

	// Get the geometry on sphere from the RFG.
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry_unreversed =
			(*section_rfg)->geometry();

	return VisibleSection(section_geometry_unreversed, section_index);
}
