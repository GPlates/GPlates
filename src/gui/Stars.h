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

#ifndef GPLATES_GUI_STARS_H
#define GPLATES_GUI_STARS_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "opengl/GLBuffer.h"
#include "opengl/GLProgram.h"
#include "opengl/GLVertexArray.h"


namespace GPlatesOpenGL
{
	class GL;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class Colour;

	/**
	 * Draws a random collection of stars in the background in the 3D globe view.
	 */
	class Stars:
			private boost::noncopyable
	{
	public:

		Stars(
				GPlatesOpenGL::GL &gl,
				GPlatesPresentation::ViewState &view_state,
				const GPlatesGui::Colour &colour);

		void
		paint(
				GPlatesOpenGL::GL &gl);

	private:
		GPlatesPresentation::ViewState &d_view_state;

		//! Shader program to render stars.
		GPlatesOpenGL::GLProgram::shared_ptr_type d_program;

		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_buffer;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_element_buffer;

		unsigned int d_num_small_star_vertices;
		unsigned int d_num_small_star_indices;
		unsigned int d_num_large_star_vertices;
		unsigned int d_num_large_star_indices;
	};
}

#endif  // GPLATES_GUI_STARS_H
