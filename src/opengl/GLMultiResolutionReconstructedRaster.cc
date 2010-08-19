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

#include <cmath>
#include <map>
#include <boost/cast.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polygon_2.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionReconstructedRaster.h"

#include "GLBindTextureState.h"
#include "GLBlendState.h"
#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLPointLinePolygonState.h"
#include "GLRenderer.h"
#include "GLTextureEnvironmentState.h"
#include "GLTextureTransformState.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLVertexArrayDrawable.h"

#include "global/CompilerWarnings.h"

#include "gui/Colour.h"

#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "utils/Profile.h"


namespace
{
	/**
	 * Vertex information stored in an OpenGL vertex array.
	 */
	struct Vertex
	{
		//! Vertex position.
		GLfloat x, y, z;
	};


	//! The inverse of log(2.0).
	const float INVERSE_LOG2 = 1.0 / std::log(2.0);


	/**
	 * Projects a unit vector point onto the plane whose normal is @a plane_normal and
	 * returns normalised version of projected point.
	 */
	GPlatesMaths::UnitVector3D
	get_orthonormal_vector(
			const GPlatesMaths::UnitVector3D &point,
			const GPlatesMaths::UnitVector3D &plane_normal)
	{
		// The projection of 'point' in the direction of 'plane_normal'.
		GPlatesMaths::Vector3D proj = dot(point, plane_normal) * plane_normal;

		// The projection of 'point' perpendicular to the direction of
		// 'plane_normal'.
		return (GPlatesMaths::Vector3D(point) - proj).get_normalisation();
	}
}


GPlatesOpenGL::GLCubeSubdivision::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_cube_subdivision()
{
	// This is the cube subdivision we are using.
	static GLCubeSubdivision::non_null_ptr_to_const_type s_cube_subdivision =
			GLCubeSubdivision::create();

	return s_cube_subdivision;
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

GPlatesOpenGL::GLMultiResolutionReconstructedRaster::GLMultiResolutionReconstructedRaster(
		const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
		const GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type &reconstructing_polygons,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager) :
	d_raster_to_reconstruct(raster_to_reconstruct),
	d_cube_subdivision(get_cube_subdivision()),
	d_reconstructing_polygons(reconstructing_polygons),
	d_texture_resource_manager(texture_resource_manager)
{
	PROFILE_FUNC();

	// Get the polygons grouped by rotation (plate id).
	// We do this so that polygon meshes associated with higher plate ids will get drawn
	// last and hence get drawn on top of other polygons in those cases where they overlap.
	std::vector<source_rotation_group_type::non_null_ptr_to_const_type> src_rotation_groups;
	reconstructing_polygons->get_rotation_groups_sorted_by_plate_id(src_rotation_groups);

	// Reserve space so we don't copy a lot when adding a new rotation group.
	// We may end up reserving more than we need but sizeof(RotationGroup) is not that big
	// so it should be fine.
	d_rotation_groups.reserve(src_rotation_groups.size());

	// Iterate over the source rotation groups in 'reconstructing_polygons'.
	BOOST_FOREACH(
			const source_rotation_group_type::non_null_ptr_to_const_type &src_rotation_group,
			src_rotation_groups)
	{
		// Add a new rotation group.
		d_rotation_groups.push_back(RotationGroup(src_rotation_group));
		RotationGroup &rotation_group = d_rotation_groups.back();

		// Iterate over the polygons in the source rotation group.
		BOOST_FOREACH(
				const source_polygon_region_type::non_null_ptr_to_const_type &src_polygon_region,
				src_rotation_group->polygon_regions)
		{
			// Clip the source polygon region to each face of the cube and then generate
			// a mesh for each cube face.
			generate_polygon_mesh(rotation_group, src_polygon_region);
		}
	}

	// Now that we've generated all the polygon meshes we can create a quad tree
	// for each face of the cube.
	initialise_cube_quad_trees();
}


unsigned int
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_level_of_detail(
		const GLTransformState &transform_state) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere =
			transform_state.get_min_pixel_size_on_unit_sphere();

	//
	// Calculate the level-of-detail.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (-lod)
	//
	// ...where 't0' is the texel size of the *lowest* resolution level-of-detail
	// (note that this is the opposite to GLMultiResolutionRaster where it's the *highest*)
	// and 't' is the projected size of a pixel of the viewport.
	//

	// The maximum texel size of any texel projected onto the unit sphere occurs at the centre
	// of the cube faces. Not all cube subdivisions occur at the face centres but the projected
	// texel size will always be less that at the face centre so at least it's bounded and the
	// variation across the cube face is not that large so we shouldn't be using a level-of-detail
	// that is much higher than what we need.
	const float max_lowest_resolution_texel_size_on_unit_sphere =
			2.0 / d_cube_subdivision->get_tile_texel_dimension();

	const float level_of_detail_factor = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// We need to round up instead of down and then clamp to zero.
	// We don't have an upper limit - as we traverse the quad tree to higher and higher
	// resolution nodes we might eventually reach the leaf nodes of the tree without
	// having satisified the requested level-of-detail resolution - in this case we'll
	// just render the leaf nodes as that's the highest we can provide.
	int level_of_detail = static_cast<int>(level_of_detail_factor + 0.99f);
	// Clamp to lowest resolution level of detail.
	if (level_of_detail < 0)
	{
		// If we get there then even our lowest resolution level of detail
		// had too much resolution - but this is pretty unlikely for all but the very
		// smallest of viewports.
		level_of_detail = 0;
	}

	return boost::numeric_cast<unsigned int>(level_of_detail);
}


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render(
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// First make sure we've created our clip texture.
	// We do this here rather than in the constructor because we know we have an active
	// OpenGL context here - because we're rendering.
	if (!d_clip_texture)
	{
		create_clip_texture();
	}

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	// We'll try to render of this level of detail if our quad tree is deep enough.
	const unsigned int render_level_of_detail = get_level_of_detail(renderer.get_transform_state());

	// Iterate through the rotation groups.
	BOOST_FOREACH(const RotationGroup &rotation_group, d_rotation_groups)
	{
		// Convert the rotation (based on plate id) from a unit quaternion to a matrix so
		// we can feed it to OpenGL.
		const GPlatesMaths::UnitQuaternion3D &quat_rotation = rotation_group.rotation->current_rotation;
		GLTransform::non_null_ptr_type rotation_transform = GLTransform::create(GL_MODELVIEW, quat_rotation);

		renderer.push_transform(*rotation_transform);

		// First get the view frustum planes.
		//
		// NOTE: We do this *after* pushing the above rotation transform because the
		// frustum planes are affected by the current model-view and projection transforms.
		// Out quad tree bounding boxes are in model space but the polygon meshes they
		// bound are rotating to new positions so we want to take that into account and map
		// the view frustum back to model space where we can test againt our bounding boxes.
		//
		const GLTransformState::FrustumPlanes &frustum_planes =
				renderer.get_transform_state().get_current_frustum_planes_in_model_space();
		// There are six frustum planes initially active.
		const boost::uint32_t frustum_plane_mask = 0x3f; // 111111 in binary

		// Traverse the quad trees of the cube faces for the current rotation group.
		for (unsigned int face = 0; face < 6; ++face)
		{
			const QuadTree &quad_tree = rotation_group.cube.faces[face].quad_tree;

			if (quad_tree.root_node)
			{
				render_quad_tree(
						renderer,
						*quad_tree.root_node.get(),
						0/*level_of_detail*/,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);
			}
		}

		renderer.pop_transform();
	}
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render_quad_tree(
		GLRenderer &renderer,
		const QuadTreeNode &quad_tree_node,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLTransformState::FrustumPlanes &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		// See if the OBB of the current OBB tree node intersects the view frustum.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						quad_tree_node.bounding_box,
						frustum_planes.planes,
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so OBB is outside the view frustum and we can cull it.
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current bounding box intersects. The bounding box is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// If we're at the right level of detail for rendering then do so and
	// return without traversing any child nodes.
	if (level_of_detail == render_level_of_detail)
	{
		render_quad_tree_node_tile(renderer, quad_tree_node);

		return;
	}

	//
	// Iterate over the child subdivision regions and create if cover source raster.
	//

	bool have_child_nodes = false;
	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			const boost::optional<QuadTreeNode::non_null_ptr_type> &child_quad_tree_node =
					quad_tree_node.child_nodes[child_v_offset][child_u_offset];
			if (child_quad_tree_node)
			{
				have_child_nodes = true;

				render_quad_tree(
						renderer,
						*child_quad_tree_node.get(),
						level_of_detail + 1,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);
			}
		}
	}

	// If this quad tree node does not have any child nodes then it means we've been requested
	// to render at a resolution level that is too high for us and so we can only render at
	// the highest we can provide which is now.
	if (!have_child_nodes)
	{
		render_quad_tree_node_tile(renderer, quad_tree_node);
	}
}


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render_quad_tree_node_tile(
		GLRenderer &renderer,
		const QuadTreeNode &quad_tree_node)
{
	//                   Get raster tile (texture) - may involve GLMRCR pushing a render target if not cached.
	//                   If age grid attached and age grid covers tile:
	//                      Get age grid coverage tile (texture) - may involve GLMRCR pushing a render target.
	//                      Get age grid tile (texture) - may involve GLMRCR pushing a render target.
	//                      Push render target (blank OpenGL state)
	//                         Set viewport, alpha-blending, etc.
	//                         Mask writes to the alpha channel.
	//                         First pass: render age grid as a single quad covering viewport.
	//                         Second pass:
	//                            Set projection matrix using that of the quad tree node.
	//                            Bind age grid coverage texture to texture unit 0.
	//                            Set texgen/texture matrix state for texture unit using projection matrix
	//                              of quad tree node (and any adjustments).
	//                            Iterate over polygon meshes:
	//                               If older than current reconstruction time:
	//                                  Wrap a vertex array drawable around mesh triangles and polygon vertex array
	//                                    and add to the renderer.
	//                               Endif
	//                            Enditerate
	//                         Third pass:
	//                            Unmask alpha channel.
	//                            Setup multiplicative blending using GL_DST_COLOR.
	//                            Render raster tile as a single quad covering viewport.
	//                      Pop render target.
	//                      Render same as below (case for no age grid) except render all polygon meshes
	//                         regardless of age of appearance/disappearance.
	//                   Else
	//                      Bind clip texture to texture unit 0.
	//                      Bind raster texture to texture unit 1.
	//                      Set texture function to modulate.
	//                      Set texgen/texture matrix state for each texture unit using projection matrix
	//                        of quad tree node (and any adjustments).
	//                      Iterate over polygon meshes:
	//                         If older than current reconstruction time:
	//                            Wrap a vertex array drawable around mesh triangles and polygon vertex array
	//                              and add to the renderer.
	//                         Endif
	//                      Enditerate
	//                   Endif


	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which is then passes to us to use).
	GLTexture::shared_ptr_to_const_type source_raster_texture =
			d_raster_to_reconstruct->get_tile_texture(
					quad_tree_node.source_raster_tile,
					renderer);

	// Create a container for a group of state sets.
	GLCompositeStateSet::non_null_ptr_type state_set = GLCompositeStateSet::create();

	// Create a state set that binds the clip texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_clip_texture = GLBindTextureState::create();
	bind_clip_texture->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	bind_clip_texture->gl_bind_texture(GL_TEXTURE_2D, d_clip_texture);
	state_set->add_state_set(bind_clip_texture);

	// Set the texture environment state on texture unit 0.
	GLTextureEnvironmentState::non_null_ptr_type clip_texture_environment_state =
			GLTextureEnvironmentState::create();
	clip_texture_environment_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	clip_texture_environment_state->gl_enable_texture_2D(GL_TRUE);
	clip_texture_environment_state->gl_tex_env_mode(GL_REPLACE);
	state_set->add_state_set(clip_texture_environment_state);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms
	// of the frustum of the current tile.
	GLTextureTransformState::non_null_ptr_type clip_texture_transform_state =
			GLTextureTransformState::create();
	clip_texture_transform_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_s;
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_t;
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_r;
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_q;
	clip_tex_gen_state_s.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_t.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_r.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_q.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_s.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	clip_tex_gen_state_t.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	clip_tex_gen_state_r.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	clip_tex_gen_state_q.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	const GLdouble clip_object_plane[4][4] =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	clip_tex_gen_state_s.gl_object_plane(clip_object_plane[0]);
	clip_tex_gen_state_t.gl_object_plane(clip_object_plane[1]);
	clip_tex_gen_state_r.gl_object_plane(clip_object_plane[2]);
	clip_tex_gen_state_q.gl_object_plane(clip_object_plane[3]);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_S, clip_tex_gen_state_s);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_T, clip_tex_gen_state_t);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_R, clip_tex_gen_state_r);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_Q, clip_tex_gen_state_q);
	GLMatrix clip_texture_matrix;
	// Convert the clip-space range (-1,1) to texture coord range (0.25, 0.75) so that the
	// frustrum edges will map to the boundary of the interior 2x2 clip region of our
	// 4x4 clip texture.
	clip_texture_matrix.gl_translate(0.5, 0.5, 0.0);
	clip_texture_matrix.gl_scale(0.25, 0.25, 1.0);
	clip_texture_matrix.gl_mult_matrix(quad_tree_node.projection_transform->get_matrix());
	clip_texture_matrix.gl_mult_matrix(quad_tree_node.view_transform->get_matrix());
	clip_texture_transform_state->gl_load_matrix(clip_texture_matrix);
	state_set->add_state_set(clip_texture_transform_state);


	// Create a state set that binds the source raster tile texture to texture unit 1.
	GLBindTextureState::non_null_ptr_type bind_tile_texture = GLBindTextureState::create();
	bind_tile_texture->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + 1);
	bind_tile_texture->gl_bind_texture(GL_TEXTURE_2D, source_raster_texture);
	state_set->add_state_set(bind_tile_texture);

	// Set the texture environment state on texture unit 1.
	// We want to modulate with the clip texture on unit 0.
	GLTextureEnvironmentState::non_null_ptr_type texture_environment_state =
			GLTextureEnvironmentState::create();
	texture_environment_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + 1);
	texture_environment_state->gl_enable_texture_2D(GL_TRUE);
	texture_environment_state->gl_tex_env_mode(GL_MODULATE);
	state_set->add_state_set(texture_environment_state);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms
	// of the frustum of the current tile.
	// Set it on same texture unit, ie texture unit 1.
	GLTextureTransformState::non_null_ptr_type texture_transform_state =
			GLTextureTransformState::create();
	texture_transform_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + 1);
	GLTextureTransformState::TexGenCoordState tex_gen_state_s;
	GLTextureTransformState::TexGenCoordState tex_gen_state_t;
	GLTextureTransformState::TexGenCoordState tex_gen_state_r;
	GLTextureTransformState::TexGenCoordState tex_gen_state_q;
	tex_gen_state_s.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_t.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_r.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_q.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_s.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_t.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_r.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_q.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	const GLdouble object_plane[4][4] =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	tex_gen_state_s.gl_object_plane(object_plane[0]);
	tex_gen_state_t.gl_object_plane(object_plane[1]);
	tex_gen_state_r.gl_object_plane(object_plane[2]);
	tex_gen_state_q.gl_object_plane(object_plane[3]);
	texture_transform_state->set_tex_gen_coord_state(GL_S, tex_gen_state_s);
	texture_transform_state->set_tex_gen_coord_state(GL_T, tex_gen_state_t);
	texture_transform_state->set_tex_gen_coord_state(GL_R, tex_gen_state_r);
	texture_transform_state->set_tex_gen_coord_state(GL_Q, tex_gen_state_q);
	GLMatrix texture_matrix;
	texture_matrix.gl_scale(0.5, 0.5, 1.0);
	texture_matrix.gl_translate(1.0, 1.0, 0.0);
	texture_matrix.gl_mult_matrix(quad_tree_node.projection_transform->get_matrix());
	texture_matrix.gl_mult_matrix(quad_tree_node.view_transform->get_matrix());
	texture_transform_state->gl_load_matrix(texture_matrix);
	state_set->add_state_set(texture_transform_state);


	// Enable alpha-blending in case texture has partial transparency.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state_set->add_state_set(blend_state);

#if 0
	GLPolygonState::non_null_ptr_type polygon_state = GLPolygonState::create();
	polygon_state->gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
	state_set->add_state_set(polygon_state);
#endif

	// Use the current reconstruction time to determine which polygons to draw based
	// on their time period.
	const GPlatesPropertyValues::GeoTimeInstant reconstruction_time(
			d_reconstructing_polygons->get_current_reconstruction_time());

	// Push the state set onto the state graph.
	renderer.push_state_set(state_set);

	// Iterate over the polygon meshes for this quad tree note and draw them.
	BOOST_FOREACH(const PolygonMesh &polygon_mesh, quad_tree_node.polygon_meshes)
	{
		const Polygon &polygon = *polygon_mesh.polygon;

		// If the current reconstruction time is within the time period of the current
		// polygon then we can display it.
		if (polygon.time_of_appearance.is_earlier_than_or_coincident_with(reconstruction_time) &&
			reconstruction_time.is_earlier_than_or_coincident_with(polygon.time_of_disappearance))
		{
			// Add the drawable to the current render target.
			renderer.add_drawable(
					GLVertexArrayDrawable::create(
							polygon.vertex_array,
							polygon_mesh.vertex_element_array));
		}
	}

	// Pop the state set.
	renderer.pop_state_set();
}


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::initialise_cube_quad_trees()
{
	PROFILE_FUNC();

	// Iterate over the rotation groups.
	BOOST_FOREACH(RotationGroup &rotation_group, d_rotation_groups)
	{
		// Prepare some information about the polygons of the current rotation group,
		// such as the polygon mesh vertices and triangles, before we traverse down the
		// cube face quad trees and partition into smaller subdivisions.
		partition_mesh_seq_type partitioned_meshes;
		partitioned_meshes.reserve(rotation_group.polygons.size());
		BOOST_FOREACH(const Polygon::non_null_ptr_type &polygon, rotation_group.polygons)
		{
			PartitionedMesh partitioned_mesh = { polygon, std::vector<GLuint>() };
			partitioned_meshes.push_back(partitioned_mesh);
			partitioned_meshes.back().vertex_element_array_data = polygon->vertex_element_array_data;
		}

		for (unsigned int face = 0; face < 6; ++face)
		{
			GLCubeSubdivision::CubeFaceType cube_face =
					static_cast<GLCubeSubdivision::CubeFaceType>(face);

			// Start traversing the root of the quad tree of the same cube face in the source raster.
			boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> source_raster_quad_tree_root_node =
					d_raster_to_reconstruct->get_root_quad_tree_node(cube_face);
			if (!source_raster_quad_tree_root_node)
			{
				// Source raster does not cover the current cube face so
				// we don't need to generate a quad tree.
				continue;
			}

			// Recursively generate a quad tree for the current cube face.
			boost::optional<QuadTreeNode::non_null_ptr_type> quad_tree_root_node =
					create_quad_tree_node(
							cube_face,
							partitioned_meshes,
							source_raster_quad_tree_root_node.get(),
							0,/*level_of_detail*/
							0,/*tile_u_offset*/
							0/*tile_v_offset*/);

			QuadTree &quad_tree = rotation_group.cube.faces[face].quad_tree;
			quad_tree.root_node = quad_tree_root_node;
		}
	}
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

boost::optional<GPlatesOpenGL::GLMultiResolutionReconstructedRaster::QuadTreeNode::non_null_ptr_type>
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create_quad_tree_node(
		GLCubeSubdivision::CubeFaceType cube_face,
		const partition_mesh_seq_type &parent_partitioned_meshes,
		const GLMultiResolutionCubeRaster::QuadTreeNode &source_raster_quad_tree_node,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset)
{
	// Create a transform state so we can get the clip planes of the cube subdivision
	// corresponding to the current quad tree node.
	GLTransformState::non_null_ptr_type transform_state = GLTransformState::create();

	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision->get_projection_transform(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset);
	transform_state->load_transform(*projection_transform);

	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision->get_view_transform(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset);
	transform_state->load_transform(*view_transform);

	const GLTransformState::FrustumPlanes &frustum_planes =
			transform_state->get_current_frustum_planes_in_model_space();

	// The box bounding the meshes of this quad tree node.
	// Use the average of the left/right/bottom/top frustum plane normals as our
	// OBB z-axis. And use the average of the left and negative right plane normals
	// as our OBB y-axis.
	const GPlatesMaths::Vector3D left_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::LEFT_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::Vector3D right_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::RIGHT_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::Vector3D bottom_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::BOTTOM_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::Vector3D top_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::TOP_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::UnitVector3D obb_z_axis =
			(left_plane_normal + right_plane_normal + bottom_plane_normal + top_plane_normal).get_normalisation();
	const GPlatesMaths::Vector3D obb_y_axis = left_plane_normal - right_plane_normal;
	GLIntersect::OrientedBoundingBoxBuilder bounding_box_builder =
			GLIntersect::create_oriented_bounding_box_builder(obb_y_axis, obb_z_axis);
	// Add the extremal point along the z-axis which is just the z-axis point itself.
	bounding_box_builder.add(obb_z_axis);

	// The partitioned mesh for the current quad tree node (if any).
	partition_mesh_seq_type partitioned_meshes;
	partitioned_meshes.reserve(parent_partitioned_meshes.size()); // Avoid excessive copying.
	polygon_mesh_seq_type polygon_meshes;
	polygon_meshes.reserve(parent_partitioned_meshes.size()); // Avoid excessive copying.

	// Iterate over the polygons and find the mesh triangles that cover the current
	// cube subdivision - in other words, that are not fully outside any frustum plane.
	// It's possible that a triangle does not intersect the frustum and is not fully
	// outside any frustum planes in which case we're including a triangle that does not
	// intersect the frustum - but the the CGAL mesher produces nice shaped triangles
	// (ie, not long skinny ones) and there won't be many of these (they'll only be
	// near where two frustum planes intersect).
	BOOST_FOREACH(const PartitionedMesh &parent_partitioned_mesh, parent_partitioned_meshes)
	{
		const std::vector<GPlatesMaths::UnitVector3D> &mesh_vertices =
				parent_partitioned_mesh.polygon->mesh_points;

		const std::vector<GLuint> &parent_mesh_triangles =
				parent_partitioned_mesh.vertex_element_array_data;

		// Any mesh triangles for the current quad tree node will go here.
		std::vector<GLuint> mesh_triangles;
		GLuint min_vertex_index = mesh_vertices.size();
		GLuint max_vertex_index = 0;

		const unsigned int num_parent_mesh_triangles = parent_mesh_triangles.size() / 3;
		for (unsigned int tri_index = 0; tri_index < num_parent_mesh_triangles; ++tri_index)
		{
			const unsigned int vertex_element_index = 3 * tri_index;

			const GLuint vertex_index0 = parent_mesh_triangles[vertex_element_index];
			const GLuint vertex_index1 = parent_mesh_triangles[vertex_element_index + 1];
			const GLuint vertex_index2 = parent_mesh_triangles[vertex_element_index + 2];

			const GPlatesMaths::UnitVector3D &tri_vertex0 = mesh_vertices[vertex_index0];
			const GPlatesMaths::UnitVector3D &tri_vertex1 = mesh_vertices[vertex_index1];
			const GPlatesMaths::UnitVector3D &tri_vertex2 = mesh_vertices[vertex_index2];

			// Test the current triangle against the frustum planes.
			bool is_triangle_outside_frustum = false;
			BOOST_FOREACH(const GLIntersect::Plane &plane, frustum_planes.planes)
			{
				// If all vertices of the triangle are outside a single plane then
				// the triangle is outside the frustum.
				if (plane.signed_distance(tri_vertex0) < 0 &&
					plane.signed_distance(tri_vertex1) < 0 &&
					plane.signed_distance(tri_vertex2) < 0)
				{
					is_triangle_outside_frustum = true;
					break;
				}
			}

			if (!is_triangle_outside_frustum)
			{
				// Add triangle to the list of triangles for the current quad tree node.
				mesh_triangles.push_back(vertex_index0);
				mesh_triangles.push_back(vertex_index1);
				mesh_triangles.push_back(vertex_index2);

				// Keep track of the minimum vertex index used by the current mesh.
				if (vertex_index0 < min_vertex_index)
				{
					min_vertex_index = vertex_index0;
				}
				if (vertex_index1 < min_vertex_index)
				{
					min_vertex_index = vertex_index1;
				}
				if (vertex_index2 < min_vertex_index)
				{
					min_vertex_index = vertex_index2;
				}
				// Keep track of the maximum vertex index used by the current mesh.
				if (vertex_index0 > max_vertex_index)
				{
					max_vertex_index = vertex_index0;
				}
				if (vertex_index1 > max_vertex_index)
				{
					max_vertex_index = vertex_index1;
				}
				if (vertex_index2 > max_vertex_index)
				{
					max_vertex_index = vertex_index2;
				}

				// Expand this quad tree node's bounding box to include the current triangle.
				bounding_box_builder.add(tri_vertex0);
				bounding_box_builder.add(tri_vertex1);
				bounding_box_builder.add(tri_vertex2);
			}
		}

		// If the current polygon has triangles that cover the current cube subdivision.
		if (!mesh_triangles.empty())
		{
			GLVertexElementArray::non_null_ptr_type vertex_element_array =
					GLVertexElementArray::create(mesh_triangles);

			// Tell it what to draw when the time comes to draw.
			vertex_element_array->gl_draw_range_elements_EXT(
					GL_TRIANGLES,
					min_vertex_index/*start*/,
					max_vertex_index/*end*/,
					mesh_triangles.size()/*count*/,
					GL_UNSIGNED_INT/*type*/,
					0 /*indices_offset*/);

			PolygonMesh polygon_mesh =
			{
				parent_partitioned_mesh.polygon,
				vertex_element_array
			};
			polygon_meshes.push_back(polygon_mesh);

			// Add some information that our child nodes can use for partitioning.
			// We're effectively reducing the number of mesh triangles that children
			// have to test against since we know that triangles outside this
			// quad tree node will also be outside all child nodes.
			PartitionedMesh partitioned_mesh = { parent_partitioned_mesh.polygon, std::vector<GLuint>() };
			partitioned_meshes.push_back(partitioned_mesh);
			partitioned_meshes.back().vertex_element_array_data.swap(mesh_triangles);
		}
	}

	// If none of the polygons in the current rotation group cover the current
	// quad tree node's cube subdivision then we don't need to create a node.
	if (partitioned_meshes.empty())
	{
		return boost::none;
	}

	// Create a quad tree node.
	QuadTreeNode::non_null_ptr_type quad_tree_node = QuadTreeNode::create(
			source_raster_quad_tree_node.get_tile_handle(),
			projection_transform,
			view_transform,
			bounding_box_builder.get_oriented_bounding_box());

	quad_tree_node->polygon_meshes.swap(polygon_meshes);

	// Build child quad tree nodes if necessary.
	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// If the source raster does not having a child node then either the raster doesn't
			// cover that cube subdivision or it has a high enough resolution to, in turn,
			// reproduce its source raster - so we don't need to create a child node either.
			boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> source_raster_child_quad_tree_node =
					source_raster_quad_tree_node.get_child_node(child_v_offset, child_u_offset);
			if (!source_raster_child_quad_tree_node)
			{
				continue;
			}

			quad_tree_node->child_nodes[child_v_offset][child_u_offset] =
					create_quad_tree_node(
							cube_face,
							partitioned_meshes,
							source_raster_child_quad_tree_node.get(),
							level_of_detail + 1,
							2 * tile_u_offset + child_u_offset,
							2 * tile_v_offset + child_v_offset);
		}
	}

	return quad_tree_node;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create_clip_texture()
{
	d_clip_texture = GLTexture::create(d_texture_resource_manager);

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	d_clip_texture->gl_bind_texture(GL_TEXTURE_2D);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	//
	// The clip texture is a 4x4 image where the centre 2x2 texels are 1.0
	// and the boundary texels are 0.0.
	// We will use the alpha channel for alpha-testing (to discard clipped regions)
	// and we'll use the colour channels to modulate
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[16] =
	{
		mask_zero, mask_zero, mask_zero, mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_zero, mask_zero, mask_zero
	};

	// Create the texture and load the data into it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::generate_polygon_mesh(
		RotationGroup &rotation_group,
		const source_polygon_region_type::non_null_ptr_to_const_type &src_polygon_region)
{
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Triangulation_vertex_base_2<K> Vb;
	typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
	typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
	typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
	typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;

	typedef CGAL::Cartesian<double> KP;
	typedef KP::Point_2 Point;
	typedef CGAL::Polygon_2<KP> Polygon_2;

	// Clip each polygon to the current cube face.
	//
	// Instead, for now, just project onto an arbitrary plane.

	// Iterate through the polygon vertices and calculate the sum of vertex positions.
	GPlatesMaths::Vector3D summed_vertex_position(0,0,0);
	int num_vertices = 0;
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator vertex_iter =
			src_polygon_region->exterior_polygon->vertex_begin();
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator vertex_end =
			src_polygon_region->exterior_polygon->vertex_end();
	for ( ; vertex_iter != vertex_end; ++vertex_iter, ++num_vertices)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::Vector3D point(vertex.position_vector());
		summed_vertex_position = summed_vertex_position + point;
	}

	// If the magnitude of the summed vertex position is zero then all the points averaged
	// to zero and hence we cannot get a plane normal to project onto.
	// This most likely happens when the vertices roughly form a great circle arc and hence
	// then are two possible projection directions and hence you could assign the orientation
	// to be either clockwise or counter-clockwise.
	// If this happens we'll just choose one orientation arbitrarily.
	if (summed_vertex_position.magSqrd() <= 0.0)
	{
		return;
	}

	// Calculate a unit vector from the sum to use as our plane normal.
	const GPlatesMaths::UnitVector3D proj_plane_normal =
			summed_vertex_position.get_normalisation();

	// First try starting with the global x axis - if it's too close to the plane normal
	// then choose the global y axis.
	GPlatesMaths::UnitVector3D proj_plane_x_axis_test_point(0, 0, 1); // global x-axis
	if (dot(proj_plane_x_axis_test_point, proj_plane_normal) > 1 - 1e-2)
	{
		proj_plane_x_axis_test_point = GPlatesMaths::UnitVector3D(0, 1, 0); // global y-axis
	}
	const GPlatesMaths::UnitVector3D proj_plane_axis_x = get_orthonormal_vector(
			proj_plane_x_axis_test_point, proj_plane_normal);

	// Determine the y axis of the plane.
	const GPlatesMaths::UnitVector3D proj_plane_axis_y(
			cross(proj_plane_normal, proj_plane_axis_x));

	Polygon_2 polygon_2;

	--vertex_end;
	// Iterate through the vertices again and project onto the plane.
	for (vertex_iter = src_polygon_region->exterior_polygon->vertex_begin();
		vertex_iter != vertex_end;
		++vertex_iter)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::UnitVector3D &point = vertex.position_vector();

		const GPlatesMaths::real_t proj_point_z = dot(proj_plane_normal, point);
		// For now, if any point isn't localised on the plane then discard polygon.
		if (proj_point_z < 0.35)
		{
			std::cout << "Unable to project polygon - it's too big." << std::endl;
			return;
		}
		const GPlatesMaths::real_t inv_proj_point_z = 1.0 / proj_point_z;

		const GPlatesMaths::real_t proj_point_x = inv_proj_point_z * dot(proj_plane_axis_x, point);
		const GPlatesMaths::real_t proj_point_y = inv_proj_point_z * dot(proj_plane_axis_y, point);

		polygon_2.push_back(Point(proj_point_x.dval(), proj_point_y.dval()));
	}

	// For now, if the polygon is not simple (ie, it's self-intersecting) then discard polygon.
	if (!polygon_2.is_simple())
	{
		std::cout << "Unable to mesh polygon - it's self-intersecting." << std::endl;
		return;
	}

	// Use a set in case CGAL merges any vertices.
	std::map<CDT::Vertex_handle, unsigned int/*vertex index*/> unique_vertex_handles;
	std::vector<CDT::Vertex_handle> vertex_handles;
	CDT cdt;
	for (Polygon_2::Vertex_iterator vert_iter = polygon_2.vertices_begin();
		vert_iter != polygon_2.vertices_end();
		++vert_iter)
	{
		CDT::Vertex_handle vertex_handle = cdt.insert(CDT::Point(vert_iter->x(), vert_iter->y()));
		if (unique_vertex_handles.insert(
				std::map<CDT::Vertex_handle, unsigned int>::value_type(
						vertex_handle, vertex_handles.size())).second)
		{
			vertex_handles.push_back(vertex_handle);
		}
	}

	// For now, if the polygon has less than three vertices then discard it.
	// This can happen if CGAL determines two points are close enough to be merged.
	if (vertex_handles.size() < 3)
	{
		std::cout << "Polygon has less than 3 vertices after triangulation." << std::endl;
		return;
	}

	// Add the boundary constraints.
	for (std::size_t vert_index = 1; vert_index < vertex_handles.size(); ++vert_index)
	{
		cdt.insert_constraint(vertex_handles[vert_index - 1], vertex_handles[vert_index]);
	}
	cdt.insert_constraint(vertex_handles[vertex_handles.size() - 1], vertex_handles[0]);

	// Mesh the domain of the triangulation - the area bounded by constraints.
	PROFILE_BEGIN(cgal_refine_triangulation, "CGAL::refine_Delaunay_mesh_2");
	CGAL::refine_Delaunay_mesh_2(cdt, Criteria(0.125, 0.25));
	PROFILE_END(cgal_refine_triangulation);

	// The vertices of the vertex array for the polygon.
	std::vector<Vertex> vertex_array_data;
	std::vector<GPlatesMaths::UnitVector3D> mesh_points;
	// The triangles indices.
	std::vector<GLuint> vertex_element_array_data;

	// Iterate over the mesh triangles and collect the triangles belonging to the domain.
	typedef std::map<CDT::Vertex_handle, std::size_t/*vertex index*/> mesh_map_type;
	mesh_map_type mesh_vertex_handles;
	for (CDT::Finite_faces_iterator triangle_iter = cdt.finite_faces_begin();
		triangle_iter != cdt.finite_faces_end();
		++triangle_iter)
	{
		if (!triangle_iter->is_in_domain())
		{
			continue;
		}

		for (unsigned int tri_vert_index = 0; tri_vert_index < 3; ++tri_vert_index)
		{
			CDT::Vertex_handle vertex_handle = triangle_iter->vertex(tri_vert_index);

			std::pair<mesh_map_type::iterator, bool> p = mesh_vertex_handles.insert(
					mesh_map_type::value_type(
							vertex_handle, vertex_array_data.size()));

			const std::size_t mesh_vertex_index = p.first->second;
			if (p.second)
			{
				// Unproject the mesh point back onto the sphere.
				const CDT::Point &point2d = vertex_handle->point();
				const GPlatesMaths::UnitVector3D point3d =
					(GPlatesMaths::Vector3D(proj_plane_normal) +
					point2d.x() * proj_plane_axis_x +
					point2d.y() * proj_plane_axis_y).get_normalisation();

				mesh_points.push_back(point3d);

				const Vertex vertex = { point3d.x().dval(), point3d.y().dval(), point3d.z().dval() };
				vertex_array_data.push_back(vertex);
			}
			vertex_element_array_data.push_back(mesh_vertex_index);
		}
	}

	// If the polygon has no time of appearance then assume distant past.
	GPlatesPropertyValues::GeoTimeInstant time_of_appearance = src_polygon_region->time_of_appearance
			? src_polygon_region->time_of_appearance.get()
			: GPlatesPropertyValues::GeoTimeInstant::create_distant_past();

	// If the polygon has no time of disappearance then assume distant future.
	GPlatesPropertyValues::GeoTimeInstant time_of_disappearance = src_polygon_region->time_of_disappearance
			? src_polygon_region->time_of_disappearance.get()
			: GPlatesPropertyValues::GeoTimeInstant::create_distant_future();

	Polygon::non_null_ptr_type polygon = Polygon::create(
			time_of_appearance, time_of_disappearance);

	polygon->mesh_points.swap(mesh_points);

	polygon->vertex_array = GLVertexArray::create(vertex_array_data);
	// We only have (x,y,z) coordinates in our vertex array.
	polygon->vertex_array->gl_enable_client_state(GL_VERTEX_ARRAY);
	polygon->vertex_array->gl_vertex_pointer(3, GL_FLOAT, sizeof(Vertex), 0);

	polygon->vertex_element_array_data.swap(vertex_element_array_data);

	rotation_group.polygons.push_back(polygon);
}
