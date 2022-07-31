/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_MAPBACKGROUND_H
#define GPLATES_GUI_MAPBACKGROUND_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "Colour.h"
#include "MapProjection.h"

#include "opengl/GLBuffer.h"
#include "opengl/GLProgram.h"
#include "opengl/GLVertexArray.h"
#include "opengl/OpenGL.h"  // For Class GL and the OpenGL constants/typedefs


namespace GPlatesOpenGL
{
	class GLViewProjection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	/**
	 * Renders a coloured background map in the map view.
	 */
	class MapBackground :
			private boost::noncopyable
	{
	public:

		/**
		 * Constructs a background with a constant @a colour.
		 */
		explicit
		MapBackground(
				GPlatesOpenGL::GL &gl,
				const MapProjection &map_projection,
				const Colour &colour);

		/**
		 * Constructs a background that uses the background colour of @a view_state,
		 * as it changes from time to time.
		 */
		explicit
		MapBackground(
				GPlatesOpenGL::GL &gl,
				const MapProjection &map_projection,
				const GPlatesPresentation::ViewState &view_state);

		/**
		 * Paints the map background.
		 */
		void
		paint(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection);

	private:

		const MapProjection& d_map_projection;

		//! The background colour obtained from view state if we were constructed to use it.
		const GPlatesPresentation::ViewState *d_view_state;

		//! The background colour if we were constructed to use a constant colour.
		boost::optional<Colour> d_constant_colour;

		boost::optional<Colour> d_last_seen_colour;
		boost::optional<MapProjectionSettings> d_last_seen_map_projection_settings;

		//! Shader program to render grid lines.
		GPlatesOpenGL::GLProgram::shared_ptr_type d_program;

		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_buffer;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_element_buffer;

		unsigned int d_num_vertex_indices;
	};
}

#endif // GPLATES_GUI_MAPBACKGROUND_H
