/* $Id$ */

/**
 * \file 
 * Interface for choosing canvas tools.
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

#ifndef GPLATES_GUI_CHOOSECANVASTOOLUNDOCOMMAND_H
#define GPLATES_GUI_CHOOSECANVASTOOLUNDOCOMMAND_H

#include <QUndoCommand>

#include "CanvasToolWorkflows.h"


namespace GPlatesGui
{
	/**
	 * Undo/redo command for choosing a canvas tool.
	 */
	class ChooseCanvasToolUndoCommand :
			public QUndoCommand
	{
	public:

		/**
		 * The canvas tool used for undo/redo is the currently active canvas tool.
		 */
		explicit
		ChooseCanvasToolUndoCommand(
				CanvasToolWorkflows &canvas_tool_workflows,
				QUndoCommand *parent = 0);

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		CanvasToolWorkflows *d_canvas_tool_workflows;
		std::pair<CanvasToolWorkflows::WorkflowType, CanvasToolWorkflows::ToolType> d_canvas_tool;
		bool d_first_redo;
	};
}

#endif // GPLATES_GUI_CHOOSECANVASTOOLUNDOCOMMAND_H
