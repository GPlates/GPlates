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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <boost/utility/in_place_factory.hpp>

#include "LayerPainter.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "opengl/GL.h"
#include "opengl/GLContext.h"
#include "opengl/GLLight.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLText.h"
#include "opengl/GLVertexUtils.h"

#include "utils/CallStackTracker.h"
#include "utils/Profile.h"

namespace GPlatesGui
{
	namespace
	{
		/**
		 * Vertex shader source code to render points, lines and polygons.
		 */
		const QString RENDER_POINT_LINE_POLYGON_VERTEX_SHADER =
				":/opengl/layer_painter/render_point_line_polygon_vertex_shader.glsl";

		/**
		 * Fragment shader source code to render points, lines and polygons.
		 */
		const QString RENDER_POINT_LINE_POLYGON_FRAGMENT_SHADER =
				":/opengl/layer_painter/render_point_line_polygon_fragment_shader.glsl";

		/**
		 * Vertex shader source code for rendering axially symmetric meshes.
		 */
		const QString RENDER_AXIALLY_SYMMETRIC_MESH_VERTEX_SHADER =
				":/opengl/layer_painter/render_axially_symmetric_mesh_vertex_shader.glsl";

		/**
		 * Fragment shader source code for rendering axially symmetric meshes.
		 */
		const QString RENDER_AXIALLY_SYMMETRIC_MESH_FRAGMENT_SHADER =
				":/opengl/layer_painter/render_axially_symmetric_mesh_fragment_shader.glsl";
	}
}


GPlatesGui::LayerPainter::LayerPainter(
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		int device_pixel_ratio,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection) :
	drawables_off_the_sphere(device_pixel_ratio),
	drawables_on_the_sphere(device_pixel_ratio),
	d_gl_visual_layers(gl_visual_layers),
	d_map_projection(map_projection)
{
}


void
GPlatesGui::LayerPainter::initialise(
		GPlatesOpenGL::GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	//
	// Create the vertex buffers.
	//

	// These are re-used across paint calls.
	d_vertex_element_buffer = GPlatesOpenGL::GLBuffer::create(gl);
	d_vertex_buffer = GPlatesOpenGL::GLBuffer::create(gl);

	//
	// Create and initialise the vertex array containing vertices of type 'coloured_vertex_type'.
	//

	d_vertex_array = GPlatesOpenGL::GLVertexArray::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_vertex_element_buffer);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_vertex_buffer);

	// Specify vertex attributes (position and colour) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(coloured_vertex_type), BUFFER_OFFSET(coloured_vertex_type, x));
	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(coloured_vertex_type), BUFFER_OFFSET(coloured_vertex_type, colour));

	//
	// Create the shader program to render points, lines and polygons.
	//

	// Vertex shader source.
	GPlatesOpenGL::GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
	vertex_shader_source.add_code_segment_from_file(RENDER_POINT_LINE_POLYGON_VERTEX_SHADER);
	// Vertex shader.
	GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(vertex_shader_source);
	vertex_shader->compile_shader();

	// Fragment shader source.
	GPlatesOpenGL::GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
	fragment_shader_source.add_code_segment_from_file(RENDER_POINT_LINE_POLYGON_FRAGMENT_SHADER);
	// Fragment shader.
	GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(fragment_shader_source);
	fragment_shader->compile_shader();

	// Vertex-fragment program.
	d_render_point_line_polygon_program = GPlatesOpenGL::GLProgram::create(gl);
	d_render_point_line_polygon_program->attach_shader(vertex_shader);
	d_render_point_line_polygon_program->attach_shader(fragment_shader);
	d_render_point_line_polygon_program->link_program();

	//
	// Create and initialise the vertex array containing vertices of type 'AxiallySymmetricMeshVertex'.
	//

	d_axially_symmetric_mesh_vertex_array = GPlatesOpenGL::GLVertexArray::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_axially_symmetric_mesh_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_vertex_element_buffer);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_vertex_buffer);

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The following reflects the structure of 'struct AxiallySymmetricMeshVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, world_space_position));
	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, colour));
	gl.EnableVertexAttribArray(2);
	gl.VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, world_space_x_axis));
	gl.EnableVertexAttribArray(3);
	gl.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, world_space_y_axis));
	gl.EnableVertexAttribArray(4);
	gl.VertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, world_space_z_axis));
	gl.EnableVertexAttribArray(5);
	gl.VertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, model_space_radial_position));
	gl.EnableVertexAttribArray(6);
	gl.VertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(AxiallySymmetricMeshVertex), BUFFER_OFFSET(AxiallySymmetricMeshVertex, radial_and_axial_normal_weights));

	//
	// Create shader program to render axially symmetric meshes.
	//

	// Vertex shader source.
	GPlatesOpenGL::GLShaderSource axially_symmetric_mesh_vertex_shader_source;
	axially_symmetric_mesh_vertex_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
	axially_symmetric_mesh_vertex_shader_source.add_code_segment_from_file(RENDER_AXIALLY_SYMMETRIC_MESH_VERTEX_SHADER);
	// Vertex shader.
	GPlatesOpenGL::GLShader::shared_ptr_type axially_symmetric_mesh_vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
	axially_symmetric_mesh_vertex_shader->shader_source(axially_symmetric_mesh_vertex_shader_source);
	axially_symmetric_mesh_vertex_shader->compile_shader();

	// Fragment shader source.
	GPlatesOpenGL::GLShaderSource axially_symmetric_mesh_fragment_shader_source;
	axially_symmetric_mesh_fragment_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
	axially_symmetric_mesh_fragment_shader_source.add_code_segment_from_file(RENDER_AXIALLY_SYMMETRIC_MESH_FRAGMENT_SHADER);
	// Fragment shader.
	GPlatesOpenGL::GLShader::shared_ptr_type axially_symmetric_mesh_fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
	axially_symmetric_mesh_fragment_shader->shader_source(axially_symmetric_mesh_fragment_shader_source);
	axially_symmetric_mesh_fragment_shader->compile_shader();

	// Vertex-fragment program.
	d_render_axially_symmetric_mesh_program = GPlatesOpenGL::GLProgram::create(gl);
	d_render_axially_symmetric_mesh_program->attach_shader(axially_symmetric_mesh_vertex_shader);
	d_render_axially_symmetric_mesh_program->attach_shader(axially_symmetric_mesh_fragment_shader);
	d_render_axially_symmetric_mesh_program->link_program();
}


void
GPlatesGui::LayerPainter::begin_painting(
		GPlatesOpenGL::GL &gl)
{
	// The vertex buffers should have already been initialised in 'initialise()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_vertex_element_buffer && d_vertex_buffer && d_vertex_array &&
				d_axially_symmetric_mesh_vertex_array,
			GPLATES_ASSERTION_SOURCE);

	drawables_off_the_sphere.begin_painting();
	drawables_on_the_sphere.begin_painting();
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::end_painting(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		float scale)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

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


	// Enable depth testing but disable depth writes by default.
	gl.Enable(GL_DEPTH_TEST);
	gl.DepthMask(GL_FALSE);

	// Paint a scalar field if there is one (note there should only be one scalar field per visual layer).
	const cache_handle_type scalar_fields_cache_handle = paint_scalar_fields(gl, view_projection);
	cache_handle->push_back(scalar_fields_cache_handle);

	// Paint rasters if there are any (note there should only be one raster per visual layer).
	// In particular pre-multiplied alpha-blending is used for reasons explained in the raster rendering code.
	const cache_handle_type rasters_cache_handle = paint_rasters(gl, view_projection);
	cache_handle->push_back(rasters_cache_handle);


#if 0 // TODO: Remove this commented-out code once we're sure that each type of primitive being rendered
	  //       should decide itself whether to pre-multiply alpha in shader program or using alpha blending.
	  //       This means all rendering can assume the default state of no alpha blending.
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
	// ...this then enables us to later use (1, 1 - src_alpha) for all RGBA channels when blending
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
#endif

	// Set the anti-aliased line state.
	gl.Enable(GL_LINE_SMOOTH);
	gl.Hint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// Turn on depth testing unless using a 2D map view.
	if (d_map_projection)
	{
		gl.Disable(GL_DEPTH_TEST);
	}
	else
	{
		gl.Enable(GL_DEPTH_TEST);
	}

	drawables_on_the_sphere.end_painting(
			gl,
			d_vertex_element_buffer,
			d_vertex_buffer,
			d_vertex_array,
			d_axially_symmetric_mesh_vertex_array,
			d_render_point_line_polygon_program,
			d_render_axially_symmetric_mesh_program,
			*d_gl_visual_layers,
			view_projection,
			d_map_projection);


	// We rendered off-the-sphere drawables after on-the-sphere drawables because, for the 2D map views,
	// there are no depth-writes (like there are for the 3D globe view) and hence nothing to make
	// the off-the-sphere drawables get draw on top of everything rendered in *all* rendered layers.
	// However we can at least make them get drawn on top *within* the current layer.
	// An example of this is rendered velocity arrows (at topological network triangulation vertices)
	// drawn on top of a filled topological network, both of which are generated by a single layer.
	{
		// Make sure we leave the OpenGL global state the way it was.
		GPlatesOpenGL::GL::StateScope save_restore_drawables_off_sphere_state(gl);

		// Turn on depth writes.
		// Drawables *off* the sphere is the only case where depth writes are enabled.
		// Only enable depth writes if not using a 2D map view.
		if (!d_map_projection)
		{
			gl.DepthMask(GL_TRUE);
		}

		// As mentioned above these off-sphere primitives should not be rendered with any anti-aliasing
		// (including polygon anti-aliasing - which we no longer use because it generates transparent
		// edges between adjacent triangles in a mesh).
		gl.Disable(GL_LINE_SMOOTH);

		drawables_off_the_sphere.end_painting(
				gl,
				d_vertex_element_buffer,
				d_vertex_buffer,
				d_vertex_array,
				d_axially_symmetric_mesh_vertex_array,
				d_render_point_line_polygon_program,
				d_render_axially_symmetric_mesh_program,
				*d_gl_visual_layers,
				view_projection,
				d_map_projection);
	}

	// Render any 2D text last (text specified at 2D viewport positions).
	paint_text_drawables_2D(gl, scale);

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
	paint_text_drawables_3D(gl, scale);

	return cache_handle;
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::paint_scalar_fields(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	// Rendering 3D scalar fields not supported in 2D map views.
	if (d_map_projection)
	{
		return cache_handle_type();
	}

	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Turn on depth writes for correct depth sorting of sub-surface geometries/fields.
	gl.DepthMask(GL_TRUE);

	// The cached view is a sequence of raster caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type> > cache_handle(
			new std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type>());
	cache_handle->reserve(scalar_fields.size());

	for (const ScalarField3DDrawable &scalar_field_drawable : scalar_fields)
	{
		// We don't want to rebuild the OpenGL structures that render the scalar field each frame
		// so those structures need to persist from one render to the next.
		cache_handle_type scalar_field_cache_handle;

		scalar_field_cache_handle = d_gl_visual_layers->render_scalar_field_3d(
				gl,
				view_projection,
				scalar_field_drawable.source_resolved_scalar_field,
				scalar_field_drawable.render_parameters);
		cache_handle->push_back(scalar_field_cache_handle);
	}

	// Now that the scalar fields have been rendered we should clear the drawables list for the next render call.
	scalar_fields.clear();

	return cache_handle;
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::paint_rasters(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// The cached view is a sequence of raster caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type> > cache_handle(
			new std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type>());
	cache_handle->reserve(rasters.size());

	for (const RasterDrawable &raster_drawable : rasters)
	{
		// We don't want to rebuild the OpenGL structures that render the raster each frame
		// so those structures need to persist from one render to the next.
		cache_handle_type raster_cache_handle;

		raster_cache_handle = d_gl_visual_layers->render_raster(
				gl,
				view_projection,
				raster_drawable.source_resolved_raster,
				raster_drawable.source_raster_colour_palette,
				raster_drawable.source_raster_modulate_colour,
				raster_drawable.normal_map_height_field_scale_factor,
				d_map_projection);
		cache_handle->push_back(raster_cache_handle);
	}

	// Now that the rasters have been rendered we should clear the drawables list for the next render call.
	rasters.clear();

	return cache_handle;
}


void
GPlatesGui::LayerPainter::paint_text_drawables_2D(
		GPlatesOpenGL::GL &gl,
		float scale)
{
	for (const TextDrawable2D &text_drawable : text_drawables_2D)
	{
		// render drop shadow, if any
		if (text_drawable.shadow_colour)
		{
			GPlatesOpenGL::GLText::render_text_2D(
					gl,
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
		GPlatesOpenGL::GLText::render_text_2D(
				gl,
				text_drawable.world_x,
				text_drawable.world_y,
				text_drawable.text,
				text_drawable.colour,
				text_drawable.x_offset,
				text_drawable.y_offset,
				text_drawable.font,
				scale);
	}

	// Now that the text has been rendered we should clear the drawables list for the next render call.
	text_drawables_2D.clear();
}


void
GPlatesGui::LayerPainter::paint_text_drawables_3D(
		GPlatesOpenGL::GL &gl,
		float scale)
{
	for (const TextDrawable3D &text_drawable : text_drawables_3D)
	{
		// render drop shadow, if any
		if (text_drawable.shadow_colour)
		{
			GPlatesOpenGL::GLText::render_text_3D(
					gl,
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
		GPlatesOpenGL::GLText::render_text_3D(
				gl,
				text_drawable.world_position.x().dval(),
				text_drawable.world_position.y().dval(),
				text_drawable.world_position.z().dval(),
				text_drawable.text,
				text_drawable.colour,
				text_drawable.x_offset,
				text_drawable.y_offset,
				text_drawable.font,
				scale);
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
		GPlatesOpenGL::GL &gl,
		GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_element_buffer,
		GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_buffer,
		GPlatesOpenGL::GLVertexArray::shared_ptr_type vertex_array,
		GPlatesOpenGL::GLVertexArray::shared_ptr_type axially_symmetric_mesh_vertex_array,
		GPlatesOpenGL::GLProgram::shared_ptr_type render_point_line_polygon_program,
		GPlatesOpenGL::GLProgram::shared_ptr_type render_axially_symmetric_mesh_program,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// If any rendered polygons (or polylines) are 'filled' then render them first.
	// This way any vector geometry in this layer gets rendered on top and hence is visible.
	paint_filled_polygons(gl, gl_visual_layers, view_projection, map_projection);

	//
	// Set up for regular rendering of points, lines and polygons.
	//

	// Also need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	// All point/line/polygon painting below uses the one vertex array so we only need to bind it once (here).
	// Note that the filled polygons above uses it own vertex array(s).
	gl.BindVertexArray(vertex_array);

	//
	// Set the state of the program object used to render points, lines and polygons.
	//
	// Lighting, if enabled, simply uses the normal to the globe as the surface normal.
	// In other words it doesn't consider any surface variations that are present in arbitrary
	// triangular meshes for instance.
	//
	set_point_line_polygon_program_state(
			gl,
			render_point_line_polygon_program,
			gl_visual_layers,
			view_projection,
			map_projection);

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

	d_triangle_drawables.end_painting(gl, GL_TRIANGLES);

	//
	// Paint the drawables representing all line primitives (if any).
	//

	// Iterate over the line width groups and paint them.
	for (line_width_to_drawables_map_type::value_type &line_width_entry : d_line_drawables_map)
	{
		const float line_width = line_width_entry.first.dval();
		Drawables<coloured_vertex_type> &lines_drawable = line_width_entry.second;

		// Set the line width for the current group of lines.
		//
		// Note: Multiply line widths to account for ratio of device pixels to device *independent* pixels.
		//       On high-DPI displays there are more pixels in the same physical area on screen and so
		//       without increasing the line width the lines would look too thin.
		gl.LineWidth(line_width * d_device_pixel_ratio);

		lines_drawable.end_painting(gl, GL_LINES);
	}

	// Clear the lines drawables because the next render may have a different collection of line widths.
	d_line_drawables_map.clear();

	//
	// Paint the drawables representing all point primitives (if any).
	//

	// Iterate over the point size groups and paint them.
	for (point_size_to_drawables_map_type::value_type &point_size_entry : d_point_drawables_map)
	{
		const float point_size = point_size_entry.first.dval();
		Drawables<coloured_vertex_type> &points_drawable = point_size_entry.second;

		// Set the point size for the current group of points.
		//
		// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
		//       On high-DPI displays there are more pixels in the same physical area on screen and so
		//       without increasing the point size the points would look too small.
		gl.PointSize(point_size * d_device_pixel_ratio);

		points_drawable.end_painting( gl, GL_POINTS);
	}

	// Clear the points drawables because the next render may have a different collection of point sizes.
	d_point_drawables_map.clear();

	//
	// Render axially symmetric primitives (if any).
	//

	// Unlike regular point/line/polygon rendering it's quite likely there aren't any axially symmetric primitives
	// to render, so we only bind the vertex array and shader program if there are primitives to render.
	if (d_axially_symmetric_mesh_triangle_drawables.has_primitives())
	{
		// Set the state of the program object used to render axially symmetric primitives.
		set_axially_symmetric_mesh_program_state(
				gl,
				render_axially_symmetric_mesh_program,
				gl_visual_layers,
				view_projection);

		gl.BindVertexArray(axially_symmetric_mesh_vertex_array);

		// Cull back faces since the lighting is not two-sided - the lighting is one-sided and
		// only meant for the front face. If the mesh is closed then this isn't necessary unless
		// the mesh is semi-transparent.
		// We use the currently set state of 'gl.CullFace()' and 'gl.FrontFace()', or the default
		// (cull back faces and front faces are CCW-oriented) if caller has not specified.
		gl.Enable(GL_CULL_FACE);
	}

	// We have to match calls to 'begin_painting()' with calls to 'end_painting()' even if
	// there's no primitives to render.
	d_axially_symmetric_mesh_triangle_drawables.end_painting(gl, GL_TRIANGLES);
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
		GPlatesOpenGL::GL &gl,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		const GPlatesOpenGL::GLViewProjection &view_projection,
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
	if (map_projection) // Rendering to a 2D map view...
	{
		gl_visual_layers.render_filled_polygons(gl, view_projection, d_filled_polygons_map_view);
	}
	else // Rendering to the 3D globe view...
	{
		gl_visual_layers.render_filled_polygons(gl, view_projection, d_filled_polygons_globe_view);
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


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::set_point_line_polygon_program_state(
		GPlatesOpenGL::GL &gl,
		GPlatesOpenGL::GLProgram::shared_ptr_type render_point_line_polygon_program,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection)
{
	// Bind the shader program.
	gl.UseProgram(render_point_line_polygon_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	glUniformMatrix4fv(
			render_point_line_polygon_program->get_uniform_location("view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Set the star colour (it never changes).
	glUniform1i(
			render_point_line_polygon_program->get_uniform_location("map_view_enabled"),
			static_cast<bool>(map_projection));

	// Get the OpenGL light if the runtime system supports it.
	boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light = gl_visual_layers.get_light(gl);

	// Set up lighting if it is enabled.
	if (gl_light &&
		gl_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_GEOMETRY_ON_SPHERE))
	{
		glUniform1i(
				render_point_line_polygon_program->get_uniform_location("lighting_enabled"),
				true);

		if (map_projection)
		{
			// Set the (ambient+diffuse) lighting.
			// For the 2D map views this is constant across the map since the surface normal is
			// constant (it's a flat surface unlike the globe).
			glUniform1f(
					render_point_line_polygon_program->get_uniform_location("map_view_ambient_and_diffuse_lighting"),
					gl_light.get()->get_map_view_constant_lighting());
		}
		else // globe view ...
		{
			// Set the world-space light direction.
			const GPlatesMaths::UnitVector3D &globe_view_world_space_light_direction =
					gl_light.get()->get_globe_view_light_direction();
			glUniform3f(
					render_point_line_polygon_program->get_uniform_location("globe_view_world_space_light_direction"),
					globe_view_world_space_light_direction.x().dval(),
					globe_view_world_space_light_direction.y().dval(),
					globe_view_world_space_light_direction.z().dval());

			// Set the light ambient contribution.
			glUniform1f(
					render_point_line_polygon_program->get_uniform_location("globe_view_light_ambient_contribution"),
					gl_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());
		}
	}
	else // light disabled ...
	{
		glUniform1i(
				render_point_line_polygon_program->get_uniform_location("lighting_enabled"),
				false);
	}
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::set_axially_symmetric_mesh_program_state(
		GPlatesOpenGL::GL &gl,
		GPlatesOpenGL::GLProgram::shared_ptr_type render_axially_symmetric_mesh_program,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	// Bind the shader program.
	gl.UseProgram(render_axially_symmetric_mesh_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	glUniformMatrix4fv(
			render_axially_symmetric_mesh_program->get_uniform_location("view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Get the OpenGL light if the runtime system supports it.
	boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light = gl_visual_layers.get_light(gl);

	// Set up lighting if it is enabled.
	if (gl_light &&
		gl_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_DIRECTION_ARROW))
	{
		glUniform1i(
				render_axially_symmetric_mesh_program->get_uniform_location("lighting_enabled"),
				true);

		// Set the world-space light direction.
		const GPlatesMaths::UnitVector3D &world_space_light_direction =
				gl_light.get()->get_globe_view_light_direction();
		glUniform3f(
				render_axially_symmetric_mesh_program->get_uniform_location("world_space_light_direction"),
				world_space_light_direction.x().dval(),
				world_space_light_direction.y().dval(),
				world_space_light_direction.z().dval());

		// Set the light ambient contribution.
		glUniform1f(
				render_axially_symmetric_mesh_program->get_uniform_location("light_ambient_contribution"),
				gl_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());
	}
	else // light disabled ...
	{
		glUniform1i(
				render_axially_symmetric_mesh_program->get_uniform_location("lighting_enabled"),
				false);
	}
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
		GPlatesOpenGL::GL &gl,
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
		draw_primitives(gl, mode);
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
		GPlatesOpenGL::GL &gl,
		GLenum mode)
{
	// Transfer vertex element data to currently bound vertex element buffer object.
	glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			d_vertex_elements.size() * sizeof(d_vertex_elements[0]),
			d_vertex_elements.data(),
			GL_STREAM_DRAW);

	// Transfer vertex data to currently bound vertex buffer object (ie, currently bound vertex array).
	glBufferData(
			GL_ARRAY_BUFFER,
			d_vertices.size() * sizeof(d_vertices[0]),
			d_vertices.data(),
			GL_STREAM_DRAW);

	// Draw the primitives.
	// NOTE: The caller has already bound this vertex array.
	glDrawElements(
			mode,
			d_vertex_elements.size()/*count*/,
			GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
			0/*indices_offset*/);
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
