/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The Geological Survey of Norway
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


#include <limits>
#include <stack>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/type_traits/is_same.hpp>

#include <QDebug>
#include <QTransform>

#include "Colour.h"
#include "ColourScheme.h"
#include "MapRenderedGeometryLayerPainter.h"
#include "MapProjection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/EllipseGenerator.h"
#include "maths/GreatCircle.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineIntersections.h"
#include "maths/PolylineOnSphere.h"

#include "opengl/GLRenderer.h"

#include "view-operations/RenderedArrowedPolyline.h"
#include "view-operations/RenderedCircleSymbol.h"
#include "view-operations/RenderedColouredEdgeSurfaceMesh.h"
#include "view-operations/RenderedColouredTriangleSurfaceMesh.h"
#include "view-operations/RenderedCrossSymbol.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedRadialArrow.h"
#include "view-operations/RenderedResolvedRaster.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"
#include "view-operations/RenderedSquareSymbol.h"
#include "view-operations/RenderedString.h"
#include "view-operations/RenderedTangentialArrow.h"
#include "view-operations/RenderedTriangleSymbol.h"

// Temporary includes for symbol testing
#include "view-operations/RenderedGeometryFactory.h"


namespace
{
	/**
	 * We will tessellate a great circle arc if the two endpoints are far enough apart.
	 */
	const double GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD = GPlatesMaths::convert_deg_to_rad(1);
	const double COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD = std::cos(GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

	/**
	 * We will tessellate a small circle (arc) to this angular resolution.
	 */
	const double SMALL_CIRCLE_ANGULAR_INCREMENT = GPlatesMaths::convert_deg_to_rad(1);

	/**
	 * We will tessellate ellipses to this angular resolution (angle between semi-major and semi-minor axes).
	 */
	const double ELLIPSE_ANGULAR_INCREMENT = GPlatesMaths::convert_deg_to_rad(1);

	/**
	 * Make sure the longitude is within [-180+EPSILON, 180-EPSILON] around the central meridian longitude.
	 * 
	 * This is to prevent subsequent map projection from wrapping (-180 -> +180 or vice versa) due to
	 * the map projection code receiving a longitude value slightly outside that range or the map
	 * projection code itself having numerical precision issues.
	 *
	 * NOTE: Doesn't need to be too accurate since this is for visual display only.
	 */
	const double LONGITUDE_RANGE_EPSILON = 1e-6;

	//! Longitude range lower limit.
	const double LONGITUDE_RANGE_LOWER_LIMIT = -180 + LONGITUDE_RANGE_EPSILON;

	//! Longitude range upper limit.
	const double LONGITUDE_RANGE_UPPER_LIMIT = 180 - LONGITUDE_RANGE_EPSILON;

	const double TWO_PI = 2. * GPlatesMaths::PI;

	// Variables for drawing velocity arrows.
	const float GLOBE_TO_MAP_SCALE_FACTOR = 180.;
	const float MAP_VELOCITY_SCALE_FACTOR = 3.0;
	const double ARROWHEAD_BASE_HEIGHT_RATIO = 0.5;

	// Scale factor for symbols.
	const double SYMBOL_SCALE_FACTOR = 1.8;

	/*!
	 * Correction factor for size of filled circle symbol, which uses the standard point rendering,
	 * and which therefore would appear considerably smaller than other symbol types.
	 *
	 * This correction factor brings it in line with the size of the unfilled circle symbol.
	 */
	const double FILLED_CIRCLE_SYMBOL_CORRECTION = 5.;

	void
	display_vertex(
		const GPlatesMaths::PointOnSphere &point)
	{
		qDebug() << "Vertex: " << point.position_vector();
	}

	void
	display_vertex(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesGui::MapProjection &projection)
	{
		double lat;
		double lon;
		projection.forward_transform(point, lon, lat);

		qDebug() << "Vertex: " << point.position_vector();
		qDebug() << QPointF(lon, lat);
		qDebug();
	}

	/*!
	 * \brief tessellate_on_plane fills @a seq with vertices describing a circle on
	 * a plane. The third component of each vertex will be zero.
	 *
	 * \param seq - vector of vertices to be filled
	 * \param centre - centre of circle
	 * \param radius - radius of circle
	 * \param angular_increment - increment between successive vertices.
	 * \param colour - colour applied to all vertices.
	 */
	void tessellate_on_plane(
			GPlatesGui::LayerPainter::coloured_vertex_seq_type &seq,
			const QPointF &centre,
			const double &radius,
			const double &angular_increment,
			const GPlatesGui::rgba8_t &colour)
	{
		// Determine number of increments
		const int num_segments = 1 + static_cast<int>(TWO_PI / SMALL_CIRCLE_ANGULAR_INCREMENT);

		// Set up a rotation about the circle centre. First we translate
		// the point so that its coordinates are relative to the circle centre, then rotate
		// about the origin, then translate back.
		//
		// In the concatenated form below the transforms are applied in reverse order
		//
		QTransform rotation;
		rotation.translate(centre.x(),centre.y()).
				rotateRadians(-SMALL_CIRCLE_ANGULAR_INCREMENT).
				translate(-centre.x(),-centre.y());

		// Set up initial point on circumference of circle. We can pick any point - might
		// as well go "north" from the centre.
		QPointF point = centre + QPointF(0,radius);
		GPlatesGui::LayerPainter::coloured_vertex_type initial_vertex(
					point.x(), point.y(), 0, colour);
		seq.push_back(initial_vertex);

		for (int n = 0; n < num_segments ; ++n)
		{
			point = point*rotation;
			GPlatesGui::LayerPainter::coloured_vertex_type vertex(
						point.x(), point.y(), 0, colour);
			seq.push_back(vertex);
		}

	}
}


const float GPlatesGui::MapRenderedGeometryLayerPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::MapRenderedGeometryLayerPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;


GPlatesGui::MapRenderedGeometryLayerPainter::MapRenderedGeometryLayerPainter(
		const MapProjection::non_null_ptr_to_const_type &map_projection,
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		const double &inverse_viewport_zoom_factor,
		const RenderSettings &render_settings,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_map_projection(map_projection),
	d_rendered_geometry_layer(rendered_geometry_layer),
	d_gl_visual_layers(gl_visual_layers),
	d_inverse_zoom_factor(inverse_viewport_zoom_factor),
	d_render_settings(render_settings),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f),
	d_dateline_wrapper(
			GPlatesMaths::DateLineWrapper::create(
					// Move the dateline wrapping to be [-180 + central_meridian, central_meridian + 180]...
					map_projection->central_llp().longitude()))
{
}


GPlatesGui::MapRenderedGeometryLayerPainter::cache_handle_type
GPlatesGui::MapRenderedGeometryLayerPainter::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		LayerPainter &layer_painter)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// We have a layer painter for the duration of this method.
	d_layer_painter = layer_painter;

	// Begin painting so our visit methods can start painting.
	layer_painter.begin_painting(renderer);

	// Visit the rendered geometries in the rendered layer.
	//
	// NOTE: Rasters get painted as they are visited - it's really mainly the point/line/polygon
	// primitives that get batched up into vertex streams for efficient rendering.
	visit_rendered_geometries(renderer);

	// Do the actual painting.
	const cache_handle_type layer_cache = layer_painter.end_painting(renderer, d_scale);

	// We no longer have a layer painter.
	d_layer_painter = boost::none;

	return layer_cache;
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_multi_point_on_sphere(
			const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)	
{
	if (!d_render_settings.show_multipoints())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_multi_point_on_sphere);
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(*colour);

	const float point_size =
			rendered_multi_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT * d_scale;

	// Get the stream for points of the current point size.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

	// Used to add points to the stream.
	stream_primitives_type::Points stream_points(stream);

	stream_points.begin_points();

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
			rendered_multi_point_on_sphere.get_multi_point_on_sphere();

	GPlatesMaths::MultiPointOnSphere::const_iterator point_iter = multi_point_on_sphere->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator point_end = multi_point_on_sphere->end();
	for ( ; point_iter != point_end; ++point_iter)
	{
		// Get the projected point position.
		const QPointF proj_pos = get_projected_unwrapped_position(*point_iter);

		// Vertex representing the projected point's position and colour.
		const coloured_vertex_type vertex(proj_pos.x(), proj_pos.y(), 0/*z*/, rgba8_color);

		stream_points.add_vertex(vertex);
	}

	stream_points.end_points();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_point_on_sphere(
			const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
{
	if (!d_render_settings.show_points())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_point_on_sphere);
	if (!colour)
	{
		return;
	}

#if 0
	//////////////////////////////////////////////////////////////////////////////////////
	// Force symbol rendering for testing. This lets me easily create symbols via the
	// digitisation tool, or by loading up point files.
//	GPlatesViewOperations::RenderedGeometry test_symbol =
//		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_square_symbol(
//			rendered_point_on_sphere.get_point_on_sphere(),
//			rendered_point_on_sphere.get_colour(),
//			1,
//			true);

	GPlatesViewOperations::RenderedGeometry test_symbol =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_circle_symbol(
			rendered_point_on_sphere.get_point_on_sphere(),
			rendered_point_on_sphere.get_colour(),
			1,
			false);

	test_symbol.accept_visitor(*this);

	return;
	// End of testing code.
	/////////////////////////////////////////////////////////////////////////////////////
#endif

	const float point_size =
			rendered_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT * d_scale;

	// Get the stream for points of the current point size.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

	// Get the projected point position.
	const QPointF proj_pos = get_projected_unwrapped_position(rendered_point_on_sphere.get_point_on_sphere());

	// Vertex representing the projected point's position and colour.
	// Convert colour from floats to bytes to use less vertex memory.
	const coloured_vertex_type vertex(proj_pos.x(), proj_pos.y(), 0/*z*/, Colour::to_rgba8(*colour));

	// Used to add points to the stream.
	stream_primitives_type::Points stream_points(stream);

	stream_points.begin_points();
	stream_points.add_vertex(vertex);
	stream_points.end_points();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_polygon_on_sphere(
			const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere)
{
	if (!d_render_settings.show_polygons())
	{
		return;
	}
	
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_polygon_on_sphere);
	if (!colour)
	{
		return;
	}

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
			rendered_polygon_on_sphere.get_polygon_on_sphere();

	if (rendered_polygon_on_sphere.get_is_filled())
	{
		GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons =
				d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons_map_view();

		// Modulate with the fill modulate colour.
		const Colour fill_colour = Colour::modulate(
				colour.get(),
				rendered_polygon_on_sphere.get_fill_modulate_colour());

		// Dateline wrap and project the polygon and render each wrapped polygon as a filled polygon.
		paint_fill_geometry<GPlatesMaths::PolygonOnSphere>(
				filled_polygons,
				polygon_on_sphere,
				fill_colour);

		return;
	}

	const float line_width =
			rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_line_geometry<GPlatesMaths::PolygonOnSphere>(polygon_on_sphere, colour.get(), stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_polyline_on_sphere(
			const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
{
	if (!d_render_settings.show_lines())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_polyline_on_sphere);
	if (!colour)
	{
		return;
	}

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_polyline_on_sphere.get_polyline_on_sphere();

	if (rendered_polyline_on_sphere.get_is_filled())
	{
		GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons =
				d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons_map_view();

		// Modulate with the fill modulate colour.
		const Colour fill_colour = Colour::modulate(
				colour.get(),
				rendered_polyline_on_sphere.get_fill_modulate_colour());

		// Dateline wrap and project the polygon and render each wrapped polygon as a filled polygon.
		paint_fill_geometry<GPlatesMaths::PolylineOnSphere>(
				filled_polygons,
				polyline_on_sphere,
				fill_colour);

		return;
	}

	const float line_width =
			rendered_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(polyline_on_sphere, colour.get(), stream);
}



void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_coloured_edge_surface_mesh(
	const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh &rendered_coloured_edge_surface_mesh)
{
	const float line_width =
			rendered_coloured_edge_surface_mesh.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	typedef GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh mesh_type;

	const mesh_type::edge_seq_type &mesh_edges =
			rendered_coloured_edge_surface_mesh.get_mesh_edges();
	const mesh_type::vertex_seq_type &mesh_vertices =
			rendered_coloured_edge_surface_mesh.get_mesh_vertices();

	// Iterate over the mesh edges.
	mesh_type::edge_seq_type::const_iterator mesh_edges_iter = mesh_edges.begin();
	mesh_type::edge_seq_type::const_iterator mesh_edges_end = mesh_edges.end();
	for ( ; mesh_edges_iter != mesh_edges_end; ++mesh_edges_iter)
	{
		const mesh_type::Edge &mesh_edge = *mesh_edges_iter;

		boost::optional<Colour> colour = mesh_edge.colour.get_colour(d_colour_scheme);
		if (!colour)
		{
			continue;
		}

		// Create a polyline with two points for the current edge.
		const GPlatesMaths::PointOnSphere edge_points[2] =
		{
			mesh_vertices[mesh_edge.vertex_indices[0]],
			mesh_vertices[mesh_edge.vertex_indices[1]]
		};
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type edge =
				GPlatesMaths::PolylineOnSphere::create_on_heap(
						edge_points,
						edge_points + 2);

		// Paint the current single great circle arc edge (it might get dateline wrapped and
		// tessellated into smaller arcs).
		paint_line_geometry<GPlatesMaths::PolylineOnSphere>(edge, colour.get(), stream);
	}
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_coloured_triangle_surface_mesh(
		const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh &rendered_coloured_triangle_surface_mesh)
{
	GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons =
			d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons_map_view();

	filled_polygons.begin_filled_triangle_mesh();

	typedef GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh mesh_type;

	const mesh_type::triangle_seq_type &mesh_triangles =
			rendered_coloured_triangle_surface_mesh.get_mesh_triangles();
	const mesh_type::vertex_seq_type &mesh_vertices =
			rendered_coloured_triangle_surface_mesh.get_mesh_vertices();

	// Iterate over the mesh triangles.
	mesh_type::triangle_seq_type::const_iterator mesh_triangles_iter = mesh_triangles.begin();
	mesh_type::triangle_seq_type::const_iterator mesh_triangles_end = mesh_triangles.end();
	for ( ; mesh_triangles_iter != mesh_triangles_end; ++mesh_triangles_iter)
	{
		const mesh_type::Triangle &mesh_triangle = *mesh_triangles_iter;

		boost::optional<Colour> colour = mesh_triangle.colour.get_colour(d_colour_scheme);
		if (!colour)
		{
			continue;
		}

		// Create a PolygonOnSphere for the current triangle so we can pass it through the
		// dateline wrapping and projection code.
		const GPlatesMaths::PointOnSphere triangle_points[3] =
		{
			mesh_vertices[mesh_triangle.vertex_indices[0]],
			mesh_vertices[mesh_triangle.vertex_indices[1]],
			mesh_vertices[mesh_triangle.vertex_indices[2]]
		};
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type triangle_polygon =
				GPlatesMaths::PolygonOnSphere::create_on_heap(
						triangle_points,
						triangle_points + 3);

		DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_triangle;
		dateline_wrap_and_project_line_geometry<GPlatesMaths::PolygonOnSphere>(
				dateline_wrapped_projected_triangle,
				triangle_polygon);

		const std::vector<unsigned int> &geometries =
				dateline_wrapped_projected_triangle.get_geometries();
		const unsigned int num_geometries = geometries.size();
		if (num_geometries == 0)
		{
			// Continue to the next triangle if there's nothing to paint - shouldn't really be able to get here.
			continue;
		}

		unsigned int great_circle_arc_index = 0;
		const std::vector<unsigned int> &great_circle_arcs =
				dateline_wrapped_projected_triangle.get_great_circle_arcs();

		unsigned int projected_vertex_index = 0;
		const std::vector<QPointF> &projected_vertices =
				dateline_wrapped_projected_triangle.get_vertices();

		// Iterate over the dateline wrapped geometries.
		for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
		{
			std::vector<QPointF> filled_triangle_geometry;

			// Note that the dateline wrapper ensures the first great circle arc of each
			// wrapped geometry contains at least two vertices.

			// Iterate over the great circle arcs of the current geometry.
			for ( ; great_circle_arc_index < geometries[geometry_index]; ++great_circle_arc_index)
			{
				// Iterate over the vertices of the current great circle arc.
				for ( ; projected_vertex_index < great_circle_arcs[great_circle_arc_index]; ++projected_vertex_index)
				{
					filled_triangle_geometry.push_back(projected_vertices[projected_vertex_index]);
				}
			}

			// If the dateline wrapped geometry remains a triangle (ie, same as before dateline wrapping)
			// then add it to the current triangle mesh drawable since it results in faster rendering.
			// Otherwise start a new polygon drawable (since it's not a triangle and could be concave).
			// We test for 4 vertices instead of 3 for a triangle because the dateline wrapping code
			// iterates over great circle arcs and outputs a vertex at beginning of first GCA and
			// also at end of last GCA (which is a duplicate vertex for a closed polygon).
			if (filled_triangle_geometry.size() == 4)
			{
				filled_polygons.add_filled_triangle_to_mesh(
						filled_triangle_geometry[0],
						filled_triangle_geometry[1],
						filled_triangle_geometry[2],
						colour.get());
			}
			else
			{
				// End the current mesh drawable.
				filled_polygons.end_filled_triangle_mesh();

				// Add the filled polygon geometry.
				filled_polygons.add_filled_polygon(filled_triangle_geometry, colour.get());

				// Start a new mesh drawable.
				filled_polygons.begin_filled_triangle_mesh();
			}
		}
	}

	// End the current filled mesh.
	filled_polygons.end_filled_triangle_mesh();
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_resolved_raster(
		const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster)
{
	// Queue the raster primitive for painting.
	d_layer_painter->rasters.push_back(
			LayerPainter::RasterDrawable(
					rendered_resolved_raster.get_resolved_raster(),
					rendered_resolved_raster.get_raster_colour_palette(),
					rendered_resolved_raster.get_raster_modulate_colour(),
					rendered_resolved_raster.get_normal_map_height_field_scale_factor()));
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (!d_render_settings.show_strings())
	{
		return;
	}

	// Get the projected text position.
	const QPointF proj_pos = get_projected_unwrapped_position(rendered_string.get_point_on_sphere());

	d_layer_painter->text_drawables_2D.push_back(
			LayerPainter::TextDrawable2D(
					rendered_string.get_string(),
					rendered_string.get_font(),
					proj_pos.x(),
					proj_pos.y(),
					rendered_string.get_x_offset(),
					rendered_string.get_y_offset(),
					get_colour_of_rendered_geometry(rendered_string),
					rendered_string.get_shadow_colour().get_colour(d_colour_scheme)));
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle);
	if (!colour)
	{
		return;
	}

	// Tessellate the small circle.
	std::vector<GPlatesMaths::PointOnSphere> points;
	tessellate(points, rendered_small_circle.get_small_circle(), SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Create a closed polyline loop from the tessellated points.
	points.push_back(points.front());
	// NOTE: We don't create a polygon because if a polygon crosses the central meridian it gets
	// rendered as multiple polygons and for a small circle this could look confusing.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type small_circle_arc_polyline =
			GPlatesMaths::PolylineOnSphere::create_on_heap(points);

	const float line_width = rendered_small_circle.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Render the small circle tessellated as a closed polyline.
	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(small_circle_arc_polyline, colour.get(), lines_stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_small_circle_arc(
		const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle_arc);
	if (!colour)
	{
		return;
	}

	// Tessellate the small circle arc.
	std::vector<GPlatesMaths::PointOnSphere> points;
	tessellate(points, rendered_small_circle_arc.get_small_circle_arc(), SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Create a polyline from the tessellated points.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type small_circle_arc_polyline =
			GPlatesMaths::PolylineOnSphere::create_on_heap(points);

	const float line_width = rendered_small_circle_arc.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Render the small circle arc tessellated as a polyline.
	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(small_circle_arc_polyline, colour.get(), lines_stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_ellipse);
	if (!colour)
	{
		return;
	}

	if (rendered_ellipse.get_semi_major_axis_radians() == 0 ||
		rendered_ellipse.get_semi_minor_axis_radians() == 0)
	{
		return;
	}

	// See comments in the GlobeRenderedGeometryLayerPainter for possibilities
	// of making the number of steps zoom-dependent.

	GPlatesMaths::EllipseGenerator ellipse_generator(
			rendered_ellipse.get_centre(),
			rendered_ellipse.get_semi_major_axis_radians(),
			rendered_ellipse.get_semi_minor_axis_radians(),
			rendered_ellipse.get_axis());
		
	// Tessellate the ellipse into a sequence of points.
	std::vector<GPlatesMaths::PointOnSphere> points;
	for (double angle = 0; angle < TWO_PI; angle += ELLIPSE_ANGULAR_INCREMENT)
	{
		const GPlatesMaths::PointOnSphere point(
				ellipse_generator.get_point_on_ellipse(angle));

		points.push_back(point);
	}

	// Create a closed polyline loop from the tessellated points.
	points.push_back(points.front());
	// NOTE: We don't create a polygon because if a polygon crosses the central meridian it gets
	// rendered as multiple polygons and for an ellipse this could look confusing.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type ellipse_polyline =
			GPlatesMaths::PolylineOnSphere::create_on_heap(points);

	const float line_width = rendered_ellipse.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Render the ellipse tessellated as a closed polyline.
	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(ellipse_polyline, colour.get(), lines_stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_radial_arrow(
		const GPlatesViewOperations::RenderedRadialArrow &rendered_radial_arrow)
{
	// We don't render the radial arrow in the map view (it's radial and hence always pointing
	// directly out of the map). We only render the symbol.

    boost::optional<Colour> symbol_colour = rendered_radial_arrow.get_symbol_colour().get_colour(d_colour_scheme);
    if (!symbol_colour)
    {
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_symbol_colour = Colour::to_rgba8(*symbol_colour);

	// Get the small circle position in scene coordinates.
	const QPointF small_circle_centre = get_projected_unwrapped_position(rendered_radial_arrow.get_position());

	// The symbol is a small circle with diameter equal to the symbol size.
	// The symbol size is specified in *scene* coordinates.
	const double small_circle_radius = 0.5 * rendered_radial_arrow.get_symbol_size() * d_inverse_zoom_factor;

	// Tessellate the circle on the plane of the map.
	coloured_vertex_seq_type small_circle_vertices;
	tessellate_on_plane(
			small_circle_vertices,
			small_circle_centre,
			small_circle_radius,
			SMALL_CIRCLE_ANGULAR_INCREMENT,
			rgba8_symbol_colour);

	// Draw the small circle outline.
	// We do this even if we're filling the small circle because it gives a nice soft anti-aliased edge.

	// The factor of 2 gives a nice look.
	const float small_circle_line_width = 2.0f * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for the small circle lines.
	stream_primitives_type &small_circle_line_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(small_circle_line_width);

	// Used to add a line loop to the stream.
	stream_primitives_type::LineLoops stream_small_circle_line_loops(small_circle_line_stream);
	stream_small_circle_line_loops.begin_line_loop();
	BOOST_FOREACH(const coloured_vertex_type &small_circle_vertex, small_circle_vertices)
	{
		stream_small_circle_line_loops.add_vertex(small_circle_vertex);
	}
	stream_small_circle_line_loops.end_line_loop();

	// Draw the filled small circle.
	if (rendered_radial_arrow.get_symbol_type() == GPlatesViewOperations::RenderedRadialArrow::SYMBOL_FILLED_CIRCLE)
	{
		stream_primitives_type &triangle_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_triangles_stream();

		stream_primitives_type::TriangleFans stream_triangle_fans(triangle_stream);

		stream_triangle_fans.begin_triangle_fan();

		// Add centre of small circle (apex of triangle fan).
		stream_triangle_fans.add_vertex(
					coloured_vertex_type(small_circle_centre.x(), small_circle_centre.y(), 0, rgba8_symbol_colour));

		// Add small circle points.
		BOOST_FOREACH(const coloured_vertex_type &small_circle_vertex, small_circle_vertices)
		{
			stream_small_circle_line_loops.add_vertex(small_circle_vertex);
		}

		stream_triangle_fans.end_triangle_fan();
	}

	// Draw the small circle centre point.
	if (rendered_radial_arrow.get_symbol_type() == GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_POINT)
	{
		// The factor of 2 gives a nice look.
		const float point_size = 2.0f * POINT_SIZE_ADJUSTMENT * d_scale;
		stream_primitives_type &point_stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

		stream_primitives_type::Points stream_points(point_stream);
		stream_points.begin_points();
		stream_points.add_vertex(
				coloured_vertex_type(small_circle_centre.x(), small_circle_centre.y(), 0, rgba8_symbol_colour));
		stream_points.end_points();
	}

	// Draw a cross in the small circle.
	if (rendered_radial_arrow.get_symbol_type() == GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS)
	{
		// The factor of 1.5 ensures the cross is not too fat.
		const float cross_line_width = 1.5f * LINE_WIDTH_ADJUSTMENT * d_scale;

		// Get the stream for the cross lines.
		stream_primitives_type &cross_line_stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(cross_line_width);

		stream_primitives_type::LineStrips stream_cross_line_strips(cross_line_stream);

		stream_cross_line_strips.begin_line_strip();
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(small_circle_centre.x() - small_circle_radius, small_circle_centre.y(), 0, rgba8_symbol_colour));
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(small_circle_centre.x() + small_circle_radius, small_circle_centre.y(), 0, rgba8_symbol_colour));
		stream_cross_line_strips.end_line_strip();

		stream_cross_line_strips.begin_line_strip();
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(small_circle_centre.x(), small_circle_centre.y() - small_circle_radius, 0, rgba8_symbol_colour));
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(small_circle_centre.x(), small_circle_centre.y() + small_circle_radius, 0, rgba8_symbol_colour));
		stream_cross_line_strips.end_line_strip();
	}
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_tangential_arrow(
	const GPlatesViewOperations::RenderedTangentialArrow &rendered_tangential_arrow)
{
	if (!d_render_settings.show_arrows())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_tangential_arrow);
	if (!colour)
	{
		return;
	}

	// Start of arrow.
	const GPlatesMaths::UnitVector3D &start = rendered_tangential_arrow.get_start_position().position_vector();

	// Calculate position from start point along tangent direction to
	// end point off the globe. The length of the arrow in world space
	// is inversely proportional to the zoom or magnification.
	const GPlatesMaths::Vector3D end = GPlatesMaths::Vector3D(start) +
			MAP_VELOCITY_SCALE_FACTOR * d_inverse_zoom_factor * rendered_tangential_arrow.get_arrow_direction();

	const GPlatesMaths::Vector3D arrowline = end - GPlatesMaths::Vector3D(start);
	const GPlatesMaths::real_t arrowline_length = arrowline.magnitude();

	// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
	if (arrowline_length == 0)
	{
		return;
	}

	double arrowhead_size =
			d_inverse_zoom_factor * rendered_tangential_arrow.get_arrowhead_projected_size();
	const float max_ratio_arrowhead_to_arrowline_length =
			rendered_tangential_arrow.get_max_ratio_arrowhead_to_arrowline_length();

	// We want to keep the projected arrowhead size constant regardless of the
	// the length of the arrowline, except...
	//
	// ...if the ratio of arrowhead size to arrowline length is large enough then
	// we need to start scaling the arrowhead size by the arrowline length so
	// that the arrowhead disappears as the arrowline disappears.
	if (arrowhead_size > max_ratio_arrowhead_to_arrowline_length * arrowline_length.dval())
	{
		arrowhead_size = max_ratio_arrowhead_to_arrowline_length * arrowline_length.dval();
	}
	// Adjust the arrow head size for the map view.
	arrowhead_size *= GLOBE_TO_MAP_SCALE_FACTOR;

	// Get the drawables for lines of the current line width.
	const float line_width = rendered_tangential_arrow.get_map_view_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;
	stream_primitives_type &line_stream =
			d_layer_painter->drawables_off_the_sphere.get_lines_stream(line_width);

	// Render a single line arc for the arrow body.
	//
	// By rendering as a great circle arc the path will not necessarily be a straight line after
	// map projection which will visually show the path taken that is equivalent to a straight-line
	// (great circle arc) tangent on the 3D globe.
	const GPlatesMaths::PointOnSphere arrow_end_points[2] =
	{
		GPlatesMaths::PointOnSphere(start),
		GPlatesMaths::PointOnSphere(end.get_normalisation())
	};
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type arrow =
			GPlatesMaths::PolylineOnSphere::create_on_heap(arrow_end_points, arrow_end_points + 2);

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(arrow, colour.get(), line_stream, arrowhead_size);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_arrowed_polyline(
	const GPlatesViewOperations::RenderedArrowedPolyline &rendered_arrowed_polyline)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_arrowed_polyline);
	if (!colour)
	{
		return;
	}

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			rendered_arrowed_polyline.get_polyline_on_sphere();

	double arrowhead_size = d_inverse_zoom_factor * rendered_arrowed_polyline.get_arrowhead_projected_size();
	if (arrowhead_size > rendered_arrowed_polyline.get_max_arrowhead_size())
	{
		arrowhead_size = rendered_arrowed_polyline.get_max_arrowhead_size();
	}
	// Adjust the arrow head size for the map view.
	arrowhead_size *= GLOBE_TO_MAP_SCALE_FACTOR;

	const float line_width = rendered_arrowed_polyline.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(polyline, colour.get(), lines_stream, arrowhead_size);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_triangle_symbol(
	const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_triangle_symbol);
	if (!colour)
	{
		return;
	}

	bool filled = rendered_triangle_symbol.get_is_filled();

	double size = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_triangle_symbol.get_size();

	// r is radius of circumscribing circle. Factor 1.33 used here to give us a triangle height of 2*d.
	double r = 1.333 * size;

	// Get the point position, and project it to the canvas coordinate system.
	const GPlatesMaths::PointOnSphere &pos =
			rendered_triangle_symbol.get_centre();
	// Point pcentre is the centre of our triangle.
	QPointF pcentre = get_projected_unwrapped_position(pos);

	// pa, pb and pc are the vertices of the triangle.
	// pa is the vertex which points "up"; pb is lower right and pc lower left.

	// FIXME: we can skip the QPointFs here and set up the coords directly in the
	// coloured_vertex_type instantiations below.
	QPointF pa(pcentre.x(),pcentre.y()+r);
	QPointF pb(pcentre.x()-0.86*r,pcentre.y()-0.5*r);
	QPointF pc(pcentre.x()+0.86*r,pcentre.y()-0.5*r);

	coloured_vertex_type va(pa.x(), pa.y(), 0,Colour::to_rgba8(*colour));
	coloured_vertex_type vb(pb.x(), pb.y(), 0,Colour::to_rgba8(*colour));
	coloured_vertex_type vc(pc.x(), pc.y(), 0,Colour::to_rgba8(*colour));

	if (filled)
	{
		stream_primitives_type &stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_triangles_stream();

		stream_primitives_type::Triangles stream_triangles(stream);

		// The polygon state is fill, front/back by default, so I shouldn't need
		// to change anything here.

		stream_triangles.begin_triangles();
		stream_triangles.add_vertex(va);
		stream_triangles.add_vertex(vb);
		stream_triangles.add_vertex(vc);
		stream_triangles.end_triangles();
	}
	else
	{
		const float line_width = rendered_triangle_symbol.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

		stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

		stream_primitives_type::LineStrips stream_line_strips(stream);

		stream_line_strips.begin_line_strip();
		stream_line_strips.add_vertex(va);
		stream_line_strips.add_vertex(vb);
		stream_line_strips.add_vertex(vc);
		stream_line_strips.add_vertex(va);
		stream_line_strips.end_line_strip();
	}

}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_square_symbol(
	const GPlatesViewOperations::RenderedSquareSymbol &rendered_square_symbol)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_square_symbol);
	if (!colour)
	{
		return;
	}

	bool filled = rendered_square_symbol.get_is_filled();

	double size = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_square_symbol.get_size();

	// Get the point position, and project it to the canvas coordinate system.
	const GPlatesMaths::PointOnSphere &pos =
			rendered_square_symbol.get_centre();

	// Point pa is the centre of our square.
	QPointF pa = get_projected_unwrapped_position(pos);

	// Points pb,pc,pd and pe are the vertices of the square beginning from
	// the top right corner and going clockwise.

	// FIXME: we can skip the QPointFs here and set up the coords directly in the
	// coloured_vertex_type instantiations below.
	QPointF pb(pa.x()+size,pa.y()+size);
	QPointF pc(pa.x()+size,pa.y()-size);
	QPointF pd(pa.x()-size,pa.y()-size);
	QPointF pe(pa.x()-size,pa.y()+size);

	coloured_vertex_type va(pa.x(), pa.y(),0,Colour::to_rgba8(*colour));
	coloured_vertex_type vb(pb.x(), pb.y(),0,Colour::to_rgba8(*colour));
	coloured_vertex_type vc(pc.x(), pc.y(),0,Colour::to_rgba8(*colour));
	coloured_vertex_type vd(pd.x(), pd.y(),0,Colour::to_rgba8(*colour));
	coloured_vertex_type ve(pe.x(), pe.y(),0,Colour::to_rgba8(*colour));

	if (filled)
	{
		stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_triangles_stream();

		stream_primitives_type::TriangleFans stream_triangle_fans(stream);

		// The polygon state is fill, front/back by default, so I shouldn't need
		// to change anything here.

		stream_triangle_fans.begin_triangle_fan();
		stream_triangle_fans.add_vertex(va);
		stream_triangle_fans.add_vertex(vb);
		stream_triangle_fans.add_vertex(vc);
		stream_triangle_fans.add_vertex(vd);
		stream_triangle_fans.add_vertex(ve);
		stream_triangle_fans.add_vertex(vb);
		stream_triangle_fans.end_triangle_fan();
	}
	else
	{
		const float line_width = rendered_square_symbol.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

		stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

		stream_primitives_type::LineStrips stream_line_strips(stream);

		stream_line_strips.begin_line_strip();
		stream_line_strips.add_vertex(vb);
		stream_line_strips.add_vertex(vc);
		stream_line_strips.add_vertex(vd);
		stream_line_strips.add_vertex(ve);
		stream_line_strips.add_vertex(vb);
		stream_line_strips.end_line_strip();
	}

}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_circle_symbol(
	const GPlatesViewOperations::RenderedCircleSymbol &rendered_circle_symbol)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_circle_symbol);
	if (!colour)
	{
		return;
	}

	bool filled = rendered_circle_symbol.get_is_filled();

	// Get the circle position.
	const GPlatesMaths::PointOnSphere &pos = rendered_circle_symbol.get_centre();
	QPointF pcentre = get_projected_unwrapped_position(pos);

	if (filled)
	{
		const float point_size = FILLED_CIRCLE_SYMBOL_CORRECTION *
			rendered_circle_symbol.get_size() * POINT_SIZE_ADJUSTMENT * d_scale;

		// Get the stream for points of the current point size.
		stream_primitives_type &stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

		// Vertex representing the point's position and colour.
		// Convert colour from floats to bytes to use less vertex memory.
		const coloured_vertex_type vertex(pcentre.x(), pcentre.y(), 0, Colour::to_rgba8(*colour));

		// Used to add points to the stream.
		stream_primitives_type::Points stream_points(stream);

		stream_points.begin_points();
		stream_points.add_vertex(vertex);
		stream_points.end_points();
	}
	else
	{
		double radius = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_circle_symbol.get_size();

		const float line_width = rendered_circle_symbol.get_size() * LINE_WIDTH_ADJUSTMENT * d_scale;

		// Tessellate the circle on the plane of the map.
		coloured_vertex_seq_type vertices;
		tessellate_on_plane(vertices,
							pcentre,
							radius,
							SMALL_CIRCLE_ANGULAR_INCREMENT,
							Colour::to_rgba8(*colour));

		// Create a closed loop from the tessellated points.
		vertices.push_back(vertices.front());

		stream_primitives_type &stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

		stream_primitives_type::LineStrips stream_line_strips(stream);

		stream_line_strips.begin_line_strip();
		BOOST_FOREACH(const coloured_vertex_type vertex,vertices)
		{
			stream_line_strips.add_vertex(vertex);
		}
		stream_line_strips.end_line_strip();
	}

}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_cross_symbol(
	const GPlatesViewOperations::RenderedCrossSymbol &rendered_cross_symbol)
{
	// Some thoughts about rendering symbols on the map:
	// * symbols should probably not be projected, or north-aligned, otherwise they would look
	// distorted at certain points on the map (e.g. near the poles in mercator/robinson).  We
	// project only the central location of the symbol and otherwise draw the vertices in the map
	// canvas coordinate system.
	// * we don't want to wrap symbols - if they occur right at the edge of the map, it's fine
	// to have part of the symbol going off the edge of the map, and onto the rest of the canvas.

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_cross_symbol);
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_colour = Colour::to_rgba8(*colour);

	const float line_width = rendered_cross_symbol.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	double size = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_cross_symbol.get_size();


	// Get the desired cross position.
	const GPlatesMaths::PointOnSphere &pos =
			rendered_cross_symbol.get_centre();

	// We want to project only this central point.
	QPointF centre = get_projected_unwrapped_position(pos);

	// FIXME: we can skip the QPointFs here and set up the coords directly in the
	// coloured_vertex_type instantiations below.
	QPointF horizontal_shift(size,0);
	QPointF vertical_shift(0,size);
	QPointF pa = centre - vertical_shift;
	QPointF pb = centre + vertical_shift;
	QPointF pc = centre - horizontal_shift;
	QPointF pd = centre + horizontal_shift;

	stream_primitives_type &stream =
		d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	const coloured_vertex_type va(pa.x(),pa.y(),0,rgba8_colour);
	const coloured_vertex_type vb(pb.x(),pb.y(),0,rgba8_colour);
	const coloured_vertex_type vc(pc.x(),pc.y(),0,rgba8_colour);
	const coloured_vertex_type vd(pd.x(),pd.y(),0,rgba8_colour);

	stream_primitives_type::LineStrips stream_line_strips(stream);

	stream_line_strips.begin_line_strip();
	stream_line_strips.add_vertex(va);
	stream_line_strips.add_vertex(vb);
	stream_line_strips.end_line_strip();

	stream_line_strips.begin_line_strip();
	stream_line_strips.add_vertex(vc);
	stream_line_strips.add_vertex(vd);
	stream_line_strips.end_line_strip();

}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_geometries(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// TODO: If there's a spatial partition of rendered geometries then do view frustum
	// culling depending on what parts of the map are visible in the viewport - this will require
	// map projecting the boundary of each cube quad-tree tile and calculating a 2D bounding box
	// in map projection space.

	// Visit each RenderedGeometry.
	std::for_each(
		d_rendered_geometry_layer.rendered_geometry_begin(),
		d_rendered_geometry_layer.rendered_geometry_end(),
		boost::bind(&GPlatesViewOperations::RenderedGeometry::accept_visitor, _1, boost::ref(*this)));
}


template <class T>
boost::optional<GPlatesGui::Colour>
GPlatesGui::MapRenderedGeometryLayerPainter::get_colour_of_rendered_geometry(
		const T &geom)
{
	return geom.get_colour().get_colour(d_colour_scheme);
}


template <typename LineGeometryType>
void
GPlatesGui::MapRenderedGeometryLayerPainter::dateline_wrap_and_project_line_geometry(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry)
{
	// Wrap the rotated geometry to the longitude range...
	//   [-180 + central_meridian, central_meridian + 180]
	std::vector<lat_lon_line_geometry_type> wrapped_line_geometries;
	d_dateline_wrapper->wrap(line_geometry, wrapped_line_geometries);

	// Paint each wrapped piece of the original geometry.
	BOOST_FOREACH(const lat_lon_line_geometry_type &wrapped_line_geometry, wrapped_line_geometries)
	{
		// If it's a wrapped polygon (not a polyline) then add the start point to the end in order
		// to close the loop - we need to do this because we're iterating over vertices not arcs.
		if (boost::is_same<LineGeometryType, GPlatesMaths::PolygonOnSphere>::value)
		{
			wrapped_line_geometry->push_back(wrapped_line_geometry->front());
		}

		project_and_tessellate_wrapped_line_geometry(
				dateline_wrapped_projected_line_geometry,
				wrapped_line_geometry->begin(),
				wrapped_line_geometry->end());
	}
}


template <typename LatLonPointForwardIter>
void
GPlatesGui::MapRenderedGeometryLayerPainter::project_and_tessellate_wrapped_line_geometry(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const LatLonPointForwardIter &begin_lat_lon_points,
		const LatLonPointForwardIter &end_lat_lon_points)
{
	if (begin_lat_lon_points == end_lat_lon_points)
	{
		return;
	}

	const double &central_longitude = d_map_projection->central_llp().longitude();

	// Initialise for arc iteration.
	GPlatesMaths::LatLonPoint arc_start_lat_lon_point = *begin_lat_lon_points;
	GPlatesMaths::PointOnSphere arc_start_point_on_sphere = make_point_on_sphere(arc_start_lat_lon_point);

	// Add the first vertex of the sequence of points.
	dateline_wrapped_projected_line_geometry.add_vertex(
			get_projected_wrapped_position(arc_start_lat_lon_point));

	// Iterate over the line geometry points.
	for (LatLonPointForwardIter lat_lon_points_iter = begin_lat_lon_points;
		lat_lon_points_iter != end_lat_lon_points;
		++lat_lon_points_iter)
	{
		// The end point of the current arc.
		const GPlatesMaths::LatLonPoint arc_end_lat_lon_point = *lat_lon_points_iter;
		const GPlatesMaths::PointOnSphere arc_end_point_on_sphere = make_point_on_sphere(arc_end_lat_lon_point);

		// Tessellate the current arc if its two endpoints are far enough apart.
		if (dot(arc_start_point_on_sphere.position_vector(), arc_end_point_on_sphere.position_vector()).dval()
			< COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD)
		{
			const GPlatesMaths::GreatCircleArc gca =
					GPlatesMaths::GreatCircleArc::create(arc_start_point_on_sphere, arc_end_point_on_sphere);

			// Tessellate the current great circle arc.
			std::vector<GPlatesMaths::PointOnSphere> tess_points;
			tessellate(tess_points, gca, GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

			const GPlatesMaths::real_t arc_start_point_longitude(arc_start_lat_lon_point.longitude());
			const GPlatesMaths::real_t arc_end_point_longitude(arc_end_lat_lon_point.longitude());

			// If the arc is entirely on the dateline (both end points on the dateline)...
			// NOTE: This excludes arcs at the north or south pole singularities - the ones that form
			// horizontal lines at the top and bottom of a rectangular projection but are degenerate.
			// We don't need to worry about these because they are zero length and won't contribute
			// any tessellated vertices.
			if (arc_start_point_longitude == arc_end_point_longitude &&
				abs(arc_start_point_longitude - central_longitude) == 180.0)
			{
				// Add the tessellated points skipping the *first* since it was added by the previous arc and
				// skipping the *last* since it will be added by this arc.
				for (unsigned int n = 1; n < tess_points.size() - 1; ++n)
				{
					// NOTE: These tessellated points have not been wrapped (dateline wrapped) and hence
					// could end up with -180 or +180 for the longitude (due to numerical precision).
					// So we must make sure their wrapping matches the arc end points (if both endpoints
					// are *on* the dateline). If only one of the arc end points is on the dateline then
					// the tessellated points *between* the arc end points (if any) are relatively safe
					// from this wrapping problem (since they're *off* the dateline somewhat).
					// Note that this is also why we exclude the start and end points in the tessellation
					// (we want to respect their original wrapping since they can be *on* the dateline).
					const GPlatesMaths::real_t tess_latitude = asin(tess_points[n].position_vector().z());
					const GPlatesMaths::LatLonPoint tess_lat_lon(
							convert_rad_to_deg(tess_latitude).dval(),
							arc_start_point_longitude.dval());

					dateline_wrapped_projected_line_geometry.add_vertex(
							get_projected_wrapped_position(tess_lat_lon));
				}
			}
			else // arc is *not* entirely on the dateline (although one of the end points could be) ...
			{
				// Add the tessellated points skipping the *first* since it was added by the previous arc and
				// skipping the *last* since it will be added by this arc.
				for (unsigned int n = 1; n < tess_points.size() - 1; ++n)
				{
					// These tessellated points have not been wrapped but they are also not *on* the
					// dateline and hence are relatively safe from wrapping problems.
					// Just make sure we keep the longitude in the range...
					//   [-180 + central_meridian, central_meridian + 180]
					// ...since we're converting from PointOnSphere to LatLonPoint (ie, [-180, 180] range).
					// Note; 'central_longitude' should be in the range [-180, 180] itself.
					GPlatesMaths::LatLonPoint tess_lat_lon = make_lat_lon_point(tess_points[n]);
					if (tess_lat_lon.longitude() < -180 + central_longitude)
					{
						tess_lat_lon = GPlatesMaths::LatLonPoint(
								tess_lat_lon.latitude(),
								tess_lat_lon.longitude() + 360);
					}
					else if (tess_lat_lon.longitude() > central_longitude + 180)
					{
						tess_lat_lon = GPlatesMaths::LatLonPoint(
								tess_lat_lon.latitude(),
								tess_lat_lon.longitude() - 360);
					}

					dateline_wrapped_projected_line_geometry.add_vertex(
							get_projected_wrapped_position(tess_lat_lon));
				}
			}
		}

		// Vertex representing the projected arc end point's position and colour.
		// NOTE: We're adding the original wrapped lat/lon point (ie, correctly wrapped).
		dateline_wrapped_projected_line_geometry.add_vertex(
				get_projected_wrapped_position(arc_end_lat_lon_point));

		dateline_wrapped_projected_line_geometry.add_great_circle_arc();

		arc_start_lat_lon_point = arc_end_lat_lon_point;
		arc_start_point_on_sphere = arc_end_point_on_sphere;
	}

	dateline_wrapped_projected_line_geometry.add_geometry();
}


template <typename LineGeometryType>
void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_fill_geometry(
		GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons,
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
		const Colour &colour)
{
	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_line_geometry;
	dateline_wrap_and_project_line_geometry<LineGeometryType>(
			dateline_wrapped_projected_line_geometry,
			line_geometry);

	const std::vector<unsigned int> &geometries = dateline_wrapped_projected_line_geometry.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	unsigned int great_circle_arc_index = 0;
	const std::vector<unsigned int> &great_circle_arcs =
			dateline_wrapped_projected_line_geometry.get_great_circle_arcs();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices =
			dateline_wrapped_projected_line_geometry.get_vertices();

	// Iterate over the dateline wrapped geometries.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		std::vector<QPointF> filled_polygon_geometry;

		// Note that the dateline wrapper ensures the first great circle arc of each wrapped geometry
		// contains at least two vertices.

		// Iterate over the great circle arcs of the current geometry.
		for ( ; great_circle_arc_index < geometries[geometry_index]; ++great_circle_arc_index)
		{
			// Iterate over the vertices of the current great circle arc.
			for ( ; vertex_index < great_circle_arcs[great_circle_arc_index]; ++vertex_index)
			{
				filled_polygon_geometry.push_back(vertices[vertex_index]);
			}
		}

		// Add the current filled polygon geometry.
		filled_polygons.add_filled_polygon(filled_polygon_geometry, colour);
	}
}


template <typename LineGeometryType>
void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_line_geometry(
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
		const Colour &colour,
		stream_primitives_type &lines_stream,
		boost::optional<double> arrow_head_size)
{
	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_line_geometry;
	dateline_wrap_and_project_line_geometry<LineGeometryType>(
			dateline_wrapped_projected_line_geometry,
			line_geometry);

	const std::vector<unsigned int> &geometries = dateline_wrapped_projected_line_geometry.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	unsigned int great_circle_arc_index = 0;
	const std::vector<unsigned int> &great_circle_arcs =
			dateline_wrapped_projected_line_geometry.get_great_circle_arcs();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices =
			dateline_wrapped_projected_line_geometry.get_vertices();

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour);

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	// Iterate over the dateline wrapped geometries.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		stream_line_strips.begin_line_strip();

		// Keep track of the last and second last vertices to calculate best estimate of
		// arrow direction tangent and the end of each great circle arc.
		const QPointF *last_vertex = NULL;
		const QPointF *second_last_vertex = NULL;

		// Iterate over the great circle arcs of the current geometry.
		for ( ; great_circle_arc_index < geometries[geometry_index]; ++great_circle_arc_index)
		{
			// Iterate over the vertices of the current great circle arc.
			for ( ; vertex_index < great_circle_arcs[great_circle_arc_index]; ++vertex_index)
			{
				const QPointF &vertex = vertices[vertex_index];
				const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, rgba8_color);
				stream_line_strips.add_vertex(coloured_vertex);

				// Keep track of the last and second last vertices to calculate best estimate of
				// arrow direction tangent and the end of each great circle arc.
				second_last_vertex = last_vertex;
				last_vertex = &vertex;
			}

			// If we need to draw an arrow head at the end of each great circle arc segment...
			if (arrow_head_size)
			{
				// The dateline wrapper ensures the first great circle arc of each wrapped geometry
				// contains at least two vertices so this should never fail.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						last_vertex && second_last_vertex,
						GPLATES_ASSERTION_SOURCE);

				paint_arrow_head(
						*last_vertex,
						// Our best estimate of the arrow direction tangent at the GCA end point...
						*last_vertex - *second_last_vertex,
						arrow_head_size.get(),
						rgba8_color);
			}
		}

		stream_line_strips.end_line_strip();
	}
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_arrow_head(
		const QPointF &arrow_head_apex,
		const QPointF &arrow_head_direction,
		const double &arrowhead_size,
		rgba8_t rgba8_color)
{
	// The length of the arrow head.
	const GPlatesMaths::real_t arrow_head_direction_magnitude =
			sqrt(arrow_head_direction.x() * arrow_head_direction.x() + arrow_head_direction.y() * arrow_head_direction.y());

	// Avoid divide-by-zero.
	if (arrow_head_direction_magnitude == 0)
	{
		return;
	}

	// Vector in the direction of the arrowhead with magnitude equal to the arrow head size.
	const QPointF arrow_head_vector =
			arrowhead_size * (arrow_head_direction / arrow_head_direction_magnitude.dval());

	// A vector perpendicular to the arrow direction, for forming the base of the triangle.
	QPointF perpendicular_vector(-arrow_head_vector.y(), arrow_head_vector.x());

	const QPointF arrow_head_base = arrow_head_apex - arrow_head_vector;
	const QPointF arrow_base_corners[2] =
	{
		arrow_head_base + perpendicular_vector * ARROWHEAD_BASE_HEIGHT_RATIO,
		arrow_head_base - perpendicular_vector * ARROWHEAD_BASE_HEIGHT_RATIO
	};

	// Used to add triangles to the stream.
	stream_primitives_type &triangles_stream = d_layer_painter->drawables_off_the_sphere.get_triangles_stream();
	stream_primitives_type::Triangles stream_triangles(triangles_stream);

	stream_triangles.begin_triangles();

	stream_triangles.add_vertex(
			coloured_vertex_type(arrow_head_apex.x(), arrow_head_apex.y(), 0/*z*/, rgba8_color));
	stream_triangles.add_vertex(
			coloured_vertex_type(arrow_base_corners[0].x(), arrow_base_corners[0].y(), 0/*z*/, rgba8_color));
	stream_triangles.add_vertex(
			coloured_vertex_type(arrow_base_corners[1].x(), arrow_base_corners[1].y(), 0/*z*/, rgba8_color));

	stream_triangles.end_triangles();
}


QPointF
GPlatesGui::MapRenderedGeometryLayerPainter::get_projected_wrapped_position(
		const GPlatesMaths::LatLonPoint &lat_lon_point) const
{
	const double &central_longitude = d_map_projection->central_llp().longitude();

	double x = lat_lon_point.longitude();
	double y = lat_lon_point.latitude();

	// Make sure the longitude is within [-180+EPSILON, 180-EPSILON] around the central meridian longitude.
	// 
	// This is to prevent subsequent map projection from wrapping (-180 -> +180 or vice versa) due to
	// the map projection code receiving a longitude value slightly outside that range or the map
	// projection code itself having numerical precision issues.
	//
	// We need this for *wrapped* vertices since they can lie *on* the dateline
	if (x < central_longitude + LONGITUDE_RANGE_LOWER_LIMIT)
	{
		x = central_longitude + LONGITUDE_RANGE_LOWER_LIMIT;
	}
	else if (x > central_longitude + LONGITUDE_RANGE_UPPER_LIMIT)
	{
		x = central_longitude + LONGITUDE_RANGE_UPPER_LIMIT;
	}

	// Project onto the map.
	d_map_projection->forward_transform(x, y);

	return QPointF(x, y);
}


QPointF
GPlatesGui::MapRenderedGeometryLayerPainter::get_projected_unwrapped_position(
		const GPlatesMaths::PointOnSphere &point_on_sphere) const
{
	// Convert to lat/lon.
	const GPlatesMaths::LatLonPoint lat_lon_point = make_lat_lon_point(point_on_sphere);
	double x = lat_lon_point.longitude();
	double y = lat_lon_point.latitude();

	// Note that unwrapped vertices do not lie *on* the dateline (within numerical tolerance)
	// and hence do not suffer from wrapping problems (ie, -180 -> 180 or vice versa).

	// Project onto the map.
	d_map_projection->forward_transform(x, y);

	return QPointF(x, y);
}
