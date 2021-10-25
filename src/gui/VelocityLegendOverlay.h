/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2017 Geological Survey of Norway
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

#ifndef GPLATES_GUI_VELOCITYLEGENDOVERLAY_H
#define GPLATES_GUI_VELOCITYLEGENDOVERLAY_H

namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class VelocityLegendOverlaySettings;

	/**
	 * TextOverlay is responsible for painting the text overlay onto the globe
	 * or map, in a manner specified by @a TextOverlaySettings.
	 */
	class VelocityLegendOverlay
	{
	public:

		explicit
		VelocityLegendOverlay();

		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const VelocityLegendOverlaySettings &settings,
				int paint_device_width,
				int paint_device_height,
				float scale);

	private:

	};

}

#endif  // GPLATES_GUI_VELOCITYLEGENDOVERLAY_H
