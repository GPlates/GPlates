/* $Id$ */

/**
 * \file Interface for activating/deactivating geometry operations.
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

#include "GeometryOperation.h"

void
GPlatesViewOperations::GeometryOperation::emit_highlight_point_signal(
		GPlatesViewOperations::GeometryBuilder *geometry_builder,
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesGui::Colour &highlight_colour)
{
	if (d_point_is_highlighted)
	{
		// Point is already highlighted.
		// If it's a different point that before then first unhighlight
		// the previous highlighted point and then highlight the new point.

		if (geometry_index == d_highlight_geometry_index &&
			point_index == d_highlight_point_index)
		{
			// Point is already highlighted and it's the same point as before.
			return;
		}

		emit unhighlight_point_in_geometry(geometry_builder,
				d_highlight_geometry_index, d_highlight_point_index);
	}

	d_point_is_highlighted = true;

	d_highlight_geometry_index = geometry_index;
	d_highlight_point_index = point_index;

	emit highlight_point_in_geometry(
			geometry_builder, geometry_index, point_index, highlight_colour);
}


/**
 * If point is currently highlighted then emit a unhighlight signal to listeners.
 */
void
GPlatesViewOperations::GeometryOperation::emit_unhighlight_signal(
		GPlatesViewOperations::GeometryBuilder *geometry_builder)
{
	if (d_point_is_highlighted)
	{
		d_point_is_highlighted = false;

		emit unhighlight_point_in_geometry(geometry_builder,
				d_highlight_geometry_index, d_highlight_point_index);
	}
}
