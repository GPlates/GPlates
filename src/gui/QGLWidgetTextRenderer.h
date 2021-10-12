/* $Id$ */

/**
 * @file 
 * Renders text on an OpenGL canvas using a QGLWidget
 * 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_QGLWIDGETTEXTRENDERER_H
#define GPLATES_GUI_QGLWIDGETTEXTRENDERER_H

#include "TextRenderer.h"

#include <QGLWidget>
#include "OpenGL.h"

namespace GPlatesGui
{
	class QGLWidgetTextRenderer :
		public TextRenderer
	{

	public:

		/**
		 * Construct an instance of QGLWidgetTextRenderer on the heap
		 */
		static
		TextRenderer::ptr_to_const_type
		create(
				QGLWidget *gl_widget_ptr)
		{
			return TextRenderer::ptr_to_const_type(
					new QGLWidgetTextRenderer(gl_widget_ptr));
		}

		/**
		 * Renders @a string at position (@a x , @a y ) in window coordinates
		 * using a particular @a colour and @a font.
		 */
		virtual
		void
		render_text(
				int x,
				int y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font = QFont(),
				float scale = 1.0f) const;

		/**
		 * Renders @a string at position (@a x , @a y , @a z ) in scene coordinates
		 * using a particular @a colour and @a font.
		 */
		virtual
		void
		render_text(
				double x,
				double y,
				double z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset = 0,
				int y_offset = 0,
				const QFont &font = QFont(),
				float scale = 1.0f) const;

	private:

		QGLWidget *d_gl_widget_ptr;

		// prevent direct instantiation
		explicit
		QGLWidgetTextRenderer(
				QGLWidget *gl_widget_ptr);

	};
}

#endif  // GPLATES_GUI_QGLWIDGETTEXTRENDERER_H
