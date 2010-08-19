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

#include "GLMultiResolutionCubeRaster.h"

#include "GLCubeSubdivision.h"
#include "GLIntersectPrimitives.h"
#include "GLResourceManager.h"
#include "GLTextureCache.h"
#include "GLTransform.h"
#include "GLVertexArray.h"
#include "GLVertexElementArray.h"

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
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager)
		{
			return non_null_ptr_type(
					new GLMultiResolutionReconstructedRaster(
							raster_to_reconstruct, reconstructing_polygons, texture_resource_manager));
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

		struct PolygonMesh
		{
			Polygon::non_null_ptr_to_const_type polygon;
			GLVertexElementArray::non_null_ptr_to_const_type vertex_element_array;
		};

		typedef std::vector<PolygonMesh> polygon_mesh_seq_type;

		struct AgeGridTile
		{
			GLMultiResolutionCubeRaster::tile_handle_type age_grid_source_tile;
			GLMultiResolutionCubeRaster::tile_handle_type age_grid_coverage_tile;
		};

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
					const GLTransform::non_null_ptr_to_const_type &view_transform_,
					const GLIntersect::OrientedBoundingBox &bounding_box_)
			{
				return non_null_ptr_type(new QuadTreeNode(
						raster_tile_, projection_transform_, view_transform_, bounding_box_));
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

			//! Projection matrix defining perspective frustum of this tile.
			GLTransform::non_null_ptr_to_const_type projection_transform;

			//! View matrix defining orientation of frustum of this tile.
			GLTransform::non_null_ptr_to_const_type view_transform;

			//! Oriented box bounding the mesh of this quad tree node.
			GLIntersect::OrientedBoundingBox bounding_box;

			//! The parts of the polygons' meshes that cover this tile.
			polygon_mesh_seq_type polygon_meshes;

			//! Optional age grid and associated coverage tile if using age grid.
			boost::optional<AgeGridTile> age_grid_tile;

			/**
			 * The texture representation of the raster data for this tile.
			 * Only required if using an age grid since need to combine raster
			 * and age grid to a render texture before can use for scene rendering.
			 */
			boost::optional<GLVolatileTexture> render_texture;

		private:
			QuadTreeNode(
					GLMultiResolutionCubeRaster::tile_handle_type raster_tile_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_,
					const GLIntersect::OrientedBoundingBox &bounding_box_) :
				source_raster_tile(raster_tile_),
				projection_transform(projection_transform_),
				view_transform(view_transform_),
				bounding_box(bounding_box_)
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
			Cube cube;

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
		struct PartitionedMesh
		{
			Polygon::non_null_ptr_to_const_type polygon;
			std::vector<GLuint> vertex_element_array_data;
		};

		//! Typedef for a sequence of @a PartitionedMesh objects.
		typedef std::vector<PartitionedMesh> partition_mesh_seq_type;


		/**
		 * The re-sampled raster we are reconstructing.
		 */
		GLMultiResolutionCubeRaster::non_null_ptr_type d_raster_to_reconstruct;

		/**
		 * Defines the quadtree subdivision of each cube face and any overlaps of extents.
		 */
		GLCubeSubdivision::non_null_ptr_to_const_type d_cube_subdivision;

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


		//! Constructor.
		GLMultiResolutionReconstructedRaster(
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type &reconstructing_polygons,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager);


		void
		generate_polygon_mesh(
				RotationGroup &rotation_group,
				const source_polygon_region_type::non_null_ptr_to_const_type &src_polygon_region);

		void
		initialise_cube_quad_trees();

		void
		create_clip_texture();

		unsigned int
		get_level_of_detail(
				const GLTransformState &transform_state) const;

		boost::optional<QuadTreeNode::non_null_ptr_type>
		create_quad_tree_node(
				GLCubeSubdivision::CubeFaceType cube_face,
				const partition_mesh_seq_type &parent_partitioned_meshes,
				const GLMultiResolutionCubeRaster::QuadTreeNode &source_raster_quad_tree_node,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset);

		void
		render_quad_tree(
				GLRenderer &renderer,
				const QuadTreeNode &quad_tree_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLTransformState::FrustumPlanes &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_node_tile(
				GLRenderer &renderer,
				const QuadTreeNode &quad_tree_node);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTER_H
