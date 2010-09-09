/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include <cstdlib>
#include <iostream>

#include "GLUQuadric.h"

#include "GLUQuadricDrawable.h"
#include "OpenGLBadAllocException.h"


namespace
{
	// Handle GLU quadric errors
	GLvoid __CONVENTION__
	QuadricError()
	{

		// XXX: Not sure if glGetError returns GLU error codes as well
		// as GL codes.
		std::cerr
		 << "Quadric Error: "
		 << gluErrorString(glGetError())
		 << std::endl;

		exit(1);  // FIXME: should this be an exception instead?
	}
}


void
GPlatesOpenGL::GLUQuadric::create_quadric_obj()
{
	// Create a new 'GLUquadricObj'.
	d_quadric.reset(gluNewQuadric(), gluDeleteQuadric);
	if (d_quadric.get() == 0)
	{
		// not enough memory to allocate object
		throw OpenGLBadAllocException(GPLATES_EXCEPTION_SOURCE,
		 "Not enough memory for OpenGL to create new quadric.");
	}

#if !defined(__APPLE__) || \
	(MAC_OSX_MAJOR_VERSION >= 10 && MAC_OSX_MINOR_VERSION >= 5) || \
	(CXX_MAJOR_VERSION >= 4 && CXX_MINOR_VERSION >= 2)
	// All non apple platforms use this path.
	// Also MacOS X using OS X version 10.5 and greater (or g++ 4.2 and greater)
	// use this code path.
	gluQuadricCallback(d_quadric.get(), GLU_ERROR, &QuadricError);
#else
	// A few OS X platforms need this instead - could be OS X 10.4 or
	// gcc 4.0.0 or PowerPC Macs ?
	// Update: it seems after many installations on different Macs that
	// Mac OSX versions 10.4 using the default compiler g++ 4.0 require this code path.
	gluQuadricCallback(d_quadric.get(), GLU_ERROR,
		reinterpret_cast< GLvoid (__CONVENTION__ *)(...) >(&QuadricError));
#endif

}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUQuadric::draw_sphere(
		GLdouble radius,
		GLint num_slices,
		GLint num_stacks,
		const GPlatesGui::Colour &colour)
{
	// Create GLUquadricObj if it hasn't been created yet.
	//
	// We do this here because this is a draw call and we know that
	// the OpenGL context is current and hence creation of a
	// GLUquadricObj should succeed.
	if (d_quadric.get() == NULL)
	{
		create_quadric_obj();
	}

	boost::shared_ptr<GLUQuadricGeometry> sphere(
			new GLUQuadricSphere(radius, num_slices, num_stacks));

	return GLUQuadricDrawable::create(d_quadric, sphere, d_current_parameters, colour);
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUQuadric::draw_disk(
		GLdouble inner,
		GLdouble outer,
		GLint num_slices,
		GLint num_loops,
		const GPlatesGui::Colour &colour)
{
	// Create GLUquadricObj if it hasn't been created yet.
	//
	// We do this here because this is a draw call and we know that
	// the OpenGL context is current and hence creation of a
	// GLUquadricObj should succeed.
	if (d_quadric.get() == NULL)
	{
		create_quadric_obj();
	}

	boost::shared_ptr<GLUQuadricGeometry> disk(
			new GLUQuadricDisk(inner, outer, num_slices, num_loops));

	return GLUQuadricDrawable::create(d_quadric, disk, d_current_parameters, colour);
}


GPlatesOpenGL::GLUQuadric::Parameters::Parameters() :
	normals(GLU_NONE),
	texture_coords(GL_FALSE),
	orientation(GLU_OUTSIDE),
	draw_style(GLU_FILL)
{
}
