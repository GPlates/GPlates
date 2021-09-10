/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <limits>
#include <boost/optional.hpp>
#include <QFont>
#include <QPaintDevice>
#include <QPainter>

#include "GLText.h"

#include "GLRenderer.h"
#include "GLViewport.h"
#include "GLViewProjection.h"
#include "OpenGLException.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Returns a scaled version of the specified font.
		 */
		QFont
		scale_font(
				const QFont &font,
				float scale)
		{
			QFont ret = font;

			const qreal min_point_size = 2.0;
			qreal point_size = QFontInfo(font).pointSizeF();
			ret.setPointSizeF((std::max)(min_point_size, point_size * scale));

			return ret;
		}


		/**
		 * Renders text by delegating to the QPainter passed into GLRenderer.
		 */
		void
		render_text(
				GLRenderer &renderer,
				float x,
				float y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font,
				float scale)
		{
		}
	}
}


void
GPlatesOpenGL::GLText::render_text_3D(
		GLRenderer &renderer,
		double world_x,
		double world_y,
		double world_z,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font,
		float scale)
{
	// Before we suspend GLRenderer (and resume QPainter) we'll get the scissor rectangle
	// if scissoring is enabled and use that as a clip rectangle.
	boost::optional<GLViewport> scissor_rect;
	if (renderer.gl_get_enable(GL_SCISSOR_TEST))
	{
		scissor_rect = renderer.gl_get_scissor();
	}

	// And before we suspend GLRenderer (and resume QPainter) we'll get the viewport,
	// model-view transform and projection transform.
	const GLViewport viewport = renderer.gl_get_viewport();
	const GLMatrix model_view_transform = renderer.gl_get_matrix(GL_MODELVIEW);
	const GLMatrix projection_transform = renderer.gl_get_matrix(GL_PROJECTION);

	// Suspend rendering with 'GLRenderer' so we can resume painting with 'QPainter'.
	// At scope exit we can resume rendering with 'GLRenderer'.
	//
	// We do this because the QPainter's paint engine might be OpenGL and we need to make sure
	// it's OpenGL state does not interfere with the OpenGL state of 'GLRenderer' and vice versa.
	// This also provides a means to retrieve the QPainter for rendering text.
	GPlatesOpenGL::GLRenderer::QPainterBlockScope qpainter_block_scope(renderer);

	boost::optional<QPainter &> qpainter = qpainter_block_scope.get_qpainter();

	// We need a QPainter - one should have been specified to 'GLRenderer::begin_render'.
	GPlatesGlobal::Assert<OpenGLException>(
			qpainter,
			GPLATES_ASSERTION_SOURCE,
			"GLText: attempted to render text using a GLRenderer that does not have a QPainter attached.");

	// The QPainter's paint device.
	const QPaintDevice *qpaint_device = qpainter->device();
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			qpaint_device,
			GPLATES_ASSERTION_SOURCE);

	const int qpaint_device_pixel_ratio = qpaint_device->devicePixelRatio();

	// Set the identity world transform since our input position is specified in *window* coordinates
	// and we don't want it transformed by the current world transform.
	qpainter->setWorldTransform(QTransform()/*identity*/);

	// Set the clip rectangle if the GLRenderer has scissor testing enabled.
	if (scissor_rect)
	{
		// Note that the scissor rectangle is in OpenGL device pixel coordinates, but parameters to QPainter
		// should be in device *independent* coordinates (hence the divide by device pixel ratio).
		//
		// Note: Using floating-point QRectF to avoid rounding to nearest 'qpaint_device_pixel_ratio' device pixel
		//       if scissor rect has, for example, odd coordinates (and device pixel ratio is the integer 2).
		qpainter->setClipRect(
				QRectF(
						scissor_rect->x() / qreal(qpaint_device_pixel_ratio),
						// Also need to convert scissor rectangle from OpenGL to Qt (ie, invert y-axis)...
						qpaint_device->height() - (scissor_rect->y() + scissor_rect->height()) / qreal(qpaint_device_pixel_ratio),
						scissor_rect->width() / qreal(qpaint_device_pixel_ratio),
						scissor_rect->height() / qreal(qpaint_device_pixel_ratio)));
	}

	// Pass our world coordinates through the model-view-projection transform and viewport
	// to get our new viewport coordinates.
	//
	// Note: Since OpenGL viewports are in device pixels the output window coordinates are also in device pixels.
	//       If the input world coordinates are in device-independent pixels (eg, 2D text rendering) then the
	//       projection transform would have been specified using the device-independent paint device dimensions.
	GLdouble win_x, win_y, win_z;
	GLViewProjection view_projection(viewport, model_view_transform, projection_transform);
	view_projection.glu_project(
			world_x, world_y, world_z,
			&win_x, &win_y, &win_z);

	// Clip if the window z-coordinate is outside the [0,1] range corresponding to the
	// near and far clip planes.
	// Note that we don't clip in the x and y directions since that's more difficult due to
	// font sizes, etc, and we just rely on window or scissor clipping (ie, pixels outside the
	// drawable region won't get rendered).
	if (win_z < 0 || win_z > 1)
	{
		return;
	}

	// Set the font and colour.
	qpainter->setPen(colour);
	qpainter->setFont(scale_font(font, scale));

	// Get the Qt window coordinates at which to render text.
	//
	// Note that 'win_x' and 'win_y' are in OpenGL device pixel coordinates, but parameters to QPainter
	// should be in device *independent* coordinates (hence the divide by device pixel ratio).
	// Also note that 'x_offset' and 'y_offset' are in device-independent coordinates (hence no divide).
	const float qt_win_x = (win_x / qpaint_device_pixel_ratio) + x_offset;
	// Also note that OpenGL and Qt y-axes are the reverse of each other.
	// This applies to both 'win_y' and 'y_offset'.
	const float qt_win_y = qpaint_device->height() - (win_y / qpaint_device_pixel_ratio) - y_offset;

	qpainter->drawText(QPointF(qt_win_x, qt_win_y), string);

	// Turn off clipping if it was turned on.
	if (scissor_rect)
	{
		qpainter->setClipRect(QRect(), Qt::NoClip);
	}

	// At scope exit we can resume rendering with 'GLRenderer'...
}
