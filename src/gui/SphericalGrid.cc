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

#include <cmath>
#include <algorithm>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>

#include "SphericalGrid.h"

#include "Colour.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GreatCircleArc.h"
#include "maths/SmallCircle.h"
#include "maths/UnitVector3D.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLVertex.h"
#include "opengl/GLVertexArray.h"


#include <boost/foreach.hpp>
namespace
{
	typedef GPlatesOpenGL::GLColourVertex vertex_type;
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
	 * Sets the OpenGL state set that defines the appearance of the grid lines.
	 */
	void
	set_line_draw_state(
			GPlatesOpenGL::GLRenderer &renderer)
	{
		// Set the anti-aliased line state.
		renderer.gl_enable(GL_LINE_SMOOTH);
		renderer.gl_hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		renderer.gl_line_width(1.0f);

		// Set up alpha blending for pre-multiplied alpha.
		// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
		// This is where the RGB channels have already been multiplied by the alpha channel.
		// See class GLVisualRasterSource for why this is done.
		//
		// To generate pre-multiplied alpha we'll use separate alpha-blend (src,dst) factors for the alpha channel...
		//
		//   RGB uses (src_alpha, 1 - src_alpha)  ->  (R,G,B) = (Rs*As,Gs*As,Bs*As) + (1-As) * (Rd,Gd,Bd)
		//     A uses (1, 1 - src_alpha)          ->        A = As + (1-As) * Ad
		if (GPlatesOpenGL::GLContext::get_parameters().framebuffer.gl_EXT_blend_func_separate)
		{
			renderer.gl_enable(GL_BLEND);
			renderer.gl_blend_func_separate(
					GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		else // otherwise resort to normal blending...
		{
			renderer.gl_enable(GL_BLEND);
			renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
	}


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

		BOOST_FOREACH(const GPlatesMaths::GreatCircleArc &great_circle_arc, great_circle_arcs)
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
	undo_rotation(
			GPlatesOpenGL::GLMatrix &transform,
			const GPlatesMaths::UnitVector3D &axis,
			double angle_in_deg)
	{
		// Undo the rotation done in Globe so that the disk always faces the camera.
		transform.gl_rotate(-angle_in_deg, axis.x().dval(), axis.y().dval(), axis.z().dval());
	}


	GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
	compile_grid_draw_state(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLVertexArray &vertex_array,
			const GPlatesMaths::Real &delta_lat,
			const GPlatesMaths::Real &delta_lon,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type stream;

		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
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
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);
#endif

		// Streamed line strips end up as indexed lines.
		const GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type draw_vertex_array =
				compile_vertex_array_draw_state(
						renderer, vertex_array, vertices, vertex_elements, GL_LINES);

		// Start compiling draw state that includes line drawing state and the vertex array draw command.
		GPlatesOpenGL::GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

		set_line_draw_state(renderer);
		renderer.apply_compiled_draw_state(*draw_vertex_array);

		return compile_draw_state_scope.get_compiled_draw_state();
	}


	GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
	compile_circumference_draw_state(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLVertexArray &vertex_array,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type stream;

		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		stream_primitives_type::StreamTarget stream_target(stream);
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		stream_line_of_lon(stream, GPlatesMaths::PI / 2.0, colour);
		stream_line_of_lon(stream, -GPlatesMaths::PI / 2.0, colour);

		stream_target.stop_streaming();

#if 0 // Update: using 32-bit indices now...
		// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
		// Shouldn't get close really but check to be sure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);
#endif

		// Streamed line strips end up as indexed lines.
		const GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type draw_vertex_array =
				compile_vertex_array_draw_state(
						renderer, vertex_array, vertices, vertex_elements, GL_LINES);

		// Start compiling draw state that includes line drawing state and the vertex array draw command.
		GPlatesOpenGL::GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

		set_line_draw_state(renderer);
		renderer.apply_compiled_draw_state(*draw_vertex_array);

		return compile_draw_state_scope.get_compiled_draw_state();
	}
}


GPlatesGui::SphericalGrid::SphericalGrid(
		GPlatesOpenGL::GLRenderer &renderer,
		const GraticuleSettings &graticule_settings) :
	d_graticule_settings(graticule_settings),
	d_grid_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer)),
	d_circumference_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer))
{  }


void
GPlatesGui::SphericalGrid::paint(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Check whether we need to compile a new draw state.
	if (!d_grid_compiled_draw_state ||
		!d_last_seen_graticule_settings ||
		*d_last_seen_graticule_settings != d_graticule_settings)
	{
		d_grid_compiled_draw_state = compile_grid_draw_state(
				renderer,
				*d_grid_vertex_array,
				d_graticule_settings.get_delta_lat(),
				d_graticule_settings.get_delta_lon(),
				Colour::to_rgba8(d_graticule_settings.get_colour()));
		d_last_seen_graticule_settings = d_graticule_settings;
	}

	renderer.apply_compiled_draw_state(*d_grid_compiled_draw_state.get());
}


void
GPlatesGui::SphericalGrid::paint_circumference(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesMaths::UnitVector3D &axis,
		double angle_in_deg)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Check whether we need to compile a new draw state.
	if (!d_circumference_compiled_draw_state ||
		!d_last_seen_graticule_settings ||
		d_last_seen_graticule_settings->get_colour() != d_graticule_settings.get_colour())
	{
		d_circumference_compiled_draw_state = compile_circumference_draw_state(
				renderer,
				*d_circumference_vertex_array,
				Colour::to_rgba8(d_graticule_settings.get_colour()));
		d_last_seen_graticule_settings = d_graticule_settings;
	}

	GPlatesOpenGL::GLMatrix transform;
	undo_rotation(transform, axis, angle_in_deg);

	renderer.gl_mult_matrix(GL_MODELVIEW, transform);

	renderer.apply_compiled_draw_state(*d_circumference_compiled_draw_state.get());
}
