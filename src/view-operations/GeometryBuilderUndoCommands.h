/* $Id$ */

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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERUNDOCOMMANDS_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERUNDOCOMMANDS_H

#include <QUndoCommand>

#include "GeometryBuilder.h"
#include "RenderedGeometryCollection.h"
#include "UndoRedo.h"
#include "gui/ChooseCanvasTool.h"
#include "maths/PointOnSphere.h"


namespace GPlatesViewOperations
{
	/**
	 * Command to add/undo a point to the current geometry of @a GeometryBuilder.
	 */
	class GeometryBuilderInsertPointUndoCommand :
		public QUndoCommand
	{
	public:
		GeometryBuilderInsertPointUndoCommand(
				GeometryBuilder* digitisation_state_ptr,
				GeometryBuilder::PointIndex point_index_to_insert_at,
				const GPlatesMaths::PointOnSphere& oriented_pos_on_globe,
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_digitisation_state_ptr(digitisation_state_ptr),
		d_point_index_to_insert_at(point_index_to_insert_at),
		d_oriented_pos_on_globe(oriented_pos_on_globe)
		{
			setText(QObject::tr("add point"));
		}

		virtual
		void
		redo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			// Add point to geometry builder.
			// This will also cause GeometryBuilder to emit a signal to
			// its observers.
			d_undo_operation = d_digitisation_state_ptr->insert_point_into_current_geometry(
				d_point_index_to_insert_at,
				d_oriented_pos_on_globe);
		}

		virtual
		void
		undo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			// The undo operation will also cause GeometryBuilder to emit
			// a signal to its observers.
			d_digitisation_state_ptr->undo(d_undo_operation);
		}

	private:
		GeometryBuilder* d_digitisation_state_ptr;
		GeometryBuilder::PointIndex d_point_index_to_insert_at;
		GPlatesMaths::PointOnSphere d_oriented_pos_on_globe;
		GeometryBuilder::UndoOperation d_undo_operation;
	};


	/**
	 * Command to move/undo a point to the current geometry of @a GeometryBuilder.
	 */
	class GeometryBuilderMovePointUndoCommand :
		public QUndoCommand
	{
	public:
		GeometryBuilderMovePointUndoCommand(
				GeometryBuilder* digitisation_state_ptr,
				GeometryBuilder::PointIndex point_index_to_move,
				const GPlatesMaths::PointOnSphere& oriented_pos_on_globe,
				bool is_intermediate_move,
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_digitisation_state_ptr(digitisation_state_ptr),
		d_point_index_to_move(point_index_to_move),
		d_oriented_pos_on_globe(oriented_pos_on_globe),
		d_is_intermediate_move(is_intermediate_move)
		{
			setText(QObject::tr("move vertex"));
		}

		virtual
		void
		redo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			// Move point in geometry builder.
			// This will also cause GeometryBuilder to emit a signal to
			// its observers.
			d_undo_operation = d_digitisation_state_ptr->move_point_in_current_geometry(
				d_point_index_to_move,
				d_oriented_pos_on_globe,
				d_is_intermediate_move);
		}

		virtual
		void
		undo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			// The undo operation will also cause GeometryBuilder to emit
			// a signal to its observers.
			d_digitisation_state_ptr->undo(d_undo_operation);
		}

		/**
		 * Merge this move command with another move command.
		 * Returns @a true if merged in which case other command will be
		 * deleted by Qt and this command will coalesce both commands.
		 * Note that since we don't override the @a id method the Qt undo stack won't
		 * try to merge us by calling @a mergeWith. This method is only used if
		 * called explicitly in our code somewhere.
		 */
		virtual
		bool
		mergeWith(
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

	private:
		GeometryBuilder* d_digitisation_state_ptr;
		GeometryBuilder::PointIndex d_point_index_to_move;
		GPlatesMaths::PointOnSphere d_oriented_pos_on_globe;
		bool d_is_intermediate_move;
		GeometryBuilder::UndoOperation d_undo_operation;
	};


	/**
	 * Command to set/undo the build type for the geometry in @a GeometryBuilder.
	 */
	class GeometryBuilderSetGeometryTypeUndoCommand :
		public QUndoCommand
	{
	public:
		/**
		 * Default command id is -1 which prevents merging of commands.
		 */
		GeometryBuilderSetGeometryTypeUndoCommand(
				GeometryBuilder* digitisation_state_ptr,
				GeometryType::Value geom_type_to_build,
				UndoRedo::CommandId commandId = UndoRedo::CommandId(),
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_digitisation_state_ptr(digitisation_state_ptr),
		d_geom_type_to_build(geom_type_to_build),
		d_commandId(commandId)
		{
			setText(QObject::tr("set geometry type"));
		}

		virtual
		int
		id() const
		{
			return d_commandId.get_id();
		}

		virtual
		bool
		mergeWith(
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

		virtual
		void
		redo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			// Set geometry type to build in geometry builder.
			// This will also cause GeometryBuilder to emit a signal to
			// its observers.
			d_undo_operation = d_digitisation_state_ptr->set_geometry_type_to_build(
				d_geom_type_to_build);
		}

		virtual
		void
		undo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			// The undo operation will also cause GeometryBuilder to emit
			// a signal to its observers.
			d_digitisation_state_ptr->undo(d_undo_operation);
		}

	private:
		GeometryBuilder* d_digitisation_state_ptr;
		GeometryType::Value d_geom_type_to_build;
		GeometryBuilder::UndoOperation d_undo_operation;
		UndoRedo::CommandId d_commandId;
	};


	/**
	* Command to add/undo a point to the current geometry of @a GeometryBuilder.
	*/
	class GeometryBuilderClearAllGeometries :
		public QUndoCommand
	{
	public:
		GeometryBuilderClearAllGeometries(
				GPlatesViewOperations::GeometryBuilder* digitisation_state_ptr,
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_digitisation_state_ptr(digitisation_state_ptr)
		{
			setText(QObject::tr("clear geometry"));
		}

		virtual
		void
		redo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			d_undo_operation = d_digitisation_state_ptr->clear_all_geometries();
		}

		virtual
			void
			undo()
		{
			// Delay any notification of changes to the rendered geometry collection
			// until end of current scope block.
			RenderedGeometryCollection::UpdateGuard update_guard;

			d_digitisation_state_ptr->undo(d_undo_operation);
		}

	private:
		GeometryBuilder* d_digitisation_state_ptr;
		GeometryBuilder::UndoOperation d_undo_operation;
	};
}


#endif	// GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERUNDOCOMMANDS_H
