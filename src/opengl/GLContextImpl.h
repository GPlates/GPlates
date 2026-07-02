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

#include <QOpenGLWidget>

#include "GLContext.h"


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
					QOpenGLWidget &qgl_widget) :
				d_qgl_widget(qgl_widget)
			{  }

			virtual
			void
			make_current()
			{
				d_qgl_widget.makeCurrent();
			}

			virtual
			const QSurfaceFormat
			get_qgl_format() const
			{
				return d_qgl_widget.format();
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
			QOpenGLWidget &d_qgl_widget;
		};
	}
}

#endif // GPLATES_OPENGL_GLCONTEXTIMPL_H
