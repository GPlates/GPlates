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

#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "LayerPainter.h"

#include "FeedbackOpenGLToQPainter.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "opengl/GLContext.h"
#include "opengl/GLLight.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLShaderProgramUtils.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLText.h"

#include "utils/Profile.h"

namespace GPlatesGui
{
	namespace
	{
		/**
		 * Vertex shader source code to render points, lines and polygons with lighting.
		 */
		const QString RENDER_POINT_LINE_POLYGON_LIGHTING_VERTEX_SHADER =
				":/opengl/layer_painter/render_point_line_polygon_lighting_vertex_shader.glsl";

		/**
		 * Fragment shader source code to render points, lines and polygons with lighting.
		 */
		const QString RENDER_POINT_LINE_POLYGON_LIGHTING_FRAGMENT_SHADER =
				":/opengl/layer_painter/render_point_line_polygon_lighting_fragment_shader.glsl";

		/**
		 * Vertex shader source code for lighting axially symmetric meshes.
		 */
		const QString RENDER_AXIALLY_SYMMETRIC_MESH_LIGHTING_VERTEX_SHADER =
				":/opengl/layer_painter/render_axially_symmetric_mesh_lighting_vertex_shader.glsl";

		/**
		 * Fragment shader source code for lighting axially symmetric meshes.
		 */
		const QString RENDER_AXIALLY_SYMMETRIC_MESH_LIGHTING_FRAGMENT_SHADER =
				":/opengl/layer_painter/render_axially_symmetric_mesh_lighting_fragment_shader.glsl";
	}
}


GPlatesGui::LayerPainter::LayerPainter(
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection) :
	d_gl_visual_layers(gl_visual_layers),
	d_map_projection(map_projection)
{
}


void
GPlatesGui::LayerPainter::initialise(
		GPlatesOpenGL::GLRenderer &renderer)
{
	//
	// Create the vertex buffers.
	//

	// These are only created *once* and re-used across paint calls.
	d_vertex_element_buffer = GPlatesOpenGL::GLVertexElementBuffer::create(
			renderer,
			GPlatesOpenGL::GLBuffer::create(renderer, GPlatesOpenGL::GLBuffer::BUFFER_TYPE_VERTEX));

	d_vertex_buffer = GPlatesOpenGL::GLVertexBuffer::create(
			renderer,
			GPlatesOpenGL::GLBuffer::create(renderer, GPlatesOpenGL::GLBuffer::BUFFER_TYPE_VERTEX));

	//
	// Create and initialise the vertex array containing vertices of type 'coloured_vertex_type'.
	//

	d_vertex_array = GPlatesOpenGL::GLVertexArray::create(renderer);

	// Attach vertex element buffer to the vertex array.
	d_vertex_array->set_vertex_element_buffer(renderer, d_vertex_element_buffer);

	// Attach vertex buffer to the vertex array.
	GPlatesOpenGL::bind_vertex_buffer_to_vertex_array<coloured_vertex_type>(
			renderer, *d_vertex_array, d_vertex_buffer);

	//
	// Create the shader program to render lighting for points, lines and polygons in a 3D *globe* view.
	//

	const char *globe_view_shader_defines = "";

	GPlatesOpenGL::GLShaderSource globe_view_vertex_shader_source;
	globe_view_vertex_shader_source.add_code_segment(globe_view_shader_defines);
	globe_view_vertex_shader_source.add_code_segment_from_file(
			GPlatesOpenGL::GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	globe_view_vertex_shader_source.add_code_segment_from_file(
			RENDER_POINT_LINE_POLYGON_LIGHTING_VERTEX_SHADER);

	GPlatesOpenGL::GLShaderSource globe_view_fragment_shader_source;
	globe_view_fragment_shader_source.add_code_segment(globe_view_shader_defines);
	globe_view_fragment_shader_source.add_code_segment_from_file(
			GPlatesOpenGL::GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	globe_view_fragment_shader_source.add_code_segment_from_file(
			RENDER_POINT_LINE_POLYGON_LIGHTING_FRAGMENT_SHADER);

	d_render_point_line_polygon_lighting_in_globe_view_program_object =
			GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					globe_view_vertex_shader_source,
					globe_view_fragment_shader_source);

	//
	// Create the shader program to render lighting for points, lines and polygons in a 2D *map* view.
	//

	const char *map_view_shader_defines = "#define MAP_VIEW\n";

	GPlatesOpenGL::GLShaderSource map_view_vertex_shader_source;
	map_view_vertex_shader_source.add_code_segment(map_view_shader_defines);
	map_view_vertex_shader_source.add_code_segment_from_file(
			GPlatesOpenGL::GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	map_view_vertex_shader_source.add_code_segment_from_file(
			RENDER_POINT_LINE_POLYGON_LIGHTING_VERTEX_SHADER);

	GPlatesOpenGL::GLShaderSource map_view_fragment_shader_source;
	map_view_fragment_shader_source.add_code_segment(map_view_shader_defines);
	map_view_fragment_shader_source.add_code_segment_from_file(
			GPlatesOpenGL::GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	map_view_fragment_shader_source.add_code_segment_from_file(
			RENDER_POINT_LINE_POLYGON_LIGHTING_FRAGMENT_SHADER);

	d_render_point_line_polygon_lighting_in_map_view_program_object =
			GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					map_view_vertex_shader_source,
					map_view_fragment_shader_source);

	//
	// Create and initialise the vertex arrays containing vertices of type 'AxiallySymmetricMeshVertex'.
	//

	d_unlit_axially_symmetric_mesh_vertex_array = GPlatesOpenGL::GLVertexArray::create(renderer);
	d_lit_axially_symmetric_mesh_vertex_array = GPlatesOpenGL::GLVertexArray::create(renderer);

	// Attach vertex element buffer to the axially symmetric vertex arrays.
	d_unlit_axially_symmetric_mesh_vertex_array->set_vertex_element_buffer(renderer, d_vertex_element_buffer);
	d_lit_axially_symmetric_mesh_vertex_array->set_vertex_element_buffer(renderer, d_vertex_element_buffer);

	//
	// Attach vertex buffer to the unlit axially symmetric vertex array.
	//
	// Unlike the lit version of the vertex array this binds non-generic vertex attributes and hence
	// can be used with fixed-function pipeline (or a shader that uses non-generic vertex attributes).
	//

	d_unlit_axially_symmetric_mesh_vertex_array->set_enable_client_state(renderer, GL_VERTEX_ARRAY, true/*enable*/);
	d_unlit_axially_symmetric_mesh_vertex_array->set_vertex_pointer(
			renderer,
			d_vertex_buffer,
			3,
			GL_FLOAT,
			sizeof(AxiallySymmetricMeshVertex),
			0);

	d_unlit_axially_symmetric_mesh_vertex_array->set_enable_client_state(renderer, GL_COLOR_ARRAY, true/*enable*/);
	d_unlit_axially_symmetric_mesh_vertex_array->set_color_pointer(
			renderer,
			d_vertex_buffer,
			4,
			GL_UNSIGNED_BYTE,
			sizeof(AxiallySymmetricMeshVertex),
			3 * sizeof(GLfloat));

	// ...note that we ignore the remaining vertex attributes (which are lighting specific).

	//
	// Create shader program for lighting axially symmetric meshes.
	//

	const char *axially_symmetric_mesh_lighting_shader_defines = "";

	GPlatesOpenGL::GLShaderSource axially_symmetric_mesh_lighting_vertex_shader_source;
	axially_symmetric_mesh_lighting_vertex_shader_source.add_code_segment(
			axially_symmetric_mesh_lighting_shader_defines);
	axially_symmetric_mesh_lighting_vertex_shader_source.add_code_segment_from_file(
			GPlatesOpenGL::GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	axially_symmetric_mesh_lighting_vertex_shader_source.add_code_segment_from_file(
			RENDER_AXIALLY_SYMMETRIC_MESH_LIGHTING_VERTEX_SHADER);

	GPlatesOpenGL::GLShaderSource axially_symmetric_mesh_lighting_fragment_shader_source;
	axially_symmetric_mesh_lighting_fragment_shader_source.add_code_segment(
			axially_symmetric_mesh_lighting_shader_defines);
	axially_symmetric_mesh_lighting_fragment_shader_source.add_code_segment_from_file(
			GPlatesOpenGL::GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	axially_symmetric_mesh_lighting_fragment_shader_source.add_code_segment_from_file(
			RENDER_AXIALLY_SYMMETRIC_MESH_LIGHTING_FRAGMENT_SHADER);

	d_render_axially_symmetric_mesh_lighting_program_object =
			GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					axially_symmetric_mesh_lighting_vertex_shader_source,
					axially_symmetric_mesh_lighting_fragment_shader_source);

	//
	// Attach vertex buffer to the lit axially symmetric vertex array.
	// And bind *generic* vertex attributes to lit axially symmetric shader program.
	//

	// If shader program was unsuccessfully compiled/linked then the lit axially symmetric vertex array
	// will never get used anyway - so doesn't matter if not attached to vertex buffer.
	if (d_render_axially_symmetric_mesh_lighting_program_object)
	{
		//
		// The following reflects the structure of 'struct AxiallySymmetricMeshVertex'.
		// It tells OpenGL how the elements of the vertex are packed together in the vertex and
		// which parts of the vertex bind to the named attributes in the shader program.
		//

		// sizeof() only works on data members if you have an object instantiated...
		AxiallySymmetricMeshVertex vertex_for_sizeof;
		// Avoid unused variable warning on some compilers not recognising sizeof() as usage.
		static_cast<void>(vertex_for_sizeof.world_space_position);
		// Offset of attribute data from start of a vertex.
		GLint offset = 0;

		GLuint attribute_index = 0;

		// The "world_space_position" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"world_space_position_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				sizeof(vertex_for_sizeof.world_space_position) / sizeof(vertex_for_sizeof.world_space_position[0]),
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		++attribute_index;
		offset += sizeof(vertex_for_sizeof.world_space_position);

		// The "colour" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"colour_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				4,
				GL_UNSIGNED_BYTE,
				GL_TRUE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		++attribute_index;
		offset += sizeof(vertex_for_sizeof.colour);

		// The "world_space_x_axis" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"world_space_x_axis_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				sizeof(vertex_for_sizeof.world_space_x_axis) / sizeof(vertex_for_sizeof.world_space_x_axis[0]),
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		++attribute_index;
		offset += sizeof(vertex_for_sizeof.world_space_x_axis);

		// The "world_space_y_axis" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"world_space_y_axis_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				sizeof(vertex_for_sizeof.world_space_y_axis) / sizeof(vertex_for_sizeof.world_space_y_axis[0]),
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		++attribute_index;
		offset += sizeof(vertex_for_sizeof.world_space_y_axis);

		// The "world_space_x_axis" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"world_space_z_axis_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				sizeof(vertex_for_sizeof.world_space_z_axis) / sizeof(vertex_for_sizeof.world_space_z_axis[0]),
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		++attribute_index;
		offset += sizeof(vertex_for_sizeof.world_space_z_axis);

		// The "model_space_radial_position" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"model_space_radial_position_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				sizeof(vertex_for_sizeof.model_space_radial_position) / sizeof(vertex_for_sizeof.model_space_radial_position[0]),
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		++attribute_index;
		offset += sizeof(vertex_for_sizeof.model_space_radial_position);

		// The "radial_and_axial_normal_weights" attribute data...
		d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_bind_attrib_location(
				"radial_and_axial_normal_weights_attribute", attribute_index);
		d_lit_axially_symmetric_mesh_vertex_array->set_enable_vertex_attrib_array(
				renderer, attribute_index, true/*enable*/);
		d_lit_axially_symmetric_mesh_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_vertex_buffer,
				attribute_index,
				sizeof(vertex_for_sizeof.radial_and_axial_normal_weights) / sizeof(vertex_for_sizeof.radial_and_axial_normal_weights[0]),
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(AxiallySymmetricMeshVertex),
				offset);

		// Now that we've changed the attribute bindings in the program object we need to
		// re-link it in order for them to take effect.
		bool link_status = d_render_axially_symmetric_mesh_lighting_program_object.get()->gl_link_program(renderer);
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				link_status,
				GPLATES_ASSERTION_SOURCE);
	}
}


void
GPlatesGui::LayerPainter::begin_painting(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// The vertex buffers should have already been initialised in 'initialise()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_vertex_element_buffer && d_vertex_buffer && d_vertex_array &&
				d_unlit_axially_symmetric_mesh_vertex_array && d_lit_axially_symmetric_mesh_vertex_array,
			GPLATES_ASSERTION_SOURCE);

	d_renderer = renderer;

	drawables_off_the_sphere.begin_painting();
	opaque_drawables_on_the_sphere.begin_painting();
	translucent_drawables_on_the_sphere.begin_painting();
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::end_painting(
		GPlatesOpenGL::GLRenderer &renderer,
		float scale,
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// The cached view is a sequence of primitive (eg, raster) caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<cache_handle_type> > cache_handle(new std::vector<cache_handle_type>());

	//
	// The following mainly applies to the 3D globe view.
	//
	// The 2D map views don't need a depth buffer (being purely 2D with no off-sphere objects
	// like arrows that should be depth sorted relative to each other).
	//

	//
	// Primitives *on* the sphere include those that don't map exactly to the sphere because
	// of their finite tessellation level but are nonetheless considered as spherical geometries.
	// For example a polyline has individual great circle arc segments that are tessellated
	// into straight lines in 3D space (for rendering) and these lines dip slightly below
	// the surface of the sphere.
	// 
	// Primitives *off* the sphere include rendered direction arrows whose geometry is
	// meant to leave the surface of the sphere.
	//
	// Primitives *on* the sphere will have depth testing turned on but depth writes turned *off*.
	// The reason for this is we want geometries *on* the sphere not to depth occlude each other
	// which is something that depends on their tessellation levels. For example a mesh geometry
	// that draws a filled polygon will have parts of its mesh dip below the surface (between
	// the mesh vertices) and a separate polyline geometry will show through at these locations
	// (if the polyline geometry had had depth writes turned on). Ideally either the filled polygon
	// or the polyline should be drawn on top in its entirety depending on the order they are
	// drawn. And this will only happen reliably if their depth writes are turned off.
	//
	// Primitives *off* the sphere will have both depth testing and depth writes turned *on*.
	// The reason for this is we don't want subsequent rendered geometry layers (containing
	// primitives *on* the sphere) to overwrite (in the colour buffer) primitives *off* the sphere.
	// So for rendered direction arrows poking out of the sphere at tangents, they should always
	// be visible. Since primitives *on* the sphere still have depth testing turned on, they will
	// fail the depth test where pixels have already been written due to the rendered direction
	// arrows and hence will not overdraw the rendered direction arrows.
	//
	// Primitives *off* the sphere should not be translucent. In other words they should not
	// be anti-aliased points, lines, etc. This is because they write to the depth buffer and
	// this will leave blending artifacts near the translucent edges of fat lines, etc.
	// These blending artifacts are typically avoided in other systems by rendering translucent
	// objects in back-to-front order (ie, render distant objects first). However that can be
	// difficult and in our case most of the primitives are *on* the sphere so for the few that
	// are *off* the sphere we can limit them to being opaque.
	//

	//
	// To further complicate matters we also separate the non-raster primitives *on* the sphere into
	// two groups, opaque and translucent. This is because they have different alpha-blending and
	// point/line anti-aliasing states. By sorting primitives to each group we minimise changing
	// OpenGL state back and forth (which can be costly).
	//
	// We don't need two groups for the primitives *off* the sphere because they should
	// consist only of opaque primitives (see comments above).
	//


	// Enable depth testing but disable depth writes by default.
	renderer.gl_enable(GL_DEPTH_TEST, GL_TRUE);
	renderer.gl_depth_mask(GL_FALSE);

	// Paint a scalar field if there is one (note there should only be one scalar field per visual layer).
	const cache_handle_type scalar_fields_cache_handle =
			paint_scalar_fields(renderer, surface_occlusion_texture);
	cache_handle->push_back(scalar_fields_cache_handle);

	// Paint rasters if there are any (note there should only be one raster per visual layer).
	// In particular pre-multiplied alpha-blending is used for reasons explained in the raster rendering code.
	const cache_handle_type rasters_cache_handle = paint_rasters(renderer);
	cache_handle->push_back(rasters_cache_handle);


	// Set up alpha blending for pre-multiplied alpha.
	// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
	// This is where the RGB channels have already been multiplied by the alpha channel.
	// See class GLVisualRasterSource for why this is done.
	//
	// NOTE: The points, lines, polygons, etc are rendered using anti-aliasing which generates the
	// fragment alpha value so we cannot really pre-multiply RGB with alpha (even in a fragment shader
	// because anti-aliasing might be coverage-based and multiplied with the fragment shader alpha).
	// Instead we'll use separate alpha-blend (src,dst) factors for the alpha channel...
	//
	//   RGB uses (src_alpha, 1 - src_alpha)  ->  (R,G,B) = (Rs*As,Gs*As,Bs*As) + (1-As) * (Rd,Gd,Bd)
	//     A uses (1, 1 - src_alpha)          ->        A = As + (1-As) * Ad
	//
	// ...this then enables use to later use (1, 1 - src_alpha) for all RGBA channels when blending
	// the render texture into the main framebuffer (if that's how we get rendered by clients).
	if (renderer.get_capabilities().framebuffer.gl_EXT_blend_func_separate)
	{
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func_separate(
				GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
				GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
	else // otherwise resort to normal blending...
	{
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// Set the alpha-test state to reject pixels where alpha is zero (they make no
	// change or contribution to the framebuffer) - this is an optimisation.
	renderer.gl_enable(GL_ALPHA_TEST);
	renderer.gl_alpha_func(GL_GREATER, GLclampf(0));

	// Set the anti-aliased point state.
	renderer.gl_enable(GL_POINT_SMOOTH);
	renderer.gl_hint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	// Set the anti-aliased line state.
	renderer.gl_enable(GL_LINE_SMOOTH);
	renderer.gl_hint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// Turn on depth testing if not using a 2D map view.
	renderer.gl_enable(GL_DEPTH_TEST, !d_map_projection);

	// Even though these primitives are opaque they are still rendered with polygon anti-aliasing
	// which relies on alpha-blending (so we don't disable it here).
	// UPDATE: We no longer enable polygon anti-aliasing because it generates transparent edges
	// between adjacent triangles in a mesh.
	opaque_drawables_on_the_sphere.end_painting(
			renderer,
			*d_vertex_element_buffer->get_buffer(),
			*d_vertex_buffer->get_buffer(),
			*d_vertex_array,
			*d_unlit_axially_symmetric_mesh_vertex_array,
			*d_lit_axially_symmetric_mesh_vertex_array,
			*d_gl_visual_layers,
			d_map_projection,
			d_render_point_line_polygon_lighting_in_globe_view_program_object,
			d_render_point_line_polygon_lighting_in_map_view_program_object,
			d_render_axially_symmetric_mesh_lighting_program_object);

	translucent_drawables_on_the_sphere.end_painting(
			renderer,
			*d_vertex_element_buffer->get_buffer(),
			*d_vertex_buffer->get_buffer(),
			*d_vertex_array,
			*d_unlit_axially_symmetric_mesh_vertex_array,
			*d_lit_axially_symmetric_mesh_vertex_array,
			*d_gl_visual_layers,
			d_map_projection,
			d_render_point_line_polygon_lighting_in_globe_view_program_object,
			d_render_point_line_polygon_lighting_in_map_view_program_object,
			d_render_axially_symmetric_mesh_lighting_program_object);


	// We rendered off-the-sphere drawables after on-the-sphere drawables because, for the 2D map views,
	// there are no depth-writes (like there are for the 3D globe view) and hence nothing to make
	// the off-the-sphere drawables get draw on top of everything rendered in *all* rendered layers.
	// However we can at least make them get drawn on top *within* the current layer.
	// An example of this is rendered velocity arrows (at topological network triangulation vertices)
	// drawn on top of a filled topological network, both of which are generated by a single layer.
	{
		// Make sure we leave the OpenGL state the way it was.
		GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_drawables_off_sphere_state(renderer);

		// Turn on depth writes.
		// Drawables *off* the sphere is the only case where depth writes are enabled.
		// Only enable depth writes if not using a 2D map view.
		if (!d_map_projection)
		{
			renderer.gl_depth_mask(GL_TRUE);
		}

		// As mentioned above these off-sphere primitives should not be rendered with any anti-aliasing
		// (including polygon anti-aliasing - which we no longer use because it generates transparent
		// edges between adjacent triangles in a mesh).
		renderer.gl_enable(GL_POINT_SMOOTH, false);
		renderer.gl_enable(GL_LINE_SMOOTH, false);

		drawables_off_the_sphere.end_painting(
				renderer,
				*d_vertex_element_buffer->get_buffer(),
				*d_vertex_buffer->get_buffer(),
				*d_vertex_array,
				*d_unlit_axially_symmetric_mesh_vertex_array,
				*d_lit_axially_symmetric_mesh_vertex_array,
				*d_gl_visual_layers,
				d_map_projection,
				d_render_point_line_polygon_lighting_in_globe_view_program_object,
				d_render_point_line_polygon_lighting_in_map_view_program_object,
				d_render_axially_symmetric_mesh_lighting_program_object);
	}

	// Render any 2D text last (text specified at 2D viewport positions).
	paint_text_drawables_2D(renderer, scale);

	// Render any 3D text last (text specified at 3D world positions).
	// This is because the text is converted from 3D space to 2D window coordinates and hence is
	// effectively *off* the sphere (in the 3D globe view) but it can't have depth writes enabled
	// (because we don't know the depth since its rendered as 2D).
	// We add it last so it gets drawn last for this layer which should put it on top.
	// However if another rendered layer is drawn after this one then the text will be overwritten
	// and not appear to hover in 3D space - currently it looks like the only layer that uses
	// text is the Measure Distance tool layer (in a canvas tools workflow rendered layer) and it
	// should get drawn *after* all the reconstruction geometry/raster layers.
	// Also it depends on how the text is meant to interact with other *off* the sphere geometries
	// such as rendered arrows (should it be on top or interleave depending on depth).
	// FIXME: We might be able to draw text as 3D and turn depth writes on (however the
	// alpha-blending could cause some visual artifacts as described above).
	paint_text_drawables_3D(renderer, scale);

	// Only used for the duration of painting.
	d_renderer = boost::none;

	return cache_handle;
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::paint_scalar_fields(
		GPlatesOpenGL::GLRenderer &renderer,
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture)
{
	// Rendering 3D scalar fields not supported in 2D map views.
	if (d_map_projection)
	{
		return cache_handle_type();
	}

	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Turn on depth writes for correct depth sorting of sub-surface geometries/fields.
	renderer.gl_depth_mask(GL_TRUE);

	// Set up scalar field alpha blending for pre-multiplied alpha.
	// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
	// This is where the RGB channels have already been multiplied by the alpha channel.
	// See class GLVisualRasterSource for why this is done (not that we use that for 3D scalar fields).
	//
	// Note: The render target (main framebuffer) is fixed-point RGBA (and not floating-point) so we
	// don't need to worry about alpha-blending not being available for floating-point render targets.
	renderer.gl_enable(GL_BLEND, GL_TRUE);
	renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// No need for alpha-testing - transparent rays are culled in shader program by discarding pixel.

	// The cached view is a sequence of raster caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type> > cache_handle(
			new std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type>());
	cache_handle->reserve(scalar_fields.size());

	BOOST_FOREACH(const ScalarField3DDrawable &scalar_field_drawable, scalar_fields)
	{
		// We don't want to rebuild the OpenGL structures that render the scalar field each frame
		// so those structures need to persist from one render to the next.
		cache_handle_type scalar_field_cache_handle;

		// Either render directly to the framebuffer, or render to a QImage and draw that to the
		// feedback paint device using a QPainter.
		if (renderer.rendering_to_context_framebuffer())
		{
			scalar_field_cache_handle = d_gl_visual_layers->render_scalar_field_3d(
					renderer,
					scalar_field_drawable.source_resolved_scalar_field,
					scalar_field_drawable.render_parameters,
					surface_occlusion_texture);
			cache_handle->push_back(scalar_field_cache_handle);
		}
		else
		{
			FeedbackOpenGLToQPainter feedback_opengl;
			FeedbackOpenGLToQPainter::ImageScope image_scope(feedback_opengl, renderer);

			// The feedback image tiling loop...
			do
			{
				GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type tile_projection =
						image_scope.begin_render_tile();

				// Adjust the current projection transform - it'll get restored before the next tile though.
				GPlatesOpenGL::GLMatrix projection_matrix(tile_projection->get_matrix());
				projection_matrix.gl_mult_matrix(renderer.gl_get_matrix(GL_PROJECTION));
				renderer.gl_load_matrix(GL_PROJECTION, projection_matrix);

				// Clear the framebuffer (colour and depth) before rendering each scalar field.
				// We also clear the stencil buffer in case it is used - also it's usually
				// interleaved with depth so it's more efficient to clear both depth and stencil.
				renderer.gl_clear_color();
				renderer.gl_clear_depth();
				renderer.gl_clear_stencil();
				renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				scalar_field_cache_handle = d_gl_visual_layers->render_scalar_field_3d(
						renderer,
						scalar_field_drawable.source_resolved_scalar_field,
						scalar_field_drawable.render_parameters,
						surface_occlusion_texture);
				cache_handle->push_back(scalar_field_cache_handle);
			}
			while (image_scope.end_render_tile());

			// Draw final scalar field QImage to feedback QPainter.
			image_scope.end_render();
		}
	}

	// Now that the scalar fields have been rendered we should clear the drawables list for the next render call.
	scalar_fields.clear();

	return cache_handle;
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::paint_rasters(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Set up raster alpha blending for pre-multiplied alpha.
	// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
	// This is where the RGB channels have already been multiplied by the alpha channel.
	// See class GLVisualRasterSource for why this is done.
	//
	// Note: The render target (main framebuffer) is fixed-point RGBA (and not floating-point) so we
	// don't need to worry about alpha-blending not being available for floating-point render targets.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Enable alpha testing as an optimisation for culling transparent raster pixels.
	renderer.gl_enable(GL_ALPHA_TEST);
	renderer.gl_alpha_func(GL_GREATER, GLclampf(0));

	// The cached view is a sequence of raster caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type> > cache_handle(
			new std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type>());
	cache_handle->reserve(rasters.size());

	BOOST_FOREACH(const RasterDrawable &raster_drawable, rasters)
	{
		// We don't want to rebuild the OpenGL structures that render the raster each frame
		// so those structures need to persist from one render to the next.
		cache_handle_type raster_cache_handle;

		// Either render directly to the framebuffer, or render to a QImage and draw that to the
		// feedback paint device using a QPainter.
		if (renderer.rendering_to_context_framebuffer())
		{
			raster_cache_handle = d_gl_visual_layers->render_raster(
					renderer,
					raster_drawable.source_resolved_raster,
					raster_drawable.source_raster_colour_palette,
					raster_drawable.source_raster_modulate_colour,
					raster_drawable.normal_map_height_field_scale_factor,
					d_map_projection);
			cache_handle->push_back(raster_cache_handle);
		}
		else
		{
			FeedbackOpenGLToQPainter feedback_opengl;
			FeedbackOpenGLToQPainter::ImageScope image_scope(feedback_opengl, renderer);

			// The feedback image tiling loop...
			do
			{
				GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type tile_projection =
						image_scope.begin_render_tile();

				// Adjust the current projection transform - it'll get restored before the next tile though.
				GPlatesOpenGL::GLMatrix projection_matrix(tile_projection->get_matrix());
				projection_matrix.gl_mult_matrix(renderer.gl_get_matrix(GL_PROJECTION));
				renderer.gl_load_matrix(GL_PROJECTION, projection_matrix);

				// Clear the framebuffer (colour and depth) before rendering each raster.
				// We also clear the stencil buffer in case it is used - also it's usually
				// interleaved with depth so it's more efficient to clear both depth and stencil.
				renderer.gl_clear_color();
				renderer.gl_clear_depth();
				renderer.gl_clear_stencil();
				renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				raster_cache_handle = d_gl_visual_layers->render_raster(
						renderer,
						raster_drawable.source_resolved_raster,
						raster_drawable.source_raster_colour_palette,
						raster_drawable.source_raster_modulate_colour,
						raster_drawable.normal_map_height_field_scale_factor,
						d_map_projection);
				cache_handle->push_back(raster_cache_handle);
			}
			while (image_scope.end_render_tile());

			// Draw final raster QImage to feedback QPainter.
			image_scope.end_render();
		}
	}

	// Now that the rasters have been rendered we should clear the drawables list for the next render call.
	rasters.clear();

	return cache_handle;
}


void
GPlatesGui::LayerPainter::paint_text_drawables_2D(
		GPlatesOpenGL::GLRenderer &renderer,
		float scale)
{
	BOOST_FOREACH(const TextDrawable2D &text_drawable, text_drawables_2D)
	{
		// render drop shadow, if any
		if (text_drawable.shadow_colour)
		{
			GPlatesOpenGL::GLText::render_text_2D(
					renderer,
					text_drawable.world_x,
					text_drawable.world_y,
					text_drawable.text,
					text_drawable.shadow_colour.get(),
					text_drawable.x_offset + 1, // right 1px
					// OpenGL viewport 'y' coord goes from bottom to top...
					text_drawable.y_offset - 1, // down 1px
					text_drawable.font,
					scale);
		}

		// render main text
		if (text_drawable.colour)
		{
			GPlatesOpenGL::GLText::render_text_2D(
					renderer,
					text_drawable.world_x,
					text_drawable.world_y,
					text_drawable.text,
					text_drawable.colour.get(),
					text_drawable.x_offset,
					text_drawable.y_offset,
					text_drawable.font,
					scale);
		}
	}

	// Now that the text has been rendered we should clear the drawables list for the next render call.
	text_drawables_2D.clear();
}


void
GPlatesGui::LayerPainter::paint_text_drawables_3D(
		GPlatesOpenGL::GLRenderer &renderer,
		float scale)
{
	BOOST_FOREACH(const TextDrawable3D &text_drawable, text_drawables_3D)
	{
		// render drop shadow, if any
		if (text_drawable.shadow_colour)
		{
			GPlatesOpenGL::GLText::render_text_3D(
					renderer,
					text_drawable.world_position.x().dval(),
					text_drawable.world_position.y().dval(),
					text_drawable.world_position.z().dval(),
					text_drawable.text,
					text_drawable.shadow_colour.get(),
					text_drawable.x_offset + 1, // right 1px
					// OpenGL viewport 'y' coord goes from bottom to top...
					text_drawable.y_offset - 1, // down 1px
					text_drawable.font,
					scale);
		}

		// render main text
		if (text_drawable.colour)
		{
			GPlatesOpenGL::GLText::render_text_3D(
					renderer,
					text_drawable.world_position.x().dval(),
					text_drawable.world_position.y().dval(),
					text_drawable.world_position.z().dval(),
					text_drawable.text,
					text_drawable.colour.get(),
					text_drawable.x_offset,
					text_drawable.y_offset,
					text_drawable.font,
					scale);
		}
	}

	// Now that the text has been rendered we should clear the drawables list for the next render call.
	text_drawables_3D.clear();
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::begin_painting()
{
	d_triangle_drawables.begin_painting();
	d_axially_symmetric_mesh_triangle_drawables.begin_painting();

	// There are multiple point and line categories depending on point sizes and line widths so
	// we only begin painting on those when we encounter a new point size or line width.
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::end_painting(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
		GPlatesOpenGL::GLBuffer &vertex_buffer_data,
		GPlatesOpenGL::GLVertexArray &vertex_array,
		GPlatesOpenGL::GLVertexArray &unlit_axially_symmetric_mesh_vertex_array,
		GPlatesOpenGL::GLVertexArray &lit_axially_symmetric_mesh_vertex_array,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection,
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				render_point_line_polygon_lighting_in_globe_view_program_object,
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				render_point_line_polygon_lighting_in_map_view_program_object,
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				render_axially_symmetric_mesh_lighting_program_object)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// If any rendered polygons (or polylines) are 'filled' then render them first.
	// This way any vector geometry in this layer gets rendered on top and hence is visible.
	paint_filled_polygons(renderer, gl_visual_layers, map_projection);

	//
	// Set up for regular rendering of points, lines and polygons.
	//

	// All painting below uses the one vertex array so we only need to bind it once (here).
	// Note that the filled polygons above uses it own vertex array(s).
	vertex_array.gl_bind(renderer);

	//
	// Apply lighting if it's enabled and the runtime system supports it, otherwise defaults
	// to the fixed-function pipeline.
	//
	// This lighting simply uses the normal to the globe as the surface normal.
	// In other words it doesn't consider any surface variations that are present in arbitrary
	// triangular meshes for instance.
	//
	set_generic_point_line_polygon_lighting_state(
			renderer,
			gl_visual_layers,
			map_projection,
			render_point_line_polygon_lighting_in_globe_view_program_object,
			render_point_line_polygon_lighting_in_map_view_program_object);

	//
	// Paint the point, line and polygon drawables with the appropriate state
	// (such as point size, line width).
	//
	// Draw polygons first then lines then points so that points appear on top of lines which
	// appear on top of polygons.
	//

	//
	// Paint the drawable representing all triangle primitives (if any).
	//

#if 0 // NOTE: This causes transparent edges between adjacent triangles in a mesh so we don't enable it...
	// Set the anti-aliased polygon state.
 	renderer.gl_enable(GL_POLYGON_SMOOTH);
 	renderer.gl_hint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
#endif

	d_triangle_drawables.end_painting(
			renderer,
			vertex_element_buffer_data,
			vertex_buffer_data,
			vertex_array,
			GL_TRIANGLES);

	//
	// Paint the drawables representing all line primitives (if any).
	//

	// Iterate over the line width groups and paint them.
	BOOST_FOREACH(line_width_to_drawables_map_type::value_type &line_width_entry, d_line_drawables_map)
	{
		const float line_width = line_width_entry.first.dval();
		Drawables<coloured_vertex_type> &lines_drawable = line_width_entry.second;

		// Set the line width for the current group of lines.
		renderer.gl_line_width(line_width);

		lines_drawable.end_painting(
				renderer,
				vertex_element_buffer_data,
				vertex_buffer_data,
				vertex_array,
				GL_LINES);
	}

	// Clear the lines drawables because the next render may have a different collection of line widths.
	d_line_drawables_map.clear();

	//
	// Paint the drawables representing all point primitives (if any).
	//

	// Iterate over the point size groups and paint them.
	BOOST_FOREACH(point_size_to_drawables_map_type::value_type &point_size_entry, d_point_drawables_map)
	{
		const float point_size = point_size_entry.first.dval();
		Drawables<coloured_vertex_type> &points_drawable = point_size_entry.second;

		// Set the point size for the current group of points.
		renderer.gl_point_size(point_size);

		points_drawable.end_painting(
				renderer,
				vertex_element_buffer_data,
				vertex_buffer_data,
				vertex_array,
				GL_POINTS);
	}

	// Clear the points drawables because the next render may have a different collection of point sizes.
	d_point_drawables_map.clear();

	//
	// Render axially symmetric primitives (if any).
	//

	// Since this uses a separate vertex array and separate shader program than regular rendering
	// we only bind them if there are primitives to render (and it's quite likely there aren't any).
	if (d_axially_symmetric_mesh_triangle_drawables.has_primitives())
	{
		// Make sure we leave the OpenGL state the way it was.
		GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_axially_symmetric_state(renderer);

		// Apply axially symmetric lighting if it's enabled and the runtime system supports it,
		// otherwise defaults to the existing state (which is either the generic lighting set above
		// or the fixed-function pipeline - both of which support non-generic vertex attribute data).
		const bool lighting_axially_symmetric_meshes =
				set_axially_symmetric_mesh_lighting_state(
						renderer,
						gl_visual_layers,
						render_axially_symmetric_mesh_lighting_program_object);

		GPlatesOpenGL::GLVertexArray &axially_symmetric_mesh_vertex_array =
				lighting_axially_symmetric_meshes
						// Generic vertex attribute data...
						? lit_axially_symmetric_mesh_vertex_array
						// Non-generic vertex attribute data...
						: unlit_axially_symmetric_mesh_vertex_array;

		axially_symmetric_mesh_vertex_array.gl_bind(renderer);

		// Cull back faces since the lighting is not two-sided - the lighting is one-sided and
		// only meant for the front face. If the mesh is closed then this isn't necessary unless
		// the mesh is semi-transparent.
		// We use the currently set state of 'gl_cull_face()' and 'gl_front_face()', or the default
		// (cull back faces and front faces are CCW-oriented) if caller has not specified.
		renderer.gl_enable(GL_CULL_FACE);

		d_axially_symmetric_mesh_triangle_drawables.end_painting(
				renderer,
				vertex_element_buffer_data,
				vertex_buffer_data,
				axially_symmetric_mesh_vertex_array,
				GL_TRIANGLES);
	}
	else
	{
		// We have to match calls to 'begin_painting()' with calls to 'end_painting()' even if
		// there's no primitives to render.
		d_axially_symmetric_mesh_triangle_drawables.end_painting(
				renderer,
				vertex_element_buffer_data,
				vertex_buffer_data,
				// Any vertex array will do - it won't get used since there's no primitives to render...
				unlit_axially_symmetric_mesh_vertex_array,
				GL_TRIANGLES);
	}
}


GPlatesGui::LayerPainter::stream_primitives_type &
GPlatesGui::LayerPainter::PointLinePolygonDrawables::get_points_stream(
		float point_size)
{
	// Get the stream for points of the current point size.
	point_size_to_drawables_map_type::iterator stream_iter = d_point_drawables_map.find(point_size);
	if (stream_iter != d_point_drawables_map.end())
	{
		Drawables<coloured_vertex_type> &drawable = stream_iter->second;
		return drawable.get_stream();
	}

	// A drawable does not yet exist for 'point_size' so create a new one.
	Drawables<coloured_vertex_type> &drawable = d_point_drawables_map[point_size];

	// Start a new stream on the drawable.
	drawable.begin_painting();

	return drawable.get_stream();
}


GPlatesGui::LayerPainter::stream_primitives_type &
GPlatesGui::LayerPainter::PointLinePolygonDrawables::get_lines_stream(
		float line_width)
{
	// Get the stream for lines of the current line width.
	line_width_to_drawables_map_type::iterator stream_iter = d_line_drawables_map.find(line_width);
	if (stream_iter != d_line_drawables_map.end())
	{
		Drawables<coloured_vertex_type> &drawable = stream_iter->second;
		return drawable.get_stream();
	}

	// A drawable does not yet exist for 'line_width' so create a new one.
	Drawables<coloured_vertex_type> &drawable = d_line_drawables_map[line_width];

	// Start a new stream on the drawable.
	drawable.begin_painting();

	return drawable.get_stream();
}


GPlatesGui::LayerPainter::stream_primitives_type &
GPlatesGui::LayerPainter::PointLinePolygonDrawables::get_triangles_stream()
{
	return d_triangle_drawables.get_stream();
}


GPlatesGui::LayerPainter::axially_symmetric_mesh_stream_primitives_type &
GPlatesGui::LayerPainter::PointLinePolygonDrawables::get_axially_symmetric_mesh_triangles_stream()
{
	return d_axially_symmetric_mesh_triangle_drawables.get_stream();
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::paint_filled_polygons(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection)
{
	// Return early if nothing to render.
	if (map_projection) // Rendering to a 2D map view...
	{
		if (d_filled_polygons_map_view.empty())
		{
			return;
		}
	}
	else // Rendering to the 3D globe view...
	{
		if (d_filled_polygons_globe_view.empty())
		{
			return;
		}
	}

	// Filled polygons are rendered as rasters (textures) and hence the state set here
	// is similar (in fact identical) to the state set for rasters.
	//
	// Either render directly to the framebuffer, or render to a QImage and draw that to the
	// feedback paint device using a QPainter. We render filled polygons to an image instead of
	// as vector geometries because filled polygons are actually rendered as a raster.
	if (renderer.rendering_to_context_framebuffer())
	{
		if (map_projection) // Rendering to a 2D map view...
		{
			gl_visual_layers.render_filled_polygons(renderer, d_filled_polygons_map_view);
		}
		else // Rendering to the 3D globe view...
		{
			gl_visual_layers.render_filled_polygons(renderer, d_filled_polygons_globe_view);
		}
	}
	else
	{
		FeedbackOpenGLToQPainter feedback_opengl;
		FeedbackOpenGLToQPainter::ImageScope image_scope(feedback_opengl, renderer);

		// The feedback image tiling loop...
		do
		{
			GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type tile_projection =
					image_scope.begin_render_tile();

			// Adjust the current projection transform - it'll get restored before the next tile though.
			GPlatesOpenGL::GLMatrix projection_matrix(tile_projection->get_matrix());
			projection_matrix.gl_mult_matrix(renderer.gl_get_matrix(GL_PROJECTION));
			renderer.gl_load_matrix(GL_PROJECTION, projection_matrix);

			// Clear the framebuffer (colour and depth) before rendering the filled polygons.
			// We also clear the stencil buffer since it is used when filling polygons - also it's
			// usually interleaved with depth so it's more efficient to clear both depth and stencil.
			renderer.gl_clear_color();
			renderer.gl_clear_depth();
			renderer.gl_clear_stencil();
			renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			if (map_projection) // Rendering to a 2D map view...
			{
				gl_visual_layers.render_filled_polygons(renderer, d_filled_polygons_map_view);
			}
			else // Rendering to the 3D globe view...
			{
				gl_visual_layers.render_filled_polygons(renderer, d_filled_polygons_globe_view);
			}
		}
		while (image_scope.end_render_tile());

		// Draw final raster QImage to feedback QPainter.
		image_scope.end_render();
	}

	// Now that the filled polygons have been rendered we should clear them for the next render call.
	if (map_projection) // Rendering to a 2D map view...
	{
		d_filled_polygons_map_view.clear();
	}
	else // Rendering to the 3D globe view...
	{
		d_filled_polygons_globe_view.clear();
	}
}


bool
GPlatesGui::LayerPainter::PointLinePolygonDrawables::set_generic_point_line_polygon_lighting_state(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection,
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				render_point_line_polygon_lighting_in_globe_view_program_object,
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				render_point_line_polygon_lighting_in_map_view_program_object)
{
	// If we are not rendering to the framebuffer then we need to use OpenGL feedback in order to
	// render to the QPainter's paint device. Currently we're using base OpenGL feedback which only
	// works with the fixed-function pipeline - so we don't turn on shaders.
	// TODO: Implement OpenGL 2/3 feedback extensions to enable feedback from vertex shaders.
	if (!renderer.rendering_to_context_framebuffer())
	{
		return false;
	}

	// Get the OpenGL light if the runtime system supports it.
	boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light = gl_visual_layers.get_light(renderer);

	// Use shader program (if supported) if lighting is enabled.
	// The shader program enables lighting of the point/polyline/polygon geometries.
	if (!gl_light ||
		!gl_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_GEOMETRY_ON_SPHERE))
	{
		return false;
	}

	if (map_projection)
	{
		if (!render_point_line_polygon_lighting_in_map_view_program_object)
		{
			return false;
		}

		// Bind the shader program.
		renderer.gl_bind_program_object(render_point_line_polygon_lighting_in_map_view_program_object.get());

		// Set the (ambient+diffuse) lighting.
		// For the 2D map views this is constant across the map since the surface normal is
		// constant (it's a flat surface unlike the globe).
		render_point_line_polygon_lighting_in_map_view_program_object.get()->gl_uniform1f(
				renderer,
				"ambient_and_diffuse_lighting",
				gl_light.get()->get_map_view_constant_lighting(renderer));
	}
	else // globe view ...
	{
		if (!render_point_line_polygon_lighting_in_globe_view_program_object)
		{
			return false;
		}

		// Bind the shader program.
		renderer.gl_bind_program_object(render_point_line_polygon_lighting_in_globe_view_program_object.get());

		// Set the world-space light direction.
		render_point_line_polygon_lighting_in_globe_view_program_object.get()->gl_uniform3f(
				renderer,
				"world_space_light_direction",
				gl_light.get()->get_globe_view_light_direction(renderer));

		// Set the light ambient contribution.
		render_point_line_polygon_lighting_in_globe_view_program_object.get()->gl_uniform1f(
				renderer,
				"light_ambient_contribution",
				gl_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());
	}

	return true;
}


bool
GPlatesGui::LayerPainter::PointLinePolygonDrawables::set_axially_symmetric_mesh_lighting_state(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				render_axially_symmetric_mesh_lighting_program_object)
{
	// If we are not rendering to the framebuffer then we need to use OpenGL feedback in order to
	// render to the QPainter's paint device. Currently we're using base OpenGL feedback which only
	// works with the fixed-function pipeline - so we don't turn on shaders.
	// TODO: Implement OpenGL 2/3 feedback extensions to enable feedback from vertex shaders.
	if (!renderer.rendering_to_context_framebuffer())
	{
		return false;
	}

	// Get the OpenGL light if the runtime system supports it.
	boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light = gl_visual_layers.get_light(renderer);

	// Use shader program (if supported) if lighting is enabled, otherwise the fixed-function pipeline (default).
	// The shader program enables lighting of the point/polyline/polygon geometries.
	if (!gl_light ||
		!gl_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_DIRECTION_ARROW))
	{
		return false;
	}

	if (!render_axially_symmetric_mesh_lighting_program_object)
	{
		return false;
	}

	// Bind the shader program.
	renderer.gl_bind_program_object(render_axially_symmetric_mesh_lighting_program_object.get());

	// Set the world-space light direction.
	render_axially_symmetric_mesh_lighting_program_object.get()->gl_uniform3f(
			renderer,
			"world_space_light_direction",
			gl_light.get()->get_globe_view_light_direction(renderer));

	// Set the light ambient contribution.
	render_axially_symmetric_mesh_lighting_program_object.get()->gl_uniform1f(
			renderer,
			"light_ambient_contribution",
			gl_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());

	return true;
}


template <class VertexType>
void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables<VertexType>::begin_painting()
{
	// Create the stream.
	d_stream.reset(new Stream());

	// The stream should target our internal vertices/indices.
	d_stream->stream_target.start_streaming(
			boost::in_place(boost::ref(d_vertices)),
			boost::in_place(boost::ref(d_vertex_elements)));
}


template <class VertexType>
void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables<VertexType>::end_painting(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
		GPlatesOpenGL::GLBuffer &vertex_buffer_data,
		GPlatesOpenGL::GLVertexArray &vertex_array,
		GLenum mode)
{
	// The stream should have already been created in 'begin_painting()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_stream,
			GPLATES_ASSERTION_SOURCE);

	// Stop targeting our internal vertices/indices.
	d_stream->stream_target.stop_streaming();

	// If there are primitives to draw...
	if (has_primitives())
	{
		// Either render directly to the framebuffer, or use OpenGL feedback to render to the
		// QPainter's paint device.
		if (renderer.rendering_to_context_framebuffer())
		{
			draw_primitives(renderer, vertex_element_buffer_data, vertex_buffer_data, vertex_array, mode);
		}
		else
		{
			draw_feedback_primitives_to_qpainter(
					renderer, vertex_element_buffer_data, vertex_buffer_data, vertex_array, mode);
		}
	}

	// Destroy the stream.
	d_stream.reset();

	d_vertex_elements.clear();
	d_vertices.clear();
}


template <class VertexType>
GPlatesOpenGL::GLDynamicStreamPrimitives<VertexType, GPlatesGui::LayerPainter::vertex_element_type> &
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables<VertexType>::get_stream()
{
	// The stream should have already been created in 'begin_painting()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_stream,
			GPLATES_ASSERTION_SOURCE);

	return d_stream->stream_primitives;
}


template <class VertexType>
bool
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables<VertexType>::has_primitives() const
{
	// The stream should have already been created in 'begin_painting()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_stream,
			GPLATES_ASSERTION_SOURCE);

	return d_stream->stream_target.get_num_streamed_vertex_elements() > 0;
}


template <class VertexType>
void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables<VertexType>::draw_primitives(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
		GPlatesOpenGL::GLBuffer &vertex_buffer_data,
		GPlatesOpenGL::GLVertexArray &vertex_array,
		GLenum mode)
{
	// Stream the vertex elements.
	vertex_element_buffer_data.gl_buffer_data(
			renderer,
			GPlatesOpenGL::GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
			d_vertex_elements,
			GPlatesOpenGL::GLBuffer::USAGE_STREAM_DRAW);

	// Stream the vertices.
	vertex_buffer_data.gl_buffer_data(
			renderer,
			GPlatesOpenGL::GLBuffer::TARGET_ARRAY_BUFFER,
			d_vertices,
			GPlatesOpenGL::GLBuffer::USAGE_STREAM_DRAW);

	// Draw the primitives.
	// NOTE: The caller has already bound this vertex array.
	vertex_array.gl_draw_range_elements(
			renderer,
			mode,
			0/*start*/,
			d_vertices.size() - 1/*end*/,
			d_vertex_elements.size()/*count*/,
			GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::type,
			0/*indices_offset*/);
}


template <class VertexType>
void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables<VertexType>::draw_feedback_primitives_to_qpainter(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
		GPlatesOpenGL::GLBuffer &vertex_buffer_data,
		GPlatesOpenGL::GLVertexArray &vertex_array,
		GLenum mode)
{
	// Determine the number of points/lines/triangles we're about to render.
	unsigned int max_num_points = 0;
	unsigned int max_num_lines = 0;
	unsigned int max_num_triangles = 0;

	if (mode == GL_POINTS)
	{
		max_num_points += d_vertex_elements.size();
	}
	else if (mode == GL_LINES)
	{
		max_num_lines += d_vertex_elements.size() / 2;
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				mode == GL_TRIANGLES,
				GPLATES_ASSERTION_SOURCE);

		max_num_triangles += d_vertex_elements.size() / 3;
	}

	// Create an OpenGL feedback buffer large enough to capture the primitives we're about to render.
	// We are rendering to the QPainter attached to GLRenderer.
	FeedbackOpenGLToQPainter feedback_opengl;
	FeedbackOpenGLToQPainter::VectorGeometryScope vector_geometry_scope(
			feedback_opengl,
			renderer,
			max_num_points,
			max_num_lines,
			max_num_triangles);

	draw_primitives(renderer, vertex_element_buffer_data, vertex_buffer_data, vertex_array, mode);
}


GPlatesGui::LayerPainter::AxiallySymmetricMeshVertex::AxiallySymmetricMeshVertex(
		const GPlatesMaths::Vector3D &world_space_position_,
		GPlatesGui::rgba8_t colour_,
		const GPlatesMaths::UnitVector3D &world_space_x_axis_,
		const GPlatesMaths::UnitVector3D &world_space_y_axis_,
		const GPlatesMaths::UnitVector3D &world_space_z_axis_,
		GLfloat model_space_x_position_,
		GLfloat model_space_y_position_,
		GLfloat radial_normal_weight_,
		GLfloat axial_normal_weight_)
{
	world_space_position[0] = world_space_position_.x().dval();
	world_space_position[1] = world_space_position_.y().dval();
	world_space_position[2] = world_space_position_.z().dval();

	colour = colour_;

	world_space_x_axis[0] = world_space_x_axis_.x().dval();
	world_space_x_axis[1] = world_space_x_axis_.y().dval();
	world_space_x_axis[2] = world_space_x_axis_.z().dval();

	world_space_y_axis[0] = world_space_y_axis_.x().dval();
	world_space_y_axis[1] = world_space_y_axis_.y().dval();
	world_space_y_axis[2] = world_space_y_axis_.z().dval();

	world_space_z_axis[0] = world_space_z_axis_.x().dval();
	world_space_z_axis[1] = world_space_z_axis_.y().dval();
	world_space_z_axis[2] = world_space_z_axis_.z().dval();

	model_space_radial_position[0] = model_space_x_position_;
	model_space_radial_position[1] = model_space_y_position_;

	radial_and_axial_normal_weights[0] = radial_normal_weight_;
	radial_and_axial_normal_weights[1] = axial_normal_weight_;
}
