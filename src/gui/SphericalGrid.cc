/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 The University of Sydney, Australia
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

#include <cmath>
#include <algorithm>
#include <boost/utility/in_place_factory.hpp>

#include "SphericalGrid.h"

#include "Colour.h"
#include "FeedbackOpenGLToQPainter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GreatCircleArc.h"
#include "maths/SmallCircle.h"
#include "maths/UnitVector3D.h"

#include "opengl/GL.h"
#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexUtils.h"
#include "opengl/GLViewProjection.h"


namespace
{
	// Vertex and fragment shader source code to render grid lines in the 3D globe views (perspective and orthographic).
	const char *VERTEX_SHADER_SOURCE =
		R"(
			uniform mat4 view_projection;
			
			// Only draw front or rear of visible globe using a clip plane (in world space).
			uniform vec4 globe_horizon_plane;
			
			layout(location = 0) in vec4 position;
			layout(location = 1) in vec4 colour;

			out vec4 line_colour;
			
			void main (void)
			{
				gl_Position = view_projection * position;
				
				line_colour = colour;
				
				// Only draw front or rear of visible globe using a clip plane (in world space).
				gl_ClipDistance[0] = dot(position, globe_horizon_plane);
			}
		)";
	const char *FRAGMENT_SHADER_SOURCE =
		R"(
			in vec4 line_colour;
			
			layout(location = 0) out vec4 colour;

			void main (void)
			{
				colour = line_colour;
			}
		)";

	typedef GPlatesOpenGL::GLVertexUtils::ColourVertex vertex_type;
	typedef GLuint vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;

	/**
	 * The angular spacing a points along a line of latitude (small circle).
	 */
	const double LINE_OF_LATITUDE_DELTA_LONGITUDE = GPlatesMaths::convert_deg_to_rad(0.5);

	/**
	 * The angular spacing a points along a line of longitude (great circle).
	 */
	const double LINE_OF_LONGITUDE_DELTA_LATITUDE = GPlatesMaths::convert_deg_to_rad(4);


	/**
	 * Draw a line of latitude for latitude @a lat.
	 * The angle is in radians.
	 */
	void
	stream_line_of_lat(
			stream_primitives_type &stream,
			const double &lat,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		// A small circle at the specified latitude.
		const GPlatesMaths::SmallCircle small_circle = GPlatesMaths::SmallCircle::create_colatitude(
				GPlatesMaths::UnitVector3D::zBasis()/*north pole*/,
				GPlatesMaths::HALF_PI - lat/*colat*/);

		// Tessellate the small circle.
		std::vector<GPlatesMaths::PointOnSphere> points;
		tessellate(points, small_circle, LINE_OF_LATITUDE_DELTA_LONGITUDE);

		// Stream the tessellated points.
		stream_primitives_type::LineLoops stream_line_loops(stream);
		stream_line_loops.begin_line_loop();

		std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_iter = points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_end = points.end();
		for ( ; points_iter != points_end; ++points_iter)
		{
			const vertex_type vertex(points_iter->position_vector(), colour);

			ok = ok && stream_line_loops.add_vertex(vertex);
		}

		// Close off the loop to the first vertex of the line loop.
		ok = ok && stream_line_loops.end_line_loop();

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	/**
	 * Draw a line of longitude for longitude @a lon from the north pole to the south pole.
	 * The angle is in radians.
	 */
	void
	stream_line_of_lon(
			stream_primitives_type &stream,
			const double &lon,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		// Use two great circle arcs to form the great circle arc from north to south pole.
		// intersecting at the equator.
		const GPlatesMaths::PointOnSphere equatorial_point(
				GPlatesMaths::UnitVector3D(std::cos(lon), std::sin(lon), 0));
		const GPlatesMaths::GreatCircleArc great_circle_arcs[2] =
		{
			GPlatesMaths::GreatCircleArc::create(GPlatesMaths::PointOnSphere::north_pole, equatorial_point),
			GPlatesMaths::GreatCircleArc::create(equatorial_point, GPlatesMaths::PointOnSphere::south_pole)
		};

		for (const GPlatesMaths::GreatCircleArc &great_circle_arc : great_circle_arcs)
		{
			// Tessellate the great circle arc.
			std::vector<GPlatesMaths::PointOnSphere> points;
			tessellate(points, great_circle_arc, LINE_OF_LONGITUDE_DELTA_LATITUDE);

			// Stream the tessellated points.
			stream_primitives_type::LineStrips stream_line_strips(stream);
			stream_line_strips.begin_line_strip();

			std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_iter = points.begin();
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_end = points.end();
			for ( ; points_iter != points_end; ++points_iter)
			{
				const vertex_type vertex(points_iter->position_vector(), colour);

				ok = ok && stream_line_strips.add_vertex(vertex);
			}

			stream_line_strips.end_line_strip();
		}

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	void
	create_grid_lines(
			std::vector<vertex_type> &vertices,
			std::vector<vertex_element_type> &vertex_elements,
			const GPlatesMaths::Real &delta_lat,
			const GPlatesMaths::Real &delta_lon,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type stream;

		stream_primitives_type::StreamTarget stream_target(stream);

		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		if (delta_lat != 0.0)
		{
			// Add lines of latitude: go down from +PI/2 to -PI/2.
			GPlatesMaths::Real curr_lat = GPlatesMaths::HALF_PI - delta_lat;
			while (curr_lat > -GPlatesMaths::HALF_PI)
			{
				stream_line_of_lat(stream, curr_lat.dval(), colour);
				curr_lat -= delta_lat;
			}
		}

		if (delta_lon != 0.0)
		{
			// Add lines of longitude; go east from -PI to +PI.
			GPlatesMaths::Real curr_lon = -GPlatesMaths::PI;
			while (curr_lon < GPlatesMaths::PI)
			{
				stream_line_of_lon(stream, curr_lon.dval(), colour);
				curr_lon += delta_lon;
			}
		}

		stream_target.stop_streaming();

#if 0 // Update: using 32-bit indices now...
		// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
		// Shouldn't get close really but check to be sure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);
#endif
	}


	void
	load_grid_lines(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLVertexArray::shared_ptr_type vertex_array,
			GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_buffer,
			GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_element_buffer,
			const std::vector<vertex_type> &vertices,
			const std::vector<vertex_element_type> &vertex_elements)
	{
		// Bind vertex array object.
		gl.BindVertexArray(vertex_array);

		// Bind vertex element buffer object to currently bound vertex array object.
		gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_element_buffer);

		// Transfer vertex element data to currently bound vertex element buffer object.
		glBufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				vertex_elements.size() * sizeof(vertex_elements[0]),
				vertex_elements.data(),
				GL_STATIC_DRAW);

		// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
		gl.BindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		// Transfer vertex data to currently bound vertex buffer object.
		glBufferData(
				GL_ARRAY_BUFFER,
				vertices.size() * sizeof(vertices[0]),
				vertices.data(),
				GL_STATIC_DRAW);

		// Specify vertex attributes (position and colour) in currently bound vertex buffer object.
		// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
		// to currently bound vertex array object.
		gl.EnableVertexAttribArray(0);
		gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_type), BUFFER_OFFSET(vertex_type, x));
		gl.EnableVertexAttribArray(1);
		gl.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_type), BUFFER_OFFSET(vertex_type, colour));
	}


	void
	compile_link_program(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLProgram::shared_ptr_type program)
	{
		// Vertex shader source.
		GPlatesOpenGL::GLShaderSource vertex_shader_source;
		vertex_shader_source.add_code_segment(VERTEX_SHADER_SOURCE);

		// Vertex shader.
		GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
		vertex_shader->shader_source(vertex_shader_source);
		vertex_shader->compile_shader();

		// Fragment shader source.
		GPlatesOpenGL::GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment(FRAGMENT_SHADER_SOURCE);

		// Fragment shader.
		GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
		fragment_shader->shader_source(fragment_shader_source);
		fragment_shader->compile_shader();

		// Vertex-fragment program.
		program->attach_shader(vertex_shader);
		program->attach_shader(fragment_shader);
		program->link_program();
	}
}


GPlatesGui::SphericalGrid::SphericalGrid(
		GPlatesOpenGL::GL &gl,
		const GraticuleSettings &graticule_settings) :
	d_graticule_settings(graticule_settings),
	d_program(GPlatesOpenGL::GLProgram::create(gl)),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(gl)),
	d_vertex_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_vertex_element_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_num_vertex_indices(0)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	compile_link_program(gl, d_program);
}


void
GPlatesGui::SphericalGrid::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const GPlatesOpenGL::GLIntersect::Plane &globe_horizon_plane)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Check whether we need to (re)generate the grid lines.
	//
	// Note: This will always happen on the first paint.
	if (d_last_seen_graticule_settings != d_graticule_settings)
	{
		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		create_grid_lines(
				vertices,
				vertex_elements,
				d_graticule_settings.get_delta_lat(),
				d_graticule_settings.get_delta_lon(),
				Colour::to_rgba8(d_graticule_settings.get_colour()));
		d_num_vertex_indices = vertex_elements.size();

		load_grid_lines(gl, d_vertex_array, d_vertex_buffer, d_vertex_element_buffer, vertices, vertex_elements);

		d_last_seen_graticule_settings = d_graticule_settings;
	}

	// Set the anti-aliased line state.
	gl.Enable(GL_LINE_SMOOTH);
	gl.Hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	gl.LineWidth(d_graticule_settings.get_line_width_hint());

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
	gl.Enable(GL_BLEND);
	gl.BlendFuncSeparate(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,  // RGB
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA);       // Alpha

	// Use the shader program.
	gl.UseProgram(d_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	glUniformMatrix4fv(
			d_program->get_uniform_location("view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Only draw front or rear of visible globe using a clip plane (in world space).
	//
	// This ensures the correct draw order of geometries on the surface of the globe since depth writes
	// are turned off. For example, if the globe is semi-transparent (due to a visible 3D scalar field)
	// then the rear of globe is rendered in a first pass, followed by the scalar field inside the globe
	// in a second pass and finally the front of the globe in a third pass.
	gl.Enable(GL_CLIP_DISTANCE0);
	GLfloat globe_horizon_float_plane[4];
	globe_horizon_plane.get_float_plane(globe_horizon_float_plane);
	glUniform4fv(
			d_program->get_uniform_location("globe_horizon_plane"),
			1, globe_horizon_float_plane);

	// Bind the vertex array.
	gl.BindVertexArray(d_vertex_array);

	// Temporarily disable OpenGL feedback (eg, to SVG) until it's re-implemented using OpenGL 3...
#if 1
	// Draw the grid lines.
	glDrawElements(
			GL_LINES,
			d_num_vertex_indices/*count*/,
			GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
			nullptr/*indices_offset*/);
#else
	// Either render directly to the framebuffer, or use OpenGL feedback to render to the
	// QPainter's paint device.
	if (renderer.rendering_to_context_framebuffer())
	{
		renderer.apply_compiled_draw_state(*d_grid_compiled_draw_state.get());
	}
	else
	{
		// Create an OpenGL feedback buffer large enough to capture the primitives we're about to render.
		// We are rendering to the QPainter attached to GLRenderer.
		FeedbackOpenGLToQPainter feedback_opengl;
		FeedbackOpenGLToQPainter::VectorGeometryScope vector_geometry_scope(
				feedback_opengl, renderer, 0, d_num_vertex_indices, 0);

		renderer.apply_compiled_draw_state(*d_grid_compiled_draw_state.get());
	}
#endif
}
