/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_OPAQUESPHERE_H
#define GPLATES_GUI_OPAQUESPHERE_H

#include <boost/noncopyable.hpp>
#include <opengl/OpenGL.h>

#include "Colour.h"

#include "maths/UnitVector3D.h"

#include "opengl/GLCompiledDrawState.h"
#include "opengl/GLVertexArray.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class OpaqueSphere :
			public boost::noncopyable
	{
	public:

		/**
		 * Constructs an OpaqueSphere with a fixed @a colour.
		 */
		explicit
		OpaqueSphere(
				GPlatesOpenGL::GLRenderer &renderer,
				const Colour &colour);

		/**
		 * Constructs an OpaqueSphere that uses the background colour of @a view_state,
		 * as it changes from time to time.
		 */
		explicit
		OpaqueSphere(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesPresentation::ViewState &view_state);

		/**
		 * Paints sphere.
		 */
		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesMaths::UnitVector3D &axis,
				double angle_in_deg);

	private:
		const GPlatesPresentation::ViewState *d_view_state;
		Colour d_colour;

		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;
		GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type d_compiled_draw_state;
	};
}

#endif  // GPLATES_GUI_OPAQUESPHERE_H
