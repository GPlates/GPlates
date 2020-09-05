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

#ifndef GPLATES_GUI_BACKGROUNDSPHERE_H
#define GPLATES_GUI_BACKGROUNDSPHERE_H

#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

#include "Colour.h"

#include "maths/UnitVector3D.h"

#include "opengl/GLProgram.h"
#include "opengl/GLVertexArray.h"


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
	class BackgroundSphere :
			public boost::noncopyable
	{
	public:

		/**
		 * Used to render a sphere with the background colour of @a view_state, as it changes from time to time.
		 */
		explicit
		BackgroundSphere(
				GPlatesOpenGL::GL &gl,
				const GPlatesPresentation::ViewState &view_state);

		/**
		 * Paints sphere using the background colour of ViewState.
		 *
		 * If the background colour alpha is translucent then alpha is unmodified at visual centre
		 * of globe but is increasingly opaque near the visual circumference (according to how much
		 * material of a thin spherical shell, at globe surface, each view ray passes through).
		 *
		 * If @a depth_writes_enabled is true then sphere fragment shader outputs z-buffer depth.
		 * The geometry is a full-screen quad (which does not output the depth of sphere), so it must
		 * be calculated in the fragment shader if it's needed.
		 * This should only be set to true if depth writes are currently enabled.
		 */
		void
		paint(
				GPlatesOpenGL::GL  &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				bool depth_writes_enabled = false);

	private:
		const GPlatesPresentation::ViewState &d_view_state;

		Colour d_background_colour;

		//! Shader program to render sphere.
		GPlatesOpenGL::GLProgram::shared_ptr_type d_program;

		//! Used to draw a full-screen quad.
		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_full_screen_quad;
	};
}

#endif  // GPLATES_GUI_BACKGROUNDSPHERE_H
