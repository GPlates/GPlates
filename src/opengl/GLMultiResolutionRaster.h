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
 
#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRASTER_H

#include <cstddef> // For std::size_t
#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "GLDrawable.h"
#include "GLIntersectPrimitives.h"
#include "GLMultiResolutionRasterSource.h"
#include "GLResourceManager.h"
#include "GLTexture.h"
#include "GLTextureCache.h"
#include "GLTextureUtils.h"
#include "GLTransformState.h"
#include "GLVertexArray.h"
#include "GLVertexArrayCache.h"
#include "GLVertexElementArray.h"

#include "maths/PointOnSphere.h"

#include "property-values/Georeferencing.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An arbitrary dimension raster image represented as a multi-resolution pyramid
	 * of tiled OpenGL textures and associated vertex meshes.
	 *
	 * The idea is to first determine the resolution required to display the raster
	 * such that one texel is roughly one pixel on the screen and secondly to find those
	 * texture tiles that are visible on the screen such that only those tiles need be
	 * generated for display.
	 *
	 * The meshes generated for each texture tile are dense enough to ensure that each texel
	 * in the raster is bounded in its deviation from its true position on the globe.
	 * This bound is a constant times the size of a texel on the globe.
	 */
	class GLMultiResolutionRaster :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionRaster>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionRaster> non_null_ptr_to_const_type;


		/**
		 * Typedef for a handle to a tile.
		 *
		 * A tile represents an arbitrary patch of the raster that is
		 * covered by a single OpenGL texture.
		 */
		typedef std::size_t tile_handle_type;


		/**
		 * The order of scanlines or rows of data in the raster as visualised in the image.
		 */
		enum RasterScanlineOrderType
		{
			TOP_TO_BOTTOM, // the first row of data in the raster is for the *top* of the image
			BOTTOM_TO_TOP  // the first row of data in the raster is for the *bottom* of the image
		};


		/**
		 * Creates a @a GLMultiResolutionRaster object.
		 *
		 * @a tile_texel_dimension must be a power-of-two - it is the OpenGL square texture
		 * dimension to use for the tiled textures that represent the multi-resolution raster.
		 *
		 * If @a tile_texel_dimension is greater than the maximum texture size supported
		 * by the run-time system then it will be reduced to the maximum texture size.
		 *
		 * Returns false if @a raster is not a proxy raster or if it's uninitialised.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				RasterScanlineOrderType raster_scanline_order = TOP_TO_BOTTOM);


		/**
		 * Returns a token that clients can store with any cached data we render for them and
		 * compare against their current token to determine if they need us to re-render.
		 */
		GLTextureUtils::ValidToken
		get_current_valid_token();


		/**
		 * Renders all tiles visible in the view frustum of the current render target
		 * of @a renderer.
		 *
		 * NOTE: The only OpenGL state set by this method is binding the individual
		 * tile textures to texture unit zero.
		 * The caller is responsible for everything else including enabling texturing.
		 *
		 * A positive @a level_of_detail_bias can be used to relax the constraint that
		 * the rendered raster have texels that are less or equal to the size of a pixel
		 * of the viewport (to avoid blockiness or blurriness of the rendered raster).
		 * The @a level_of_detail_bias is a log2 so 1.0 means use a level of detail texture
		 * that requires half the resolution (eg, 256x256 instead of 512x512) of what would
		 * normally be used if a LOD bias of zero were used, and 2.0 means a quarter
		 * (eg, 128x128 instead of 512x512). Fractional values are allowed (and more useful).
		 */
		void
		render(
				GLRenderer &renderer,
				float level_of_detail_bias = 0.0f);


		/**
		 * Renders the specified tiles to the current render target of @a renderer.
		 */
		void
		render(
				GLRenderer &renderer,
				const std::vector<tile_handle_type> &tiles);


		/**
		 * Returns the number of levels of detail.
		 *
		 * The highest resolution (original raster) is level 0 and the lowest
		 * resolution level is 'N-1' where 'N' is the number of levels.
		 */
		std::size_t
		get_num_levels_of_detail() const
		{
			return d_level_of_detail_pyramid.size();
		}


		/**
		 * Returns a list of tiles  that are visible the inside the view frustum planes defined
		 * by the transform state @a transform_state.
		 *
		 * All returned tiles are from the same level-of-detail in the multi-resolution raster.
		 *
		 * Returns the lowest resolution tiles that adequately fulfill the resolution
		 * needs of the current render target (or the highest level-of-detail if none of the
		 * level-of-details is adequate).
		 * The required resolution is determined by the current viewport dimensions and the
		 * current model-view-projection transform (both provided by @a transform_state).
		 *
		 * See @a render for a description of @a level_of_detail_bias.
		 *
		 * Also returns the unclamped exact floating-point level-of-detail factor that theoretically
		 * represents the exact level-of-detail that would be required to fulfill the resolution
		 * needs of the current render target (as defined by @a transform_state).
		 * Since tiles are only at integer level-of-detail factors, and are clamped to lie
		 * in the range [0,N) where 'N' is the integer returned by @a get_num_levels_of_detail,
		 * an unclampled floating-point number is only useful to determine if the current
		 * render target is big enough or if it's too big, ie, if it's outside the range [0,N).
		 */
		float
		get_visible_tiles(
				const GLTransformState &transform_state,
				std::vector<tile_handle_type> &visible_tiles,
				float level_of_detail_bias = 0.0f) const;

	private:
		/**
		 * A tile represents an arbitrary patch of the raster that is
		 * covered by a single OpenGL texture.
		 *
		 * This tile has valid vertices and a valid texture and is ready
		 * to draw - as long as this @a Tile object exists the vertices and texture
		 * will be valid.
		 */
		class Tile
		{
		public:
			Tile(
					const GLVertexArray::shared_ptr_to_const_type &vertex_array,
					const GLVertexElementArray::non_null_ptr_to_const_type &vertex_element_array,
					const GLTexture::shared_ptr_to_const_type &texture);

			/**
			 * Returns the drawable for this tile.
			 *
			 * NOTE: The returned shared pointer should only be used for rendering
			 * and then discarded. This is because the vertex array inside the drawable
			 * is part of a vertex array cache and so the vertex array cannot be recycled
			 * if a shared pointer is holding onto it.
			 */
			GLDrawable::non_null_ptr_to_const_type
			get_drawable() const
			{
				return d_drawable;
			}

			/**
			 * Returns texture of tile.
			 *
			 * NOTE: The returned shared pointer should only be used for rendering
			 * and then discarded. This is because the texture is part of a texture cache
			 * and so it cannot be recycled if a shared pointer is holding onto it.
			 */
			GLTexture::shared_ptr_to_const_type
			get_texture() const
			{
				return d_texture;
			}

		private:
			GLDrawable::non_null_ptr_to_const_type d_drawable;
			GLTexture::shared_ptr_to_const_type d_texture;
		};


		/**
		 * Retains information to build a single tile of the raster.
		 */
		class LevelOfDetailTile :
				public GPlatesUtils::ReferenceCount<LevelOfDetailTile>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<LevelOfDetailTile> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const LevelOfDetailTile> non_null_ptr_to_const_type;


			static
			non_null_ptr_type
			create(
					unsigned int lod_level_,
					unsigned int x_geo_start_,
					unsigned int x_geo_end_,
					unsigned int y_geo_start_,
					unsigned int y_geo_end_,
					unsigned int x_num_vertices_,
					unsigned int y_num_vertices_,
					float u_start_,
					float u_end_,
					float v_start_,
					float v_end_,
					unsigned int u_lod_texel_offset_,
					unsigned int v_lod_texel_offset_,
					unsigned int num_u_lod_texels_,
					unsigned int num_v_lod_texels_,
					const GLVertexElementArray::non_null_ptr_to_const_type &vertex_element_array_)
			{
				return non_null_ptr_type(new LevelOfDetailTile(
						lod_level_,
						x_geo_start_, x_geo_end_, y_geo_start_, y_geo_end_,
						x_num_vertices_, y_num_vertices_, u_start_, u_end_, v_start_, v_end_,
						u_lod_texel_offset_, v_lod_texel_offset_, num_u_lod_texels_, num_v_lod_texels_,
						vertex_element_array_));
			}


			/**
			 * The level-of-detail of this tile.
			 */
			unsigned int lod_level;


			// NOTE: 'x_geo_start', 'x_geo_end', 'y_geo_start' and 'y_geo_end'
			// are georeference coordinates of the original high-resolution raster and not
			// the texels of this level-of-detail (unless, of course, this
			// level-of-detail is the highest resolution).
			//
			// This is because georeferencing works with the highest resolution pixel coordinates.

			/**
			 * The start georeference x coordinate of this tile.
			 * The left edge of the first pixel (not the pixel centre).
			 */
			unsigned int x_geo_start;

			/**
			 * The end georeference x coordinate of this tile.
			 * The right edge of the last pixel (not the pixel centre).
			 */
			unsigned int x_geo_end;

			/**
			 * The start georeference y coordinate of this tile.
			 * The top edge of the first pixel (not the pixel centre).
			 */
			unsigned int y_geo_start;

			/**
			 * The end georeference y coordinate of this tile.
			 * The bottom edge of the last pixel (not the pixel centre).
			 */
			unsigned int y_geo_end;

			/**
			 * The number of vertices along the x direction of this tile.
			 */
			unsigned int x_num_vertices;

			/**
			 * The number of vertices along the y direction of this tile.
			 */
			unsigned int y_num_vertices;

			//! The 'u' texture coordinate corresponding to @a x_start.
			float u_start;
			//! The 'u' texture coordinate corresponding to @a x_end.
			float u_end;

			//! The 'v' texture coordinate corresponding to @a y_start.
			float v_start;
			//! The 'v' texture coordinate corresponding to @a y_end.
			float v_end;

			/*
			 * Offsets to the first texel in the region covered by this tile.
			 * Note that the 'y' and the 'v' directions are inverted wrt each other -
			 * this is reflected in the 'v' texture coordinates.
			 */
			unsigned int u_lod_texel_offset;
			unsigned int v_lod_texel_offset;

			/*
			 * The number of texels that we need to load from the raster image in order
			 * to cover the tile.
			 *
			 * For example, the texels needed by a 5x5 raster image are:
			 * Level 0: 5x5
			 * Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
			 * Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
			 * Level 3: 1x1 (covers same area as level 2)
			 *
			 * Whereas the same area on the globe must be covered by all levels of detail so
			 * the area covered on the globe in units of texels (at that level-of-detail) is:
			 * Level 0: 5x5
			 * Level 1: 2.5 x 2.5
			 * Level 2: 1.25 * 1.25
			 * Level 3: 0.625 * 0.625
			 *
			 * ...so if this is a level 1 tile then the number of texels needed is 3x3 whereas
			 * only 2.5 x 2.5 texels actually contribute to the tile on the globe. However
			 * we need 3x3 texels because we can't load partial texels into an OpenGL texture.
			 */
			unsigned int num_u_lod_texels;
			unsigned int num_v_lod_texels;

			/**
			 * The vertex indices used for this tile.
			 * This is typically shared with other tiles.
			 */
			GLVertexElementArray::non_null_ptr_to_const_type vertex_element_array;

			//
			// The following data members use @a GLCache because:
			// - we don't want to generate them up front for all tiles,
			// - resource memory usage would be too expensive if we allocated all tiles up front,
			// - the time taken to generate all tiles up front might be too expensive,
			// - not all tiles might be needed (eg, user might not zoom into all high-res regions).
			//
			// We'd like to treat these members as const since we're not really changing
			// the raster but, for the above reasons, we need to generate tile data
			// on an as-needed basis so we make these members 'mutable'.
			//

			/**
			 * The vertices required to draw this tile onto the globe.
			 */
			mutable GLVolatileVertexArray vertex_array;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			mutable GLVolatileTexture texture;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GLTextureUtils::ValidToken source_texture_valid_token;

		private:
			//! Constructor.
			LevelOfDetailTile(
					unsigned int lod_level_,
					unsigned int x_geo_start_,
					unsigned int x_geo_end_,
					unsigned int y_geo_start_,
					unsigned int y_geo_end_,
					unsigned int x_num_vertices_,
					unsigned int y_num_vertices_,
					float u_start_,
					float u_end_,
					float v_start_,
					float v_end_,
					unsigned int u_lod_texel_offset_,
					unsigned int v_lod_texel_offset_,
					unsigned int num_u_lod_texels_,
					unsigned int num_v_lod_texels_,
					const GLVertexElementArray::non_null_ptr_to_const_type &vertex_element_array_);
		};


		/**
		 * A level-of-detail represents a full set of tiles covering the entire raster,
		 * but at a particular resolution.
		 */
		class LevelOfDetail :
				public GPlatesUtils::ReferenceCount<LevelOfDetail>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<LevelOfDetail> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const LevelOfDetail> non_null_ptr_to_const_type;

			/**
			 * A node in an oriented bounding box tree.
			 *
			 * The tree is recursively visited to quickly find tiles that are visible
			 * in the view frustum (tiles are referenced by leaf nodes of the tree).
			 */
			struct OBBTreeNode
			{
				//! Constructor.
				OBBTreeNode(
						const GLIntersect::OrientedBoundingBox &bounding_box_,
						bool is_leaf_node_) :
					bounding_box(bounding_box_),
					is_leaf_node(is_leaf_node_)
				{  }

				/**
				 * An oriented box that bounds the subtree rooted at this node (for internal
				 * tree nodes) or bounds the vertices of a raster tile and hence the
				 * triangle mesh attached to its vertices (for leaf tree nodes).
				 */
				GLIntersect::OrientedBoundingBox bounding_box;

				//! Structure containing internal or leaf node information.
				struct
				{
					bool is_leaf_node;

					union
					{
						//! The child indices if this is *not* a leaf node.
						std::size_t child_node_indices[2];

						//! The tile handle if this *is* a leaf node.
						tile_handle_type tile;
					};
				};
			};

			//! Typedef for the container of all OBB tree nodes belonging to a level-of-detail.
			typedef std::vector<OBBTreeNode> obb_tree_node_seq_type;


			static
			non_null_ptr_type
			create(
					unsigned int lod_level)
			{
				return non_null_ptr_type(new LevelOfDetail(lod_level));
			}


			/**
			 * Returns the node of the OBB tree at index @a node_index.
			 *
			 * NOTE: The root node is the entry point for accessing the tiles (in the leaf nodes).
			 */
			const OBBTreeNode &
			get_obb_tree_node(
					std::size_t node_index) const;

			/**
			 * Returns the node of the OBB tree at index @a node_index.
			 *
			 * NOTE: The root node is the entry point for accessing the tiles (in the leaf nodes).
			 */
			OBBTreeNode &
			get_obb_tree_node(
					std::size_t node_index);


			/**
			 * The level-of-detail level (highest resolution is 0).
			 */
			unsigned int lod_level;

			/**
			 * The OBB tree nodes are stored here.
			 */
			obb_tree_node_seq_type obb_tree_nodes;

			/**
			 * Index of the root OBB tree node in @a obb_tree_nodes.
			 */
			std::size_t obb_tree_root_node_index;

		private:
			explicit
			LevelOfDetail(
					unsigned int lod_level_);
		};


		//! Typedef for a sequence of level-of-detail tiles.
		typedef std::vector<LevelOfDetailTile::non_null_ptr_type> level_of_detail_tile_seq_type;

		//! Typedef for a sequence of level-of-details.
		typedef std::vector<LevelOfDetail::non_null_ptr_type> level_of_detail_seq_type;

		//! Typedef for mapping a tile's vertex dimensions to vertex indices.
		typedef std::map<
				std::pair<unsigned int,unsigned int>,
				GLVertexElementArray::non_null_ptr_to_const_type> vertex_element_array_map_type;


		/**
		 * Georeferencing information to position the raster onto the globe.
		 */
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;

		/**
		 * The source of multi-resolution raster data.
		 */
		GLMultiResolutionRasterSource::non_null_ptr_type d_raster_source;

		/**
		 * Used to determine if we need to rebuild any cached textures due to source data changing.
		 */
		GLTextureUtils::ValidToken d_raster_source_valid_token;

		//! Original raster width.
		unsigned int d_raster_width;

		//! Original raster height.
		unsigned int d_raster_height;

		/**
		 * The scanline order of the raster (whether first row of data is at top or bottom of image).
		 */
		RasterScanlineOrderType d_raster_scanline_order;


		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * All tiles of all resolution are grouped into one array for easy lookup for clients.
		 */
		level_of_detail_tile_seq_type d_tiles;

		level_of_detail_seq_type d_level_of_detail_pyramid;

		/**
		 * The maximum size of any texel in the original raster (the highest resolution
		 * level-of-detail) when projected onto the unit sphere.
		 *
		 * This is used for level-of-detail selection.
		 */
		float d_max_highest_resolution_texel_size_on_unit_sphere;

		/**
		 * This raster has its own cache of textures which gets reused/recycled as the
		 * view pans across the raster.
		 * Since the maximum number of screen pixels covered by a raster is bounded
		 * (by the number of pixels in the viewport) then there is also a bound on the
		 * maximum number of texture tiles needed to display the raster - note that this
		 * is independent of raster resolution currently being displayed because as the view
		 * zooms into a raster the visible part of the raster decreases to compensate for the
		 * increased raster resolution being displayed.
		 */
		GLTextureCache::non_null_ptr_type d_texture_cache;

		/**
		 * A cache of vertex arrays to limit memory usage.
		 *
		 * Later we may change the vertex array to a vertex buffer object that gets
		 * stored on the GPU in VRAM - in that case having a cache also means we are
		 * not creating/destroying vertex buffer objects unnecessarily.
		 */
		GLVertexArrayCache::non_null_ptr_type d_vertex_array_cache;

		/**
		 * Shared vertex indices used by the tiles of this raster.
		 */
		vertex_element_array_map_type d_vertex_element_arrays;


		/**
		 * The number of texels between two adjacent vertices along a horizontal or
		 * vertical edge of the tile.
		 *
		 * This is relatively small to minimise texel deviation on the sphere.
		 * In other words we want each texel to be positioned approximately where it
		 * should be on the globe. The texel is positioned exactly only at vertices
		 * because vertices tie the raster to the globe. The further a texel is from
		 * a vertex the higher it's position error is likely to be, so more vertices
		 * means less error.
		 *
		 * We could instead have adaptively sampled the mesh where the deviation exceeded
		 * a threshold but there are benefits to having a simple mesh and GPUs are
		 * fast enough that having more vertices than we need won't slow it down.
		 * See http://developer.nvidia.com/docs/IO/8230/BatchBatchBatch.pdf
		 * FIXME: Move these relatively dense meshes to vertex buffer objects so they
		 * reside on the GPU and we're not transferring them to the GPU each time we draw.
		 *
		 * NOTE: This means there are NUM_TEXELS_PER_VERTEX * NUM_TEXELS_PER_VERTEX
		 * texels between four adjacent vertices that form a quad (two mesh triangles).
		 */
		static const unsigned int NUM_TEXELS_PER_VERTEX = 16;


		/**
		 * We also need to make sure there are enough vertices to follow the curvature of the
		 * globe, otherwise the mesh segments will dip too far below the surface of the sphere.
		 */
		static const unsigned int MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES = 5;


		//! Constructor.
		GLMultiResolutionRaster(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				RasterScanlineOrderType raster_scanline_order);

		/**
		 * Creates the level-of-detail pyramid structures.
		 * They initially contain no raster or vertex data though.
		 */
		void
		initialise_level_of_detail_pyramid();


		/**
		 * Creates a level-of-detail structure for level @a lod_level.
		 */
		LevelOfDetail::non_null_ptr_type
		create_level_of_detail(
				const unsigned int lod_level);


		/**
		 * Creates an oriented bounding box tree covering a level-of-detail.
		 */
		std::size_t
		create_obb_tree(
				LevelOfDetail &level_of_detail,
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end);


		/**
		 * Creates a leaf node of an OBB tree covering a specific level-of-detail.
		 */
		std::size_t
		create_obb_tree_leaf_node(
				LevelOfDetail &level_of_detail,
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end);


		/**
		 * Creates a raster tile structure containing enough information to
		 * subsequently generate texture and vertex data for rendering the tile.
		 */
		tile_handle_type
		create_level_of_detail_tile(
				LevelOfDetail &level_of_detail,
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end);


		/**
		 * Creates an oriented bounding box for @a lod_tile.
		 */
		GLIntersect::OrientedBoundingBox
		bound_level_of_detail_tile(
				const LevelOfDetailTile &lod_tile) const;

		/**
		 * Creates an oriented bounding box (OBB) builder whose z-axis coincides with
		 * the position of the specified pixel coordinates (of the raster) on the unit sphere.
		 * This same point on the sphere is then added to the builder to set the maximum
		 * extent of the OBB along its z-axis. The caller will need to add more points though.
		 *
		 * The returned bounding box (builder) depends on georeferencing (mapping of pixel
		 * coordinates to a position on the globe).
		 */
		GLIntersect::OrientedBoundingBoxBuilder
		create_oriented_bounding_box_builder(
				const double &x_geo_coord,
				const double &y_geo_coord) const;


		/**
		 * Calculates the maximum size of any texel of @a lod_tile projected onto the unit sphere.
		 */
		float
		calc_max_texel_size_on_unit_sphere(
				const unsigned int lod_level,
				const LevelOfDetailTile &lod_tile) const;


		/**
		 * Calculates the number of vertices required for the specified tile.
		 */
		const std::pair<unsigned int, unsigned int>
		calculate_num_vertices_along_tile_edges(
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end,
				const unsigned int num_u_texels,
				const unsigned int num_v_texels);


		/**
		 * Gets, or creates if one doesn't exist, a uniform mesh of triangles covering a tile
		 * with @a num_vertices_along_tile_x_edge by @a num_vertices_along_tile_y_edge vertices.
		 *
		 * This is just the vertex indices (not the vertices themselves which differ for each tile).
		 */
		GLVertexElementArray::non_null_ptr_to_const_type
		get_vertex_element_array(
				const unsigned int num_vertices_along_tile_x_edge,
				const unsigned int num_vertices_along_tile_y_edge);


		/**
		 * Returns the level-of-detail that fulfills the resolution requirements, and no more,
		 * of the current viewport and projection of @a transform_state.
		 */
		unsigned int
		get_level_of_detail(
				const GLTransformState &transform_state,
				float level_of_detail_bias,
				float &level_of_detail_factor) const;

		/**
		 * Recursively traverses OBB tree to find visible tiles.
		 */
		void
		get_visible_tiles(
				const GLTransformState::FrustumPlanes &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				const LevelOfDetail &lod,
				const LevelOfDetail::OBBTreeNode &obb_tree_node,
				std::vector<tile_handle_type> &visible_tiles) const;


		/**
		 * Updates our valid token to match that of our raster source.
		 */
		void
		update_raster_source_valid_token();


		/**
		 * Returns the tile corresponding to @a tile_handle.
		 *
		 * Discard the returned tile after rendering so that it's vertices and texture
		 * can be recycled for subsequent scene renders.
		 */
		Tile
		get_tile(
				tile_handle_type tile_handle,
				GLRenderer &renderer);

		/**
		 * Returns the texture for the tile @a lod_tile.
		 */
		GLTexture::shared_ptr_to_const_type
		get_tile_texture(
				const LevelOfDetailTile &lod_tile,
				GLRenderer &renderer);

		/**
		 * Returns the vertex array for the tile @a lod_tile.
		 */
		GLVertexArray::shared_ptr_to_const_type
		get_tile_vertex_array(
				const LevelOfDetailTile &lod_tile);

		/**
		 * Loads raster data into @a lod_tile.
		 */
		void
		load_raster_data_into_tile_texture(
				const LevelOfDetailTile &lod_tile,
				const GLTexture::shared_ptr_type &tile_texture,
				GLRenderer &renderer);

		/**
		 * Creates a texture in OpenGL but doesn't load any image data into it.
		 */
		void
		create_tile_texture(
				const GLTexture::shared_ptr_type &texture);

		/**
		 * Loads vertex data into @a lod_tile.
		 */
		void
		load_vertices_into_tile_vertex_array(
				const LevelOfDetailTile &lod_tile,
				const GLVertexArray::shared_ptr_type &vertex_array,
				bool vertex_array_was_recycled);


		/**
		 * Converts from raster pixel coordinates to a position on the globe.
		 */
		GPlatesMaths::PointOnSphere
		convert_pixel_coord_to_geographic_coord(
				const double &x_pixel_coord,
				const double &y_pixel_coord) const;
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRASTER_H
