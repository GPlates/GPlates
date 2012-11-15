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
#include <QPolygonF>

#include "Colour.h"
#include "ColourScheme.h"
#include "MapRenderedGeometryLayerPainter.h"
#include "MapProjection.h"

#include "maths/EllipseGenerator.h"
#include "maths/GreatCircle.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineIntersections.h"
#include "maths/PolylineOnSphere.h"
#include "maths/Rotation.h"

#include "opengl/GLRenderer.h"

#include "view-operations/RenderedArrowedPolyline.h"
#include "view-operations/RenderedCrossSymbol.h"
#include "view-operations/RenderedDirectionArrow.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedGeometryFactory.h" // include this while triangle and squares need to create a point-on-sphere RenderedGeom.
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedResolvedRaster.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"
#include "view-operations/RenderedSquareSymbol.h"
#include "view-operations/RenderedCircleSymbol.h"
#include "view-operations/RenderedString.h"
#include "view-operations/RenderedTriangleSymbol.h"


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
}


const float GPlatesGui::MapRenderedGeometryLayerPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::MapRenderedGeometryLayerPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;


GPlatesGui::MapRenderedGeometryLayerPainter::MapRenderedGeometryLayerPainter(
		const MapProjection::non_null_ptr_to_const_type &map_projection,
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		const double &inverse_viewport_zoom_factor,
		RenderSettings &render_settings,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_map_projection(map_projection),
	d_rendered_geometry_layer(rendered_geometry_layer),
	d_gl_visual_layers(gl_visual_layers),
	d_inverse_zoom_factor(inverse_viewport_zoom_factor),
	d_render_settings(render_settings),
	d_text_renderer_ptr(text_renderer_ptr),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f),
	d_dateline_wrapper(GPlatesMaths::DateLineWrapper::create()),
	// Rotates, about north pole, to move central meridian longitude to zero longitude...
	d_central_meridian_reference_frame_rotation(
			GPlatesMaths::Rotation::create(
					GPlatesMaths::UnitVector3D::zBasis()/*north pole*/,
					GPlatesMaths::convert_deg_to_rad(-map_projection->central_llp().longitude())))
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
	const cache_handle_type layer_cache =
			layer_painter.end_painting(renderer, *d_text_renderer_ptr, d_scale);

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

#if 0 // TODO: Implement filled polygons in map view.
	if (rendered_polygon_on_sphere.get_is_filled())
	{
		GPlatesOpenGL::GLMultiResolutionFilledPolygons::filled_polygons_type &filled_polygons =
				d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons();

		// Add the filled polygon at the current location (if any) in the rendered geometries spatial partition.
		filled_polygons.add_filled_polygon(
				*polygon_on_sphere,
				Colour::to_rgba8(colour.get()),
				boost::none/*transform*/,
				d_current_cube_quad_tree_location);

		return;
	}
#endif

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

	const float line_width =
			rendered_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_polyline_on_sphere.get_polyline_on_sphere();

	paint_line_geometry<GPlatesMaths::PolylineOnSphere>(polyline_on_sphere, colour.get(), stream);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_resolved_raster(
		const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster)
{
	// Queue the raster primitive for painting.
	d_layer_painter->rasters.push_back(
			LayerPainter::RasterDrawable(
					rendered_resolved_raster.get_resolved_raster(),
					rendered_resolved_raster.get_raster_colour_palette(),
					rendered_resolved_raster.get_raster_modulate_colour()));
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (!d_render_settings.show_strings())
	{
		return;
	}

	if (d_text_renderer_ptr)
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
						get_colour_of_rendered_geometry(rendered_string),
						rendered_string.get_shadow_colour().get_colour(d_colour_scheme)));
	}
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
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_direction_arrow(
	const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow)
{
	if (!d_render_settings.show_arrows())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_direction_arrow);
	if (!colour)
	{
		return;
	}

	// Start of arrow.
	const GPlatesMaths::UnitVector3D &start = rendered_direction_arrow.get_start_position().position_vector();

	// Calculate position from start point along tangent direction to
	// end point off the globe. The length of the arrow in world space
	// is inversely proportional to the zoom or magnification.
	const GPlatesMaths::Vector3D end = GPlatesMaths::Vector3D(start) +
			MAP_VELOCITY_SCALE_FACTOR * d_inverse_zoom_factor * rendered_direction_arrow.get_arrow_direction();

	const GPlatesMaths::Vector3D arrowline = end - GPlatesMaths::Vector3D(start);
	const GPlatesMaths::real_t arrowline_length = arrowline.magnitude();

	// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
	if (arrowline_length == 0)
	{
		return;
	}

	double arrowhead_size = d_inverse_zoom_factor * rendered_direction_arrow.get_arrowhead_projected_size();
	const float min_ratio_arrowhead_to_arrowline = rendered_direction_arrow.get_min_ratio_arrowhead_to_arrowline();

	// We want to keep the projected arrowhead size constant regardless of the
	// the length of the arrowline, except...
	//
	// ...if the ratio of arrowhead size to arrowline length is large enough then
	// we need to start scaling the arrowhead size by the arrowline length so
	// that the arrowhead disappears as the arrowline disappears.
	if (arrowhead_size > min_ratio_arrowhead_to_arrowline * arrowline_length.dval())
	{
		arrowhead_size = min_ratio_arrowhead_to_arrowline * arrowline_length.dval();
	}
	// Adjust the arrow head size for the map view.
	arrowhead_size *= GLOBE_TO_MAP_SCALE_FACTOR;

	// Get the drawables for lines of the current line width.
	const float line_width = rendered_direction_arrow.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;
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
    // visit a point on sphere for now.
    GPlatesViewOperations::RenderedGeometry point_on_sphere =
	    GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
		rendered_triangle_symbol.get_centre(),
		rendered_triangle_symbol.get_colour(),
		rendered_triangle_symbol.get_line_width_hint());

    point_on_sphere.accept_visitor(*this);

}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_square_symbol(
	const GPlatesViewOperations::RenderedSquareSymbol &rendered_square_symbol)
{
    // visit a point on sphere for now.
    GPlatesViewOperations::RenderedGeometry point_on_sphere =
	    GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
		rendered_square_symbol.get_centre(),
		rendered_square_symbol.get_colour(),
		rendered_square_symbol.get_line_width_hint());

    point_on_sphere.accept_visitor(*this);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_circle_symbol(
	const GPlatesViewOperations::RenderedCircleSymbol &rendered_circle_symbol)
{
    // visit a point on sphere for now.
    GPlatesViewOperations::RenderedGeometry point_on_sphere =
	    GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
		rendered_circle_symbol.get_centre(),
		rendered_circle_symbol.get_colour(),
		rendered_circle_symbol.get_line_width_hint());

    point_on_sphere.accept_visitor(*this);
}

void
GPlatesGui::MapRenderedGeometryLayerPainter::visit_rendered_cross_symbol(
	const GPlatesViewOperations::RenderedCrossSymbol &rendered_cross_symbol)
{
    // visit a point on sphere for now.
    GPlatesViewOperations::RenderedGeometry point_on_sphere =
	    GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
		rendered_cross_symbol.get_centre(),
		rendered_cross_symbol.get_colour(),
		rendered_cross_symbol.get_line_width_hint());

    point_on_sphere.accept_visitor(*this);
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
GPlatesGui::MapRenderedGeometryLayerPainter::paint_line_geometry(
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
		const Colour &colour,
		stream_primitives_type &lines_stream,
		boost::optional<double> arrow_head_size)
{
	// If the bounding small circle of the line geometry (in the central meridian reference frame)
	// intersects the dateline then it's possible the line geometry does to (and hence needs wrapping).
	//
	// First we need to shift the geometry into the reference frame where the central meridian
	// has longitude zero (because this is where we can do dateline wrapping [-180,180]).
	// Instead of rotating the geometry (expensive) we rotate the centre of its bounding small circle.
	// Then we only need to rotate the geometry if the rotated bounding small circle intersects the dateline.
	const GPlatesMaths::BoundingSmallCircle central_meridian_reference_frame_bounding_small_circle =
			d_central_meridian_reference_frame_rotation * line_geometry->get_bounding_small_circle();

	if (d_dateline_wrapper->intersects_dateline(central_meridian_reference_frame_bounding_small_circle))
	{
		// We now also need to rotate the geometry into the central meridian reference frame.
		typename LineGeometryType::non_null_ptr_to_const_type rotated_line_geometry =
				d_central_meridian_reference_frame_rotation * line_geometry;

		// Wrap the rotated geometry to the longitude range [-180,180].
		std::vector<lat_lon_line_geometry_type> wrapped_line_geometries;
		d_dateline_wrapper->wrap_to_dateline(rotated_line_geometry, wrapped_line_geometries);

		// Paint each wrapped piece of the original geometry.
		BOOST_FOREACH(const lat_lon_line_geometry_type &wrapped_line_geometry, wrapped_line_geometries)
		{
			// If it's a wrapped polygon (not a polyline) then add the start point to the end
			// in order to close the loop - we need to do this because we're iterating over
			// vertices not arcs like the 'paint_unwrapped_line_geometry()' method is.
			if (boost::is_same<LineGeometryType, GPlatesMaths::PolygonOnSphere>::value)
			{
				wrapped_line_geometry->push_back(wrapped_line_geometry->front());
			}

			paint_wrapped_line_geometry(
					wrapped_line_geometry->begin(),
					wrapped_line_geometry->end(),
					colour,
					lines_stream,
					arrow_head_size);
		}

		return;
	}

	// The line geometry does not need any wrapping so we can just paint it without wrapping
	// and its associated rotation adjustments.
	paint_unwrapped_line_geometry(
			line_geometry->begin(),
			line_geometry->end(),
			colour,
			lines_stream,
			arrow_head_size);
}


template <typename LatLonPointForwardIter>
void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_wrapped_line_geometry(
		const LatLonPointForwardIter &begin_lat_lon_points,
		const LatLonPointForwardIter &end_lat_lon_points,
		const Colour &colour,
		stream_primitives_type &lines_stream,
		boost::optional<double> arrow_head_size)
{
	if (begin_lat_lon_points == end_lat_lon_points)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour);

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	stream_line_strips.begin_line_strip();

	// Initialise for arc iteration.
	GPlatesMaths::LatLonPoint arc_start_lat_lon_point = *begin_lat_lon_points;
	GPlatesMaths::PointOnSphere arc_start_point_on_sphere = make_point_on_sphere(arc_start_lat_lon_point);

	// Add the first vertex of the sequence of points.
	// NOTE: We're adding the original wrapped lat/lon point (ie, correctly wrapped).
	// Keep track of the last projected point to calculate arrow head tangent direction.
	QPointF last_projected_point = get_projected_wrapped_position(arc_start_lat_lon_point);
	const coloured_vertex_type first_vertex(last_projected_point.x(), last_projected_point.y(), 0/*z*/, rgba8_color);
	stream_line_strips.add_vertex(first_vertex);

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
				abs(arc_start_point_longitude) == 180.0)
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

					// Keep track of the last projected point to calculate arrow head tangent direction.
					last_projected_point = get_projected_wrapped_position(tess_lat_lon);
					const coloured_vertex_type tess_vertex(
							last_projected_point.x(), last_projected_point.y(), 0/*z*/, rgba8_color);
					stream_line_strips.add_vertex(tess_vertex);
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
					const GPlatesMaths::LatLonPoint tess_lat_lon = make_lat_lon_point(tess_points[n]);

					// Keep track of the last projected point to calculate arrow head tangent direction.
					last_projected_point = get_projected_wrapped_position(tess_lat_lon);
					const coloured_vertex_type tess_vertex(
							last_projected_point.x(), last_projected_point.y(), 0/*z*/, rgba8_color);
					stream_line_strips.add_vertex(tess_vertex);
				}
			}
		}

		// Vertex representing the projected arc end point's position and colour.
		// NOTE: We're adding the original wrapped lat/lon point (ie, correctly wrapped).
		const QPointF projected_gca_end_point = get_projected_wrapped_position(arc_end_lat_lon_point);
		const coloured_vertex_type end_vertex(
				projected_gca_end_point.x(), projected_gca_end_point.y(), 0/*z*/, rgba8_color);
		stream_line_strips.add_vertex(end_vertex);

		// If we need to draw an arrow head at the end of each great circle arc segment...
		if (arrow_head_size)
		{
			paint_arrow_head(
					projected_gca_end_point,
					// Our best estimate of the arrow direction tangent at the GCA end point...
					projected_gca_end_point - last_projected_point,
					arrow_head_size.get(),
					rgba8_color);
		}

		arc_start_lat_lon_point = arc_end_lat_lon_point;
		arc_start_point_on_sphere = arc_end_point_on_sphere;
		// Keep track of the last projected point to calculate arrow head tangent direction.
		last_projected_point = projected_gca_end_point;
	}

	stream_line_strips.end_line_strip();
}


template <typename GreatCircleArcForwardIter>
void
GPlatesGui::MapRenderedGeometryLayerPainter::paint_unwrapped_line_geometry(
		const GreatCircleArcForwardIter &begin_arcs,
		const GreatCircleArcForwardIter &end_arcs,
		const Colour &colour,
		stream_primitives_type &lines_stream,
		boost::optional<double> arrow_head_size)
{
	if (begin_arcs == end_arcs)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour);

	// Used to add line strips to the stream.
	stream_primitives_type::LineStrips stream_line_strips(lines_stream);

	stream_line_strips.begin_line_strip();

	// Add the first vertex of the sequence of great circle arcs.
	// Keep track of the last projected point to calculate arrow head tangent direction.
	QPointF last_projected_point = get_projected_unwrapped_position(begin_arcs->start_point());
	const coloured_vertex_type first_vertex(last_projected_point.x(), last_projected_point.y(), 0/*z*/, rgba8_color);
	stream_line_strips.add_vertex(first_vertex);

	// Iterate over the great circle arcs.
	for (GreatCircleArcForwardIter gca_iter = begin_arcs ; gca_iter != end_arcs; ++gca_iter)
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
			for (unsigned int n = 1; n < tess_points.size() - 1; ++n)
			{
				// Keep track of the last projected point to calculate arrow head tangent direction.
				last_projected_point = get_projected_unwrapped_position(tess_points[n]);
				const coloured_vertex_type tess_vertex(
						last_projected_point.x(), last_projected_point.y(), 0/*z*/, rgba8_color);
				stream_line_strips.add_vertex(tess_vertex);
			}
		}

		// Vertex representing the end point's position and colour.
		const QPointF projected_gca_end_point = get_projected_unwrapped_position(gca.end_point());
		const coloured_vertex_type end_vertex(
				projected_gca_end_point.x(), projected_gca_end_point.y(), 0/*z*/, rgba8_color);
		stream_line_strips.add_vertex(end_vertex);

		// If we need to draw an arrow head at the end of each great circle arc segment...
		if (arrow_head_size)
		{
			paint_arrow_head(
					projected_gca_end_point,
					// Our best estimate of the arrow direction tangent at the GCA end point...
					projected_gca_end_point - last_projected_point,
					arrow_head_size.get(),
					rgba8_color);
		}

		// Keep track of the last projected point to calculate arrow head tangent direction.
		last_projected_point = projected_gca_end_point;
	}

	stream_line_strips.end_line_strip();
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
		const GPlatesMaths::LatLonPoint &central_meridian_reference_frame_point) const
{
	const double &central_longitude = d_map_projection->central_llp().longitude();

	double x = central_meridian_reference_frame_point.longitude();
	double y = central_meridian_reference_frame_point.latitude();

	// Make sure the longitude is within [-180+EPSILON, 180-EPSILON] around the central meridian longitude.
	// 
	// This is to prevent subsequent map projection from wrapping (-180 -> +180 or vice versa) due to
	// the map projection code receiving a longitude value slightly outside that range or the map
	// projection code itself having numerical precision issues.
	//
	// We need this for *wrapped* vertices since they can lie *on* the dateline
	if (x < LONGITUDE_RANGE_LOWER_LIMIT)
	{
		x = LONGITUDE_RANGE_LOWER_LIMIT;
	}
	else if (x > LONGITUDE_RANGE_UPPER_LIMIT)
	{
		x = LONGITUDE_RANGE_UPPER_LIMIT;
	}

	// Convert from the central meridian reference frame (centred at longitude zero) to the
	// original latitude/longitude coordinate frame.
	x += central_longitude;

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
