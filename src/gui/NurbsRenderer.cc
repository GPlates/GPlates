/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <iostream>
#include "NurbsRenderer.h"
#include "OpenGLBadAllocException.h"

using namespace GPlatesGui;

namespace
{
	// Handle GLU NURBS errors
	void
	NurbsError(GLenum error) {

		std::cerr
		 << "NURBS Error: "
		 << gluErrorString(error)
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
	gluNurbsCallback(_nr, GLU_ERROR,
	 reinterpret_cast< void (*)() >(&NurbsError));
}
