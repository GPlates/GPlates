/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_OPENGL3_H
#define GPLATES_OPENGL_OPENGL3_H

//
// OpenGL 3.3 core.
//
// Include only at the TOP of ".cc" files (and use <opengl/OpenGL1.h> elsewhere).
//
// GPlates currently uses the OpenGL 3.3 core profile.
// This, and all other OpenGL versions, is exposed via the OpenGL Extension Wrangler Library (GLEW).
//
// Note: The regular OpenGL header <GL/gl.h> is only guaranteed to provide the basic types/parameters
//       available in OpenGL version 1.1, like 'GLenum', 'GL_BLEND', etc. This is because the Windows
//       platform only provides OpenGL 1.1. Also note that <GL/glew.h> replaces <GL/gl.h>, so it's
//       only necessary to include the former here.
//
// NOTE: This header MUST be included before the regular OpenGL header <GL/gl.h>
//       (which also means before Qt headers).
//
//       To ensure this, it's best to only include this header at the TOP of ".cc" files.
//
//       This is because <GL/glew.h> must be included before the regular OpenGL header <GL/gl.h>.
//       If this header is not included at the TOP of a ".cc" file then it's possible a Qt header is
//       directly or indirectly included before <GL/glew.h>, and that Qt header might include the
//       regular OpenGL header <GL/gl.h>. So getting the order of includes correct throughout the
//       application becomes quite difficult (unless this header is included at the TOP of ".cc" files).
//       
//       If you do need to include OpenGL from a header ".h" file, include <openGL/OpenGL1.h> instead.
//       Typically an interface in a header can be written to rely only on the basic OpenGL types
//       (like 'GLenum'). For example, in the declaration of a function that has a parameter of
//       type 'GLenum' - the implementation of that function is then placed in a ".cc" file which
//       then includes <opengl/OpenGL3.h> (at the TOP) to access core OpenGL 3.3 functionality.
//

#include <GL/glew.h>

#endif // GPLATES_OPENGL_OPENGL3_H
