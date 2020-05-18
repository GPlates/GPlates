/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBEPROJECTIONTYPE_H
#define GPLATES_GUI_GLOBEPROJECTIONTYPE_H

namespace GPlatesGui
{
	namespace GlobeProjection
	{
		/**
		 * An enumeration of globe projection types.
		 */
		enum Type
		{
			ORTHOGRAPHIC,  // 3D orthographic view frustum - view rays are parallel.
			PERSPECTIVE,   // 3D perspective view frustum - view rays emanate from eye/camera position.

			NUM_PROJECTIONS
		};

		/**
		 * Return a suitable label naming the specified projection type.
		 */
		const char *
		get_display_name(
				Type projection_type);
	}
}

#endif // GPLATES_GUI_GLOBEPROJECTIONTYPE_H
