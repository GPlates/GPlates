/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <iostream>
#include "Quadrics.h"
#include "OpenGLBadAllocException.h"

namespace
{
	// Handle GLU quadric errors
	void
	QuadricError(GLenum error) {

		std::cerr
		 << "Quadric Error: "
		 << gluErrorString(error)
		 << std::endl;

		exit(1);  // FIXME: should this be an exception instead?
	}
}


GPlatesGui::Quadrics::Quadrics() {

	_q = gluNewQuadric();
	if (_q == 0) {

		// not enough memory to allocate object
		throw OpenGLBadAllocException(
		 "Not enough memory for OpenGL to create new quadric.");
	}
	// Previously, the type-parameter of the cast was 'void (*)()'.
	// On Mac OS X, the compiler complained, so it was changed to this.
	gluQuadricCallback(_q, GLU_ERROR,
	 reinterpret_cast< GLvoid (*)(...) >(&QuadricError));
}
