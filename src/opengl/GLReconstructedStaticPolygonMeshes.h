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

#ifndef GPLATES_OPENGL_GLRECONSTRUCTEDSTATICPOLYGONMESHES_H
#define GPLATES_OPENGL_GLRECONSTRUCTEDSTATICPOLYGONMESHES_H

#include <map>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/ref.hpp>

#include "GLCubeSubdivisionCache.h"
#include "GLDrawable.h"
#include "GLTransform.h"
#include "GLTransformState.h"
#include "GLVertexArray.h"
#include "GLVertexBufferResource.h"
#include "GLVertexElementArray.h"

#include "app-logic/ReconstructContext.h"
#include "app-logic/ReconstructMethodFiniteRotation.h"

#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/PolygonMesh.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A raster that is reconstructed by mapping it onto a set of present-day static polygons and
	 * reconstructing the polygons (and hence partitioned pieces of the raster).
	 */
	class GLReconstructedStaticPolygonMeshes :
			public GPlatesUtils::ReferenceCount<GLReconstructedStaticPolygonMeshes>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLReconstructedStaticPolygonMeshes.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLReconstructedStaticPolygonMeshes> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLReconstructedStaticPolygonMeshes.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLReconstructedStaticPolygonMeshes> non_null_ptr_to_const_type;

		//! Typedef for a sequence of @a PolygonMesh objects.
		typedef std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> >
				polygon_mesh_seq_type;

		//! Typedef for a sequence of geometries.
		typedef std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometries_seq_type;

		//! Typedef for a handle (index) into a sequence of present day polygon meshes.
		typedef unsigned int present_day_polygon_mesh_handle_type;

		//! Typedef for a spatial partition of reconstructed feature geometries.
		typedef GPlatesMaths::CubeQuadTreePartition<GPlatesAppLogic::ReconstructContext::Reconstruction>
				reconstructions_spatial_partition_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of projection transforms of quad tree nodes.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/>
						cube_subdivision_projection_transforms_cache_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of loose bounds of quad tree nodes.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				false/*CacheProjectionTransform*/,
				false/*CacheBounds*/,
				true/*CacheLooseBounds*/>
						cube_subdivision_loose_bounds_cache_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of bounding polygons of quad tree nodes.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				false/*CacheProjectionTransform*/,
				false/*CacheBounds*/,
				false/*CacheLooseBounds*/,
				true/*CacheBoundingPolygon*/>
						cube_subdivision_bounding_polygons_cache_type;


		/**
		 * Represents the boolean membership state of present day polygon meshes.
		 *
		 * It's currently used to represent those polygon meshes with the same transform.
		 * It's also used to represent those polygon meshes that possibly intersect a cube quad tree node.
		 */
		class PresentDayPolygonMeshMembership :
				public GPlatesUtils::ReferenceCount<PresentDayPolygonMeshMembership>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<PresentDayPolygonMeshMembership> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const PresentDayPolygonMeshMembership> non_null_ptr_to_const_type;


			//! Initially no polygon meshes are in the membership set to false.
			static
			non_null_ptr_type
			create(
					unsigned int num_polygon_meshes)
			{
				return non_null_ptr_type(new PresentDayPolygonMeshMembership(num_polygon_meshes));
			}

			//! Used to directly set the polygon meshes membership.
			static
			non_null_ptr_type
			create(
					const boost::dynamic_bitset<> &polygon_meshes_membership)
			{
				return non_null_ptr_type(new PresentDayPolygonMeshMembership(polygon_meshes_membership));
			}


			/**
			 * Returns a bitset that can be indexed by @a present_day_polygon_mesh_handle_type
			 * to test for membership.
			 *
			 * Or it can be compared against another bitset of the same size (ie, same total number of polygon meshes).
			 */
			const boost::dynamic_bitset<> &
			get_polygon_meshes_membership() const
			{
				return d_polygon_meshes_membership;
			}

			/**
			 * Adds the specified polygon mesh as a member.
			 */
			void
			add_present_day_polygon_mesh(
					present_day_polygon_mesh_handle_type present_day_polygon_mesh_handle)
			{
				d_polygon_meshes_membership.set(present_day_polygon_mesh_handle);
			}

		private:
			boost::dynamic_bitset<> d_polygon_meshes_membership;

			//! Constructor sets flags, for all polygon meshes, to false.
			explicit
			PresentDayPolygonMeshMembership(
					unsigned int num_polygon_meshes) :
				d_polygon_meshes_membership(num_polygon_meshes)
			{  }

			//! Constructor sets flags to match @a polygon_meshes_membership.
			explicit
			PresentDayPolygonMeshMembership(
					const boost::dynamic_bitset<> &polygon_meshes_membership) :
				d_polygon_meshes_membership(polygon_meshes_membership)
			{  }
		};


		/**
		 * Keeps track of which present day polygon meshes intersect which cube quad tree nodes.
		 *
		 * This boolean state is conservative in that it can be true even if a polygon mesh
		 * does *not* intersect the node. But it won't be false if a polygon mesh *does*
		 * intersect the node.
		 */
		class PresentDayPolygonMeshesNodeIntersections :
				private boost::noncopyable
		{
		public:
			//! Opaque structure to store implementation details at each node of cube quad tree.
			class Node
			{
			private:
				PresentDayPolygonMeshMembership::non_null_ptr_type d_polygon_mesh_membership;
				unsigned int d_quad_tree_depth;

				Node(
						const PresentDayPolygonMeshMembership::non_null_ptr_type &polygon_mesh_membership,
						unsigned int quad_tree_depth) :
					d_polygon_mesh_membership(polygon_mesh_membership),
					d_quad_tree_depth(quad_tree_depth)
				{  }

				friend class PresentDayPolygonMeshesNodeIntersections;
			};

			//! Typedef for an opaque cube quad tree with a polygon mesh membership for each node.
			typedef GPlatesMaths::CubeQuadTree<Node> intersection_partition_type;


			//! Constructor.
			explicit
			PresentDayPolygonMeshesNodeIntersections(
					unsigned int num_polygon_meshes) :
				d_intersection_partition(intersection_partition_type::create()),
				d_num_polygon_meshes(num_polygon_meshes)
			{  }

			//! Returns the quad tree root node, or NULL if no polygon meshes intersect cube face.
			const intersection_partition_type::node_type *
			get_quad_tree_root_node(
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const
			{
				return d_intersection_partition->get_quad_tree_root_node(cube_face);
			}

			//! Returns child of specified node, or NULL if no polygon meshes intersect child node.
			const intersection_partition_type::node_type *
			get_child_node(
					const intersection_partition_type::node_type &parent_node,
					unsigned int child_x_offset,
					unsigned int child_y_offset) const;

			//! Returns the polygon mesh membership given an opaque node.
			const PresentDayPolygonMeshMembership &
			get_intersecting_polygon_meshes(
					const intersection_partition_type::node_type &node) const
			{
				return *node.get_element().d_polygon_mesh_membership;
			}

			//! Gets, and creates if necessary, a root quad tree node.
			intersection_partition_type::node_type &
			get_or_create_quad_tree_root_node(
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face);

			//! Gets, and creates if necessary, a child node.
			intersection_partition_type::node_type &
			get_or_create_child_node(
					intersection_partition_type::node_type &parent_node,
					unsigned int child_x_offset,
					unsigned int child_y_offset);

			//! Returns the polygon mesh membership so it can be modified.
			PresentDayPolygonMeshMembership &
			get_intersecting_polygon_meshes(
					intersection_partition_type::node_type &node)
			{
				return *node.get_element().d_polygon_mesh_membership;
			}

			/**
			 * Returns true if the specified node is at the maximum depth.
			 *
			 * Nodes can not be created deeper than this.
			 */
			bool
			is_node_at_maximum_depth(
					const intersection_partition_type::node_type &node) const
			{
				return node.get_element().d_quad_tree_depth == MAXIMUM_DEPTH;
			}

		private:
			/**
			 * The maximum depth of the polygon meshes intersection cube quad tree.
			 */
			static const unsigned int MAXIMUM_DEPTH = 6;

			intersection_partition_type::non_null_ptr_type d_intersection_partition;
			unsigned int d_num_polygon_meshes;
		};


		/**
		 * Contains all reconstructed polygon meshes that have the same transform.
		 */
		class ReconstructedPolygonMeshTransformGroup
		{
		public:
			/**
			 * Constructor.
			 *
			 * NOTE: @a num_polygon_meshes is the total number of *all*
			 * present day polygon meshes and not just the number in this group.
			 */
			ReconstructedPolygonMeshTransformGroup(
					const GLTransform::non_null_ptr_to_const_type &finite_rotation,
					unsigned int num_polygon_meshes) :
				d_finite_rotation(finite_rotation),
				d_present_day_polygon_meshes_for_active_reconstructions(
						PresentDayPolygonMeshMembership::create(num_polygon_meshes)),
				d_present_day_polygon_meshes_for_active_or_inactive_reconstructions(
						PresentDayPolygonMeshMembership::create(num_polygon_meshes))
			{  }

			/**
			 * Returns the finite rotation for all reconstructed polygon meshes in this transform group.
			 */
			const GLTransform::non_null_ptr_to_const_type &
			get_finite_rotation() const
			{
				return d_finite_rotation;
			}

			/**
			 * Returns those polygon meshes that are reconstructed by this transform group.
			 */
			const PresentDayPolygonMeshMembership &
			get_present_day_polygon_meshes_for_active_reconstructions() const
			{
				return *d_present_day_polygon_meshes_for_active_reconstructions;
			}

			/**
			 * Returns those polygon meshes that are reconstructed by this transform group.
			 *
			 * NOTE: This includes reconstructing features that are not active (or not defined)
			 * for the current reconstruction time.
			 *
			 * The returned membership will be empty if 'GLReconstructedStaticPolygonMeshes::update()'
			 * is called without the 'active_or_inactive_reconstructions_spatial_partition' argument.
			 */
			const PresentDayPolygonMeshMembership &
			get_present_day_polygon_meshes_for_active_or_inactive_reconstructions() const
			{
				return *d_present_day_polygon_meshes_for_active_or_inactive_reconstructions;
			}

			/**
			 * Adds the specified polygon mesh to this transform group.
			 */
			void
			add_present_day_polygon_mesh_for_active_reconstruction(
					present_day_polygon_mesh_handle_type present_day_polygon_mesh_handle)
			{
				d_present_day_polygon_meshes_for_active_reconstructions->add_present_day_polygon_mesh(present_day_polygon_mesh_handle);
			}

			/**
			 * Adds the specified polygon mesh to this transform group.
			 *
			 * NOTE: This includes reconstructing features that are not active (or not defined)
			 * for the current reconstruction time.
			 */
			void
			add_present_day_polygon_mesh_for_active_or_inactive_reconstruction(
					present_day_polygon_mesh_handle_type present_day_polygon_mesh_handle)
			{
				d_present_day_polygon_meshes_for_active_or_inactive_reconstructions->add_present_day_polygon_mesh(present_day_polygon_mesh_handle);
			}

		private:
			GLTransform::non_null_ptr_to_const_type d_finite_rotation;
			PresentDayPolygonMeshMembership::non_null_ptr_type d_present_day_polygon_meshes_for_active_reconstructions;
			PresentDayPolygonMeshMembership::non_null_ptr_type d_present_day_polygon_meshes_for_active_or_inactive_reconstructions;
		};

		//! Typedef for a sequence of reconstructed polygon mesh transform groups.
		typedef std::vector<ReconstructedPolygonMeshTransformGroup>
				reconstructed_polygon_mesh_transform_group_seq_type;

		/**
		 * Contains all reconstructed polygon meshes for all transforms.
		 */
		class ReconstructedPolygonMeshTransformsGroups :
				public GPlatesUtils::ReferenceCount<ReconstructedPolygonMeshTransformsGroups>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedPolygonMeshTransformsGroups> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedPolygonMeshTransformsGroups> non_null_ptr_to_const_type;


			//! Initially no polygon meshes are in the membership set.
			static
			non_null_ptr_type
			create(
					const reconstructed_polygon_mesh_transform_group_seq_type &transform_groups,
					unsigned int num_polygon_meshes)
			{
				return non_null_ptr_type(new ReconstructedPolygonMeshTransformsGroups(transform_groups, num_polygon_meshes));
			}


			/**
			 * Returns the sequence of transforms groups for the reconstructed polygon meshes.
			 */
			const reconstructed_polygon_mesh_transform_group_seq_type &
			get_transform_groups() const
			{
				return d_transform_groups;
			}

			/**
			 * Returns the present-day polygon meshes for *all* transform groups.
			 */
			const PresentDayPolygonMeshMembership &
			get_present_day_polygon_meshes_for_active_reconstructions() const
			{
				return *d_present_day_polygon_meshes_for_active_reconstructions;
			}

			/**
			 * Returns the present-day polygon meshes for *all* transform groups.
			 *
			 * NOTE: This includes reconstructing features that are not active (or not defined)
			 * for the current reconstruction time.
			 *
			 * The returned membership will be empty if 'GLReconstructedStaticPolygonMeshes::update()'
			 * is called without the 'active_or_inactive_reconstructions_spatial_partition' argument.
			 */
			const PresentDayPolygonMeshMembership &
			get_present_day_polygon_meshes_for_active_or_inactive_reconstructions() const
			{
				return *d_present_day_polygon_meshes_for_active_or_inactive_reconstructions;
			}

		private:
			reconstructed_polygon_mesh_transform_group_seq_type d_transform_groups;

			PresentDayPolygonMeshMembership::non_null_ptr_type d_present_day_polygon_meshes_for_active_reconstructions;
			PresentDayPolygonMeshMembership::non_null_ptr_type d_present_day_polygon_meshes_for_active_or_inactive_reconstructions;

			/**
			 * Constructor sets present day polygon meshes flags to union of flags of @a transform_groups.
			 */
			ReconstructedPolygonMeshTransformsGroups(
					const reconstructed_polygon_mesh_transform_group_seq_type &transform_groups,
					unsigned int num_polygon_meshes) :
				d_transform_groups(transform_groups),
				d_present_day_polygon_meshes_for_active_reconstructions(
						gather_present_day_polygon_mesh_memberships_for_active_reconstructions(
								transform_groups, num_polygon_meshes)),
				d_present_day_polygon_meshes_for_active_or_inactive_reconstructions(
						gather_present_day_polygon_mesh_memberships_for_active_or_inactive_reconstructions(
								transform_groups, num_polygon_meshes))
			{  }

			static
			PresentDayPolygonMeshMembership::non_null_ptr_type
			gather_present_day_polygon_mesh_memberships_for_active_reconstructions(
					const reconstructed_polygon_mesh_transform_group_seq_type &transform_groups,
					unsigned int num_polygon_meshes);

			static
			PresentDayPolygonMeshMembership::non_null_ptr_type
			gather_present_day_polygon_mesh_memberships_for_active_or_inactive_reconstructions(
					const reconstructed_polygon_mesh_transform_group_seq_type &transform_groups,
					unsigned int num_polygon_meshes);
		};


		/**
		 * Typedef for a sequence of OpenGL drawables representing the present day polygon meshes.
		 *
		 * The sequence contains *optional* drawables - this is because it might not be possible
		 * to create polygon meshes for some present day geometries.
		 * These geometries won't be indexed by @a get_visible_reconstructed_polygon_meshes anyway.
		 *
		 * This sequence can be indexed by @a present_day_polygon_mesh_handle_type.
		 */
		typedef std::vector<boost::optional<GLDrawable::non_null_ptr_to_const_type> >
				present_day_polygon_mesh_drawables_seq_type;



		/**
		 * Creates a @a GLReconstructedStaticPolygonMeshes object.
		 *
		 * @param polygon_meshes the present day polygon meshes.
		 *        These should be indexable by ReconstructContext::Reconstruction.
		 * @param present_day_geometries the present day geometries associated with @a polygon_meshes.
		 *        These should also be indexable by ReconstructContext::Reconstruction.
		 * @param reconstructions_spatial_partition is the initial reconstructed feature geometries
		 *        for the current reconstruction time - it will get updated via @a update.
		 *
		 * NOTE: Use @a update to specify any *inactive* reconstructions (eg, when have an age grid).
		 */
		static
		non_null_ptr_type
		create(
				const polygon_mesh_seq_type &polygon_meshes,
				const geometries_seq_type &present_day_geometries,
				const reconstructions_spatial_partition_type::non_null_ptr_to_const_type &reconstructions_spatial_partition,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &cube_subdivision_projection_transforms_cache,
				const cube_subdivision_loose_bounds_cache_type::non_null_ptr_type &cube_subdivision_loose_bounds_cache,
				const cube_subdivision_bounding_polygons_cache_type::non_null_ptr_type &cube_subdivision_bounding_polygons_cache,
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager)
		{
			return non_null_ptr_type(
					new GLReconstructedStaticPolygonMeshes(
							polygon_meshes,
							present_day_geometries,
							reconstructions_spatial_partition,
							cube_subdivision_projection_transforms_cache,
							cube_subdivision_loose_bounds_cache,
							cube_subdivision_bounding_polygons_cache,
							vertex_buffer_resource_manager));
		}


		/**
		 * Returns the present day polygon mesh drawables.
		 *
		 * The returned sequence can be indexed by @a present_day_polygon_mesh_handle_type.
		 *
		 * NOTE: These meshes are constant for the lifetime of 'this' object.
		 */
		const present_day_polygon_mesh_drawables_seq_type &
		get_present_day_polygon_mesh_drawables() const
		{
			return d_present_day_polygon_mesh_drawables;
		}


		/**
		 * Returns the cube quad tree that represents which present day polygon meshes intersect
		 * which cube quad tree nodes.
		 *
		 * This boolean state is conservative in that it can be true even if a polygon mesh
		 * does *not* intersect a node. But it won't be false if a polygon mesh *does*
		 * intersect a node.
		 *
		 * NOTE: The returned intersection cube quad tree is constant for the lifetime of 'this' object.
		 */
		const PresentDayPolygonMeshesNodeIntersections &
		get_present_day_polygon_mesh_node_intersections() const
		{
			return d_present_day_polygon_meshes_node_intersections;
		}


		/**
		 * Updates the reconstructed static polygons corresponding to the present day polygons
		 * passed into the constructor.
		 *
		 * NOTE: If the present day polygons have changed then a new
		 * @a GLReconstructedStaticPolygonMeshes object should be created instead of continuing
		 * to use 'this' one.
		 *
		 * NOTE: With @a reconstructions_spatial_partition since some polygon features might be
		 * inactive at the current reconstruction time not every present day polygons will be
		 * represented by a reconstructed feature geometry.
		 *
		 * NOTE: If @a active_or_inactive_reconstructions_spatial_partition is specified then it
		 * represents features that have been reconstructed even if they are inactive (or not
		 * defined at the current reconstruction time).
		 *
		 * @a active_or_inactive_reconstructions_spatial_partition is optional since it's only needed
		 * when reconstructing raster with the aid of an age grid (because the age grid determines
		 * when to mask off oceanic raster, not the begin time of the reconstructing polygons).
		 */
		void
		update(
				const reconstructions_spatial_partition_type::non_null_ptr_to_const_type &reconstructions_spatial_partition,
				boost::optional<reconstructions_spatial_partition_type::non_null_ptr_to_const_type>
						active_or_inactive_reconstructions_spatial_partition = boost::none);


		/**
		 * Returns the visible reconstructed feature geometries grouped by finite rotation transforms
		 * along with the present day OpenGL polygon meshes.
		 *
		 * The visibility is determined by the view frustum that is in turn determined by the
		 * specified transform state.
		 */
		ReconstructedPolygonMeshTransformsGroups::non_null_ptr_to_const_type
		get_visible_reconstructed_polygon_meshes(
				const GLTransformState &transform_state);

	private:
		/**
		 * Typedef for mapping finite rotations to a group of reconstructed polygon meshes.
		 *
		 * We need boost::reference_wrapper because we need to compare the finite rotation object itself
		 * rather that the non_null_ptr_type.
		 */
		typedef std::map<
				boost::reference_wrapper<const GPlatesAppLogic::ReconstructMethodFiniteRotation>,
				reconstructed_polygon_mesh_transform_group_seq_type::size_type>
						reconstructed_polygon_mesh_transform_group_map_type;


		/**
		 * Used to retrieve projection transforms and frustums during cube quad tree visitation.
		 */
		cube_subdivision_projection_transforms_cache_type::non_null_ptr_type d_cube_subdivision_projection_transforms_cache;

		/**
		 * Used to retrieve loose bounds during cube quad tree visitation.
		 */
		cube_subdivision_loose_bounds_cache_type::non_null_ptr_type d_cube_subdivision_loose_bounds_cache;

		/**
		 * Used to retrieve bounding small circles (from bounding polygons) during cube quad tree visitation.
		 */
		cube_subdivision_bounding_polygons_cache_type::non_null_ptr_type d_cube_subdivision_bounding_polygons_cache;

		/**
		 * Used for creating vertex arrays and vertex element arrays for the polygon meshes.
		 */
		GLVertexBufferResourceManager::shared_ptr_type d_vertex_buffer_resource_manager;

		/**
		 * All polygon mesh drawables share a single vertex array.
		 */
		GLVertexArray::shared_ptr_type d_polygon_meshes_vertex_array;

		/**
		 * All polygon mesh drawables share a single vertex element array.
		 */
		GLVertexElementArray::shared_ptr_type d_polygon_meshes_vertex_element_array;

		/**
		 * The OpenGL drawables representing each present day polygon mesh.
		 */
		present_day_polygon_mesh_drawables_seq_type d_present_day_polygon_mesh_drawables;

		/**
		 * Boolean state of intersection of present day polygon meshes with cube quad tree nodes.
		 */
		PresentDayPolygonMeshesNodeIntersections d_present_day_polygon_meshes_node_intersections;

		/**
		 * The reconstructed feature geometries for the current reconstruction time.
		 */
		reconstructions_spatial_partition_type::non_null_ptr_to_const_type d_reconstructions_spatial_partition;

		/**
		 * The reconstructed feature geometries for the current reconstruction time even if
		 * the features (that they were reconstructed from) are no active (or not defined) at
		 * the reconstruction time.
		 *
		 * This is optional since it's not needed by clients in all situations (only needed when
		 * reconstructing raster *with* the aid of an age grid).
		 */
		boost::optional<reconstructions_spatial_partition_type::non_null_ptr_to_const_type>
				d_active_or_inactive_reconstructions_spatial_partition;


		//! Constructor.
		GLReconstructedStaticPolygonMeshes(
				const polygon_mesh_seq_type &polygon_meshes,
				const geometries_seq_type &present_day_geometries,
				const reconstructions_spatial_partition_type::non_null_ptr_to_const_type &reconstructions_spatial_partition,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &cube_subdivision_projection_transforms_cache,
				const cube_subdivision_loose_bounds_cache_type::non_null_ptr_type &cube_subdivision_loose_bounds_cache,
				const cube_subdivision_bounding_polygons_cache_type::non_null_ptr_type &cube_subdivision_bounding_polygons_cache,
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager);


		/**
		 * Get the reconstructions that are visible in the view frustum for the specified
		 * spatial partition node.
		 */
		void
		get_visible_reconstructed_polygon_meshes_from_quad_tree(
				reconstructed_polygon_mesh_transform_group_seq_type &reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group_map_type &reconstructed_polygon_mesh_transform_group_map,
				unsigned int num_polygon_meshes,
				const reconstructions_spatial_partition_type::const_node_reference_type &reconstructions_quad_tree_node,
				const reconstructions_spatial_partition_type::const_node_reference_type &active_or_inactive_reconstructions_quad_tree_node,
				const cube_subdivision_loose_bounds_cache_type::node_reference_type &loose_bounds_quad_tree_node,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);


		/**
		 * Adds a sequence of polygon meshes belonging to the cube root or to a quad tree node
		 * that are visible in the view frustum when reconstructed.
		 */
		void
		add_visible_reconstructed_polygon_meshes(
				reconstructed_polygon_mesh_transform_group_seq_type &reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group_map_type &reconstructed_polygon_mesh_transform_group_map,
				unsigned int num_polygon_meshes,
				const reconstructions_spatial_partition_type::element_const_iterator &begin_reconstructions,
				const reconstructions_spatial_partition_type::element_const_iterator &end_reconstructions,
				bool active_reconstructions_only);


		/**
		 * Creates the vertex array and vertex element array of the polygon meshes and
		 * wraps each individual polygon mesh in a drawable.
		 */
		void
		create_polygon_mesh_drawables(
				const polygon_mesh_seq_type &polygon_meshes);

		/**
		 * For each present day polygon mesh determines, and records, which nodes of the
		 * cube quad tree node it possibly intersects.
		 */
		void
		find_present_day_polygon_mesh_node_intersections(
				const geometries_seq_type &present_day_geometries,
				const polygon_mesh_seq_type &polygon_meshes);

		void
		find_present_day_polygon_mesh_node_intersections(
				const present_day_polygon_mesh_handle_type present_day_polygon_mesh_handle,
				unsigned int num_polygon_meshes,
				const GPlatesMaths::PolygonMesh &polygon_mesh,
				const boost::optional<const GPlatesMaths::BoundingSmallCircle &> &polygon_mesh_bounding_small_circle,
				const std::vector<unsigned int> &polygon_mesh_parent_triangle_indices,
				PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type &intersections_quad_tree_node,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
				const cube_subdivision_bounding_polygons_cache_type::node_reference_type &bounding_polygons_cache_node,
				unsigned int current_depth);
	};
}

#endif // GPLATES_OPENGL_GLRECONSTRUCTEDSTATICPOLYGONMESHES_H
