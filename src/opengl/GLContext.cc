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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <string>
#include <utility>
#include <QDebug>
#include <QtGlobal>

#include "GLContext.h"

#include "GL.h"
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
	// We need an alpha channel in case falling back to main framebuffer for render textures.
	QGLFormat format(/*QGL::SampleBuffers |*/ QGL::DepthBuffer | QGL::StencilBuffer | QGL::AlphaChannel);

	// Use the OpenGL 3.3 core profile with forward compatibility (ie, deprecated functions removed).
	//
	// This is because we cannot easily access OpenGL 3 functionality (required for volume visualization)
	// on macOS and still retain functionality in OpenGL 1 and 2. MacOS which gives you a choice of either
	// 2.1 (which excludes volume visualization), or 3.2+ (their hardware supports 3.3/4.1) but only as a
	// core profile which means no backward compatibility (and hence no fixed-function pipeline and hence
	// no support for old graphics cards) and also only with forward compatibility (which removes support
	// for wide lines). Previously we supported right back to OpenGL version 1.1 and used OpengGL extensions to
	// provide optional support up to and including version 3 (eg, for volume visualization) when available.
	// However we started running into problems with things like 'texture2D()' not working in GLSL
	// (is called 'texture()' now). So it was easiest to move to OpenGL 3.3, and the time is right since
	// (as of 2020) most hardware in last decade should support 3.3 (and it's also supported on Ubuntu 16.04,
	// our current min Ubuntu requirement, by Mesa 11.2). It also gives us geometry shaders
	// (which is useful for rendering wide lines and symbology in general). And removes the need to deal
	// with all the various OpenGL extensions we've been dealing with. Eventually Apple with remove OpenGL though,
	// and the hope is we will have access to the Rendering Hardware Interface (RHI) in the upcoming Qt6
	// (it's currently in Qt5.14 but only accessible to Qt internally). RHI will allow us to use GLSL shaders,
	// and will provide us with a higher-level interface/abstraction for setting the graphics pipeline state
	// (that RHI then hands off to OpenGL/Vulkan/Direct3D/Metal). But for now it's OpenGL 3.3.
	//
	format.setVersion(3, 3);
	//
	// Qt5 versions prior to 5.9 are unable to mix QPainter calls with OpenGL 3.x *core* profile calls:
	//   https://www.qt.io/blog/2017/01/27/opengl-core-profile-context-support-qpainter
	//
	// This means we must fallback to an OpenGL *compatibility* profile for Qt5 versions less than 5.9.
	// As mentioned above, this is not an option on macOS, which means macOS requires Qt5.9 or above.
	//
#if QT_VERSION >= QT_VERSION_CHECK(5,9,0)
	format.setProfile(QGLFormat::CoreProfile);
#else // Qt < 5.9 ...
	#if defined(Q_OS_MACOS)
		#error "macOS requires Qt5.9 or above."
	#else //  not macOS ...
		format.setProfile(QGLFormat::CompatibilityProfile);
	#endif
#endif

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
			std::string error_message = std::string("Unable to initialise GLEW: ") +
					reinterpret_cast<const char *>(glewGetErrorString(err));
			throw OpenGLException(
					GPLATES_ASSERTION_SOURCE,
					error_message.c_str());
		}
		//qDebug() << "Status: Using GLEW " << reinterpret_cast<const char *>(glewGetString(GLEW_VERSION));

		s_initialised_GLEW = true;

		// Get the OpenGL capabilities and parameters from the current OpenGL implementation.
		s_capabilities.initialise();
	}

	// The QGLFormat of our OpenGL context.
	const QGLFormat &qgl_format = get_qgl_format();

	// Make sure we got OpenGL 3.3 or greater.
	if (qgl_format.majorVersion() < 3 ||
		(qgl_format.majorVersion() == 3 && qgl_format.minorVersion() < 3))
	{
		throw OpenGLException(
			GPLATES_ASSERTION_SOURCE,
			"OpenGL 3.3 or greater is required.");
	}

	// We require a main framebuffer with an alpha channel.
	// A lot of main framebuffer and render-target rendering uses an alpha channel.
	//
	// TODO: Now that we're guaranteed support framebuffer objects we no longer need the main framebuffer
	//       for render-target rendering. Maybe we don't need alpha in main buffer.
	//       But modern H/W will have it anyway.
	if (!qgl_format.alpha())
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"Could not get alpha channel on main framebuffer.");
	}

	// We require a main framebuffer with a stencil buffer (usually interleaved with depth).
	// A lot of main framebuffer and render-target rendering uses a stencil buffer.
	if (!qgl_format.stencil())
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"Could not get a stencil buffer on the main framebuffer.");
	}

	qDebug() << "Context QGLFormat:" << qgl_format;
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GL>::non_null_ptr_type
GPlatesOpenGL::GLContext::create_gl()
{
	return GL::create(
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
	get_non_shared_state()->get_framebuffer_resource_manager()->deallocate_queued_resources();
	get_non_shared_state()->get_vertex_array_resource_manager()->deallocate_queued_resources();

	get_shared_state()->get_texture_object_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_renderbuffer_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_buffer_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_shader_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_program_resource_manager()->deallocate_queued_resources();
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_texture_object_resource_manager(GLTexture::resource_manager_type::create()),
	d_renderbuffer_resource_manager(GLRenderbuffer::resource_manager_type::create()),
	d_buffer_resource_manager(GLBuffer::resource_manager_type::create()),
	d_shader_resource_manager(GLShaderObject::resource_manager_type::create()),
	d_program_resource_manager(GLProgramObject::resource_manager_type::create())
{
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_texture(
		GL &gl,
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
			GLTexture::create_as_unique_ptr(gl));

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
				gl, texture_object, target, internalformat, width, height_opt.get(), depth_opt.get(), border, mipmapped);

		return texture_object;
	}

	if (height_opt)
	{
		// It's a 2D texture.
		GLTextureUtils::initialise_texture_object_2D(
				gl, texture_object, target, internalformat, width, height_opt.get(), border, mipmapped);

		return texture_object;
	}

	// It's a 1D texture.
	GLTextureUtils::initialise_texture_object_1D(
			gl, texture_object, target, internalformat, width, border, mipmapped);

	return texture_object;
}


GPlatesOpenGL::GLPixelBuffer::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_pixel_buffer(
		GL &gl,
		unsigned int size,
		GLenum usage)
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
	GLBuffer::shared_ptr_type buffer = GLBuffer::create(gl, GLBuffer::BUFFER_TYPE_PIXEL);
	buffer->gl_buffer_data(
			gl,
			// Could be 'TARGET_PIXEL_UNPACK_BUFFER' or 'TARGET_PIXEL_PACK_BUFFER'.
			// Doesn't really matter because only used internally as a temporary bind target...
			GLBuffer::TARGET_PIXEL_PACK_BUFFER,
			size,
			NULL/*Uninitialised memory*/,
			usage);

	// Create a new object and add it to the cache.
	const GLPixelBuffer::shared_ptr_type pixel_buffer = pixel_buffer_cache->allocate_object(
			GLPixelBuffer::create_as_unique_ptr(gl, buffer));

	return pixel_buffer;
}


GPlatesOpenGL::GLVertexArray::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_vertex_array(
		GL &gl)
{
	// Attempt to acquire a recycled object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array_opt =
			d_vertex_array_cache->allocate_object();
	if (!vertex_array_opt)
	{
		// Create a new vertex array and add it to the cache.
		vertex_array_opt = d_vertex_array_cache->allocate_object(
				GLVertexArray::create_unique(gl));
	}
	const GLVertexArray::shared_ptr_type &vertex_array = vertex_array_opt.get();

	// First clear the vertex array before returning to the client.
	vertex_array->clear(gl);

	return vertex_array;
}


GPlatesOpenGL::GLRenderbuffer::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::acquire_renderbuffer(
		GL &gl,
		GLint internalformat,
		GLsizei width,
		GLsizei height)
{
	// Lookup the correct renderbuffer object cache (matching the specified client parameters).
	const renderbuffer_key_type renderbuffer_key(internalformat, width, height);

	const renderbuffer_cache_type::shared_ptr_type renderbuffer_cache =
			get_renderbuffer_cache(renderbuffer_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer_opt =
			renderbuffer_cache->allocate_object();
	if (renderbuffer_opt)
	{
		const GLRenderbuffer::shared_ptr_type renderbuffer = renderbuffer_opt.get();

		// Make sure the previous client did not change the renderbuffer dimensions before
		// recycling the renderbuffer.
		GPlatesGlobal::Assert<OpenGLException>(
				renderbuffer->get_dimensions() == std::pair<GLuint,GLuint>(width, height) &&
					renderbuffer->get_internal_format() == internalformat,
				GPLATES_ASSERTION_SOURCE,
				"GLContext::SharedState::acquire_renderbuffer: Dimensions/format of "
					"recycled renderbuffer were changed.");

		return renderbuffer;
	}

	// Create a new object and add it to the cache.
	const GLRenderbuffer::shared_ptr_type renderbuffer = renderbuffer_cache->allocate_object(
			GLRenderbuffer::create_unique(gl));

	// Initialise the newly created renderbuffer object.
	renderbuffer->gl_renderbuffer_storage(gl, internalformat, width, height);

	return renderbuffer;
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
GPlatesOpenGL::GLContext::SharedState::get_full_screen_2D_textured_quad(
		GL &gl)
{
	// Create the sole unbound vertex array compile state if it hasn't already been created.
	if (!d_full_screen_2D_textured_quad)
	{
		d_full_screen_2D_textured_quad = GLUtils::create_full_screen_2D_textured_quad(gl);
	}

	return d_full_screen_2D_textured_quad.get();
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


GPlatesOpenGL::GLContext::SharedState::renderbuffer_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_renderbuffer_cache(
		const renderbuffer_key_type &renderbuffer_key)
{
	// Attempt to insert the pixel buffer key into the pixel buffer cache map.
	const std::pair<renderbuffer_cache_map_type::iterator, bool> insert_result =
			d_renderbuffer_cache_map.insert(
					renderbuffer_cache_map_type::value_type(
							renderbuffer_key,
							// Dummy (NULL) pixel buffer cache...
							renderbuffer_cache_type::shared_ptr_type()));

	renderbuffer_cache_map_type::iterator renderbuffer_cache_map_iter = insert_result.first;

	// If the pixel buffer key was inserted into the map then create the corresponding pixel buffer cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		renderbuffer_cache_map_iter->second = renderbuffer_cache_type::create();
	}

	return renderbuffer_cache_map_iter->second;
}


GPlatesOpenGL::GLStateStore::non_null_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_state_store(
		const GLCapabilities &capabilities)
{
	if (!d_state_store)
	{
		d_state_store = GLStateStore::create(
				GLStateSetStore::create(),
				GLStateSetKeys::create(capabilities));
	}

	return d_state_store.get();
}


GPlatesOpenGL::GLContext::NonSharedState::NonSharedState() :
	d_framebuffer_resource_manager(GLFramebuffer::resource_manager_type::create()),
	// Start off with an initial cache size of 1 - it'll grow as needed...
	d_renderbuffer_resource_manager(GLRenderbuffer::resource_manager_type::create()),
	d_vertex_array_resource_manager(GLVertexArray::resource_manager_type::create())
{
}


GPlatesOpenGL::GLFramebuffer::shared_ptr_type
GPlatesOpenGL::GLContext::NonSharedState::acquire_framebuffer(
		GL &gl,
		const GLFramebuffer::Classification &classification)
{
	// Lookup the correct framebuffer object cache (matching the specified classification).
	const framebuffer_key_type framebuffer_key = classification.get_tuple();

	const framebuffer_cache_type::shared_ptr_type framebuffer_cache =
			get_framebuffer_cache(framebuffer_key);

	// Attempt to acquire a recycled object.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer_opt =
			framebuffer_cache->allocate_object();
	if (framebuffer_opt)
	{
		const GLFramebuffer::shared_ptr_type framebuffer = framebuffer_opt.get();

		// First clear the framebuffer attachments before returning to the client.
		framebuffer->gl_detach_all(gl);

		// Also reset the glDrawBuffer(s)/glReadBuffer state to the default state for
		// a non-default, application-created framebuffer object.
		framebuffer->gl_draw_buffers(gl);
		framebuffer->gl_read_buffer(gl);

		return framebuffer;
	}

	// Create a new object and add it to the cache.
	const GLFramebuffer::shared_ptr_type framebuffer =
			framebuffer_cache->allocate_object(
					GLFramebuffer::create_as_unique_ptr(gl));

	return framebuffer;
}


bool
GPlatesOpenGL::GLContext::NonSharedState::check_framebuffer_completeness(
		GL &gl,
		const GLFramebuffer::shared_ptr_to_const_type &framebuffer,
		const GLFramebuffer::Classification &framebuffer_classification) const
{
	// See if we've already cached the framebuffer completeness status for the specified
	// framebuffer object classification.
	framebuffer_state_to_status_map_type::iterator framebuffer_status_iter =
			d_framebuffer_state_to_status_map.find(framebuffer_classification.get_tuple());
	if (framebuffer_status_iter == d_framebuffer_state_to_status_map.end())
	{
		const bool framebuffer_status = framebuffer->gl_check_framebuffer_status(gl);

		d_framebuffer_state_to_status_map[framebuffer_classification.get_tuple()] = framebuffer_status;

		return framebuffer_status;
	}

	return framebuffer_status_iter->second;
}


GPlatesOpenGL::GLContext::NonSharedState::framebuffer_cache_type::shared_ptr_type
GPlatesOpenGL::GLContext::NonSharedState::get_framebuffer_cache(
		const framebuffer_key_type &framebuffer_key)
{
	// Attempt to insert the framebuffer object key into the framebuffer object cache map.
	const std::pair<framebuffer_cache_map_type::iterator, bool> insert_result =
			d_framebuffer_cache_map.insert(
					framebuffer_cache_map_type::value_type(
							framebuffer_key,
							// Dummy (NULL) framebuffer object cache...
							framebuffer_cache_type::shared_ptr_type()));

	framebuffer_cache_map_type::iterator framebuffer_cache_map_iter = insert_result.first;

	// If the framebuffer object key was inserted into the map then create the corresponding framebuffer object cache.
	if (insert_result.second)
	{
		// Start off with an initial cache size of 1 - it'll grow as needed...
		framebuffer_cache_map_iter->second = framebuffer_cache_type::create();
	}

	return framebuffer_cache_map_iter->second;
}
