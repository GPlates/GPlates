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
 
#ifndef GPLATES_OPENGL_GLVERTEXARRAY_H
#define GPLATES_OPENGL_GLVERTEXARRAY_H

#include <bitset>
#include <cstddef> // For std::size_t
#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLArray.h"
#include "GLVertex.h"
#include "GLVertexBufferResource.h"


namespace GPlatesOpenGL
{
	/**
	 * An OpenGL vertex array.
	 *
	 * NOTE: The template function 'configure_vertex_array' (see GLVertex.h) must be
	 * specialised for the 'VertexType' template type of the @a create and
	 * @a set_array_data methods.
	 */
	class GLVertexArray
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLVertexArray.
		typedef boost::shared_ptr<GLVertexArray> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexArray> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexArray.
		typedef boost::weak_ptr<GLVertexArray> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexArray> weak_ptr_to_const_type;


		//
		// The following structures are used in the following methods as an alternative way
		// to pass function arguments:
		// @a gl_vertex_pointer
		// @a gl_color_pointer
		// @a gl_normal_pointer
		// @a gl_tex_coord_pointer
		//
		struct VertexPointer
		{
			GLint size;
			GLenum type;
			GLsizei stride;
			GLint array_offset;
		};
		struct ColorPointer
		{
			GLint size;
			GLenum type;
			GLsizei stride;
			GLint array_offset;
		};
		struct NormalPointer
		{
			GLenum type;
			GLsizei stride;
			GLint array_offset;
		};
		struct TexCoordPointer
		{
			GLint size;
			GLenum type;
			GLsizei stride;
			GLint array_offset;
		};


		/**
		 * Creates a @a GLVertexArray object with no array data.
		 *
		 * You'll need to call @a set_array_data.
		 */
		static
		shared_ptr_type
		create(
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none)
		{
			return shared_ptr_type(create_as_auto_ptr(usage, vertex_buffer_manager).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLVertexArray>
		create_as_auto_ptr(
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none)
		{
			return std::auto_ptr<GLVertexArray>(
					new GLVertexArray(
							GLArray::create(
									GLArray::ARRAY_TYPE_VERTICES, usage, vertex_buffer_manager)));
		}


		/**
		 * Creates a @a GLVertexArray object.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * @a VertexType is the type (or structure) of the vertex data passed in.
		 * NOTE: we don't keep track of the vertex structure.
		 *
		 * If @a configure is true then the client state and attribute pointers are set
		 * depending on 'VertexType'.
		 *
		 * NOTE: The template function 'configure_vertex_array' (see GLVertex.h) must be
		 * specialised for the 'VertexType' template type of the @a create and
		 * @a set_array_data methods.
		 *
		 * If @a vertex_buffer_manager is specified *and* vertex buffer objects are supported
		 * then an OpenGL vertex buffer object is used internally to store the vertices.
		 */
		template <class VertexType>
		static
		shared_ptr_type
		create(
				const VertexType *vertices,
				const unsigned int num_vertices,
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none,
				bool configure = true)
		{
			const shared_ptr_type vertex_array(
					new GLVertexArray(
							GLArray::create(
									vertices, num_vertices,
									GLArray::ARRAY_TYPE_VERTICES, usage,
									vertex_buffer_manager)));

			if (configure)
			{
				configure_vertex_array<VertexType>(*vertex_array);
			}

			return vertex_array;
		}


		/**
		 * Creates a @a GLVertexArray object.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * @a VertexType is the type (or structure) of the vertex data passed in.
		 * NOTE: we don't keep track of the vertex structure.
		 *
		 * If @a configure is true then the client state and attribute pointers are set
		 * depending on 'VertexType'.
		 *
		 * NOTE: The template function 'configure_vertex_array' (see GLVertex.h) must be
		 * specialised for the 'VertexType' template type of the @a create and
		 * @a set_array_data methods.
		 *
		 * If @a vertex_buffer_manager is specified *and* vertex buffer objects are supported
		 * then an OpenGL vertex buffer object is used internally to store the vertices.
		 */
		template <class VertexType>
		static
		shared_ptr_type
		create(
				const std::vector<VertexType> &vertices,
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none,
				bool configure = true)
		{
			return create(&vertices[0], vertices.size(), usage, vertex_buffer_manager, configure);
		}


		/**
		 * Creates a @a GLVertexArray object that uses the same array data as another @a GLVertexArray.
		 *
		 * This is useful when you want to use the same set of vertices in two different ways.
		 * For example, vertices with (x,y,z) position, colour and texture coordinates can be
		 * used as (1) position and colour, (2) position, colour and texture coordinates, etc.
		 * In case (1) the texture coordinates are not needed and simply ignored.
		 *
		 * Note that you will need to explicitly enable the appropriate client state and
		 * explicitly set the appropriate vertex attribute pointers - this is because you are
		 * choosing to view the same vertex array in a way that is different than the vertex type
		 * and hence you'll need to specify this.
		 *
		 * However note that if you change the array data it will affect all @a GLVertexArray
		 * instances sharing it.
		 * NOTE: If you later call @a set_array_data, be careful not to change the VertexType
		 * because it could mesh up how other @a GLVertexArray instances (that share it) will see it.
		 */
		static
		shared_ptr_type
		create(
				const GLArray::non_null_ptr_type &array_data)
		{
			return shared_ptr_type(new GLVertexArray(array_data));
		}


		/**
		 * Returns the array data referenced by us.
		 */
		GLArray::non_null_ptr_type
		get_array_data()
		{
			return d_array_data;
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexArray.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * If @a configure is true then the client state and attribute pointers are set
		 * depending on 'VertexType'.
		 *
		 * NOTE: The template function 'configure_vertex_array' (see GLVertex.h) must be
		 * specialised for the 'VertexType' template type of the @a create and
		 * @a set_array_data methods.
		 */
		template <class VertexType>
		void
		set_array_data(
				const VertexType *vertices,
				const unsigned int num_vertices,
				bool configure = true)
		{
			d_array_data->set_array_data(vertices, num_vertices);

			if (configure)
			{
				disable_client_state();
				configure_vertex_array<VertexType>(*this);
			}
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexArray.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 *
		 * If @a configure is true then the client state and attribute pointers are set
		 * depending on 'VertexType'.
		 *
		 * NOTE: The template function 'configure_vertex_array' (see GLVertex.h) must be
		 * specialised for the 'VertexType' template type of the @a create and
		 * @a set_array_data methods.
		 */
		template <class VertexType>
		void
		set_array_data(
				const std::vector<VertexType> &vertices,
				bool configure = true)
		{
			set_array_data(&vertices[0], vertices.size(), configure);
		}


		/**
		 * Disables all vertex attributes.
		 */
		void
		disable_client_state();


		/**
		 * Selects the current texture unit that a subsequent @a gl_tex_coord_pointer
		 * should be directed to.
		 *
		 * Like the other 'gl*()' methods in this class the same-named call to OpenGL
		 * is not made here (it is delayed until @a bind is called).
		 *
		 * The default texture unit is texture unit 0, in which case it is not necessary
		 * to call this before calling @a gl_tex_coord_pointer. The default is texture unit 0
		 * regardless of the currently active unit for some other @a GLVertexArray object.
		 *
		 * If the runtime system doesn't support the GL_ARB_multitexture extension
		 * (and hence only supports one texture unit) then it is not necessary
		 * to call this function.
		 */
		GLVertexArray &
		gl_client_active_texture_ARB(
				GLenum texture);


		/**
		 * Call once for each vertex array that you want to enable.
		 *
		 * These are the arrays used by @a bind.
		 * By default they are all disabled for 'this' drawable.
		 *
		 * @a array must be one of GL_VERTEX_ARRAY, GL_COLOR_ARRAY,
		 * GL_NORMAL_ARRAY, or GL_TEXTURE_COORD_ARRAY.
		 *
		 * NOTE: not including GL_INDEX_ARRAY since using RGBA mode (and not colour index mode)
		 * and not including GL_EDGE_FLAG_ARRAY for now.
		 */
		GLVertexArray &
		gl_enable_client_state(
				GLenum array)
		{
			set_enable_disable_client_state(array, true);
			return *this;
		}

		/**
		 * The reverse of @a gl_enable_client_state.
		 *
		 * This method is only needed if you have enabled an array and now want to disable it.
		 */
		GLVertexArray &
		gl_disable_client_state(
				GLenum array)
		{
			set_enable_disable_client_state(array, false);
			return *this;
		}


		/**
		 * Stores position parameters for the same-named call to OpenGL inside @a bind.
		 *
		 * @a array_offset is a byte offset from the start of the vertex array
		 * (passed into @a create) from which to start retrieving position data.
		 *
		 * This should be called if 'GL_VERTEX_ARRAY' was passed to @a gl_enable_client_state.
		 */
		GLVertexArray &
		gl_vertex_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint array_offset)
		{
			const VertexPointer vertex_pointer = { size, type, stride, array_offset };
			return gl_vertex_pointer(vertex_pointer);
		}

		//! Same as the other overload of @a gl_vertex_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_vertex_pointer(
				const VertexPointer &vp);


		/**
		 * Stores colour parameters for the same-named call to OpenGL inside @a bind.
		 *
		 * @a array_offset is a byte offset from the start of the vertex array
		 * (passed into @a create) from which to start retrieving colour data.
		 *
		 * This should be called if 'GL_COLOR_ARRAY' was passed to @a gl_enable_client_state.
		 */
		GLVertexArray &
		gl_color_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint array_offset)
		{
			const ColorPointer color_pointer = { size, type, stride, array_offset };
			return gl_color_pointer(color_pointer);
		}

		//! Same as the other overload of @a gl_color_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_color_pointer(
				const ColorPointer &cp);


		/**
		 * Stores normal parameters for the same-named call to OpenGL inside @a bind.
		 *
		 * @a array_offset is a byte offset from the start of the vertex array
		 * (passed into @a create) from which to start retrieving normal data.
		 *
		 * This should be called if 'GL_NORMAL_ARRAY' was passed to @a gl_enable_client_state.
		 */
		GLVertexArray &
		gl_normal_pointer(
				GLenum type,
				GLsizei stride,
				GLint array_offset)
		{
			const NormalPointer normal_pointer = { type, stride, array_offset };
			return gl_normal_pointer(normal_pointer);
		}

		//! Same as the other overload of @a gl_normal_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_normal_pointer(
				const NormalPointer &np);


		/**
		 * Stores texture coordinates for the same-named call to OpenGL inside @a bind.
		 *
		 * Texture coordinates are applied to the current texture unit
		 * (see @a gl_client_active_texture_ARB).
		 *
		 * @a array_offset is a byte offset from the start of the vertex array
		 * (passed into @a create) from which to start retrieving texture coordinate data.
		 *
		 * This should be called if 'GL_TEXTURE_COORD_ARRAY' was passed to @a gl_enable_client_state.
		 */
		GLVertexArray &
		gl_tex_coord_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint array_offset)
		{
			const TexCoordPointer tex_coord_pointer = { size, type, stride, array_offset };
			return gl_tex_coord_pointer(tex_coord_pointer);
		}

		//! Same as the other overload of @a gl_tex_coord_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_tex_coord_pointer(
				const TexCoordPointer &tcp);


		/**
		 * Does the actual enabling/binding of the vertex array pointers to OpenGL.
		 *
		 * Takes all the information set up in the above 'gl*()' functions and applies them.
		 */
		void
		bind() const;

	private:
		/**
		 * The opaque vertex array data.
		 */
		GLArray::non_null_ptr_type d_array_data;

		//! Which texture unit are we currently directing to ?
		GLenum d_client_active_texture_ARB;

		//
		// Enable/disable vertex attributes client state.
		//
		// This state needs to be stored and executed later (during @a bind) for both
		// types of vertex array.
		//
		bool d_enable_vertex_array;
		bool d_enable_color_array;
		bool d_enable_normal_array;
		// GL_ARB_multitexture supports up 32 textures
		std::bitset<32> d_enable_texture_coord_array;

		//
		// Optional vertex attribute pointers.
		//
		// Pointers (offsets) to the various vertex attributes.
		//
		boost::optional<VertexPointer> d_vertex_pointer;
		boost::optional<ColorPointer> d_color_pointer;
		boost::optional<NormalPointer> d_normal_pointer;
		// GL_ARB_multitexture supports up 32 textures
		boost::optional<TexCoordPointer> d_tex_coord_pointer[32];


		//! Constructor.
		explicit
		GLVertexArray(
				const GLArray::non_null_ptr_type &array_data);


		void
		set_enable_disable_client_state(
				GLenum array,
				bool enable);

		void
		bind_client_state() const;

		void
		bind_attribute_pointers(
				const GLubyte *array_data) const;
	};
}

#endif // GPLATES_OPENGL_GLVERTEXARRAY_H
