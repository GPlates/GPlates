/**
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

#ifndef GPLATES_GUI_SCENEOVERLAYS_H
#define GPLATES_GUI_SCENEOVERLAYS_H

#include <boost/scoped_ptr.hpp>

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLViewProjection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class TextOverlay;
	class TextOverlaySettings;
	class VelocityLegendOverlay;
	class VelocityLegendOverlaySettings;

	/**
	 * Any overlays that get rendered in 2D, on top of the 3D scene (globe and map).
	 */
	class SceneOverlays :
			public GPlatesUtils::ReferenceCount<SceneOverlays>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<SceneOverlays> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const SceneOverlays> non_null_ptr_to_const_type;

		~SceneOverlays();


		/**
		 * Creates a new @a Scene object.
		 */
		static
		non_null_ptr_type
		create(
				GPlatesPresentation::ViewState &view_state)
		{
			return non_null_ptr_type(new SceneOverlays(view_state));
		}

		/**
		 * Render the 2D overlays.
		 */
		void
		render(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int device_pixel_ratio);

	private:

		//! Text overlay settings.
		const TextOverlaySettings &d_text_overlay_settings;

		//! Renders an optional text overlay onto the scene.
		boost::scoped_ptr<TextOverlay> d_text_overlay;


		//! Velocity legendd overlay settings.
		const VelocityLegendOverlaySettings &d_velocity_legend_overlay_settings;

		//! Renders an optional velocity legend overlay onto the scene.
		boost::scoped_ptr<VelocityLegendOverlay> d_velocity_legend_overlay;


		explicit
		SceneOverlays(
				GPlatesPresentation::ViewState &view_state);
	};
}

#endif // GPLATES_GUI_SCENEOVERLAYS_H
