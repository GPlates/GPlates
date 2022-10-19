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

#include <string>
#include <utility>
#include <QtGlobal>
#include <QDebug>

#include "GLContext.h"

#include "GLStateStore.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLContext::GLContext()
{
	// The boost::shared_ptr<> deleter is a lambda function that does nothing
	// (since we don't actually want to delete 'this').
	d_context_handle.reset(this, [](GLContext*) {});

	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesOpenGL::GLContext::~GLContext()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


void
GPlatesOpenGL::GLContext::initialise_gl(
		Vulkan &vulkan)
{
	d_vulkan = vulkan;

	// Create the OpenGL functions wrapper (it's currently empty - it's just so things will compile until ported to Vulkan).
	d_opengl_functions = OpenGLFunctions::create();

	// Used by GL to efficiently allocate GLState objects.
	d_state_store = GLStateStore::create(GLStateSetStore::create(), GLStateSetKeys::create(d_capabilities));
}


void
GPlatesOpenGL::GLContext::shutdown_gl()
{
	d_state_store = boost::none;
	d_opengl_functions = boost::none;

	d_vulkan = boost::none;
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GL>::non_null_ptr_type
GPlatesOpenGL::GLContext::access_opengl()
{
	// We should be between 'initialise_gl()' and 'shutdown_gl()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_vulkan && d_opengl_functions && d_state_store,
			GPLATES_ASSERTION_SOURCE);

	// The default viewport  (not currently used - it's just so things will compile until ported to Vulkan).
	const GLViewport default_viewport(0, 0, 100, 100);

	// The default framebuffer object.
	const GLuint default_framebuffer_object = 0;

	return GL::create(
			d_vulkan.get(),
			get_non_null_pointer(this),
			d_capabilities,
			*d_opengl_functions.get(),
			d_state_store.get(),
			default_viewport,
			default_framebuffer_object);
}


boost::optional<GPlatesOpenGL::OpenGLFunctions &>
GPlatesOpenGL::GLContext::get_opengl_functions()
{
	if (!d_opengl_functions)
	{
		return boost::none;
	}

	return *d_opengl_functions.get();
}
