/* $Id$ */

/**
 * @file 
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

#ifndef GPLATES_GUI_TEXTOVERLAY_H
#define GPLATES_GUI_TEXTOVERLAY_H

#include <QPaintDevice>


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesGui
{
	class TextOverlaySettings;

	/**
	 * TextOverlay is responsible for painting the text overlay onto the globe
	 * or map, in a manner specified by @a TextOverlaySettings.
	 */
	class TextOverlay
	{
	public:

		explicit
		TextOverlay(
				const GPlatesAppLogic::ApplicationState &application_state);

		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const TextOverlaySettings &settings,
				const QPaintDevice &paint_device,
				float scale);

	private:

		const GPlatesAppLogic::ApplicationState &d_application_state;
	};

}

#endif  // GPLATES_GUI_TEXTOVERLAY_H
