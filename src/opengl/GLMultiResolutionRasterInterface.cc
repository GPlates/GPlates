/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <cmath>

#include "GLMultiResolutionRasterInterface.h"

#include "GLRenderer.h"


float
GPlatesOpenGL::GLMultiResolutionRasterInterface::get_viewport_dimension_scale(
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		const GLViewport &viewport,
		float level_of_detail) const
{
	const float current_level_of_detail = get_level_of_detail(model_view_transform, projection_transform, viewport);

	// new_viewport_dimension = viewport_dimension * pow(2, current_level_of_detail - level_of_detail)
	return std::pow(2.0f, current_level_of_detail - level_of_detail);
}


bool
GPlatesOpenGL::GLMultiResolutionRasterInterface::render(
		GLRenderer &renderer,
		cache_handle_type &cache_handle,
		float level_of_detail_bias)
{
	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	const int level_of_detail =
			clamp_level_of_detail(
					get_level_of_detail(
							renderer.gl_get_matrix(GL_MODELVIEW),
							renderer.gl_get_matrix(GL_PROJECTION),
							renderer.gl_get_viewport(),
							level_of_detail_bias));

	return render(renderer, level_of_detail, cache_handle);
}
