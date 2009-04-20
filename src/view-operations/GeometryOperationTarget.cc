/* $Id$ */

/**
 * \file 
 * Determines which @a GeometryBuilder each geometry operation canvas tool should target.
 * $Revision$
 * $Date$
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

#include "GeometryOperationTarget.h"
#include "RenderedGeometryCollection.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


namespace
{
	/**
	 * Returns true if @a tool_type is the drag or zoom tool.
	 */
	bool
	is_drag_or_zoom_tool(
			GPlatesCanvasTools::CanvasToolType::Value tool_type)
	{
		return
				tool_type == GPlatesCanvasTools::CanvasToolType::DRAG_GLOBE ||
				tool_type == GPlatesCanvasTools::CanvasToolType::ZOOM_GLOBE;
	}

	/**
	 * Returns true if @a tool_type is a canvas tool that digitises new geometry.
	 */
	bool
	is_digitise_new_geometry_tool(
			GPlatesCanvasTools::CanvasToolType::Value tool_type)
	{
		return
				tool_type == GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYLINE ||
				tool_type == GPlatesCanvasTools::CanvasToolType::DIGITISE_MULTIPOINT ||
				tool_type == GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYGON;
	}

	/**
	 * Returns true if @a tool_type is a canvas tool that performs geometry operations.
	 */
	bool
	is_geometry_operation_tool(
			GPlatesCanvasTools::CanvasToolType::Value tool_type)
	{
		return
				is_digitise_new_geometry_tool(tool_type) ||
				tool_type == GPlatesCanvasTools::CanvasToolType::MOVE_VERTEX ||
				tool_type == GPlatesCanvasTools::CanvasToolType::DELETE_VERTEX ||
				tool_type == GPlatesCanvasTools::CanvasToolType::INSERT_VERTEX;
	}
}

GPlatesViewOperations::GeometryOperationTarget::GeometryOperationTarget(
		GeometryBuilder &digitise_geom_builder,
		GeometryBuilder &focused_feature_geom_builder,
		const GPlatesGui::FeatureFocus &feature_focus,
		const GPlatesGui::ChooseCanvasTool &choose_canvas_tool) :
d_digitise_new_geom_builder(&digitise_geom_builder),
d_focused_feature_geom_builder(&focused_feature_geom_builder),
d_feature_focus(&feature_focus),
d_current_geometry_builder(&digitise_geom_builder)
{
	connect_to_feature_focus(feature_focus);
	connect_to_choose_canvas_tool(choose_canvas_tool);
}

GPlatesViewOperations::GeometryBuilder *
GPlatesViewOperations::GeometryOperationTarget::get_geometry_builder_if_canvas_tool_is_chosen_next(
		GPlatesCanvasTools::CanvasToolType::Value next_canvas_tool) const
{
	// Get a copy of the target chooser so we don't modify any of our own state.
	// This is effectively letting us know what would happen if we actually
	// changed the current canvas tool to 'next_canvas_tool'.
	TargetChooser next_target_chooser(d_target_chooser);

	next_target_chooser.change_canvas_tool(next_canvas_tool);

	return get_target(next_target_chooser);
}

GPlatesViewOperations::GeometryBuilder *
GPlatesViewOperations::GeometryOperationTarget::get_and_set_current_geometry_builder_for_newly_activated_tool(
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type)
{
	d_target_chooser.change_canvas_tool(canvas_tool_type);

	update_current_geometry_builder();

	return d_current_geometry_builder;
}

GPlatesViewOperations::GeometryBuilder *
GPlatesViewOperations::GeometryOperationTarget::get_current_geometry_builder() const
{
	return d_current_geometry_builder;
}

void
GPlatesViewOperations::GeometryOperationTarget::connect_to_feature_focus(
		const GPlatesGui::FeatureFocus &feature_focus)
{
	QObject::connect(
		&feature_focus,
		SIGNAL(focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT(set_focus(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
}

void
GPlatesViewOperations::GeometryOperationTarget::connect_to_choose_canvas_tool(
		const GPlatesGui::ChooseCanvasTool &choose_canvas_tool)
{
	QObject::connect(
		&choose_canvas_tool,
		SIGNAL(chose_canvas_tool(
				GPlatesGui::ChooseCanvasTool &,
				GPlatesCanvasTools::CanvasToolType::Value)),
		this,
		SLOT(chose_canvas_tool(
				GPlatesGui::ChooseCanvasTool &,
				GPlatesCanvasTools::CanvasToolType::Value)));
}

GPlatesViewOperations::GeometryBuilder *
GPlatesViewOperations::GeometryOperationTarget::get_target(
		const TargetChooser &target_chooser) const
{
	switch (target_chooser.get_target_type())
	{
	case TargetChooser::TARGET_DIGITISE_NEW_GEOMETRY:
		return d_digitise_new_geom_builder;

	case TargetChooser::TARGET_FOCUS_GEOMETRY:
		return d_focused_feature_geom_builder;

	case TargetChooser::TARGET_NONE:
	default:
		break;
	}

	return NULL;
}

void
GPlatesViewOperations::GeometryOperationTarget::update_current_geometry_builder()
{
	//
	// See if geometry builder has changed.
	//

	GeometryBuilder *new_geometry_builder = get_target(d_target_chooser);

	if (new_geometry_builder != d_current_geometry_builder)
	{
		d_current_geometry_builder = new_geometry_builder;

		emit switched_geometry_builder(*this, new_geometry_builder);
	}
}

void
GPlatesViewOperations::GeometryOperationTarget::set_focus(
		GPlatesModel::FeatureHandle::weak_ref /*feature_ref*/,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	d_target_chooser.set_focused_geometry(focused_geometry);

	update_current_geometry_builder();
}

void
GPlatesViewOperations::GeometryOperationTarget::chose_canvas_tool(
		GPlatesGui::ChooseCanvasTool &,
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type)
{
	d_target_chooser.change_canvas_tool(canvas_tool_type);

	update_current_geometry_builder();
}


GPlatesViewOperations::GeometryOperationTarget::internal_state_type
GPlatesViewOperations::GeometryOperationTarget::get_internal_state() const
{
	// Currently just store the target chooser as our internal state.
	// It contains boolean focus geometry flag. Currently undo/redo is
	// not supported when the focus geometry changes. But this will need
	// to be looked at again when it is supported.
	return boost::any(d_target_chooser);
}


void
GPlatesViewOperations::GeometryOperationTarget::set_internal_state(
		internal_state_type internal_state)
{
	// Convert from opaque type to TargetChooser type.
	TargetChooser* target_chooser = boost::any_cast<TargetChooser>(&internal_state);

	GPlatesGlobal::Assert(target_chooser != NULL,
		GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	d_target_chooser = *target_chooser;

	// Now that our internal state has changed we need to update so we
	// can notify listeners if the current geometry builder has changed.
	update_current_geometry_builder();
}


GPlatesViewOperations::GeometryOperationTarget::TargetChooser::TargetChooser() :
d_is_geometry_in_focus(false),
d_user_is_digitising_new_geometry(false)
{
}

void
GPlatesViewOperations::GeometryOperationTarget::TargetChooser::set_focused_geometry(
		bool is_geometry_in_focus)
{
	d_is_geometry_in_focus = is_geometry_in_focus;
}

void
GPlatesViewOperations::GeometryOperationTarget::TargetChooser::change_canvas_tool(
		GPlatesCanvasTools::CanvasToolType::Value chosen_canvas_tool)
{
	//
	// This method introduces no hysteresis.
	// That is 'chosen_canvas_tool' will have a one-to-one mapping to
	// 'd_user_is_digitising_new_geometry'.
	// This makes undo/redo easier because undoing a change of canvas tool
	// will return 'd_user_is_digitising_new_geometry' to its previous value
	// automatically.
	// If there was hysteresis then undo would need to be implemented for this class.
	//

	if (d_user_is_digitising_new_geometry)
	{
		// If the user has used a "digitise new geometry" tool and been using
		// "geometry operation" tools ever since (or drag/zoom) then we want to
		// target the new digitised geometry even if there's a feature in focus.
		// Otherwise we will give preference to focused feature geometry.
		if (!is_geometry_operation_tool(chosen_canvas_tool) &&
				!is_drag_or_zoom_tool(chosen_canvas_tool))
		{
			d_user_is_digitising_new_geometry = false;
		}
	}
	else if (is_digitise_new_geometry_tool(chosen_canvas_tool))
	{
		// The user has selected a "digitise new geometry" tool.
		d_user_is_digitising_new_geometry = true;
	}
}

GPlatesViewOperations::GeometryOperationTarget::TargetChooser::TargetType
GPlatesViewOperations::GeometryOperationTarget::TargetChooser::get_target_type() const
{
	// If there is a feature in focus and the user is not currently digitising
	// new geometry then future geometry operations will target the focused
	// feature geometry.
	if (d_user_is_digitising_new_geometry)
	{
		return TARGET_DIGITISE_NEW_GEOMETRY;
	}
	else if (d_is_geometry_in_focus)
	{
		return TARGET_FOCUS_GEOMETRY;
	}

	// Shouldn't be targeting either geometry.
	return TARGET_NONE;
}


GPlatesViewOperations::GeometryOperationTargetUndoCommand::GeometryOperationTargetUndoCommand(
		GeometryOperationTarget *geometry_operation_target,
		QUndoCommand *parent) :
QUndoCommand(parent),
d_geometry_operation_target(geometry_operation_target),
d_internal_state(geometry_operation_target->get_internal_state()),
d_first_redo(true)
{
}


void
GPlatesViewOperations::GeometryOperationTargetUndoCommand::redo()
{
	// Don't do anything the first call to 'redo()'.
	if (d_first_redo)
	{
		d_first_redo = false;
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	d_geometry_operation_target->set_internal_state(d_internal_state);
}


void
GPlatesViewOperations::GeometryOperationTargetUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	d_geometry_operation_target->set_internal_state(d_internal_state);
}
