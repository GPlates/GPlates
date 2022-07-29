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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTERINTERFACE_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTERINTERFACE_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <qopengl.h>  // For OpenGL constants and typedefs.

#include "GLMatrix.h"
#include "GLTexture.h"

#include "maths/CubeCoordinateFrame.h"

#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GL;

	/**
	 * Interface for any raster data in a multi-resolution cube map.
	 *
	 * For example this could be a regular raster or a reconstructed raster.
	 */
	class GLMultiResolutionCubeRasterInterface :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionCubeRasterInterface>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeRasterInterface.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeRasterInterface> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeRasterInterface.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeRasterInterface> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular tile of this raster.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Used during traversal of the raster cube quad tree to obtain quad tree node texture tiles.
		 */
		class QuadTreeNode
		{
		public:
			// Forward declaration.
			class ImplInterface;

			//! Constructor.
			explicit
			QuadTreeNode(
					const GPlatesUtils::non_null_intrusive_ptr<ImplInterface> &impl) :
				d_impl(impl)
			{  }

			/**
			 * Returns true if this quad tree node is at the highest resolution.
			 *
			 * In other words a resolution high enough to capture the full resolution of the source raster.
			 *
			 * If true is returned then this quad tree node will have *no* children, otherwise
			 * it will have one or more children depending on which child nodes are covered by the
			 * source raster (eg, if the source raster is non-global).
			 *
			 * NOTE: Some derived classes have no leaf nodes and hence never terminate.
			 * For these types 'false' is always returned.
			 */
			bool
			is_leaf_node() const
			{
				return d_impl->is_leaf_node();
			}

			/**
			 * Returns the specified tile's texture or boost::none if there's no texture for this node
			 * (for example there was no raster covering the node's tile).
			 *
			 * Note that for derived class @a GLMultiResolutionCubeRaster this will always return
			 * a valid tile texture since there will be no quad tree nodes over regions where there
			 * is not raster coverage.
			 *
			 * @a gl is used if the tile's texture is not currently cached and needs to be re-rendered.
			 *
			 * @a cache_handle is to be stored by the client to keep textures (and vertices) cached.
			 */
			boost::optional<GLTexture::shared_ptr_type>
			get_tile_texture(
					GL &gl,
					cache_handle_type &cache_handle) const
			{
				return d_impl->get_tile_texture(gl, cache_handle);
			}

		private:
			GPlatesUtils::non_null_intrusive_ptr<ImplInterface> d_impl;

		public: // For derived parent classes to implement...

			//! Implementation of quad tree node to be implemented by derived parent classes.
			class ImplInterface :
					public GPlatesUtils::ReferenceCount<ImplInterface>
			{
			public:
				virtual
				~ImplInterface()
				{  }

				virtual
				bool
				is_leaf_node() const = 0;

				virtual
				boost::optional<GLTexture::shared_ptr_type>
				get_tile_texture(
						GL &gl,
						cache_handle_type &cache_handle) const = 0;
			};

			ImplInterface &
			get_impl() const
			{
				return *d_impl;
			}
		};

		//! Typedef for a quad tree node.
		typedef QuadTreeNode quad_tree_node_type;


		virtual
		~GLMultiResolutionCubeRasterInterface()
		{  }


		/**
		 * Gets the transform that is applied to raster/geometries when rendering into the cube map.
		 *
		 * The initial (default) transform is the identity transform.
		 */
		virtual
		const GLMatrix &
		get_world_transform() const = 0;


		/**
		 * Sets the transform to apply to raster/geometries when rendering into the cube map.
		 *
		 * This also invalidates all cached tile textures (if any) such that they will get regenerated
		 * (if needed) the next time 'QuadTreeNode::get_tile_texture()' is called on all and any tiles.
		 *
		 * The main use for this method currently is to rotate the cube map to align it with the
		 * central meridian used in the map-projections (for the 2D map view as opposed to 3D globe view).
		 *
		 * The initial (default) transform is the identity transform.
		 *
		 * NOTE: This can invalidate the quad tree nodes (@a quad_tree_node_type) if the transform is different.
		 *       Currently this is the case for derived class @a GLMultiResolutionCubeRaster.
		 *       This shouldn't matter as long as you don't call @a set_world_transform in the middle of a cube quad tree traversal.
		 *       Typically you'd start a fresh traversal at the top by calling @a get_quad_tree_root_node for each cube face
		 *       after calling @a set_world_transform.
		 */
		virtual
		void
		set_world_transform(
				const GLMatrix &world_transform) = 0;


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		virtual
		const GPlatesUtils::SubjectToken &
		get_subject_token() const = 0;


		/**
		 * Returns the quad tree root node of the specified cube face.
		 *
		 * Returns boost::none if the source raster does not overlap the specified cube face.
		 */
		virtual
		boost::optional<quad_tree_node_type>
		get_quad_tree_root_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) = 0;


		/**
		 * Returns the specified child cube quad tree node of specified parent node.
		 *
		 * Returns boost::none if the source raster does not overlap the specified child node.
		 */
		virtual
		boost::optional<quad_tree_node_type>
		get_child_node(
				const quad_tree_node_type &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset) = 0;


		/**
		 * Returns the tile texel dimension.
		 */
		virtual
		unsigned int
		get_tile_texel_dimension() const = 0;


		/**
		 * Returns the texture internal format that can be used if rendering to a texture as
		 * opposed to the main framebuffer.
		 */
		virtual
		GLint
		get_tile_texture_internal_format() const = 0;


		/**
		 * Returns the optional texture swizzle for the alpha channel (GL_TEXTURE_SWIZZLE_A).
		 *
		 * If not specified then the alpha swizzle is unchanged (ie, alpha value comes from alpha channel).
		 * This is useful for data (RG) rasters where the data value is in the Red channel and the coverage (alpha)
		 * value is in the Green channel (in which case a swizzle of GL_GREEN copies the green channel to alpha channel).
		 */
		virtual
		boost::optional<GLenum>
		get_tile_texture_swizzle_alpha() const = 0;


		/**
		 * Returns true if the raster is displayed visually (as opposed to a data raster used
		 * for numerical calculations).
		 *
		 * This is used to determine texture filtering for optimal display.
		 */
		virtual
		bool
		tile_texture_is_visual() const = 0;
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTERINTERFACE_H
