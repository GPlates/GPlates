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
 
#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTER_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include "GLMultiResolutionCubeRaster.h"

#include "GLClearBuffers.h"
#include "GLClearBuffersState.h"
#include "GLCompositeStateSet.h"
#include "GLCubeSubdivision.h"
#include "GLMaskBuffersState.h"
#include "GLIntersectPrimitives.h"
#include "GLResourceManager.h"
#include "GLStateSet.h"
#include "GLTextureCache.h"
#include "GLTextureUtils.h"
#include "GLTransform.h"
#include "GLVertexArray.h"
#include "GLVertexElementArray.h"
#include "GLViewport.h"
#include "GLViewportState.h"

#include "app-logic/ReconstructRasterPolygons.h"

#include "maths/UnitVector3D.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesOpenGL
{
	/**
	 * A raster that is reconstructed by mapping it onto a set of present-day polygons and
	 * reconstructing the polygons (and hence partitioned pieces of the raster) using the
	 * polygons plate ids.
	 */
	class GLMultiResolutionReconstructedRaster :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionReconstructedRaster>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionReconstructedRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionReconstructedRaster> non_null_ptr_to_const_type;


		/**
		 * A static function to return the cube subdivision required by this class.
		 *
		 * This same cube subdivision should be used by @a GLMultiResolutionCubeRaster objects
		 * passed into instances of this class.
		 */
		static
		GLCubeSubdivision::non_null_ptr_to_const_type
		get_cube_subdivision();


		/**
		 * Creates a @a GLMultiResolutionReconstructedRaster object.
		 *
		 * @param raster_to_reconstruct the raster to be reconstructed
		 * @param reconstructing_polygons the reconstructable polygon regions that will be
		 *        used to partition the raster and reconstruct it.
		 */
		static
		non_null_ptr_type
		create(
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type &reconstructing_polygons,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> &age_grid_mask_raster = boost::none,
				const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> &age_grid_coverage_raster = boost::none)
		{
			return non_null_ptr_type(
					new GLMultiResolutionReconstructedRaster(
							raster_to_reconstruct, reconstructing_polygons, texture_resource_manager,
							age_grid_mask_raster, age_grid_coverage_raster));
		}


		/**
		 * Reconstructs the polygon set and renders the corresponding rotated partitioned
		 * pieces of raster.
		 */
		void
		render(
				GLRenderer &renderer);

	private:
		typedef GPlatesAppLogic::ReconstructRasterPolygons::RotationGroup source_rotation_group_type;
		typedef GPlatesAppLogic::ReconstructRasterPolygons::ReconstructablePolygonRegion source_polygon_region_type;


		/**
		 * Represents a the part of a polygon covering a single cube face.
		 */
		class Polygon :
				public GPlatesUtils::ReferenceCount<Polygon>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<Polygon> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Polygon> non_null_ptr_to_const_type;


			static
			non_null_ptr_type
			create(
					const GPlatesPropertyValues::GeoTimeInstant &time_of_appearance_,
					const GPlatesPropertyValues::GeoTimeInstant &time_of_disappearance_)
			{
				return non_null_ptr_type(new Polygon(time_of_appearance_, time_of_disappearance_));
			}


			GPlatesPropertyValues::GeoTimeInstant time_of_appearance;
			GPlatesPropertyValues::GeoTimeInstant time_of_disappearance;

			/**
			 * Contains vertices of this polygon's mesh triangles that cover one cube face.
			 */
			GLVertexArray::shared_ptr_type vertex_array;

			/**
			 * Also keep a copy of the vertices as unit vectors as it's more convenient
			 * to work with if we have to rebuild the quad trees.
			 */
			std::vector<GPlatesMaths::UnitVector3D> mesh_points;

			/**
			 * The array storing the vertex indices representing the triangles of the polygon mesh.
			 */
			std::vector<GLuint> vertex_element_array_data;

		private:
			//! Constructor.
			Polygon(
					const GPlatesPropertyValues::GeoTimeInstant &time_of_appearance_,
					const GPlatesPropertyValues::GeoTimeInstant &time_of_disappearance_) :
				time_of_appearance(time_of_appearance_),
				time_of_disappearance(time_of_disappearance_)
			{  }
		};


		//! Typedef for a sequence of @a Polygon objects.
		typedef std::vector<Polygon::non_null_ptr_type> polygon_seq_type;

		struct PartitionedMesh
		{
			PartitionedMesh(
					const Polygon::non_null_ptr_to_const_type &polygon_,
					const GLVertexElementArray::non_null_ptr_to_const_type &vertex_element_array_) :
				polygon(polygon_),
				vertex_element_array(vertex_element_array_)
			{  }

			Polygon::non_null_ptr_to_const_type polygon;
			GLVertexElementArray::non_null_ptr_to_const_type vertex_element_array;
		};

		typedef std::vector<PartitionedMesh> partitioned_mesh_seq_type;


		/**
		 * Partitioned polygons from the same rotation group (ie, same plate id and hence the same rotation matrix).
		 */
		struct PartitionedRotationGroup :
				public GPlatesUtils::ReferenceCount<PartitionedRotationGroup>
		{
			typedef boost::intrusive_ptr<PartitionedRotationGroup> maybe_null_ptr_type;


			static
			maybe_null_ptr_type
			create(
					const GLIntersect::OrientedBoundingBox &bounding_box_)
			{
				return maybe_null_ptr_type(new PartitionedRotationGroup(bounding_box_));
			}

			//! Oriented box bounding the meshes of this partition.
			GLIntersect::OrientedBoundingBox bounding_box;

			//! The parts of the polygons' meshes that cover this tile.
			partitioned_mesh_seq_type partitioned_meshes;

		private:
			PartitionedRotationGroup(
					const GLIntersect::OrientedBoundingBox &bounding_box_) :
				bounding_box(bounding_box_)
			{  }
		};

		/**
		 * Typedef for a sequence of @a PartitionedRotationGroup object pointers.
		 */
		typedef std::vector<PartitionedRotationGroup::maybe_null_ptr_type> partitioned_rotation_group_seq_type;


		struct QuadTreeNode :
				public GPlatesUtils::ReferenceCount<QuadTreeNode>
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<QuadTreeNode> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const QuadTreeNode> non_null_ptr_to_const_type;


			static
			non_null_ptr_type
			create(
					GLMultiResolutionCubeRaster::tile_handle_type raster_tile_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_)
			{
				return non_null_ptr_type(new QuadTreeNode(
						raster_tile_, projection_transform_, view_transform_));
			}


			/**
			 * Optional coverage of polygons (in a rotation group), source raster
			 * and even age grid. Not all these things will necessarily cover a child tile.
			 *
			 * The 2D array has 'v' offsets as 1st array and 'u' offsets as 2nd array.
			 */
			boost::optional<QuadTreeNode::non_null_ptr_type> child_nodes[2][2];

			//! Tile representing raster to be reconstructed.
			GLMultiResolutionCubeRaster::tile_handle_type source_raster_tile;

			//! Optional age grid and associated coverage tile if using age grid.
			boost::optional<GLMultiResolutionCubeRaster::tile_handle_type> age_grid_mask_tile;
			boost::optional<GLMultiResolutionCubeRaster::tile_handle_type> age_grid_coverage_tile;

			//! Projection matrix defining perspective frustum of this tile.
			GLTransform::non_null_ptr_to_const_type projection_transform;

			//! View matrix defining orientation of frustum of this tile.
			GLTransform::non_null_ptr_to_const_type view_transform;

			/**
			 * The polygon mesh information for each rotation group.
			 *
			 * NOTE: Some rotation groups won't cover the current quad tree node tile in which
			 * case their respective pointer in this sequence will be NULL.
			 */
			partitioned_rotation_group_seq_type partitioned_rotation_groups;

			/**
			 * The texture representation of the raster data for this tile.
			 *
			 * NOTE: It's only used if we're using an age grid since we need to combine source
			 * raster and age grid to a render texture before we can render the scene.
			 */
			GLVolatileTexture age_masked_render_texture;

			/*
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			GLTextureUtils::ValidToken source_texture_valid_token;
			GLTextureUtils::ValidToken age_grid_mask_texture_valid_token;
			GLTextureUtils::ValidToken age_grid_coverage_texture_valid_token;

		private:
			QuadTreeNode(
					GLMultiResolutionCubeRaster::tile_handle_type raster_tile_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_) :
				source_raster_tile(raster_tile_),
				projection_transform(projection_transform_),
				view_transform(view_transform_)
			{  }
		};


		struct QuadTree
		{
			// Optional coverage of polygons in a rotation group.
			boost::optional<QuadTreeNode::non_null_ptr_type> root_node;
		};


		struct CubeFace
		{
			QuadTree quad_tree;
		};


		struct Cube
		{
			CubeFace faces[6];
		};


		/**
		 * All polygons in a rotation group have the same plate id and hence the same rotation matrix.
		 */
		struct RotationGroup
		{
			explicit
			RotationGroup(
					const GPlatesAppLogic::ReconstructRasterPolygons::RotationGroup::non_null_ptr_to_const_type &
							rotation_) :
				rotation(rotation_)
			{  }

			GPlatesAppLogic::ReconstructRasterPolygons::RotationGroup::non_null_ptr_to_const_type rotation;

			/**
			 * Polygons clipped to this cube face and then meshed.
			 *
			 * NOTE: For now we don't clip before meshing.
			 */
			polygon_seq_type polygons;
		};

		//! Typedef for a sequence of @a RotationGroup objects.
		typedef std::vector<RotationGroup> rotation_group_seq_type;


		/**
		 * This is used only when building a quadtree.
		 */
		struct PartitionedMeshBuilder
		{
			explicit
			PartitionedMeshBuilder(
					const Polygon::non_null_ptr_to_const_type &polygon_) :
				polygon(polygon_)
			{  }

			Polygon::non_null_ptr_to_const_type polygon;
			std::vector<GLuint> vertex_element_array_data;
		};

		//! Typedef for a sequence of @a PartitionedMeshBuilder objects.
		typedef std::vector<PartitionedMeshBuilder> partitioned_mesh_builder_seq_type;


		/**
		 * This is used only when building a quadtree.
		 */
		struct PartitionedRotationGroupBuilder
		{
			/**
			 * Used to build the parts of the polygons' meshes as we traverse down the quad tree.
			 */
			partitioned_mesh_builder_seq_type partitioned_mesh_builders;
		};

		//! Typedef for a sequence of @a PartitionedRotationGroupBuilder objects.
		typedef std::vector<PartitionedRotationGroupBuilder> partitioned_rotation_group_builder_seq_type;


		/**
		 * The re-sampled raster we are reconstructing.
		 */
		GLMultiResolutionCubeRaster::non_null_ptr_type d_raster_to_reconstruct;

		/**
		 * Defines the quadtree subdivision of each cube face and any overlaps of extents.
		 */
		GLCubeSubdivision::non_null_ptr_to_const_type d_cube_subdivision;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		std::size_t d_tile_texel_dimension;

		/**
		 * Contains a quad tree for each face of the cube.
		 */
		Cube d_cube;

		/**
		 * Keeping a reference to the original source of polygons in case we need to rebuild
		 * our polygon meshes for some reason.
		 */
		GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type d_reconstructing_polygons;

		rotation_group_seq_type d_rotation_groups;

		/**
		 * Used to allocate any textures we need.
		 */
		GLTextureResourceManager::shared_ptr_type d_texture_resource_manager;

		/**
		 * Texture used to clip parts of a mesh that hang over a tile.
		 */
		GLTexture::shared_ptr_type d_clip_texture;

		/**
		 * Convenient boolean flag to indicate whether we're using an age grid or not.
		 * This is true if we have both an age grid coverage raster and an age grid raster -
		 * we'll either have neither or both since they're both sourced from a single proxied raster.
		 */
		bool d_using_age_grid;

		// Since the age grid mask changes dynamically as the reconstruction time changes
		// we don't need to worry about caching so much - just enough caching so that panning
		// the view doesn't mean every tile on screen needs to be regenerated - just the ones
		// near the edges.
		// This can be achieved by setting the cache size to one and just letting it grow as needed.
		boost::optional<GLTextureCache::non_null_ptr_type> d_age_masked_raster_texture_cache;

		/**
		 * Optional age grid raster have per-texel age masking instead of per-polygon.
		 */
		boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> d_age_grid_mask_raster;

		/**
		 * Optional age grid coverage raster (is zero where there are no age values in age grid raster).
		 */
		boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> d_age_grid_coverage_raster;

		/**
		 * Used to determine if we need to rebuild any cached age-masked textures due to source data changing.
		 *
		 * NOTE: This is only needed if we're using an age grid because otherwise we don't
		 * cache anything.
		 */
		GLTextureUtils::ValidToken d_source_raster_valid_token;

		/**
		 * Used to determine if we need to rebuild any cached age-masked textures due to changed age grid mask.
		 */
		GLTextureUtils::ValidToken d_age_grid_mask_raster_valid_token;

		/**
		 * Used to determine if we need to rebuild any cached age-masked textures due to changed age grid coverage.
		 */
		GLTextureUtils::ValidToken d_age_grid_coverage_raster_valid_token;

		//
		// Various state used when rendering to age grid mask render texture.
		//
		GLClearBuffersState::non_null_ptr_type d_clear_buffers_state;
		GLClearBuffers::non_null_ptr_type d_clear_buffers;
		GLViewport d_viewport;
		GLViewportState::non_null_ptr_type d_viewport_state;

		// Used to draw a textured full-screen quad into render texture.
		GLVertexArray::shared_ptr_type d_full_screen_quad_vertex_array;
		GLVertexElementArray::non_null_ptr_type d_full_screen_quad_vertex_element_array;

		// The composite state sets used for each of the three render passes required to
		// render an age grid mask.
		GLCompositeStateSet::non_null_ptr_type d_first_age_mask_render_pass_state;
		GLCompositeStateSet::non_null_ptr_type d_second_age_mask_render_pass_state;
		GLCompositeStateSet::non_null_ptr_type d_third_age_mask_render_pass_state;


		//! Constructor.
		GLMultiResolutionReconstructedRaster(
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type &reconstructing_polygons,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> &age_grid_mask_raster,
				const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> &age_grid_coverage_raster);


		void
		generate_polygon_mesh(
				RotationGroup &rotation_group,
				const source_polygon_region_type::non_null_ptr_to_const_type &src_polygon_region);

		void
		initialise_cube_quad_trees();

		void
		create_age_masked_tile_texture(
				const GLTexture::shared_ptr_type &texture);

		void
		create_clip_texture();

		unsigned int
		get_level_of_detail(
				const GLTransformState &transform_state) const;

		boost::optional<QuadTreeNode::non_null_ptr_type>
		create_quad_tree_node(
				GLCubeSubdivision::CubeFaceType cube_face,
				const partitioned_rotation_group_builder_seq_type &parent_partitioned_rotation_group_builders,
				const GLMultiResolutionCubeRaster::QuadTreeNode &source_raster_quad_tree_node,
				const boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> &age_grid_mask_raster_quad_tree_node,
				const boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> &age_grid_coverage_raster_quad_tree_node,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset);

		PartitionedRotationGroup::maybe_null_ptr_type
		partition_rotation_group(
				const PartitionedRotationGroupBuilder &parent_partitioned_rotation_group_builder,
				// The partitioned mesh builders for the current partition (if any polygons intersect our partition).
				PartitionedRotationGroupBuilder &partitioned_rotation_group_builder,
				const GLTransformState::FrustumPlanes &frustum_planes,
				const GLIntersect::OrientedBoundingBoxBuilder &initial_bounding_box_builder);

		void
		update_input_rasters_valid_tokens();

		void
		render_quad_tree(
				GLRenderer &renderer,
				QuadTreeNode &quad_tree_node,
				unsigned int rotation_group_index,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLTransformState::FrustumPlanes &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_node_tile(
				GLRenderer &renderer,
				QuadTreeNode &quad_tree_node,
				const PartitionedRotationGroup &partitioned_rotation_group);

		void
		render_age_masked_source_raster_into_tile(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &age_mask_tile_texture,
				QuadTreeNode &quad_tree_node);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &source_raster_texture,
				QuadTreeNode &quad_tree_node,
				const PartitionedRotationGroup &partitioned_rotation_group);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTER_H
