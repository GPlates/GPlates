/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GlobeCanvasToolChoice.h"

#include "GlobeCanvasTool.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/ReorientGlobe.h"
#include "canvas-tools/ZoomGlobe.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::GlobeCanvasToolChoice::GlobeCanvasToolChoice(
		Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		GPlatesQtWidgets::ViewportWindow &viewport_window,
		GPlatesPresentation::ViewState &view_state,
		const GPlatesCanvasTools::ClickGeometry::non_null_ptr_type &click_geometry_tool,
		const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type &digitise_polyline_tool,
		const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type &digitise_multipoint_tool,
		const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type &digitise_polygon_tool,
		const GPlatesCanvasTools::MoveVertex::non_null_ptr_type &move_vertex_tool,
		const GPlatesCanvasTools::DeleteVertex::non_null_ptr_type &delete_vertex_tool,
		const GPlatesCanvasTools::InsertVertex::non_null_ptr_type &insert_vertex_tool,
		const GPlatesCanvasTools::SplitFeature::non_null_ptr_type &split_feature_tool,
		const GPlatesCanvasTools::ManipulatePole::non_null_ptr_type &manipulate_pole_tool,
		const GPlatesCanvasTools::BuildTopology::non_null_ptr_type &build_topology_tool,
		const GPlatesCanvasTools::EditTopology::non_null_ptr_type &edit_topology_tool,
		const GPlatesCanvasTools::MeasureDistance::non_null_ptr_type &measure_distance_tool) :
	d_reorient_globe_tool_ptr(
			new GPlatesCanvasTools::ReorientGlobe(
				globe,
				globe_canvas,
				viewport_window)),
	d_zoom_globe_tool_ptr(
			new GPlatesCanvasTools::ZoomGlobe(
				globe,
				globe_canvas,
				viewport_window,
				view_state)),
	d_click_geometry_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				click_geometry_tool,
				globe,
				globe_canvas)),
	d_digitise_polyline_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				digitise_polyline_tool,
				globe,
				globe_canvas)),
	d_digitise_multipoint_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				digitise_multipoint_tool,
				globe,
				globe_canvas)),
	d_digitise_polygon_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				digitise_polygon_tool,
				globe,
				globe_canvas)),
	d_move_vertex_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				move_vertex_tool,
				globe,
				globe_canvas)),
	d_delete_vertex_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				delete_vertex_tool,
				globe,
				globe_canvas)),
	d_insert_vertex_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				insert_vertex_tool,
				globe,
				globe_canvas)),
	d_split_feature_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				split_feature_tool,
				globe,
				globe_canvas)),
	d_manipulate_pole_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				manipulate_pole_tool,
				globe,
				globe_canvas)),
	d_build_topology_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				build_topology_tool,
				globe,
				globe_canvas)),
	d_edit_topology_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				edit_topology_tool,
				globe,
				globe_canvas)),
	d_measure_distance_tool_ptr(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
				measure_distance_tool,
				globe,
				globe_canvas)),
	d_tool_choice_ptr(d_reorient_globe_tool_ptr.get())
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	tool_choice().handle_activation();
}


GPlatesGui::GlobeCanvasToolChoice::~GlobeCanvasToolChoice()
{  }


void
GPlatesGui::GlobeCanvasToolChoice::change_tool_if_necessary(
		GlobeCanvasTool *new_tool_choice)
{
	if (new_tool_choice == d_tool_choice_ptr) {
		// The specified tool is already chosen.  Nothing to do here.
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	d_tool_choice_ptr->handle_deactivation();
	d_tool_choice_ptr = new_tool_choice;
	d_tool_choice_ptr->handle_activation();
}

