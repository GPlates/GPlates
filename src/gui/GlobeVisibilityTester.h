/* $Id$ */

/**
 * @file 
 * Contains the defintion of the GlobeVisibilityTester class.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_GLOBEVISIBILITYTESTER_H
#define GPLATES_GUI_GLOBEVISIBILITYTESTER_H

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesGui
{
	class GlobeVisibilityTester
	{
		public:

			/**
			 * Constructs an instance of GlobeVisibilityTester given the instance
			 * of @a globe_canvas used in the main window.
			 */
			explicit
			GlobeVisibilityTester(
					const GPlatesQtWidgets::GlobeCanvas &globe_canvas) :
				d_globe_canvas_ptr(&globe_canvas)
			{
			}

			/**
			 * Returns true iff the @a point_on_sphere is on the near side of the sphere
			 * based on the globe's current camera position.
			 */
			bool
			is_point_visible(
					const GPlatesMaths::PointOnSphere &point_on_sphere);

		private:

			//! A pointer to the GlobeCanvas, through which we can get the camera position
			const GPlatesQtWidgets::GlobeCanvas *d_globe_canvas_ptr;
	};
}

#endif  // GPLATES_GUI_GLOBEVISIBILITYTESTER_H
