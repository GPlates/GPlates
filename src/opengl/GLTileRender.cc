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

#include <boost/cast.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "GLTileRender.h"

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLTileRender::GLTileRender(
		unsigned int render_target_width,
		unsigned int render_target_height,
		const GLViewport &destination_viewport,
		unsigned int border) :
	d_destination_viewport(destination_viewport),
	d_border(border),
	d_current_tile_index(0)
{
	// We want a non-zero tile width and height.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			render_target_width > 0 && render_target_height > 0,
			GPLATES_ASSERTION_SOURCE);

	// The maximum tile dimensions are the smaller of the dimensions of the render target and destination viewport.
	d_max_tile_width = (render_target_width >= boost::numeric_cast<unsigned int>(d_destination_viewport.width()))
			? d_destination_viewport.width()
			: render_target_width;
	d_max_tile_height = (render_target_height >= boost::numeric_cast<unsigned int>(d_destination_viewport.height()))
			? d_destination_viewport.height()
			: render_target_height;

	d_num_tile_columns = (d_destination_viewport.width() / d_max_tile_width) +
			((d_destination_viewport.width() % d_max_tile_width) ? 1 : 0);
	d_num_tile_rows = (d_destination_viewport.height() / d_max_tile_height) +
			((d_destination_viewport.height() % d_max_tile_height) ? 1 : 0);
}


void
GPlatesOpenGL::GLTileRender::first_tile()
{
	d_current_tile_index = 0;

	initialise_current_tile();
}


void
GPlatesOpenGL::GLTileRender::next_tile()
{
	// Move to the next tile.
	++d_current_tile_index;

	// If we were at the last tile then indicate that we are finished.
	if (d_current_tile_index == d_num_tile_rows * d_num_tile_columns)
	{
		d_current_tile_index = 0;
		d_current_tile = boost::none;
		return;
	}

	initialise_current_tile();
}


bool
GPlatesOpenGL::GLTileRender::finished() const
{
	return !d_current_tile;
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLTileRender::get_tile_projection_transform() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_tile,
			GPLATES_ASSERTION_SOURCE);

	return d_current_tile->projection;
}


void
GPlatesOpenGL::GLTileRender::get_tile_render_target_viewport(
		GLViewport &tile_render_target_viewport) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_tile,
			GPLATES_ASSERTION_SOURCE);

	tile_render_target_viewport = d_current_tile->render_target_viewport;
}


void
GPlatesOpenGL::GLTileRender::get_tile_source_viewport(
		GLViewport &tile_source_viewport) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_tile,
			GPLATES_ASSERTION_SOURCE);

	tile_source_viewport = d_current_tile->source_viewport;
}


void
GPlatesOpenGL::GLTileRender::get_tile_destination_viewport(
		GLViewport &tile_destination_viewport) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_tile,
			GPLATES_ASSERTION_SOURCE);

	tile_destination_viewport = d_current_tile->destination_viewport;
}


void
GPlatesOpenGL::GLTileRender::initialise_current_tile()
{
	const unsigned int current_tile_column = d_current_tile_index % d_num_tile_columns;
	const unsigned int current_tile_row = d_current_tile_index / d_num_tile_columns;

	unsigned int current_tile_width;
	unsigned int current_tile_height;

	if (current_tile_column < d_num_tile_columns - 1)
	{
		current_tile_width = d_max_tile_width;
	}
	else // The tile is at the right boundary...
	{
		current_tile_width = d_destination_viewport.width() - (d_num_tile_columns - 1) * d_max_tile_width;
	}

	if (current_tile_row < d_num_tile_rows - 1)
	{
		current_tile_height = d_max_tile_height;
	}
	else // The tile is at the right boundary...
	{
		current_tile_height = d_destination_viewport.height() - (d_num_tile_rows - 1) * d_max_tile_height;
	}

	// The render target viewport includes the border pixels.
	//
	// NOTE: It's fine for the viewport to go outside the render target (eg, negative x and y offsets).
	// The viewport is just a window coordinate transform - it doesn't clip.
	const GLViewport current_tile_render_target_viewport(
			-static_cast<int>(d_border),
			-static_cast<int>(d_border),
			current_tile_width + 2 * d_border,
			current_tile_height + 2 * d_border);

	// The source viewport is the centre section of the render target viewport (ie, minus the border).
	const GLViewport current_tile_source_viewport(
			0,
			0,
			current_tile_width,
			current_tile_height);

	// Place the current tile in the correct location within the destination viewport.
	const GLViewport current_tile_destination_viewport(
			d_destination_viewport.x() + current_tile_column * d_max_tile_width,
			d_destination_viewport.y() + current_tile_row * d_max_tile_height,
			current_tile_width,
			current_tile_height);

	//
	// See http://www.opengl.org/archives/resources/code/samples/sig99/advanced99/notes/node30.html
	// for an explanation of the projection transform scaling and translating.
	//

	double tile_projection_scale_x = double(d_destination_viewport.width()) / current_tile_width;
	double tile_projection_scale_y = double(d_destination_viewport.height()) / current_tile_height;

	// Factor the border into the scaling.
	// The render target includes border pixels so adjust the projection transform accordingly.
	tile_projection_scale_x *= double(current_tile_width) / (current_tile_width + 2 * d_border);
	tile_projection_scale_y *= double(current_tile_height) / (current_tile_height + 2 * d_border);

	const double tile_centre_x = current_tile_column * d_max_tile_width + 0.5 * current_tile_width;
	const double tile_centre_y = current_tile_row * d_max_tile_height + 0.5 * current_tile_height;

	const double tile_projection_translate_x = 1 - 2.0 * tile_centre_x / d_destination_viewport.width();
	const double tile_projection_translate_y = 1 - 2.0 * tile_centre_y / d_destination_viewport.height();

	// Start off with an identify projection matrix.
	GLTransform::non_null_ptr_type projection = GLTransform::create();
	GLMatrix &projection_matrix = projection->get_matrix();

	// Scale the view volume of the scene such that only the current tile fills NDC space (-1,1)
	// instead of the entire scene..
	projection_matrix.gl_scale(tile_projection_scale_x, tile_projection_scale_y, 1);

	// Translate the tile so that it is centred about the z axis - the centre of NDC space (-1,1).
	projection_matrix.gl_translate(tile_projection_translate_x, tile_projection_translate_y, 0);

	// Create the current tile.
	d_current_tile = boost::in_place(
			projection,
			current_tile_render_target_viewport,
			current_tile_source_viewport,
			current_tile_destination_viewport);
}


GPlatesOpenGL::GLTileRender::Tile::Tile(
		const GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type &projection_,
		const GLViewport &render_target_viewport_,
		const GLViewport &source_viewport_,
		const GLViewport &destination_viewport_) :
	projection(projection_),
	render_target_viewport(render_target_viewport_),
	source_viewport(source_viewport_),
	destination_viewport(destination_viewport_)
{
}
