/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_MATHS_POLYGONORIENTATION_H
#define GPLATES_MATHS_POLYGONORIENTATION_H


namespace GPlatesMaths
{
	class PolygonOnSphere;

	namespace PolygonOrientation
	{
		//! Orientation of a polygon.
		enum Orientation
		{
			CLOCKWISE,
			COUNTERCLOCKWISE
		};


		/**
		 * Calculates the orientation of the vertices of @a polygon
		 * when viewed from above the globe (looking down on the globe's surface).
		 *
		 * A polygon that is larger than a half-sphere is really a smaller polygon
		 * on the other side of the globe and the viewpoint (for orientation considerations)
		 * will be looking down from the other side of the globe.
		 */
		Orientation
		calculate_polygon_orientation(
				const PolygonOnSphere &polygon);
	}
}

#endif // GPLATES_MATHS_POLYGONORIENTATION_H
