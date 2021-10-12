/* $Id$ */

/**
 * \file 
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
		d_geometry_builder(digitisation_state_ptr),
		d_point_index_to_insert_at(point_index_to_insert_at),
		d_oriented_pos_on_globe(oriented_pos_on_globe)
		{
			setText(QObject::tr("add point"));
		}

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		GeometryBuilder* d_geometry_builder;
		GeometryBuilder::PointIndex d_point_index_to_insert_at;
		GPlatesMaths::PointOnSphere d_oriented_pos_on_globe;
		GeometryBuilder::UndoOperation d_undo_operation;
	};


	/**
	 * Command to remove/undo a point from the current geometry of @a GeometryBuilder.
	 */
	class GeometryBuilderRemovePointUndoCommand :
		public QUndoCommand
	{
	public:
		GeometryBuilderRemovePointUndoCommand(
				GeometryBuilder* geometry_builder,
				GeometryBuilder::PointIndex point_index_to_remove_at,
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_geometry_builder(geometry_builder),
		d_point_index_to_remove_at(point_index_to_remove_at)
		{
			setText(QObject::tr("remove point"));
		}

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		GeometryBuilder* d_geometry_builder;
		GeometryBuilder::PointIndex d_point_index_to_remove_at;
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
				GeometryBuilder* geometry_builder,
				GeometryBuilder::PointIndex point_index_to_move,
				const GPlatesMaths::PointOnSphere& oriented_pos_on_globe,
				bool is_intermediate_move,
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_geometry_builder(geometry_builder),
		d_point_index_to_move(point_index_to_move),
		d_oriented_pos_on_globe(oriented_pos_on_globe),
		d_is_intermediate_move(is_intermediate_move)
		{
			setText(QObject::tr("move vertex"));
		}

		virtual
		void
		redo();

		virtual
		void
		undo();

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
				const QUndoCommand *other_command);
	private:
		GeometryBuilder* d_geometry_builder;
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
				GeometryBuilder* geometry_builder,
				GeometryType::Value geom_type_to_build,
				UndoRedo::CommandId commandId = UndoRedo::CommandId(),
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_geometry_builder(geometry_builder),
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
				const QUndoCommand *other_command);

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		GeometryBuilder* d_geometry_builder;
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
				GPlatesViewOperations::GeometryBuilder* geometry_builder,
				QUndoCommand *parent = 0) :
		QUndoCommand(parent),
		d_geometry_builder(geometry_builder)
		{
			setText(QObject::tr("clear geometry"));
		}

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		GeometryBuilder* d_geometry_builder;
		GeometryBuilder::UndoOperation d_undo_operation;
	};
}


#endif	// GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERUNDOCOMMANDS_H
