/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_OPENGL_H
#define GPLATES_GUI_OPENGL_H

extern "C" {
#if defined(__APPLE__)
/* Assume compilation on Mac OS X. */
#define __CONVENTION__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif defined(__WINDOWS__)
/* Necessary to include windows.h before including gl.h */
#include <windows.h>
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

#endif  // GPLATES_GUI_OPENGL_H
