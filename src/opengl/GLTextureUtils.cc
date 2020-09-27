/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <boost/cstdint.hpp>

#include "GLTextureUtils.h"

#include "GL.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


void
GPlatesOpenGL::GLTextureUtils::load_argb32_qimage_into_rgba8_array(
		const QImage &argb32_qimage,
		boost::scoped_array<GPlatesGui::rgba8_t> &rgba8_array)
{
	PROFILE_FUNC();

	const int image_width = argb32_qimage.width();
	const int image_height = argb32_qimage.height();

	// Allocate the array of RGBA8 pixels.
	rgba8_array.reset(new GPlatesGui::rgba8_t[image_width * image_height]);
	GPlatesGui::rgba8_t *const rgba8_data = rgba8_array.get();
	for (int i = 0; i < image_height; ++i)
	{
		// Convert a row of QImage::Format_ARGB32 pixels to GPlatesGui::rgba8_t.
		convert_argb32_to_rgba8(
				reinterpret_cast<const boost::uint32_t *>(argb32_qimage.scanLine(i)),
				rgba8_data + i * image_width,
				image_width);
	}
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLTextureUtils::create_xy_clip_texture_2D(
		GL &gl)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	GLTexture::shared_ptr_type xy_clip_texture = GLTexture::create(gl);

	gl.BindTexture(GL_TEXTURE_2D, xy_clip_texture);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//
	// The clip texture is a 4x4 image where the centre 2x2 texels are 1.0
	// and the boundary texels are 0.0.
	// We will use the alpha channel for alpha-testing (to discard clipped regions).
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[16] =
	{
		mask_zero, mask_zero, mask_zero, mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_zero, mask_zero, mask_zero
	};

	// Create the texture and load the data into it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);

	return xy_clip_texture;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLTextureUtils::create_z_clip_texture_2D(
		GL &gl)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	GLTexture::shared_ptr_type z_clip_texture = GLTexture::create(gl);

	gl.BindTexture(GL_TEXTURE_2D, z_clip_texture);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//
	// The clip texture is a 2x1 image where the one texel is white and the other black.
	// We will use the alpha channel for alpha-testing (to discard clipped regions).
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[2] = { mask_zero, mask_one };

	// Create the texture and load the data into it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);

	return z_clip_texture;
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()
{
	// Note that the scale is slightly less than 0.25 - this is to avoid seams/gaps between
	// adjacent tiles - this can occur if a screen pixel centre (in render-target) falls right on
	// the tile boundary - in this case slight numerical differences can mean the pixel is just
	// outside the clip zone of both adjacent tiles and hence does not get drawn - this usually
	// only happens when the view is aligned perfectly orthogonally to the tile boundary and this
	// can be the case when GPlates first starts - once the user rotates the view with the mouse
	// it generally isn't noticeable anymore. So the solution is to make the clip regions of
	// adjacent tiles overlap very slightly - here the overlap is 1/2,000th of a texel assuming
	// a 256x256 texel tile so the distortion should be very negligible and undetectable.
	static const double clip_texture_scale = 0.249999;
	static GLMatrix matrix = GLMatrix()
			.gl_translate(0.5, 0.5, 0.0)
			.gl_scale(clip_texture_scale, clip_texture_scale, 1.0);

	return matrix;
}
