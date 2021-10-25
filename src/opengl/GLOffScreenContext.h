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

#ifndef GPLATES_OPENGL_GLOFFSCREENCONTEXT_H
#define GPLATES_OPENGL_GLOFFSCREENCONTEXT_H

#include <utility>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <QGLFormat>
#include <QGLPixelBuffer>
#include <QGLWidget>
#include <QPainter>

#include "GLContext.h"
#include "GLContextImpl.h"
#include "GLSaveRestoreFrameBuffer.h"
#include "GLScreenRenderTarget.h"

#include "global/PointerTraits.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An off-screen OpenGL context (or fall back to emulation of off-screen using a QGLWidget frame buffer).
	 *
	 * This is mainly useful for when you need to avoid, where possible, rendering to the
	 * *main* frame buffer of a QGLWidget (because, while you can use its OpenGL context outside
	 * its paint event, you cannot modify its *main* frame buffer outside its paint event).
	 *
	 * This class really just takes the extra precaution of using an off-screen 'pbuffer',
	 * if supported, before being forced to fall back to using the *main* frame buffer of QGLWidget.
	 *
	 * Otherwise using the GLRenderer interface with its render target abilities should suffice
	 * for rendering to render targets.
	 */
	class GLOffScreenContext :
			public GPlatesUtils::ReferenceCount<GLOffScreenContext>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLOffScreenContext.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLOffScreenContext> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLOffScreenContext.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLOffScreenContext> non_null_ptr_to_const_type;


		/**
		 * Associates a QGLWidget with its OpenGL context.
		 *
		 * In some cases we use QGLPixelBuffer (which has its own OpenGL context) for off-screen
		 * rendering and it explicitly requires a QGLWidget in order to enable sharing of texture,
		 * etc, resources between the two contexts.
		 */
		struct QGLWidgetContext
		{
			QGLWidgetContext(
					QGLWidget *qgl_widget_,
					const GLContext::non_null_ptr_type &context_) :
				qgl_widget(qgl_widget_),
				context(context_)
			{  }

			QGLWidget *qgl_widget;
			GLContext::non_null_ptr_type context;
		};


		/**
		 * Creates an off-screen OpenGL context and associated frame buffer using the specified format.
		 *
		 * If the window-system-specific 'pbuffer' extension is supported then a 'pbuffer' OpenGL
		 * context and associated frame buffer are created. Additionally if GL_EXT_framebuffer_object
		 * is supported then it is used as the frame buffer within the 'pbuffer' OpenGL context.
		 *
		 * If the 'pbuffer' extension is not supported then @a is_valid will return false.
		 */
		static
		non_null_ptr_type
		create(
				const QGLFormat &qgl_format)
		{
			return non_null_ptr_type(new GLOffScreenContext(qgl_format));
		}


		/**
		 * Creates an off-screen render target that attempts to use the OpenGL context of the
		 * specified QGLWidget.
		 *
		 * If GL_EXT_framebuffer_object is supported then a frame buffer object is used as the
		 * off-screen frame buffer (since it's more efficient than 'pbuffer's).
		 * Otherwise, if the window-system-specific 'pbuffer' extension is supported, a 'pbuffer'
		 * OpenGL context and associated frame buffer are created (the context shares texture, etc,
		 * with the QGLWidget context).
		 * Otherwise falls back to using the main frame buffer of the QGLWidget context (with additional
		 * save/restore of the frame buffer contents to avoid corrupting any previous rendering).
		 */
		static
		non_null_ptr_type
		create(
				const QGLWidgetContext &qgl_widget_context)
		{
			return non_null_ptr_type(new GLOffScreenContext(qgl_widget_context));
		}


		/**
		 * Returns true if the off-screen context is valid.
		 *
		 * If this returns false then it cannot be used for rendering.
		 *
		 * This always returns true if a QGLWidget context was passed into @a create.
		 *
		 * This can return false if the 'pbuffer' extension is not supported and
		 * no QGLWidget OpenGL context was provided.
		 */
		bool
		is_valid() const;


		/**
		 * Returns true if the rendering will truly be off-screen.
		 *
		 * If false is returned then rendering will fallback to the *main* frame buffer of
		 * the QGLWidget specified in @a create - in order to emulate off-screen rendering.
		 */
		bool
		is_off_screen() const;


		/**
		 * Begins an off-screen render scope that targets this off-screen context and associated frame buffer.
		 *
		 * NOTE: This should only be called when you know the full OpenGL state is set to the default OpenGL state.
		 * This is the assumption that the returned renderer is making.
		 *
		 * @a main_frame_buffer_width and @a main_frame_buffer_height represent the desired
		 * dimensions of the off-screen frame buffer.
		 *
		 * NOTE: If fall back to the main frame buffer (of a QGLWidget) is used then
		 * the frame buffer dimensions will be that of the QGLWidget - see @a create.
		 *
		 * The final frame buffer dimensions can be queried using
		 *   GLRenderer::get_current_frame_buffer_dimensions().
		 *
		 * See GLRenderer::begin_render() for details involving QPainter.
		 *
		 * Throws exception if @a is_valid return false.
		 */
		GPlatesGlobal::PointerTraits<GLRenderer>::non_null_ptr_type
		begin_off_screen_render(
				unsigned int frame_buffer_width,
				unsigned int frame_buffer_height,
				boost::optional<QPainter &> qpainter = boost::none,
				// Does the QPainter render to the framebuffer or some other paint device ? ...
				bool paint_device_is_framebuffer = true);

		/**
		 * Ends the current off-screen render scope.
		 *
		 * The GLRenderer returned by @a begin_off_screen_render should not be used after this.
		 *
		 * Throws exception if @a is_valid return false.
		 */
		void
		end_off_screen_render();


		/**
		 * RAII class to call @a begin_render and @a end_render over a scope.
		 */
		class RenderScope :
				private boost::noncopyable
		{
		public:
			RenderScope(
					GLOffScreenContext &off_screen_context,
					unsigned int frame_buffer_width,
					unsigned int frame_buffer_height,
					boost::optional<QPainter &> qpainter = boost::none,
					bool paint_device_is_framebuffer = true);

			~RenderScope();

			//! Returns the renderer.
			GPlatesGlobal::PointerTraits<GLRenderer>::non_null_ptr_type
			get_renderer() const;

			//! Opportunity to end off-screen rendering before the scope exits (when destructor is called).
			void
			end_off_screen_render();

		private:
			GLOffScreenContext &d_off_screen_context;
			// NOTE: This is a shared_ptr instead of non_null_intrusive_ptr purely so we don't
			// have to include "GLRenderer.h"...
			boost::shared_ptr<GLRenderer> d_renderer;
			bool d_called_end_render;
		};

	private:

		/**
		 * This is only valid if a QGLWidget context was provided.
		 */
		const boost::optional<QGLWidgetContext> d_qgl_widget_context;

		/**
		 * The OpenGL context used for off-screen rendering.
		 *
		 * This is boost::none if falling back to emulation via main frame buffer of QGLWidget.
		 */
		boost::optional<GLContext::non_null_ptr_type> d_off_screen_context;

		//
		// Various options for implementing off-screen rendering.
		//

		boost::optional<GLScreenRenderTarget::shared_ptr_type> d_screen_render_target;

		boost::optional<QGLPixelBuffer> d_qgl_pixel_buffer;
		boost::optional< boost::shared_ptr<GLContextImpl::QGLPixelBufferImpl> > d_qgl_pixel_buffer_impl;

		/**
		 * The renderer is only valid between @a begin_off_screen_render and @a end_off_screen_render.
		 *
		 * NOTE: This is a shared_ptr instead of non_null_intrusive_ptr purely so we don't
		 * have to include "GLRenderer.h".
		 */
		boost::optional< boost::shared_ptr<GLRenderer> > d_renderer;

		/**
		 * Used to save/restore the QGLWidget frame buffer when 'pbuffer' and frame buffer objects not supported.
		 *
		 * This is only valid between @a begin_off_screen_render and @a end_off_screen_render.
		 */
		boost::optional<GLSaveRestoreFrameBuffer> d_save_restore_framebuffer;


		GLOffScreenContext(
				const QGLFormat &qgl_format);

		GLOffScreenContext(
				const QGLWidgetContext &qgl_widget_context);

		void
		initialise(
				const QGLFormat &qgl_format);

		bool
		initialise_screen_render_target();

		bool
		initialise_pbuffer_context(
				const QGLFormat &qgl_format,
				int initial_width,
				int initial_height);

	};
}

#endif // GPLATES_OPENGL_GLOFFSCREENCONTEXT_H
