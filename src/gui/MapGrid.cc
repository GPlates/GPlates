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

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>
#include <opengl/OpenGL.h>

#include "MapGrid.h"

#include "Colour.h"
#include "ProjectionException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLVertex.h"
#include "opengl/GLVertexArray.h"

#include <boost/foreach.hpp>

namespace
{
	// Vertex stream.
	typedef GPlatesOpenGL::GLColourVertex vertex_type;
	typedef GLuint vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;

	// Projection coordinates.
	typedef std::pair<double, double> projection_coord_type;
	typedef std::vector<projection_coord_type> projection_coord_seq_type;

	/**
	 * The number of line segments along a line of latitude.
	 */
	const int LINE_OF_LATITUDE_NUM_SEGMENTS = 100;

	/**
	 * The number of line segments along a line of longitude.
	 */
	const int LINE_OF_LONGITUDE_NUM_SEGMENTS = 200;

	// LATITUDE_MARGIN represents the number of degrees above -90 and below 90 between
	// which each line of longitude will be drawn. This gives you a little space
	// at the top and bottom, so that grid lines which converge at the poles don't
	// look too busy. 

	//const double LATITUDE_MARGIN = 1.5;
	const double LATITUDE_MARGIN = 0.0;

	/**
	 * The angular spacing a points along a line of latitude.
	 */
	const double LINE_OF_LATITUDE_DELTA_LONGITUDE = 360.0 / LINE_OF_LATITUDE_NUM_SEGMENTS;

	/**
	 * The angular spacing a points along a line of longitude.
	 */
	const double LINE_OF_LONGITUDE_DELTA_LATITUDE = (180.0 - 2.0 * LATITUDE_MARGIN) / LINE_OF_LONGITUDE_NUM_SEGMENTS;


	/**
	 * Projects the specified latitude/longitude using the specified map projection.
	 */
	projection_coord_type
	project_lat_lon(
			double lat,
			double lon,
			const GPlatesGui::MapProjection &projection)
	{
		projection.forward_transform(lon, lat);
		return projection_coord_type(lon, lat);
	}


	/**
	 * Sets the OpenGL state set that defines the appearance of the grid lines.
	 */
	void
	set_line_draw_state(
			GPlatesOpenGL::GLRenderer &renderer)
	{
		// Set the alpha-blend state.
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Set the anti-aliased line state.
		renderer.gl_enable(GL_LINE_SMOOTH);
		renderer.gl_hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		renderer.gl_line_width(1.0f);
	}


	/**
	 * Draw lines of latitude.
	 */
	void
	stream_lines_of_lat(
			stream_primitives_type &stream,
			const GPlatesGui::MapProjection &map_projection,
			const double &lat_0,
			const double &lon_0,
			const double &delta_lat,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		stream_primitives_type::LineStrips stream_line_strips(stream);

		// Lines of latitude.
		GPlatesMaths::real_t lat = lat_0;
		bool loop = true;
		while (loop)
		{
			if (lat <= -90)
			{
				loop = false;
				lat = -90;
			}

			// Project the vertices using the map projection.
			projection_coord_seq_type projected_coords;
			try
			{
				GPlatesMaths::real_t lon = lon_0;
				projected_coords.push_back(project_lat_lon(lat.dval(), lon.dval(), map_projection));
				for (int n = 0; n < LINE_OF_LATITUDE_NUM_SEGMENTS; ++n)
				{
					lon += LINE_OF_LATITUDE_DELTA_LONGITUDE;
					projected_coords.push_back(
							project_lat_lon(lat.dval(), lon.dval(), map_projection));
				}
			}
			catch (const GPlatesGui::ProjectionException &exc)
			{
				// Ignore exception.
				qWarning() << exc;

				// Only draw if no projection exceptions thrown.
				continue;
			}

			// Stream the projected vertices into the line stream.
			stream_line_strips.begin_line_strip();

			for (projection_coord_seq_type::const_iterator projected_iter = projected_coords.begin();
				projected_iter != projected_coords.end();
				++projected_iter)
			{
				const projection_coord_type &projected_coord = *projected_iter;
				const vertex_type vertex(
						projected_coord.first, projected_coord.second, 0/*z*/, colour);

				ok = ok && stream_line_strips.add_vertex(vertex);
			}

			stream_line_strips.end_line_strip();

			lat -= delta_lat;
		}

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	/**
	 * Draw lines of longitude.
	 */
	void
	stream_lines_of_lon(
			stream_primitives_type &stream,
			const GPlatesGui::MapProjection &map_projection,
			const double &lat_0,
			const double &lon_0,
			const double &delta_lon,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		stream_primitives_type::LineStrips stream_line_strips(stream);

		// Lines of longitude.
		GPlatesMaths::real_t lon = lon_0;
		bool loop = true;
		while (loop)
		{
			if (lon >= lon_0 + 360)
			{
				loop = false;
				lon = lon_0 + 360;
			}

			// Project the vertices using the map projection.
			projection_coord_seq_type projected_coords;
			try
			{
				GPlatesMaths::real_t lat = lat_0 + LATITUDE_MARGIN;
				projected_coords.push_back(project_lat_lon(lat.dval(), lon.dval(), map_projection));
				for (int n = 0; n < LINE_OF_LONGITUDE_NUM_SEGMENTS; ++n)
				{
					lat -= LINE_OF_LONGITUDE_DELTA_LATITUDE;
					projected_coords.push_back(
							project_lat_lon(lat.dval(), lon.dval(), map_projection));
				}
			}
			catch (const GPlatesGui::ProjectionException &exc)
			{
				// Ignore exception.
				qWarning() << exc;

				// Only draw if no projection exceptions thrown.
				continue;
			}

			// Stream the projected vertices into the line stream.
			stream_line_strips.begin_line_strip();

			for (projection_coord_seq_type::const_iterator projected_iter = projected_coords.begin();
				projected_iter != projected_coords.end();
				++projected_iter)
			{
				const projection_coord_type &projected_coord = *projected_iter;
				const vertex_type vertex(
						projected_coord.first, projected_coord.second, 0/*z*/, colour);

				ok = ok && stream_line_strips.add_vertex(vertex);
			}

			stream_line_strips.end_line_strip();

			lon += delta_lon;
		}

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
	compile_grid_draw_state(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLVertexArray &vertex_array,
			const GPlatesGui::MapProjection &map_projection,
			const GPlatesMaths::Real &delta_lat,
			const GPlatesMaths::Real &delta_lon,
			const GPlatesGui::rgba8_t &colour)
	{
		const double lat_0 = 90;
		const double lon_0 = map_projection.central_llp().longitude() - 180;

		stream_primitives_type stream;

		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		stream_primitives_type::StreamTarget stream_target(stream);
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		if (delta_lat != 0.0)
		{
			stream_lines_of_lat(stream, map_projection, lat_0, lon_0, delta_lat.dval(), colour);
		}

		if (delta_lon != 0.0)
		{
			stream_lines_of_lon(stream, map_projection, lat_0, lon_0, delta_lon.dval(), colour);
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
}


GPlatesGui::MapGrid::MapGrid(
		GPlatesOpenGL::GLRenderer &renderer,
		const MapProjection &map_projection,
		const GraticuleSettings &graticule_settings) :
	d_map_projection(map_projection),
	d_graticule_settings(graticule_settings),
	d_grid_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer))
{  }


void
GPlatesGui::MapGrid::paint(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	const MapProjectionSettings map_projection_settings = d_map_projection.get_projection_settings();

	// Check whether we need to compile a new draw state.
	if (!d_grid_compiled_draw_state ||
		!d_last_seen_graticule_settings ||
		*d_last_seen_graticule_settings != d_graticule_settings ||
		!d_last_seen_map_projection_settings ||
		*d_last_seen_map_projection_settings != map_projection_settings)
	{
		d_grid_compiled_draw_state = compile_grid_draw_state(
				renderer,
				*d_grid_vertex_array,
				d_map_projection,
				GPlatesMaths::convert_rad_to_deg(d_graticule_settings.get_delta_lat()),
				GPlatesMaths::convert_rad_to_deg(d_graticule_settings.get_delta_lon()),
				Colour::to_rgba8(d_graticule_settings.get_colour()));

		d_last_seen_graticule_settings = d_graticule_settings;
		d_last_seen_map_projection_settings = map_projection_settings;
	}

	renderer.apply_compiled_draw_state(*d_grid_compiled_draw_state.get());
}
