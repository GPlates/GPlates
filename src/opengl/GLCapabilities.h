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

#include <opengl/OpenGL.h>


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

		//! Parameters related to viewports.
		struct Viewport
		{
			Viewport();

			//! GL_MAX_VIEWPORTS query result - defaults to one.
			GLuint gl_max_viewports;

			//! Maximum supported width of viewport - is at least as large as display being renderer to.
			GLuint gl_max_viewport_width;

			//! Maximum supported height of viewport - is at least as large as display being renderer to.
			GLuint gl_max_viewport_height;
		};

		//! Parameters related to the framebuffers.
		struct Framebuffer
		{
			Framebuffer();

			/**
			 * Simply GL_COLOR_ATTACHMENT0_EXT.
			 *
			 * This is here solely so we can include <GL/glew.h>, which defines
			 * GL_COLOR_ATTACHMENT0_EXT, in "GLContext.cc" and hence avoid problems caused by
			 * including <GL/glew.h> in header files (because <GL/glew.h> must be included
			 * before OpenGL headers which means before Qt headers which is difficult).
			 */
			static const GLenum gl_COLOR_ATTACHMENT0; // GL_COLOR_ATTACHMENT0

			//! Is GL_EXT_framebuffer_object supported?
			bool gl_EXT_framebuffer_object;

			/**
			 * GL_MAX_COLOR_ATTACHMENTS query result - zero if GL_EXT_framebuffer_object not supported.
			 *
			 * NOTE: The GL_EXT_framebuffer_object extension says it's possible for this value
			 * to change when binding a framebuffer object or changing its attachment state
			 * in which case it probably belongs to class @a GLFrameBufferObject but we're keeping
			 * it here because it's unlikely to change and it's awkward from a programming perspective
			 * to first have to set up framebuffer object attachments and then determine the
			 * maximum allowed attachments (given the framebuffer object state).
			 */
			GLuint gl_max_color_attachments;

			/**
			 * GL_MAX_RENDERBUFFER_SIZE query result - zero if GL_EXT_framebuffer_object not supported.
			 */
			GLuint gl_max_renderbuffer_size;

			//! Is GL_ARB_draw_buffers supported?
			bool gl_ARB_draw_buffers;

			/**
			 * GL_MAX_DRAW_BUFFERS query result - one if GL_ARB_draw_buffers not supported.
			 *
			 * NOTE: The GL_EXT_framebuffer_object extension says it's possible for this value
			 * to change when binding a framebuffer object or changing its attachment state
			 * in which case it probably belongs to class @a GLFrameBufferObject but we're keeping
			 * it here because it's unlikely to change and it's awkward from a programming perspective
			 * to first have to set up framebuffer object attachments and then determine the
			 * maximum allowed draw buffers (given the framebuffer object state).
			 */
			GLuint gl_max_draw_buffers;

			//! Is GL_EXT_blend_equation_separate supported?
			bool gl_EXT_blend_equation_separate;

			//! Is GL_EXT_blend_func_separate supported?
			bool gl_EXT_blend_func_separate;

			//! Is GL_EXT_blend_minmax supported?
			bool gl_EXT_blend_minmax;
		};

		struct Shader
		{
			Shader();

			//! Is GL_ARB_shader_objects supported?
			bool gl_ARB_shader_objects;

			//! Is GL_ARB_vertex_shader supported?
			bool gl_ARB_vertex_shader;

			//! Is GL_ARB_fragment_shader supported?
			bool gl_ARB_fragment_shader;

			//! Is GL_EXT_geometry_shader4 supported?
			bool gl_EXT_geometry_shader4;

			// Limits related to the GL_EXT_geometry_shader4 extension...
			// All are zero if GL_EXT_geometry_shader4 is not supported.
			GLuint gl_max_geometry_texture_image_units; // GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT
			GLuint gl_max_geometry_varying_components; // GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT
			GLuint gl_max_vertex_varying_components; // GL_MAX_VERTEX_VARYING_COMPONENTS_EXT
			GLuint gl_max_varying_components; // GL_MAX_VARYING_COMPONENTS_EXT
			GLuint gl_max_geometry_uniform_components; // GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT
			GLuint gl_max_geometry_output_vertices; // GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT
			GLuint gl_max_geometry_total_output_components; // GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT

			//! Is GL_EXT_gpu_shader4 supported?
			bool gl_EXT_gpu_shader4;

			//! Is GL_ARB_gpu_shader_fp64 supported?
			bool gl_ARB_gpu_shader_fp64;

			//! Is GL_ARB_vertex_attrib_64bit supported?
			bool gl_ARB_vertex_attrib_64bit;

			/**
			 * The maximum number of generic vertex attributes supported by the
			 * GL_ARB_vertex_shader extension (or zero if it's not supported).
			 */
			GLuint gl_max_vertex_attribs; // GL_MAX_VERTEX_ATTRIBS_ARB query result
		};

		//! Parameters related to textures.
		struct Texture
		{
			Texture();

			/**
			 * Simply GL_TEXTURE0.
			 *
			 * This is here solely so we can include <GL/glew.h>, which defines
			 * GL_TEXTURE0, in "GLContext.cc" and hence avoid problems caused by
			 * including <GL/glew.h> in header files (because <GL/glew.h> must be included
			 * before OpenGL headers which means before Qt headers which is difficult).
			 */
			static const GLenum gl_TEXTURE0; // GL_TEXTURE0

			/**
			 * The minimum texture size (dimension) that all OpenGL implementations
			 * are required to support - this is without texture borders.
			 */
			static const GLuint gl_MIN_TEXTURE_SIZE = 64;

			/**
			 * The maximum texture size (dimension) this OpenGL implementation/driver will support.
			 * This is without texture borders and will be a power-of-two.
			 *
			 * NOTE: This doesn't necessarily mean it will be hardware accelerated but
			 * it probably will be, especially if we use standard formats like GL_RGBA8.
			 */
			GLuint gl_max_texture_size; // GL_MAX_TEXTURE_SIZE query result

			/**
			 * The maximum cube map texture size (dimension) this OpenGL implementation/driver will support.
			 * This is without texture borders and will be a power-of-two.
			 */
			GLuint gl_max_cube_map_texture_size; // GL_MAX_CUBE_MAP_TEXTURE_SIZE query result

			//! Is GL_ARB_texture_cube_map supported?
			bool gl_ARB_texture_cube_map;

			//! Is GL_ARB_texture_non_power_of_two supported?
			bool gl_ARB_texture_non_power_of_two;

			/**
			 * The maximum number of texture units supported by the
			 * GL_ARB_multitexture extension (or one if it's not supported).
			 *
			 * NOTE: This is the 'old style' number of texture units where number of texture
			 * coordinates and number of texture images is the same.
			 *
			 * NOTE: This value should be used when using the fixed-function pipeline.
			 * For fragment shaders you can use @a gl_max_texture_image_units and
			 * @a gl_max_texture_coords which are either the same as @a gl_max_texture_units or larger.
			 * But you can *not* use them for the fixed-function pipeline.
			 */
			GLuint gl_max_texture_units; // GL_MAX_TEXTURE_UNITS query result

			/**
			 * The maximum number of texture *image* units supported by the
			 * GL_ARB_fragment_shader extension (or @a gl_max_texture_units if it's not supported).
			 *
			 * NOTE: This is the 'new style' number of texture units where number of texture
			 * *image* units differs to the number of texture coordinates.
			 *
			 * NOTE: This can be used for fragment shaders (can also use @a gl_max_texture_units
			 * but it's less than or equal to @a gl_max_texture_image_units).
			 */
			GLuint gl_max_texture_image_units; // GL_MAX_TEXTURE_IMAGE_UNITS query result

			/**
			 * The maximum number of texture coordinates supported by the
			 * GL_ARB_fragment_shader extension (or @a gl_max_texture_units if it's not supported).
			 *
			 * NOTE: This is the 'new style' number of texture units where number of texture
			 * *image* units differs to the number of texture coordinates.
			 *
			 * NOTE: This can be used for fragment shaders (can also use @a gl_max_texture_units
			 * but it's less than or equal to @a gl_max_texture_coords).
			 */
			GLuint gl_max_texture_coords; // GL_MAX_TEXTURE_COORDS query result

			/**
			 * The maximum texture filtering anisotropy supported by the
			 * GL_EXT_texture_filter_anisotropic extension (or 1.0 if it's not supported).
			 */
			GLfloat gl_texture_max_anisotropy; // GL_TEXTURE_MAX_ANISOTROPY query result

			/**
			 * Is GL_EXT_texture_edge_clamp supported?
			 *
			 * This is the standard texture clamping in Direct3D - it's easier for hardware to implement
			 * since it avoids accessing the texture border colour (even in (bi)linear filtering mode).
			 */
			bool gl_EXT_texture_edge_clamp;

			//! Is GL_SGIS_texture_edge_clamp supported? Same as GL_EXT_texture_edge_clamp extension really.
			bool gl_SGIS_texture_edge_clamp;

			//! Is GL_ARB_texture_env_combine supported?
			bool gl_ARB_texture_env_combine;

			//! Is GL_ARB_texture_env_dot3 supported?
			bool gl_ARB_texture_env_dot3;

			/**
			 * Are 3D textures supported?
			 *
			 * This used to test for GL_EXT_texture3D and GL_EXT_subtexture but they are not
			 * exposed on some systems (notably MacOS) so instead this tests for core OpenGL 1.2.
			 */
			bool gl_is_texture3D_supported;

			//! Is GL_EXT_texture_array supported?
			bool gl_EXT_texture_array;

			//! The number of texture array layers supported - is 1 if GL_EXT_texture_array not supported.
			GLuint gl_max_texture_array_layers;

			//! Is GL_EXT_texture_buffer_object supported?
			bool gl_EXT_texture_buffer_object;

			//! Is GL_ARB_texture_float supported?
			bool gl_ARB_texture_float;

			//! Is GL_ARB_texture_rg supported?
			bool gl_ARB_texture_rg;

			/**
			 * Is GL_ARB_color_buffer_float supported?
			 *
			 * This affects things other than floating-point textures (samplers or render-targets) but
			 * we put it with the texture parameters since it's most directly related to floating-point
			 * colour buffers (eg, floating-point textures attached to a framebuffer object).
			 *
			 * Unfortunately for Mac OSX 10.5 (Leopard) this is not supported.
			 * It is supported in Snow Leopard (10.6), and above, however.
			 */
			bool gl_ARB_color_buffer_float;

			/**
			 * Is true if filtering of floating-point textures is supported and alpha-blending
			 * to floating-point render targets is supported.
			 *
			 * NOTE: There is no OpenGL extension to query for this and no easy way to detect it.
			 * The presence of the GL_ARB_texture_float extension does not guarantee it
			 * (notably on OpenGL 2.0 hardware). According to...
			 *    http://www.opengl.org/wiki/Floating_point_and_mipmapping_and_filtering
			 * ...all OpenGL 3.0 hardware supports this. Instead of testing for GLEW_VERSION_3_0
			 * we test for GL_EXT_texture_array (which was introduced in OpenGL 3.0) - this is
			 * done because OpenGL 3.0 is not officially supported on MacOS Snow Leopard in that
			 * it supports OpenGL 3.0 extensions but not the specific OpenGL 3.0 functions (according to
			 * http://www.cultofmac.com/26065/snow-leopard-10-6-3-update-significantly-improves-opengl-3-0-support/).
			 */
			bool gl_supports_floating_point_filtering_and_blending;
		};

		//! Parameters related to buffer objects.
		struct Buffer
		{
			Buffer();

			//! Is GL_ARB_vertex_buffer_object supported?
			bool gl_ARB_vertex_buffer_object;

			//! Is GL_ARB_vertex_array_object supported?
			bool gl_ARB_vertex_array_object;

			//! Is GL_ARB_pixel_buffer_object supported?
			bool gl_ARB_pixel_buffer_object;

			//! Is GL_ARB_map_buffer_range supported?
			bool gl_ARB_map_buffer_range;

			//! Is GL_APPLE_flush_buffer_range supported?
			bool gl_APPLE_flush_buffer_range;
		};

		Viewport viewport;
		Framebuffer framebuffer;
		Shader shader;
		Texture texture;
		Buffer buffer;

	private:

		GLCapabilities()
		{  }

		void
		initialise();

		void
		initialise_viewport();

		void
		initialise_framebuffer();

		void
		initialise_shader();

		void
		initialise_texture();

		void
		initialise_buffer();

		/**
		 * Only GLContext can create a GLCapabilities - this is to prevent clients
		 * from creating and initialising their own GLCapabilities - it must be initialised
		 * from a GLContext once the GLEW library has been initialised.
		 */
		friend class GLContext;

	};
}

#endif // GPLATES_OPENGL_GLCAPABILITIES_H
