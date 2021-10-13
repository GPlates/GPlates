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
			NoProjectionTransform(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset)
			{  }
		};

		struct ProjectionTransform
		{
			ProjectionTransform(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				projection_transform(
						cube_subdivision.get_projection_transform(
								level_of_detail, tile_u_offset, tile_v_offset)),
				frustum(
						cube_subdivision.get_view_transform(cube_face)->get_matrix(),
						projection_transform->get_matrix())
			{  }

			//! The projection transform.
			GLTransform::non_null_ptr_to_const_type projection_transform;
			//! The view frustum - since it's dependent on projection we might as well cache it too at a little extra cost.
			GLFrustum frustum;
		};


		struct NoOrientedBoundingBox
		{
			NoOrientedBoundingBox(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset)
			{  }
		};

		struct OrientedBoundingBox
		{
			OrientedBoundingBox(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				oriented_bounding_box(
						cube_subdivision.get_oriented_bounding_box(
								cube_face, level_of_detail, tile_u_offset, tile_v_offset))
			{  }

			//! The oriented box bounding the current cube subdivision tile.
			GLIntersect::OrientedBoundingBox oriented_bounding_box;
		};


		struct NoLooseOrientedBoundingBox
		{
			NoLooseOrientedBoundingBox(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset)
			{  }
		};

		struct LooseOrientedBoundingBox
		{
			LooseOrientedBoundingBox(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				oriented_bounding_box(
						cube_subdivision.get_loose_oriented_bounding_box(
								cube_face, level_of_detail, tile_u_offset, tile_v_offset))
			{  }

			//! The loose oriented box bounding the current cube subdivision tile.
			GLIntersect::OrientedBoundingBox oriented_bounding_box;
		};


		struct NoBoundingPolygon
		{
			NoBoundingPolygon(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset)
			{  }
		};

		struct BoundingPolygon
		{
			BoundingPolygon(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				bounding_polygon(
						cube_subdivision.get_bounding_polygon(
								cube_face, level_of_detail, tile_u_offset, tile_v_offset))
			{  }

			//! The polygon boundary of the current cube subdivision tile.
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type bounding_polygon;
		};


		struct NoLooseBoundingPolygon
		{
			NoLooseBoundingPolygon(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset)
			{  }
		};

		struct LooseBoundingPolygon
		{
			LooseBoundingPolygon(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				loose_bounding_polygon(
						cube_subdivision.get_loose_bounding_polygon(
								cube_face, level_of_detail, tile_u_offset, tile_v_offset))
			{  }

			//! The polygon boundary of the loose bounds of the current cube subdivision tile.
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type loose_bounding_polygon;
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
	template <bool CacheProjectionTransform, bool CacheBounds = false, bool CacheLooseBounds = false,
			bool CacheBoundingPolygon = false, bool CacheLooseBoundingPolygon = false>
	class GLCubeSubdivisionCache :
			public GPlatesUtils::ReferenceCount<
					GLCubeSubdivisionCache<
							CacheProjectionTransform,
							CacheBounds,
							CacheLooseBounds,
							CacheBoundingPolygon,
							CacheLooseBoundingPolygon> >
	{
	public:
		//! Typedef for this class type.
		typedef GLCubeSubdivisionCache<
				CacheProjectionTransform,
				CacheBounds,
				CacheLooseBounds,
				CacheBoundingPolygon,
				CacheLooseBoundingPolygon>
						subdivision_cache_type;

		//! A convenience typedef for a shared pointer to a non-const @a GLCubeSubdivisionCache.
		typedef GPlatesUtils::non_null_intrusive_ptr<subdivision_cache_type> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCubeSubdivisionCache.
		typedef GPlatesUtils::non_null_intrusive_ptr<const subdivision_cache_type> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLCubeSubdivisionCache object that caches the queries obtained
		 * from @a cube_subdivision.
		 */
		static
		non_null_ptr_type
		create(
				const GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
				unsigned int max_num_cached_elements)
		{
			return non_null_ptr_type(new subdivision_cache_type(cube_subdivision, max_num_cached_elements));
		}


		/**
		 * The composite element that is cached in the cube subdivision.
		 */
		template <class ProjectionTransformPolicy, class BoundsPolicy, class LooseBoundsPolicy,
				class BoundingPolygonPolicy, class LooseBoundingPolygonPolicy>
		class Element :
				private ProjectionTransformPolicy,
				private BoundsPolicy,
				private LooseBoundsPolicy,
				private BoundingPolygonPolicy,
				private LooseBoundingPolygonPolicy
		{
		public:
			Element(
					const GLCubeSubdivision &cube_subdivision,
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset) :
				ProjectionTransformPolicy(cube_subdivision, cube_face, level_of_detail, tile_u_offset, tile_v_offset),
				BoundsPolicy(cube_subdivision, cube_face, level_of_detail, tile_u_offset, tile_v_offset),
				LooseBoundsPolicy(cube_subdivision, cube_face, level_of_detail, tile_u_offset, tile_v_offset),
				BoundingPolygonPolicy(cube_subdivision, cube_face, level_of_detail, tile_u_offset, tile_v_offset),
				LooseBoundingPolygonPolicy(cube_subdivision, cube_face, level_of_detail, tile_u_offset, tile_v_offset)
			{   }

			/**
			 * Returns the projection transform of this cached element.
			 *
			 * NOTE: Only *compiles* if 'CacheProjectionTransform' is true.
			 */
			const GLTransform::non_null_ptr_to_const_type &
			get_projection_transform() const
			{
				return ProjectionTransformPolicy::projection_transform;
			}

			/**
			 * Returns the view frustum of this cached element.
			 *
			 * NOTE: Only *compiles* if 'CacheProjectionTransform' is true.
			 */
			const GLFrustum &
			get_frustum() const
			{
				return ProjectionTransformPolicy::frustum;
			}

			/**
			 * Returns the oriented bounding box of this cached element.
			 *
			 * NOTE: Only *compiles* if 'CacheBounds' is true.
			 */
			const GLIntersect::OrientedBoundingBox &
			get_oriented_bounding_box() const
			{
				return BoundsPolicy::oriented_bounding_box;
			}

			/**
			 * Returns the loose oriented bounding box of this cached element.
			 *
			 * NOTE: Only *compiles* if 'CacheLooseBounds' is true.
			 */
			const GLIntersect::OrientedBoundingBox &
			get_loose_oriented_bounding_box() const
			{
				return LooseBoundsPolicy::oriented_bounding_box;
			}

			/**
			 * Returns the polygon boundary of this cached element.
			 *
			 * NOTE: Only *compiles* if 'CacheBoundingPolygon' is true.
			 */
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &
			get_bounding_polygon() const
			{
				return BoundingPolygonPolicy::bounding_polygon;
			}

			/**
			 * Returns the polygon loose boundary of this cached element.
			 *
			 * NOTE: Only *compiles* if 'CacheLooseBoundingPolygon' is true.
			 */
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &
			get_loose_bounding_polygon() const
			{
				return BoundingPolygonPolicy::loose_bounding_polygon;
			}
		};


		//! Typedef for cached element of cube subdivision.
		typedef Element<
				typename boost::mpl::if_c<CacheProjectionTransform,
						Implementation::ProjectionTransform,
						Implementation::NoProjectionTransform>::type,
				typename boost::mpl::if_c<CacheBounds,
						Implementation::OrientedBoundingBox,
						Implementation::NoOrientedBoundingBox>::type,
				typename boost::mpl::if_c<CacheLooseBounds,
						Implementation::LooseOrientedBoundingBox,
						Implementation::NoLooseOrientedBoundingBox>::type,
				typename boost::mpl::if_c<CacheBoundingPolygon,
						Implementation::BoundingPolygon,
						Implementation::NoBoundingPolygon>::type,
				typename boost::mpl::if_c<CacheLooseBoundingPolygon,
						Implementation::LooseBoundingPolygon,
						Implementation::NoLooseBoundingPolygon>::type
				> element_type;


	private:
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
					CacheProjectionTransform, CacheBounds, CacheLooseBounds, CacheBoundingPolygon, CacheLooseBoundingPolygon>;
		};

		//! Typedef for a reference to a cube quad tree node.
		typedef NodeReference node_reference_type;


		/**
		 * Returns the texel dimension of any tile at any subdivision level and uv offset.
		 */
		std::size_t
		get_tile_texel_dimension() const
		{
			return d_cube_subdivision->get_tile_texel_dimension();
		}


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
		 * Returns the cached element for the specified cube quad tree node reference.
		 *
		 * The returned element cannot be recycled until the returned shared_ptr (and any copies)
		 * are destroyed.
		 */
		boost::shared_ptr<const element_type>
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
					*element = element_type(
							*d_cube_subdivision,
							node.d_cube_face,
							node.d_level_of_detail,
							node.d_tile_u_offset,
							node.d_tile_v_offset);
				}
				else
				{
					// Create a new element and set it in the cache.
					element = volatile_element.set_cached_object(
							std::auto_ptr<element_type>(new element_type(
									*d_cube_subdivision,
									node.d_cube_face,
									node.d_level_of_detail,
									node.d_tile_u_offset,
									node.d_tile_v_offset)));
				}
			}

			return element;
		}


		/**
		 * Returns the view transform for the specified face.
		 *
		 * Note that this is not cached like the projection transforms because it is
		 * constant across each cube face (so we only need to store six transforms).
		 */
		GLTransform::non_null_ptr_to_const_type
		get_view_transform(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const
		{
			return d_view_transforms[cube_face];
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
	};
}

#endif // GPLATES_OPENGL_GLCUBESUBDIVISIONCACHE_H
