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

#include <QDebug>

#include "GLCapabilities.h"

#include "OpenGLFunctions.h"

#include "global/config.h" // For GPLATES_USE_VULKAN_BACKEND


GPlatesOpenGL::GLCapabilities::GLCapabilities() :
	//
	// Note: Non-zero values represent the minimum required by OpenGL 3.3 (core) for all implementations.
	//
	gl_max_color_attachments(8),
	gl_max_renderbuffer_size(1024),
	gl_max_draw_buffers(8),
	gl_max_dual_source_draw_buffers(1),
	gl_sub_pixel_bits(4),
	gl_max_sample_mask_words(1),
	gl_uniform_buffer_offset_alignment(1),
	gl_max_vertex_attribs(16),
	gl_max_clip_distances(8),
	gl_max_combined_texture_image_units(48),
	gl_max_combined_uniform_blocks(36),
	gl_max_texture_image_units(16),
	gl_max_fragment_uniform_components(1024),
	gl_max_fragment_uniform_blocks(12),
	gl_max_vertex_texture_image_units(16),
	gl_max_vertex_output_components(64),
	gl_max_vertex_uniform_components(1024),
	gl_max_vertex_uniform_blocks(12),
	gl_max_geometry_texture_image_units(16),
	gl_max_geometry_output_vertices(256),
	gl_max_geometry_input_components(64),
	gl_max_geometry_output_components(128),
	gl_max_geometry_total_output_components(1024),
	gl_max_geometry_uniform_components(1024),
	gl_max_geometry_uniform_blocks(12),
	gl_max_texture_size(1024),
	gl_max_cube_map_texture_size(1024),
	gl_EXT_texture_filter_anisotropic(false),
	gl_texture_max_anisotropy(1.0f),
	gl_max_texture_array_layers(256),
	d_is_initialised(false)
{
	gl_max_viewport_dims[0] = 0;
	gl_max_viewport_dims[1] = 0;
}


void
GPlatesOpenGL::GLCapabilities::initialise(
		OpenGLFunctions &opengl_functions
#if !defined(GPLATES_USE_VULKAN_BACKEND)
	, const QOpenGLContext &opengl_context
#endif
)
{
	//
	// Viewport
	//

	GLint max_viewport_dims[2];
	opengl_functions.glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &max_viewport_dims[0]);
	gl_max_viewport_dims[0] = max_viewport_dims[0];
	gl_max_viewport_dims[1] = max_viewport_dims[1];


	//
	// Framebuffer
	//

	gl_max_color_attachments = query_integer(opengl_functions, GL_MAX_COLOR_ATTACHMENTS);
	gl_max_renderbuffer_size = query_integer(opengl_functions, GL_MAX_RENDERBUFFER_SIZE);
	gl_max_draw_buffers = query_integer(opengl_functions, GL_MAX_DRAW_BUFFERS);
	gl_max_dual_source_draw_buffers = query_integer(opengl_functions, GL_MAX_DUAL_SOURCE_DRAW_BUFFERS);
	gl_sub_pixel_bits = query_integer(opengl_functions, GL_SUBPIXEL_BITS);
	gl_max_sample_mask_words = query_integer(opengl_functions, GL_MAX_SAMPLE_MASK_WORDS);


	//
	// Buffer
	//

	gl_uniform_buffer_offset_alignment = query_integer(opengl_functions, GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);


	//
	// Shader
	//

	gl_max_vertex_attribs = query_integer(opengl_functions, GL_MAX_VERTEX_ATTRIBS);
	gl_max_clip_distances = query_integer(opengl_functions, GL_MAX_CLIP_DISTANCES);

	gl_max_combined_texture_image_units = query_integer(opengl_functions, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
	gl_max_combined_uniform_blocks = query_integer(opengl_functions, GL_MAX_COMBINED_UNIFORM_BLOCKS);

	gl_max_texture_image_units = query_integer(opengl_functions, GL_MAX_TEXTURE_IMAGE_UNITS);
	gl_max_fragment_uniform_components = query_integer(opengl_functions, GL_MAX_FRAGMENT_UNIFORM_COMPONENTS);
	gl_max_fragment_uniform_blocks = query_integer(opengl_functions, GL_MAX_FRAGMENT_UNIFORM_BLOCKS);

	gl_max_vertex_texture_image_units = query_integer(opengl_functions, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
	gl_max_vertex_output_components = query_integer(opengl_functions, GL_MAX_VERTEX_OUTPUT_COMPONENTS);
	gl_max_vertex_uniform_components = query_integer(opengl_functions, GL_MAX_VERTEX_UNIFORM_COMPONENTS);
	gl_max_vertex_uniform_blocks = query_integer(opengl_functions, GL_MAX_VERTEX_UNIFORM_BLOCKS);

	gl_max_geometry_texture_image_units = query_integer(opengl_functions, GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS);
	gl_max_geometry_output_vertices = query_integer(opengl_functions, GL_MAX_GEOMETRY_OUTPUT_VERTICES);
	gl_max_geometry_input_components = query_integer(opengl_functions, GL_MAX_GEOMETRY_INPUT_COMPONENTS);
	gl_max_geometry_output_components = query_integer(opengl_functions, GL_MAX_GEOMETRY_OUTPUT_COMPONENTS);
	gl_max_geometry_total_output_components = query_integer(opengl_functions, GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS);
	gl_max_geometry_uniform_components = query_integer(opengl_functions, GL_MAX_GEOMETRY_UNIFORM_COMPONENTS);
	gl_max_geometry_uniform_blocks = query_integer(opengl_functions, GL_MAX_GEOMETRY_UNIFORM_BLOCKS);


	//
	// Texture
	//

	gl_max_texture_size = query_integer(opengl_functions, GL_MAX_TEXTURE_SIZE);
	gl_max_cube_map_texture_size = query_integer(opengl_functions, GL_MAX_CUBE_MAP_TEXTURE_SIZE);

#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Is GL_EXT_texture_filter_anisotropic supported?
	//
	// Turns out its support is ubiquitous but it was not made core until OpenGL 4.6.
	// https://www.khronos.org/opengl/wiki/Ubiquitous_Extension
	//
	if (opengl_context.hasExtension("GL_EXT_texture_filter_anisotropic"))
	{
		gl_EXT_texture_filter_anisotropic = true;
		gl_texture_max_anisotropy = query_integer(opengl_functions, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
	}
#endif

	gl_max_texture_array_layers = query_integer(opengl_functions, GL_MAX_ARRAY_TEXTURE_LAYERS);

	// Capabilities have been successfully initialised.
	d_is_initialised = true;
}


GLuint
GPlatesOpenGL::GLCapabilities::query_integer(
		OpenGLFunctions &opengl_functions,
		GLenum pname)
{
	GLint value;
	opengl_functions.glGetIntegerv(pname, &value);

	// Returns GLint result as GLuint to avoid unsigned/signed comparison compiler warnings. 
	return value;
}
