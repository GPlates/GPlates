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

#include <exception>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>

#include <QDebug>

#include "GLOffScreenContext.h"

#include "GLRenderer.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLOffScreenContext::GLOffScreenContext(
		const QGLFormat &qgl_format)
{
	initialise(qgl_format);
}


GPlatesOpenGL::GLOffScreenContext::GLOffScreenContext(
		const QGLWidgetContext &qgl_widget_context) :
	d_qgl_widget_context(qgl_widget_context)
{
	// Use the same format as the existing context...
	initialise(qgl_widget_context.context->get_qgl_format());
}


bool
GPlatesOpenGL::GLOffScreenContext::is_valid() const
{
	return d_off_screen_context || d_qgl_widget_context;
}


bool
GPlatesOpenGL::GLOffScreenContext::is_off_screen() const
{
	// If truly rendering to off-screen then the off-screen context should be valid.
	return d_off_screen_context;
}


GPlatesOpenGL::GLRenderer::non_null_ptr_type
GPlatesOpenGL::GLOffScreenContext::begin_off_screen_render(
		unsigned int frame_buffer_width,
		unsigned int frame_buffer_height,
		boost::optional<QPainter &> qpainter,
		bool paint_device_is_framebuffer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_valid() && !d_renderer,
			GPLATES_ASSERTION_SOURCE);

	if (d_off_screen_context)
	{
		GLContext &off_screen_context = *d_off_screen_context.get();

		// Make sure our OpenGL context is the currently active context.
		// It could be either the QGLWidget context or the 'pbuffer' context.
		off_screen_context.make_current();

		if (d_screen_render_target)
		{
			// Create a renderer.
			// Convert from non_null_intrusive_ptr to shared_ptr so that 'GLOffScreenContext.h'
			// does not need to include 'GLRenderer.h'...
			d_renderer = make_shared_from_intrusive(off_screen_context.create_renderer());

			// Start a new render scope before we can use the renderer.
			if (qpainter)
			{
				d_renderer.get()->begin_render(qpainter.get(), paint_device_is_framebuffer);
			}
			else
			{
				d_renderer.get()->begin_render();
			}

			// Begin rendering to the screen render target.
			d_screen_render_target.get()->begin_render(
					*d_renderer.get(),
					frame_buffer_width,
					frame_buffer_height);
		}
		else // Using a 'pbuffer'...
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_qgl_pixel_buffer && d_qgl_pixel_buffer_impl,
					GPLATES_ASSERTION_SOURCE);

			// If using a 'pbuffer' then update its dimensions if necessary.
			if (frame_buffer_width != off_screen_context.get_width() ||
				frame_buffer_height != off_screen_context.get_height())
			{
				// Release the current 'pbuffer'.
				d_qgl_pixel_buffer = boost::none;

				// Create a new 'pbuffer'.
				d_qgl_pixel_buffer = boost::in_place(
						frame_buffer_width,
						frame_buffer_height,
						off_screen_context.get_qgl_format(),
						// It's important to share textures, etc, with our QGLWidget OpenGL context (if provided)...
						d_qgl_widget_context /*shareWidget*/
								? d_qgl_widget_context->qgl_widget
								: static_cast<QGLWidget *>(0));

				// Install the new QGLPixelBuffer into our 'pbuffer' context impl.
				d_qgl_pixel_buffer_impl.get()->set_pixel_buffer(d_qgl_pixel_buffer.get());

				// We've just installed a new 'pbuffer' context so make it current.
				off_screen_context.make_current();
			}

			// Create a renderer.
			// NOTE: We do this after making any changes to the 'pbuffer' dimensions so
			// that the renderer gets the correct frame buffer dimensions.
			// Convert from non_null_intrusive_ptr to shared_ptr so that 'GLOffScreenContext.h'
			// does not need to include 'GLRenderer.h'...
			d_renderer = make_shared_from_intrusive(off_screen_context.create_renderer());

			// Start a new render scope before we can use the renderer.
			if (qpainter)
			{
				d_renderer.get()->begin_render(qpainter.get(), paint_device_is_framebuffer);
			}
			else
			{
				d_renderer.get()->begin_render();
			}
		}
	}
	else // Emulating off-screen rendering via the QGLWidget main frame buffer...
	{
		GLContext &qgl_widget_context = *d_qgl_widget_context->context;

		// Make sure the QGLWidget OpenGL context is the currently active context.
		qgl_widget_context.make_current();

		// Create a renderer.
			// Convert from non_null_intrusive_ptr to shared_ptr so that 'GLOffScreenContext.h'
			// does not need to include 'GLRenderer.h'...
		d_renderer = make_shared_from_intrusive(qgl_widget_context.create_renderer());

		// Start a new render scope before we can use the renderer.
		if (qpainter)
		{
			d_renderer.get()->begin_render(qpainter.get(), paint_device_is_framebuffer);
		}
		else
		{
			d_renderer.get()->begin_render();
		}

		// We are falling back to using the *main* frame buffer of the QGLWidget context.
		// We need to preserve the main frame buffer (since not using frame buffer object or pbuffer).
		d_save_restore_framebuffer = boost::in_place(
				d_renderer.get()->get_capabilities(),
				qgl_widget_context.get_width(),
				qgl_widget_context.get_height(),
				GL_RGBA8/*save_restore_colour_texture_internalformat*/,
				qgl_widget_context.get_qgl_format().depth()/*save_restore_depth_buffer*/,
				qgl_widget_context.get_qgl_format().stencil()/*save_restore_stencil_buffer*/);

		// Save its contents.
		d_save_restore_framebuffer->save(*d_renderer.get());
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_renderer,
			GPLATES_ASSERTION_SOURCE);

	// Convert from shared_ptr back to non_null_intrusive_ptr.
	return GLRenderer::non_null_ptr_type(d_renderer.get().get());
}


void
GPlatesOpenGL::GLOffScreenContext::end_off_screen_render()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_valid() && d_renderer,
			GPLATES_ASSERTION_SOURCE);

	if (d_off_screen_context)
	{
		// End rendering to the off-screen target.
		if (d_screen_render_target)
		{
			d_screen_render_target.get()->end_render(*d_renderer.get());
		}
		// else if 'pbuffer' then nothing to do.
	}
	else
	{
		// We are falling back to using the *main* frame buffer of the QGLWidget context.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_qgl_widget_context && d_save_restore_framebuffer,
				GPLATES_ASSERTION_SOURCE);

		// Restore its contents.
		d_save_restore_framebuffer->restore(*d_renderer.get());

		// Finished with the save/restore.
		d_save_restore_framebuffer = boost::none;
	}

	// End the render scope.
	d_renderer.get()->end_render();

	// Finished rendering.
	d_renderer = boost::none;
}


void
GPlatesOpenGL::GLOffScreenContext::initialise(
		const QGLFormat &qgl_format)
{
	if (d_qgl_widget_context)
	{
		// Try a frame buffer object in the QGLWidget context.
		d_off_screen_context = d_qgl_widget_context->context;
		if (initialise_screen_render_target())
		{
			return;
		}

		// Next try a 'pbuffer' context.
		d_off_screen_context = boost::none;
		if (initialise_pbuffer_context(
				qgl_format,
				d_qgl_widget_context->qgl_widget->width(),
				d_qgl_widget_context->qgl_widget->height()))
		{
			return;
		}

		// Fall back to using main frame buffer of the QGLWidget.
		d_off_screen_context = boost::none;

		return;
	}

	// We need to specify buffer dimensions but we don't know this until @a begin_off_screen_render
	// is called - for now we'll just specify an arbitrary dimension.
	const int initial_pbuffer_dimension = 256;
	if (initialise_pbuffer_context(
			qgl_format,
			initial_pbuffer_dimension/*initial_width*/,
			initial_pbuffer_dimension/*initial_height*/))
	{
		// Attempt to use a frame buffer object even though we already have an off-screen buffer
		// in the form of a 'pbuffer'. This is because it's faster to later change the dimensions
		// of an FBO than it is for a 'pbuffer'.
		if (initialise_screen_render_target())
		{
			return;
		}

		// Otherwise just use the 'pbuffer' itself as the off-screen render target.
		return;
	}

	// If we get here then 'is_valid()' should return false.
	d_off_screen_context = boost::none;
}


bool
GPlatesOpenGL::GLOffScreenContext::initialise_screen_render_target()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_off_screen_context,
			GPLATES_ASSERTION_SOURCE);

	GLContext &off_screen_context = *d_off_screen_context.get();

	// Make sure our OpenGL context is the currently active context.
	off_screen_context.make_current();

	// Create a temporary renderer so we can query for screen render target support and also create one.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GLRenderer::non_null_ptr_type renderer = off_screen_context.create_renderer();

	// Pass in the viewport of the window currently attached to the OpenGL context.
	GLRenderer::RenderScope render_scope(*renderer);

	const GLint texture_internalformat = GL_RGBA8;
	const bool include_depth_buffer = off_screen_context.get_qgl_format().depth();
	const bool include_stencil_buffer = off_screen_context.get_qgl_format().stencil();

	if (!GLScreenRenderTarget::is_supported(
			*renderer,
			texture_internalformat,
			include_depth_buffer,
			include_stencil_buffer))
	{
		return false;
	}

	d_screen_render_target =
			GLScreenRenderTarget::create(
					*renderer,
					texture_internalformat,
					include_depth_buffer,
					include_stencil_buffer);

	return true;
}


bool
GPlatesOpenGL::GLOffScreenContext::initialise_pbuffer_context(
		const QGLFormat &qgl_format,
		int initial_width,
		int initial_height)
{
	// Return early if 'pbuffer' extension is not supported.
	if (!QGLPixelBuffer::hasOpenGLPbuffers())
	{
		return false;
	}

	// Create a QGLPixelBuffer.
	d_qgl_pixel_buffer = boost::in_place(
			initial_width,
			initial_height,
			qgl_format,
			// It's important to share textures, etc, with our QGLWidget OpenGL context (if provided)...
			d_qgl_widget_context /*shareWidget*/
					? d_qgl_widget_context->qgl_widget
					: static_cast<QGLWidget *>(0));

	// Return early if the QGLPixelBuffer is invalid.
	if (!d_qgl_pixel_buffer->isValid())
	{
		d_qgl_pixel_buffer = boost::none;
		return false;
	}

	d_qgl_pixel_buffer_impl = boost::in_place(
			new GLContextImpl::QGLPixelBufferImpl(d_qgl_pixel_buffer.get()));

	// Create a context (wrapper) for the QGLPixelBuffer.
	if (d_qgl_widget_context)
	{
		d_off_screen_context = GLContext::create(
				d_qgl_pixel_buffer_impl.get(), 
				*d_qgl_widget_context->context);
	}
	else
	{
		d_off_screen_context = GLContext::create(
				d_qgl_pixel_buffer_impl.get());
	}

	return true;
}


GPlatesOpenGL::GLOffScreenContext::RenderScope::RenderScope(
		GLOffScreenContext &off_screen_context,
		unsigned int frame_buffer_width,
		unsigned int frame_buffer_height,
		boost::optional<QPainter &> qpainter,
		bool paint_device_is_framebuffer) :
	d_off_screen_context(off_screen_context),
	d_renderer(
			// Convert from non_null_intrusive_ptr to shared_ptr so that 'GLOffScreenContext.h'
			// does not need to include 'GLRenderer.h'...
			make_shared_from_intrusive(
					off_screen_context.begin_off_screen_render(
							frame_buffer_width,
							frame_buffer_height,
							qpainter,
							paint_device_is_framebuffer))),
	d_called_end_render(false)
{
}


GPlatesOpenGL::GLOffScreenContext::RenderScope::~RenderScope()
{
	if (!d_called_end_render)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_off_screen_context.end_off_screen_render();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GLOffScreenContext: exception thrown during render scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "GLOffScreenContext: exception thrown during render scope: Unknown error";
		}
	}
}


GPlatesOpenGL::GLRenderer::non_null_ptr_type
GPlatesOpenGL::GLOffScreenContext::RenderScope::get_renderer() const
{
	// Convert from shared_ptr back to non_null_intrusive_ptr.
	return GLRenderer::non_null_ptr_type(d_renderer.get());
}


void
GPlatesOpenGL::GLOffScreenContext::RenderScope::end_off_screen_render()
{
	if (!d_called_end_render)
	{
		d_off_screen_context.end_off_screen_render();
		d_called_end_render = true;
	}
}
