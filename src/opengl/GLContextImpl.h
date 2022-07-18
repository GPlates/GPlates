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

#include <QOpenGLContext>
#include <QOpenGLWidget>

#include "GLContext.h"
#include "OpenGLException.h"

#include "global/GPlatesAssert.h"


namespace GPlatesOpenGL
{
	namespace GLContextImpl
	{
		/**
		 * A derivation of GLContext::Impl for QOpenGLWidget.
		 */
		class QOpenGLWidgetImpl :
				public GLContext::Impl
		{
		public:
			explicit
			QOpenGLWidgetImpl(
					QOpenGLWidget &opengl_widget) :
				d_opengl_widget(opengl_widget)
			{  }

			virtual
			void
			make_current()
			{
				d_opengl_widget.makeCurrent();
			}

			virtual
			const QSurfaceFormat
			get_surface_format() const
			{
				// Make sure the QOpenGLContext used by QOpenGLWidget has been initialised.
				const QOpenGLContext *opengl_context = d_opengl_widget.context();
				GPlatesGlobal::Assert<OpenGLException>(
						opengl_context,
						GPLATES_ASSERTION_SOURCE,
						"QOpenGLContext not yet initialized.");

				return opengl_context->format();
			}

			virtual
			unsigned int
			get_width() const
			{
				// Dimensions, in OpenGL, are in device pixels.
				return d_opengl_widget.width() * d_opengl_widget.devicePixelRatio();
			}

			virtual
			unsigned int
			get_height() const
			{
				// Dimensions, in OpenGL, are in device pixels.
				return d_opengl_widget.height() * d_opengl_widget.devicePixelRatio();
			}

		private:
			QOpenGLWidget &d_opengl_widget;
		};
	}
}

#endif // GPLATES_OPENGL_GLCONTEXTIMPL_H
