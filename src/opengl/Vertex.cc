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

#include "Vertex.h"

#include "GLVertexArray.h"


namespace GPlatesOpenGL
{
	template<>
	GLStreamPrimitives<Vertex>::non_null_ptr_type
	create_stream<Vertex>()
	{
		//
		// The following reflects the structure of 'struct Vertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		const GPlatesOpenGL::GLVertexArray::VertexPointer vertex_pointer = {
				3, GL_FLOAT, sizeof(Vertex), 0 };

		const GPlatesOpenGL::GLVertexArray::ColorPointer colour_pointer = {
				4, GL_UNSIGNED_BYTE, sizeof(Vertex), 3 * sizeof(GLfloat) };

		GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type stream =
				GPlatesOpenGL::GLStreamPrimitives<Vertex>::create(vertex_pointer, colour_pointer);

		return stream;
	}
}

