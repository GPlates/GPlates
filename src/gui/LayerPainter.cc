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

#include "LayerPainter.h"

#include "opengl/GLRenderer.h"
#include "opengl/GLText.h"

#include "utils/Profile.h"


GPlatesGui::LayerPainter::LayerPainter(
		bool use_depth_buffer) :
	d_use_depth_buffer(use_depth_buffer)
{
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables::begin_painting()
{
	// Create the stream.
	d_stream.reset(new Stream());

	// The stream should target our internal vertices/indices.
	d_stream->stream_target.start_streaming(
			boost::in_place(boost::ref(d_vertices)),
			boost::in_place(boost::ref(d_vertex_elements)));
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::Drawables::end_painting(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
		GPlatesOpenGL::GLBuffer &vertex_buffer_data,
		GPlatesOpenGL::GLVertexArray &vertex_array,
		GLenum mode)
{
	// Stop targeting our internal vertices/indices.
	d_stream->stream_target.stop_streaming();

	// If there are primitives to draw...
	if (!d_vertex_elements.empty())
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

	// Destroy the stream.
	d_stream.reset();

	d_vertex_elements.clear();
	d_vertices.clear();
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::begin_painting()
{
	d_triangle_drawables.begin_painting();
}


void
GPlatesGui::LayerPainter::PointLinePolygonDrawables::end_painting(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
		GPlatesOpenGL::GLBuffer &vertex_buffer_data,
		GPlatesOpenGL::GLVertexArray &vertex_array,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	//
	// If any rendered polygons (or polylines) are 'filled' then render them first.
	// This way any vector geometry in this layer gets rendered on top and hence is visible.
	//
	// Filled polygons are rendered as rasters (textures) and hence the state set here
	// is similar (in fact identical) to the state set for rasters.
	//

	gl_visual_layers.render_filled_polygons(renderer, d_filled_polygons);
	// Now that the filled polygons have been rendered we should clear them for the next render call.
	d_filled_polygons.clear();

	// All painting below uses the one vertex array so we only need to bind it once (here).
	// Note that the filled polygons above uses it own vertex array(s).
	vertex_array.gl_bind(renderer);

	//
	// Paint the point, line and polygon drawables with the appropriate state
	// (such as point size, line width).
	//

	//
	// Paint the drawables representing all point primitives (if any).
	//

	// Iterate over the point size groups and paint them.
	BOOST_FOREACH(point_size_to_drawables_map_type::value_type &point_size_entry, d_point_drawables_map)
	{
		const float point_size = point_size_entry.first.dval();
		Drawables &points_drawable = point_size_entry.second;

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
	// Paint the drawables representing all line primitives (if any).
	//

	// Iterate over the line width groups and paint them.
	BOOST_FOREACH(line_width_to_drawables_map_type::value_type &line_width_entry, d_line_drawables_map)
	{
		const float line_width = line_width_entry.first.dval();
		Drawables &lines_drawable = line_width_entry.second;

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
}


GPlatesGui::LayerPainter::stream_primitives_type &
GPlatesGui::LayerPainter::PointLinePolygonDrawables::get_points_stream(
		float point_size)
{
	// Get the stream for points of the current point size.
	point_size_to_drawables_map_type::iterator stream_iter = d_point_drawables_map.find(point_size);
	if (stream_iter != d_point_drawables_map.end())
	{
		Drawables &drawable = stream_iter->second;
		return drawable.get_stream();
	}

	// A drawable does not yet exist for 'point_size' so create a new one.
	Drawables &drawable = d_point_drawables_map[point_size];

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
		Drawables &drawable = stream_iter->second;
		return drawable.get_stream();
	}

	// A drawable does not yet exist for 'line_width' so create a new one.
	Drawables &drawable = d_line_drawables_map[line_width];

	// Start a new stream on the drawable.
	drawable.begin_painting();

	return drawable.get_stream();
}


void
GPlatesGui::LayerPainter::begin_painting(
		GPlatesOpenGL::GLRenderer &renderer)
{
	d_renderer = renderer;

	// Create the vertex buffers and array if not already done so.
	// NOTE: These are only created *once* and re-used across paint calls.
	if (!d_vertex_element_buffer)
	{
		d_vertex_element_buffer = GPlatesOpenGL::GLVertexElementBuffer::create(
				renderer,
				GPlatesOpenGL::GLBuffer::create(renderer));
	}
	if (!d_vertex_buffer)
	{
		d_vertex_buffer = GPlatesOpenGL::GLVertexBuffer::create(
				renderer,
				GPlatesOpenGL::GLBuffer::create(renderer));
	}
	if (!d_vertex_array)
	{
		d_vertex_array = GPlatesOpenGL::GLVertexArray::create(renderer);

		// Attach vertex element buffer to the vertex array.
		d_vertex_array->set_vertex_element_buffer(renderer, d_vertex_element_buffer);

		// Attach vertex buffer to the vertex array.
		GPlatesOpenGL::bind_vertex_buffer_to_vertex_array<coloured_vertex_type>(
				renderer, *d_vertex_array, d_vertex_buffer);
	}

	drawables_off_the_sphere.begin_painting();
	opaque_drawables_on_the_sphere.begin_painting();
	translucent_drawables_on_the_sphere.begin_painting();

	current_cube_quad_tree_location = boost::none;
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::end_painting(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
		const TextRenderer &text_renderer,
		float scale)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

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


	// Set the alpha-blend state in case filled polygons are semi-transparent.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

	// Turn on depth testing if the client has requested to use the depth buffer.
	renderer.gl_enable(GL_DEPTH_TEST, d_use_depth_buffer);

	// Turn off depth writes.
	renderer.gl_depth_mask(GL_FALSE);

	// Paint a raster if there is one (note there should only be one raster in a layer).
	const cache_handle_type rasters_cache_handle = paint_rasters(renderer, gl_visual_layers);

	{
		// Make sure we leave the OpenGL state the way it was.
		GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_drawables_off_sphere_state(renderer);

		// Turn on depth writes.
		// Drawables *off* the sphere is the only case where depth writes are enabled.
		// Only enable depth writes if client requested to use the depth buffer.
		if (d_use_depth_buffer)
		{
			renderer.gl_depth_mask(GL_TRUE);
		}

		// As mentioned above these off-sphere primitives should not be rendered with any anti-aliasing
		// (including polygon anti-aliasing - which we no longer use due to artifacts at triangle edges).
		renderer.gl_enable(GL_POINT_SMOOTH, false);
		renderer.gl_enable(GL_LINE_SMOOTH, false);

		drawables_off_the_sphere.end_painting(
				renderer,
				*d_vertex_element_buffer->get_buffer(),
				*d_vertex_buffer->get_buffer(),
				*d_vertex_array,
				gl_visual_layers);
	}

	// Even though these primitives are opaque they are still rendered with polygon anti-aliasing
	// which relies on alpha-blending (so we don't disable it here).
	opaque_drawables_on_the_sphere.end_painting(
			renderer,
			*d_vertex_element_buffer->get_buffer(),
			*d_vertex_buffer->get_buffer(),
			*d_vertex_array,
			gl_visual_layers);

	translucent_drawables_on_the_sphere.end_painting(
			renderer,
			*d_vertex_element_buffer->get_buffer(),
			*d_vertex_buffer->get_buffer(),
			*d_vertex_array,
			gl_visual_layers);

	// Render any 2D text last (text specified at 2D viewport positions).
	paint_text_drawables_2D(renderer, text_renderer, scale);

	// Render any 3D text last (text specified at 3D world positions).
	// This is because the text is converted from 3D space to 2D window coordinates and hence is
	// effectively *off* the sphere (in the 3D globe view) but it can't have depth writes enabled
	// (because we don't know the depth since its rendered as 2D).
	// We add it last so it gets drawn last for this layer which should put it on top.
	// However if another rendered layer is drawn after this one then the text will be overwritten
	// and not appear to hover in 3D space - currently it looks like the only layer that uses
	// text is the Measure Distance tool layer and that's the last layer.
	// Also it depends on how the text is meant to interact with other *off* the sphere geometries
	// such as rendered arrows (should it be on top or interleave depending on depth).
	// FIXME: We might be able to draw text as 3D and turn depth writes on (however the
	// alpha-blending could cause some visual artifacts as described above).
	paint_text_drawables_3D(renderer, text_renderer, scale);

	// Only used for the duration of painting.
	d_renderer = boost::none;

	return rasters_cache_handle;
}


GPlatesGui::LayerPainter::cache_handle_type
GPlatesGui::LayerPainter::paint_rasters(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLVisualLayers &gl_visual_layers)
{
	// The cached view is a sequence of raster caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type> > cache_handle(
			new std::vector<GPlatesOpenGL::GLVisualLayers::cache_handle_type>());
	cache_handle->reserve(rasters.size());

	BOOST_FOREACH(const RasterDrawable &raster_drawable, rasters)
	{
		// We don't want to rebuild the OpenGL structures that render the raster each frame
		// so those structures need to persist from one render to the next.
		const cache_handle_type raster_cache_handle = gl_visual_layers.render_raster(
				renderer,
				raster_drawable.source_resolved_raster,
				raster_drawable.source_raster_colour_palette,
				raster_drawable.source_raster_modulate_colour,
				raster_drawable.map_projection);

		cache_handle->push_back(raster_cache_handle);
	}

	// Now that the rasters have been rendered we should clear the drawables list for the next render call.
	rasters.clear();

	return cache_handle;
}


void
GPlatesGui::LayerPainter::paint_text_drawables_2D(
		GPlatesOpenGL::GLRenderer &renderer,
		const TextRenderer &text_renderer,
		float scale)
{
	BOOST_FOREACH(const TextDrawable2D &text_drawable, text_drawables_2D)
	{
		// render drop shadow, if any
		if (text_drawable.shadow_colour)
		{
			GPlatesOpenGL::GLText::render_text_2D(
					renderer,
					text_renderer,
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
					text_renderer,
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
		const TextRenderer &text_renderer,
		float scale)
{
	BOOST_FOREACH(const TextDrawable3D &text_drawable, text_drawables_3D)
	{
		// render drop shadow, if any
		if (text_drawable.shadow_colour)
		{
			GPlatesOpenGL::GLText::render_text_3D(
					renderer,
					text_renderer,
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
					text_renderer,
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
