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

#ifndef GPLATES_OPENGL_GLCONTEXTLIFETIME_H
#define GPLATES_OPENGL_GLCONTEXTLIFETIME_H

namespace GPlatesOpenGL
{
	class GL;

	/**
	 * Interface for creating and destroying OpenGL resources when initialising and shutting down OpenGL.
	 */
	class GLContextLifetime
	{
	public:

		virtual
		~GLContextLifetime()
		{  }


		/**
		 * The OpenGL context has been created.
		 *
		 * So any OpenGL resources can be created now.
		 */
		virtual
		void
		initialise_gl(
				GL &gl) = 0;

		/**
		 * The OpenGL context is about to be destroyed.
		 *
		 * So any OpenGL resources should now be destroyed.
		 */
		virtual
		void
		shutdown_gl(
				GL &gl) = 0;
	};
}

#endif // GPLATES_OPENGL_GLCONTEXTLIFETIME_H
