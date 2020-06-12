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

#ifndef GPLATES_OPENGL_GLCONTEXTIMPL_H
#define GPLATES_OPENGL_GLCONTEXTIMPL_H

#include <QGLPixelBuffer>
#include <QGLWidget>

#include "GLContext.h"


namespace GPlatesOpenGL
{
	namespace GLContextImpl
	{
		/**
		 * A derivation of GLContext::Impl for QGLWidget.
		 */
		class QGLWidgetImpl :
				public GLContext::Impl
		{
		public:
			explicit
			QGLWidgetImpl(
					QGLWidget &qgl_widget) :
				d_qgl_widget(qgl_widget)
			{  }

			virtual
			void
			make_current()
			{
				d_qgl_widget.makeCurrent();
			}

			virtual
			const QGLFormat
			get_qgl_format() const
			{
				return d_qgl_widget.context()->format();
			}

			virtual
			unsigned int
			get_width() const
			{
				// Dimensions, in OpenGL, are in device pixels.
				return d_qgl_widget.width() * d_qgl_widget.devicePixelRatio();
			}

			virtual
			unsigned int
			get_height() const
			{
				// Dimensions, in OpenGL, are in device pixels.
				return d_qgl_widget.height() * d_qgl_widget.devicePixelRatio();
			}

		private:
			QGLWidget &d_qgl_widget;
		};


		/**
		 * A derivation of GLContext::Impl for QGLPixelBuffer.
		 */
		class QGLPixelBufferImpl :
				public GLContext::Impl
		{
		public:
			explicit
			QGLPixelBufferImpl(
					QGLPixelBuffer &qgl_pixel_buffer) :
				d_qgl_pixel_buffer(&qgl_pixel_buffer)
			{  }

			void
			set_pixel_buffer(
					QGLPixelBuffer &qgl_pixel_buffer)
			{
				d_qgl_pixel_buffer = &qgl_pixel_buffer;
			}

			QGLPixelBuffer &
			get_pixel_buffer() const
			{
				return *d_qgl_pixel_buffer;
			}

			virtual
			void
			make_current()
			{
				d_qgl_pixel_buffer->makeCurrent();
			}

			virtual
			const QGLFormat
			get_qgl_format() const
			{
				return d_qgl_pixel_buffer->format();
			}

			virtual
			unsigned int
			get_width() const
			{
				// Dimensions, in OpenGL, are in device pixels.
				return d_qgl_pixel_buffer->width() * d_qgl_pixel_buffer->devicePixelRatio();
			}

			virtual
			unsigned int
			get_height() const
			{
				// Dimensions, in OpenGL, are in device pixels.
				return d_qgl_pixel_buffer->height() * d_qgl_pixel_buffer->devicePixelRatio();
			}

		private:
			QGLPixelBuffer *d_qgl_pixel_buffer;
		};
	}
}

#endif // GPLATES_OPENGL_GLCONTEXTIMPL_H
