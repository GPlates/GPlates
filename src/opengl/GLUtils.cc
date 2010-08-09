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

#include <OpenGL.h>
#include <QDebug>

#include "GLUtils.h"

#include "OpenGLException.h"

#include "global/GPlatesAssert.h"


void
GPlatesOpenGL::GLUtils::assert_no_gl_errors(
		const GPlatesUtils::CallStack::Trace &assert_location)
{
	const GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const char *gl_error_string = reinterpret_cast<const char *>(gluErrorString(error));

		qWarning() << "OpenGL error: " << gl_error_string;

#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(assert_location);
#else
		throw GPlatesOpenGL::OpenGLException(assert_location, gl_error_string);
#endif
	}
}
