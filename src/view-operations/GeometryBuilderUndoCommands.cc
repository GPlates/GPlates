/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include "GeometryBuilderUndoCommands.h"

	
void
GPlatesViewOperations::GeometryBuilderInsertPointUndoCommand::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Add point to geometry builder.
	// This will also cause GeometryBuilder to emit a signal to
	// its observers.
	d_undo_operation = d_geometry_builder.insert_point_into_current_geometry(
		d_point_index_to_insert_at,
		d_oriented_pos_on_globe);
}


void
GPlatesViewOperations::GeometryBuilderInsertPointUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// The undo operation will also cause GeometryBuilder to emit
	// a signal to its observers.
	d_geometry_builder.undo(d_undo_operation);
}


void
GPlatesViewOperations::GeometryBuilderRemovePointUndoCommand::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Remove point from geometry builder.
	// This will also cause GeometryBuilder to emit a signal to
	// its observers.
	d_undo_operation = d_geometry_builder.remove_point_from_current_geometry(
		d_point_index_to_remove_at);
}


void
GPlatesViewOperations::GeometryBuilderRemovePointUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// The undo operation will also cause GeometryBuilder to emit
	// a signal to its observers.
	d_geometry_builder.undo(d_undo_operation);
}


void
GPlatesViewOperations::GeometryBuilderMovePointUndoCommand::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Move point in geometry builder.
	// This will also cause GeometryBuilder to emit a signal to
	// its observers.
	std::vector<GPlatesMaths::PointOnSphere> secondary_points;
	for (int i = 0; i < static_cast<int>(d_secondary_geometries.size()) ; ++i)
	{
		secondary_points.push_back(d_oriented_pos_on_globe);
	}

	d_undo_operation = d_geometry_builder.move_point_in_current_geometry(
		d_point_index_to_move,
		d_oriented_pos_on_globe,
		d_secondary_geometries,
		secondary_points,
		d_is_intermediate_move);

}


void
GPlatesViewOperations::GeometryBuilderMovePointUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// The undo operation will also cause GeometryBuilder to emit
	// a signal to its observers.
	d_geometry_builder.undo(d_undo_operation);
}


bool
GPlatesViewOperations::GeometryBuilderMovePointUndoCommand::mergeWith(
		const QUndoCommand *other_command)
{
	// If other command is same type as us then coalesce its command into us.
	const GeometryBuilderMovePointUndoCommand *other_move_command =
		dynamic_cast<const GeometryBuilderMovePointUndoCommand *>(other_command);

	if (other_move_command != NULL)
	{
		//
		// Merge the other move vertex command with ours.
		//

		// Use the other command's destination vertex position.
		d_oriented_pos_on_globe = other_move_command->d_oriented_pos_on_globe;

		// If the other command is not an intermediate move then the merged
		// command is also not an intermediate move.
		if (!other_move_command->d_is_intermediate_move)
		{
			d_is_intermediate_move = false;
		}

		// But keep our undo operation.

		return true;
	}

	return false;
}


bool
GPlatesViewOperations::GeometryBuilderSetGeometryTypeUndoCommand::mergeWith(
		const QUndoCommand *other_command)
{
	// Is other command same type as us ?
	const GeometryBuilderSetGeometryTypeUndoCommand *other_set_geom_type_command =
		dynamic_cast<const GeometryBuilderSetGeometryTypeUndoCommand *>(other_command);

	if (other_set_geom_type_command != NULL)
	{
		// We use our undo operation for undo'ing but use their geometry type
		// for redo'ing.
		d_geom_type_to_build = other_set_geom_type_command->d_geom_type_to_build;

		return true;
	}

	return false;
}


void
GPlatesViewOperations::GeometryBuilderSetGeometryTypeUndoCommand::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Set geometry type to build in geometry builder.
	// This will also cause GeometryBuilder to emit a signal to
	// its observers.
	d_undo_operation = d_geometry_builder.set_geometry_type_to_build(
		d_geom_type_to_build);
}


void
GPlatesViewOperations::GeometryBuilderSetGeometryTypeUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// The undo operation will also cause GeometryBuilder to emit
	// a signal to its observers.
	d_geometry_builder.undo(d_undo_operation);
}


void
GPlatesViewOperations::GeometryBuilderClearAllGeometries::redo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	d_undo_operation = d_geometry_builder.clear_all_geometries();
}


void
GPlatesViewOperations::GeometryBuilderClearAllGeometries::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	d_geometry_builder.undo(d_undo_operation);
}



