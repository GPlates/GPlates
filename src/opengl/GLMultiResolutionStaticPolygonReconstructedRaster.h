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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/ref.hpp>

#include "GLCubeSubdivisionCache.h"
#include "GLMaskBuffersState.h"
#include "GLMultiResolutionCubeRaster.h"
#include "GLReconstructedStaticPolygonMeshes.h"
#include "GLResourceManager.h"
#include "GLStateSet.h"
#include "GLTexture.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLVertexArrayDrawable.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "maths/MathsFwd.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"

#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A raster that is reconstructed by mapping it onto a set of present-day static polygons and
	 * reconstructing the polygons (and hence partitioned pieces of the raster).
	 */
	class GLMultiResolutionStaticPolygonReconstructedRaster :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionStaticPolygonReconstructedRaster>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionStaticPolygonReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionStaticPolygonReconstructedRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionStaticPolygonReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionStaticPolygonReconstructedRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of projection transforms.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/>
						cube_subdivision_projection_transforms_cache_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of subdivision tile bounds.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				false/*CacheProjectionTransform*/,
				true/*CacheBounds*/>
						cube_subdivision_bounds_cache_type;

		/**
		 * Age grid mask and coverage rasters.
		 */
		struct AgeGridParams
		{
			AgeGridParams(
					const GLMultiResolutionCubeRaster::non_null_ptr_type &age_grid_mask_cube_raster_,
					const GLMultiResolutionCubeRaster::non_null_ptr_type &age_grid_coverage_cube_raster_) :
				age_grid_mask_cube_raster(age_grid_mask_cube_raster_),
				age_grid_coverage_cube_raster(age_grid_coverage_cube_raster_)
			{  }

			GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_mask_cube_raster;
			GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_coverage_cube_raster;
		};


		/**
		 * Creates a @a GLMultiResolutionStaticPolygonReconstructedRaster object.
		 *
		 * NOTE: An OpenGL context must currently be active.
		 *
		 * @param raster_to_reconstruct the raster to be reconstructed.
		 * @param reconstructed_static_polygon_meshes the reconstructed present day polygon meshes.
		 * @param age_grid_params is the optional age grid mask and coverage rasters.
		 */
		static
		non_null_ptr_type
		create(
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &
						cube_subdivision_projection_transforms_cache,
				const cube_subdivision_bounds_cache_type::non_null_ptr_type &cube_subdivision_bounds_cache,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager,
				boost::optional<AgeGridParams> age_grid_params = boost::none)
		{
			return non_null_ptr_type(
					new GLMultiResolutionStaticPolygonReconstructedRaster(
							raster_to_reconstruct,
							reconstructed_static_polygon_meshes,
							cube_subdivision_projection_transforms_cache,
							cube_subdivision_bounds_cache,
							texture_resource_manager,
							vertex_buffer_resource_manager,
							age_grid_params));
		}


		/**
		 * Renders the reconstructed raster using the current transform state of @a renderer.
		 */
		void
		render(
				GLRenderer &renderer);

	private:
		/**
		 * Used to cache information, specific to a quad tree node, *during* traversal of the source raster.
		 *
		 * The cube quad tree containing these nodes is destroyed at the end of each render.
		 */
		struct RenderQuadTreeNode
		{
			/**
			 * The state set used to render a tile (also references the tile source texture).
			 *
			 * If we re-use this for each transform group then the renderer will sort all
			 * our drawables by these state sets which effectively means our drawables are sorted
			 * by tile textures (making rendering a bit faster - probably not noticeable though).
			 *
			 * NOTE: Since our cube quad tree is destroyed at the end of the render call we
			 * don't need to worry about holding a reference to the tile texture which would
			 * prevent it from being re-used (even if this tile is not visible later on).
			 *
			 * NOTE: It is optional because the internal nodes of the cube quad tree will not get rendered.
			 */
			boost::optional<GLStateSet::non_null_ptr_to_const_type> scene_tile_state_set;
		};

		/**
		 * Typedef for a cube quad tree used during traversal of the source raster.
		 *
		 * Note that this cube quad tree's scope is limited to the @a render method.
		 * It provides a cache to avoid recalculating information as the source raster
		 * is traversed for each transform in the reconstructed polygon meshes.
		 */
		typedef GPlatesMaths::CubeQuadTree<RenderQuadTreeNode> render_traversal_cube_quad_tree_type;


		/**
		 * Used to cache information, specific to a quad tree node, *across* render traversals (across frames).
		 *
		 * The cube quad tree containing these nodes persists across multiple render calls.
		 */
		struct QuadTreeNode
		{
			explicit
			QuadTreeNode(
					const GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type &age_masked_tile_texture_) :
				age_masked_tile_texture(age_masked_tile_texture_)
			{  }

			/**
			 * Keeps tracks of whether the source raster has changed underneath us and
			 * we need to re-render our age-masked tile texture.
			 */
			mutable GPlatesUtils::ObserverToken source_raster_observer_token;

			/**
			 * Keeps tracks of whether the age grid mask raster has changed underneath us and
			 * we need to re-render our age-masked tile texture.
			 */
			mutable GPlatesUtils::ObserverToken age_grid_mask_observer_token;

			/**
			 * Keeps tracks of whether the age grid coverage raster has changed underneath us and
			 * we need to re-render our age-masked tile texture.
			 */
			mutable GPlatesUtils::ObserverToken age_grid_coverage_observer_token;

			/**
			 * The cached age-masked tile texture.
			 */
			GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type age_masked_tile_texture;
		};

		/**
		 * Typedef for a cube quad tree that persists from one render call to the next.
		 *
		 * The primary purpose of this cube quad tree is to cache age-masked render textures
		 * that are visible so they can be re-used is subsequent render calls (frames).
		 */
		typedef GPlatesMaths::CubeQuadTree<QuadTreeNode> cube_quad_tree_type;


		//! Typedef for a cube quad tree of raster tiles.
		typedef GLMultiResolutionCubeRaster::cube_quad_tree_type raster_cube_quad_tree_type;

		//! Typedef for a cube quad tree of age grid mask tiles.
		typedef GLMultiResolutionCubeRaster::cube_quad_tree_type age_grid_mask_cube_quad_tree_type;

		//! Typedef for a cube quad tree of age grid coverage tiles.
		typedef GLMultiResolutionCubeRaster::cube_quad_tree_type age_grid_coverage_cube_quad_tree_type;

		//! Typedef for a sequence of present day polygon mesh drawables.
		typedef GLReconstructedStaticPolygonMeshes::present_day_polygon_mesh_drawables_seq_type
				present_day_polygon_mesh_drawables_seq_type;

		/**
		 * Typedef for a cube quad tree representing possible intersections of present day polygon
		 * meshes with each cube quad tree node.
		 */
		typedef GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections
				present_day_polygon_meshes_node_intersections_type;
		typedef present_day_polygon_meshes_node_intersections_type::intersection_partition_type
				present_day_polygon_meshes_intersection_partition_type;

		//! Typedef for membership of present day polygon meshes.
		typedef GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshMembership
				present_day_polygon_mesh_membership_type;

		//! Typedef for a sequence of transform groups (of reconstructed polygon meshes).
		typedef GLReconstructedStaticPolygonMeshes::reconstructed_polygon_mesh_transform_group_seq_type
				reconstructed_polygon_mesh_transform_group_seq_type;

		//! Typedef for a transform group (of reconstructed polygon meshes).
		typedef GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformGroup
				reconstructed_polygon_mesh_transform_group_type;

		//! Typedef for a sequences of transform groups (of reconstructed polygon meshes).
		typedef GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformsGroups
				reconstructed_polygon_mesh_transform_groups_type;

		//! Typedef for a sequence of drawables.
		typedef std::vector<GLDrawable::non_null_ptr_to_const_type> drawable_seq_type;


		/**
		 * The re-sampled raster we are reconstructing.
		 */
		GLMultiResolutionCubeRaster::non_null_ptr_type d_raster_to_reconstruct;

		/**
		 * The reconstructed present day static polygon meshes.
		 */
		GLReconstructedStaticPolygonMeshes::non_null_ptr_type d_reconstructed_static_polygon_meshes;

		/**
		 * Optional age grid mask and coverage rasters.
		 *
		 * If these are not specified then no age-grid smoothing takes place during reconstruction.
		 */
		boost::optional<AgeGridParams> d_age_grid_params;

		/**
		 * Used to get projection transforms as the cube quad tree is traversed.
		 */
		cube_subdivision_projection_transforms_cache_type::non_null_ptr_type
				d_cube_subdivision_projection_transforms_cache;

		/**
		 * Used to get node bounds for view-frustum culling as the cube quad tree is traversed.
		 */
		cube_subdivision_bounds_cache_type::non_null_ptr_type d_cube_subdivision_bounds_cache;

		/**
		 * The texture dimension of a cube quad tree tile.
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * Used to allocate any textures we need.
		 */
		GLTextureResourceManager::shared_ptr_type d_texture_resource_manager;

		/**
		 * Cache of textures for age-masked source tiles.
		 */
		GPlatesUtils::ObjectCache<GLTexture>::shared_ptr_type d_age_masked_texture_cache;

		/**
		 * Used when allocating vertex arrays.
		 */
		GLVertexBufferResourceManager::shared_ptr_type d_vertex_buffer_resource_manager;

		/**
		 * Texture used to clip parts of a mesh that hang over a tile (in the cube face x/y plane).
		 */
		GLTexture::shared_ptr_type d_xy_clip_texture;

		/**
		 * Texture used to clip parts of a mesh that are inside a tile's frustum but on
		 * the opposite side of the globe (in the cube face z axis).
		 *
		 * Some polygon meshes are large enough to wrap around the globe and can appear in both
		 * a cube tile's frustum and the mirror image of that frustum on the opposite side of the
		 * globe (mirrored orthogonal to the local z-axis of the cube face that the tile is within).
		 *
		 * This texture is used to clip away negative z values.
		 */
		GLTexture::shared_ptr_type d_z_clip_texture;

		/**
		 * Matrix to convert texture coordinates from range [0,1] to range [0.25, 0.75] to
		 * map to the interior 2x2 texel region of the 4x4 clip texture.
		 */
		GLMatrix d_xy_clip_texture_transform;

		// Used to draw a textured full-screen quad into render texture.
		GLVertexArrayDrawable::non_null_ptr_type d_full_screen_quad_drawable;

		/**
		 * Caches age-masked render textures for the cube quad tree tiles for re-use over multiple frames.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;


		//! Constructor.
		GLMultiResolutionStaticPolygonReconstructedRaster(
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &
						cube_subdivision_projection_transforms_cache,
				const cube_subdivision_bounds_cache_type::non_null_ptr_type &cube_subdivision_bounds_cache,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager,
				boost::optional<AgeGridParams> age_grid_params);


		unsigned int
		get_level_of_detail(
				const GLTransformState &transform_state) const;

		void
		render_quad_tree_source_raster(
				GLRenderer &renderer,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
				const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
				const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
				const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_source_raster_and_age_grid(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
				const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		bool
		cull_quad_tree(
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
				const GLFrustum &frustum_planes,
				boost::uint32_t &frustum_plane_mask);

		GLStateSet::non_null_ptr_to_const_type
		create_scene_tile_state_set(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &scene_tile_texture,
				const GLUtils::QuadTreeUVTransform &scene_tile_uv_transform,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node);

		void
		render_source_raster_tile_to_scene(
				GLRenderer &renderer,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				const GLStateSet::non_null_ptr_to_const_type &render_scene_tile_state_set,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables);

		void
		render_polygon_drawables(
				GLRenderer &renderer,
				const boost::dynamic_bitset<> &polygon_mesh_membership,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables);

		void
		render_source_raster_and_age_grid_tile_to_scene(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node);

		GLTexture::shared_ptr_to_const_type
		get_age_masked_source_raster_tile(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node);

		void
		render_age_masked_source_raster_into_tile(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &age_masked_source_tile_texture,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node);

		void
		render_first_pass_age_masked_source_raster(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
				const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
				const GLMaskBuffersState::non_null_ptr_type &mask_colour_channels_state);

		void
		render_second_pass_age_masked_source_raster(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture,
				const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
				const GLMaskBuffersState::non_null_ptr_type &mask_colour_channels_state,
				const GLTransform &projection_transform,
				const GLTransform &view_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables);

		void
		render_third_pass_age_masked_source_raster(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &source_raster_texture,
				const GLMatrix &source_raster_tile_coverage_texture_matrix);

		void
		create_age_masked_source_tile_texture(
				const GLTexture::shared_ptr_type &texture);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H
