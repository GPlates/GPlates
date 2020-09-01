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

#include <opengl/OpenGL1.h>


namespace GPlatesOpenGL
{
	class GLContext;

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

		GLuint gl_max_viewport_dims[2]; // GL_MAX_VIEWPORT_DIMS query result


		//
		// Framebuffer
		//

		/**
		 * Simply GL_COLOR_ATTACHMENT0.
		 *
		 * Note that GL_COLOR_ATTACHMENT0 is not defined in <opengl/OpenGL1.h>, so this definition is here solely
		 * to avoid forcing the include of <opengl/OpenGL3.h> which cannot be included from header files.
		 */
		static const GLenum gl_COLOR_ATTACHMENT0; // GL_COLOR_ATTACHMENT0

		GLuint gl_max_color_attachments; // GL_MAX_COLOR_ATTACHMENTS query result

		GLuint gl_max_renderbuffer_size; // GL_MAX_RENDERBUFFER_SIZE query result

		GLuint gl_max_draw_buffers; // GL_MAX_DRAW_BUFFERS query result
		GLuint gl_max_dual_source_draw_buffers; // GL_MAX_DUAL_SOURCE_DRAW_BUFFERS query result

		/**
		 * Number of bits of sub-pixel precision in pixel rasterizer.
		 *
		 * OpenGL specifies a minimum of 4 bits, but most consumer hardware these days support 8 bits.
		 */
		GLuint gl_sub_pixel_bits; // GL_SUBPIXEL_BITS query result

		GLuint gl_max_sample_mask_words; // GL_MAX_SAMPLE_MASK_WORDS query result


		//
		// Shader
		//

		GLuint gl_max_vertex_attribs; // GL_MAX_VERTEX_ATTRIBS query result

		GLuint gl_max_clip_distances; // GL_MAX_CLIP_DISTANCES query result

		//! Maximum number of texture image units available to vertex/geometry/fragment shaders combined.
		GLuint gl_max_combined_texture_image_units; // GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS query result

		//! Maximum number of texture image units available to fragment shader.
		GLuint gl_max_texture_image_units; // GL_MAX_TEXTURE_IMAGE_UNITS query result
		GLuint gl_max_fragment_uniform_components; // GL_MAX_FRAGMENT_UNIFORM_COMPONENTS query result

		GLuint gl_max_vertex_texture_image_units; // GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS query result
		GLuint gl_max_vertex_output_components; // GL_MAX_VERTEX_OUTPUT_COMPONENTS query result
		GLuint gl_max_vertex_uniform_components; // GL_MAX_VERTEX_UNIFORM_COMPONENTS query result

		GLuint gl_max_geometry_texture_image_units; // GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS query result
		GLuint gl_max_geometry_output_vertices; // GL_MAX_GEOMETRY_OUTPUT_VERTICES query result
		GLuint gl_max_geometry_input_components; // GL_MAX_GEOMETRY_INPUT_COMPONENTS query result
		GLuint gl_max_geometry_output_components; // GL_MAX_GEOMETRY_OUTPUT_COMPONENTS query result
		GLuint gl_max_geometry_total_output_components; // GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS query result
		GLuint gl_max_geometry_uniform_components; // GL_MAX_GEOMETRY_UNIFORM_COMPONENTS query result

		
		//
		// Texture
		//

		/**
		 * Simply GL_TEXTURE0.
		 *
		 * Note that GL_TEXTURE0 is not defined in <opengl/OpenGL1.h>, so this definition is here solely
		 * to avoid forcing the include of <opengl/OpenGL3.h> which cannot be included from header files.
		 */
		static const GLenum gl_TEXTURE0; // GL_TEXTURE0

		GLuint gl_max_texture_size; // GL_MAX_TEXTURE_SIZE query result

		GLuint gl_max_cube_map_texture_size; // GL_MAX_CUBE_MAP_TEXTURE_SIZE query result

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
		GLfloat gl_texture_max_anisotropy; // GL_TEXTURE_MAX_ANISOTROPY query result

		//! The number of texture array layers supported.
		GLuint gl_max_texture_array_layers;  // GL_MAX_ARRAY_TEXTURE_LAYERS query result

	private:

		GLCapabilities();

		void
		initialise();

		/**
		 * Calls 'glGetIntegerv'.
		 *
		 * Returns GLint result as GLuint to avoid unsigned/signed comparison compiler warnings.
		 */
		static
		GLuint
		query_integer(
				GLenum pname);


		/**
		 * Only GLContext can create a GLCapabilities - this is to prevent clients
		 * from creating and initialising their own GLCapabilities - it must be initialised
		 * from a GLContext once the GLEW library has been initialised.
		 */
		friend class GLContext;
	};
}

#endif // GPLATES_OPENGL_GLCAPABILITIES_H
