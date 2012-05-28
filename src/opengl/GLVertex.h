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

#include "GLVertexBuffer.h"

#include "maths/UnitVector3D.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLVertexArray;

	/**
	 * Specifies the source of vertex attribute data (vertices) as a vertex buffer and binds
	 * the attribute data contained within to the vertex array (the internal @a GLVertexArray).
	 *
	 * NOTE: It's possible to set multiple vertex buffers if the vertex attribute data is spread
	 * across multiple vertex streams - in this case 'VertexType' represents the subset of vertex
	 * attribute data for the specified vertex buffer (stream).
	 *
	 * The template parameter 'VertexType' is used to bind the vertex data in the vertex buffer
	 * as individual vertex attribute arrays.
	 *
	 * @a offset is the byte offset from the beginning of the vertex buffer to start retrieving vertices.
	 * NOTE: The byte offset must satisfy the alignment requirements of the vertex type 'VertexType'.
	 *
	 * Note that this method is only necessary if the @a GLVertexArray passed into @a create
	 * has no vertex buffer(s) bound.
	 *
	 * Note that @a vertex_buffer can be initialised with data before *or* after this call.
	 *
	 * @a offset is the byte offset in the vertex buffer at which vertices will be read from in draw calls.
	 * NOTE: The byte offset must satisfy the alignment requirements of the vertex type 'VertexType'.
	 *
	 * In order for this function to work with a specific type of vertex it must be specialised for that vertex type.
	 *
	 * This unspecialised function is intentionally not defined anywhere.
	 */
	template <typename VertexType>
	void
	bind_vertex_buffer_to_vertex_array(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset = 0);


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
	 * Binds a @a GLVertexBuffer, containing vertices of type @a GLVertex, to a @a GLVertexArray.
	 */
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset);


	/**
	 * A vertex with 3D position and a colour.
	 */
	struct GLColourVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLColourVertex()
		{  }

		GLColourVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_,
				GPlatesGui::rgba8_t colour_) :
			x(x_),
			y(y_),
			z(z_),
			colour(colour_)
		{  }

		GLColourVertex(
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
	 * Binds a @a GLVertexBuffer, containing vertices of type @a GLColourVertex, to a @a GLVertexArray.
	 */
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLColourVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset);


	/**
	 * A vertex with 3D position and 2D texture coordinates.
	 */
	struct GLTextureVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLTextureVertex()
		{  }

		GLTextureVertex(
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

		GLTextureVertex(
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
	 * Binds a @a GLVertexBuffer, containing vertices of type @a GLTextureVertex, to a @a GLVertexArray.
	 */
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLTextureVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset);


	/**
	 * A vertex with 3D position and *3D* texture coordinates.
	 */
	struct GLTexture3DVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLTexture3DVertex()
		{  }

		GLTexture3DVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_,
				GLfloat s_,
				GLfloat t_,
				GLfloat r_) :
			x(x_),
			y(y_),
			z(z_),
			s(s_),
			t(t_),
			r(r_)
		{  }

		GLTexture3DVertex(
				const GPlatesMaths::UnitVector3D &vertex_,
				GLfloat s_,
				GLfloat t_,
				GLfloat r_) :
			x(vertex_.x().dval()),
			y(vertex_.y().dval()),
			z(vertex_.z().dval()),
			s(s_),
			t(t_),
			r(r_)
		{  }


		GLfloat x, y, z;
		GLfloat s, t, r;
	};

	/**
	 * Binds a @a GLVertexBuffer, containing vertices of type @a GLTexture3DVertex, to a @a GLVertexArray.
	 */
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLTexture3DVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset);


	/**
	 * A vertex with 3D position, a colour and 2D texture coordinates.
	 */
	struct GLColourTextureVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLColourTextureVertex()
		{  }

		GLColourTextureVertex(
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

		GLColourTextureVertex(
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

	/**
	 * Binds a @a GLVertexBuffer, containing vertices of type @a GLColourTextureVertex, to a @a GLVertexArray.
	 */
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLColourTextureVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset);


	/**
	 * A vertex with 3D position, 2D texture coordinates and a tangent-space frame consisting
	 * of three 3D texture coordinates representing the three frame axes.
	 *
	 * The 2D texture coordinates are on texture unit 0.
	 * The tangent, binormal and normal of the tangent-space frame are on texture units 1, 2, and 3 respectively.
	 */
	struct GLTextureTangentSpaceVertex
	{
		//! NOTE: Default constructor does *not* initialise !
		GLTextureTangentSpaceVertex()
		{  }

		GLTextureTangentSpaceVertex(
				GLfloat x_,
				GLfloat y_,
				GLfloat z_,
				GLfloat u_,
				GLfloat v_,
				GLfloat tangent_x_,
				GLfloat tangent_y_,
				GLfloat tangent_z_,
				GLfloat binormal_x_,
				GLfloat binormal_y_,
				GLfloat binormal_z_,
				GLfloat normal_x_,
				GLfloat normal_y_,
				GLfloat normal_z_) :
			x(x_),
			y(y_),
			z(z_),
			u(u_),
			v(v_),
			tangent_x(tangent_x_),
			tangent_y(tangent_y_),
			tangent_z(tangent_z_),
			binormal_x(binormal_x_),
			binormal_y(binormal_y_),
			binormal_z(binormal_z_),
			normal_x(normal_x_),
			normal_y(normal_y_),
			normal_z(normal_z_)
		{  }

		GLTextureTangentSpaceVertex(
				const GPlatesMaths::UnitVector3D &vertex_,
				GLfloat u_,
				GLfloat v_,
				const GPlatesMaths::UnitVector3D &tangent_,
				const GPlatesMaths::UnitVector3D &binormal_,
				const GPlatesMaths::UnitVector3D &normal_) :
			x(vertex_.x().dval()),
			y(vertex_.y().dval()),
			z(vertex_.z().dval()),
			u(u_),
			v(v_),
			tangent_x(tangent_.x().dval()),
			tangent_y(tangent_.y().dval()),
			tangent_z(tangent_.z().dval()),
			binormal_x(binormal_.x().dval()),
			binormal_y(binormal_.y().dval()),
			binormal_z(binormal_.z().dval()),
			normal_x(normal_.x().dval()),
			normal_y(normal_.y().dval()),
			normal_z(normal_.z().dval())
		{  }


		GLfloat x, y, z;
		GLfloat u, v;
		GLfloat tangent_x, tangent_y, tangent_z;
		GLfloat binormal_x, binormal_y, binormal_z;
		GLfloat normal_x, normal_y, normal_z;
	};

	/**
	 * Binds a @a GLVertexBuffer, containing vertices of type @a GLTextureTangentSpaceVertex, to a @a GLVertexArray.
	 */
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLTextureTangentSpaceVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset);
}


#endif // GPLATES_OPENGL_VERTEX_H
