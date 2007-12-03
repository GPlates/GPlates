/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include <iostream>
#include "NurbsRenderer.h"
#include "OpenGLBadAllocException.h"

using namespace GPlatesGui;

namespace
{
	// Handle GLU NURBS errors
	GLvoid
	NurbsError() {

		// XXX I'm not sure if glGetError actually returns the GLU 
		// error codes as well as the GL codes.
		std::cerr
		 << "NURBS Error: "
		 << gluErrorString(glGetError())
		 << std::endl;

		exit(1);  // FIXME: should this be an exception instead?
	}
}


GPlatesGui::NurbsRenderer::NurbsRenderer() {

	_nr = gluNewNurbsRenderer();
	if (_nr == 0) {

		// not enough memory to allocate object
		throw OpenGLBadAllocException(
		 "Not enough memory for OpenGL to create new NURBS renderer.");
	}
	// Previously, the type-parameter of the cast was 'void (*)()'.
	// On Mac OS X, the compiler complained, so it was changed to this.
	// Update: Fixed the prototype of the NurbsError callback function 
	// and removed the varargs ellipsis from the cast type.
	gluNurbsCallback(_nr, GLU_ERROR,
#if defined(__APPLE__)
			// Assume compilation on OS X.
			reinterpret_cast< GLvoid (__CONVENTION__ *)(...) >(&NurbsError));
#else
			reinterpret_cast< GLvoid (__CONVENTION__ *)() >(&NurbsError));
#endif
}
