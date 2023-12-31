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

#include <utility>
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
#include "OpenGLException.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


bool GPlatesOpenGL::GLContext::s_initialised_GLEW = false;
GPlatesOpenGL::GLCapabilities GPlatesOpenGL::GLContext::s_capabilities;


QGLFormat
GPlatesOpenGL::GLContext::get_qgl_format_to_create_context_with()
{
	// We turn *off* multisampling because lines actually look better without it...
	// We need a stencil buffer for filling polygons.
	// We need an alpha channel in case falling back to main frame buffer for render textures.
	QGLFormat format(/*QGL::SampleBuffers |*/ QGL::StencilBuffer | QGL::AlphaChannel);

	// We use features deprecated in OpenGL 3 so use compatibility profile and allowed deprecated functions.
	format.setProfile(QGLFormat::CompatibilityProfile);
	format.setOption(QGL::DeprecatedFunctions);

	const QGLFormat::OpenGLVersionFlags opengl_version_flags = QGLFormat::openGLVersionFlags();

	// We use OpenGL extensions in GPlates and hence don't rely on a particular OpenGL core version.
	// So we just set the version to OpenGL 1.1 which is supported by everything and is the
	// only version supported by the Microsoft software renderer (fallback from hardware).
	//
	// NOTE: If GL_EXT_framebuffer_object is not supported (but the newer GL_ARB_framebuffer_object is)
	// then a possible reason for this could be...
	//
	// From http://stackoverflow.com/questions/15017911/oes-ext-arb-framebuffer-object:
	//  Issues with GL_EXT_framebuffer_object a) GL_EXT_framebuffer_object might not be listed in
	//  GL 3.x contexts because FBO's are core. b) also, if have a GL 2.x context with newer hardware,
	//  possible that GL_EXT_framebuffer_object is not listed but GL_ARB_framebuffer_object is.
	//
	// ...so this is another reason to set the OpenGL version to 1.1 (instead of 2.x, or 3.x) below.
	// But it's possible (according to the above comment) that it may not be enough.
	// Note that we use GL_EXT_framebuffer_object only, and not GL_ARB_framebuffer_object, due to
	// widespread hardware support of the (less flexible) GL_EXT_framebuffer_object.
	//
	if (opengl_version_flags.testFlag(QGLFormat::OpenGL_Version_1_1))
	{
		format.setVersion(1, 1);
	}

	return format;
}


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
			// (such as "if (get_capabilities().gl_ARB_multitexture) ..." will fail since they just
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

		// Get the OpenGL capabilities and parameters from the current OpenGL implementation.
		s_capabilities.initialise();

		// Provide information about lack of framebuffer object support.
		if (!s_capabilities.framebuffer.gl_EXT_framebuffer_object)
		{
			qDebug() << "Falling back to main frame buffer for render targets.";
		}
	}

	// A lot of main frame buffer and render-target rendering uses an alpha channel so emit
	// a warning if the frame buffer doesn't have an alpha channel.
	if (!get_qgl_format().alpha())
	{
		qWarning("Could not get alpha channel on main frame buffer.");

		// If there's no framebuffer object support then the main framebuffer will be used
		// to emulate render targets and lack of an alpha channel will affects the results.
		if (!get_capabilities().framebuffer.gl_EXT_framebuffer_object)
		{
			qDebug("Render-target results will be suboptimal.");
		}
	}

	// A lot of main frame buffer and render-target rendering uses a stencil buffer so emit
	// a warning if the frame buffer doesn't have a stencil buffer.
	if (!get_qgl_format().stencil())
	{
		qWarning("Could not get a stencil buffer on the main frame buffer.");
	}
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLRenderer>::non_null_ptr_type
GPlatesOpenGL::GLContext::create_renderer()
{
	return GLRenderer::create(
			get_non_null_pointer(this),
			get_shared_state()->get_state_store(get_capabilities()));
}


const GPlatesOpenGL::GLCapabilities &
GPlatesOpenGL::GLContext::get_capabilities() const
{
	// GLEW must have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			s_initialised_GLEW,
			GPLATES_ASSERTION_SOURCE);

	return s_capabilities;
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
	//__GLEW_ARB_vertex_buffer_object = 0; __GLEW_ARB_pixel_buffer_object = 0;
	//__GLEW_ARB_pixel_buffer_object = 0;
	//__GLEW_EXT_framebuffer_object = 0;
	//__GLEW_EXT_packed_depth_stencil = 0;
	//__GLEW_ARB_vertex_shader = 0;
	//__GLEW_ARB_multitexture = 0;
	//__GLEW_ARB_texture_non_power_of_two = 0;
	//__GLEW_ARB_texture_float = 0;
	//__GLEW_ARB_shader_objects = 0;
	//__GLEW_ARB_fragment_shader = 0;
	//__GLEW_EXT_texture_edge_clamp = 0; __GLEW_SGIS_texture_edge_clamp = 0;
	//__GLEW_ARB_map_buffer_range = 0; __GLEW_APPLE_flush_buffer_range = 0;
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
#ifdef GL_EXT_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	d_geometry_shader_object_resource_manager =
			GLShaderObject::resource_manager_type::create(
					GLShaderObject::allocator_type(GL_GEOMETRY_SHADER_EXT));
#endif
}


const boost::shared_ptr<GPlatesOpenGL::GLShaderObject::resource_manager_type> &
GPlatesOpenGL::GLContext::SharedState::get_shader_object_resource_manager(
		GLRenderer &renderer,
		GLenum shader_type) const
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	switch (shader_type)
	{
	case GL_VERTEX_SHADER_ARB:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				capabilities.shader.gl_ARB_vertex_shader,
				GPLATES_ASSERTION_SOURCE);
		return d_vertex_shader_object_resource_manager;

	case GL_FRAGMENT_SHADER_ARB:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				capabilities.shader.gl_ARB_fragment_shader,
				GPLATES_ASSERTION_SOURCE);
		return d_fragment_shader_object_resource_manager;

#ifdef GL_EXT_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	case GL_GEOMETRY_SHADER_EXT:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				capabilities.shader.gl_EXT_geometry_shader4,
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
GPlatesOpenGL::GLContext::SharedState::get_program_object_resource_manager(
		GLRenderer &renderer) const
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_ARB_shader_objects,
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
		const GLTexture::shared_ptr_type texture_object = texture_object_opt.get();

		// Make sure the previous client did not change the texture dimensions before recycling the texture.
		GPlatesGlobal::Assert<OpenGLException>(
				texture_object->get_width() == GLuint(width) &&
					texture_object->get_height() == boost::optional<GLuint>(height_opt) &&
					texture_object->get_depth() == boost::optional<GLuint>(depth_opt) &&
					texture_object->get_internal_format() == internalformat,
				GPLATES_ASSERTION_SOURCE,
				"GLContext::SharedState::acquire_texture: Dimensions/format of recycled texture were changed.");

		return texture_object;
	}

	// Create a new object and add it to the cache.
	const GLTexture::shared_ptr_type texture_object = texture_cache->allocate_object(
			GLTexture::create_as_unique_ptr(renderer));

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


GPlatesOpenGL::GLPixelBuffer::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_pixel_buffer(
		GLRenderer &renderer,
		unsigned int size,
		GLBuffer::usage_type usage)
{
	// Lookup the correct pixel buffer cache (matching the specified client parameters).
	const pixel_buffer_key_type pixel_buffer_key(size, usage);

	const pixel_buffer_cache_type::shared_ptr_type pixel_buffer_cache =
			get_pixel_buffer_cache(pixel_buffer_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLPixelBuffer::shared_ptr_type> pixel_buffer_opt = pixel_buffer_cache->allocate_object();
	if (pixel_buffer_opt)
	{
		const GLPixelBuffer::shared_ptr_type pixel_buffer = pixel_buffer_opt.get();

		// Make sure the previous client did not change the pixel buffer before recycling.
		GPlatesGlobal::Assert<OpenGLException>(
				pixel_buffer->get_buffer()->get_buffer_size() == size,
				GPLATES_ASSERTION_SOURCE,
				"GLContext::SharedState::acquire_pixel_buffer: Size of recycled pixel buffer was changed.");

		return pixel_buffer;
	}

	// Create a new buffer with the specified parameters.
	GLBuffer::shared_ptr_type buffer = GLBuffer::create(renderer, GLBuffer::BUFFER_TYPE_PIXEL);
	buffer->gl_buffer_data(
			renderer,
			// Could be 'TARGET_PIXEL_UNPACK_BUFFER' or 'TARGET_PIXEL_PACK_BUFFER'.
			// Doesn't really matter because only used internally as a temporary bind target...
			GLBuffer::TARGET_PIXEL_PACK_BUFFER,
			size,
			NULL/*Uninitialised memory*/,
			usage);

	// Create a new object and add it to the cache.
	const GLPixelBuffer::shared_ptr_type pixel_buffer = pixel_buffer_cache->allocate_object(
			GLPixelBuffer::create_as_unique_ptr(renderer, buffer));

	return pixel_buffer;
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
				GLVertexArray::create_as_unique_ptr(renderer));
	}
	const GLVertexArray::shared_ptr_type &vertex_array = vertex_array_opt.get();

	// First clear the vertex array before returning to the client.
	vertex_array->clear(renderer);

	return vertex_array;
}


GPlatesOpenGL::GLRenderBufferObject::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_render_buffer_object(
		GLRenderer &renderer,
		GLint internalformat,
		GLsizei width,
		GLsizei height)
{
	// Must support GL_EXT_framebuffer_object.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().framebuffer.gl_EXT_framebuffer_object,
			GPLATES_ASSERTION_SOURCE);

	// Lookup the correct render buffer object cache (matching the specified client parameters).
	const render_buffer_object_key_type render_buffer_object_key(internalformat, width, height);

	const render_buffer_object_cache_type::shared_ptr_type render_buffer_object_cache =
			get_render_buffer_object_cache(render_buffer_object_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLRenderBufferObject::shared_ptr_type> render_buffer_object_opt =
			render_buffer_object_cache->allocate_object();
	if (render_buffer_object_opt)
	{
		const GLRenderBufferObject::shared_ptr_type render_buffer_object = render_buffer_object_opt.get();

		// Make sure the previous client did not change the render buffer dimensions before
		// recycling the render buffer.
		GPlatesGlobal::Assert<OpenGLException>(
				render_buffer_object->get_dimensions() == std::pair<GLuint,GLuint>(width, height) &&
					render_buffer_object->get_internal_format() == internalformat,
				GPLATES_ASSERTION_SOURCE,
				"GLContext::SharedState::acquire_render_buffer_object: Dimensions/format of "
					"recycled render buffer were changed.");

		return render_buffer_object;
	}

	// Create a new object and add it to the cache.
	const GLRenderBufferObject::shared_ptr_type render_buffer_object = render_buffer_object_cache->allocate_object(
			GLRenderBufferObject::create_as_unique_ptr(renderer));

	// Initialise the newly created render buffer object.
	render_buffer_object->gl_render_buffer_storage(renderer, internalformat, width, height);

	return render_buffer_object;
}


boost::optional<GPlatesOpenGL::GLRenderTarget::shared_ptr_type>
GPlatesOpenGL::GLContext::SharedState::acquire_render_target(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer,
		bool include_stencil_buffer,
		unsigned int render_target_width,
		unsigned int render_target_height)
{
	// Render targets must be supported.
	if (!GLRenderTarget::is_supported(
		renderer,
		texture_internalformat,
		include_depth_buffer,
		include_stencil_buffer,
		render_target_width,
		render_target_height))
	{
		return boost::none;
	}

	// Lookup the correct render target cache (matching the specified client parameters).
	const render_target_key_type render_target_key(
			texture_internalformat,
			include_depth_buffer,
			include_stencil_buffer,
			render_target_width,
			render_target_height);

	const render_target_cache_type::shared_ptr_type render_target_cache =
			get_render_target_cache(render_target_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLRenderTarget::shared_ptr_type> render_target_opt =
			render_target_cache->allocate_object();
	if (render_target_opt)
	{
		return render_target_opt.get();
	}

	// Create a new object and add it to the cache.
	const GLRenderTarget::shared_ptr_type render_target =
			render_target_cache->allocate_object(
					GLRenderTarget::create_as_unique_ptr(
							renderer,
							texture_internalformat,
							include_depth_buffer,
							include_stencil_buffer,
							render_target_width,
							render_target_height));

	return render_target;
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
		const unsigned int MAX_TEXTURE_COORDS = renderer.get_capabilities().texture.gl_max_texture_coords;
		for (unsigned int texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_COORDS; ++texture_coord_index)
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
		for (unsigned int texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_COORDS; ++texture_coord_index)
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
		const GLuint MAX_VERTEX_ATTRIBS = renderer.get_capabilities().shader.gl_max_vertex_attribs;
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


GPlatesOpenGL::GLContext::SharedState::pixel_buffer_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_pixel_buffer_cache(
		const pixel_buffer_key_type &pixel_buffer_key)
{
	// Attempt to insert the pixel buffer key into the pixel buffer cache map.
	const std::pair<pixel_buffer_cache_map_type::iterator, bool> insert_result =
			d_pixel_buffer_cache_map.insert(
					pixel_buffer_cache_map_type::value_type(
							pixel_buffer_key,
							// Dummy (NULL) pixel buffer cache...
							pixel_buffer_cache_type::shared_ptr_type()));

	pixel_buffer_cache_map_type::iterator pixel_buffer_cache_map_iter = insert_result.first;

	// If the pixel buffer key was inserted into the map then create the corresponding pixel buffer cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		pixel_buffer_cache_map_iter->second = pixel_buffer_cache_type::create();
	}

	return pixel_buffer_cache_map_iter->second;
}


GPlatesOpenGL::GLContext::SharedState::render_buffer_object_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_render_buffer_object_cache(
		const render_buffer_object_key_type &render_buffer_object_key)
{
	// Attempt to insert the pixel buffer key into the pixel buffer cache map.
	const std::pair<render_buffer_object_cache_map_type::iterator, bool> insert_result =
			d_render_buffer_object_cache_map.insert(
					render_buffer_object_cache_map_type::value_type(
							render_buffer_object_key,
							// Dummy (NULL) pixel buffer cache...
							render_buffer_object_cache_type::shared_ptr_type()));

	render_buffer_object_cache_map_type::iterator render_buffer_object_cache_map_iter = insert_result.first;

	// If the pixel buffer key was inserted into the map then create the corresponding pixel buffer cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		render_buffer_object_cache_map_iter->second = render_buffer_object_cache_type::create();
	}

	return render_buffer_object_cache_map_iter->second;
}


GPlatesOpenGL::GLContext::SharedState::render_target_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_render_target_cache(
		const render_target_key_type &render_target_key)
{
	// Attempt to insert the render target key into the render target cache map.
	const std::pair<render_target_cache_map_type::iterator, bool> insert_result =
			d_render_target_cache_map.insert(
					render_target_cache_map_type::value_type(
							render_target_key,
							// Dummy (NULL) render target cache...
							render_target_cache_type::shared_ptr_type()));

	render_target_cache_map_type::iterator render_target_cache_map_iter = insert_result.first;

	// If the render target key was inserted into the map then
	// create the corresponding render target cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		render_target_cache_map_iter->second = render_target_cache_type::create();
	}

	return render_target_cache_map_iter->second;
}


GPlatesOpenGL::GLStateStore::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_state_store(
		const GLCapabilities &capabilities)
{
	if (!d_state_store)
	{
		d_state_store = GLStateStore::create(
				capabilities,
				GLStateSetStore::create(),
				GLStateSetKeys::create(capabilities));
	}

	return d_state_store.get();
}


GPlatesOpenGL::GLContext::NonSharedState::NonSharedState() :
	d_frame_buffer_object_resource_manager(GLFrameBufferObject::resource_manager_type::create()),
	// Start off with an initial cache size of 1 - it'll grow as needed...
	d_render_buffer_object_resource_manager(GLRenderBufferObject::resource_manager_type::create()),
	d_vertex_array_object_resource_manager(GLVertexArrayObject::resource_manager_type::create())
{
}


GPlatesOpenGL::GLFrameBufferObject::shared_ptr_type
GPlatesOpenGL::GLContext::NonSharedState::acquire_frame_buffer_object(
		GLRenderer &renderer,
		const GLFrameBufferObject::Classification &classification)
{
	// Must support GL_EXT_framebuffer_object.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().framebuffer.gl_EXT_framebuffer_object,
			GPLATES_ASSERTION_SOURCE);

	// Lookup the correct framebuffer object cache (matching the specified classification).
	const frame_buffer_object_key_type frame_buffer_object_key = classification.get_tuple();

	const frame_buffer_object_cache_type::shared_ptr_type frame_buffer_object_cache =
			get_frame_buffer_object_cache(frame_buffer_object_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLFrameBufferObject::shared_ptr_type> frame_buffer_object_opt =
			frame_buffer_object_cache->allocate_object();
	if (frame_buffer_object_opt)
	{
		const GLFrameBufferObject::shared_ptr_type frame_buffer_object = frame_buffer_object_opt.get();

		// First clear the framebuffer attachments before returning to the client.
		frame_buffer_object->gl_detach_all(renderer);

		// Also reset the glDrawBuffer(s)/glReadBuffer state to the default state for
		// a non-default, application-created framebuffer object.
		frame_buffer_object->gl_draw_buffers(renderer);
		frame_buffer_object->gl_read_buffer(renderer);

		return frame_buffer_object;
	}

	// Create a new object and add it to the cache.
	const GLFrameBufferObject::shared_ptr_type frame_buffer_object =
			frame_buffer_object_cache->allocate_object(
					GLFrameBufferObject::create_as_unique_ptr(renderer));

	return frame_buffer_object;
}


bool
GPlatesOpenGL::GLContext::NonSharedState::check_framebuffer_object_completeness(
		GLRenderer &renderer,
		const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object,
		const GLFrameBufferObject::Classification &frame_buffer_object_classification) const
{
	// See if we've already cached the framebuffer completeness status for the specified
	// frame buffer object classification.
	frame_buffer_state_to_status_map_type::iterator framebuffer_status_iter =		
			d_frame_buffer_state_to_status_map.find(frame_buffer_object_classification.get_tuple());
	if (framebuffer_status_iter == d_frame_buffer_state_to_status_map.end())
	{
		const bool framebuffer_status = frame_buffer_object->gl_check_frame_buffer_status(renderer);

		d_frame_buffer_state_to_status_map[frame_buffer_object_classification.get_tuple()] = framebuffer_status;

		return framebuffer_status;
	}

	return framebuffer_status_iter->second;
}


boost::optional<GPlatesOpenGL::GLScreenRenderTarget::shared_ptr_type>
GPlatesOpenGL::GLContext::NonSharedState::acquire_screen_render_target(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer,
		bool include_stencil_buffer)
{
	// Screen render targets must be supported.
	if (!GLScreenRenderTarget::is_supported(
			renderer,
			texture_internalformat,
			include_depth_buffer,
			include_stencil_buffer))
	{
		return boost::none;
	}

	// Lookup the correct screen render target cache (matching the specified client parameters).
	const screen_render_target_key_type screen_render_target_key(
			texture_internalformat,
			include_depth_buffer,
			include_stencil_buffer);

	const screen_render_target_cache_type::shared_ptr_type screen_render_target_cache =
			get_screen_render_target_cache(screen_render_target_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLScreenRenderTarget::shared_ptr_type> screen_render_target_opt =
			screen_render_target_cache->allocate_object();
	if (screen_render_target_opt)
	{
		return screen_render_target_opt.get();
	}

	// Create a new object and add it to the cache.
	const GLScreenRenderTarget::shared_ptr_type screen_render_target =
			screen_render_target_cache->allocate_object(
					GLScreenRenderTarget::create_as_unique_ptr(
							renderer,
							texture_internalformat,
							include_depth_buffer,
							include_stencil_buffer));

	return screen_render_target;
}


GPlatesOpenGL::GLContext::NonSharedState::screen_render_target_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::NonSharedState::get_screen_render_target_cache(
		const screen_render_target_key_type &screen_render_target_key)
{
	// Attempt to insert the screen render target key into the screen render target cache map.
	const std::pair<screen_render_target_cache_map_type::iterator, bool> insert_result =
			d_screen_render_target_cache_map.insert(
					screen_render_target_cache_map_type::value_type(
							screen_render_target_key,
							// Dummy (NULL) screen render target cache...
							screen_render_target_cache_type::shared_ptr_type()));

	screen_render_target_cache_map_type::iterator screen_render_target_cache_map_iter = insert_result.first;

	// If the screen render target key was inserted into the map then
	// create the corresponding screen render target cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		screen_render_target_cache_map_iter->second = screen_render_target_cache_type::create();
	}

	return screen_render_target_cache_map_iter->second;
}


GPlatesOpenGL::GLContext::NonSharedState::frame_buffer_object_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::NonSharedState::get_frame_buffer_object_cache(
		const frame_buffer_object_key_type &frame_buffer_object_key)
{
	// Attempt to insert the frame buffer object key into the frame buffer object cache map.
	const std::pair<frame_buffer_object_cache_map_type::iterator, bool> insert_result =
			d_frame_buffer_object_cache_map.insert(
					frame_buffer_object_cache_map_type::value_type(
							frame_buffer_object_key,
							// Dummy (NULL) frame buffer object cache...
							frame_buffer_object_cache_type::shared_ptr_type()));

	frame_buffer_object_cache_map_type::iterator frame_buffer_object_cache_map_iter = insert_result.first;

	// If the frame buffer object key was inserted into the map then create the corresponding frame buffer object cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		frame_buffer_object_cache_map_iter->second = frame_buffer_object_cache_type::create();
	}

	return frame_buffer_object_cache_map_iter->second;
}
