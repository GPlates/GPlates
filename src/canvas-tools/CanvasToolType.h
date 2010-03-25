/* $Id$ */

/**
 * \file 
 * 
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

#ifndef GPLATES_CANVASTOOLS_CANVASTOOLTYPE_H
#define GPLATES_CANVASTOOLS_CANVASTOOLTYPE_H

namespace GPlatesCanvasTools
{
	namespace CanvasToolType
	{
		/**
		 * Type of canvas tool.
		 */
		enum Value
		{
			NONE,

			DRAG_GLOBE,
			ZOOM_GLOBE,
			CLICK_GEOMETRY,
			DIGITISE_POLYLINE,
			DIGITISE_MULTIPOINT,
			DIGITISE_POLYGON,
			MOVE_GEOMETRY,
			MOVE_VERTEX,
			DELETE_VERTEX,
			INSERT_VERTEX,
			SPLIT_FEATURE,
			MANIPULATE_POLE,
			BUILD_TOPOLOGY,
			EDIT_TOPOLOGY,
			MEASURE_DISTANCE
		};
	}
}

#endif // GPLATES_CANVASTOOLS_CANVASTOOLTYPE_H
