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

#ifndef GPLATES_GUI_SCENE_H
#define GPLATES_GUI_SCENE_H

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Globe.h"
#include "Map.h"

#include "opengl/GLVisualLayers.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLViewProjection;

	namespace GLIntersect
	{
		class Plane;
	}
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	/**
	 * The scene contains the globe and map.
	 */
	class Scene :
			public GPlatesUtils::ReferenceCount<Scene>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Scene> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Scene> non_null_ptr_to_const_type;


		/**
		 * Typedef for an opaque object that caches a particular rendering.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Creates a new @a Scene object.
		 */
		static
		non_null_ptr_type
		create(
				GPlatesPresentation::ViewState &view_state,
				int device_pixel_ratio)
		{
			return non_null_ptr_type(new Scene(view_state, device_pixel_ratio));
		}


		/**
		 * The OpenGL context has been created.
		 */
		void
		initialise_gl(
				GPlatesOpenGL::GL &gl);

		/**
		 * The OpenGL context is about to be destroyed.
		 */
		void
		shutdown_gl(
				GPlatesOpenGL::GL &gl);


		/**
		 * Render the scene into the globe view.
		 */
		cache_handle_type
		render_globe(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				const double &viewport_zoom_factor,
				const GPlatesOpenGL::GLIntersect::Plane &front_globe_horizon_plane);

		/**
		 * Render the scene into the map view.
		 */
		cache_handle_type
		render_map(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				const double &viewport_zoom_factor);


		GPlatesGui::Globe &
		globe()
		{
			return d_globe;
		}

		const GPlatesGui::Globe &
		globe() const
		{
			return d_globe;
		}


		GPlatesGui::Map &
		map()
		{
			return d_map;
		}

		const GPlatesGui::Map &
		map() const
		{
			return d_map;
		}


		/**
		 * Returns the OpenGL layers used to filled polygons, render rasters and scalar fields.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type
		get_gl_visual_layers()
		{
			return d_gl_visual_layers;
		}

	private:

		//! Keeps track of OpenGL objects that persist from one render to another.
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		//! Holds the globe state.
		Globe d_globe;

		//! Holds the map state.
		Map d_map;


		Scene(
				GPlatesPresentation::ViewState &view_state,
				int device_pixel_ratio);
	};
}


#endif // GPLATES_GUI_SCENE_H
