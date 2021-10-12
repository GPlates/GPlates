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
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLArray.h"


namespace GPlatesOpenGL
{
	/**
	 * An OpenGL vertex array.
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
		create()
		{
			return shared_ptr_type(new GLVertexArray());
		}


		/**
		 * Creates a @a GLVertexArray object.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * @a VertexType is the type (or structure) of the vertex data passed in.
		 * NOTE: we don't keep track of the vertex structure.
		 */
		template <class VertexType>
		static
		shared_ptr_type
		create(
				const std::vector<VertexType> &array_data)
		{
			return shared_ptr_type(new GLVertexArray(GLArray(array_data)));
		}


		/**
		 * Creates a @a GLVertexArray object.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * @a VertexForwardIter is an iterator of the type (or structure) of the vertex data passed in.
		 * NOTE: we don't keep track of the vertex structure.
		 */
		template <typename VertexForwardIter>
		static
		shared_ptr_type
		create(
				VertexForwardIter begin,
				VertexForwardIter end)
		{
			return shared_ptr_type(new GLVertexArray(GLArray(begin, end)));
		}


		/**
		 * Creates a @a GLVertexArray object.
		 * 
		 * Ownership of the vertex array data is shared internally.
		 *
		 * @a VertexType is the type (or structure) of the vertex data passed in.
		 * NOTE: we don't keep track of the vertex structure.
		 */
		template <class VertexType>
		static
		shared_ptr_type
		create(
				const boost::shared_array<VertexType> &array_data)
		{
			return shared_ptr_type(new GLVertexArray(GLArray(array_data)));
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
		 * NOTE: If 'VertexType' is a different type to that passed to @a create
		 * then you may need to reset the offsets with @a gl_vertex_pointer, @a gl_color_pointer, etc.
		 */
		template <class VertexType>
		void
		set_array_data(
				const std::vector<VertexType> &array_data)
		{
			d_array_data = GLArray(array_data);
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexArray.
		 *
		 * The vertex array data is copied into an internal array.
		 *
		 * NOTE: If 'VertexType' is a different type to that passed to @a create
		 * then you may need to reset the offsets with @a gl_vertex_pointer, @a gl_color_pointer, etc.
		 */
		template <typename VertexForwardIter>
		void
		set_array_data(
				VertexForwardIter begin,
				VertexForwardIter end)
		{
			d_array_data = GLArray(begin, end);
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexArray.
		 * 
		 * Ownership of the vertex array data is shared internally.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 *
		 * NOTE: If 'VertexType' is a different type to that passed to @a create
		 * then you may need to reset the offsets with @a gl_vertex_pointer, @a gl_color_pointer, etc.
		 */
		template <class VertexType>
		void
		set_array_data(
				const boost::shared_array<VertexType> &array_data)
		{
			d_array_data = GLArray(array_data);
		}


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
				GLint array_offset);

		//! Same as the other overload of @a gl_vertex_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_vertex_pointer(
				const VertexPointer &vp)
		{
			gl_vertex_pointer(vp.size, vp.type, vp.stride, vp.array_offset);
			return *this;
		}


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
				GLint array_offset);

		//! Same as the other overload of @a gl_color_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_color_pointer(
				const ColorPointer &cp)
		{
			gl_color_pointer(cp.size, cp.type, cp.stride, cp.array_offset);
			return *this;
		}


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
				GLint array_offset);

		//! Same as the other overload of @a gl_normal_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_normal_pointer(
				const NormalPointer &np)
		{
			gl_normal_pointer(np.type, np.stride, np.array_offset);
			return *this;
		}


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
				GLint array_offset);

		//! Same as the other overload of @a gl_tex_coord_pointer except takes arguments in a structure.
		GLVertexArray &
		gl_tex_coord_pointer(
				const TexCoordPointer &tcp)
		{
			gl_tex_coord_pointer(tcp.size, tcp.type, tcp.stride, tcp.array_offset);
			return *this;
		}


		/**
		 * Does the actual enabling/binding of the vertex array pointers to OpenGL.
		 *
		 * Takes all the information set up in the above 'gl*()' functions and applies them.
		 */
		void
		bind() const;

	private:
		/**
		 * The vertex data.
		 */
		boost::optional<GLArray> d_array_data;

		bool d_enable_vertex_array;
		boost::optional<VertexPointer> d_vertex_pointer;

		bool d_enable_color_array;
		boost::optional<ColorPointer> d_color_pointer;

		bool d_enable_normal_array;
		boost::optional<NormalPointer> d_normal_pointer;

		// Which texture unit are we currently directing to ?
		GLenum d_client_active_texture_ARB;
		// GL_ARB_multitexture supports up 32 textures
		std::bitset<32> d_enable_texture_coord_array;
		boost::optional<TexCoordPointer> d_tex_coord_pointer[32];


		//! Default constructor.
		GLVertexArray();

		//! Constructor.
		explicit
		GLVertexArray(
				const GLArray &array_data);

		void
		set_enable_disable_client_state(
				GLenum array,
				bool enable);

		void
		bind_vertex_pointer() const;

		void
		bind_color_pointer() const;

		void
		bind_normal_pointer() const;

		void
		bind_tex_coord_pointers() const;

		void
		bind_tex_coord_pointer(
				std::size_t texture_unit) const;
	};
}

#endif // GPLATES_OPENGL_GLVERTEXARRAY_H
