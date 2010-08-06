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

#include "GLPointLinePolygonState.h"


void
GPlatesOpenGL::GLPointState::enter_state_set() const
{
	if (d_point_smooth)
	{
		if (d_point_smooth.get())
		{
			glEnable(GL_POINT_SMOOTH);
		}
		else
		{
			glDisable(GL_POINT_SMOOTH);
		}
	}
	if (d_point_smooth_hint)
	{
		glHint(GL_POINT_SMOOTH_HINT, d_point_smooth_hint.get());
	}
	if (d_point_size)
	{
		glPointSize(d_point_size.get());
	}
}


void
GPlatesOpenGL::GLPointState::leave_state_set() const
{
	// Set states back to the default state.
	if (d_point_smooth)
	{
		glDisable(GL_POINT_SMOOTH);
	}
	if (d_point_smooth_hint)
	{
		glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
	}
	if (d_point_size)
	{
		glPointSize(1.0f);
	}
}


void
GPlatesOpenGL::GLLineState::enter_state_set() const
{
	if (d_line_smooth)
	{
		if (d_line_smooth.get())
		{
			glEnable(GL_LINE_SMOOTH);
		}
		else
		{
			glDisable(GL_LINE_SMOOTH);
		}
	}
	if (d_line_smooth_hint)
	{
		glHint(GL_LINE_SMOOTH_HINT, d_line_smooth_hint.get());
	}
	if (d_line_width)
	{
		glLineWidth(d_line_width.get());
	}
}


void
GPlatesOpenGL::GLLineState::leave_state_set() const
{
	// Set states back to the default state.
	if (d_line_smooth)
	{
		glDisable(GL_LINE_SMOOTH);
	}
	if (d_line_smooth_hint)
	{
		glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	}
	if (d_line_width)
	{
		glLineWidth(1.0f);
	}
}


void
GPlatesOpenGL::GLPolygonState::enter_state_set() const
{
	if (d_polygon_smooth)
	{
		if (d_polygon_smooth.get())
		{
			glEnable(GL_POLYGON_SMOOTH);
		}
		else
		{
			glDisable(GL_POLYGON_SMOOTH);
		}
	}

	if (d_polygon_smooth_hint)
	{
		glHint(GL_POLYGON_SMOOTH_HINT, d_polygon_smooth_hint.get());
	}

	if (d_polygon_mode)
	{
		glPolygonMode(d_polygon_mode->face, d_polygon_mode->mode);
	}

	if (d_front_face)
	{
		glFrontFace(d_front_face.get());
	}

	if (d_enable_cull_face)
	{
		if (d_enable_cull_face.get())
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
	}

	if (d_cull_face)
	{
		glCullFace(d_cull_face.get());
	}

	if (d_enable_polygon_offset_point)
	{
		if (d_enable_polygon_offset_point.get())
		{
			glEnable(GL_POLYGON_OFFSET_POINT);
		}
		else
		{
			glDisable(GL_POLYGON_OFFSET_POINT);
		}
	}

	if (d_enable_polygon_offset_line)
	{
		if (d_enable_polygon_offset_line.get())
		{
			glEnable(GL_POLYGON_OFFSET_LINE);
		}
		else
		{
			glDisable(GL_POLYGON_OFFSET_LINE);
		}
	}

	if (d_enable_polygon_offset_fill)
	{
		if (d_enable_polygon_offset_fill.get())
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
		}
		else
		{
			glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}

	if (d_polygon_offset)
	{
		glPolygonOffset(d_polygon_offset->factor, d_polygon_offset->units);
	}
}


void
GPlatesOpenGL::GLPolygonState::leave_state_set() const
{
	// Set states back to the default state.
	if (d_polygon_smooth)
	{
		glDisable(GL_POLYGON_SMOOTH);
	}

	if (d_polygon_smooth_hint)
	{
		glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
	}

	if (d_polygon_mode)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (d_front_face)
	{
		glFrontFace(GL_CCW);
	}

	if (d_enable_cull_face)
	{
		glDisable(GL_CULL_FACE);
	}

	if (d_cull_face)
	{
		glCullFace(GL_BACK);
	}

	if (d_enable_polygon_offset_point)
	{
		glDisable(GL_POLYGON_OFFSET_POINT);
	}

	if (d_enable_polygon_offset_line)
	{
		glDisable(GL_POLYGON_OFFSET_LINE);
	}

	if (d_enable_polygon_offset_fill)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (d_polygon_offset)
	{
		glPolygonOffset(0, 0);
	}
}
