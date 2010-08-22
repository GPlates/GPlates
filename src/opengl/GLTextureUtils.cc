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

#include <boost/scoped_array.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLTextureUtils.h"

#include "GLUtils.h"


void
GPlatesOpenGL::GLTextureUtils::load_colour_into_texture(
		const GLTexture::shared_ptr_type &texture,
		const GPlatesGui::rgba8_t &colour,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-colour entries.
	boost::scoped_array<GPlatesGui::rgba8_t> image_data_storage(
			new GPlatesGui::rgba8_t[num_texels_to_load]);
	GPlatesGui::rgba8_t *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[n] = colour;
	}

	// Load image into texture...
	load_rgba8_image_into_texture(
			texture,
			image_data_storage.get(),
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::load_rgba8_image_into_texture(
		const GLTexture::shared_ptr_type &texture,
		const GPlatesGui::rgba8_t *image,
		unsigned int image_width,
		unsigned int image_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{

	// Each row of texels, in the raster image, is not aligned to 4 bytes.
	// This is a direct call to OpenGL but it only affects how images are unpacked
	// from CPU memory so its really a client side state (rather than a graphics card state).
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	texture->gl_bind_texture(GL_TEXTURE_2D);

	// The client has ensured that the texture has been created in OpenGL (eg, by using
	// glTexImage2D) so we can use the faster glTexSubImage2D that doesn't recreate the texture.
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			texel_u_offset, texel_v_offset, image_width, image_height,
			GL_RGBA, GL_UNSIGNED_BYTE, image);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}
