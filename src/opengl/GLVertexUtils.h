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
 
#ifndef GPLATES_OPENGL_VERTEXUTILS_H
#define GPLATES_OPENGL_VERTEXUTILS_H

#include <cstddef>  // offsetof
#include <boost/cstdint.hpp>
#include <boost/integer_traits.hpp>
#include <qopengl.h>  // For OpenGL constants and typedefs.

#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	namespace GLVertexUtils
	{
		/**
		 * Useful when converting a byte offset to a 'void *' pointer (for example, in glDrawElements).
		 */
		constexpr
		GLvoid *
		buffer_offset(
				int num_bytes)
		{
			return reinterpret_cast<GLubyte *>(0) + num_bytes;
		}

		/**
		 * Useful when converting the offset of an attribute (data member) of a vertex class to a 'void *' pointer
		 * (for example, in glVertexAttribPointer).
		 *
		 * Note that @a vertex_class should be a standard-layout class (dictated by the C++11 'offsetof' function macro).
		 */
#define BUFFER_OFFSET(vertex_class, vertex_data_member) \
		GPlatesOpenGL::GLVertexUtils::buffer_offset(offsetof(vertex_class, vertex_data_member))


		/**
		 * Traits class to find the size of a vertex element from its type.
		 */
		template <typename VertexElementType>
		struct ElementTraits; // Unspecialised class intentionally not defined.

		template <>
		struct ElementTraits<GLubyte>
		{
			//! GL_UNSIGNED_BYTE
			static const GLenum type;
			//! The maximum number of vertices that can be indexed.
			static const unsigned int MAX_INDEXABLE_VERTEX = boost::integer_traits<boost::uint8_t>::const_max;
		};

		template <>
		struct ElementTraits<GLushort>
		{
			//! GL_UNSIGNED_SHORT
			static const GLenum type;
			//! The maximum number of vertices that can be indexed.
			static const unsigned int MAX_INDEXABLE_VERTEX = boost::integer_traits<boost::uint16_t>::const_max;
		};

		template <>
		struct ElementTraits<GLuint>
		{
			//! GL_UNSIGNED_INT
			static const GLenum type;
			//! The maximum number of vertices that can be indexed.
			static const unsigned int MAX_INDEXABLE_VERTEX = boost::integer_traits<boost::uint32_t>::const_max;
		};


		/**
		 * A vertex with 3D position.
		 */
		struct Vertex
		{
			//! NOTE: Default constructor does *not* initialise !
			Vertex()
			{  }

			Vertex(
					GLfloat x_,
					GLfloat y_,
					GLfloat z_) :
				x(x_),
				y(y_),
				z(z_)
			{  }

			explicit
			Vertex(
					const GPlatesMaths::UnitVector3D &vertex_) :
				x(vertex_.x().dval()),
				y(vertex_.y().dval()),
				z(vertex_.z().dval())
			{  }

			explicit
			Vertex(
					const GPlatesMaths::Vector3D &vertex_) :
				x(vertex_.x().dval()),
				y(vertex_.y().dval()),
				z(vertex_.z().dval())
			{  }


			GLfloat x, y, z;
		};


		/**
		 * A vertex with 3D position and a colour.
		 */
		struct ColourVertex
		{
			//! NOTE: Default constructor does *not* initialise !
			ColourVertex()
			{  }

			ColourVertex(
					GLfloat x_,
					GLfloat y_,
					GLfloat z_,
					GPlatesGui::rgba8_t colour_) :
				x(x_),
				y(y_),
				z(z_),
				colour(colour_)
			{  }

			ColourVertex(
					const GPlatesMaths::UnitVector3D &vertex_,
					GPlatesGui::rgba8_t colour_) :
				x(vertex_.x().dval()),
				y(vertex_.y().dval()),
				z(vertex_.z().dval()),
				colour(colour_)
			{  }

			ColourVertex(
					const GPlatesMaths::Vector3D &vertex_,
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
		struct TextureVertex
		{
			//! NOTE: Default constructor does *not* initialise !
			TextureVertex()
			{  }

			TextureVertex(
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

			TextureVertex(
					const GPlatesMaths::UnitVector3D &vertex_,
					GLfloat u_,
					GLfloat v_) :
				x(vertex_.x().dval()),
				y(vertex_.y().dval()),
				z(vertex_.z().dval()),
				u(u_),
				v(v_)
			{  }

			TextureVertex(
					const GPlatesMaths::Vector3D &vertex_,
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
		 * A vertex with 3D position and *3D* texture coordinates.
		 */
		struct Texture3DVertex
		{
			//! NOTE: Default constructor does *not* initialise !
			Texture3DVertex()
			{  }

			Texture3DVertex(
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

			Texture3DVertex(
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

			Texture3DVertex(
					const GPlatesMaths::Vector3D &vertex_,
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
		 * A vertex with 3D position, a colour and 2D texture coordinates.
		 */
		struct ColourTextureVertex
		{
			//! NOTE: Default constructor does *not* initialise !
			ColourTextureVertex()
			{  }

			ColourTextureVertex(
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

			ColourTextureVertex(
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

			ColourTextureVertex(
					const GPlatesMaths::Vector3D &vertex_,
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
		 * A vertex with 3D position, 2D texture coordinates and a tangent-space frame consisting
		 * of three 3D texture coordinates representing the three frame axes.
		 *
		 * The 2D texture coordinates are on texture unit 0.
		 * The tangent, binormal and normal of the tangent-space frame are on texture units 1, 2, and 3 respectively.
		 */
		struct TextureTangentSpaceVertex
		{
			//! NOTE: Default constructor does *not* initialise !
			TextureTangentSpaceVertex()
			{  }

			TextureTangentSpaceVertex(
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

			TextureTangentSpaceVertex(
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

			TextureTangentSpaceVertex(
					const GPlatesMaths::UnitVector3D &vertex_,
					GLfloat u_,
					GLfloat v_,
					const GPlatesMaths::Vector3D &tangent_,
					const GPlatesMaths::Vector3D &binormal_,
					const GPlatesMaths::Vector3D &normal_) :
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
	}
}


#endif // GPLATES_OPENGL_VERTEXUTILS_H
