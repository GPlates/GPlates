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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLCapabilities.h"

#include "global/CompilerWarnings.h"

// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


// Set the GL_COLOR_ATTACHMENT0_EXT constant.
const GLenum GPlatesOpenGL::GLCapabilities::Framebuffer::gl_COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0_EXT;

// Set the GL_TEXTURE0 constant.
const GLenum GPlatesOpenGL::GLCapabilities::Texture::gl_TEXTURE0 = GL_TEXTURE0;


void
GPlatesOpenGL::GLCapabilities::initialise()
{
	if (GLEW_VERSION_1_2)
	{
		gl_version_1_2 = true;
	}
	if (GLEW_VERSION_1_4)
	{
		gl_version_1_4 = true;
	}

	qDebug() << "On this system GPlates supports the following OpenGL extensions...";

	initialise_viewport();
	initialise_framebuffer();
	initialise_shader();
	initialise_texture();
	initialise_vertex();
	initialise_buffer();

	qDebug() << "...end of OpenGL extension list.";
}


void
GPlatesOpenGL::GLCapabilities::initialise_viewport()
{
#ifdef GL_ARB_viewport_array // In case old 'glew.h' header (GL_ARB_viewport_array is fairly recent)
	if (GLEW_ARB_viewport_array)
	{
		viewport.gl_ARB_viewport_array = true;

		// Get the maximum number of viewports.
		GLint max_viewports;
		glGetIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		viewport.gl_max_viewports = max_viewports;

		qDebug() << "  GL_ARB_viewport_array";
	}
#endif

	// Query the maximum viewport dimensions supported.
	GLint max_viewport_dimensions[2];
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &max_viewport_dimensions[0]);
	viewport.gl_max_viewport_width = max_viewport_dimensions[0];
	viewport.gl_max_viewport_height = max_viewport_dimensions[1];
}


void
GPlatesOpenGL::GLCapabilities::initialise_framebuffer()
{
	if (GLEW_EXT_framebuffer_object)
	{
		framebuffer.gl_EXT_framebuffer_object = true;

		// Get the maximum number of color attachments.
		GLint max_color_attachments;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &max_color_attachments);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		framebuffer.gl_max_color_attachments = max_color_attachments;

		// Get the maximum render buffer size.
		GLint max_renderbuffer_size;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &max_renderbuffer_size);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		framebuffer.gl_max_renderbuffer_size = max_renderbuffer_size;

		qDebug() << "  GL_EXT_framebuffer_object";
	}

	if (GLEW_ARB_draw_buffers)
	{
		framebuffer.gl_ARB_draw_buffers = true;

		// Get the maximum number of draw buffers (multiple render targets).
		GLint max_draw_buffers;
		glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &max_draw_buffers);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		framebuffer.gl_max_draw_buffers = max_draw_buffers;

		qDebug() << "  GL_ARB_draw_buffers";
	}

	if (GLEW_EXT_packed_depth_stencil)
	{
		framebuffer.gl_EXT_packed_depth_stencil = true;

		qDebug() << "  GL_EXT_packed_depth_stencil";
	}

	if (GLEW_EXT_blend_equation_separate)
	{
		framebuffer.gl_EXT_blend_equation_separate = true;

		qDebug() << "  GL_EXT_blend_equation_separate";
	}

	if (GLEW_EXT_blend_func_separate)
	{
		framebuffer.gl_EXT_blend_func_separate = true;

		qDebug() << "  GL_EXT_blend_func_separate";
	}

	if (GLEW_EXT_blend_minmax)
	{
		framebuffer.gl_EXT_blend_minmax = true;

		qDebug() << "  GL_EXT_blend_minmax";
	}
}


void
GPlatesOpenGL::GLCapabilities::initialise_shader()
{
	if (GLEW_ARB_shader_objects)
	{
		shader.gl_ARB_shader_objects = true;

		qDebug() << "  GL_ARB_shader_objects";
	}

	if (GLEW_ARB_vertex_shader)
	{
		shader.gl_ARB_vertex_shader = true;

		// Get the maximum supported number of generic vertex attributes.
		GLint max_vertex_attribs;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &max_vertex_attribs);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		shader.gl_max_vertex_attribs = max_vertex_attribs;

		qDebug() << "  GL_ARB_vertex_shader";
	}

	if (GLEW_ARB_fragment_shader)
	{
		shader.gl_ARB_fragment_shader = true;

		qDebug() << "  GL_ARB_fragment_shader";
	}

#ifdef GL_EXT_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	if (GLEW_EXT_geometry_shader4)
	{
		shader.gl_EXT_geometry_shader4 = true;

		// Query as signed but store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		GLint max_value;
		glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT, &max_value);
		shader.gl_max_geometry_texture_image_units = max_value;
		glGetIntegerv(GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT, &max_value);
		shader.gl_max_geometry_varying_components = max_value;
		glGetIntegerv(GL_MAX_VERTEX_VARYING_COMPONENTS_EXT, &max_value);
		shader.gl_max_vertex_varying_components = max_value;
		glGetIntegerv(GL_MAX_VARYING_COMPONENTS_EXT, &max_value);
		shader.gl_max_varying_components = max_value;
		glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT, &max_value);
		shader.gl_max_geometry_uniform_components = max_value;
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &max_value);
		shader.gl_max_geometry_output_vertices = max_value;
		glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT, &max_value);
		shader.gl_max_geometry_total_output_components = max_value;

		qDebug() << "  GL_EXT_geometry_shader4";
	}
#endif

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	if (GLEW_EXT_gpu_shader4)
	{
		shader.gl_EXT_gpu_shader4 = true;

		qDebug() << "  GL_EXT_gpu_shader4";
	}
#endif

// In case old 'glew.h' (since extension added relatively recently).
#ifdef GL_ARB_gpu_shader_fp64
	// Some versions of 'glew.h' appear to define 'GL_ARB_gpu_shader_fp64' but not the 'glUniform...' API functions.
	// And there are also some run-time systems that report 'GLEW_ARB_gpu_shader_fp64' as 'false', indicating
	// they don't support the extension, yet still provide non-null 'glUniform...' API functions.
	// Oddly this extension can report unsupported if functions like 'glProgramUniform1dEXT' are not found.
	// However we don't need these 'direct access state' functions - as long as we have the
	// regular uniform function like 'glUniform1d' that's all we need.
	// Note that Mac OSX (even Lion) doesn't support this extension so this is a
	// Windows and Linux only extension.
	//
	// So our way of detecting this extension is just to look for non-null 'glUniform...' API functions.
	// If they are there then we turn the extension on, otherwise we turn it off.
	//
	// FIXME: Find a better way to override the extension (to disable it).
	// This is the global flag used by 'GLEW_ARB_gpu_shader_fp64'.
	__GLEW_ARB_gpu_shader_fp64 = false;

	#if defined(glUniform1d) && \
		defined(glUniform1dv) && \
		defined(glUniform2d) && \
		defined(glUniform2dv) && \
		defined(glUniform3d) && \
		defined(glUniform3dv) && \
		defined(glUniform4d) && \
		defined(glUniform4dv) && \
		defined(glUniformMatrix2dv) && \
		defined(glUniformMatrix2x3dv) && \
		defined(glUniformMatrix2x4dv) && \
		defined(glUniformMatrix3dv) && \
		defined(glUniformMatrix3x2dv) && \
		defined(glUniformMatrix3x4dv) && \
		defined(glUniformMatrix4dv) && \
		defined(glUniformMatrix4x2dv) && \
		defined(glUniformMatrix4x3dv)

		// We know they've been defined in the 'glew.h' file so check that they return non-null
		// indicating they are available on the run-time system...
		if (
			GPLATES_OPENGL_BOOL(glUniform1d) &&
			GPLATES_OPENGL_BOOL(glUniform1dv) &&
			GPLATES_OPENGL_BOOL(glUniform2d) &&
			GPLATES_OPENGL_BOOL(glUniform2dv) &&
			GPLATES_OPENGL_BOOL(glUniform3d) &&
			GPLATES_OPENGL_BOOL(glUniform3dv) &&
			GPLATES_OPENGL_BOOL(glUniform4d) &&
			GPLATES_OPENGL_BOOL(glUniform4dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix2dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix2x3dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix2x4dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix3dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix3x2dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix3x4dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix4dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix4x2dv) &&
			GPLATES_OPENGL_BOOL(glUniformMatrix4x3dv))
		{
			shader.gl_ARB_gpu_shader_fp64 = true;

			// FIXME: Find a better way to override the extension (to enable it).
			// This is the global flag used by 'GLEW_ARB_gpu_shader_fp64'.
			__GLEW_ARB_gpu_shader_fp64 = true;

			qDebug() << "  GL_ARB_gpu_shader_fp64";
		}
	#endif
#endif

#ifdef GL_ARB_vertex_attrib_64bit // In case old 'glew.h' (since extension added relatively recently).
	if (GLEW_ARB_vertex_attrib_64bit)
	{
		shader.gl_ARB_vertex_attrib_64bit = true;

		qDebug() << "  GL_ARB_vertex_attrib_64bit";
	}
#endif
}


void
GPlatesOpenGL::GLCapabilities::initialise_texture()
{
	// Get the maximum texture size (dimension).
	GLint max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
	texture.gl_max_texture_size = max_texture_size;

	// Get the maximum cube map texture size (dimension).
	if (GLEW_ARB_texture_cube_map)
	{
		texture.gl_ARB_texture_cube_map = true;;
		qDebug() << "  GL_ARB_texture_cube_map";

		GLint max_cube_map_texture_size;
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &max_cube_map_texture_size);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture.gl_max_cube_map_texture_size = max_cube_map_texture_size;
	}

	// Are non-power-of-two dimension textures supported?
	if (GLEW_ARB_texture_non_power_of_two)
	{
		texture.gl_ARB_texture_non_power_of_two = true;;

		qDebug() << "  GL_ARB_texture_non_power_of_two";
	}

	// Get the maximum number of texture units supported.
	if (GLEW_ARB_multitexture)
	{
		texture.gl_ARB_multitexture = true;

		GLint max_texture_units;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max_texture_units);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture.gl_max_texture_units = max_texture_units;

		qDebug() << "  GL_ARB_multitexture";
	}

	// Get the maximum number of texture *image* units and texture coordinates supported by fragment shaders.
	if (GLEW_ARB_fragment_shader)
	{
		GLint max_texture_image_units;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &max_texture_image_units);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture.gl_max_texture_image_units = max_texture_image_units;

		GLint max_texture_coords;
		glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &max_texture_coords);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture.gl_max_texture_coords = max_texture_coords;
	}
	else if (GLEW_ARB_multitexture)
	{
		// Fallback to the 'old-style' way of reporting texture units...
		GLint max_texture_units;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max_texture_units);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture.gl_max_texture_image_units = max_texture_units;
		texture.gl_max_texture_coords = max_texture_units;
	}
	// ...else they are both left as their default values of 1

	// Is clamping to the centre of texture edge pixels supported?
	//
	// This is the standard texture clamping in Direct3D - it's easier for hardware to implement
	// since it avoids accessing the texture border colour (even in (bi)linear filtering mode).
	//
	// Seems Mac OSX use the SGIS version exclusively but in general the EXT version is more common.
	if (GLEW_EXT_texture_edge_clamp)
	{
		texture.gl_EXT_texture_edge_clamp = true;;

		qDebug() << "  GL_EXT_texture_edge_clamp";
	}
	if (GLEW_SGIS_texture_edge_clamp)
	{
		texture.gl_SGIS_texture_edge_clamp = true;;

		qDebug() << "  GL_SGIS_texture_edge_clamp";
	}

	// Get the maximum texture anisotropy supported.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		texture.gl_EXT_texture_filter_anisotropic = true;

		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &texture.gl_texture_max_anisotropy);

		qDebug() << "  GL_EXT_texture_filter_anisotropic";
	}

	// Is GLEW_ARB_texture_env_combine supported?
	if (GLEW_ARB_texture_env_combine)
	{
		texture.gl_ARB_texture_env_combine = true;

		qDebug() << "  GL_ARB_texture_env_combine";
	}

	// Is GLEW_ARB_texture_env_dot3 supported?
	if (GLEW_ARB_texture_env_dot3)
	{
		texture.gl_ARB_texture_env_dot3 = true;

		qDebug() << "  GL_ARB_texture_env_dot3";
	}

	// Are 3D textures supported?
	// This used to test for GL_EXT_texture3D and GL_EXT_subtexture but they are not
	// exposed on some systems (notably MacOS) so instead this tests for core OpenGL 1.2.
	if (GLEW_VERSION_1_2)
	{
		texture.gl_is_texture3D_supported = true;

		if (GLEW_EXT_texture3D)
		{
			qDebug() << "  GL_EXT_texture3D";
		}
		else
		{
			qDebug() << "  GL_EXT_texture3D (in core 1.2)";
		}
	}

#ifdef GL_EXT_texture_array // In case old 'glew.h' header
	if (GLEW_EXT_texture_array)
	{
		texture.gl_EXT_texture_array = true;

		// Get the maximum number of texture array layers.
		GLint max_texture_array_layers;
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &max_texture_array_layers);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture.gl_max_texture_array_layers = max_texture_array_layers;

		qDebug() << "  GL_EXT_texture_array";
	}
#endif

	// Are texture buffer objects supported?
#ifdef GL_EXT_texture_buffer_object
	if (GLEW_EXT_texture_buffer_object)
	{
		texture.gl_EXT_texture_buffer_object = true;

		qDebug() << "  GL_EXT_texture_buffer_object";
	}
#endif

	// Is GLEW_ARB_texture_float supported?
	if (GLEW_ARB_texture_float)
	{
		texture.gl_ARB_texture_float = true;

		qDebug() << "  GL_ARB_texture_float";
	}

	// Is GLEW_ARB_texture_rg supported?
#ifdef GL_ARB_texture_rg
	if (GLEW_ARB_texture_rg)
	{
		texture.gl_ARB_texture_rg = true;

		qDebug() << "  GL_ARB_texture_rg";
	}
#endif

	// Is GLEW_ARB_color_buffer_float supported?
	// This affects things other than floating-point textures (samplers or render-targets) but
	// we put it with the texture parameters since it's most directly related to floating-point
	// colour buffers (eg, floating-point textures attached to a framebuffer object).
	if (GLEW_ARB_color_buffer_float)
	{
		texture.gl_ARB_color_buffer_float = true;

		qDebug() << "  GL_ARB_color_buffer_float";
	}

	// See if floating-point filtering/blending is supported.
	// See comment in Parameters::Texture for how this is detected.
	if (GLEW_VERSION_3_0 || GLEW_EXT_texture_array)
	{
		texture.gl_supports_floating_point_filtering_and_blending = true;
	}
}


void
GPlatesOpenGL::GLCapabilities::initialise_vertex()
{
	if (GLEW_EXT_draw_range_elements)
	{
		vertex.gl_EXT_draw_range_elements = true;

		qDebug() << "  GL_EXT_draw_range_elements";
	}
}


void
GPlatesOpenGL::GLCapabilities::initialise_buffer()
{
	if (GLEW_ARB_vertex_buffer_object)
	{
		buffer.gl_ARB_vertex_buffer_object = true;

		qDebug() << "  GL_ARB_vertex_buffer_object";
	}

#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	if (GLEW_ARB_vertex_array_object)
	{
		buffer.gl_ARB_vertex_array_object = true;

		qDebug() << "  GL_ARB_vertex_array_object";
	}
#endif

	if (GLEW_ARB_pixel_buffer_object)
	{
		buffer.gl_ARB_pixel_buffer_object = true;

		qDebug() << "  GL_ARB_pixel_buffer_object";
	}

	if (GLEW_ARB_map_buffer_range)
	{
		buffer.gl_ARB_map_buffer_range = true;

		qDebug() << "  GL_ARB_map_buffer_range";
	}

	if (GLEW_APPLE_flush_buffer_range)
	{
		buffer.gl_APPLE_flush_buffer_range = true;

		qDebug() << "  GL_APPLE_flush_buffer_range";
	}
}


GPlatesOpenGL::GLCapabilities::Viewport::Viewport() :
	gl_ARB_viewport_array(false),
	gl_max_viewports(1),
	gl_max_viewport_width(0),
	gl_max_viewport_height(0)
{
}


GPlatesOpenGL::GLCapabilities::Framebuffer::Framebuffer() :
	gl_EXT_framebuffer_object(false),
	gl_max_color_attachments(0),
	gl_max_renderbuffer_size(0),
	gl_ARB_draw_buffers(false),
	gl_max_draw_buffers(1),
	gl_EXT_packed_depth_stencil(false),
	gl_EXT_blend_equation_separate(false),
	gl_EXT_blend_func_separate(false),
	gl_EXT_blend_minmax(false)
{
}


GPlatesOpenGL::GLCapabilities::Shader::Shader() :
	gl_ARB_shader_objects(false),
	gl_ARB_vertex_shader(false),
	gl_ARB_fragment_shader(false),
	gl_EXT_geometry_shader4(false),
	gl_max_geometry_texture_image_units(0),
	gl_max_geometry_varying_components(0),
	gl_max_vertex_varying_components(0),
	gl_max_varying_components(0),
	gl_max_geometry_uniform_components(0),
	gl_max_geometry_output_vertices(0),
	gl_max_geometry_total_output_components(0),
	gl_EXT_gpu_shader4(false),
	gl_ARB_gpu_shader_fp64(false),
	gl_ARB_vertex_attrib_64bit(false),
	gl_max_vertex_attribs(0)
{
}


GPlatesOpenGL::GLCapabilities::Vertex::Vertex() :
	gl_EXT_draw_range_elements(false)
{
}


GPlatesOpenGL::GLCapabilities::Texture::Texture() :
	gl_max_texture_size(gl_MIN_TEXTURE_SIZE),
	gl_max_cube_map_texture_size(16/*OpenGL minimum value*/),
	gl_ARB_texture_cube_map(false),
	gl_ARB_texture_non_power_of_two(false),
	gl_ARB_multitexture(false),
	gl_max_texture_units(1),
	gl_max_texture_image_units(1),
	gl_max_texture_coords(1),
	gl_EXT_texture_filter_anisotropic(false),
	gl_texture_max_anisotropy(1.0f),
	gl_EXT_texture_edge_clamp(false),
	gl_SGIS_texture_edge_clamp(false),
	gl_ARB_texture_env_combine(false),
	gl_ARB_texture_env_dot3(false),
	gl_is_texture3D_supported(false),
	gl_EXT_texture_array(false),
	gl_max_texture_array_layers(1),
	gl_EXT_texture_buffer_object(false),
	gl_ARB_texture_float(false),
	gl_ARB_texture_rg(false),
	gl_ARB_color_buffer_float(false),
	gl_supports_floating_point_filtering_and_blending(false)
{
}


GPlatesOpenGL::GLCapabilities::Buffer::Buffer() :
	gl_ARB_vertex_buffer_object(false),
	gl_ARB_vertex_array_object(false),
	gl_ARB_pixel_buffer_object(false),
	gl_ARB_map_buffer_range(false),
	gl_APPLE_flush_buffer_range(false)
{
}
