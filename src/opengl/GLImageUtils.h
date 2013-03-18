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

#ifndef GPLATES_OPENGL_GLIMAGEUTILS_H
#define GPLATES_OPENGL_GLIMAGEUTILS_H

#include <QColor>
#include <QImage>
#include <QString>

#include "GLPixelBuffer.h"
#include "GLViewport.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	namespace GLImageUtils
	{
		/**
		 * Copies the specified source rectangle of the currently bound frame buffer into
		 * the specified destination rectangle of the QImage.
		 *
		 * NOTE: The currently bound frame buffer is expected to be fixed-point RGBA8 format and
		 * the image format is expected to be QImage::Format_ARGB32.
		 *
		 * Note that OpenGL and Qt y-axes are the reverse of each other and both viewports are
		 * specified in the OpenGL coordinate frame.
		 */
		void
		copy_rgba8_frame_buffer_into_argb32_qimage(
				GLRenderer &renderer,
				QImage &image,
				const GLViewport &source_viewport,
				const GLViewport &destination_viewport);


		/**
		 * Draws the specified text into a QImage the specified size.
		 */
		QImage
		draw_text_into_qimage(
				const QString &text,
				unsigned int image_width,
				unsigned int image_height,
				const float text_scale = 1.0f,
				const QColor &text_colour = QColor(255, 255, 255, 255)/*white*/,
				const QColor &background_colour = QColor(0, 0, 0, 255)/*black*/);
	}
}

#endif // GPLATES_OPENGL_GLIMAGEUTILS_H
