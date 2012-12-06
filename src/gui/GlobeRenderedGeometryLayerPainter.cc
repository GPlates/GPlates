/* $Id$ */

/**
 * \file Draws rendered geometries in a specific @a RenderedGeometryLayer onto 3d orthographic
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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
#include <boost/bind.hpp>
#include <opengl/OpenGL.h>

#include "ColourScheme.h"
#include "GlobeRenderedGeometryLayerPainter.h"
#include "LayerPainter.h"
#include "SceneLightingParameters.h"

#include "maths/EllipseGenerator.h"
#include "maths/Real.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/MathsUtils.h"

#include "opengl/GLIntersect.h"
#include "opengl/GLRenderer.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/XsString.h"

#include "utils/Profile.h"

#include "view-operations/RenderedArrowedPolyline.h"
#include "view-operations/RenderedCrossSymbol.h"
#include "view-operations/RenderedDirectionArrow.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"
#include "view-operations/RenderedGeometryUtils.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedResolvedRaster.h"
#include "view-operations/RenderedResolvedScalarField3D.h"
#include "view-operations/RenderedSquareSymbol.h"
#include "view-operations/RenderedCircleSymbol.h"
#include "view-operations/RenderedString.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"
#include "view-operations/RenderedTriangleSymbol.h"

// Temporary includes for triangle testing
#include "view-operations/RenderedGeometryFactory.h"

#include <boost/foreach.hpp>

namespace 
{
	/**
	 * We will tessellate a great circle arc if the two endpoints are far enough apart.
	 */
	const double GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD = GPlatesMaths::convert_deg_to_rad(5);
	const double COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD = std::cos(GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

	/**
	 * We will tessellate a small circle (arc) to this angular resolution.
	 */
	const double SMALL_CIRCLE_ANGULAR_INCREMENT = GPlatesMaths::convert_deg_to_rad(1);

	const double TWO_PI = 2. * GPlatesMaths::PI;

	/*!
	 * Scaling factor for symbols
	 */
	const double SYMBOL_SCALE_FACTOR = 0.01;

	/*!
	 * Correction factor for size of filled circle symbol, which uses the standard point rendering,
	 * and which therefore would appear considerably smaller than other symbol types.
	 *
	 * This correction factor brings it in line with the size of the unfilled circle symbol.
	 */
	const double FILLED_CIRCLE_SYMBOL_CORRECTION = 5.;

} // anonymous namespace


const float GPlatesGui::GlobeRenderedGeometryLayerPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::GlobeRenderedGeometryLayerPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;


GPlatesGui::GlobeRenderedGeometryLayerPainter::GlobeRenderedGeometryLayerPainter(
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const double &inverse_viewport_zoom_factor,
		RenderSettings &render_settings,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme,
		PaintRegionType paint_region,
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture) :
	d_rendered_geometry_layer(rendered_geometry_layer),
	d_inverse_zoom_factor(inverse_viewport_zoom_factor),
	d_render_settings(render_settings),
	d_text_renderer_ptr(text_renderer_ptr),
	d_visibility_tester(visibility_tester),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f),
	d_paint_region(paint_region),
	d_surface_occlusion_texture(surface_occlusion_texture)
{
}


GPlatesGui::GlobeRenderedGeometryLayerPainter::cache_handle_type
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint(
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
	// The point/line/polygon primitives get batched up into vertex streams for efficient rendering.
	visit_rendered_geometries(renderer);

	// Do the actual painting.
	const cache_handle_type layer_cache =
			layer_painter.end_painting(
					renderer,
					*d_text_renderer_ptr,
					d_scale,
					d_surface_occlusion_texture);

	// We no longer have a layer painter.
	d_layer_painter = boost::none;

	return layer_cache;
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_point_on_sphere(
		const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

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
	// Force triangle rendering for testing. This lets me easily create triangles via the
	// digitisation tool, or by loading up point files.
	GPlatesViewOperations::RenderedGeometry triangle =
		GPlatesViewOperations::create_rendered_triangle_symbol(
		    rendered_point_on_sphere.get_point_on_sphere(),
		    rendered_point_on_sphere.get_colour(),
		    true);

	triangle.accept_visitor(*this);

	return;
	// End of triangle testing code.
	/////////////////////////////////////////////////////////////////////////////////////
#endif
	const float point_size =
			rendered_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT * d_scale;

	// Get the stream for points of the current point size.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

	// Get the point position.
	const GPlatesMaths::UnitVector3D &pos =
			rendered_point_on_sphere.get_point_on_sphere().position_vector();

	// Vertex representing the point's position and colour.
	// Convert colour from floats to bytes to use less vertex memory.
	const coloured_vertex_type vertex(pos, Colour::to_rgba8(*colour));

	// Used to add points to the stream.
	stream_primitives_type::Points stream_points(stream);

	stream_points.begin_points();
	stream_points.add_vertex(vertex);
	stream_points.end_points();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_multi_point_on_sphere(
		const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

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
		// Get the point position.
		const GPlatesMaths::UnitVector3D &pos = point_iter->position_vector();

		// Vertex representing the point's position and colour.
		const coloured_vertex_type vertex(pos, rgba8_color);

		stream_points.add_vertex(vertex);
	}

	stream_points.end_points();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_polyline_on_sphere(
		const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

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

	paint_great_circle_arcs(
			polyline_on_sphere->begin(),
			polyline_on_sphere->end(),
			colour.get(),
			stream);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_polygon_on_sphere(
		const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

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

	const float line_width =
			rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_great_circle_arcs(
			polygon_on_sphere->begin(),
			polygon_on_sphere->end(),
			colour.get(),
			stream);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_resolved_raster(
		const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	// Queue the raster primitive for painting.
	d_layer_painter->rasters.push_back(
			LayerPainter::RasterDrawable(
					rendered_resolved_raster.get_resolved_raster(),
					rendered_resolved_raster.get_raster_colour_palette(),
					rendered_resolved_raster.get_raster_modulate_colour()));
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_resolved_scalar_field_3d(
		const GPlatesViewOperations::RenderedResolvedScalarField3D &rendered_resolved_scalar_field)
{
	if (d_paint_region != PAINT_SUB_SURFACE)
	{
		return;
	}

	// Queue the scalar field for painting.
	d_layer_painter->scalar_fields.push_back(
			LayerPainter::ScalarField3DDrawable(
					rendered_resolved_scalar_field.get_resolved_scalar_field_3d(),
					rendered_resolved_scalar_field.get_render_parameters()));
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_direction_arrow(
		const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	if (!d_render_settings.show_arrows())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_direction_arrow);
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(*colour);

	const GPlatesMaths::Vector3D start(
			rendered_direction_arrow.get_start_position().position_vector());

	// Calculate position from start point along tangent direction to
	// end point off the globe. The length of the arrow in world space
	// is inversely proportional to the zoom or magnification.
	const GPlatesMaths::Vector3D end = GPlatesMaths::Vector3D(start) +
			d_inverse_zoom_factor * rendered_direction_arrow.get_arrow_direction();

	const GPlatesMaths::Vector3D arrowline = end - start;
	const GPlatesMaths::real_t arrowline_length = arrowline.magnitude();

	// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
	if (arrowline_length == 0)
	{
		return;
	}
	const GPlatesMaths::Vector3D arrowline_unit_vector = (1.0 / arrowline_length) * arrowline;

	GPlatesMaths::real_t arrowhead_size =
			d_inverse_zoom_factor * rendered_direction_arrow.get_arrowhead_projected_size();

	const float min_ratio_arrowhead_to_arrowline =
			rendered_direction_arrow.get_min_ratio_arrowhead_to_arrowline();

	// We want to keep the projected arrowhead size constant regardless of the
	// the length of the arrowline, except...
	//
	// ...if the ratio of arrowhead size to arrowline length is large enough then
	// we need to start scaling the arrowhead size by the arrowline length so
	// that the arrowhead disappears as the arrowline disappears.
	if (arrowhead_size > min_ratio_arrowhead_to_arrowline * arrowline_length)
	{
		arrowhead_size = min_ratio_arrowhead_to_arrowline * arrowline_length;
	}
	
	// Specify end of arrowhead and direction of arrow.
	paint_cone(
			end,
			arrowhead_size * arrowline_unit_vector,
			rgba8_color,
			d_layer_painter->drawables_off_the_sphere.get_triangles_stream());


	const float line_width =
			rendered_direction_arrow.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the drawables for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->drawables_off_the_sphere.get_lines_stream(line_width);

	// Render a single line segment for the arrow body.

	// Used to add lines to the stream.
	stream_primitives_type::Lines stream_lines(stream);

	stream_lines.begin_lines();

	// Vertex representing the start and end point's position and colour.
	const coloured_vertex_type start_vertex(start.x().dval(), start.y().dval(), start.z().dval(), rgba8_color);
	const coloured_vertex_type end_vertex(end.x().dval(), end.y().dval(), end.z().dval(), rgba8_color);

	stream_lines.add_vertex(start_vertex);
	stream_lines.add_vertex(end_vertex);

	stream_lines.end_lines();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	if (!d_render_settings.show_strings())
	{
		return;
	}

	if (d_visibility_tester.is_point_visible(rendered_string.get_point_on_sphere()))
	{
		d_layer_painter->text_drawables_3D.push_back(
				LayerPainter::TextDrawable3D(
						rendered_string.get_string(),
						rendered_string.get_font(),
						rendered_string.get_point_on_sphere().position_vector(),
						rendered_string.get_x_offset(),
						rendered_string.get_y_offset(),
						get_colour_of_rendered_geometry(rendered_string),
						rendered_string.get_shadow_colour().get_colour(d_colour_scheme)));
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle);
	if (!colour)
	{
		return;
	}

	const float line_width = rendered_small_circle.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	std::vector<GPlatesMaths::PointOnSphere> points;
	tessellate(points, rendered_small_circle.get_small_circle(), SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Used to add a line loop to the stream.
	stream_primitives_type::LineLoops stream_line_loops(stream);

	stream_line_loops.begin_line_loop();

	for (unsigned int i = 0; i < points.size(); ++i)
	{
		stream_line_loops.add_vertex(coloured_vertex_type(points[i].position_vector(), rgba8_color));
	}

	stream_line_loops.end_line_loop();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_small_circle_arc(
		const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle_arc);
	if (!colour)
	{
		return;
	}

	const float line_width = rendered_small_circle_arc.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

	std::vector<GPlatesMaths::PointOnSphere> points;
	tessellate(points, rendered_small_circle_arc.get_small_circle_arc(), SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Used to add a line strip to the stream.
	stream_primitives_type::LineStrips stream_line_strips(stream);

	stream_line_strips.begin_line_strip();

	for (unsigned int i = 0; i < points.size(); ++i)
	{
		stream_line_strips.add_vertex(coloured_vertex_type(points[i].position_vector(), rgba8_color));
	}

	stream_line_strips.end_line_strip();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_ellipse);
	if (!colour)
	{
		return;
	}

	const float line_width = rendered_ellipse.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

	paint_ellipse(rendered_ellipse, colour.get(), stream);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_arrowed_polyline(
		const GPlatesViewOperations::RenderedArrowedPolyline &rendered_arrowed_polyline)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	// Based on the "visit_rendered_direction_arrow" code 

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_arrowed_polyline);
	if (!colour)
	{
		return;
	}

	const rgba8_t rgba8_colour = Colour::to_rgba8(*colour);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			rendered_arrowed_polyline.get_polyline_on_sphere();

	GPlatesMaths::PolylineOnSphere::const_iterator 
			polyline_points_iter = polyline->begin(),
			polyline_points_end = polyline->end();
	for (; polyline_points_iter != polyline_points_end ; ++polyline_points_iter)
	{	
		const GPlatesMaths::GreatCircleArc &gca = *polyline_points_iter;

		GPlatesMaths::real_t arrowhead_size =
				d_inverse_zoom_factor * rendered_arrowed_polyline.get_arrowhead_projected_size();
		if (arrowhead_size > rendered_arrowed_polyline.get_max_arrowhead_size())
		{
			arrowhead_size = rendered_arrowed_polyline.get_max_arrowhead_size();
		}

		// For the direction of the arrow, we really want the tangent to the curve at
		// the end of the curve. The curve will ultimately be a small circle arc; the 
		// current implementation uses a great circle arc. 
		if (!gca.is_zero_length())
		{
			const GPlatesMaths::Vector3D tangent_direction =
					GPlatesMaths::cross(
							gca.rotation_axis(),
							gca.end_point().position_vector());
			const GPlatesMaths::UnitVector3D arrowline_unit_vector(tangent_direction);

			paint_cone(
					GPlatesMaths::Vector3D(gca.end_point().position_vector()),
					arrowhead_size * arrowline_unit_vector,
					rgba8_colour,
					d_layer_painter->drawables_off_the_sphere.get_triangles_stream());
		}
	}

	const float line_width =
		rendered_arrowed_polyline.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	stream_primitives_type &lines_stream =
		d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);


	paint_great_circle_arcs(
		polyline->begin(),
		polyline->end(),
		colour.get(),
		lines_stream);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_triangle_symbol(
	const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

    boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_triangle_symbol);
    if (!colour)
    {
            return;
    }

    bool filled = rendered_triangle_symbol.get_is_filled();

    // Quick and dirty way to get triangle vertex coordinates at desired location:
    // Define the triangle in the tangent plane at the north pole,
    // then rotate the triangle down to required latitude, and
    // then east/west to required longitude.
    //
    // (Two rotations are required to maintain the north-alignment).
    //
    //
    // Can I use a new render node to do this rotation more efficiently?
    //
    // Reminder about coordinate system:
    // x is out of the screen as we look at the globe on startup.
    // y is towards right (east) as we look at the globe on startup.
    // z is up...

    // Get the point position.
    const GPlatesMaths::PointOnSphere &pos =
		    rendered_triangle_symbol.get_centre();

    GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

    static const GPlatesMaths::UnitVector3D axis1 = GPlatesMaths::UnitVector3D(0.,1.,0.);
    GPlatesMaths::Rotation r1 =
	    GPlatesMaths::Rotation::create(axis1,GPlatesMaths::HALF_PI - GPlatesMaths::convert_deg_to_rad(llp.latitude()));

    static const GPlatesMaths::UnitVector3D axis2 = GPlatesMaths::UnitVector3D(0.,0.,1.);
    GPlatesMaths::Rotation r2 =
	    GPlatesMaths::Rotation::create(axis2,GPlatesMaths::convert_deg_to_rad(llp.longitude()));

    GPlatesMaths::Rotation r3 = r2*r1;


	double size = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_triangle_symbol.get_size();

	// r is radius of circumscribing circle. Factor 1.33 used here to give us a triangle height of 2*d.
	double r = 1.333 * size;

	// Triangle vertices defined in the plane z=1.
	GPlatesMaths::Vector3D va(-r,0.,1.);
	GPlatesMaths::Vector3D vb(0.5*r,-0.86*r,1.);
	GPlatesMaths::Vector3D vc(0.5*r,0.86*r,1.);

	// Rotate to desired location.
	va = r3 * va;
	vb = r3 * vb;
	vc = r3 * vc;


	coloured_vertex_type a(va.x().dval(), va.y().dval(),va.z().dval(),Colour::to_rgba8(*colour));
	coloured_vertex_type b(vb.x().dval(), vb.y().dval(),vb.z().dval(),Colour::to_rgba8(*colour));
	coloured_vertex_type c(vc.x().dval(), vc.y().dval(),vc.z().dval(),Colour::to_rgba8(*colour));

	if (filled)
	{
		stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_triangles_stream();

		stream_primitives_type::Triangles stream_triangles(stream);

		// The polygon state is fill, front/back by default, so I shouldn't need
		// to change anything here.

		stream_triangles.begin_triangles();
		stream_triangles.add_vertex(a);
		stream_triangles.add_vertex(b);
		stream_triangles.add_vertex(c);
		stream_triangles.end_triangles();
	}
	else
	{
		const float line_width = rendered_triangle_symbol.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

		stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

		stream_primitives_type::LineStrips stream_line_strips(stream);

		stream_line_strips.begin_line_strip();
		stream_line_strips.add_vertex(a);
		stream_line_strips.add_vertex(b);
		stream_line_strips.add_vertex(c);
		stream_line_strips.add_vertex(a);
		stream_line_strips.end_line_strip();
	}
}

void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_square_symbol(
	const GPlatesViewOperations::RenderedSquareSymbol &rendered_square_symbol)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

    boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_square_symbol);
    if (!colour)
    {
	    return;
    }

    bool filled = rendered_square_symbol.get_is_filled();

    // Define the square in the tangent plane at the north pole,
    // then rotate down to required latitude, and
    // then east/west to required longitude.
    //
    // (Two rotations are required to maintain the north-alignment).
    //
    //
    // Can I use a new render node to do this rotation more efficiently?
    //
    // Reminder about coordinate system:
    // x is out of the screen as we look at the globe on startup.
    // y is towards right (east) as we look at the globe on startup.
    // z is up...

    // Get the point position.
    const GPlatesMaths::PointOnSphere &pos =
		    rendered_square_symbol.get_centre();

    GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

    static const GPlatesMaths::UnitVector3D axis1 = GPlatesMaths::UnitVector3D(0.,1.,0.);
    GPlatesMaths::Rotation r1 =
	    GPlatesMaths::Rotation::create(axis1,GPlatesMaths::HALF_PI - GPlatesMaths::convert_deg_to_rad(llp.latitude()));

    static const GPlatesMaths::UnitVector3D axis2 = GPlatesMaths::UnitVector3D(0.,0.,1.);
    GPlatesMaths::Rotation r2 =
	    GPlatesMaths::Rotation::create(axis2,GPlatesMaths::convert_deg_to_rad(llp.longitude()));

    GPlatesMaths::Rotation r3 = r2*r1;

	double size = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_square_symbol.get_size();

    // Make a triangle fan with centre (0,0,1)
    GPlatesMaths::Vector3D v3d_a(0.,0.,1.);
	GPlatesMaths::Vector3D v3d_b(-size,-size,1.);
	GPlatesMaths::Vector3D v3d_c(-size,size,1.);
	GPlatesMaths::Vector3D v3d_d(size,size,1.);
	GPlatesMaths::Vector3D v3d_e(size,-size,1.);

    // Rotate to desired location.
    v3d_a = r3 * v3d_a;
    v3d_b = r3 * v3d_b;
    v3d_c = r3 * v3d_c;
    v3d_d = r3 * v3d_d;
    v3d_e = r3 * v3d_e;


    coloured_vertex_type va(v3d_a.x().dval(), v3d_a.y().dval(),v3d_a.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vb(v3d_b.x().dval(), v3d_b.y().dval(),v3d_b.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vc(v3d_c.x().dval(), v3d_c.y().dval(),v3d_c.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vd(v3d_d.x().dval(), v3d_d.y().dval(),v3d_d.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type ve(v3d_e.x().dval(), v3d_e.y().dval(),v3d_e.z().dval(),Colour::to_rgba8(*colour));

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
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_circle_symbol(
	const GPlatesViewOperations::RenderedCircleSymbol &rendered_circle_symbol)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

    boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_circle_symbol);
    if (!colour)
    {
	    return;
    }

    bool filled = rendered_circle_symbol.get_is_filled();
	


    if (filled)
    {
		const float point_size = FILLED_CIRCLE_SYMBOL_CORRECTION *
				rendered_circle_symbol.get_size() * POINT_SIZE_ADJUSTMENT * d_scale;

		// Get the stream for points of the current point size.
		stream_primitives_type &stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

		// Get the point position.
		const GPlatesMaths::UnitVector3D &pos = rendered_circle_symbol.get_centre().position_vector();

		// Vertex representing the point's position and colour.
		// Convert colour from floats to bytes to use less vertex memory.
		const coloured_vertex_type vertex(pos, Colour::to_rgba8(*colour));

		// Used to add points to the stream.
		stream_primitives_type::Points stream_points(stream);

		stream_points.begin_points();
		stream_points.add_vertex(vertex);
		stream_points.end_points();
    }
    else
    {
		const float line_width = rendered_circle_symbol.get_size() * LINE_WIDTH_ADJUSTMENT * d_scale;

		double radius = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_circle_symbol.get_size();

		// Get the stream for lines of the current line width.
		stream_primitives_type &stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

		// Convert colour from floats to bytes to use less vertex memory.
		const rgba8_t rgba8_color = Colour::to_rgba8(colour.get());

		const GPlatesMaths::SmallCircle small_circle = GPlatesMaths::SmallCircle::create_colatitude(
				rendered_circle_symbol.get_centre().position_vector(),
				radius);

		std::vector<GPlatesMaths::PointOnSphere> points;
		tessellate(points, small_circle, SMALL_CIRCLE_ANGULAR_INCREMENT);

		// Used to add a line loop to the stream.
		stream_primitives_type::LineLoops stream_line_loops(stream);

		stream_line_loops.begin_line_loop();

		for (unsigned int i = 0; i < points.size(); ++i)
		{
			stream_line_loops.add_vertex(coloured_vertex_type(points[i].position_vector(), rgba8_color));
		}

		stream_line_loops.end_line_loop();
    }
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_cross_symbol(
	const GPlatesViewOperations::RenderedCrossSymbol &rendered_cross_symbol)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

    boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_cross_symbol);
    if (!colour)
    {
	    return;
    }

    //
    // Reminder about coordinate system:
    // x is out of the screen as we look at the globe on startup.
    // y is towards right (east) as we look at the globe on startup.
    // z is up...

    // Get the point position.
    const GPlatesMaths::PointOnSphere &pos =
		    rendered_cross_symbol.get_centre();

    GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);

    static const GPlatesMaths::UnitVector3D axis1 = GPlatesMaths::UnitVector3D(0.,1.,0.);
    GPlatesMaths::Rotation r1 =
		GPlatesMaths::Rotation::create(axis1,GPlatesMaths::HALF_PI - GPlatesMaths::convert_deg_to_rad(llp.latitude()));

    static const GPlatesMaths::UnitVector3D axis2 = GPlatesMaths::UnitVector3D(0.,0.,1.);
    GPlatesMaths::Rotation r2 =
	    GPlatesMaths::Rotation::create(axis2,GPlatesMaths::convert_deg_to_rad(llp.longitude()));

    GPlatesMaths::Rotation r3 = r2*r1;

	double d = SYMBOL_SCALE_FACTOR * d_inverse_zoom_factor * rendered_cross_symbol.get_size();

    // Set up the vertices of a cross with centre (0,0,1).
    GPlatesMaths::Vector3D v3d_a(0.,d,1.);
    GPlatesMaths::Vector3D v3d_b(0,-d,1.);
    GPlatesMaths::Vector3D v3d_c(-d,0.,1.);
    GPlatesMaths::Vector3D v3d_d(d,0.,1.);

    // Rotate to desired location.
    v3d_a = r3 * v3d_a;
    v3d_b = r3 * v3d_b;
    v3d_c = r3 * v3d_c;
    v3d_d = r3 * v3d_d;


    coloured_vertex_type va(v3d_a.x().dval(), v3d_a.y().dval(),v3d_a.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vb(v3d_b.x().dval(), v3d_b.y().dval(),v3d_b.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vc(v3d_c.x().dval(), v3d_c.y().dval(),v3d_c.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vd(v3d_d.x().dval(), v3d_d.y().dval(),v3d_d.z().dval(),Colour::to_rgba8(*colour));


    const float line_width = rendered_cross_symbol.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

    stream_primitives_type &stream =
	    d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

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
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_geometries(
		GPlatesOpenGL::GLRenderer &renderer)
{
	d_current_cube_quad_tree_location = boost::none;

	// See if there's a spatial partition of rendered geometries.
	boost::optional<const rendered_geometries_spatial_partition_type &>
			rendered_geometries_spatial_partition_opt = d_rendered_geometry_layer.get_rendered_geometries();
	// If not then just render all rendered geometries without view-frustum culling.
	if (!rendered_geometries_spatial_partition_opt)
	{
		// Visit each RenderedGeometry.
		std::for_each(
			d_rendered_geometry_layer.rendered_geometry_begin(),
			d_rendered_geometry_layer.rendered_geometry_end(),
			boost::bind(&GPlatesViewOperations::RenderedGeometry::accept_visitor, _1, boost::ref(*this)));

		return;
	}

	// Render using the spatial partition to do view-frustum culling (the geometries completely
	// outside the view frustum are not rendered).
	const rendered_geometries_spatial_partition_type &rendered_geometries_spatial_partition =
			rendered_geometries_spatial_partition_opt.get();
	render_spatial_partition(renderer, rendered_geometries_spatial_partition);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::render_spatial_partition(
		GPlatesOpenGL::GLRenderer &renderer,
		const rendered_geometries_spatial_partition_type &rendered_geometries_spatial_partition)
{

	// Visit the rendered geometries in the root of the cube quad tree.
	// These are unpartitioned and hence must be rendered regardless of the view frustum.
	// Start out at the root of the cube quad tree.
	d_current_cube_quad_tree_location = boost::none;
	std::for_each(
		rendered_geometries_spatial_partition.begin_root_elements(),
		rendered_geometries_spatial_partition.end_root_elements(),
		boost::bind(&GPlatesViewOperations::RenderedGeometry::accept_visitor, _1, boost::ref(*this)));

	// Create a loose oriented-bounding-box cube quad tree cache so we can do view-frustum culling
	// as we traverse the spatial partition of rendered geometries.
	// We're only visiting each subdivision node once so there's no need for caching.
	cube_subdivision_cache_type::non_null_ptr_type cube_subdivision_cache =
			cube_subdivision_cache_type::create(
					GPlatesOpenGL::GLCubeSubdivision::create());

	// Get the view frustum planes.
	const GPlatesOpenGL::GLFrustum frustum_planes(
			renderer.gl_get_matrix(GL_MODELVIEW),
			renderer.gl_get_matrix(GL_PROJECTION));

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The root node of the current quad tree.
		const rendered_geometries_spatial_partition_type::const_node_reference_type
				loose_quad_tree_root_node = rendered_geometries_spatial_partition
						.get_quad_tree_root_node(cube_face);

		// If there is not quad tree root node in the current loose cube face
		// then continue to next cube face.
		if (!loose_quad_tree_root_node)
		{
			continue;
		}

		const cube_subdivision_cache_type::node_reference_type
				cube_subdivision_cache_root_node = cube_subdivision_cache->get_quad_tree_root_node(cube_face);

		// We're at the root node of the quad tree of the current cube face.
		const GPlatesMaths::CubeQuadTreeLocation root_cube_quad_tree_node_location(cube_face);

		render_spatial_partition_quad_tree(
				root_cube_quad_tree_node_location,
				loose_quad_tree_root_node,
				*cube_subdivision_cache,
				cube_subdivision_cache_root_node,
				frustum_planes,
				// There are six frustum planes initially active
				GPlatesOpenGL::GLFrustum::ALL_PLANES_ACTIVE_MASK);
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::render_spatial_partition_quad_tree(
		const GPlatesMaths::CubeQuadTreeLocation &cube_quad_tree_node_location,
		rendered_geometries_spatial_partition_type::const_node_reference_type rendered_geometries_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		const GPlatesOpenGL::GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		// See if the current quad tree render node intersects the view frustum.
		// Use the static quad tree node's bounding box.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GPlatesOpenGL::GLIntersect::intersect_OBB_frustum(
						cube_subdivision_cache.get_loose_oriented_bounding_box(cube_subdivision_cache_node),
						frustum_planes.get_planes(),
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so quad tree node is outside view frustum and we can cull it.
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current quad tree render node intersects. The node is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// Visit the rendered geometries in the current quad tree node.
	d_current_cube_quad_tree_location = cube_quad_tree_node_location;
	std::for_each(
		rendered_geometries_quad_tree_node.begin(),
		rendered_geometries_quad_tree_node.end(),
		boost::bind(&GPlatesViewOperations::RenderedGeometry::accept_visitor, _1, boost::ref(*this)));


	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// See if there is a child node in the rendered geometries.
			const rendered_geometries_spatial_partition_type::const_node_reference_type
					child_rendered_geometries_quad_tree_node =
							rendered_geometries_quad_tree_node.get_child_node(
									child_u_offset, child_v_offset);

			if (!child_rendered_geometries_quad_tree_node)
			{
				continue;
			}

			const cube_subdivision_cache_type::node_reference_type
					child_loose_bounds_node = cube_subdivision_cache.get_child_node(
							cube_subdivision_cache_node, child_u_offset, child_v_offset);

			// We're at the child node location of the current node location.
			const GPlatesMaths::CubeQuadTreeLocation child_cube_quad_tree_node_location(
							cube_quad_tree_node_location,
							child_u_offset,
							child_v_offset);

			render_spatial_partition_quad_tree(
					child_cube_quad_tree_node_location,
					child_rendered_geometries_quad_tree_node,
					cube_subdivision_cache,
					child_loose_bounds_node,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


template <class T>
boost::optional<GPlatesGui::Colour>
GPlatesGui::GlobeRenderedGeometryLayerPainter::get_colour_of_rendered_geometry(
		const T &geom)
{
	return geom.get_colour().get_colour(d_colour_scheme);
}


template <typename GreatCircleArcForwardIter>
void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_great_circle_arcs(
		const GreatCircleArcForwardIter &begin_arcs,
		const GreatCircleArcForwardIter &end_arcs,
		const Colour &colour,
		stream_primitives_type &lines_stream)
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
	stream_line_strips.add_vertex(
			coloured_vertex_type(begin_arcs->start_point().position_vector(), rgba8_color));

	// Iterate over the great circle arcs.
	for (GreatCircleArcForwardIter gca_iter = begin_arcs ; gca_iter != end_arcs; ++gca_iter)
	{
		const GPlatesMaths::GreatCircleArc &gca = *gca_iter;

		// Tessellate the current arc if its two endpoints are far enough apart.
		if (gca.dot_of_endpoints() < COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD)
		{
			// Tessellate the current great circle arc.
			std::vector<GPlatesMaths::PointOnSphere> points;
			tessellate(points, gca, GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

			// Add the tessellated points skipping the first since it was added by the previous GCA.
			for (unsigned int n = 1; n < points.size(); ++n)
			{
				const GPlatesMaths::PointOnSphere &point = points[n];
				stream_line_strips.add_vertex(coloured_vertex_type(point.position_vector(), rgba8_color));
			}

			continue;
		}

		// Get the end position of the great circle arc.
		const GPlatesMaths::UnitVector3D &end = gca.end_point().position_vector();

		// Vertex representing the end point's position and colour.
		stream_line_strips.add_vertex(coloured_vertex_type(end, rgba8_color));
	}

	stream_line_strips.end_line_strip();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
		const Colour &colour,
		stream_primitives_type &lines_stream)
{
	// We could make this zoom dependent, but:
	// For an ellipse with fairly tight curvature, at maximum zoom (10000%),
	// 128 steps gives a just-about-noticeable jagged appearance; 256 steps
	// appears pretty smooth (to me, at least).  We could reduce this at lower
	// zooms, but anything below about 64 steps makes large ellipses (e.g. one which
	// is effectively a great circle) appear jagged at minimum zoom (100%).
	// So we could make the number of steps vary from (say) 64 at 100% zoom to
	// 256 at 10000% zoom.
	// The inverse zoom factor varies from 1 at 100% zoom to 0.01 at 10000% zoom. 
	// Using the sqrt of the inverse zoom factor, we could use 64 steps at min zoom
	// and 640 steps at max zoom, for example.

	static const unsigned int nsteps = 256;
	static const double dt = TWO_PI/nsteps;

	if ((rendered_ellipse.get_semi_major_axis_radians() == 0.)
		|| (rendered_ellipse.get_semi_minor_axis_radians() == 0.))
	{
		return;	
	}
	
	GPlatesMaths::EllipseGenerator ellipse_generator(
			rendered_ellipse.get_centre(),
			rendered_ellipse.get_semi_major_axis_radians(),
			rendered_ellipse.get_semi_minor_axis_radians(),
			rendered_ellipse.get_axis());

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour);

	// Used to add line loops to the stream.
	stream_primitives_type::LineLoops stream_line_loops(lines_stream);

	stream_line_loops.begin_line_loop();

	for (double i = 0 ; i < TWO_PI ; i += dt)
	{
		GPlatesMaths::UnitVector3D uv = ellipse_generator.get_point_on_ellipse(i);

		// Vertex representing the ellipse point position and colour.
		const coloured_vertex_type vertex(uv, rgba8_color);

		stream_line_loops.add_vertex(vertex);
	}

	stream_line_loops.end_line_loop();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_cone(
		const GPlatesMaths::Vector3D &apex,
		const GPlatesMaths::Vector3D &cone_axis,
		rgba8_t rgba8_color,
		stream_primitives_type &triangles_stream)
{
	const GPlatesMaths::Vector3D centre_base_circle = apex - cone_axis;

	const GPlatesMaths::real_t cone_axis_mag = cone_axis.magnitude();

	// Avoid divide-by-zero - and if cone length is near zero it won't be visible.
	if (cone_axis_mag == 0)
	{
		return;
	}

	const GPlatesMaths::UnitVector3D cone_zaxis( (1 / cone_axis_mag) * cone_axis );

	// Find an orthonormal basis using 'cone_axis'.
	const GPlatesMaths::UnitVector3D cone_yaxis = generate_perpendicular(cone_zaxis);
	const GPlatesMaths::UnitVector3D cone_xaxis( cross(cone_yaxis, cone_zaxis) );

	static const int NUM_VERTICES_IN_BASE_UNIT_CIRCLE = 6;
	static const double s_vertex_angle =
			2 * GPlatesMaths::PI / NUM_VERTICES_IN_BASE_UNIT_CIRCLE;
	static const GPlatesMaths::real_t s_base_unit_circle[NUM_VERTICES_IN_BASE_UNIT_CIRCLE][2] =
			{
				{ GPlatesMaths::cos(0 * s_vertex_angle), GPlatesMaths::sin(0 * s_vertex_angle) },
				{ GPlatesMaths::cos(1 * s_vertex_angle), GPlatesMaths::sin(1 * s_vertex_angle) },
				{ GPlatesMaths::cos(2 * s_vertex_angle), GPlatesMaths::sin(2 * s_vertex_angle) },
				{ GPlatesMaths::cos(3 * s_vertex_angle), GPlatesMaths::sin(3 * s_vertex_angle) },
				{ GPlatesMaths::cos(4 * s_vertex_angle), GPlatesMaths::sin(4 * s_vertex_angle) },
				{ GPlatesMaths::cos(5 * s_vertex_angle), GPlatesMaths::sin(5 * s_vertex_angle) }
			};

	// Radius of cone base circle is proportional to the distance from the apex to
	// the centre of the base circle.
	const float ratio_cone_radius_to_axis = 0.5f;
	const GPlatesMaths::real_t radius_cone_circle = ratio_cone_radius_to_axis * cone_axis_mag;

	// Generate the cone vertices in the frame of reference of the cone axis.
	// We could use an OpenGL transformation matrix to do this for us but that's
	// overkill since cone only needs to be transformed once.
	const GPlatesMaths::Vector3D cone_base_circle[NUM_VERTICES_IN_BASE_UNIT_CIRCLE] =
			{
				centre_base_circle + radius_cone_circle * (
						s_base_unit_circle[0][0] * cone_xaxis +
						s_base_unit_circle[0][1] * cone_yaxis),
				centre_base_circle + radius_cone_circle * (
						s_base_unit_circle[1][0] * cone_xaxis +
						s_base_unit_circle[1][1] * cone_yaxis),
				centre_base_circle + radius_cone_circle * (
						s_base_unit_circle[2][0] * cone_xaxis +
						s_base_unit_circle[2][1] * cone_yaxis),
				centre_base_circle + radius_cone_circle * (
						s_base_unit_circle[3][0] * cone_xaxis +
						s_base_unit_circle[3][1] * cone_yaxis),
				centre_base_circle + radius_cone_circle * (
						s_base_unit_circle[4][0] * cone_xaxis +
						s_base_unit_circle[4][1] * cone_yaxis),
				centre_base_circle + radius_cone_circle * (
						s_base_unit_circle[5][0] * cone_xaxis +
						s_base_unit_circle[5][1] * cone_yaxis)
			};

	// We draw both sides of polygons to avoid having to close the 3d mesh
	// used to render the arrow head.
	// This is the default state for OpenGL so we don't need to set it.

	// Used to add triangle fan to the stream.
	stream_primitives_type::TriangleFans stream_triangle_fans(triangles_stream);

	stream_triangle_fans.begin_triangle_fan();

	const coloured_vertex_type apex_vertex(apex.x().dval(), apex.y().dval(), apex.z().dval(), rgba8_color);
	stream_triangle_fans.add_vertex(apex_vertex);

	for (int vertex_index = 0;
		vertex_index < NUM_VERTICES_IN_BASE_UNIT_CIRCLE;
		++vertex_index)
	{
		const GPlatesMaths::Vector3D &boundary = cone_base_circle[vertex_index];
		const coloured_vertex_type boundary_vertex(
				boundary.x().dval(), boundary.y().dval(), boundary.z().dval(), rgba8_color);
		stream_triangle_fans.add_vertex(boundary_vertex);
	}
	const GPlatesMaths::Vector3D &last_circle = cone_base_circle[0];
	const coloured_vertex_type last_circle_vertex(
			last_circle.x().dval(), last_circle.y().dval(), last_circle.z().dval(), rgba8_color);
	stream_triangle_fans.add_vertex(last_circle_vertex);

	stream_triangle_fans.end_triangle_fan();
}
