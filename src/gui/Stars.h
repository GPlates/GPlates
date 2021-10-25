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

#include "opengl/GLCompiledDrawState.h"
#include "opengl/GLVertexArray.h"


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
	class Colour;

	/**
	 * Draws a random collection of stars in the background in the 3D globe view.
	 */
	class Stars:
			private boost::noncopyable
	{
	public:

		Stars(
				GPlatesOpenGL::GLRenderer &renderer,
				GPlatesPresentation::ViewState &view_state,
				const GPlatesGui::Colour &colour);

		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer);

	private:
		GPlatesPresentation::ViewState &d_view_state;

		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;
		unsigned int d_num_points;
		boost::optional<GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type> d_compiled_draw_state;
	};
}

#endif  // GPLATES_GUI_STARS_H
