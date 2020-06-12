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

#ifndef GPLATES_OPENGL_GLCUBESUBDIVISIONCACHE_H
#define GPLATES_OPENGL_GLCUBESUBDIVISIONCACHE_H

#include <boost/mpl/if.hpp>
#include <boost/optional.hpp>

#include "GLCubeSubdivision.h"

#include "global/PreconditionViolationError.h"

#include "maths/CubeQuadTree.h"

#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	// Policy classes - two classes for each method in @a GLCubeSubdivision.
	// One class is empty and the other contains the results of the specific method.
	// These classes contain the data that is cached.
	// All constructors take the same arguments which specify the location in the cube subdivision.
	// The template parameters of @a GLCubeSubdivisionCache determine what data is cached.
	namespace Implementation
	{
		struct NoProjectionTransform
		{
		};
		struct ProjectionTransform
		{
			boost::optional<GLTransform::non_null_ptr_to_const_type> projection_transform;
		};

		struct NoLooseProjectionTransform
		{
		};
		struct LooseProjectionTransform
		{
			boost::optional<GLTransform::non_null_ptr_to_const_type> loose_projection_transform;
		};

		struct NoFrustum
		{
		};
		struct Frustum
		{
			boost::optional<GLFrustum> frustum;
		};

		struct NoLooseFrustum
		{
		};
		struct LooseFrustum
		{
			boost::optional<GLFrustum> loose_frustum;
		};

		struct NoBoundingPolygon
		{
		};
		struct BoundingPolygon
		{
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> bounding_polygon;
		};

		struct NoLooseBoundingPolygon
		{
		};
		struct LooseBoundingPolygon
		{
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> loose_bounding_polygon;
		};

		struct NoOrientedBoundingBox
		{
		};
		struct OrientedBoundingBox
		{
			boost::optional<GLIntersect::OrientedBoundingBox> oriented_bounding_box;
		};

		struct NoLooseOrientedBoundingBox
		{
		};
		struct LooseOrientedBoundingBox
		{
			boost::optional<GLIntersect::OrientedBoundingBox> loose_oriented_bounding_box;
		};
	}


	/**
	 * Caches queries made to @a GLCubeDivision.
	 *
	 * Typical use is to traverse the cube quad tree of this class in parallel with another
	 * cube quad tree and get cached queries of @a GLCubeSubdivision using the
	 * @a get_cached_element method to retrieve, for example, the projection matrix at
	 * a specific location in the cube subdivision.
	 *
	 * The boolean template parameters determine what aspects of class @a GLCubeSubdivision are cached.
	 */
	template <
		bool CacheProjectionTransform = false, bool CacheLooseProjectionTransform = false,
		bool CacheFrustum = false, bool CacheLooseFrustum = false,
		bool CacheBoundingPolygon = false, bool CacheLooseBoundingPolygon = false,
		bool CacheBounds = false, bool CacheLooseBounds = false>
	class GLCubeSubdivisionCache :
			public GPlatesUtils::ReferenceCount<
					GLCubeSubdivisionCache<
							CacheProjectionTransform, CacheLooseProjectionTransform,
							CacheFrustum, CacheLooseFrustum,
							CacheBoundingPolygon, CacheLooseBoundingPolygon,
							CacheBounds, CacheLooseBounds> >
	{
	public:
		//! Typedef for this class type.
		typedef GLCubeSubdivisionCache<
				CacheProjectionTransform, CacheLooseProjectionTransform,
				CacheFrustum, CacheLooseFrustum,
				CacheBoundingPolygon, CacheLooseBoundingPolygon,
				CacheBounds, CacheLooseBounds>
						subdivision_cache_type;

		//! A convenience typedef for a shared pointer to a non-const @a GLCubeSubdivisionCache.
		typedef GPlatesUtils::non_null_intrusive_ptr<subdivision_cache_type> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCubeSubdivisionCache.
		typedef GPlatesUtils::non_null_intrusive_ptr<const subdivision_cache_type> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLCubeSubdivisionCache object that caches the queries obtained
		 * from @a cube_subdivision.
		 *
		 * The default value for @a max_num_cached_elements effectively means no caching.
		 * This is useful if each node in the subdivision hierarchy is visited only once
		 * (in which case caching is of no benefit).
		 * In this case this class effectively becomes a traverser of @a GLCubeSubdivision.
		 */
		static
		non_null_ptr_type
		create(
				const GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
				unsigned int max_num_cached_elements = 1)
		{
			return non_null_ptr_type(new subdivision_cache_type(cube_subdivision, max_num_cached_elements));
		}


	private:
		/**
		 * The composite element that is cached in the cube subdivision.
		 */
		template <
				class ProjectionTransformPolicy, class LooseProjectionTransformPolicy,
				class FrustumPolicy, class LooseFrustumPolicy,
				class BoundingPolygonPolicy, class LooseBoundingPolygonPolicy,
				class BoundsPolicy, class LooseBoundsPolicy>
		class Element :
				public ProjectionTransformPolicy, public LooseProjectionTransformPolicy,
				public FrustumPolicy, public LooseFrustumPolicy,
				public BoundingPolygonPolicy, public LooseBoundingPolygonPolicy,
				public BoundsPolicy, public LooseBoundsPolicy
		{
		public:
			typedef ProjectionTransformPolicy projection_transform_policy_type;
			typedef LooseProjectionTransformPolicy loose_projection_transform_policy_type;
			typedef FrustumPolicy frustum_policy_type;
			typedef LooseFrustumPolicy loose_frustum_policy_type;
			typedef BoundingPolygonPolicy bounding_polygon_policy_type;
			typedef LooseBoundingPolygonPolicy loose_bounding_polygon_policy_type;
			typedef BoundsPolicy bounds_policy_type;
			typedef LooseBoundsPolicy loose_bounds_policy_type;
		};


		//! Typedef for cached element of cube subdivision.
		typedef Element<
				typename boost::mpl::if_c<CacheProjectionTransform,
						Implementation::ProjectionTransform,
						Implementation::NoProjectionTransform>::type,
				typename boost::mpl::if_c<CacheLooseProjectionTransform,
						Implementation::LooseProjectionTransform,
						Implementation::NoLooseProjectionTransform>::type,
				typename boost::mpl::if_c<CacheFrustum,
						Implementation::Frustum,
						Implementation::NoFrustum>::type,
				typename boost::mpl::if_c<CacheLooseFrustum,
						Implementation::LooseFrustum,
						Implementation::NoLooseFrustum>::type,
				typename boost::mpl::if_c<CacheBoundingPolygon,
						Implementation::BoundingPolygon,
						Implementation::NoBoundingPolygon>::type,
				typename boost::mpl::if_c<CacheLooseBoundingPolygon,
						Implementation::LooseBoundingPolygon,
						Implementation::NoLooseBoundingPolygon>::type,
				typename boost::mpl::if_c<CacheBounds,
						Implementation::OrientedBoundingBox,
						Implementation::NoOrientedBoundingBox>::type,
				typename boost::mpl::if_c<CacheLooseBounds,
						Implementation::LooseOrientedBoundingBox,
						Implementation::NoLooseOrientedBoundingBox>::type
				> element_type;


		//! Typedef for an object cache of @a element_type.
		typedef GPlatesUtils::ObjectCache<element_type> element_cache_type;

		//! Typedef for an object cache volatile object referencing an element.
		typedef typename element_cache_type::volatile_object_type volatile_element_type;

		//! Typedef for an object cache volatile pointer to an element.
		typedef typename element_cache_type::volatile_object_ptr_type volatile_element_ptr_type;

		//! Typedef for a cube quad tree of volatile element pointers.
		typedef GPlatesMaths::CubeQuadTree<volatile_element_ptr_type> cube_quad_tree_type;

		//! Typedef for a node of the cube quad tree of volatile element pointers.
		typedef typename cube_quad_tree_type::node_type cube_quad_tree_node_type;


	public:
		/**
		 * A reference, or handle, to a node of this cube subdivision.
		 */
		class NodeReference
		{
		public:
			//! Returns the cube face of the referenced node.
			GPlatesMaths::CubeCoordinateFrame::CubeFaceType
			get_cube_face() const
			{
				return d_cube_face;
			}

			//! Returns the level-of-detail (or quad tree depth).
			unsigned int
			get_level_of_detail() const
			{
				return d_level_of_detail;
			}

			//! Returns the tile 'u' offset.
			unsigned int
			get_tile_u_offset() const
			{
				return d_tile_u_offset;
			}

			//! Returns the tile 'v' offset.
			unsigned int
			get_tile_v_offset() const
			{
				return d_tile_v_offset;
			}

		private:
			cube_quad_tree_node_type *d_node;

			GPlatesMaths::CubeCoordinateFrame::CubeFaceType d_cube_face;
			unsigned int d_level_of_detail;
			unsigned int d_tile_u_offset;
			unsigned int d_tile_v_offset;


			NodeReference(
					cube_quad_tree_node_type &node,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				d_node(&node),
				d_cube_face(cube_face),
				d_level_of_detail(level_of_detail),
				d_tile_u_offset(tile_u_offset),
				d_tile_v_offset(tile_v_offset)
			{  }

			// Make friend so can construct.
			friend class GLCubeSubdivisionCache<
					CacheProjectionTransform, CacheLooseProjectionTransform,
					CacheFrustum, CacheLooseFrustum,
					CacheBoundingPolygon, CacheLooseBoundingPolygon,
					CacheBounds, CacheLooseBounds>;
		};

		//! Typedef for a reference to a cube quad tree node.
		typedef NodeReference node_reference_type;


		/**
		 * Returns the root node of the specified cube face quad tree (creates a root node
		 * if it doesn't exist).
		 */
		node_reference_type
		get_quad_tree_root_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
		{
			cube_quad_tree_node_type *node = d_cube_quad_tree->get_quad_tree_root_node(cube_face);
			if (!node)
			{
				// Node doesn't exist so create a new one with a volatile element.
				node = &d_cube_quad_tree->set_quad_tree_root_node(
						cube_face,
						d_element_cache->allocate_volatile_object());
			}

			return node_reference_type(
					*node,
					cube_face,
					0/*level_of_detail*/,
					0/*tile_u_offset*/,
					0/*tile_v_offset*/);
		}


		/**
		 * Returns a reference to the specified child node (creates a child node if it doesn't exist).
		 */
		node_reference_type
		get_child_node(
				const node_reference_type &node,
				unsigned int child_u_offset,
				unsigned int child_v_offset)
		{
			cube_quad_tree_node_type *child_node =
					node.d_node->get_child_node(child_u_offset, child_v_offset);
			if (!child_node)
			{
				// Node doesn't exist so create a new one with a volatile element.
				child_node = &d_cube_quad_tree->set_child_node(
						*node.d_node,
						child_u_offset,
						child_v_offset,
						d_element_cache->allocate_volatile_object());
			}

			return node_reference_type(
					*child_node,
					node.d_cube_face,
					node.d_level_of_detail + 1,
					2 * node.d_tile_u_offset + child_u_offset,
					2 * node.d_tile_v_offset + child_v_offset);
		}


		/**
		 * Returns the view transform of this cached element.
		 *
		 * Note that this is not cached like the projection transforms because it is
		 * constant across each cube face (so we only need to store six transforms).
		 */
		GLTransform::non_null_ptr_to_const_type
		get_view_transform(
				const node_reference_type &node) const
		{
			return d_view_transforms[node.get_cube_face()];
		}


		/**
		 * Returns the projection transform of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheProjectionTransform' is true.
		 */
		GLTransform::non_null_ptr_to_const_type
		get_projection_transform(
				const node_reference_type &node)
		{
			boost::optional<GLTransform::non_null_ptr_to_const_type> &projection_transform_opt =
					get_cached_element(node)->element_type::projection_transform_policy_type::projection_transform;
			// Calculate if not cached yet.
			if (!projection_transform_opt)
			{
				projection_transform_opt = d_cube_subdivision->get_projection_transform(
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return projection_transform_opt.get();
		}

		/**
		 * Returns the loose projection transform of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheLooseProjectionTransform' is true.
		 */
		GLTransform::non_null_ptr_to_const_type
		get_loose_projection_transform(
				const node_reference_type &node)
		{
			boost::optional<GLTransform::non_null_ptr_to_const_type> &loose_projection_transform_opt =
					get_cached_element(node)->element_type::loose_projection_transform_policy_type::loose_projection_transform;
			// Calculate if not cached yet.
			if (!loose_projection_transform_opt)
			{
				loose_projection_transform_opt = d_cube_subdivision->get_loose_projection_transform(
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return loose_projection_transform_opt.get();
		}


		/**
		 * Returns the view frustum of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheFrustum' is true.
		 */
		GLFrustum
		get_frustum(
				const node_reference_type &node)
		{
			boost::optional<GLFrustum> &frustum_opt =
					get_cached_element(node)->element_type::frustum_policy_type::frustum;
			// Calculate if not cached yet.
			if (!frustum_opt)
			{
				frustum_opt = d_cube_subdivision->get_frustum(
								node.get_cube_face(),
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return frustum_opt.get();
		}

		/**
		 * Returns the loose view frustum of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheLooseFrustum' is true.
		 */
		GLFrustum
		get_loose_frustum(
				const node_reference_type &node)
		{
			boost::optional<GLFrustum> &loose_frustum_opt =
					get_cached_element(node)->element_type::loose_frustum_policy_type::loose_frustum;
			// Calculate if not cached yet.
			if (!loose_frustum_opt)
			{
				loose_frustum_opt = d_cube_subdivision->get_loose_frustum(
								node.get_cube_face(),
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return loose_frustum_opt.get();
		}


		/**
		 * Returns the polygon boundary of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheBoundingPolygon' is true.
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		get_bounding_polygon(
				const node_reference_type &node)
		{
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &bounding_polygon_opt =
					get_cached_element(node)->element_type::bounding_polygon_policy_type::bounding_polygon;
			// Calculate if not cached yet.
			if (!bounding_polygon_opt)
			{
				bounding_polygon_opt = d_cube_subdivision->get_bounding_polygon(
								node.get_cube_face(),
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return bounding_polygon_opt.get();
		}

		/**
		 * Returns the loose polygon boundary of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheLooseBoundingPolygon' is true.
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		get_loose_bounding_polygon(
				const node_reference_type &node)
		{
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &loose_bounding_polygon_opt =
					get_cached_element(node)->element_type::loose_bounding_polygon_policy_type::loose_bounding_polygon;
			// Calculate if not cached yet.
			if (!loose_bounding_polygon_opt)
			{
				loose_bounding_polygon_opt = d_cube_subdivision->get_loose_bounding_polygon(
								node.get_cube_face(),
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return loose_bounding_polygon_opt.get();
		}


		/**
		 * Returns the oriented bounding box of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheBounds' is true.
		 */
		GLIntersect::OrientedBoundingBox
		get_oriented_bounding_box(
				const node_reference_type &node)
		{
			boost::optional<GLIntersect::OrientedBoundingBox> &oriented_bounding_box_opt =
					get_cached_element(node)->element_type::bounds_policy_type::oriented_bounding_box;
			// Calculate if not cached yet.
			if (!oriented_bounding_box_opt)
			{
				oriented_bounding_box_opt = d_cube_subdivision->get_oriented_bounding_box(
								node.get_cube_face(),
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return oriented_bounding_box_opt.get();
		}

		/**
		 * Returns the loose oriented bounding box of this cached element.
		 *
		 * NOTE: Only *compiles* if 'CacheLooseBounds' is true.
		 */
		GLIntersect::OrientedBoundingBox
		get_loose_oriented_bounding_box(
				const node_reference_type &node)
		{
			boost::optional<GLIntersect::OrientedBoundingBox> &loose_oriented_bounding_box_opt =
					get_cached_element(node)->element_type::loose_bounds_policy_type::loose_oriented_bounding_box;
			// Calculate if not cached yet.
			if (!loose_oriented_bounding_box_opt)
			{
				loose_oriented_bounding_box_opt = d_cube_subdivision->get_loose_oriented_bounding_box(
								node.get_cube_face(),
								node.get_level_of_detail(),
								node.get_tile_u_offset(),
								node.get_tile_v_offset());
			}

			return loose_oriented_bounding_box_opt.get();
		}

	private:
		/**
		 * The cube subdivision whose queries we're caching.
		 */
		GLCubeSubdivision::non_null_ptr_to_const_type d_cube_subdivision;

		/**
		 * The cached elements.
		 */
		typename element_cache_type::shared_ptr_type d_element_cache;

		/**
		 * The cube quad tree referencing the cached elements.
		 */
		typename cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		/**
		 * The view transform for each cube face.
		 */
		std::vector<GLTransform::non_null_ptr_to_const_type> d_view_transforms;


		GLCubeSubdivisionCache(
				const GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
				unsigned int max_num_cached_elements) :
			d_cube_subdivision(cube_subdivision),
			d_element_cache(element_cache_type::create(max_num_cached_elements)),
			d_cube_quad_tree(cube_quad_tree_type::create())
		{
			// Store the six view transforms (one for each cube face).
			for (unsigned int cube_face = 0; cube_face < 6; ++cube_face)
			{
				d_view_transforms.push_back(
						cube_subdivision->get_view_transform(
								static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(cube_face)));
			}
		}


		/**
		 * Returns the cached element for the specified cube quad tree node reference.
		 *
		 * The returned element cannot be recycled until the returned shared_ptr (and any copies)
		 * are destroyed.
		 */
		boost::shared_ptr<element_type>
		get_cached_element(
				const node_reference_type &node)
		{
			volatile_element_type &volatile_element = *node.d_node->get_element();

			boost::shared_ptr<element_type> element = volatile_element.get_cached_object();
			if (!element)
			{
				element = volatile_element.recycle_an_unused_object();
				if (element)
				{
					// Assign recycled element to a new cached value.
					*element = element_type();
				}
				else
				{
					// Create a new element and set it in the cache.
					element = volatile_element.set_cached_object(
							std::unique_ptr<element_type>(new element_type()));
				}
			}

			return element;
		}
	};
}

#endif // GPLATES_OPENGL_GLCUBESUBDIVISIONCACHE_H
