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

#ifndef GPLATES_OPENGL_GLTILERENDER_H
#define GPLATES_OPENGL_GLTILERENDER_H

#include <boost/optional.hpp>

#include "GLTransform.h"
#include "GLViewport.h"


namespace GPlatesOpenGL
{
	/**
	 * Used when compositing a destination (image) from a sequence of smaller rendered tiles.
	 *
	 * This is usually needed when the destination image dimensions are larger than the render
	 * target used to draw the image.
	 *
	 * The compositing is achieved by rendering the same scene multiple times (once per tile)
	 * but using a different projection transform each time (to capture a separate tile rectangle
	 * area of the final destination scene).
	 */
	class GLTileRender
	{
	public:

		/**
		 * @a render_target_width and @a render_target_height are the dimensions of the render target
		 * used to render each tile.
		 *
		 * @a destination_viewport is the destination of the final tile-composited image.
		 *
		 * @a border is the number of pixels around the actual tile size to prevent clipping of
		 * wide lines and points. Points are only rendered if their centre is inside the view frustum
		 * and hence points with size greater than one will pop into view unless the view frustum
		 * is expanded to include some border pixels around the tile. A similar problem happens with
		 * wide lines (for pixels near a clipped line vertex). The size of the border required is the
		 * maximum of all the point sizes and line widths divided by two (eg, diameter versus radius).
		 *
		 * The actual tile dimensions are 'render_target_width - 2 * border' and
		 * 'render_target_height - 2 * border'.
		 *
		 * @throws PreconditionViolationError if @a render_target_width is <= '2 * border' or
		 * if @a render_target_height is <= '2 * border'.
		 */
		GLTileRender(
				unsigned int render_target_width,
				unsigned int render_target_height,
				const GLViewport &destination_viewport,
				unsigned int border = 0);


		/**
		 * Returns the maximum render target tile dimensions across all tiles.
		 *
		 * This is the maximum dimensions of all calls to @a get_tile_render_target_viewport.
		 */
		void
		get_max_tile_render_target_dimensions(
				unsigned int &max_tile_render_target_width,
				unsigned int &max_tile_render_target_height) const;


		/**
		 * Starts at the first tile.
		 */
		void
		first_tile();


		/**
		 * Moves to the next tile.
		 *
		 * This should be followed by a call to @a finished to see if the (next) tile is valid.
		 */
		void
		next_tile();


		/**
		 * Returns true if finished iterating over the tiles.
		 */
		bool
		finished() const;


		/**
		 * The projection transform adjustment for the current tile.
		 *
		 * This transform should be pre-multiplied with the actual projection transform used
		 * to render the scene.
		 */
		GLTransform::non_null_ptr_to_const_type
		get_tile_projection_transform() const;


		/**
		 * The viewport that should be specified to 'GLRenderer::gl_viewport()' before rendering
		 * to the current tile (this viewport includes the tile's border pixels).
		 *
		 * Note that, unlike the other viewports, the viewport x and y offsets are always zero here.
		 */
		void
		get_tile_render_target_viewport(
				GLViewport &tile_render_target_viewport) const;


		/**
		 * The viewport containing the actual rendered tile data (excludes the border pixels).
		 *
		 * This is useful when copying or transferring the data in the render target to the destination.
		 */
		void
		get_tile_source_viewport(
				GLViewport &tile_source_viewport) const;


		/**
		 * The viewport in the larger destintation viewport where the current tile's source data
		 * should be copied or tranferred to.
		 *
		 * This is useful when copying or transferring the data in the render target to the destination.
		 */
		void
		get_tile_destination_viewport(
				GLViewport &tile_destination_viewport) const;

	private:

		/**
		 * Holds information for the current tile.
		 */
		struct Tile
		{
			Tile(
					const GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type &projection_,
					const GLViewport &render_target_viewport_,
					const GLViewport &source_viewport_,
					const GLViewport &destination_viewport_);

			GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type projection;
			GLViewport render_target_viewport;
			GLViewport source_viewport;
			GLViewport destination_viewport;
		};


		GLViewport d_destination_viewport;

		unsigned int d_border_x;
		unsigned int d_border_y;

		unsigned int d_max_tile_width;
		unsigned int d_max_tile_height;

		unsigned int d_num_tile_columns;
		unsigned int d_num_tile_rows;

		/**
		 * Index to the current tile.
		 */
		unsigned int d_current_tile_index;

		/**
		 * The current tile's parameters, or boost::none if no current tile.
		 */
		boost::optional<Tile> d_current_tile;


		/**
		 * Create @a d_current_tile associated with the current tile index @a d_current_tile_index.
		 */
		void
		initialise_current_tile();

	};
}

#endif // GPLATES_OPENGL_GLTILERENDER_H
