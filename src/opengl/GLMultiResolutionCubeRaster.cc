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

#include <cmath>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>

#include "GLMultiResolutionCubeRaster.h"

#include "GL.h"
#include "GLContext.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLViewport.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLMultiResolutionCubeRaster::GLMultiResolutionCubeRaster(
		GL &gl,
		const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
		unsigned int tile_texel_dimension,
		bool adapt_tile_dimension_to_source_resolution,
		CacheTileTexturesType cache_tile_textures) :
	d_multi_resolution_raster(multi_resolution_raster),
	d_tile_texel_dimension(tile_texel_dimension),
	// Start with small size cache and just let the cache grow in size as needed (if caching enabled)...
	d_texture_cache(tile_texture_cache_type::create(2/* GPU pipeline breathing room in case caching disabled*/)),
	d_cache_tile_textures(cache_tile_textures),
	d_tile_framebuffer(GLFramebuffer::create(gl)),
	d_have_checked_tile_framebuffer_completeness(false),
	d_cube_quad_tree(cube_quad_tree_type::create()),
	d_num_source_levels_of_detail_used(1)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Adjust the tile dimension to the source raster resolution if non-power-of-two textures are supported.
	// The number of levels of detail returned might not be all levels of the source raster -
	// just the ones used by this cube map raster (the lowest resolutions might get left off).
	adjust_tile_texel_dimension(adapt_tile_dimension_to_source_resolution, capabilities);

	// Make sure the, possibly adapted, tile dimension does not exceed the maximum texture size...
	if (d_tile_texel_dimension > capabilities.gl_max_texture_size)
	{
		d_tile_texel_dimension = capabilities.gl_max_texture_size;
	}

	initialise_cube_quad_trees();

	// If the client has requested the entire cube quad tree be cached...
	if (d_cache_tile_textures == CACHE_TILE_TEXTURES_ENTIRE_CUBE_QUAD_TREE)
	{
		// This does not consume memory until each individual tile is requested.
		// This effectively disables any recycling that would otherwise happen in the cache.
		d_texture_cache->set_min_num_objects(d_cube_quad_tree->size());
	}
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::adjust_tile_texel_dimension(
		bool adapt_tile_dimension_to_source_resolution,
		const GLCapabilities &capabilities)
{
	// We don't worry about half-texel expansion of the projection frustums here because
	// we just need to determine viewport dimensions. There will be a slight error by neglecting
	// the half texel but it's already an approximation anyway.
	// Besides, the half texel depends on the tile texel dimension and we're going to change that below.
	GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision = GLCubeSubdivision::create();

	// Get the projection transforms of an entire cube face (the lowest resolution level-of-detail).
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision->get_projection_transform(
					0/*level_of_detail*/, 0/*tile_u_offset*/, 0/*tile_v_offset*/);

	// Get the view transform - it doesn't matter which cube face we choose because, although
	// the view transform are different, it won't matter to us since we're projecting onto
	// a spherical globe from its centre and all faces project the same way.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision->get_view_transform(
					GPlatesMaths::CubeCoordinateFrame::POSITIVE_X);

	const GLViewProjection view_projection(
			// Start off with a tile-sized viewport - we'll adjust its width and height shortly...
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension),
			view_transform->get_matrix(),
			projection_transform->get_matrix());

	// Determine the scale factor for our viewport dimensions required to capture the resolution
	// of the highest level of detail (level 0) of the source raster into an entire cube face.
	double viewport_dimension_scale = d_multi_resolution_raster->get_viewport_dimension_scale(
			view_projection,
			0/*level_of_detail*/);

	// The source raster level-of-detail (and hence viewport dimension scale) is determined such
	// that a pixel on the globe covers no more than one pixel in the cube map. However the
	// variation in cube map projection from face centre to face corner is approximately
	// a factor of two (or one level-of-detail difference). This means two pixels on the globe
	// can fit into one pixel in the cube map at a face centre. This is fine for each level-of-detail
	// because it ensures plenty enough detail is rendered into a cube map tile. However it does
	// mean that we can get one more resolution level than expected. This means we can zoom further
	// into the globe before it reaches full resolution and starts to get blurry.
	// The factor is sqrt(3) * (1 / cos(A)); where sin(A) = (1 / sqrt(3)).
	// This is the same as 3 / sqrt(2).
	// The sqrt(3) is length of cube half-diagonal (divided by unit-length globe radius).
	// The cos(A) is a 35 degree angle between the cube face and globe tangent plane at cube corner
	// (globe tangent calculated at position on globe that cube corner projects onto).
	// This factor is how much a pixel on the globe expands in size when projected to a pixel on the
	// cube face at its corner (and is close to a factor of two).
	viewport_dimension_scale *= 3.0 / std::sqrt(2.0);

	// Clamp the viewport dimension scale to a maximum value in case the entire raster is georeferenced
	// to a ridiculously tiny region for some reason (eg, the raster's projection transform went bad).
	// This can prevent a crash due to 32-bit integer overflow caused by a quad tree depth of 32
	// or greater (the tile indices range up to 2^32). However we make it lower than 32 to avoid
	// excessive CPU and memory building too deep a quad tree.
	const double MAX_VIEWPORT_DIMENSION_SCALE = std::exp(std::log(2.0) * 25.0/*max log2_viewport_dimension_scale*/);
	if (viewport_dimension_scale > MAX_VIEWPORT_DIMENSION_SCALE)
	{
		viewport_dimension_scale = MAX_VIEWPORT_DIMENSION_SCALE;
	}

	// Since our cube quad tree is an *integer* power-of-two subdivision of tiles find out the
	// *non-integer* power-of-two scale factor - next we'll adjust this so it becomes an *integer*
	// power-of-two (by adjusting the tile texel dimension to be a non-power-of-two).
	double log2_viewport_dimension_scale = std::log(viewport_dimension_scale) / std::log(2.0);

	// We can have non-power-of-two texture dimensions but we've been asked not to adapt the tile dimension.
	if (!adapt_tile_dimension_to_source_resolution)
	{
		if (log2_viewport_dimension_scale < 0)
		{
			// Only one level of detail needed.
			d_num_source_levels_of_detail_used = 1;
			return;
		}

		// Return number of levels of detail used.
		// The first '+1' rounds up to the next integer level-of-detail.
		// The second '+1' converts from integer level-of-detail to number of levels of detail.
		d_num_source_levels_of_detail_used = static_cast<int>(log2_viewport_dimension_scale + 1) + 1;

		return;
	}

	// If the new viewport dimension is less than the original tile dimension then there will only
	// be one level (the root) in the cube quad tree.
	if (log2_viewport_dimension_scale < 0)
	{
		// Scale the viewport dimension down and round up to the nearest integer.
		// The scale is in the range (0,1).
		d_tile_texel_dimension = static_cast<int>(viewport_dimension_scale * d_tile_texel_dimension + 1);

		// Only one level of detail needed.
		d_num_source_levels_of_detail_used = 1;
		return;
	}
	// The viewport dimension scale is greater than 1...

	// We want to keep the tile texel dimension in the range:
	//
	//    [d_tile_texel_dimension, 2 * d_tile_texel_dimension)
	//
	// ...just because that's a reasonable range that doesn't deviate too much from the
	// client's requested size and provides close to the desired granularity of tiles.
	//
	// Remove the *integer* power-of-two part of the scale factor.
	// That will get taken care of by our power-of-two recursion into the cube quad tree.
	double log2_viewport_dimension_scale_int;
	const double log2_viewport_dimension_scale_fract =
			std::modf(log2_viewport_dimension_scale, &log2_viewport_dimension_scale_int);

	// This scale factor is now in the range [1, 2).
	const double tile_texel_dimension_scale = std::pow(2.0, log2_viewport_dimension_scale_fract);

	// Increase the tile texel dimension from a power-of-two value to a non-power-of-two value.
	d_tile_texel_dimension = static_cast<int>(tile_texel_dimension_scale * d_tile_texel_dimension + 1);

	// Return number of levels of detail used.
	// Convert to integer (it's already integral - the 0.5 is just to avoid numerical precision issues).
	// The '+1' converts from integer level-of-detail to number of levels of detail.
	d_num_source_levels_of_detail_used = static_cast<int>(log2_viewport_dimension_scale_int + 0.5) + 1;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::initialise_cube_quad_trees()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_source_levels_of_detail_used > 0,
			GPLATES_ASSERTION_SOURCE);

	// Create a projection transforms cube quad tree cache so we can determine the view frustum
	// to render each spatial partition node into.
	// We're only visiting each subdivision node once so there's no need for caching.
	//
	// Expand the tile frustums by half a texel around the border of each frustum.
	// This causes the texel centres of the border tile texels to fall right on the edge
	// of the unmodified frustum which means adjacent tiles will have the same colour after
	// bilinear filtering and hence there will be no visible colour seams (or discontinuities
	// in the raster data if the source raster is floating-point).
	// The nice thing is this works for both bilinear filtering and nearest neighbour filtering
	// (ie, there'll be no visible seams in nearest neighbour filtering either).
	cube_subdivision_cache_type::non_null_ptr_type
			cube_subdivision_cache =
					cube_subdivision_cache_type::create(
							GLCubeSubdivision::create(
									GLCubeSubdivision::get_expand_frustum_ratio(
											d_tile_texel_dimension,
											0.5/* half a texel */)));

	for (unsigned int face = 0; face < 6; ++face)
	{
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The viewport is the same for all subdivisions since they use the same tile texture dimension.
		const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

		// Get the cube subdivision cache root node.
		const cube_subdivision_cache_type::node_reference_type
				cube_subdivision_cache_root_node =
						cube_subdivision_cache->get_quad_tree_root_node(cube_face);

		// Recursively generate a quad tree for the current cube face.
		boost::optional<cube_quad_tree_type::node_type::ptr_type> quad_tree_root_node =
				create_quad_tree_node(
						viewport,
						*cube_subdivision_cache,
						cube_subdivision_cache_root_node,
						// Start out at the lowest resolution level of detail supported by
						// the root level of our quad tree...
						d_num_source_levels_of_detail_used - 1/*source_level_of_detail*/);

		// If the source raster covered any part of the current cube face.
		if (quad_tree_root_node)
		{
			d_cube_quad_tree->set_quad_tree_root_node(cube_face, quad_tree_root_node.get());
		}
	}
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::cube_quad_tree_type::node_type::ptr_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_quad_tree_node(
		const GLViewport &viewport,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		const unsigned int source_level_of_detail)
{
	//
	// Setup the model-view and projection matrices and setup the viewport.
	// These are required when the source raster looks for tiles visible in the view frustum
	// of our quad tree subdivision cell (the viewport is used to determine the correct
	// level-of-detail tiles to use).
	//

	// Set the view transform.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(cube_subdivision_cache_node);

	// Multiply the current world transform into the model-view matrix.
	GLMatrix world_model_view_transform_matrix(view_transform->get_matrix());
	world_model_view_transform_matrix.gl_mult_matrix(d_world_transform);
	const GLTransform::non_null_ptr_to_const_type world_model_view_transform =
			GLTransform::create(world_model_view_transform_matrix);

	// Set the projection transform.
	// Get the expanded tile frustum (by half a texel around the border of the frustum).
	// This causes the texel centres of the border tile texels to fall right on the edge
	// of the unmodified frustum which means adjacent tiles will have the same colour after
	// bilinear filtering and hence there will be no visible colour seams (or discontinuities
	// in the raster data if the source raster is floating-point).
	const GLTransform::non_null_ptr_to_const_type half_texel_expanded_projection_transform =
			cube_subdivision_cache.get_projection_transform(
					cube_subdivision_cache_node);

	const GLViewProjection view_projection(
			viewport,
			world_model_view_transform_matrix,
			half_texel_expanded_projection_transform->get_matrix());

	// Determine the source raster level-of-detail from the current viewport and view/projection matrices.
	// Only to see if we can use a lower resolution than pre-calculated.
	// Level-of-detail will get clamped such that it's ">= 0" (and hence can be represented as unsigned).
	unsigned int tile_level_of_detail = boost::numeric_cast<unsigned int>(
			d_multi_resolution_raster->clamp_level_of_detail(
					d_multi_resolution_raster->get_level_of_detail(view_projection)));
	// If we can use a lower-resolution level-of-detail just for this tile then might as well
	// otherwise just use the pre-calculated level-of-detail.
	// If the tile texel dimension has been adjusted to the optimal non-power-of-two value
	// then this is not likely to happen.
	// However if we're forced to use power-of-two dimensions then, as we approach tiles near
	// the centre of the cube face, we might be able to use lower-resolution level-of-details
	// due to the texel resolution of those tiles being lower due to the cube map projection.
	//
	// However don't go to a higher resolution than pre-calculated - this might happen due to numerical
	// precision and the fact that the viewport dimensions have been adjusted to give almost
	// exactly integer level-of-detail values (and might get level-of-detail slightly less than
	// the integer which would get truncated to the next higher resolution LOD).
	if (tile_level_of_detail < source_level_of_detail)
	{
		// Need to clamp because 'source_level_of_detail' might be a lower resolution than
		// supported by the source raster.
		tile_level_of_detail = boost::numeric_cast<unsigned int>(
				d_multi_resolution_raster->clamp_level_of_detail(source_level_of_detail));
	}

	// Get the source tiles that are visible in the current view frustum.
	std::vector<GLMultiResolutionRaster::tile_handle_type> source_raster_tile_handles;
	d_multi_resolution_raster->get_visible_tiles(
			source_raster_tile_handles,
			view_projection.get_view_projection_transform(),
			tile_level_of_detail);

	// If there are no tiles it means the source raster does not have global extents
	// and we are looking at a part of the globe not covered by it.
	if (source_raster_tile_handles.empty())
	{
		return boost::none;
	}

	// Create a quad tree node.
	cube_quad_tree_type::node_type::ptr_type quad_tree_node =
			d_cube_quad_tree->create_node(
					CubeQuadTreeNode(
							tile_level_of_detail,
							world_model_view_transform,
							half_texel_expanded_projection_transform,
							d_texture_cache->allocate_volatile_object()));

	// Initialise the quad tree node's source tiles.
	quad_tree_node->get_element().d_src_raster_tiles.swap(source_raster_tile_handles);

	// If we have reached the highest resolution level of detail in the source raster
	// then we don't need to create any child quad tree nodes.
	if (source_level_of_detail == 0)
	{
		return quad_tree_node;
	}


	//
	// Iterate over the child subdivision regions and create if cover source raster.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// Get the project transforms child quad tree node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Returns a non-null child if child quad tree node is covered by source raster.
			const boost::optional<cube_quad_tree_type::node_type::ptr_type> child_node =
					create_quad_tree_node(
							viewport,
							cube_subdivision_cache,
							child_cube_subdivision_cache_node,
							// Child is at a higher resolution level-of-detail...
							source_level_of_detail - 1);

			// Add the child node if one was created.
			if (child_node)
			{
				// Add the node as a child.
				d_cube_quad_tree->set_child_node(
						*quad_tree_node, child_u_offset, child_v_offset, child_node.get());

				// Since we've given the quad tree node a child it is now an internal node.
				quad_tree_node->get_element().d_is_leaf_node = false;
			}
		}
	}

	return quad_tree_node;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::set_world_transform(
		const GLMatrix &world_transform)
{
	// If the world transform has changed then set it, and re-create our quad tree of tiles
	// (just the tiles, their textures will be re-generated as needed).
	if (d_world_transform != world_transform)
	{
		d_world_transform = world_transform;

		// For example, the cube map is rotated to align with the central meridian of the map projection.
		//
		// This can have two effects:
		//   1) The tile textures are no longer valid, and
		//   2) For regional rasters (that don't cover the entire globe) some quad-tree nodes will be NULL
		//      where the tile does not cover raster data. As the cube map rotates, each cube face quad tree
		//      also rotates and different regions of the raster are covered resulting in the branching
		//      pattern in each quad tree changing.
		//
		// As a result of (2) it's best to clear the cube map quad trees and re-generate them in the new
		// rotated cube map position. This also has the by-product of invalidating all the tile textures.
		// Note that the tile 'textures' are not re-generated here (only regenerated in 'get_tile_texture()').
		d_cube_quad_tree->clear();
		initialise_cube_quad_trees();

		// Let any clients know that they're now out-of-date (since our cube map texture has a new orientation).
		d_subject_token.invalidate();
	}
}


const GPlatesUtils::SubjectToken &
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_subject_token() const
{
	//
	// This covers changes to the inputs that don't require completely re-creating the inputs.
	// That is beyond our scope and is detected and managed by our owners (and owners of our inputs).
	//

	// If the source raster has changed.
	if (!d_multi_resolution_raster->get_subject_token().is_observer_up_to_date(
				d_multi_resolution_raster_observer_token))
	{
		d_subject_token.invalidate();

		d_multi_resolution_raster->get_subject_token().update_observer(
				d_multi_resolution_raster_observer_token);
	}

	return d_subject_token;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_tile_texture(
		GL &gl,
		const CubeQuadTreeNode &tile,
		cache_handle_type &cache_handle)
{
	//
	// Get the texture for the tile.
	//

	// See if we've generated our tile texture and
	// see if it hasn't been recycled by the texture cache.
	tile_texture_cache_type::object_shared_ptr_type tile_texture = tile.d_tile_texture->get_cached_object();
	if (!tile_texture)
	{
		tile_texture = tile.d_tile_texture->recycle_an_unused_object();
		if (!tile_texture)
		{
			// Create a new tile texture.
			tile_texture = tile.d_tile_texture->set_cached_object(
					std::unique_ptr<TileTexture>(new TileTexture(gl)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, boost::placeholders::_1));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(gl, tile_texture->texture, tile);

			//qDebug() << "GLMultiResolutionCubeRaster: " << d_texture_cache->get_current_num_objects_in_use();
		}

		// The filtering depends on whether the current tile is a leaf node or not.
		// So set the texture's filtering options.
		set_tile_texture_filtering(gl, tile_texture->texture, tile);

		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(gl, tile, *tile_texture);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_multi_resolution_raster->get_subject_token().is_observer_up_to_date(
				tile.d_source_texture_observer_token))
	{
		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(gl, tile, *tile_texture);
	}

	// The caller will cache this tile to keep it from being prematurely recycled by our caches.
	//
	// Note that none of this has any effect if the client specified the entire cube quad tree
	// be cached (in the 'create()' method) in which case it'll get cached regardless.
	cache_handle = boost::shared_ptr<ClientCacheTile>(
			new ClientCacheTile(tile_texture, d_cache_tile_textures));

	return tile_texture->texture;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::render_raster_data_into_tile_texture(
		GL &gl,
		const CubeQuadTreeNode &tile,
		TileTexture &tile_texture)
{
	PROFILE_FUNC();

	// We might do multiple source raster render calls (due to render target tiling).
	boost::shared_ptr<std::vector<GLMultiResolutionRaster::cache_handle_type> > tile_cache_handle(
			new std::vector<GLMultiResolutionRaster::cache_handle_type>());

	// Make sure we leave the OpenGL state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object for rendering to tile textures.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_tile_framebuffer);

	// Begin rendering to the 2D target tile texture.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tile_texture.texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a tile texture is attached to it).
	// We only need to do this once because, while the tile texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_tile_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for rendering multi-resolution cube raster tiles.");

		d_have_checked_tile_framebuffer_completeness = true;
	}

	// Specify a viewport that matches the tile dimensions.
	gl.Viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	// The view projection of the current tile.
	const GLViewProjection tile_view_projection(
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension),
			tile.d_world_model_view_transform->get_matrix(),
			tile.d_projection_transform->get_matrix());

	// Clear the framebuffer as appropriate for the source raster type.
	//
	// For example, for a *regional* normal map raster the normals outside the region must be
	// normal to the globe's surface - so the framebuffer pixels must represent this.
	//
	// Other raster types simply clear the colour buffer to a constant colour - usually RGBA(0,0,0,0).
	d_multi_resolution_raster->clear_framebuffer(gl, tile_view_projection.get_view_projection_transform());

	// Get the source raster to render into the render target using the view frustum
	// we have provided. We have already pre-calculated the list of visible source raster tiles
	// that need to be rendered into our frustum to save it a bit of culling work.
	// And that's why we also don't need to test the return value to see if anything was rendered.
	GLMultiResolutionRaster::cache_handle_type source_cache_handle;
	d_multi_resolution_raster->render(
			gl,
			tile_view_projection.get_view_projection_transform(),
			tile.d_src_raster_tiles,
			source_cache_handle);
	tile_cache_handle->push_back(source_cache_handle);

#if 0	// Debug the tiles by writing them out to file...
	QImage image(QSize(d_tile_texel_dimension, d_tile_texel_dimension), QImage::Format_ARGB32);
	image.fill(QColor(0,0,0,0).rgba());

	GLImageUtils::copy_rgba8_framebuffer_into_argb32_qimage(
			gl,
			image,
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension),
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension));

	static int count = 0;
	++count;
	image.save(QString("tile_image_") + QString::number(count) + ".png");
#endif

	tile_texture.source_cache_handle = tile_cache_handle;

	// This tile texture is now update-to-date with respect to the source multi-resolution raster.
	d_multi_resolution_raster->get_subject_token().update_observer(
			tile.d_source_texture_observer_token);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	const cube_quad_tree_type::node_type *cube_root_node =
			d_cube_quad_tree->get_quad_tree_root_node(cube_face);
	if (cube_root_node == NULL)
	{
		return boost::none;
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_root_node, *this)));
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_child_node(
		const quad_tree_node_type &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset)
{
	// Get our internal cube quad tree parent node.
	const cube_quad_tree_type::node_type &cube_parent_node = get_cube_quad_tree_node(parent_node);

	// Get the internal cube quad tree child node.
	const cube_quad_tree_type::node_type *cube_child_node =
			cube_parent_node.get_child_node(child_x_offset, child_y_offset);
	if (cube_child_node == NULL)
	{
		return boost::none;
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_child_node, *this)));
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_tile_texture(
		GL &gl,
		const GLTexture::shared_ptr_type &tile_texture,
		const CubeQuadTreeNode &tile)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the texture.
	gl.BindTexture(GL_TEXTURE_2D, tile_texture);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.
	glTexImage2D(GL_TEXTURE_2D, 0,
			d_multi_resolution_raster->get_tile_texture_internal_format(),
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::set_tile_texture_filtering(
		GL &gl,
		const GLTexture::shared_ptr_type &tile_texture,
		const CubeQuadTreeNode &tile)
{
	const GLCapabilities& capabilities = gl.get_capabilities();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the texture.
	gl.BindTexture(GL_TEXTURE_2D, tile_texture);

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//

	if (tile_texture_is_visual())
	{
		// Use nearest filtering for the 'min' filter since it displays a visually crisper image.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// For the 'mag' filter, only if it's a leaf node do we specify bilinear filtering - since only
		// then will the texture start to magnify (the bilinear makes it smooth instead of pixellated).
		if (tile.d_is_leaf_node)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		// Specify anisotropic filtering (if supported) to reduce aliasing in case tile texture is
		// subsequently sampled non-isotropically.
		//
		// Anisotropic filtering is an ubiquitous extension (that didn't become core until OpenGL 4.6).
		if (capabilities.gl_EXT_texture_filter_anisotropic)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, capabilities.gl_texture_max_anisotropy);
		}
	}
	else if (tile_texture_has_coverage())
	{
		// Texture has data (red component) and coverage (green component), and so needs filtering
		// to be implemented in shader program. Use 'nearest' filtering, and no anisotropic filtering.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		if (capabilities.gl_EXT_texture_filter_anisotropic)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1/*isotropic*/);
		}
	}
	else // a data texture with no coverage ...
	{
		// Texture just has data (no coverage) and hence filtering can be done in hardware.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Specify anisotropic filtering (if supported) to reduce aliasing in case tile texture is
		// subsequently sampled non-isotropically.
		//
		// Anisotropic filtering is an ubiquitous extension (that didn't become core until OpenGL 4.6).
		if (capabilities.gl_EXT_texture_filter_anisotropic)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, capabilities.gl_texture_max_anisotropy);
		}
	}
}
