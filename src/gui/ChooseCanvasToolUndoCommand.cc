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

#include "ChooseCanvasToolUndoCommand.h"


GPlatesGui::ChooseCanvasToolUndoCommand::ChooseCanvasToolUndoCommand(
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		QUndoCommand *parent) :
	QUndoCommand(parent),
	d_canvas_tool_workflows(&canvas_tool_workflows),
	d_canvas_tool(canvas_tool_workflows.get_active_canvas_tool()),
	d_first_redo(true)
{
}


void
GPlatesGui::ChooseCanvasToolUndoCommand::redo()
{
	// Don't do anything the first call to 'redo()' because we're
	// already in the right tool (we just added a point).
	if (d_first_redo)
	{
		d_first_redo = false;
		return;
	}

	// Choose the canvas tool.
	d_canvas_tool_workflows->choose_canvas_tool(
			d_canvas_tool.first/*workflow*/,
			d_canvas_tool.second/*tool*/);
}

void
GPlatesGui::ChooseCanvasToolUndoCommand::undo()
{
	// Choose the canvas tool.
	d_canvas_tool_workflows->choose_canvas_tool(
			d_canvas_tool.first/*workflow*/,
			d_canvas_tool.second/*tool*/);
}
