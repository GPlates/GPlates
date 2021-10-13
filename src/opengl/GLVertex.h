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

#include <opengl/OpenGL.h>

#include "maths/UnitVector3D.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	/**
	 * A vertex with 3D position.
	 */
	struct GLVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLVertex()
		{  }

		GLVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_) :
			x(x_),
			y(y_),
			z(z_)
		{  }

		explicit
		GLVertex(
				const GPlatesMaths::UnitVector3D &vertex_) :
			x(vertex_.x().dval()),
			y(vertex_.y().dval()),
			z(vertex_.z().dval())
		{  }


		GLfloat x, y, z;
	};


	/**
	 * A vertex with 3D position and a colour.
	 */
	struct GLColouredVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLColouredVertex()
		{  }

		GLColouredVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_,
				GPlatesGui::rgba8_t colour_) :
			x(x_),
			y(y_),
			z(z_),
			colour(colour_)
		{  }

		GLColouredVertex(
				const GPlatesMaths::UnitVector3D &vertex_,
				GPlatesGui::rgba8_t colour_) :
			x(vertex_.x().dval()),
			y(vertex_.y().dval()),
			z(vertex_.z().dval()),
			colour(colour_)
		{  }


		GLfloat x, y, z;
		GPlatesGui::rgba8_t colour;
	};


	/**
	 * A vertex with 3D position and 2D texture coordinates.
	 */
	struct GLTexturedVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLTexturedVertex()
		{  }

		GLTexturedVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_,
				GLfloat u_,
				GLfloat v_) :
			x(x_),
			y(y_),
			z(z_),
			u(u_),
			v(v_)
		{  }

		GLTexturedVertex(
				const GPlatesMaths::UnitVector3D &vertex_,
				GLfloat u_,
				GLfloat v_) :
			x(vertex_.x().dval()),
			y(vertex_.y().dval()),
			z(vertex_.z().dval()),
			u(u_),
			v(v_)
		{  }


		GLfloat x, y, z;
		GLfloat u, v;
	};


	/**
	 * A vertex with 3D position, a colour and 2D texture coordinates.
	 */
	struct GLColouredTexturedVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLColouredTexturedVertex()
		{  }

		GLColouredTexturedVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_,
				GLfloat u_,
				GLfloat v_,
				GPlatesGui::rgba8_t colour_) :
			x(x_),
			y(y_),
			z(z_),
			u(u_),
			v(v_),
			colour(colour_)
		{  }

		GLColouredTexturedVertex(
				const GPlatesMaths::UnitVector3D &vertex_,
				GLfloat u_,
				GLfloat v_,
				GPlatesGui::rgba8_t colour_) :
			x(vertex_.x().dval()),
			y(vertex_.y().dval()),
			z(vertex_.z().dval()),
			u(u_),
			v(v_),
			colour(colour_)
		{  }


		GLfloat x, y, z;
		GLfloat u, v;
		GPlatesGui::rgba8_t colour;
	};


	class GLVertexArray;

	/**
	 * Configures a @a GLVertexArray based on the type of vertices it stores.
	 *
	 * In order for this function to work with a specific type of vertex it must be
	 * specialised for that vertex type.
	 *
	 * Unspecialised function intentionally not defined anywhere.
	 */
	template <typename VertexType>
	void
	configure_vertex_array(
			GLVertexArray &vertex_array);


	//! Specialisation for @a GLVertex.
	template <>
	void
	configure_vertex_array<GLVertex>(
			GLVertexArray &vertex_array);

	//! Specialisation for @a GLColouredVertex.
	template <>
	void
	configure_vertex_array<GLColouredVertex>(
			GLVertexArray &vertex_array);

	//! Specialisation for @a GLTexturedVertex.
	template <>
	void
	configure_vertex_array<GLTexturedVertex>(
			GLVertexArray &vertex_array);

	//! Specialisation for @a GLTexturedVertex.
	template <>
	void
	configure_vertex_array<GLColouredTexturedVertex>(
			GLVertexArray &vertex_array);
}


#endif // GPLATES_OPENGL_VERTEX_H
