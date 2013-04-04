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
		 * wide lines and points. Fat points are only rendered if their centre is inside the view frustum
		 * and hence points with size greater than one will suddently pop into view unless the view frustum
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
		 * Returns the maximum render target tile width across all tiles.
		 *
		 * This is the maximum of all calls to @a get_tile_render_target_scissor_rectangle.
		 */
		unsigned int
		get_max_tile_render_target_width() const
		{
			return d_max_tile_width;
		}

		/**
		 * Returns the maximum render target tile height across all tiles.
		 *
		 * This is the maximum of all calls to @a get_tile_render_target_scissor_rectangle.
		 */
		unsigned int
		get_max_tile_render_target_height() const
		{
			return d_max_tile_height;
		}


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
		 *
		 * This transform adjusts the regular scene's view frustum such that it covers only the
		 * current tile portion of the scene. This transform also includes the adjustments for
		 * the tile's border pixels (if any).
		 */
		GLTransform::non_null_ptr_to_const_type
		get_tile_projection_transform() const;


		/**
		 * The viewport that should be specified to 'GLRenderer::gl_viewport()' before rendering
		 * to the current tile (this viewport includes the tile's border pixels).
		 *
		 * Note that if there are border pixels then the viewport is larger than the source tile.
		 * This enables fat points and wide lines just outside the tile region to rasterize pixels
		 * within the tile region.
		 * Also note that the viewport can go outside the render target bounds (eg, has negative
		 * viewport x and y offsets). The viewport does not clip (that's what the projection transform)
		 * is for - the viewport is only a transformation of Normalised Device Coordinates
		 * (in the range [-1,1]) to window coordinates. Also note that since the projection transform
		 * includes the border it also does not clip away the border pixels. It is the scissor rectangle
		 * that clips away the border pixels (if the tile region is actually smaller than the render target).
		 */
		void
		get_tile_render_target_viewport(
				GLViewport &tile_render_target_viewport) const;


		/**
		 * The scissor rectangle that should be specified to 'GLRenderer::gl_scissor()' before rendering
		 * to the current tile (this rectangle excludes the tile's border pixels).
		 *
		 * NOTE: You *must* specify a scissor rectangle (see 'GLRenderer::gl_scissor()') otherwise
		 * fat points and wide lines can render to pixels outside the tile (scissor) region.
		 * This only really matters if the tile region is smaller than the render target.
		 */
		void
		get_tile_render_target_scissor_rectangle(
				GLViewport &tile_render_target_scissor_rect) const
		{
			get_tile_source_viewport(tile_render_target_scissor_rect);
		}


		/**
		 * The viewport containing the actual rendered tile data (excludes the border pixels).
		 *
		 * This is useful when copying or transferring the data in the render target to the destination.
		 */
		void
		get_tile_source_viewport(
				GLViewport &tile_source_viewport) const;


		/**
		 * The viewport in the larger destination viewport where the current tile's source data
		 * should be copied or transferred to.
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

		unsigned int d_border;

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
