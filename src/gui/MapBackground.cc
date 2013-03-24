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

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>
#include <opengl/OpenGL.h>

#include "MapBackground.h"

#include "Colour.h"
#include "FeedbackOpenGLToQPainter.h"
#include "ProjectionException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLVertex.h"
#include "opengl/GLVertexArray.h"

#include "presentation/ViewState.h"


namespace
{
	// Vertex stream.
	typedef GPlatesOpenGL::GLColourVertex vertex_type;
	typedef GLuint vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;

	// Projection coordinates.
	typedef std::pair<double, double> projection_coord_type;

	/**
	 * The number of line segments along a line of latitude.
	 */
	const int LINE_OF_LATITUDE_NUM_SEGMENTS = 100;

	/**
	 * The number of line segments along a line of longitude.
	 */
	const int LINE_OF_LONGITUDE_NUM_SEGMENTS = 200;

	/**
	 * The angular spacing a points along a line of latitude.
	 */
	const double LINE_OF_LATITUDE_DELTA_LONGITUDE = 360.0 / LINE_OF_LATITUDE_NUM_SEGMENTS;

	/**
	 * The angular spacing a points along a line of longitude.
	 */
	const double LINE_OF_LONGITUDE_DELTA_LATITUDE = 180.0 / LINE_OF_LONGITUDE_NUM_SEGMENTS;


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


	void
	stream_background(
			stream_primitives_type &stream,
			const GPlatesGui::MapProjection &projection,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type::Primitives triangle_mesh(stream);

		const bool ok = triangle_mesh.begin_primitive(
				(LINE_OF_LONGITUDE_NUM_SEGMENTS + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1)/*max_num_vertices*/,
				3 * 2 * LINE_OF_LONGITUDE_NUM_SEGMENTS * LINE_OF_LATITUDE_NUM_SEGMENTS/*max_num_vertex_elements*/);

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);

		const double lat_0 = -90;
		const double lon_0 = projection.central_llp().longitude() - 180;

		// Project the vertices using the map projection.
		try
		{
			// Add the mesh vertices.
			for (int y = 0; y < LINE_OF_LONGITUDE_NUM_SEGMENTS + 1; ++y)
			{
				const GPlatesMaths::real_t lat = (y == LINE_OF_LONGITUDE_NUM_SEGMENTS)
						// At last row explicitly to avoid going slightly above
						// 'lat_0 + 180' due to numerical precision...
						? lat_0 + 180
						: lat_0 + y * LINE_OF_LONGITUDE_DELTA_LATITUDE;

				for (int x = 0; x < LINE_OF_LATITUDE_NUM_SEGMENTS + 1; ++x)
				{
					const GPlatesMaths::real_t lon = (x == LINE_OF_LATITUDE_NUM_SEGMENTS)
							// At last column explicitly to avoid going slightly above
							// 'lon_0 + 360' due to numerical precision...
							? lon_0 + 360
							: lon_0 + x * LINE_OF_LATITUDE_DELTA_LONGITUDE;

					const projection_coord_type projected_coord =
							project_lat_lon(lat.dval(), lon.dval(), projection);
					const vertex_type vertex(projected_coord.first, projected_coord.second, 0/*z*/, colour);

					triangle_mesh.add_vertex(vertex);
				}
			}
		}
		catch (const GPlatesGui::ProjectionException &exc)
		{
			// Ignore exception.
			qWarning() << exc;
		}

		// Add the mesh vertex elements.
		for (int y = 0; y < LINE_OF_LONGITUDE_NUM_SEGMENTS; ++y)
		{
			for (int x = 0; x < LINE_OF_LATITUDE_NUM_SEGMENTS; ++x)
			{
				// First triangle of current quad.
				triangle_mesh.add_vertex_element(y * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + x);
				triangle_mesh.add_vertex_element(y * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + (x + 1));
				triangle_mesh.add_vertex_element((y + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + x);

				// Second triangle of current quad.
				triangle_mesh.add_vertex_element((y + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + (x + 1));
				triangle_mesh.add_vertex_element((y + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + x);
				triangle_mesh.add_vertex_element(y * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + (x + 1));
			}
		}

		triangle_mesh.end_primitive();
	}


	GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
	compile_background_draw_state(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLVertexArray &vertex_array,
			const GPlatesGui::MapProjection &map_projection,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type stream;

		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		stream_primitives_type::StreamTarget stream_target(stream);
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		stream_background(stream, map_projection, colour);

		stream_target.stop_streaming();

#if 0 // Update: using 32-bit indices now...
		// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
		// Shouldn't get close really but check to be sure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);
#endif

		// Streamed triangle strips end up as indexed triangles.
		const GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type draw_vertex_array =
				compile_vertex_array_draw_state(
						renderer, vertex_array, vertices, vertex_elements, GL_TRIANGLES);

		// Start compiling draw state that includes alpha blend state and the vertex array draw command.
		GPlatesOpenGL::GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

		renderer.apply_compiled_draw_state(*draw_vertex_array);

		return compile_draw_state_scope.get_compiled_draw_state();
	}
}


GPlatesGui::MapBackground::MapBackground(
		GPlatesOpenGL::GLRenderer &renderer,
		const MapProjection &map_projection,
		const Colour &colour) :
	d_view_state(NULL),
	d_map_projection(map_projection),
	d_colour(colour),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer)),
	d_compiled_draw_state(
			compile_background_draw_state(
					renderer,
					*d_vertex_array,
					map_projection,
					Colour::to_rgba8(d_colour)))
{  }


GPlatesGui::MapBackground::MapBackground(
		GPlatesOpenGL::GLRenderer &renderer,
		const MapProjection &map_projection,
		const GPlatesPresentation::ViewState &view_state) :
	d_view_state(&view_state),
	d_map_projection(map_projection),
	d_colour(view_state.get_background_colour()),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer)),
	d_compiled_draw_state(
			compile_background_draw_state(
					renderer,
					*d_vertex_array,
					map_projection,
					Colour::to_rgba8(d_colour)))
{  }


void
GPlatesGui::MapBackground::paint(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Check whether we need to compile a new draw state.
	bool recompile_draw_state = false;

	const MapProjectionSettings map_projection_settings = d_map_projection.get_projection_settings();
	if (!d_last_seen_map_projection_settings ||
		d_last_seen_map_projection_settings.get() != map_projection_settings)
	{
		d_last_seen_map_projection_settings = map_projection_settings;
		recompile_draw_state = true;
	}

	// Check whether the view state's background colour has changed (if we're tracking it).
	if (d_view_state &&
		d_view_state->get_background_colour() != d_colour)
	{
		d_colour = d_view_state->get_background_colour();
		recompile_draw_state = true;
	}

	if (recompile_draw_state)
	{
		d_compiled_draw_state = compile_background_draw_state(
				renderer,
				*d_vertex_array,
				d_map_projection,
				Colour::to_rgba8(d_colour));
	}

	// Either render directly to the framebuffer, or render to a QImage and draw that to the
	// feedback paint device using a QPainter.
	//
	// NOTE: For feedback to a QPainter we render to an image instead of rendering vector geometries.
	// This is because, for SVG output, we don't want a large number of vector geometries due to this
	// map background - we really only want actual geological data and grid lines as SVG vector data.
	if (renderer.rendering_to_context_framebuffer())
	{
		renderer.apply_compiled_draw_state(*d_compiled_draw_state.get());
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

			// Clear the main framebuffer (colour and depth) before rendering the image.
			renderer.gl_clear_color();
			renderer.gl_clear_depth();
			renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Render the actual map background.
			renderer.apply_compiled_draw_state(*d_compiled_draw_state.get());
		}
		while (image_scope.end_render_tile());

		// Draw final raster QImage to feedback QPainter.
		image_scope.end_render();
	}
}
