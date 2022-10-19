/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLCAPABILITIES_H
#define GPLATES_OPENGL_GLCAPABILITIES_H

#include <boost/noncopyable.hpp>
#include <qopengl.h>  // For OpenGL constants and typedefs.

#include "global/config.h" // For GPLATES_USE_VULKAN_BACKEND
#if !defined(GPLATES_USE_VULKAN_BACKEND)
#	include <QOpenGLContext>
#endif


namespace GPlatesOpenGL
{
	class GLContext;
	class OpenGLFunctions;

	/**
	 * Various OpenGL implementation-dependent capabilities and parameters.
	 */
	class GLCapabilities :
			// Don't want clients copying and caching capabilities - must be retrieved from a GLContext...
			private boost::noncopyable
	{
	public:

		//
		// Viewport
		//

		GLuint gl_max_viewports; // GL_MAX_VIEWPORTS
		GLuint gl_max_viewport_dims[2]; // GL_MAX_VIEWPORT_DIMS


		//
		// Framebuffer
		//

		GLuint gl_max_color_attachments; // GL_MAX_COLOR_ATTACHMENTS

		GLuint gl_max_renderbuffer_size; // GL_MAX_RENDERBUFFER_SIZE

		GLuint gl_max_draw_buffers; // GL_MAX_DRAW_BUFFERS
		GLuint gl_max_dual_source_draw_buffers; // GL_MAX_DUAL_SOURCE_DRAW_BUFFERS

		/**
		 * Number of bits of sub-pixel precision in pixel rasterizer.
		 *
		 * OpenGL specifies a minimum of 4 bits, but most consumer hardware these days support 8 bits.
		 */
		GLuint gl_sub_pixel_bits; // GL_SUBPIXEL_BITS

		GLuint gl_max_sample_mask_words; // GL_MAX_SAMPLE_MASK_WORDS


		//
		// Buffer
		//

		// Alignment.
		GLuint gl_uniform_buffer_offset_alignment; // GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
		GLuint gl_shader_storage_buffer_offset_alignment; // GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT

		// Block size.
		GLuint64 gl_max_uniform_block_size; // GL_MAX_UNIFORM_BLOCK_SIZE
		GLuint64 gl_max_shader_storage_block_size; // GL_MAX_SHADER_STORAGE_BLOCK_SIZE


		//
		// Shader
		//

		GLuint gl_max_vertex_attribs; // GL_MAX_VERTEX_ATTRIBS
		GLuint gl_max_vertex_attrib_bindings; // GL_MAX_VERTEX_ATTRIB_BINDINGS

		GLuint gl_max_clip_distances; // GL_MAX_CLIP_DISTANCES

		// Texture image units.
		GLuint gl_max_combined_texture_image_units; // GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
		GLuint gl_max_vertex_texture_image_units; // GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS
		GLuint gl_max_texture_image_units/*fragment shader*/; // GL_MAX_TEXTURE_IMAGE_UNITS
		GLuint gl_max_compute_texture_image_units; // GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS

		// Image units.
		GLuint gl_max_image_units/*combined*/; // GL_MAX_IMAGE_UNITS
		GLuint gl_max_fragment_image_uniforms; // GL_MAX_FRAGMENT_IMAGE_UNIFORMS
		GLuint gl_max_compute_image_uniforms; // GL_MAX_COMPUTE_IMAGE_UNIFORMS

		// Buffer bindings.
		GLuint gl_max_uniform_buffer_bindings; // GL_MAX_UNIFORM_BUFFER_BINDINGS
		GLuint gl_max_shader_storage_buffer_bindings; // GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
		GLuint gl_max_atomic_counter_buffer_bindings; // GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS

		// Uniform blocks.
		GLuint gl_max_combined_uniform_blocks; // GL_MAX_COMBINED_UNIFORM_BLOCKS
		GLuint gl_max_vertex_uniform_blocks; // GL_MAX_VERTEX_UNIFORM_BLOCKS
		GLuint gl_max_fragment_uniform_blocks; // GL_MAX_FRAGMENT_UNIFORM_BLOCKS
		GLuint gl_max_compute_uniform_blocks; // GL_MAX_COMPUTE_UNIFORM_BLOCKS

		// Shader storage blocks.
		GLuint gl_max_combined_shader_storage_blocks; // GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS
		GLuint gl_max_fragment_shader_storage_blocks; // GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS
		GLuint gl_max_compute_shader_storage_blocks; // GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS

		// Atomic counters.
		GLuint gl_max_combined_atomic_counters; // GL_MAX_COMBINED_ATOMIC_COUNTERS
		GLuint gl_max_fragment_atomic_counters; // GL_MAX_FRAGMENT_ATOMIC_COUNTERS
		GLuint gl_max_compute_atomic_counters; // GL_MAX_COMPUTE_ATOMIC_COUNTERS

		// Atomic counter buffers.
		GLuint gl_max_combined_atomic_counter_buffers; // GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS
		GLuint gl_max_fragment_atomic_counter_buffers; // GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS
		GLuint gl_max_compute_atomic_counter_buffers; // GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS

		//
		// Texture
		//

		GLuint gl_max_texture_size; // GL_MAX_TEXTURE_SIZE

		GLuint gl_max_texture_buffer_size; // GL_MAX_TEXTURE_BUFFER_SIZE

		GLuint gl_max_cube_map_texture_size; // GL_MAX_CUBE_MAP_TEXTURE_SIZE

		/**
		 * Is GL_EXT_texture_filter_anisotropic supported?
		 *
		 * Turns out its support is ubiquitous but it was not made core until OpenGL 4.6.
		 * https://www.khronos.org/opengl/wiki/Ubiquitous_Extension
		 */
		bool gl_EXT_texture_filter_anisotropic;

		/**
		 * The maximum texture filtering anisotropy supported by the
		 * GL_EXT_texture_filter_anisotropic extension (or 1.0 if it's not supported).
		 */
		GLfloat gl_texture_max_anisotropy; // GL_TEXTURE_MAX_ANISOTROPY

		//! The number of texture array layers supported.
		GLuint gl_max_texture_array_layers;  // GL_MAX_ARRAY_TEXTURE_LAYERS


	private: // For use by @a GLContext...

		//
		// Only GLContext can create a GLCapabilities - this is to prevent clients
		// from creating and initialising their own GLCapabilities - it must be initialised
		// from a GLContext once it has been initialised.
		//
		friend class GLContext;

		GLCapabilities();

		void
		initialise(
				OpenGLFunctions &opengl_functions
#if !defined(GPLATES_USE_VULKAN_BACKEND)
				, const QOpenGLContext& opengl_context
#endif
		);

		bool
		is_initialised() const
		{
			return d_is_initialised;
		}

		//! Had @a initialise been called yet.
		bool d_is_initialised;

	private:

		/**
		 * Calls 'glGetIntegerv'.
		 *
		 * Returns GLint result as GLuint to avoid unsigned/signed comparison compiler warnings.
		 */
		static
		GLuint
		query_integer(
				OpenGLFunctions &opengl_functions,
				GLenum pname);

		/**
		 * Calls 'glGetInteger64v'.
		 *
		 * Returns GLint64 result as GLuint64 to avoid unsigned/signed comparison compiler warnings.
		 */
		static
		GLuint64
		query_integer64(
				OpenGLFunctions &opengl_functions,
				GLenum pname);
	};
}

#endif // GPLATES_OPENGL_GLCAPABILITIES_H
