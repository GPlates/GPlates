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
 
#ifndef GPLATES_OPENGL_GLCUBESUBDIVISION_H
#define GPLATES_OPENGL_GLCUBESUBDIVISION_H

#include <opengl/OpenGL.h>

#include "GLFrustum.h"
#include "GLIntersectPrimitives.h"
#include "GLTransform.h"

#include "maths/CubeCoordinateFrame.h"
#include "maths/PolygonOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Defines a quad-tree subdivision of each face of a cube with optional overlapping extents.
	 */
	class GLCubeSubdivision :
			public GPlatesUtils::ReferenceCount<GLCubeSubdivision>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLCubeSubdivision.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLCubeSubdivision> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCubeSubdivision.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLCubeSubdivision> non_null_ptr_to_const_type;


		/**
		 * Calculates a frustum expand ratio (for use in @a create) for a tile of the specified
		 * texel dimension and the desired overlap in units of texels.
		 *
		 * For example to ensure the centres of border texels lie exactly on the frustum plane
		 * boundaries set @a frustum_border_overlap_in_texels to '0.5'.
		 *
		 * NOTE: This can also be used for 'loose' tiles that have the specified texel dimension
		 * covering the 'loose' tile with the desired overlap in units of texels.
		 */
		static
		double
		get_expand_frustum_ratio(
				unsigned int tile_texel_dimension,
				const double &frustum_border_overlap_in_texels)
		{
			return tile_texel_dimension / (tile_texel_dimension - 2.0 * frustum_border_overlap_in_texels);
		}


		/**
		 * Creates a @a GLCubeSubdivision object.
		 *
		 * @a expand_frustum_ratio is the factor used to scale a frustum.
		 * This can be used to, for example, ensure that a texture tile has the centres of its
		 * border texels lie exactly on the frustum boundaries (planes) which means adjacent tiles
		 * will have matching border texels (texture pixels).
		 * Since a texel has a centre sample point but is also a square area this means that
		 * the tile textures overlap slightly (by one texel).
		 * This is necessary if the tile textures are bilinearly filtered because it avoids
		 * seams, or discontinuities, across tiles - see
		 * http://hacksoflife.blogspot.com/2009/12/texture-coordinate-system-for-opengl.html
		 * for a good overview of this.
		 * NOTE: For 'loose' tiles the expand frustum ratio is doubled to account for the fact that
		 * 'loose' tiles are double the size. As it turns out the same expand frustum ratio works
		 * out this way for both non-loose and loose tiles if they both use the same texel dimension
		 * (ie, if the texels covering the 'loose' tile are twice as big as those for the non-loose tiles).
		 * For 'nearest texel' filtering this is not needed hence no frustum expansion is necessary
		 * (for either non-loose of loose tiles).
		 *
		 * @a zNear and @a zFar specify the near and far distance (positive) of
		 * the view frustums generated for each subdivision.
		 *
		 * NOTE: @a zNear is set close to zero but can't be set to zero since it's not possible
		 * to have a zero-distance view frustum projection matrix.
		 * However, by default, we set it close to zero in order to avoid clipping to the near plane
		 * when rendering (when rendering in OpenGL the view frustum clips to the four side planes
		 * and the far and near planes). This near plane clipping can happen if we render a geometry
		 * that has a very long arc between vertices (if this arc is not tessellated, to introduce
		 * more vertices, then the straight line segment between the two arc endpoints, which lie
		 * on the sphere, can actually intersect the part of the near plane that forms part of
		 * the convex view frustum) - when this happens it gets clipped - but by setting the near
		 * distance close to zero we can get large arcs near 180 degrees, the maximum allowed arc
		 * length, without any clipping.
		 * NOTE: This does make the depth buffer precision (for OpenGL rendering) quite inaccurate
		 * but pretty much all rendering is done on the sphere (or projected onto sphere) so no
		 * depth buffer is required (there's no depth complexity).
		 * If a depth buffer is required for cube rendering then the near parameter can be increased.
		 * NOTE: We don't make it too small in order to avoid numerical issues in projection matrices, etc.
		 * Just small enough that untessellated geometries (such as used in filled polygons) can
		 * span close to 180 degrees and miss the near-plane part of a view frustum. For example,
		 * 1/1000 the radius of the globe gives us a near plane section that is 1/1000 from the origin
		 * and has a square dimension of 2/1000 the radius of the globe.
		 * Also note that clipping to the sides of the frustum is ok and desired (clipping to the
		 * near plane section is not desired) - the far plane, as long as its outside the globe
		 * doesn't exhibit these tessellation problems because any tessellation of geometries on the
		 * sphere only bring the geometry closer to the origin (ie, closer to the near plane).
		 */
		static
		non_null_ptr_type
		create(
				const double &expand_frustum_ratio = 1.0,
				GLdouble zNear = 1e-3,
				GLdouble zFar = 2.0)
		{
			return non_null_ptr_type(new GLCubeSubdivision(expand_frustum_ratio, zNear, zFar));
		}


		/**
		 * Returns the view matrix used to render a scene into a subdivision tile.
		 *
		 * @param level_of_detail represents the level of subdivision (0 means the whole cube face).
		 * @param tile_u_offset represents the offset along the texture 'u' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 * @param tile_v_offset represents the offset along the texture 'v' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 */
		GLTransform::non_null_ptr_to_const_type
		get_view_transform(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const;


		/**
		 * Returns the projection matrix used to render a scene into a subdivision tile.
		 *
		 * @param level_of_detail represents the level of subdivision (0 means the whole cube face).
		 * @param tile_u_offset represents the offset along the texture 'u' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 * @param tile_v_offset represents the offset along the texture 'v' direction and
		 *        must be in the range [0, 2^level_of_detail).
		 *
		 * Note that if the frustum has been expanded (eg, half a texel) and if texture coordinates
		 * are calculated by transforming world position by this projection transform (along with
		 * the model-view transform) then they don't need to be explicitly adjusted (eg, half a texel)
		 * because it will naturally happen as a result of using the expanded projection transform
		 * as a texture transform.
		 */
		GLTransform::non_null_ptr_to_const_type
		get_projection_transform(
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			return create_projection_transform(
					level_of_detail,
					tile_u_offset,
					tile_v_offset,
					d_expanded_projection_scale);
		}


		/**
		 * Returns the loose projection matrix used to render a scene into a subdivision tile, but
		 * with the tile doubled in size (about the tile centre) on the plane of the cube face.
		 *
		 * In other words the tile size, on the cube face, is exactly double and the tile centre
		 * is the same as the centre of the regular tile at the same location.
		 *
		 * See class @a CubeQuadTreePartition for a detailed description on loose bounds.
		 */
		GLTransform::non_null_ptr_to_const_type
		get_loose_projection_transform(
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			return create_projection_transform(
					level_of_detail,
					tile_u_offset,
					tile_v_offset,
					// See http://www.opengl.org/resources/code/samples/sig99/advanced99/notes/node30.html
					// to help understand why the *inverse* ratio is used to scale the projection transform...
					0.5 * d_expanded_projection_scale);
		}


		/**
		 * Returns the six-plane frustum from the view-projection transform obtained from
		 * @a get_view_transform and @a get_projection_transform.
		 */
		GLFrustum
		get_frustum(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			return GLFrustum(
					get_view_transform(cube_face)->get_matrix(),
					get_projection_transform(level_of_detail, tile_u_offset, tile_v_offset)->get_matrix());
		}


		/**
		 * Returns the six-plane loose frustum from the view-projection transform obtained from
		 * @a get_view_transform and @a get_loose_projection_transform.
		 *
		 * Note that the loose tile size is double in size (about the tile centre) on the plane of the cube face.
		 */
		GLFrustum
		get_loose_frustum(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			return GLFrustum(
					get_view_transform(cube_face)->get_matrix(),
					get_loose_projection_transform(level_of_detail, tile_u_offset, tile_v_offset)->get_matrix());
		}


		/**
		 * Returns the polygon (on sphere) containing four great circle arcs that
		 * bound the specified subdivision tile.
		 *
		 * The returned polygon is clockwise when viewed from above the surface of the sphere.
		 *
		 * Because the cube is a gnomic projection (displays all great circles as straight lines)
		 * the projection of a subdivision frustum onto the sphere is bounded by four great
		 * circle arcs which is a polygon.
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		get_bounding_polygon(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			const FrustumCornerPoints corner_points(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset, d_expand_frustum_ratio);

			return create_bounding_polygon(corner_points);
		}


		/**
		 * Returns the polygon (on sphere) containing four great circle arcs that
		 * form the 'loose' bounds the specified subdivision tile.
		 *
		 * The loose bounds is the tile doubled in size (about the tile centre) on the plane
		 * of the cube face.
		 * In other words the tile size, on the cube face, is exactly double and the tile centre
		 * is the same as the centre of the regular tile at the same location.
		 * See class @a CubeQuadTreePartition for a detailed description on loose bounds.
		 *
		 * The returned polygon is clockwise when viewed from above the surface of the sphere.
		 *
		 * Because the cube is a gnomic projection (displays all great circles as straight lines)
		 * the projection of a subdivision frustum onto the sphere is bounded by four great
		 * circle arcs which is a polygon.
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		get_loose_bounding_polygon(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			// NOTE: Since this is a loose bounds we double the normal size of the tile which
			// involves subtracting/adding 'inv_num_subdivisions' compared to a non-loose tile.
			//
			// Note that this still gives the correct texel overlap if the 'loose' tile is also
			// the same texel dimension as used for the regular (non-loose) tiles.
			const FrustumCornerPoints corner_points(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset, 2 * d_expand_frustum_ratio);

			return create_bounding_polygon(corner_points);
		}


		/**
		 * Returns the oriented bounding box (OBB) containing four great circle arcs that bound
		 * the specified subdivision tile and the area inside them on the surface of the globe.
		 */
		GLIntersect::OrientedBoundingBox
		get_oriented_bounding_box(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			const FrustumCornerPoints corner_points(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset, d_expand_frustum_ratio);

			return create_oriented_bounding_box(corner_points);
		}


		/**
		 * Returns the loose oriented bounding box (OBB) formed from the specified tile, but
		 * with the tile doubled in size (about the tile centre) on the plane of the cube face.
		 *
		 * In other words the tile size, on the cube face, is exactly double and the tile centre
		 * is the same as the centre of the regular tile at the same location.
		 *
		 * See class @a CubeQuadTreePartition for a detailed description on loose bounds.
		 */
		GLIntersect::OrientedBoundingBox
		get_loose_oriented_bounding_box(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset) const
		{
			// NOTE: Since this is a loose bounds we double the normal size of the tile which
			// involves subtracting/adding 'inv_num_subdivisions' compared to a non-loose tile.
			//
			// Note that this still gives the correct texel overlap if the 'loose' tile is also
			// the same texel dimension as used for the regular (non-loose) tiles.
			const FrustumCornerPoints corner_points(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset, 2 * d_expand_frustum_ratio);

			return create_oriented_bounding_box(corner_points);
		}

	private:
		/**
		 * Calculates and contains the four (unnormalised) corner points of a frustum.
		 */
		struct FrustumCornerPoints
		{
			/**
			 * Constructor.
			 *
			 * @a frustum_expand is 
			 */
			FrustumCornerPoints(
					GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
					unsigned int level_of_detail,
					unsigned int tile_u_offset,
					unsigned int tile_v_offset,
					const double &expand_frustum_ratio = 1.0);

			//
			// NOTE: Most of these variables are data members only so that the final
			// result 'lower_u_lower_v', etc, can be initialised directly rather than
			// initialised to their default values and then immediately overwritten.
			//
			// Since this class is instantiated on the stack you can picture these as stack variables.
			//

			const unsigned int num_subdivisions;
			const double inv_num_subdivisions;

			const GPlatesMaths::Vector3D face_centre;
			const GPlatesMaths::UnitVector3D &u_direction;
			const GPlatesMaths::UnitVector3D &v_direction;

			const double frustum_expand;

			const double lower_u_scale;
			const double upper_u_scale;
			const double lower_v_scale;
			const double upper_v_scale;

			//
			// The actual values we're interested in (the four corner points).
			//

			const GPlatesMaths::Vector3D lower_u_lower_v;
			const GPlatesMaths::Vector3D lower_u_upper_v;
			const GPlatesMaths::Vector3D upper_u_lower_v;
			const GPlatesMaths::Vector3D upper_u_upper_v;
		};


		/**
		 * Factor by which to expand the frustum around the tile border.
		 */
		double d_expand_frustum_ratio;

		/**
		 * Scale factor used when/if expanding a projection transform around a frustum border.
		 *
		 * A value of 1.0 means no scaling.
		 */
		double d_expanded_projection_scale;

		//! Frustum near plane distance.
		GLdouble d_near;
		//! Frustum far plane distance.
		GLdouble d_far;


		//! Constructor.
		GLCubeSubdivision(
				const double &expand_frustum_ratio,
				GLdouble zNear,
				GLdouble zFar);

		GLTransform::non_null_ptr_to_const_type
		create_projection_transform(
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset,
				const double &expanded_projection_scale) const;

		static
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		create_bounding_polygon(
				const FrustumCornerPoints &frustum_corner_points);

		static
		GLIntersect::OrientedBoundingBox
		create_oriented_bounding_box(
				const FrustumCornerPoints &frustum_corner_points);
	};
}

#endif // GPLATES_OPENGL_GLCUBESUBDIVISION_H
