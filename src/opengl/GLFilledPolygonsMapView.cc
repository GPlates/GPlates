/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <QDebug>

#include "GLFilledPolygonsMapView.h"

#include "GLContext.h"
#include "GLVertexUtils.h"
#include "GLViewProjection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/CallStackTracker.h"
#include "utils/Profile.h"


namespace
{
	// Vertex and fragment shader source code to render filled polygons.
	const char *VERTEX_SHADER_SOURCE =
		R"(
			uniform mat4 view_projection;
			
			layout(location = 0) in vec4 position;
			layout(location = 1) in vec4 colour;

			out vec4 fill_colour;
			
			void main (void)
			{
				gl_Position = view_projection * position;
				fill_colour = colour;
			}
		)";
	const char *FRAGMENT_SHADER_SOURCE =
		R"(
			in vec4 fill_colour;
			
			layout(location = 0) out vec4 colour;

			void main (void)
			{
				colour = fill_colour;
			}
		)";
}


GPlatesOpenGL::GLFilledPolygonsMapView::GLFilledPolygonsMapView(
		GL &gl) :
	d_drawables_vertex_array(GLVertexArray::create(gl)),
	d_drawables_vertex_buffer(GLBuffer::create(gl)),
	d_drawables_vertex_element_buffer(GLBuffer::create(gl)),
	d_program(GLProgram::create(gl))
{
	create_drawables_vertex_array(gl);
	compile_link_shader_program(gl);
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::render(
		GL &gl,
		const GLViewProjection &view_projection,
		const filled_drawables_type &filled_drawables)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// If there are no filled drawables to render then return early.
	if (filled_drawables.d_drawable_vertex_elements.empty())
	{
		return;
	}

	// Write the vertices/indices of all filled drawables (gathered by the client) into our
	// vertex buffer and vertex element buffer.
	write_filled_drawables_to_vertex_array(gl, filled_drawables);

	// Clear the stencil buffer.
	gl.ClearStencil();
	gl.Clear(GL_STENCIL_BUFFER_BIT);

	//
	// For alpha-blending we want:
	//
	//   RGB = A_src * RGB_src + (1-A_src) * RGB_dst
	//     A =     1 *   A_src + (1-A_src) *   A_dst
	//
	// ...so we need to use separate (src,dst) blend factors for the RGB and alpha channels...
	//
	//   RGB uses (A_src, 1 - A_src)
	//     A uses (    1, 1 - A_src)
	//
	// ...this enables the destination to be a texture that is subsequently blended into the final scene.
	// In this case the destination alpha must be correct in order to properly blend the texture into the final scene.
	// However if we're rendering directly into the scene (ie, no render-to-texture) then destination alpha is not
	// actually used (since only RGB in the final scene is visible) and therefore could use same blend factors as RGB.
	//
	gl.BlendFuncSeparate(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Enable stencil writes (this is the default OpenGL state anyway).
	gl.StencilMask(~0);

	// Enable stencil testing.
	gl.Enable(GL_STENCIL_TEST);

	// Bind the shader program for rendering.
	gl.UseProgram(d_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind the vertex array before using it to draw.
	gl.BindVertexArray(d_drawables_vertex_array);

	// Iterate over the filled drawables and render each one into the scene.
	for (const filled_drawable_type& filled_drawable : filled_drawables.d_filled_drawables)
	{
		// Set the stencil function to always pass.
		gl.StencilFunc(GL_ALWAYS, 0, ~0);
		// Set the stencil operation to invert the stencil buffer value every time a pixel is
		// rendered (this means we get 1 where a pixel is covered by an odd number of triangles
		// and 0 by an even number of triangles).
		gl.StencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

		// Disable colour writes and alpha blending.
		// We only want to modify the stencil buffer on this pass.
		gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		gl.Disable(GL_BLEND);

		// Render the current filled drawable.
		gl.DrawRangeElements(
				GL_TRIANGLES,
				filled_drawable.d_drawable.start,
				filled_drawable.d_drawable.end,
				filled_drawable.d_drawable.count,
				GLVertexUtils::ElementTraits<drawable_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(filled_drawable.d_drawable.indices_offset));

		// Set the stencil function to pass only if the stencil buffer value is non-zero.
		// This means we only draw into the tile texture for pixels 'interior' to the filled drawable.
		gl.StencilFunc(GL_NOTEQUAL, 0, ~0);
		// Set the stencil operation to set the stencil buffer to zero in preparation
		// for the next drawable (also avoids multiple alpha-blending due to overlapping fan
		// triangles as mentioned below).
		gl.StencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

		// Re-enable colour writes and alpha blending.
		gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		gl.Enable(GL_BLEND);

		// Render the current filled drawable.
		// This drawable covers at least all interior pixels of the filled drawable.
		// It also can covers exterior pixels of the filled drawable.
		// However only the interior pixels (where stencil buffer is non-zero) will
		// pass the stencil test and get written into the tile (colour) texture.
		// The drawable also can render pixels multiple times due to overlapping fan triangles.
		// To avoid alpha blending each pixel more than once, the above stencil operation zeros
		// the stencil buffer value of each pixel that passes the stencil test such that the next
		// overlapping pixel will then fail the stencil test (avoiding multiple-alpha-blending).
		gl.DrawRangeElements(
				GL_TRIANGLES,
				filled_drawable.d_drawable.start,
				filled_drawable.d_drawable.end,
				filled_drawable.d_drawable.count,
				GLVertexUtils::ElementTraits<drawable_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(filled_drawable.d_drawable.indices_offset));
	}
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::create_drawables_vertex_array(
		GL &gl)
{
	// Bind vertex element buffer object to the vertex array object.
	gl.VertexArrayElementBuffer(d_drawables_vertex_array, d_drawables_vertex_element_buffer);

	// Bind vertex buffer object to the vertex array object.
	gl.VertexArrayVertexBuffer(d_drawables_vertex_array, 0/*bindingindex*/, d_drawables_vertex_buffer, 0/*offset*/, sizeof(drawable_vertex_type));

	// Specify vertex attributes (position and colour).
	gl.EnableVertexArrayAttrib(d_drawables_vertex_array, 0);
	gl.VertexArrayAttribFormat(d_drawables_vertex_array, 0, 3, GL_FLOAT, GL_FALSE, ATTRIB_OFFSET_IN_VERTEX(drawable_vertex_type, x));
	gl.VertexArrayAttribBinding(d_drawables_vertex_array, 0, 0/*bindingindex*/);

	gl.EnableVertexArrayAttrib(d_drawables_vertex_array, 1);
	gl.VertexArrayAttribFormat(d_drawables_vertex_array, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, ATTRIB_OFFSET_IN_VERTEX(drawable_vertex_type, colour));
	gl.VertexArrayAttribBinding(d_drawables_vertex_array, 1, 0/*bindingindex*/);
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::write_filled_drawables_to_vertex_array(
		GL &gl,
		const filled_drawables_type &filled_drawables)
{
	// Transfer vertex element data to the vertex element buffer object.
	gl.NamedBufferData(
			d_drawables_vertex_element_buffer,
			filled_drawables.d_drawable_vertex_elements.size() * sizeof(filled_drawables.d_drawable_vertex_elements[0]),
			filled_drawables.d_drawable_vertex_elements.data(),
			GL_STATIC_DRAW);

	// Transfer vertex element data to the vertex buffer object.
	gl.NamedBufferData(
			d_drawables_vertex_buffer,
			filled_drawables.d_drawable_vertices.size() * sizeof(filled_drawables.d_drawable_vertices[0]),
			filled_drawables.d_drawable_vertices.data(),
			GL_STATIC_DRAW);

	//qDebug() << "Writing triangles: " << filled_drawables.d_drawable_vertex_elements.size() / 3;
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::compile_link_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	//
	// Shader program to render filled drawables to the scene.
	//

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment(VERTEX_SHADER_SOURCE);

	// Vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment(FRAGMENT_SHADER_SOURCE);

	// Fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_program->attach_shader(gl, vertex_shader);
	d_program->attach_shader(gl, fragment_shader);
	d_program->link_program(gl);
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::add_filled_polygon(
		const std::vector<QPointF> &line_geometry,
		GPlatesGui::rgba8_t rgba8_color)
{
	const unsigned int num_points = line_geometry.size();

	// Need at least three points for a polygon.
	if (num_points < 3)
	{
		return;
	}

	begin_filled_drawable();

	add_line_geometry_to_current_filled_drawable(line_geometry, rgba8_color);

	end_filled_drawable();
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::add_filled_polygon(
		const std::vector< std::vector<QPointF> > &line_geometries,
		GPlatesGui::rgba8_t rgba8_color)
{
	if (line_geometries.empty())
	{
		return;
	}

	begin_filled_drawable();

	for (unsigned int line_geometry_index = 0; line_geometry_index < line_geometries.size(); ++line_geometry_index)
	{
		add_line_geometry_to_current_filled_drawable(line_geometries[line_geometry_index], rgba8_color);
	}

	end_filled_drawable();
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::add_line_geometry_to_current_filled_drawable(
		const std::vector<QPointF> &line_geometry,
		GPlatesGui::rgba8_t rgba8_color)
{
	const unsigned int num_points = line_geometry.size();
	// Need at least three points for a polygon ring.
	if (num_points < 3)
	{
		return;
	}

	unsigned int n;

	// Calculate centroid of polygon ring.
	QPointF centroid;
	for (n = 0; n < num_points; ++n)
	{
		centroid += line_geometry[n];
	}
	centroid /= num_points;

	//
	// Create the OpenGL coloured vertices for the filled polygon (fan) mesh.
	//

	const GLsizei initial_vertex_elements_size = d_drawable_vertex_elements.size();
	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();
	drawable_vertex_element_type vertex_index = base_vertex_index;

	// First vertex is the centroid.
	d_drawable_vertices.push_back(
			drawable_vertex_type(
					centroid.x(),
					centroid.y(),
					0/*z*/,
					rgba8_color));
	++vertex_index;

	// The remaining vertices form the boundary.
	for (n = 0; n < num_points; ++n, ++vertex_index)
	{
		d_drawable_vertices.push_back(
				drawable_vertex_type(
						line_geometry[n].x(),
						line_geometry[n].y(),
						0/*z*/,
						rgba8_color));

		d_drawable_vertex_elements.push_back(base_vertex_index); // Centroid.
		d_drawable_vertex_elements.push_back(vertex_index); // Current boundary point.
		d_drawable_vertex_elements.push_back(vertex_index + 1); // Next boundary point.
	}

	// Wraparound back to the first boundary vertex to close off the polygon.
	d_drawable_vertices.push_back(
			drawable_vertex_type(
					line_geometry[0].x(),
					line_geometry[0].y(),
					0/*z*/,
					rgba8_color));

	// Update the current filled drawable.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);
	d_current_drawable->end = vertex_index;
	d_current_drawable->count += d_drawable_vertex_elements.size() - initial_vertex_elements_size;
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::add_filled_triangle_to_mesh(
		const QPointF &vertex1,
		const QPointF &vertex2,
		const QPointF &vertex3,
		GPlatesGui::rgba8_t rgba8_color)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();

	d_drawable_vertices.push_back(drawable_vertex_type(vertex1.x(), vertex1.y(), 0/*z*/, rgba8_color));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex2.x(), vertex2.y(), 0/*z*/, rgba8_color));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex3.x(), vertex3.y(), 0/*z*/, rgba8_color));

	d_drawable_vertex_elements.push_back(base_vertex_index);
	d_drawable_vertex_elements.push_back(base_vertex_index + 1);
	d_drawable_vertex_elements.push_back(base_vertex_index + 2);

	// Update the current filled drawable.
	d_current_drawable->end += 3;
	d_current_drawable->count += 3;
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::add_filled_triangle_to_mesh(
		const QPointF &vertex1,
		const QPointF &vertex2,
		const QPointF &vertex3,
		GPlatesGui::rgba8_t rgba8_vertex_color1,
		GPlatesGui::rgba8_t rgba8_vertex_color2,
		GPlatesGui::rgba8_t rgba8_vertex_color3)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();

	// Alpha blending will be set up for pre-multiplied alpha.
	d_drawable_vertices.push_back(drawable_vertex_type(vertex1.x(), vertex1.y(), 0/*z*/, rgba8_vertex_color1));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex2.x(), vertex2.y(), 0/*z*/, rgba8_vertex_color2));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex3.x(), vertex3.y(), 0/*z*/, rgba8_vertex_color3));

	d_drawable_vertex_elements.push_back(base_vertex_index);
	d_drawable_vertex_elements.push_back(base_vertex_index + 1);
	d_drawable_vertex_elements.push_back(base_vertex_index + 2);

	// Update the current filled drawable.
	d_current_drawable->end += 3;
	d_current_drawable->count += 3;
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::begin_filled_drawable()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	const GLsizei base_vertex_element_index = d_drawable_vertex_elements.size();
	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();

	d_current_drawable = boost::in_place(
			base_vertex_index/*start*/,
			base_vertex_index/*end*/, // This will get updated.
			0/*count*/, // This will get updated.
			base_vertex_element_index * sizeof(drawable_vertex_element_type)/*indices_offset*/);
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledDrawables::end_filled_drawable()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	// Add the filled drawable if it's not empty.
	if (d_current_drawable->count > 0)
	{
		const filled_drawable_type filled_drawable(d_current_drawable.get());
		d_filled_drawables.push_back(filled_drawable);
	}

	// Finished with the current filled drawable.
	d_current_drawable = boost::none;
}
