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
 
#ifndef GPLATES_OPENGL_GLUTILS_H
#define GPLATES_OPENGL_GLUTILS_H

#include "utils/CallStackTracker.h"


namespace GPlatesOpenGL
{
	namespace GLUtils
	{
		/**
		 * Asserts that no OpenGL errors (see glGetError) have been recorded.
		 *
		 * If an error is detected then this function aborts for debug builds and
		 * throws @a OpenGLException (with the specific OpenGL error message) for release builds.
		 */
		void
		assert_no_gl_errors(
				const GPlatesUtils::CallStack::Trace &assert_location);
	}
}


#endif // GPLATES_OPENGL_GLUTILS_H
