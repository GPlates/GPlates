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
 
#ifndef GPLATES_OPENGL_GLTEXTUREUTILS_H
#define GPLATES_OPENGL_GLTEXTUREUTILS_H

#include <boost/scoped_array.hpp>
#include <QImage>

#include "GLMatrix.h"
#include "GLTexture.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	class GL;

	namespace GLTextureUtils
	{
		/**
		 * Loads the specified QImage, must be QImage::Format_ARGB32 into @a rgba8_array.
		 *
		 * @a rgba8_array is allocated to fit the image.
		 */
		void
		load_argb32_qimage_into_rgba8_array(
				const QImage &argb32_qimage,
				boost::scoped_array<GPlatesGui::rgba8_t> &rgba8_array);


		/**
		 * Creates a new 4x4 texel clip texture whose centre 2x2 texels are white with the
		 * remaining texels black (including alpha channel).
		 */
		GLTexture::shared_ptr_type
		create_xy_clip_texture_2D(
				GL &gl);


		/**
		 * Creates a new 2x1 texel clip texture whose first texel is black and second texel white (including alpha channel).
		 *
		 * NOTE: The created texture is actually a 2D texture and *not* a 1D texture.
		 */
		GLTexture::shared_ptr_type
		create_z_clip_texture_2D(
				GL &gl);


		/**
		 * Initialise clip texture transform to convert the clip-space range [-1, 1] to
		 * range [0.25, 0.75] to map to the interior 2x2 texel region of the 4x4 clip texture.
		 */
		const GLMatrix &
		get_clip_texture_clip_space_to_texture_space_transform();
	}
}

#endif // GPLATES_OPENGL_GLTEXTUREUTILS_H
