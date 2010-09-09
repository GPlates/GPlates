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
 
#ifndef GPLATES_OPENGL_VERTEX_H
#define GPLATES_OPENGL_VERTEX_H

#include "GLStreamPrimitives.h"
#include "OpenGL.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	/**
	 * Every primitive type has one or more vertices of this type.
	 */
	struct Vertex
	{
		GLfloat x, y, z;
		GPlatesGui::rgba8_t colour;
	};

	template<typename T>
	typename GLStreamPrimitives<T>::non_null_ptr_type
	create_stream();
		// Unspecialised function intentionally not defined anywhere.

	template<>
	GLStreamPrimitives<Vertex>::non_null_ptr_type
	create_stream<Vertex>();
		// Defined in .cc file.
}


#endif // GPLATES_OPENGL_VERTEX_H
