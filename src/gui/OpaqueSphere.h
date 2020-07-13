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

#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <opengl/OpenGL.h>

#include "Colour.h"

#include "maths/UnitVector3D.h"

#include "opengl/GLCompiledDrawState.h"
#include "opengl/GLProgramObject.h"


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
	class OpaqueSphere :
			public boost::noncopyable
	{
	public:

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
		 *
		 * If @a depth_writes_enabled is true then sphere fragment shader outputs z-buffer depth.
		 * The geometry is a full-screen quad (which does not output the depth of sphere), so it must
		 * be calculated in the fragment shader if it's needed.
		 * This is only needed if depth writes are currently enabled.
		 */
		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				bool depth_writes_enabled = true);

	private:
		const GPlatesPresentation::ViewState &d_view_state;

		Colour d_background_colour;

		//! Shader program to render sphere.
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type> d_program_object;

		//! Used to draw a full-screen quad.
		GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type d_full_screen_quad;
	};
}

#endif  // GPLATES_GUI_OPAQUESPHERE_H
