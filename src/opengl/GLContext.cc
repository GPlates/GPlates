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

#include <cstddef> // For std::size_t
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLContext.h"

#include "GLBufferImpl.h"
#include "GLRenderer.h"
#include "GLStateStore.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


bool GPlatesOpenGL::GLContext::s_initialised_GLEW = false;
boost::optional<GPlatesOpenGL::GLContext::Parameters> GPlatesOpenGL::GLContext::s_parameters;

// Set the GL_TEXTURE0 constant.
const GLenum GPlatesOpenGL::GLContext::Parameters::Texture::gl_texture0 = GL_TEXTURE0;

void
GPlatesOpenGL::GLContext::initialise()
{
	// Currently we only initialise once for the whole application instead of
	// once for each rendering context.
	// This is because the GLEW library would need to be compiled with the GLEW_MX
	// flag (see http://glew.sourceforge.net/advanced.html under multiple rendering contexts)
	// and this does not appear to be supported in all package managers (eg, linux and MacOS X).
	// There's not much information on whether we need one if we share contexts in Qt but
	// this is the assumption here.
	if (!s_initialised_GLEW)
	{
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			// glewInit failed.
			//
			// We'll assume all calls to test whether an extension is available
			// (such as "if (GLEW_ARB_multitexture) ..." will fail since they just
			// test boolean variables which are assumed to be initialised by GLEW to zero.
			// This just means we will be forced to fall back to OpenGL version 1.1.
			qWarning() << "Error: " << reinterpret_cast<const char *>(glewGetErrorString(err));
		}
		//qDebug() << "Status: Using GLEW " << reinterpret_cast<const char *>(glewGetString(GLEW_VERSION));

		s_initialised_GLEW = true;

		// Disable specific OpenGL extensions to either:
		//  1) Test different code paths, or
		//  2) An easy way to disable an already coded path that turns out to be faster or better
		//     without the extension (an example is vertex array objects).
		disable_opengl_extensions();

		// Get the API parameters from the current OpenGL implementation.
		initialise_parameters();
	}
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLRenderer>::non_null_ptr_type
GPlatesOpenGL::GLContext::create_renderer()
{
	return GLRenderer::create(
			get_non_null_pointer(this),
			get_shared_state()->get_state_store());
}


void
GPlatesOpenGL::GLContext::initialise_parameters()
{
	// Start off with default parameters.
	s_parameters = Parameters();

	qDebug() << "On this system GPlates supports the following OpenGL extensions...";

	initialise_viewport_parameters(s_parameters->viewport);
	initialise_framebuffer_parameters(s_parameters->framebuffer);
	initialise_shader_parameters(s_parameters->shader);
	initialise_texture_parameters(s_parameters->texture);
	initialise_buffer_parameters(s_parameters->buffer);

	qDebug() << "...end of OpenGL extension list.";
}


void
GPlatesOpenGL::GLContext::initialise_viewport_parameters(
		Parameters::Viewport &viewport_parameters)
{
#ifdef GL_ARB_viewport_array // In case old 'glew.h' header (GL_ARB_viewport_array is fairly recent)
	if (GLEW_ARB_viewport_array)
	{
		// Get the maximum number of viewports.
		GLint max_viewports;
		glGetIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		viewport_parameters.gl_max_viewports = max_viewports;

		qDebug() << "  GL_ARB_viewport_array";
	}
#endif

	// Query the maximum viewport dimensions supported.
	GLint max_viewport_dimensions[2];
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &max_viewport_dimensions[0]);
	viewport_parameters.gl_max_viewport_width = max_viewport_dimensions[0];
	viewport_parameters.gl_max_viewport_height = max_viewport_dimensions[1];
}


void
GPlatesOpenGL::GLContext::initialise_framebuffer_parameters(
		Parameters::Framebuffer &framebuffer_parameters)
{
	if (GLEW_EXT_framebuffer_object)
	{
		framebuffer_parameters.gl_EXT_framebuffer_object = true;

		// Get the maximum number of color attachments.
		GLint max_color_attachments;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &max_color_attachments);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		framebuffer_parameters.gl_max_color_attachments = max_color_attachments;

		qDebug() << "  GL_EXT_framebuffer_object";
	}
}


void
GPlatesOpenGL::GLContext::initialise_shader_parameters(
		Parameters::Shader &shader_parameters)
{
	if (GLEW_ARB_shader_objects)
	{
		shader_parameters.gl_ARB_shader_objects = true;

		qDebug() << "  GL_ARB_shader_objects";
	}

	if (GLEW_ARB_vertex_shader)
	{
		shader_parameters.gl_ARB_vertex_shader = true;

		// Get the maximum supported number of generic vertex attributes.
		GLint max_vertex_attribs;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS_ARB, &max_vertex_attribs);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		shader_parameters.gl_max_vertex_attribs = max_vertex_attribs;

		qDebug() << "  GL_ARB_vertex_shader";
	}

	if (GLEW_ARB_fragment_shader)
	{
		shader_parameters.gl_ARB_fragment_shader = true;

		qDebug() << "  GL_ARB_fragment_shader";
	}

#ifdef GL_ARB_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	if (GLEW_ARB_geometry_shader4)
	{
		shader_parameters.gl_ARB_geometry_shader4 = true;

		qDebug() << "  GL_ARB_geometry_shader4";
	}
#endif
}


void
GPlatesOpenGL::GLContext::initialise_texture_parameters(
		Parameters::Texture &texture_parameters)
{
	// Get the maximum texture size (dimension).
	GLint max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
	texture_parameters.gl_max_texture_size = max_texture_size;

	// Get the maximum number of texture units supported.
	if (GLEW_ARB_multitexture)
	{
		GLint max_texture_units;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &max_texture_units);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture_parameters.gl_max_texture_units = max_texture_units;

		qDebug() << "  GL_ARB_multitexture";
	}

	// Get the maximum texture anisotropy supported.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &texture_parameters.gl_texture_max_anisotropy);

		qDebug() << "  GL_EXT_texture_filter_anisotropic";
	}

	// Are 3D textures supported?
	if (GLEW_EXT_texture3D)
	{
		texture_parameters.gl_EXT_texture3D = true;

		qDebug() << "  GL_EXT_texture3D";
	}

#ifdef GL_EXT_texture_array // In case old 'glew.h' header
	if (GLEW_EXT_texture_array)
	{
		texture_parameters.gl_EXT_texture_array = true;

		// Get the maximum number of texture array layers.
		GLint max_texture_array_layers;
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &max_texture_array_layers);
		// Store as unsigned since it avoids unsigned/signed comparison compiler warnings.
		texture_parameters.gl_max_texture_array_layers = max_texture_array_layers;

		qDebug() << "  GL_EXT_texture_array";
	}
#endif

	// Are texture buffer objects supported?
#ifdef GL_EXT_texture_buffer_object
	if (GLEW_EXT_texture_buffer_object)
	{
		texture_parameters.gl_EXT_texture_buffer_object = true;

		qDebug() << "  GL_EXT_texture_buffer_object";
	}
#endif
}


void
GPlatesOpenGL::GLContext::initialise_buffer_parameters(
		Parameters::Buffer &buffer_parameters)
{
	if (GLEW_ARB_vertex_buffer_object)
	{
		buffer_parameters.gl_ARB_vertex_buffer_object = true;

		qDebug() << "  GL_ARB_vertex_buffer_object";
	}

#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	if (GLEW_ARB_vertex_array_object)
	{
		buffer_parameters.gl_ARB_vertex_array_object = true;

		qDebug() << "  GL_ARB_vertex_array_object";
	}
#endif
}


const GPlatesOpenGL::GLContext::Parameters &
GPlatesOpenGL::GLContext::get_parameters()
{
	// GLEW must have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			s_initialised_GLEW && s_parameters,
			GPLATES_ASSERTION_SOURCE);

	return s_parameters.get();
}


void
GPlatesOpenGL::GLContext::begin_render()
{
	deallocate_queued_object_resources();
}


void
GPlatesOpenGL::GLContext::end_render()
{
	deallocate_queued_object_resources();
}


void
GPlatesOpenGL::GLContext::deallocate_queued_object_resources()
{
	get_non_shared_state()->get_frame_buffer_object_resource_manager()->deallocate_queued_resources();
	get_non_shared_state()->get_vertex_array_object_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_texture_object_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_buffer_object_resource_manager()->deallocate_queued_resources();
}


void
GPlatesOpenGL::GLContext::disable_opengl_extensions()
{
#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	// It turns out that using vertex array objects is slower than just setting the
	// vertex attribute arrays (and vertex element buffer) at each vertex array bind.
	__GLEW_ARB_vertex_array_object = 0;
#endif

	//
	// For testing different code paths.
	//
	//__GLEW_ARB_vertex_buffer_object = 0;
	//__GLEW_EXT_framebuffer_object = 0;
	//__GLEW_ARB_vertex_shader = 0;
	//__GLEW_ARB_multitexture = 0;
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_texture_object_resource_manager(GLTexture::resource_manager_type::create()),
	d_buffer_object_resource_manager(GLBufferObject::resource_manager_type::create()),
	d_vertex_shader_object_resource_manager(
			GLShaderObject::resource_manager_type::create(
					GLShaderObject::allocator_type(GL_VERTEX_SHADER_ARB))),
	d_fragment_shader_object_resource_manager(
			GLShaderObject::resource_manager_type::create(
					GLShaderObject::allocator_type(GL_FRAGMENT_SHADER_ARB))),
	d_program_object_resource_manager(GLProgramObject::resource_manager_type::create())
{
#ifdef GL_ARB_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	d_geometry_shader_object_resource_manager =
			GLShaderObject::resource_manager_type::create(
					GLShaderObject::allocator_type(GL_GEOMETRY_SHADER_ARB));
#endif
}


const boost::shared_ptr<GPlatesOpenGL::GLShaderObject::resource_manager_type> &
GPlatesOpenGL::GLContext::SharedState::get_shader_object_resource_manager(
		GLenum shader_type) const
{
	switch (shader_type)
	{
	case GL_VERTEX_SHADER_ARB:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_shader),
				GPLATES_ASSERTION_SOURCE);
		return d_vertex_shader_object_resource_manager;

	case GL_FRAGMENT_SHADER_ARB:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_fragment_shader),
				GPLATES_ASSERTION_SOURCE);
		return d_fragment_shader_object_resource_manager;

#ifdef GL_ARB_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	case GL_GEOMETRY_SHADER_ARB:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_geometry_shader4),
				GPLATES_ASSERTION_SOURCE);
		return d_geometry_shader_object_resource_manager.get();
#endif

	default:
		break;
	}

	// Shouldn't get here.
	throw GPlatesGlobal::PreconditionViolationError(GPLATES_ASSERTION_SOURCE);
}


const boost::shared_ptr<GPlatesOpenGL::GLProgramObject::resource_manager_type> &
GPlatesOpenGL::GLContext::SharedState::get_program_object_resource_manager() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	return d_program_object_resource_manager;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_texture(
		GLRenderer &renderer,
		GLenum target,
		GLint internalformat,
		GLsizei width,
		boost::optional<GLsizei> height_opt,
		boost::optional<GLsizei> depth_opt,
		GLint border,
		bool mipmapped)
{
	// Lookup the correct texture cache (matching the specified client parameters).
	const texture_key_type texture_key(target, internalformat, width, height_opt, depth_opt, border, mipmapped);

	const texture_cache_type::shared_ptr_type texture_cache = get_texture_cache(texture_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLTexture::shared_ptr_type> texture_object_opt = texture_cache->allocate_object();
	if (texture_object_opt)
	{
		return texture_object_opt.get();
	}

	// Create a new object and add it to the cache.
	const GLTexture::shared_ptr_type texture_object = texture_cache->allocate_object(
			GLTexture::create_as_auto_ptr(renderer));

	//
	// Initialise the newly created texture object.
	//
	
	if (depth_opt)
	{
		// It's a 3D texture.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				height_opt,
				GPLATES_ASSERTION_SOURCE);

		GLTextureUtils::initialise_texture_object_3D(
				renderer, texture_object, target, internalformat, width, height_opt.get(), depth_opt.get(), border, mipmapped);

		return texture_object;
	}

	if (height_opt)
	{
		// It's a 2D texture.
		GLTextureUtils::initialise_texture_object_2D(
				renderer, texture_object, target, internalformat, width, height_opt.get(), border, mipmapped);

		return texture_object;
	}

	// It's a 1D texture.
	GLTextureUtils::initialise_texture_object_1D(
			renderer, texture_object, target, internalformat, width, border, mipmapped);

	return texture_object;
}


GPlatesOpenGL::GLVertexArray::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_vertex_array(
		GLRenderer &renderer)
{
	// Attempt to acquire a recycled object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array_opt =
			d_vertex_array_cache->allocate_object();
	if (!vertex_array_opt)
	{
		// Create a new vertex array and add it to the cache.
		vertex_array_opt = d_vertex_array_cache->allocate_object(
				GLVertexArray::create_as_auto_ptr(renderer));
	}
	const GLVertexArray::shared_ptr_type &vertex_array = vertex_array_opt.get();

	// First clear the vertex array before returning to the client.
	vertex_array->clear(renderer);

	return vertex_array;
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
GPlatesOpenGL::GLContext::SharedState::get_full_screen_2D_textured_quad(
		GLRenderer &renderer)
{
	// Create the sole unbound vertex array compile state if it hasn't already been created.
	if (!d_full_screen_2D_textured_quad)
	{
		d_full_screen_2D_textured_quad = GLUtils::create_full_screen_2D_textured_quad(renderer);
	}

	return d_full_screen_2D_textured_quad.get();
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
GPlatesOpenGL::GLContext::SharedState::get_unbound_vertex_array_compiled_draw_state(
		GLRenderer &renderer)
{
	// Create the sole unbound vertex array compile state if it hasn't already been created.
	if (!d_unbound_vertex_array_compiled_draw_state)
	{
		GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

		// Create a 'GLBufferImpl' (even though GLBufferObject might be supported).
		// This represents client memory arrays which is the default state in OpenGL.
		// Using buffer objects (ie, GLBufferObject) is non-default.
		// We'll use the GLBufferImpl with an empty array (un-allocated) to set the default.
		// It wouldn't matter if it was allocated (and subsequently de-allocated at the end of
		// this method) because it will never get dereferenced by OpenGL (if it does then it's
		// a bug in our use of OpenGL somewhere).
		GLBufferImpl::shared_ptr_to_const_type default_client_memory_buffer = GLBufferImpl::create(renderer);

		/////////////////////////
		//
		// Non-generic attributes
		//
		/////////////////////////

		//
		// Disable all vertex attribute client state.
		//
		renderer.gl_enable_client_state(GL_VERTEX_ARRAY, false);
		renderer.gl_enable_client_state(GL_COLOR_ARRAY, false);
		renderer.gl_enable_client_state(GL_NORMAL_ARRAY, false);
		// Iterate over the enable texture coordinate client state flags.
		const unsigned int MAX_TEXTURE_UNITS = GLContext::get_parameters().texture.gl_max_texture_units;
		for (unsigned int texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_UNITS; ++texture_coord_index)
		{
			renderer.gl_enable_client_texture_state(GL_TEXTURE0 + texture_coord_index, false);
		}

		//
		// Unbind all vertex attribute arrays.
		//
		// This releases any GL_*_ARRAY_BUFFER_BINDING binding to vertex buffer objects.
		//
		// NOTE: By specifying a client memory pointer (which is also NULL) we effectively unbind
		// a vertex attribute array from a vertex buffer.
		//
		renderer.gl_vertex_pointer(4, GL_FLOAT, 0, 0, default_client_memory_buffer);
		renderer.gl_color_pointer(4, GL_FLOAT, 0, 0, default_client_memory_buffer);
		renderer.gl_normal_pointer(GL_FLOAT, 0, 0, default_client_memory_buffer);
		// Iterate over the texture coordinate arrays.
		for (unsigned int texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_UNITS; ++texture_coord_index)
		{
			renderer.gl_tex_coord_pointer(4, GL_FLOAT, 0, 0, default_client_memory_buffer, GL_TEXTURE0 + texture_coord_index);
		}

		/////////////////////////////////////////////////////////////////////////////////////
		//
		// Generic attributes
		//
		// NOTE: nVidia hardware aliases the built-in attributes with the generic attributes -
		// see 'GLProgramObject::gl_bind_attrib_location' for more details - so, eg, setting generic
		// attribute index 0 affects the non-generic 'GL_VERTEX_ARRAY' attribute also - which
		// can lead to one overwriting the other (we have them assigned to different GLStateSet slots,
		// in GLState, but nVidia hardware sees them as the same). However the filtering of redundant
		// state-sets in GLState is quite robust at filtering all state changes, so as long as only one
		// of the two slots (that map to each other on nVidia hardware) differs from the default OpenGL
		// state then no overwriting should occur at the OpenGL level (GLState wraps OpenGL).
		// In other words, eg, use gl_vertex_pointer or gl_vertex_attrib_pointer(0,...) but not both.
		//
		/////////////////////////////////////////////////////////////////////////////////////

		//
		// Disable all *generic* vertex attribute arrays.
		//
		const GLuint MAX_VERTEX_ATTRIBS = GLContext::get_parameters().shader.gl_max_vertex_attribs;
		// Iterate over the supported number of generic vertex attribute arrays.
		for (GLuint attribute_index = 0; attribute_index < MAX_VERTEX_ATTRIBS; ++attribute_index)
		{
			renderer.gl_enable_vertex_attrib_array(attribute_index, false);
		}

		//
		// Unbind all *generic* vertex attribute arrays.
		//
		// This releases any GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING binding to vertex buffer objects.
		//
		// NOTE: By specifying a client memory pointer (which is also NULL) we effectively unbind
		// a vertex attribute array from a vertex buffer.
		//
		// Iterate over the supported number of generic vertex attribute arrays.
		for (GLuint attribute_index = 0; attribute_index < MAX_VERTEX_ATTRIBS; ++attribute_index)
		{
			renderer.gl_vertex_attrib_pointer(attribute_index, 4, GL_FLOAT, GL_FALSE, 0, 0, default_client_memory_buffer);
		}


		//
		// Note that we do *not* specifically unbind vertex buffer.
		// A vertex buffer binding is recorded for each vertex attribute pointer above, both
		// generic and non-generic, but a vertex array does not record which vertex buffer is
		// currently bound - see http://www.opengl.org/wiki/Vertex_Array_Object for more details.
		//
		//renderer.gl_unbind_vertex_buffer_object();

		//
		// No bound vertex element buffer.
		//
		// The vertex element buffer, unlike the vertex buffer, *is* recorded in the vertex array.
		//
		renderer.gl_unbind_vertex_element_buffer_object();

		// Cache the sole unbound vertex array compiled state for reuse.
		d_unbound_vertex_array_compiled_draw_state = compile_draw_state_scope.get_compiled_draw_state();
	}

	return d_unbound_vertex_array_compiled_draw_state.get();
}


GPlatesOpenGL::GLContext::SharedState::texture_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_texture_cache(
		const texture_key_type &texture_key)
{
	// Attempt to insert the texture key into the texture cache map.
	const std::pair<texture_cache_map_type::iterator, bool> insert_result =
			d_texture_cache_map.insert(
					texture_cache_map_type::value_type(
							texture_key,
							// Dummy (NULL) texture cache...
							texture_cache_type::shared_ptr_type()));

	texture_cache_map_type::iterator texture_cache_map_iter = insert_result.first;

	// If the texture key was inserted into the map then create the corresponding texture cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		texture_cache_map_iter->second = texture_cache_type::create();
	}

	return texture_cache_map_iter->second;
}


GPlatesOpenGL::GLStateStore::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_state_store()
{
	if (!d_state_store)
	{
		d_state_store = GLStateStore::create(GLStateSetStore::create(), GLStateSetKeys::create());
	}

	return d_state_store.get();
}


GPlatesOpenGL::GLContext::NonSharedState::NonSharedState() :
	d_frame_buffer_object_resource_manager(GLFrameBufferObject::resource_manager_type::create()),
	// Start off with an initial cache size of 1 - it'll grow as needed...
	d_frame_buffer_object_cache(GPlatesUtils::ObjectCache<GLFrameBufferObject>::create()),
	d_vertex_array_object_resource_manager(GLVertexArrayObject::resource_manager_type::create())
{
}


GPlatesOpenGL::GLFrameBufferObject::shared_ptr_type
GPlatesOpenGL::GLContext::NonSharedState::acquire_frame_buffer_object()
{
	// Attempt to acquire a recycled object.
	boost::optional<GLFrameBufferObject::shared_ptr_type> frame_buffer_object_opt =
			d_frame_buffer_object_cache->allocate_object();
	if (!frame_buffer_object_opt)
	{
		// Create a new object and add it to the cache.
		frame_buffer_object_opt = d_frame_buffer_object_cache->allocate_object(
				GLFrameBufferObject::create_as_auto_ptr(d_frame_buffer_object_resource_manager));
	}

	return frame_buffer_object_opt.get();
}


GPlatesOpenGL::GLContext::Parameters::Viewport::Viewport() :
	gl_max_viewports(1),
	gl_max_viewport_width(0),
	gl_max_viewport_height(0)
{
}


GPlatesOpenGL::GLContext::Parameters::Framebuffer::Framebuffer() :
	gl_EXT_framebuffer_object(false),
	gl_max_color_attachments(0)
{
}


GPlatesOpenGL::GLContext::Parameters::Shader::Shader() :
	gl_ARB_shader_objects(false),
	gl_ARB_vertex_shader(false),
	gl_ARB_fragment_shader(false),
	gl_ARB_geometry_shader4(false),
	gl_max_vertex_attribs(0)
{
}


GPlatesOpenGL::GLContext::Parameters::Texture::Texture() :
	gl_max_texture_size(gl_min_texture_size),
	gl_max_texture_units(1),
	gl_texture_max_anisotropy(1.0f),
	gl_EXT_texture3D(false),
	gl_EXT_texture_array(false),
	gl_max_texture_array_layers(1),
	gl_EXT_texture_buffer_object(false)
{
}


GPlatesOpenGL::GLContext::Parameters::Buffer::Buffer() :
	gl_ARB_vertex_buffer_object(false),
	gl_ARB_vertex_array_object(false)
{
}
