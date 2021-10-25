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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYOPERATIONUNDO_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYOPERATIONUNDO_H

#include <memory> // std::auto_ptr
#include <boost/scoped_ptr.hpp>
#include <QUndoCommand>
#include <QString>

#include "view-operations/UndoRedo.h"


namespace GPlatesGui
{
	class CanvasToolWorkflows;
	class ChooseCanvasToolUndoCommand;
}

namespace GPlatesViewOperations
{
	class GeometryOperation;

	/**
	 * Undo/redo command for handling canvas tool choice undo/redo,
	 * geometry operation activation/deactivation and
	 * the specific geometry operation undo/redo itself.
	 *
	 * NOTE: The canvas tool used for undo/redo is the currently active canvas tool.
	 */
	class GeometryOperationUndoCommand :
		public QUndoCommand
	{
	public:
		GeometryOperationUndoCommand(
				const QString &text_,
				std::auto_ptr<QUndoCommand> geometry_operation_command,
				GeometryOperation *geometry_operation,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				UndoRedo::CommandId command_id = UndoRedo::CommandId(),
				QUndoCommand *parent_ = 0);

		~GeometryOperationUndoCommand();

		virtual
		void
		redo();

		virtual
		void
		undo();

		/**
		 * The default returned command id is -1 in which case Qt will not try
		 * to merge commands.
		 */
		virtual
		int
		id() const
		{
			return d_command_id.get_id();
		}

		/**
		 * Merge our geometry operation command with the other geometry operation command.
		 * Returns @a true if merged in which case other command will be
		 * deleted by Qt and this command will coalesce both commands.
		 */
		virtual
		bool
		mergeWith(
				const QUndoCommand *other_command);

	private:
		bool d_first_redo;
		UndoRedo::CommandId d_command_id;
		boost::scoped_ptr<QUndoCommand> d_geometry_operation_command;
		GeometryOperation *d_geometry_operation;
		boost::scoped_ptr<GPlatesGui::ChooseCanvasToolUndoCommand> d_choose_canvas_tool_command;
	};
}

#endif // GPLATES_VIEWOPERATIONS_GEOMETRYOPERATIONUNDO_H
