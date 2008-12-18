/* $Id$ */

/**
 * \file 
 * Parameters that specify how geometry operations should draw geometry.
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

#include "GeometryOperationRenderParameters.h"

GPlatesViewOperations::GeometryOperationRenderParameters::GeometryOperationRenderParameters(
		float line_width_hint,
		float regular_point_size_hint,
		float large_point_size_hint,
		const GPlatesGui::Colour &focus_colour,
		const GPlatesGui::Colour &not_in_focus_colour) :
d_line_width_hint(line_width_hint),
d_regular_point_size_hint(regular_point_size_hint),
d_large_point_size_hint(large_point_size_hint),
d_focus_colour(focus_colour),
d_not_in_focus_colour(not_in_focus_colour)
{
}
