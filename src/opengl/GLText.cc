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

#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLViewport.h"
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
			// Before we suspend GLRenderer (and resume QPainter) we'll get the scissor rectangle
			// if scissoring is enabled and use that as a clip rectangle.
			boost::optional<GLViewport> scissor_rect;
			if (renderer.gl_get_enable(GL_SCISSOR_TEST))
			{
				scissor_rect = renderer.gl_get_scissor();
			}

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

			// Set the identity world transform since our input position is specified in *window* coordinates
			// and we don't want it transformed by the current world transform.
			qpainter->setWorldTransform(QTransform()/*identity*/);

			// Set the clip rectangle if the GLRenderer has scissor testing enabled.
			if (scissor_rect)
			{
				qpainter->setClipRect(
						scissor_rect->x(),
						// Also need to convert scissor rectangle from OpenGL to Qt (ie, invert y-axis)...
						qpaint_device->height() - scissor_rect->y() - scissor_rect->height(),
						scissor_rect->width(),
						scissor_rect->height());
			}

			// Set the font and colour.
			qpainter->setPen(colour);
			qpainter->setFont(scale_font(font, scale));

			// Get the Qt window coordinates at which to render text.
			const float qt_win_x = x;
			// Note that OpenGL and Qt y-axes are the reverse of each other.
			const float qt_win_y = qpaint_device->height() - y;

			qpainter->drawText(QPointF(qt_win_x, qt_win_y), string);

			// Turn off clipping if it was turned on.
			if (scissor_rect)
			{
				qpainter->setClipRect(QRect(), Qt::NoClip);
			}

			// At scope exit we can resume rendering with 'GLRenderer'...
		}
	}
}

void
GPlatesOpenGL::GLText::render_text_2D(
		GLRenderer &renderer,
		const double &world_x,
		const double &world_y,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font,
		float scale)
{
	render_text_3D(renderer, world_x, world_y, 0.0, string, colour, x_offset, y_offset, font, scale);
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
	const GLViewport &viewport = renderer.gl_get_viewport();
	const GLMatrix &model_view_transform = renderer.gl_get_matrix(GL_MODELVIEW);
	const GLMatrix &projection_transform = renderer.gl_get_matrix(GL_PROJECTION);

	GLdouble win_x, win_y, win_z;
	GLProjectionUtils::glu_project(
			viewport, model_view_transform, projection_transform,
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

	// Delegate to Qt to do the actual rendering of text.
	render_text(renderer, win_x + x_offset, win_y + y_offset, string, colour, font, scale);
}
