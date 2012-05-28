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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERINTERFACE_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERINTERFACE_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLMatrix.h"
#include "GLTexture.h"
#include "GLViewport.h"

#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * Interface for a (possibly reconstructed) multi-resolution raster.
	 *
	 * For example this could be a regular raster or a reconstructed raster.
	 */
	class GLMultiResolutionRasterInterface :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionRasterInterface>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionRasterInterface.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionRasterInterface> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionRasterInterface.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionRasterInterface> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular render of this raster.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		virtual
		~GLMultiResolutionRasterInterface()
		{  }


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		virtual
		const GPlatesUtils::SubjectToken &
		get_subject_token() const = 0;


		/**
		 * Returns the number of levels of detail.
		 *
		 * The highest resolution (original raster) is level 0 and the lowest
		 * resolution level is 'N-1' where 'N' is the number of levels.
		 */
		virtual
		std::size_t
		get_num_levels_of_detail() const = 0;


		/**
		 * Returns the unclamped exact floating-point level-of-detail that theoretically represents
		 * the exact level-of-detail that would be required to fulfill the resolution needs of a
		 * render target (as defined by the specified viewport and view/projection matrices).
		 *
		 * Since tiles are only at integer level-of-detail factors an unclamped floating-point number
		 * is only useful to determine if the current render target is big enough or if it's too big,
		 * ie, if it's less than zero.
		 *
		 * See @a render for a description of @a level_of_detail_bias.
		 * NOTE: @a level_of_detail_bias is simply added to the level-of-detail calculated internally.
		 */
		virtual
		float
		get_level_of_detail(
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform,
				const GLViewport &viewport,
				float level_of_detail_bias = 0.0f) const = 0;


		/**
		 * Given the specified viewport (and model-view/projection matrices) and the desired
		 * level-of-detail this method determines the scale factor that needs to be applied to
		 * @a viewport width and height such that it is sized correctly to contain the resolution
		 * of the desired level-of-detail.
		 *
		 * This is useful if you want to adapt the render-target (viewport) size to an integer
		 * level-of-detail rather than adapt the level-of-detail to the render target size.
		 * Typically the latter is used for visual display while the former is used for
		 * processing floating-point rasters at a user-specified level-of-detail (where the user
		 * specifies an integer level-of-detail simply as a way to control memory usage and speed).
		 *
		 * The new render-target size appropriate for @a level_of_detail should be calculated as:
		 *   new_viewport_dimension = viewport_dimension * returned_scale_factor
		 * ...which should resize it if it's either too big or too small.
		 *
		 * NOTE: @a level_of_detail is a 'float' (not an integer) unlike other methods.
		 * This is to allow adjustment of an integer level-of-detail with a bias.
		 */
		float
		get_viewport_dimension_scale(
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform,
				const GLViewport &viewport,
				float level_of_detail) const;


		/**
		 * Takes an unclamped level-of-detail (see @a get_level_of_detail) and clamps it to lie
		 * within a valid range of levels:
		 *   1) Regular raster:       the range [0, @a get_num_levels_of_detail - 1],
		 *   2) Reconstructed raster: the range [-Infinity, @a get_num_levels_of_detail - 1],
		 *
		 * NOTE: The returned level-of-detail is *signed* because a *reconstructed* raster
		 * can have a negative LOD (useful when reconstructed raster uses an age grid mask that is
		 * higher resolution than the source raster itself).
		 *
		 * NOTE: The returned level-of-detail is a float instead of an integer.
		 * Float can represent clamped integers (up to 23 bits) exactly so returning as float is fine.
		 * Tiles only exist (and hence can only be rendered) at *integer* levels of details.
		 * So conversion to integer is done, for example, when the raster is rendered.
		 * This conversion rounds *down* (including negative numbers, eg, -2.1 becomes -3).
		 */
		virtual
		float
		clamp_level_of_detail(
				float level_of_detail) const = 0;


		/**
		 * Renders all tiles visible in the view frustum (determined by the current viewport and
		 * model-view/projection transforms of @a renderer) and returns true if any tiles were rendered.
		 *
		 * @a cache_handle_type should be kept alive until the next call to @a render.
		 * This is designed purely to take advantage of frame-to-frame coherency.
		 * For example:
		 *
		 *   cache_handle_type my_cached_view;
		 *   // Frame 1...
		 *   my_cached_view = raster->render(...);
		 *   // Frame 2...
		 *   my_cached_view = raster->render(...);
		 *
		 * ...NOTE: In frame 2 'my_cached_view' is overwritten *after* 'render()' is called.
		 * This enables reuse of calculations from frame 1 by keeping them alive during frame 2.
		 *
		 * A positive @a level_of_detail_bias can be used to relax the constraint that
		 * the rendered raster have texels that are less than or equal to the size of a pixel
		 * of the viewport (to avoid blockiness or blurriness of the rendered raster).
		 * The @a level_of_detail_bias is a log2 so 1.0 means use a level of detail texture
		 * that requires half the resolution (eg, 256x256 instead of 512x512) of what would
		 * normally be used if a LOD bias of zero were used, and 2.0 means a quarter
		 * (eg, 128x128 instead of 512x512). Fractional values are allowed (and more useful).
		 *
		 * The framebuffer colour buffer format should typically match the texture format of this
		 * raster. The two main examples are:
		 *   1) visual display of raster: an RGBA fixed-point raster rendered to the main framebuffer,
		 *   2) data analysis: a data-value/data-coverage floating-point raster rendered to
		 *      a floating-point texture attached to a framebuffer object.
		 * NOTE: It is possible to render a fixed-point raster to a floating-point colour buffer
		 * or vice versa but there's no real need or use for that.
		 */
		bool
		render(
				GLRenderer &renderer,
				cache_handle_type &cache_handle,
				float level_of_detail_bias = 0.0f);


		/**
		 * Renders all tiles visible in the view frustum (determined by the current model-view/projection
		 * transforms of @a renderer) and returns true if any tiles were rendered.
		 *
		 * This differs from the above @a render method in that the current viewport is *not* used
		 * to determine the level-of-detail (because the level-of-detail is explicitly provided).
		 *
		 * NOTE: @a level_of_detail is a float - see @a clamp_level_of_detail for details.
		 *
		 * See the first overload of @a render for more details.
		 *
		 * Throws exception if @a level_of_detail is outside the valid range.
		 * Use @a clamp_level_of_detail to clamp to a valid range before calling this method.
		 */
		virtual
		bool
		render(
				GLRenderer &renderer,
				float level_of_detail,
				cache_handle_type &cache_handle) = 0;
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERINTERFACE_H
