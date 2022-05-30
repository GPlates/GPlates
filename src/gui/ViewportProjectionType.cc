/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include "ViewportProjectionType.h"

#include "global/GPlatesAssert.h"


const char *
GPlatesGui::ViewportProjection::get_display_name(
		Type projection_type)
{
	switch (projection_type)
	{
	case ORTHOGRAPHIC:
		return "Orthographic";
	case PERSPECTIVE:
		return "Perspective";
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}
