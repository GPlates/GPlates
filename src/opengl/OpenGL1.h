2/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_OPENGL1_H
#define GPLATES_OPENGL_OPENGL1_H

//
// OpenGL version 1.1.
//
// Include only from header files (and use <opengl/OpenGL3.h> at the TOP of ".cc" files).
//
// This only includes the regular OpenGL header <GL/gl.h>, which is only guaranteed to provide
// the basic types/parameters available in OpenGL version 1.1, like 'GLenum', 'GL_BLEND', etc.
// This is because the Windows platform only provides OpenGL 1.1 (though other platforms may provide higher).
//
// As a result, this header is only useful when you cannot include <opengl/OpenGL3.h>.
// And you cannot reliably include <opengl/OpenGL3.h> in header ".h" files
// because <opengl/OpenGL3.h> should only be included at the TOP of ".cc" files
// (please see <opengl/OpenGL3.h> for an explanation).
//
// So, since GPlates relies on the OpenGL 3.3 core profile, this header is only useful for providing
// basic types (like 'GLenum') in header files. For example, in the declaration of a function that
// has a parameter of type 'GLenum' - the implementation of that function, in a ".cc" file, then
// includes <opengl/OpenGL3.h> (at the TOP) to access core OpenGL 3.3 functionality.

#include <QtGlobal>

extern "C"
{
	#if defined(Q_OS_MACOS)
		/* Assume compilation on Mac OS X. */
		#define __CONVENTION__
		#include <OpenGL/gl.h>
	#elif defined(Q_OS_WIN)
		/* Necessary to include windows.h before including gl.h */
		#include <windows.h>
		#define __CONVENTION__ WINAPI
		#include <GL/gl.h>
	#else
		/* Other platforms */
		#define __CONVENTION__ 
		#include <GL/gl.h>
	#endif
}

#endif  // GPLATES_OPENGL_OPENGL1_H
