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


#include <cmath>
#include <limits>
#include <stack>
#include <iostream>
#include <boost/foreach.hpp>

#include <QDebug>
#include <QLineF>
#include <QPointF>
#include <QTransform>

#include "Colour.h"
#include "ColourScheme.h"
#include "MapRenderedGeometryLayerPainter.h"
#include "MapProjection.h"

#include "app-logic/GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/AngularDistance.h"
#include "maths/AngularExtent.h"
#include "maths/EllipseGenerator.h"
#include "maths/GreatCircle.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/SphericalArea.h"

#include "opengl/GLRenderer.h"

#include "view-operations/RenderedArrowedPolyline.h"
#include "view-operations/RenderedCircleSymbol.h"
#include "view-operations/RenderedColouredEdgeSurfaceMesh.h"
#include "view-operations/RenderedColouredMultiPointOnSphere.h"
#include "view-operations/RenderedColouredPolygonOnSphere.h"
#include "view-operations/RenderedColouredPolylineOnSphere.h"
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
#include "view-operations/RenderedSubductionTeethPolyline.h"
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
	const GPlatesMaths::AngularExtent GREAT_CIRCLE_ARC_ANGULAR_EXTENT_THRESHOLD =
			GPlatesMaths::AngularExtent::create_from_cosine(COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

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

	/**
	 * Max arrowhead size for arrowed polylines (in post-projection space).
	 *
	 * Ensures the arrowheads don't get too big when the map is small in the viewport resulting
	 * in the vertices of polyline being close together (and causing arrowheads, at each vertex, to clump).
	 */
	const double MAX_ARROWED_POLYLINE_ARROWHEAD_SIZE = 0.005 * 180;

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
		const QPointF projected_point = projection.forward_transform(point);

		qDebug() << "Vertex: " << point.position_vector();
		qDebug() << projected_point;
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

	/**
	 * Used when refining (subdividing) a filled triangle using vertex colouring
	 * (the vertex colours must also be interpolated).
	 *
	 * Flat filled colouring of polygons/triangles only need to be tessellated along their
	 * boundaries since a concave polygon with a flat colour can be rendered using stencil testing.
	 */
	struct RefinedVertexColouredTriangle
	{
		RefinedVertexColouredTriangle(
				const GPlatesMaths::PointOnSphere &vertex_point0_,
				const GPlatesMaths::PointOnSphere &vertex_point1_,
				const GPlatesMaths::PointOnSphere &vertex_point2_,
				const GPlatesGui::Colour &vertex_colour0_,
				const GPlatesGui::Colour &vertex_colour1_,
				const GPlatesGui::Colour &vertex_colour2_,
				boost::optional<GPlatesMaths::LatLonPoint> vertex_lat_lon_point0_ = boost::none,
				boost::optional<GPlatesMaths::LatLonPoint> vertex_lat_lon_point1_ = boost::none,
				boost::optional<GPlatesMaths::LatLonPoint> vertex_lat_lon_point2_ = boost::none)
		{
			vertex_points[0] = vertex_point0_;
			vertex_points[1] = vertex_point1_;
			vertex_points[2] = vertex_point2_;

			vertex_colours[0] = vertex_colour0_;
			vertex_colours[1] = vertex_colour1_;
			vertex_colours[2] = vertex_colour2_;

			vertex_lat_lon_points[0] = vertex_lat_lon_point0_;
			vertex_lat_lon_points[1] = vertex_lat_lon_point1_;
			vertex_lat_lon_points[2] = vertex_lat_lon_point2_;

			set_edge_lengths();
		}

		// Using boost::optional since native array elements must be default constructible.
		// And indexing into arrays makes our code easier to write.
		boost::optional<GPlatesMaths::PointOnSphere> vertex_points[3];
		boost::optional<GPlatesMaths::LatLonPoint> vertex_lat_lon_points[3]; // Only used for wrapped triangles.
		boost::optional<GPlatesGui::Colour> vertex_colours[3];
		boost::optional<GPlatesMaths::AngularDistance> edge_lengths[3];

	private:

		void
		set_edge_lengths()
		{
			for (unsigned int e = 0; e < 3; ++e)
			{
				edge_lengths[e] = GPlatesMaths::AngularDistance::create_from_cosine(
						dot(
								vertex_points[e]->position_vector(),
								vertex_points[(e + 1) % 3]->position_vector()));
			}
		}
	};
}


const float GPlatesGui::MapRenderedGeometryLayerPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::MapRenderedGeometryLayerPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;


GPlatesGui::MapRenderedGeometryLayerPainter::MapRenderedGeometryLayerPainter(
		const MapProjection::non_null_ptr_to_const_type &map_projection,
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		const double &inverse_viewport_zoom_factor,
		const double &device_independent_pixel_to_map_space_ratio,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_map_projection(map_projection),
	d_rendered_geometry_layer(rendered_geometry_layer),
	d_gl_visual_layers(gl_visual_layers),
	d_inverse_zoom_factor(inverse_viewport_zoom_factor),
	d_device_independent_pixel_to_map_space_ratio(device_independent_pixel_to_map_space_ratio),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f),
	d_dateline_wrapper(
			GPlatesMaths::DateLineWrapper::create(
					// Move the dateline wrapping to be [-180 + central_meridian, central_meridian + 180]...
					map_projection->central_meridian()))
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
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_multi_point_on_sphere.get_colour());
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
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_coloured_multi_point_on_sphere(
			const GPlatesViewOperations::RenderedColouredMultiPointOnSphere &rendered_coloured_multi_point_on_sphere)	
{
	// The multipoint and its associated per-point colours.
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
			rendered_coloured_multi_point_on_sphere.get_multi_point_on_sphere();
	const std::vector<ColourProxy> &point_colours = rendered_coloured_multi_point_on_sphere.get_point_colours();

	const unsigned int num_points = multi_point_on_sphere->number_of_points();

	// Each point must have an associated colour.
	if (point_colours.size() != num_points)
	{
		return;
	}

	// Convert the point colours.
	std::vector<Colour> vertex_colours;
	vertex_colours.reserve(num_points);
	for (unsigned int c = 0; c < num_points; ++c)
	{
		boost::optional<Colour> vertex_colour = get_vector_geometry_colour(point_colours[c]);
		if (!vertex_colour)
		{
			// Should always get a valid vertex colour - if not then return without rendering.
			return;
		}

		vertex_colours.push_back(vertex_colour.get());
	}

	const float point_size =
			rendered_coloured_multi_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT * d_scale;

	// Get the stream for points of the current point size.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

	// Used to add points to the stream.
	stream_primitives_type::Points stream_points(stream);

	stream_points.begin_points();

	GPlatesMaths::MultiPointOnSphere::const_iterator point_iter = multi_point_on_sphere->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator point_end = multi_point_on_sphere->end();
	for (unsigned int point_index = 0; point_iter != point_end; ++point_iter, ++point_index)
	{
		// Get the projected point position.
		const QPointF proj_pos = get_projected_unwrapped_position(*point_iter);

		// Vertex representing the projected point's position and colour.
		const coloured_vertex_type vertex(
				proj_pos.x(),
				proj_pos.y(),
				0/*z*/,
				Colour::to_rgba8(vertex_colours[point_index]));

		stream_points.add_vertex(vertex);
	}

	stream_points.end_points();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_point_on_sphere(
			const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_point_on_sphere.get_colour());
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
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_polygon_on_sphere.get_colour());
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

		// Convert colour from floats to bytes to use less vertex memory.
		const rgba8_t rgba8_fill_colour = Colour::to_rgba8(fill_colour);

		// Dateline wrap and project the polygon and render each wrapped polygon as a filled polygon.
		paint_fill_geometry<GPlatesMaths::PolygonOnSphere>(
				filled_polygons,
				polygon_on_sphere,
				rgba8_fill_colour);

		return;
	}

	const float line_width =
			rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	paint_line_geometry<GPlatesMaths::PolygonOnSphere>(polygon_on_sphere, rgba8_color, stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_coloured_polygon_on_sphere(
			const GPlatesViewOperations::RenderedColouredPolygonOnSphere &rendered_coloured_polygon_on_sphere)
{
	// The polygon and its associated per-point colours.
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
			rendered_coloured_polygon_on_sphere.get_polygon_on_sphere();
	const std::vector<ColourProxy> &point_colours = rendered_coloured_polygon_on_sphere.get_point_colours();

	const unsigned int num_points = polygon_on_sphere->number_of_vertices_in_exterior_ring();

	// Each point must have an associated colour.
	//
	// NOTE: Currently there are only scalar values for the exterior ring
	// TODO: Add scalar values for interior rings also.
	if (point_colours.size() != num_points)
	{
		return;
	}

	// Convert the point colours.
	std::vector<Colour> vertex_colours;
	vertex_colours.reserve(num_points);
	for (unsigned int c = 0; c < num_points; ++c)
	{
		boost::optional<Colour> vertex_colour = get_vector_geometry_colour(point_colours[c]);
		if (!vertex_colour)
		{
			// Should always get a valid vertex colour - if not then return without rendering.
			return;
		}

		vertex_colours.push_back(vertex_colour.get());
	}

	const float line_width =
			rendered_coloured_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Paint the polygon's exterior ring.
	paint_vertex_coloured_polygon(polygon_on_sphere, vertex_colours, stream);
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_polyline_on_sphere(
			const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_polyline_on_sphere.get_colour());
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

		// Convert colour from floats to bytes to use less vertex memory.
		const rgba8_t rgba8_fill_colour = Colour::to_rgba8(fill_colour);

		// Dateline wrap and project the polygon and render each wrapped polygon as a filled polygon.
		paint_fill_geometry<GPlatesMaths::PolylineOnSphere>(
				filled_polygons,
				polyline_on_sphere,
				rgba8_fill_colour);

		return;
	}

	const float line_width =
			rendered_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(polyline_on_sphere, rgba8_color, stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_coloured_polyline_on_sphere(
			const GPlatesViewOperations::RenderedColouredPolylineOnSphere &rendered_coloured_polyline_on_sphere)
{
	// The polyline and its associated per-point colours.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_coloured_polyline_on_sphere.get_polyline_on_sphere();
	const std::vector<ColourProxy> &point_colours = rendered_coloured_polyline_on_sphere.get_point_colours();

	const unsigned int num_points = polyline_on_sphere->number_of_vertices();

	// Each point must have an associated colour.
	if (point_colours.size() != num_points)
	{
		return;
	}

	// Convert the point colours.
	std::vector<Colour> vertex_colours;
	vertex_colours.reserve(num_points);
	for (unsigned int c = 0; c < num_points; ++c)
	{
		boost::optional<Colour> vertex_colour = get_vector_geometry_colour(point_colours[c]);
		if (!vertex_colour)
		{
			// Should always get a valid vertex colour - if not then return without rendering.
			return;
		}

		vertex_colours.push_back(vertex_colour.get());
	}

	const float line_width =
			rendered_coloured_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_vertex_coloured_polyline(polyline_on_sphere, vertex_colours, stream);
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_subduction_teeth_polyline(
		const GPlatesViewOperations::RenderedSubductionTeethPolyline &rendered_subduction_teeth_polyline)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_subduction_teeth_polyline.get_colour());
	if (!colour)
	{
		return;
	}

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_subduction_teeth_polyline.get_polyline_on_sphere();

	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_polyline;
	dateline_wrap_and_project_line_geometry(dateline_wrapped_projected_polyline, polyline_on_sphere);

	const std::vector<unsigned int> &geometries = dateline_wrapped_projected_polyline.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_colour = Colour::to_rgba8(colour.get());

	// Get the stream for lines of the current line width.
	const float line_width = rendered_subduction_teeth_polyline.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;
	stream_primitives_type &lines_stream = d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	const double teeth_width = d_device_independent_pixel_to_map_space_ratio * d_scale * rendered_subduction_teeth_polyline.get_teeth_width_in_pixels();
	const double teeth_spacing = teeth_width * rendered_subduction_teeth_polyline.get_teeth_spacing_to_width_ratio();
	const double teeth_height = teeth_width * rendered_subduction_teeth_polyline.get_teeth_height_to_width_ratio();
	// Wide lines push the teeth further from the centre of the line by half the line's width
	// (reduced by half a pixel since the line is typically anti-aliased, so we don't want a tiny gap between the line and teeth).
	const double teeth_offset = (0.5 * line_width - 0.5) * d_device_independent_pixel_to_map_space_ratio;
	// If overriding plate is on the left side then need to segment normal (which is on the right - see 'QLineF::normalVector()').
	const double overriding_normal_factor =
			(rendered_subduction_teeth_polyline.get_subduction_polarity() == GPlatesViewOperations::RenderedSubductionTeethPolyline::SubductionPolarity::RIGHT)
			? 1.0
			: -1.0;

	// Get the stream for triangles (for the subduction teeth).
	stream_primitives_type &teeth_stream = d_layer_painter->translucent_drawables_on_the_sphere.get_triangles_stream();
	stream_primitives_type::Triangles stream_teeth(teeth_stream);
	stream_teeth.begin_triangles();

	unsigned int geometry_part_index = 0;
	const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_polyline.get_geometry_parts();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices = dateline_wrapped_projected_polyline.get_vertices();

	// Iterate over the dateline wrapped polylines.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		// Iterate over the parts of the current wrapped polyline (there's only one part per wrapped polyline).
		const unsigned int end_geometry_part_index = geometries[geometry_index];
		for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
		{
			stream_line_strips.begin_line_strip();

			// Start vertex.
			const unsigned int start_vertex_index = vertex_index;
			const QPointF &start_vertex = vertices[start_vertex_index];
			const coloured_vertex_type coloured_start_vertex(start_vertex.x(), start_vertex.y(), 0/*z*/, rgba8_colour);
			stream_line_strips.add_vertex(coloured_start_vertex);

			const QPointF *prev_vertex = &start_vertex;
			++vertex_index;
			double length_since_last_tooth = 0;

			// Iterate over the segment-end vertices of the current geometry part (current wrapped polyline).
			const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
			for ( ; vertex_index < end_vertex_index; ++vertex_index)
			{
				// End vertex of current segment.
				const QPointF &vertex = vertices[vertex_index];
				const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, rgba8_colour);
				stream_line_strips.add_vertex(coloured_vertex);

				// Current segment (from previous vertex to current vertex).
				const QLineF segment(*prev_vertex, vertex);
				const double segment_length = segment.length();
				const double inv_segment_length = 1.0 / segment_length;

				// Paint the teeth at uniform spacings along the polyline.
				length_since_last_tooth += segment_length;
				while (length_since_last_tooth > teeth_spacing)
				{
					if (GPlatesMaths::are_almost_exactly_equal(segment_length, 0))
					{
						continue;
					}

					// Interpolate between the current segment's start and end points to get the tooth position.
					const double interp = inv_segment_length * (segment_length - (length_since_last_tooth - teeth_spacing));
					const QPointF tooth_base_midpoint = segment.pointAt(interp);

					// Direction along the current segment's line.
					const QPointF tooth_base_direction(
							inv_segment_length * segment.dx(),
							inv_segment_length * segment.dy());

					// Unit normal to the current segment.
					const QLineF segment_normal = segment.normalVector();  // same length as 'segment'
					const QPointF tooth_normal_direction(  // unit normal
							overriding_normal_factor * inv_segment_length * segment_normal.dx(),
							overriding_normal_factor * inv_segment_length * segment_normal.dy());

					// Wide lines push the teeth further from the centre of the line.
					const QPointF tooth_offset = teeth_offset * tooth_normal_direction;

					const QPointF tooth_triangle_vertices[3] =
					{
						tooth_base_midpoint + tooth_offset + teeth_height * tooth_normal_direction,
						tooth_base_midpoint + tooth_offset - 0.5 * teeth_width * tooth_base_direction,
						tooth_base_midpoint + tooth_offset + 0.5 * teeth_width * tooth_base_direction,
					};

					stream_teeth.add_vertex(coloured_vertex_type(tooth_triangle_vertices[0].x(), tooth_triangle_vertices[0].y(), 0/*z*/, rgba8_colour));
					stream_teeth.add_vertex(coloured_vertex_type(tooth_triangle_vertices[1].x(), tooth_triangle_vertices[1].y(), 0/*z*/, rgba8_colour));
					stream_teeth.add_vertex(coloured_vertex_type(tooth_triangle_vertices[2].x(), tooth_triangle_vertices[2].y(), 0/*z*/, rgba8_colour));

					length_since_last_tooth -= teeth_spacing;
				}

				prev_vertex = &vertex;
			}

			stream_line_strips.end_line_strip();
		}
	}

	stream_teeth.end_triangles();
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
	const mesh_type::colour_seq_type &mesh_colours =
			rendered_coloured_edge_surface_mesh.get_mesh_colours();

	if (rendered_coloured_edge_surface_mesh.get_use_vertex_colours())
	{
		const unsigned int num_mesh_colours = mesh_colours.size();

		// Convert the mesh vertex colours.
		std::vector<Colour> vertex_colours;
		vertex_colours.reserve(num_mesh_colours);
		for (unsigned int c = 0; c < num_mesh_colours; ++c)
		{
			boost::optional<Colour> vertex_colour = get_vector_geometry_colour(mesh_colours[c]);
			if (!vertex_colour)
			{
				// Should always get a valid vertex colour - if not then return without rendering mesh.
				return;
			}

			vertex_colours.push_back(vertex_colour.get());
		}

		// Iterate over the mesh edges.
		const unsigned int num_mesh_edges = mesh_edges.size();
		for (unsigned int e = 0; e < num_mesh_edges; ++e)
		{
			const mesh_type::Edge &mesh_edge = mesh_edges[e];

			// Create a polyline with two points for the current edge.
			const GPlatesMaths::PointOnSphere edge_points[2] =
			{
				mesh_vertices[mesh_edge.vertex_indices[0]],
				mesh_vertices[mesh_edge.vertex_indices[1]]
			};
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type edge_polyline =
					GPlatesMaths::PolylineOnSphere::create(
							edge_points,
							edge_points + 2);

			//
			// Paint the current single great circle arc edge (it might get dateline wrapped and
			// tessellated into smaller arcs).
			//

			DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_edge;
			dateline_wrap_and_project_line_geometry(
					dateline_wrapped_projected_edge,
					edge_polyline);

			const std::vector<unsigned int> &geometries = dateline_wrapped_projected_edge.get_geometries();
			const unsigned int num_geometries = geometries.size();
			if (num_geometries == 0)
			{
				// Continue to the next edge if there's nothing to paint - shouldn't really be able to get here.
				continue;
			}

			unsigned int geometry_part_index = 0;
			const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_edge.get_geometry_parts();

			unsigned int vertex_index = 0;
			const std::vector<QPointF> &vertices = dateline_wrapped_projected_edge.get_vertices();
			const DatelineWrappedProjectedLineGeometry::interpolate_original_segment_seq_type &
					interpolate_original_segments = dateline_wrapped_projected_edge.get_interpolate_original_segments();

			const Colour edge_vertex_colours[2] =
			{
				vertex_colours[mesh_edge.vertex_indices[0]],
				vertex_colours[mesh_edge.vertex_indices[1]]
			};

			// Used to add line strips to the stream.
			stream_primitives_type::LineStrips stream_line_strips(stream);

			// Iterate over the dateline wrapped geometries.
			for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
			{
				// Iterate over the parts of the current geometry (there will be only one part though).
				const unsigned int end_geometry_part_index = geometries[geometry_index];
				for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
				{
					stream_line_strips.begin_line_strip();

					// Iterate over the vertices of the current geometry part.
					const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
					for ( ; vertex_index < end_vertex_index; ++vertex_index)
					{
						const boost::optional<DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment> &
								interpolate_original_segment = interpolate_original_segments[vertex_index];
						if (interpolate_original_segment/*should always be true for polylines*/)
						{
							const Colour vertex_colour = Colour::linearly_interpolate(
									// There's only one original segment (our original edge)...
									edge_vertex_colours[0],
									edge_vertex_colours[1],
									interpolate_original_segment->interpolate_ratio);

							const QPointF &vertex = vertices[vertex_index];
							const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, Colour::to_rgba8(vertex_colour));
							stream_line_strips.add_vertex(coloured_vertex);
						}
					}

					stream_line_strips.end_line_strip();
				}
			}
		}
	}
	else // edge colouring ...
	{
		// Iterate over the mesh edges.
		const unsigned int num_mesh_edges = mesh_edges.size();
		for (unsigned int e = 0; e < num_mesh_edges; ++e)
		{
			const mesh_type::Edge &mesh_edge = mesh_edges[e];

			boost::optional<Colour> edge_colour = get_vector_geometry_colour(mesh_colours[e]);
			if (!edge_colour)
			{
				continue;
			}

			// Convert colour from floats to bytes to use less vertex memory.
			const rgba8_t edge_rgba8_color = Colour::to_rgba8(edge_colour.get());

			// Create a polyline with two points for the current edge.
			const GPlatesMaths::PointOnSphere edge_points[2] =
			{
				mesh_vertices[mesh_edge.vertex_indices[0]],
				mesh_vertices[mesh_edge.vertex_indices[1]]
			};
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type edge_polyline =
					GPlatesMaths::PolylineOnSphere::create(
							edge_points,
							edge_points + 2);

			//
			// Paint the current single great circle arc edge (it might get dateline wrapped and
			// tessellated into smaller arcs).
			//

			DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_edge;
			dateline_wrap_and_project_line_geometry(
					dateline_wrapped_projected_edge,
					edge_polyline);

			const std::vector<unsigned int> &geometries = dateline_wrapped_projected_edge.get_geometries();
			const unsigned int num_geometries = geometries.size();
			if (num_geometries == 0)
			{
				// Continue to the next edge if there's nothing to paint - shouldn't really be able to get here.
				continue;
			}

			unsigned int geometry_part_index = 0;
			const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_edge.get_geometry_parts();

			unsigned int vertex_index = 0;
			const std::vector<QPointF> &vertices = dateline_wrapped_projected_edge.get_vertices();

			// Used to add line strips to the stream.
			stream_primitives_type::LineStrips stream_line_strips(stream);

			// Iterate over the dateline wrapped geometries.
			for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
			{
				// Iterate over the parts of the current geometry (there will be only one part though).
				const unsigned int end_geometry_part_index = geometries[geometry_index];
				for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
				{
					stream_line_strips.begin_line_strip();

					// Iterate over the vertices of the current geometry part.
					const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
					for ( ; vertex_index < end_vertex_index; ++vertex_index)
					{
						const QPointF &vertex = vertices[vertex_index];
						const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, edge_rgba8_color);
						stream_line_strips.add_vertex(coloured_vertex);
					}

					stream_line_strips.end_line_strip();
				}
			}
		}
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
	const mesh_type::colour_seq_type &mesh_colours =
			rendered_coloured_triangle_surface_mesh.get_mesh_colours();

	if (rendered_coloured_triangle_surface_mesh.get_use_vertex_colours())
	{
		const unsigned int num_mesh_colours = mesh_colours.size();

		// Convert the mesh vertex colours.
		std::vector<Colour> vertex_colours;
		vertex_colours.reserve(num_mesh_colours);
		for (unsigned int c = 0; c < num_mesh_colours; ++c)
		{
			boost::optional<Colour> vertex_colour = get_vector_geometry_colour(mesh_colours[c]);
			if (!vertex_colour)
			{
				// Should always get a valid vertex colour - if not then return without rendering mesh.
				return;
			}

			// Modulate with the fill modulate colour...
			vertex_colour = Colour::modulate(
					vertex_colour.get(),
					rendered_coloured_triangle_surface_mesh.get_fill_modulate_colour());

			vertex_colours.push_back(vertex_colour.get());
		}

		// Iterate over the mesh triangles.
		const unsigned int num_mesh_triangles = mesh_triangles.size();
		for (unsigned int t = 0; t < num_mesh_triangles; ++t)
		{
			const mesh_type::Triangle &mesh_triangle = mesh_triangles[t];

			const Colour triangle_vertex_colours[3] =
			{
				vertex_colours[mesh_triangle.vertex_indices[0]],
				vertex_colours[mesh_triangle.vertex_indices[1]],
				vertex_colours[mesh_triangle.vertex_indices[2]]
			};

			// Create a PolygonOnSphere for the current triangle so we can pass it through the
			// dateline wrapping and projection code.
			const GPlatesMaths::PointOnSphere triangle_vertex_points[3] =
			{
				mesh_vertices[mesh_triangle.vertex_indices[0]],
				mesh_vertices[mesh_triangle.vertex_indices[1]],
				mesh_vertices[mesh_triangle.vertex_indices[2]]
			};
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type triangle_polygon =
					GPlatesMaths::PolygonOnSphere::create(
							triangle_vertex_points,
							triangle_vertex_points + 3);

			std::stack<RefinedVertexColouredTriangle> refined_triangles_to_process;
			bool use_wrapped_coordinates = false;
			bool use_separate_filled_drawable = false;

			if (d_dateline_wrapper->possibly_wraps(triangle_polygon))
			{
				use_wrapped_coordinates = true;

				// Wrap the triangle to the longitude range...
				//   [-180 + central_meridian, central_meridian + 180]
				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon> wrapped_triangle_polygons;
				d_dateline_wrapper->wrap_polygon(
						triangle_polygon,
						wrapped_triangle_polygons,
						// Note: We don't tessellate because we will mesh each wrapped polygon piece ourselves so that
						// we can interpolate between vertex colours. It also means we won't get any 'tessellated' vertices
						// along the dateline (which won't have interpolate information - since not on original polygon)...
						boost::none/*tessellate_threshold*/,
						false/*group_interior_with_exterior_rings*/);

				// Each wrapped piece of the original triangle.
				BOOST_FOREACH(
						GPlatesMaths::DateLineWrapper::LatLonPolygon &wrapped_triangle_polygon,
						wrapped_triangle_polygons)
				{
					std::vector<GPlatesMaths::PointOnSphere> wrapped_triangle_points;
					std::vector<GPlatesMaths::LatLonPoint> wrapped_triangle_lat_lon_points;
					std::vector<Colour> wrapped_triangle_colours;

					// Wrapped polygon should only have an exterior ring (since original had no interiors).
					const GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type &lat_lon_points =
							wrapped_triangle_polygon.get_exterior_ring_points();
					GPlatesMaths::DateLineWrapper::LatLonPolygon::interpolate_original_segment_seq_type interpolate_original_segments;
					wrapped_triangle_polygon.get_exterior_ring_interpolate_original_segments(interpolate_original_segments);

					// Iterate over the wrapped polygon points.
					const unsigned int num_lat_lon_points = lat_lon_points.size();
					for (unsigned int lat_lon_point_index = 0; lat_lon_point_index < num_lat_lon_points; ++lat_lon_point_index)
					{
						wrapped_triangle_lat_lon_points.push_back(lat_lon_points[lat_lon_point_index]);
						wrapped_triangle_points.push_back(make_point_on_sphere(lat_lon_points[lat_lon_point_index]));

						const boost::optional<GPlatesMaths::DateLineWrapper::LatLonPolygon::InterpolateOriginalSegment> &
								interpolate_original_segment = interpolate_original_segments[lat_lon_point_index];
						if (interpolate_original_segment)
						{
							wrapped_triangle_colours.push_back(
									Colour::linearly_interpolate(
											triangle_vertex_colours[interpolate_original_segment->original_segment_index],
											// Handle wrap-around to the start vertex using '% 3'...
											triangle_vertex_colours[(interpolate_original_segment->original_segment_index + 1) % 3],
											interpolate_original_segment->interpolate_ratio));
						}
						else // wrapped point not on segment of original triangle ...
						{
							// The wrapped triangle overlaps the North or South pole and hence the wrapped polygon
							// can be concave in shape (requiring a separate filled drawable call to resolve
							// the concavity). This is not the best way to render a concave polygon but this
							// case should be fairly rare and the colours should still come out interpolated
							// reasonably well.
							use_separate_filled_drawable = true;

							// The wrapped triangle was not tessellated, so the only time we get points not on
							// the original triangle segments is when the original triangle's interior overlaps the
							// North or South pole. In this case we'll interpolate the original triangle's vertex
							// colours using barycentric interpolation.
							//
							// First determine whether North or South pole.
							const GPlatesMaths::PointOnSphere &point = (lat_lon_points[lat_lon_point_index].latitude() > 0)
									? GPlatesMaths::PointOnSphere::north_pole
									: GPlatesMaths::PointOnSphere::south_pole;

							const GPlatesMaths::real_t triangle_area = GPlatesMaths::SphericalArea::calculate_spherical_triangle_area(
									triangle_vertex_points[0], triangle_vertex_points[1], triangle_vertex_points[2]);
							if (triangle_area == 0)
							{
								// The triangle area is too close to zero so just use the average vertex colour.
								wrapped_triangle_colours.push_back(
										Colour::linearly_interpolate(
												triangle_vertex_colours[0], triangle_vertex_colours[1], triangle_vertex_colours[2],
												0.333, 0.333));
								continue;
							}
							const GPlatesMaths::real_t inv_triangle_area = 1.0 / triangle_area;

							const GPlatesMaths::real_t interp0 = inv_triangle_area *
									GPlatesMaths::SphericalArea::calculate_spherical_triangle_area(
											point,
											triangle_vertex_points[1],
											triangle_vertex_points[2]);
							const GPlatesMaths::real_t interp1 = inv_triangle_area *
									GPlatesMaths::SphericalArea::calculate_spherical_triangle_area(
											point,
											triangle_vertex_points[2],
											triangle_vertex_points[0]);
							wrapped_triangle_colours.push_back(
									Colour::linearly_interpolate(
											triangle_vertex_colours[0], triangle_vertex_colours[1], triangle_vertex_colours[2],
											interp0.dval(), interp1.dval()));
						}
					}

					// Should have at least 3 vertices.
					const unsigned int num_vertices = wrapped_triangle_lat_lon_points.size();
					if (num_vertices < 3)
					{
						// This shouldn't happen though.
						continue;
					}

					// Emit a fan of triangles with the first vertex as the fan apex vertex.
					// The fan should be convex (unless original triangle contains the North or
					// South pole - but that case is handled above by using a separate filled drawable)
					// and hence fan always represents interior fill of wrapped triangle with no overlap.
					const unsigned int num_triangles = num_vertices - 2;
					for (unsigned int tri = 0; tri < num_triangles; ++tri)
					{
						refined_triangles_to_process.push(
								RefinedVertexColouredTriangle(
										wrapped_triangle_points[0],
										wrapped_triangle_points[tri + 1],
										wrapped_triangle_points[tri + 2],
										wrapped_triangle_colours[0],
										wrapped_triangle_colours[tri + 1],
										wrapped_triangle_colours[tri + 2],
										wrapped_triangle_lat_lon_points[0],
										wrapped_triangle_lat_lon_points[tri + 1],
										wrapped_triangle_lat_lon_points[tri + 2]));
					}
				}
			}
			else // Triangle does not need any wrapping...
			{
				refined_triangles_to_process.push(
						RefinedVertexColouredTriangle(
								triangle_vertex_points[0], triangle_vertex_points[1], triangle_vertex_points[2],
								triangle_vertex_colours[0], triangle_vertex_colours[1], triangle_vertex_colours[2]));
			}

			if (use_separate_filled_drawable)
			{
				filled_polygons.end_filled_triangle_mesh();
				filled_polygons.begin_filled_triangle_mesh();
			}

			// Recurse into triangle(s) and refine until reach GCA threshold.
			while (!refined_triangles_to_process.empty())
			{
				const RefinedVertexColouredTriangle refined_triangle = refined_triangles_to_process.top();
				refined_triangles_to_process.pop();

				// Find the longest edge of the current triangle.
				unsigned int longest_edge_index = 0;
				for (unsigned int e = 1; e < 3; ++e)
				{
					if (refined_triangle.edge_lengths[e]->is_precisely_greater_than(
						refined_triangle.edge_lengths[longest_edge_index].get()))
					{
						longest_edge_index = e;
					}
				}

				// Skip refinement of current triangle if its longest edge is under threshold and
				// just output current triangle instead.
				const GPlatesMaths::AngularDistance &longest_edge_length =
						refined_triangle.edge_lengths[longest_edge_index].get();
				if (longest_edge_length.is_precisely_less_than(GREAT_CIRCLE_ARC_ANGULAR_EXTENT_THRESHOLD))
				{
					if (use_wrapped_coordinates)
					{
						filled_polygons.add_filled_triangle_to_mesh(
								get_projected_wrapped_position(refined_triangle.vertex_lat_lon_points[0].get()),
								get_projected_wrapped_position(refined_triangle.vertex_lat_lon_points[1].get()),
								get_projected_wrapped_position(refined_triangle.vertex_lat_lon_points[2].get()),
								Colour::to_rgba8(refined_triangle.vertex_colours[0].get()),
								Colour::to_rgba8(refined_triangle.vertex_colours[1].get()),
								Colour::to_rgba8(refined_triangle.vertex_colours[2].get()));
					}
					else
					{
						filled_polygons.add_filled_triangle_to_mesh(
								get_projected_unwrapped_position(refined_triangle.vertex_points[0].get()),
								get_projected_unwrapped_position(refined_triangle.vertex_points[1].get()),
								get_projected_unwrapped_position(refined_triangle.vertex_points[2].get()),
								Colour::to_rgba8(refined_triangle.vertex_colours[0].get()),
								Colour::to_rgba8(refined_triangle.vertex_colours[1].get()),
								Colour::to_rgba8(refined_triangle.vertex_colours[2].get()));
					}

					continue;
				}

				// Refine the current triangle into two triangles by splitting the longest edge.
				const unsigned int post_longest_edge_index = (longest_edge_index + 1) % 3;
				const unsigned int pre_longest_edge_index = (longest_edge_index + 2) % 3;

				// Note: Edge endpoints won't be antipodal because they came from great circle arcs
				// (which cannot have antipodal endpoints), so 'get_normalisation()' shouldn't throw.
				// Also it's not possible for a triangle to pass through both the North and South poles
				// and so we won't get the situation where there's an (antipodal) arc from North to South
				// poles generated by the dateline wrapper.
				const GPlatesMaths::PointOnSphere edge_mid_point(
							(GPlatesMaths::Vector3D(refined_triangle.vertex_points[longest_edge_index]->position_vector()) +
							GPlatesMaths::Vector3D(refined_triangle.vertex_points[post_longest_edge_index]->position_vector()))
									.get_normalisation());

				const Colour edge_mid_colour = Colour::linearly_interpolate(
						refined_triangle.vertex_colours[longest_edge_index].get(),
						refined_triangle.vertex_colours[post_longest_edge_index].get(),
						0.5);

				if (use_wrapped_coordinates)
				{
					const double central_longitude = d_map_projection->central_meridian();

					GPlatesMaths::LatLonPoint edge_mid_lat_lon_point = make_lat_lon_point(edge_mid_point);

					// Vertices on the edge to split.
					const GPlatesMaths::LatLonPoint &edge_vertex_lat_lon0 =
							refined_triangle.vertex_lat_lon_points[longest_edge_index].get();
					const GPlatesMaths::LatLonPoint &edge_vertex_lat_lon1 =
							refined_triangle.vertex_lat_lon_points[post_longest_edge_index].get();

					// See if the split edge is on the dateline.
					if (GPlatesMaths::are_almost_exactly_equal(edge_vertex_lat_lon0.longitude(), edge_vertex_lat_lon1.longitude()) &&
						GPlatesMaths::are_almost_exactly_equal(std::fabs(edge_vertex_lat_lon0.longitude() - central_longitude), 180.0))
					{
						// The edge midpoint has not been wrapped (dateline wrapped) and hence could end up with
						// -180 or +180 for the longitude (due to numerical precision). So we must make sure its
						// wrapping matches the edge end points (both should be on the same side of the dateline,
						// ie -180 or 180, since they have been wrapped - so we can arbitrarily pick one).
						edge_mid_lat_lon_point = GPlatesMaths::LatLonPoint(
								edge_mid_lat_lon_point.latitude(),
								edge_vertex_lat_lon0.longitude());
					}
					else
					{
						// The edge midpoint is not *on* the dateline and hence is relatively safe from wrapping problems.
						// Just make sure we keep the longitude in the range...
						//   [-180 + central_meridian, central_meridian + 180]
						// ...since we're converting from PointOnSphere to LatLonPoint (ie, [-180, 180] range).
						// Note: 'central_longitude' should be in the range [-180, 180] itself.
						if (edge_mid_lat_lon_point.longitude() < -180 + central_longitude)
						{
							edge_mid_lat_lon_point = GPlatesMaths::LatLonPoint(
									edge_mid_lat_lon_point.latitude(),
									edge_mid_lat_lon_point.longitude() + 360);
						}
						else if (edge_mid_lat_lon_point.longitude() > central_longitude + 180)
						{
							edge_mid_lat_lon_point = GPlatesMaths::LatLonPoint(
									edge_mid_lat_lon_point.latitude(),
									edge_mid_lat_lon_point.longitude() - 360);
						}
					}

					// Push the two split triangles including lat/lon coordinates (to retain correct dateline longitude).
					refined_triangles_to_process.push(
							RefinedVertexColouredTriangle(
									refined_triangle.vertex_points[longest_edge_index].get(),
									edge_mid_point,
									refined_triangle.vertex_points[pre_longest_edge_index].get(),
									refined_triangle.vertex_colours[longest_edge_index].get(),
									edge_mid_colour,
									refined_triangle.vertex_colours[pre_longest_edge_index].get(),
									refined_triangle.vertex_lat_lon_points[longest_edge_index].get(),
									edge_mid_lat_lon_point,
									refined_triangle.vertex_lat_lon_points[pre_longest_edge_index].get()));
					refined_triangles_to_process.push(
							RefinedVertexColouredTriangle(
									edge_mid_point,
									refined_triangle.vertex_points[post_longest_edge_index].get(),
									refined_triangle.vertex_points[pre_longest_edge_index].get(),
									edge_mid_colour,
									refined_triangle.vertex_colours[post_longest_edge_index].get(),
									refined_triangle.vertex_colours[pre_longest_edge_index].get(),
									edge_mid_lat_lon_point,
									refined_triangle.vertex_lat_lon_points[post_longest_edge_index].get(),
									refined_triangle.vertex_lat_lon_points[pre_longest_edge_index].get()));
				}
				else // unwrapped coordinates ...
				{
					// Push the two split triangles (ignoring lat/lon coordinates - only needed when wrapping).
					refined_triangles_to_process.push(
							RefinedVertexColouredTriangle(
									refined_triangle.vertex_points[longest_edge_index].get(),
									edge_mid_point,
									refined_triangle.vertex_points[pre_longest_edge_index].get(),
									refined_triangle.vertex_colours[longest_edge_index].get(),
									edge_mid_colour,
									refined_triangle.vertex_colours[pre_longest_edge_index].get()));
					refined_triangles_to_process.push(
							RefinedVertexColouredTriangle(
									edge_mid_point,
									refined_triangle.vertex_points[post_longest_edge_index].get(),
									refined_triangle.vertex_points[pre_longest_edge_index].get(),
									edge_mid_colour,
									refined_triangle.vertex_colours[post_longest_edge_index].get(),
									refined_triangle.vertex_colours[pre_longest_edge_index].get()));
				}
			}

			if (use_separate_filled_drawable)
			{
				filled_polygons.end_filled_triangle_mesh();
				filled_polygons.begin_filled_triangle_mesh();
			}
		}
	}
	else // triangle colouring ...
	{
		// Iterate over the mesh triangles.
		const unsigned int num_mesh_triangles = mesh_triangles.size();
		for (unsigned int t = 0; t < num_mesh_triangles; ++t)
		{
			const mesh_type::Triangle &mesh_triangle = mesh_triangles[t];

			boost::optional<Colour> colour = get_vector_geometry_colour(mesh_colours[t]);
			if (!colour)
			{
				continue;
			}

			// Modulate with the fill modulate colour.
			const Colour fill_colour = Colour::modulate(
					colour.get(),
					rendered_coloured_triangle_surface_mesh.get_fill_modulate_colour());

			// Convert colour from floats to bytes to use less vertex memory.
			const rgba8_t rgba8_fill_colour = Colour::to_rgba8(fill_colour);

			// Create a PolygonOnSphere for the current triangle so we can pass it through the
			// dateline wrapping and projection code.
			const GPlatesMaths::PointOnSphere triangle_points[3] =
			{
				mesh_vertices[mesh_triangle.vertex_indices[0]],
				mesh_vertices[mesh_triangle.vertex_indices[1]],
				mesh_vertices[mesh_triangle.vertex_indices[2]]
			};
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type triangle_polygon =
					GPlatesMaths::PolygonOnSphere::create(
							triangle_points,
							triangle_points + 3);

			DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_triangle;
			dateline_wrap_and_project_line_geometry(
					dateline_wrapped_projected_triangle,
					triangle_polygon);

			const std::vector<unsigned int> &geometries = dateline_wrapped_projected_triangle.get_geometries();
			const unsigned int num_geometries = geometries.size();
			if (num_geometries == 0)
			{
				// Continue to the next triangle if there's nothing to paint - shouldn't really be able to get here.
				continue;
			}

			unsigned int geometry_part_index = 0;
			const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_triangle.get_geometry_parts();

			unsigned int projected_vertex_index = 0;
			const std::vector<QPointF> &projected_vertices = dateline_wrapped_projected_triangle.get_vertices();

			// Iterate over the dateline wrapped geometries.
			for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
			{
				// Iterate over the parts of the current geometry (there will be only one part/ring though).
				const unsigned int end_geometry_part_index = geometries[geometry_index];
				for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
				{
					std::vector<QPointF> filled_triangle_geometry;

					// Iterate over the vertices of the current geometry part.
					const unsigned int end_projected_vertex_index = geometry_parts[geometry_part_index];
					for ( ; projected_vertex_index < end_projected_vertex_index; ++projected_vertex_index)
					{
						filled_triangle_geometry.push_back(projected_vertices[projected_vertex_index]);
					}

					// If the dateline wrapped triangle remains a triangle (ie, same triangle as before dateline wrapping
					// or a wrapped piece of original triangle that happens to be a triangle) then we know it's convex
					// in shape and hence doesn't need to be rendered as a separate drawable so we add it to the
					// current triangle mesh drawable since it results in faster rendering.
					//
					// Otherwise either the original triangle was tessellated and/or dateline wrapped.
					// If it was tessellated then it could have a curved edge in the map projection making
					// it potentially concave shaped and hence requiring it to be rendered in a separate drawable
					// (to ensure it gets filled, with its separate colour, correctly).
					// If it was dateline wrapped but not tessellated then, in all current map projections it will end
					// up convex but it's harder to determine this (wrapped but not tessellated) and happens less frequently
					// so we just lump it into a separate drawable anyway.
					//
					// This means the fine-grained areas if meshes won't need tessellation and, if not wrapped,
					// can then be grouped into fewer drawables for rendering efficiency. Whereas low-resolution
					// areas of meshes will use more drawables but there's fewer required since less dense.
					//
					// We test for 4 vertices instead of 3 for a triangle because 'dateline_wrap_and_project_line_geometry()',
					// for a polygon, ensures the last point duplicates the first point (to close off ring).
					if (filled_triangle_geometry.size() == 4)
					{
						filled_polygons.add_filled_triangle_to_mesh(
								filled_triangle_geometry[0],
								filled_triangle_geometry[1],
								filled_triangle_geometry[2],
								rgba8_fill_colour);
					}
					else
					{
						// End the current mesh drawable.
						filled_polygons.end_filled_triangle_mesh();

						// Add the filled polygon geometry.
						filled_polygons.add_filled_polygon(filled_triangle_geometry, rgba8_fill_colour);

						// Start a new mesh drawable.
						filled_polygons.begin_filled_triangle_mesh();
					}
				}
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
					get_vector_geometry_colour(rendered_string.get_colour()),
					get_vector_geometry_colour(rendered_string.get_shadow_colour())));
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_small_circle.get_colour());
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	// Tessellate the small circle.
	std::vector<GPlatesMaths::PointOnSphere> points;
	tessellate(points, rendered_small_circle.get_small_circle(), SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Create a closed polyline loop from the tessellated points.
	points.push_back(points.front());
	// NOTE: We don't create a polygon because if a polygon crosses the central meridian it gets
	// rendered as multiple polygons and for a small circle this could look confusing.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type small_circle_arc_polyline =
			GPlatesMaths::PolylineOnSphere::create(points);

	const float line_width = rendered_small_circle.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Render the small circle tessellated as a closed polyline.
	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(small_circle_arc_polyline, rgba8_color, lines_stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_small_circle_arc(
		const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_small_circle_arc.get_colour());
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	// Tessellate the small circle arc.
	std::vector<GPlatesMaths::PointOnSphere> points;
	tessellate(points, rendered_small_circle_arc.get_small_circle_arc(), SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Create a polyline from the tessellated points.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type small_circle_arc_polyline =
			GPlatesMaths::PolylineOnSphere::create(points);

	const float line_width = rendered_small_circle_arc.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Render the small circle arc tessellated as a polyline.
	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(small_circle_arc_polyline, rgba8_color, lines_stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_ellipse.get_colour());
	if (!colour)
	{
		return;
	}

	if (rendered_ellipse.get_semi_major_axis_radians() == 0 ||
		rendered_ellipse.get_semi_minor_axis_radians() == 0)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

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
			GPlatesMaths::PolylineOnSphere::create(points);

	const float line_width = rendered_ellipse.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Render the ellipse tessellated as a closed polyline.
	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(ellipse_polyline, rgba8_color, lines_stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_radial_arrow(
		const GPlatesViewOperations::RenderedRadialArrow &rendered_radial_arrow)
{
	// We don't render the radial arrow in the map view (it's radial and hence always pointing
	// directly out of the map). We only render the symbol.

    boost::optional<Colour> symbol_colour = get_vector_geometry_colour(rendered_radial_arrow.get_symbol_colour());
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
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_tangential_arrow.get_colour());
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

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
			GPlatesMaths::PolylineOnSphere::create(arrow_end_points, arrow_end_points + 2);

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(arrow, rgba8_color, line_stream, arrowhead_size);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_arrowed_polyline(
	const GPlatesViewOperations::RenderedArrowedPolyline &rendered_arrowed_polyline)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_arrowed_polyline.get_colour());
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline = rendered_arrowed_polyline.get_polyline_on_sphere();

	double arrowhead_size = d_device_independent_pixel_to_map_space_ratio * d_scale * rendered_arrowed_polyline.get_arrowhead_size_in_pixels();
	if (arrowhead_size > MAX_ARROWED_POLYLINE_ARROWHEAD_SIZE)
	{
		arrowhead_size = MAX_ARROWED_POLYLINE_ARROWHEAD_SIZE;
	}

	const float line_width = rendered_arrowed_polyline.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;
	stream_primitives_type &lines_stream = d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(polyline, rgba8_color, lines_stream, arrowhead_size);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_triangle_symbol(
	const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol)
{
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_triangle_symbol.get_colour());
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

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(*colour);

	coloured_vertex_type va(pa.x(), pa.y(), 0, rgba8_color);
	coloured_vertex_type vb(pb.x(), pb.y(), 0, rgba8_color);
	coloured_vertex_type vc(pc.x(), pc.y(), 0, rgba8_color);

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
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_square_symbol.get_colour());
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

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(*colour);

	coloured_vertex_type va(pa.x(), pa.y(),0, rgba8_color);
	coloured_vertex_type vb(pb.x(), pb.y(),0, rgba8_color);
	coloured_vertex_type vc(pc.x(), pc.y(),0, rgba8_color);
	coloured_vertex_type vd(pd.x(), pd.y(),0, rgba8_color);
	coloured_vertex_type ve(pe.x(), pe.y(),0, rgba8_color);

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
	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_circle_symbol.get_colour());
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

	boost::optional<Colour> colour = get_vector_geometry_colour(rendered_cross_symbol.get_colour());
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
		boost::bind(
				&GPlatesViewOperations::RenderedGeometry::accept_visitor,
				boost::placeholders::_1,
				boost::ref(*this)));
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::dateline_wrap_and_project_line_geometry(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline_on_sphere)
{
	if (!d_dateline_wrapper->possibly_wraps(polyline_on_sphere))
	{
		// The polyline does not need any wrapping so we can just project it without wrapping.
		//
		// This avoids converting to lat/lon (in dateline wrapper) then converting to x/y/z
		// (to tessellate line segments) and then converting back to lat/lon prior to projection.
		// Instead, unwrapped polylines can just be tessellated and then converted to lat/lon,
		// saving expensive x/y/z <-> lat/lon conversions.
		project_and_tessellate_unwrapped_polyline(
				dateline_wrapped_projected_line_geometry,
				polyline_on_sphere);

		return;
	}

	// Wrap the rotated geometry to the longitude range...
	//   [-180 + central_meridian, central_meridian + 180]
	//
	// The dateline wrapper also tessellates the wrapped geometry.
	std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline> wrapped_polylines;
	d_dateline_wrapper->wrap_polyline(
			polyline_on_sphere,
			wrapped_polylines,
			GREAT_CIRCLE_ARC_ANGULAR_EXTENT_THRESHOLD);

	// Paint each wrapped piece of the original geometry.
	BOOST_FOREACH(
			const GPlatesMaths::DateLineWrapper::LatLonPolyline &wrapped_polyline,
			wrapped_polylines)
	{
		project_tessellated_wrapped_polyline(
				dateline_wrapped_projected_line_geometry,
				wrapped_polyline);
	}
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::dateline_wrap_and_project_line_geometry(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon_on_sphere)
{
	if (!d_dateline_wrapper->possibly_wraps(polygon_on_sphere))
	{
		// The polygon does not need any wrapping so we can just project it without wrapping.
		//
		// This avoids converting to lat/lon (in dateline wrapper) then converting to x/y/z
		// (to tessellate line segments) and then converting back to lat/lon prior to projection.
		// Instead, unwrapped polygons can just be tessellated and then converted to lat/lon,
		// saving expensive x/y/z <-> lat/lon conversions.
		project_and_tessellate_unwrapped_polygon(
				dateline_wrapped_projected_line_geometry,
				polygon_on_sphere);

		return;
	}

	// Wrap the rotated geometry to the longitude range...
	//   [-180 + central_meridian, central_meridian + 180]
	//
	// The dateline wrapper also tessellates the wrapped geometry.
	std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon> wrapped_polygons;
	d_dateline_wrapper->wrap_polygon(
			polygon_on_sphere,
			wrapped_polygons,
			GREAT_CIRCLE_ARC_ANGULAR_EXTENT_THRESHOLD,
			// This can noticeably speed up rendering of some complex polygons containing lots of
			// interior rings, where some of the polygon's rings intersect dateline but many don't,
			// since it avoids attempting to group interior rings with exteriors.
			// 
			// It doesn't matter if all the rings end up in separate polygons as exterior rings because
			// the line rendering of rings is the same regardless of whether they are exterior or interior,
			// and for filled rendering we just take all the rings regardless of whether they are
			// exterior or interior rings and render them the same way (inverting pixel's stencil buffer
			// value each time pixel drawn takes care of correctly masking out polygon holes and intersections).
			false/*group_interior_with_exterior_rings*/);

	// Paint each wrapped piece of the original geometry.
	BOOST_FOREACH(
			GPlatesMaths::DateLineWrapper::LatLonPolygon &wrapped_polygon,
			wrapped_polygons)
	{
		project_tessellated_wrapped_polygon(
				dateline_wrapped_projected_line_geometry,
				wrapped_polygon);
	}
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::project_tessellated_wrapped_polyline(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::DateLineWrapper::LatLonPolyline &wrapped_polyline)
{
	const GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type &points = wrapped_polyline.get_points();

	std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline::point_flags_type> point_flags;
	wrapped_polyline.get_point_flags(point_flags);

	GPlatesMaths::DateLineWrapper::LatLonPolyline::interpolate_original_segment_seq_type interpolate_original_segments;
	wrapped_polyline.get_interpolate_original_segments(interpolate_original_segments);

	// Iterate over the geometry points.
	const unsigned int num_lat_lon_points = points.size();
	for (unsigned int lat_lon_point_index = 0; lat_lon_point_index < num_lat_lon_points; ++lat_lon_point_index)
	{
		const GPlatesMaths::DateLineWrapper::LatLonPolyline::InterpolateOriginalSegment &interpolate_original_segment =
				interpolate_original_segments[lat_lon_point_index];

		dateline_wrapped_projected_line_geometry.add_vertex(
				get_projected_wrapped_position(points[lat_lon_point_index]),
				point_flags[lat_lon_point_index].test(GPlatesMaths::DateLineWrapper::LatLonPolyline::ORIGINAL_POINT),
				point_flags[lat_lon_point_index].test(GPlatesMaths::DateLineWrapper::LatLonPolyline::ON_DATELINE),
				DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment(
						interpolate_original_segment.interpolate_ratio,
						interpolate_original_segment.original_segment_index));
	}

	dateline_wrapped_projected_line_geometry.add_geometry_part();
	dateline_wrapped_projected_line_geometry.add_geometry();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::project_tessellated_wrapped_polygon(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::DateLineWrapper::LatLonPolygon &wrapped_polygon)
{
	// Polygon's exterior ring.
	std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type> exterior_ring_point_flags;
	wrapped_polygon.get_exterior_ring_point_flags(exterior_ring_point_flags);
	GPlatesMaths::DateLineWrapper::LatLonPolygon::interpolate_original_segment_seq_type exterior_ring_interpolate_original_segments;
	wrapped_polygon.get_exterior_ring_interpolate_original_segments(exterior_ring_interpolate_original_segments);
	project_tessellated_wrapped_ring(
			dateline_wrapped_projected_line_geometry,
			wrapped_polygon.get_exterior_ring_points(),
			exterior_ring_point_flags,
			exterior_ring_interpolate_original_segments);

	// Polygon's interior rings.
	const unsigned int num_interior_rings = wrapped_polygon.get_num_interior_rings();
	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
	{
		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type> interior_ring_point_flags;
		wrapped_polygon.get_interior_ring_point_flags(interior_ring_point_flags, interior_ring_index);
		GPlatesMaths::DateLineWrapper::LatLonPolygon::interpolate_original_segment_seq_type interior_ring_interpolate_original_segments;
		wrapped_polygon.get_interior_ring_interpolate_original_segments(interior_ring_interpolate_original_segments, interior_ring_index);
		project_tessellated_wrapped_ring(
				dateline_wrapped_projected_line_geometry,
				wrapped_polygon.get_interior_ring_points(interior_ring_index),
				interior_ring_point_flags,
				interior_ring_interpolate_original_segments);
	}

	dateline_wrapped_projected_line_geometry.add_geometry();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::project_tessellated_wrapped_ring(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type &lat_lon_points,
		const std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type> &point_flags,
		const GPlatesMaths::DateLineWrapper::LatLonPolygon::interpolate_original_segment_seq_type &interpolate_original_segments)
{
	// Iterate over the line geometry points.
	const unsigned int num_lat_lon_points = lat_lon_points.size();
	for (unsigned int lat_lon_point_index = 0; lat_lon_point_index < num_lat_lon_points; ++lat_lon_point_index)
	{
		const boost::optional<GPlatesMaths::DateLineWrapper::LatLonPolygon::InterpolateOriginalSegment> &
				interpolate_original_segment = interpolate_original_segments[lat_lon_point_index];
		if (interpolate_original_segment)
		{
			dateline_wrapped_projected_line_geometry.add_vertex(
					get_projected_wrapped_position(lat_lon_points[lat_lon_point_index]),
					point_flags[lat_lon_point_index].test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ORIGINAL_POINT),
					point_flags[lat_lon_point_index].test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ON_DATELINE),
					DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment(
							interpolate_original_segment->interpolate_ratio,
							interpolate_original_segment->original_segment_index,
							interpolate_original_segment->original_ring_index));
		}
		else
		{
			dateline_wrapped_projected_line_geometry.add_vertex(
					get_projected_wrapped_position(lat_lon_points[lat_lon_point_index]),
					point_flags[lat_lon_point_index].test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ORIGINAL_POINT),
					point_flags[lat_lon_point_index].test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ON_DATELINE));
		}
	}

	//
	// It's a wrapped polygon (not a polyline) so add the start point to the end in order
	// to close the loop - we need to do this because we're iterating over vertices not arcs.
	//
	const boost::optional<GPlatesMaths::DateLineWrapper::LatLonPolygon::InterpolateOriginalSegment> &
			end_interpolate_original_segment = interpolate_original_segments.front();
	if (end_interpolate_original_segment)
	{
		dateline_wrapped_projected_line_geometry.add_vertex(
				get_projected_wrapped_position(lat_lon_points.front()),
				point_flags.front().test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ORIGINAL_POINT),
				point_flags.front().test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ON_DATELINE),
				DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment(
						end_interpolate_original_segment->interpolate_ratio,
						end_interpolate_original_segment->original_segment_index,
						end_interpolate_original_segment->original_ring_index));
	}
	else
	{
		dateline_wrapped_projected_line_geometry.add_vertex(
				get_projected_wrapped_position(lat_lon_points.front()),
				point_flags.front().test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ORIGINAL_POINT),
				point_flags.front().test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ON_DATELINE));
	}

	dateline_wrapped_projected_line_geometry.add_geometry_part();
}


template <typename GreatCircleArcForwardIter>
void
GPlatesGui::MapRenderedGeometryLayerPainter::project_and_tessellate_unwrapped_geometry_part(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GreatCircleArcForwardIter &begin_arcs,
		const GreatCircleArcForwardIter &end_arcs,
		unsigned int geometry_part_index)
{
	// Add the first vertex of the sequence of great circle arcs.
	// Keep track of the last projected point to calculate arrow head tangent direction.
	dateline_wrapped_projected_line_geometry.add_vertex(
			get_projected_unwrapped_position(begin_arcs->start_point()),
			true/*is_original_point*/,
			false/*on_dateline*/,
			DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment(
					0.0/*interpolate_ratio*/,
					0/*original_segment_index*/,
					geometry_part_index/*original_geometry_part_index*/));

	// Iterate over the great circle arcs.
	unsigned int gca_index = 0;
	for (GreatCircleArcForwardIter gca_iter = begin_arcs ; gca_iter != end_arcs; ++gca_iter, ++gca_index)
	{
		const GPlatesMaths::GreatCircleArc &gca = *gca_iter;

		// Tessellate the current arc if its two endpoints are far enough apart.
		if (gca.dot_of_endpoints() < COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD)
		{
			// Tessellate the current great circle arc.
			std::vector<GPlatesMaths::PointOnSphere> tess_points;
			tessellate(tess_points, gca, GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

			// Add the tessellated points skipping the first since it was added by the previous GCA.
			// We also skip the last since it gets added by the current GCA.
			const unsigned int num_tessellated_segments = tess_points.size() - 1;
			const double inv_num_tessellated_segments = 1.0 / num_tessellated_segments;
			for (unsigned int n = 1; n < num_tessellated_segments; ++n)
			{
				// Keep track of the last projected point to calculate arrow head tangent direction.
				dateline_wrapped_projected_line_geometry.add_vertex(
						get_projected_unwrapped_position(tess_points[n]),
						false/*is_original_point*/,
						false/*on_dateline*/,
						DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment(
								n * inv_num_tessellated_segments/*interpolate_ratio*/,
								gca_index/*original_segment_index*/,
								geometry_part_index/*original_geometry_part_index*/));
			}
		}

		// Vertex representing the end point's position and colour.
		dateline_wrapped_projected_line_geometry.add_vertex(
				get_projected_unwrapped_position(gca.end_point()),
				true/*is_original_point*/,
				false/*on_dateline*/,
				DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment(
						1.0/*interpolate_ratio*/,
						gca_index/*original_segment_index*/,
						geometry_part_index/*original_geometry_part_index*/));
	}

	dateline_wrapped_projected_line_geometry.add_geometry_part();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::project_and_tessellate_unwrapped_polyline(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline_on_sphere)
{
	project_and_tessellate_unwrapped_geometry_part(
			dateline_wrapped_projected_line_geometry,
			polyline_on_sphere->begin(),
			polyline_on_sphere->end());

	dateline_wrapped_projected_line_geometry.add_geometry();
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::project_and_tessellate_unwrapped_polygon(
		DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon_on_sphere)
{
	// Polygon's exterior ring.
	project_and_tessellate_unwrapped_geometry_part(
			dateline_wrapped_projected_line_geometry,
			polygon_on_sphere->exterior_ring_begin(),
			polygon_on_sphere->exterior_ring_end(),
			0/*geometry_part_index*/);

	// Polygon's interior rings.
	const unsigned int num_interior_rings = polygon_on_sphere->number_of_interior_rings();
	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
	{
		project_and_tessellate_unwrapped_geometry_part(
				dateline_wrapped_projected_line_geometry,
				polygon_on_sphere->interior_ring_begin(interior_ring_index),
				polygon_on_sphere->interior_ring_end(interior_ring_index),
				interior_ring_index + 1/*geometry_part_index*/);
	}

	dateline_wrapped_projected_line_geometry.add_geometry();
}


template <typename LineGeometryType>
void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_fill_geometry(
		GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons,
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
		rgba8_t rgba8_color)
{
	// Note: We always dateline-wrap a polygon even if the line geometry is a polyline.
	// This is because the geometry is filled and only a polygon is wrapped correctly for filling.
	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_line_geometry;
	dateline_wrap_and_project_line_geometry(
			dateline_wrapped_projected_line_geometry,
			GPlatesAppLogic::GeometryUtils::force_convert_geometry_to_polygon(*line_geometry));

	const std::vector<unsigned int> &geometries =
			dateline_wrapped_projected_line_geometry.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	unsigned int geometry_part_index = 0;
	const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_line_geometry.get_geometry_parts();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices = dateline_wrapped_projected_line_geometry.get_vertices();

	// Even though the filled polyline/polygon might have been dateline wrapped into multiple
	// geometries (each with potentially multiple parts/rings) we still render them all together
	// in one filled polygon drawable so that they can invert each other where they intersect.
	// They are, after all, coming from a single input polyline/polygon.
	// This also puts less pressure on the dateline wrapper to correctly assign polygon unclipped
	// interior rings to the correct clipped exterior ring for example.
	//
	// TODO: Should probably convert self-intersecting polygons to non-self-intersecting parts
	// before passing to dateline wrapper - although that might slow us down.
	std::vector< std::vector<QPointF> > filled_polygon;
	filled_polygon.reserve(geometry_parts.size());

	// Iterate over the dateline wrapped geometries.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		// Iterate over the parts of the current geometry.
		const unsigned int end_geometry_part_index = geometries[geometry_index];
		for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
		{
			// Add a new ring to the polygon.
			filled_polygon.push_back(std::vector<QPointF>());
			std::vector<QPointF> &filled_polygon_ring = filled_polygon.back();

			// Iterate over the vertices of the current geometry part.
			const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
			for ( ; vertex_index < end_vertex_index; ++vertex_index)
			{
				filled_polygon_ring.push_back(vertices[vertex_index]);
			}
		}
	}

	// Add the current filled polygon geometry.
	filled_polygons.add_filled_polygon(filled_polygon, rgba8_color);
}


template <typename LineGeometryType>
void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_line_geometry(
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
		rgba8_t rgba8_color,
		stream_primitives_type &lines_stream,
		boost::optional<double> arrow_head_size)
{
	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_line_geometry;
	dateline_wrap_and_project_line_geometry(
			dateline_wrapped_projected_line_geometry,
			line_geometry);

	const std::vector<unsigned int> &geometries = dateline_wrapped_projected_line_geometry.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	unsigned int geometry_part_index = 0;
	const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_line_geometry.get_geometry_parts();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices =
			dateline_wrapped_projected_line_geometry.get_vertices();
	const std::vector<bool> &is_original_point_flags =
			dateline_wrapped_projected_line_geometry.get_is_original_point_flags();

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	// Iterate over the dateline wrapped geometries.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		// Iterate over the parts of the current geometry (either a polyline or a ring of a polygon).
		const unsigned int end_geometry_part_index = geometries[geometry_index];
		for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
		{
			stream_line_strips.begin_line_strip();

			// Iterate over the vertices of the current geometry part.
			const unsigned int start_vertex_index = vertex_index;
			const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
			for ( ; vertex_index < end_vertex_index; ++vertex_index)
			{
				const QPointF &vertex = vertices[vertex_index];
				const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, rgba8_color);
				stream_line_strips.add_vertex(coloured_vertex);

				// If we're painting arrow heads they are only painted at the end points of
				// the original (un-dateline-wrapped and untessellated) arcs.
				if (arrow_head_size &&
					is_original_point_flags[vertex_index] &&
					vertex_index != start_vertex_index)
				{
					paint_arrow_head(
							vertex,
							// Our best estimate of the arrow direction tangent at the GCA end point...
							vertex - vertices[vertex_index - 1],
							arrow_head_size.get(),
							rgba8_color);
				}
			}

			stream_line_strips.end_line_strip();
		}
	}
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_vertex_coloured_polyline(
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline,
		const std::vector<Colour> &original_vertex_colours,
		stream_primitives_type &lines_stream)
{
	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_polyline;
	dateline_wrap_and_project_line_geometry(
			dateline_wrapped_projected_polyline,
			polyline);

	const std::vector<unsigned int> &geometries = dateline_wrapped_projected_polyline.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	unsigned int geometry_part_index = 0;
	const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_polyline.get_geometry_parts();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices = dateline_wrapped_projected_polyline.get_vertices();
	const DatelineWrappedProjectedLineGeometry::interpolate_original_segment_seq_type &interpolate_original_segments =
			dateline_wrapped_projected_polyline.get_interpolate_original_segments();

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	// Iterate over the dateline wrapped polylines.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		// Iterate over the parts of the current geometry (polylines will only have one though).
		const unsigned int end_geometry_part_index = geometries[geometry_index];
		for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
		{
			stream_line_strips.begin_line_strip();

			// Iterate over the vertices of the current geometry part.
			const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
			for ( ; vertex_index < end_vertex_index; ++vertex_index)
			{
				// This should always be valid for polylines (ie, should never be none).
				const boost::optional<DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment> &
						interpolate_original_segment = interpolate_original_segments[vertex_index];
				if (interpolate_original_segment)
				{
					const unsigned int original_segment_start_vertex_index = interpolate_original_segment->original_segment_index;
					// No vertex wraparound needed for polylines (only needed for polygon rings).
					unsigned int original_segment_end_vertex_index = original_segment_start_vertex_index + 1;

					const Colour vertex_colour = Colour::linearly_interpolate(
							original_vertex_colours[original_segment_start_vertex_index],
							original_vertex_colours[original_segment_end_vertex_index],
							interpolate_original_segment->interpolate_ratio);

					const QPointF &vertex = vertices[vertex_index];
					const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, Colour::to_rgba8(vertex_colour));
					stream_line_strips.add_vertex(coloured_vertex);
				}
			}

			stream_line_strips.end_line_strip();
		}
	}
}


void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_vertex_coloured_polygon(
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon,
		const std::vector<Colour> &original_vertex_colours,
		stream_primitives_type &lines_stream)
{
	DatelineWrappedProjectedLineGeometry dateline_wrapped_projected_polygon;
	dateline_wrap_and_project_line_geometry(
			dateline_wrapped_projected_polygon,
			polygon);

	const std::vector<unsigned int> &geometries = dateline_wrapped_projected_polygon.get_geometries();
	const unsigned int num_geometries = geometries.size();
	if (num_geometries == 0)
	{
		// Return early if there's nothing to paint - shouldn't really be able to get here.
		return;
	}

	const unsigned int num_original_vertices = original_vertex_colours.size();

	unsigned int geometry_part_index = 0;
	const std::vector<unsigned int> &geometry_parts = dateline_wrapped_projected_polygon.get_geometry_parts();

	unsigned int vertex_index = 0;
	const std::vector<QPointF> &vertices = dateline_wrapped_projected_polygon.get_vertices();
	const std::vector<bool> &is_on_dateline_flags = dateline_wrapped_projected_polygon.get_is_on_dateline_flags();
	const DatelineWrappedProjectedLineGeometry::interpolate_original_segment_seq_type &interpolate_original_segments =
			dateline_wrapped_projected_polygon.get_interpolate_original_segments();

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	// Iterate over the dateline wrapped geometries.
	for (unsigned int geometry_index = 0; geometry_index < num_geometries; ++geometry_index)
	{
		// Iterate over the parts of the current geometry (either a polyline or a ring of a polygon).
		const unsigned int end_geometry_part_index = geometries[geometry_index];
		for ( ; geometry_part_index < end_geometry_part_index; ++geometry_part_index)
		{
			stream_line_strips.begin_line_strip();

			bool last_emitted_vertex_on_dateline = false;

			// Iterate over the vertices of the current geometry part.
			const unsigned int end_vertex_index = geometry_parts[geometry_part_index];
			for ( ; vertex_index < end_vertex_index; ++vertex_index)
			{
				// This can be none for polygon ring vertices tessellated along dateline.
				const boost::optional<DatelineWrappedProjectedLineGeometry::InterpolateOriginalSegment> &
						interpolate_original_segment = interpolate_original_segments[vertex_index];
				if (!interpolate_original_segment ||
					// Skip *interior* polygon rings because currently only *exterior* rings have scalar values.
					// TODO: Add scalar values for interior rings also.
					interpolate_original_segment->original_geometry_part_index != 0)
				{
					continue;
				}

				// Avoid drawing segments along the dateline since these segments are not part of an
				// original polygon ring since the dateline segments are there just to close the wrapped ring.
				//
				// TODO: Actually it's possible some original ring segments coincide with the dateline
				// in which case we should draw them (most often they won't though and the above case will apply).
				if (last_emitted_vertex_on_dateline &&
					is_on_dateline_flags[vertex_index])
				{
					// End current line strip - if previous line strip has 0 or 1 vertex then no lines emitted.
					stream_line_strips.end_line_strip();
					// Start a new line strip.
					stream_line_strips.begin_line_strip();
				}

				const unsigned int original_segment_start_vertex_index = interpolate_original_segment->original_segment_index;
				unsigned int original_segment_end_vertex_index = original_segment_start_vertex_index + 1;
				// Handle wrap-around to the start vertex (vertex on last segment maps to first vertex).
				if (original_segment_end_vertex_index >= num_original_vertices)
				{
					original_segment_end_vertex_index -= num_original_vertices;
				}

				const Colour vertex_colour = Colour::linearly_interpolate(
						original_vertex_colours[original_segment_start_vertex_index],
						original_vertex_colours[original_segment_end_vertex_index],
						interpolate_original_segment->interpolate_ratio);

				const QPointF &vertex = vertices[vertex_index];
				const coloured_vertex_type coloured_vertex(vertex.x(), vertex.y(), 0/*z*/, Colour::to_rgba8(vertex_colour));
				stream_line_strips.add_vertex(coloured_vertex);
				last_emitted_vertex_on_dateline = is_on_dateline_flags[vertex_index];
			}

			stream_line_strips.end_line_strip();
		}
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
	const double central_longitude = d_map_projection->central_meridian();

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
