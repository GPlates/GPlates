/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_OPENGL_GLVIEWPORT_H
#define GPLATES_OPENGL_GLVIEWPORT_H

#include <opengl/OpenGL.h>


namespace GPlatesOpenGL
{
	/**
	 * OpenGL viewport parameters.
	 */
	class GLViewport
	{
	public:
		//! Constructor.
		GLViewport(
				GLint x_,
				GLint y_,
				GLsizei width_,
				GLsizei height_)
		{
			gl_viewport(x_, y_, width_, height_);
		}


		/**
		 * Performs function of similarly named OpenGL function.
		 *
		 * This method is useful for changing the viewport parameters.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 */
		void
		gl_viewport(
				GLint x_,
				GLint y_,
				GLsizei width_,
				GLsizei height_)
		{
			d_viewport.x = x_;
			d_viewport.y = y_;
			d_viewport.width = width_;
			d_viewport.height = height_;
		}


		GLint
		x() const
		{
			return d_viewport.x;
		}

		GLint
		y() const
		{
			return d_viewport.y;
		}

		GLsizei
		width() const
		{
			return d_viewport.width;
		}

		GLsizei
		height() const
		{
			return d_viewport.height;
		}


		//! Typedef for an array of four integers representing the viewport parameters.
		typedef GLint viewport_type[4];

		/**
		 * Returns the viewport parameters as an array of four integers.
		 */
		const viewport_type &
		viewport() const
		{
			return d_viewport.viewport;
		}

	private:
		struct Viewport
		{
			union
			{
				GLint viewport[4];

				struct
				{
					GLint x;
					GLint y;
					GLsizei width;
					GLsizei height;
				};
			};
		};

		Viewport d_viewport;
	};
}

#endif // GPLATES_OPENGL_GLVIEWPORT_H
