/* $Id$ */

/**
 * \file Undo/redo command to handle geometry operations.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "GeometryOperationUndo.h"

#include "GeometryOperation.h"

#include "gui/ChooseCanvasTool.h"
#include "view-operations/GeometryOperationTarget.h"


GPlatesViewOperations::GeometryOperationUndoCommand::GeometryOperationUndoCommand(
		const QString &text_,
		std::auto_ptr<QUndoCommand> geometry_operation_command,
		GeometryOperation *geometry_operation,
		GeometryOperationTarget *geometry_operation_target,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		GPlatesGui::ChooseCanvasTool *choose_canvas_tool,
		void (GPlatesGui::ChooseCanvasTool::*choose_geometry_operation_tool)(),
		UndoRedo::CommandId command_id,
		QUndoCommand *parent_) :
	QUndoCommand(text_, parent_),
	d_first_redo(true),
	d_command_id(command_id),
	d_geometry_operation_command(geometry_operation_command.release()),
	d_geometry_operation(geometry_operation),
	d_geometry_operation_target(geometry_operation_target),
	d_geometry_operation_target_undo_command(
			new GeometryOperationTargetUndoCommand(geometry_operation_target)),
	d_main_layer_type(main_layer_type),
	d_choose_canvas_tool_command(
		// Add undo command for selecting the geometry operation tool.
		new GPlatesGui::ChooseCanvasToolUndoCommand(
				choose_canvas_tool, choose_geometry_operation_tool))
{
}


GPlatesViewOperations::GeometryOperationUndoCommand::~GeometryOperationUndoCommand()
{
	// boost::scoped_ptr destructor requires class declaration.
}

void
GPlatesViewOperations::GeometryOperationUndoCommand::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Visit child commands.
	//
	// 1) Restore the GeometryOperationTarget state to what it was.
	//    This is done before changing the canvas tool because the GeometryOperationTarget
	//    listens for tool changes and uses this to determine the current geometry builder.
	// 2) Activate canvas tool - shows appropriate high-level GUI stuff.
	// 3) Activate geometry operation so it's in same state as when
	//    it performed the original operation.
	//    This needs to done because the canvas tool, and hence the geometry operation,
	//    only changes geometry builder target when the tool changes and the tool
	//    might not change (eg, the user might do a move vertex on the focused geometry
	//    and then do a move vertex on the digitised geometry - so the tool hasn't
	//    changed but the geometry builder target has).
	// 4) Redo the geometry operation.

	d_geometry_operation_target_undo_command->redo();
	d_choose_canvas_tool_command->redo();

	// Don't do anything the first call to 'redo()' because the
	// geometry operation is already activated.
	if (!d_first_redo)
	{
		d_geometry_operation->deactivate();
		// Activate using the geometry builder determined by the
		// GeometryOperationTarget - this should be the same geometry builder
		// that was targetted in the initial operation.
		d_geometry_operation->activate(
				d_geometry_operation_target->get_current_geometry_builder(),
				d_main_layer_type);
	}

	d_geometry_operation_command->redo();

	d_first_redo = false;
}


void
GPlatesViewOperations::GeometryOperationUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Visit child commands.
	//
	// 1) Restore the GeometryOperationTarget state to what it was.
	//    This is done before changing the canvas tool because the GeometryOperationTarget
	//    listens for tool changes and uses this to determine the current geometry builder.
	// 2) Activate canvas tool - shows appropriate high-level GUI stuff.
	// 3) Activate geometry operation so it's in same state as when
	//    it performed the original operation.
	//    This needs to done because the canvas tool, and hence the geometry operation,
	//    only changes geometry builder target when the tool changes and the tool
	//    might not change (eg, the user might do a move vertex on the focused geometry
	//    and then do a move vertex on the digitised geometry - so the tool hasn't
	//    changed but the geometry builder target has).
	// 4) Undo the geometry operation.
	d_geometry_operation_target_undo_command->undo();
	d_choose_canvas_tool_command->undo();

	d_geometry_operation->deactivate();
	// Activate using the geometry builder determined by the
	// GeometryOperationTarget - this should be the same geometry builder
	// that was targetted in the initial operation.
	d_geometry_operation->activate(
			d_geometry_operation_target->get_current_geometry_builder(),
			d_main_layer_type);

	d_geometry_operation_command->undo();
}


bool
GPlatesViewOperations::GeometryOperationUndoCommand::mergeWith(
		const QUndoCommand *other_command)
{
	// If other command is same type as us then coalesce its command into us.
	const GeometryOperationUndoCommand *other_geometry_operation_command =
		dynamic_cast<const GeometryOperationUndoCommand *>(other_command);

	if (other_geometry_operation_command != NULL)
	{
		// Merge the other geometry operation command with ours.
		// Currently only the move vertex geometry operation uses this.
		return d_geometry_operation_command->mergeWith(
				other_geometry_operation_command->d_geometry_operation_command.get());

		// Forget about the other select canvas tool command as it does
		// the same as ours.
	}

	return false;
}
