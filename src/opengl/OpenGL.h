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

#ifndef GPLATES_OPENGL_OPENGL_H
#define GPLATES_OPENGL_OPENGL_H

#include <QtGlobal>

// The OpenGL Extension Wrangler Library (GLEW).
// Must be included before the OpenGL headers (which also means before Qt headers).
// But, as noted below, it's best to try and include it in ".cc" files only (rather than here).
//
// NOTE: We're not including the OpenGL Extension Wrangler Library (GLEW) library here
// even though it complains if it's not included before the regular OpenGL headers.
// This is because other modules include this (<opengl/OpenGL.h>) header and they also
// include Qt headers which in turn include regular (GL/gl.h>) OpenGL headers.
// So getting the order of includes correct throughout the application becomes quite difficult.
// A better way is to only include GLEW headers in the OpenGL module ".cc" files (which is the
// only place it should be used) and provide a good enough interface to limit exposure
// of internal details in order to eliminate the exposure any GLEW "#include"s to outside modules.
//
//#include <GL/glew.h>

extern "C" {
#if defined(Q_OS_MAC)
/* Assume compilation on Mac OS X. */
#define __CONVENTION__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif defined(Q_OS_WIN)
// Necessary to include windows.h before including gl.h.
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
#include <GL/glu.h>
#else
/* Other platforms */
#define __CONVENTION__ 
#include <GL/gl.h>
#include <GL/glu.h>
#endif
}


/**
 * This can be used to test a 'GLboolean' which is typically typedef'ed to 'unsigned char'.
 */
#ifndef GPLATES_OPENGL_BOOL
#define GPLATES_OPENGL_BOOL(b) ((b) != 0)
#endif


/**
 * Useful when converting a buffer offset to a 'void *' pointer.
 */
#ifndef GPLATES_OPENGL_BUFFER_OFFSET
#define GPLATES_OPENGL_BUFFER_OFFSET(bytes) (reinterpret_cast<GLubyte *>(0) + (bytes))
#endif

#endif  // GPLATES_OPENGL_OPENGL_H
