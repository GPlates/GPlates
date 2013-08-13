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

#include <boost/cstdint.hpp>
#include <QDebug>
#include <QPainter>
#include <opengl/OpenGL.h>

#include "GLImageUtils.h"

#include "GLBuffer.h"
#include "GLContext.h"
#include "GLPixelBuffer.h"
#include "GLRenderer.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


void
GPlatesOpenGL::GLImageUtils::copy_rgba8_frame_buffer_into_argb32_qimage(
		GLRenderer &renderer,
		QImage &image,
		const GLViewport &source_viewport,
		const GLViewport &destination_viewport)
{
	// Make sure image is a format we're expecting.
	// Restrict to Format_ARGB32 and Format_ARGB32_Premultiplied since only those two are supported
	// when rendering to a QImage using a QPainter.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			image.format() == QImage::Format_ARGB32 ||
				image.format() == QImage::Format_ARGB32_Premultiplied,
			GPLATES_ASSERTION_SOURCE);

	// Acquire a pixel buffer to read the framebuffer pixels into.
	GLPixelBuffer::shared_ptr_type pixel_buffer =
			renderer.get_context().get_shared_state()->acquire_pixel_buffer(
					renderer,
					4/*RGBA8*/
							// We use power-of-two dimensions since (due to the finite number of
							// power-of-two dimensions) we have more chance of re-using a pixel buffer...
							* GPlatesUtils::Base2::next_power_of_two(source_viewport.width())
							* GPlatesUtils::Base2::next_power_of_two(source_viewport.height()),
					GLBuffer::USAGE_STREAM_READ);

	// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
	pixel_buffer->gl_bind_pack(renderer);

	// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
	// since our data is RGBA (already 4-byte aligned).
	pixel_buffer->gl_read_pixels(
			renderer,
			source_viewport.x(),
			source_viewport.y(),
			source_viewport.width(),
			source_viewport.height(),
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			0);

	// Map the pixel buffer to access its data.
	GLBuffer::MapBufferScope map_pixel_buffer_scope(
			renderer,
			*pixel_buffer->get_buffer(),
			GLBuffer::TARGET_PIXEL_PACK_BUFFER);

	// Map the pixel buffer data.
	const void *const pixel_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);
	const GPlatesGui::rgba8_t *const rgba8_pixel_data = static_cast<const GPlatesGui::rgba8_t *>(pixel_data);

	// Iterate over the pixel lines in the rendered tile and copy into a sub-rect of the image.
	for (GLsizei tile_y = 0; tile_y < source_viewport.height(); ++tile_y)
	{
		// Source pixel data.
		const GPlatesGui::rgba8_t *src_pixel_data = rgba8_pixel_data + tile_y * source_viewport.width();

		// Destination pixel data.
		const int dst_y = destination_viewport.y() + tile_y;
		boost::uint32_t *dst_pixel_data =
				reinterpret_cast<boost::uint32_t *>(
						// Note that OpenGL and Qt y-axes are the reverse of each other...
						image.scanLine(image.height()-1 - dst_y)) +
						destination_viewport.x();

		// Convert the current line to the QImage::Format_ARGB32 format supported by QImage.
		GPlatesGui::convert_rgba8_to_argb32(src_pixel_data, dst_pixel_data, source_viewport.width());
	}

	const bool unmap_success = map_pixel_buffer_scope.gl_unmap_buffer();
	if (!unmap_success)
	{
		// FIXME: This should probably be an exception, but a thrown exception will need to be handled.
		qWarning() << "GlobeCanvas::render_scene_tile_into_image: error retrieving image tile.";
	}
}


QImage
GPlatesOpenGL::GLImageUtils::draw_text_into_qimage(
		const QString &text,
		unsigned int image_width,
		unsigned int image_height,
		const float text_scale,
		const QColor &text_colour,
		const QColor &background_colour)
{
	PROFILE_FUNC();

	// Start off with half-size dimensions - we'll scale to full-size later
	// so that image is more visible (because image will map roughly one texel to one
	// screen pixel which can be hard to read).

	const int scaled_width = static_cast<int>(image_width / text_scale);
	const int scaled_height = static_cast<int>(image_height / text_scale);

	QImage scaled_image(scaled_width, scaled_height, QImage::Format_ARGB32);

	QPainter painter(&scaled_image);
	// Draw filled background
	painter.fillRect(QRect(0, 0, scaled_width, scaled_height), background_colour);
	painter.setPen(text_colour);
	painter.drawText(
			0, 0,
			scaled_width, scaled_height,
			(Qt::AlignCenter | Qt::TextWordWrap),
			text);
	painter.end();

	// Scale the rendered text.
	return scaled_image.scaled(
			image_width, image_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
