/* $Id$ */

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
// has a parameter of type 'GLenum', in a ".h" file. The implementation of that function, in a ".cc" file,
// then includes <opengl/OpenGL3.h> (at the TOP) to access core OpenGL 3.3 functionality.

#include <QtGlobal>

extern "C"
{
	#if defined(Q_OS_MACOS)
		/* Assume compilation on Mac OS X. */
		#define __CONVENTION__
		#include <OpenGL/gl.h>
	#elif defined(Q_OS_WIN)
		/* Necessary to include windows.h before including gl.h */
		// Note: Prevent windows.h from defining min/max macros since these
		//       interfere with things like 'std::numeric_limits<int>::max()'.
		#ifndef NOMINMAX
		#	define NOMINMAX
		#	define NOMINMAX_DEFINED_FROM_GLOBAL_OPENGL_H
		#endif
		#include <windows.h>
		// If NOMINMAX was defined above then undo that now.
		#ifdef NOMINMAX_DEFINED_FROM_GLOBAL_OPENGL_H
		#	undef NOMINMAX_DEFINED_FROM_GLOBAL_OPENGL_H
		#	undef NOMINMAX
		#endif
		#define __CONVENTION__ WINAPI
		#include <GL/gl.h>
	#else
		/* Other platforms */
		#define __CONVENTION__ 
		#include <GL/gl.h>
	#endif


	//
	// The following declarations might not be in <gl.h> since it might only declare OpenGL 1.1 (eg, on Windows).
	// So here we declare those (above OpenGL 1.1) that we use in our *header* files, solely to avoid
	// forcing the include of <glew.h> which cannot be included from header files.
	//
	// Note that if <glew.h> has already been included (eg, a ".cc" file included <glew.h> and then later included
	// a ".h" which included us) then these declarations will not get re-declared (due to their "#ifndef" guards).
	// Also note that <gl.h> has its own include guards which <glew.h> explicitly defines so that <gl.h> essentially
	// does nothing if it is included after <glew.h>.
	// In which case no new declarations will be added by *this* file (everything will be added by <glew.h>).
	//

	#ifndef GL_TEXTURE0
	#	define GL_TEXTURE0 0x84C0
	#endif

	#ifndef GL_COLOR_ATTACHMENT0
	#	define GL_COLOR_ATTACHMENT0 0x8CE0
	#endif

	#ifndef GL_INVALID_INDEX
	#	define GL_INVALID_INDEX 0xFFFFFFFFu
	#endif
}

#endif  // GPLATES_OPENGL_OPENGL1_H
