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
 
#ifndef GPLATES_OPENGL_GLVERTEXARRAYDRAWABLE_H
#define GPLATES_OPENGL_GLVERTEXARRAYDRAWABLE_H

#include <opengl/OpenGL.h>

#include "GLDrawable.h"
#include "GLVertexArray.h"
#include "GLVertexElementArray.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A drawable using vanilla OpenGL vertex arrays.
	 */
	class GLVertexArrayDrawable :
			public GLDrawable
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLVertexArrayDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLVertexArrayDrawable> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLVertexArrayDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLVertexArrayDrawable> non_null_ptr_to_const_type;

		/**
		 * Creates a @a GLVertexArrayDrawable object.
		 *
		 * @a vertex_array contains the vertices and
		 * @a vertex_element_array contains the vertex indices used to render the primitives.
		 */
		static
		non_null_ptr_type
		create(
				const GLVertexArray::shared_ptr_to_const_type &vertex_array,
				const GLVertexElementArray::shared_ptr_to_const_type &vertex_element_array)
		{
			return non_null_ptr_type(
					new GLVertexArrayDrawable(vertex_array, vertex_element_array));
		}


		virtual
		void
		bind() const
		{
			d_vertex_array->bind();
		}


		virtual
		void
		draw() const
		{
			d_vertex_element_array->draw();
		}

	private:
		GLVertexArray::shared_ptr_to_const_type d_vertex_array;
		GLVertexElementArray::shared_ptr_to_const_type d_vertex_element_array;


		//! Constructor.
		GLVertexArrayDrawable(
				const GLVertexArray::shared_ptr_to_const_type &vertex_array,
				const GLVertexElementArray::shared_ptr_to_const_type &vertex_element_array) :
			d_vertex_array(vertex_array),
			d_vertex_element_array(vertex_element_array)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLVERTEXARRAYDRAWABLE_H
