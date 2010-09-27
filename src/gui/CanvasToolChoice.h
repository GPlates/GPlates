/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_CANVASTOOLCHOICE_H
#define GPLATES_GUI_CANVASTOOLCHOICE_H


namespace GPlatesGui
{
	namespace CanvasToolChoice
	{
		/**
		 * An enumeration that represents the current choice of canvas tool.
		 */
		enum Type
		{
			REORIENT_GLOBE_OR_PAN_MAP,
			ZOOM,
			CLICK_GEOMETRY,
			DIGITISE_POLYLINE,
			DIGITISE_MULTIPOINT,
			DIGITISE_POLYGON,
			MOVE_VERTEX,
			DELETE_VERTEX,
			INSERT_VERTEX,
			SPLIT_FEATURE,
			MANIPULATE_POLE,
			MEASURE_DISTANCE,
			BUILD_TOPOLOGY,
			EDIT_TOPOLOGY
		};
	}
}

#endif  // GPLATES_GUI_CANVASTOOLCHOICE_H
