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
#include <cmath>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>

#include "ColourScheme.h"
#include "GlobeRenderedGeometryLayerPainter.h"
#include "LayerPainter.h"
#include "SceneLightingParameters.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/EllipseGenerator.h"
#include "maths/Real.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/MathsUtils.h"

#include "opengl/GLIntersect.h"
#include "opengl/GLIntersectPrimitives.h"
#include "opengl/GLRenderer.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/XsString.h"

#include "utils/Profile.h"

#include "view-operations/RenderedArrowedPolyline.h"
#include "view-operations/RenderedCircleSymbol.h"
#include "view-operations/RenderedColouredEdgeSurfaceMesh.h"
#include "view-operations/RenderedColouredTriangleSurfaceMesh.h"
#include "view-operations/RenderedCrossSymbol.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"
#include "view-operations/RenderedGeometryUtils.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedRadialArrow.h"
#include "view-operations/RenderedResolvedRaster.h"
#include "view-operations/RenderedResolvedScalarField3D.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"
#include "view-operations/RenderedSquareSymbol.h"
#include "view-operations/RenderedStrainMarkerSymbol.h"
#include "view-operations/RenderedString.h"
#include "view-operations/RenderedTangentialArrow.h"
#include "view-operations/RenderedTriangleSymbol.h"

// Temporary includes for triangle testing
#include "view-operations/RenderedGeometryFactory.h"

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

	const double ARROWHEAD_BASE_HEIGHT_RATIO = 0.5;
	const double COSINE_ARROWHEAD_BASE_HEIGHT_RATIO = std::cos(std::atan(ARROWHEAD_BASE_HEIGHT_RATIO));
	const double SINE_ARROWHEAD_BASE_HEIGHT_RATIO = std::sin(std::atan(ARROWHEAD_BASE_HEIGHT_RATIO));

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
const float GPlatesGui::GlobeRenderedGeometryLayerPainter::STRAIN_LINE_WIDTH_ADJUSTMENT = 1.0f;


GPlatesGui::GlobeRenderedGeometryLayerPainter::GlobeRenderedGeometryLayerPainter(
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const double &inverse_viewport_zoom_factor,
		const RenderSettings &render_settings,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme,
		PaintRegionType paint_region,
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture,
		bool improve_performance_reduce_quality_hint) :
	d_rendered_geometry_layer(rendered_geometry_layer),
	d_inverse_zoom_factor(inverse_viewport_zoom_factor),
	d_render_settings(render_settings),
	d_visibility_tester(visibility_tester),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f),
	d_paint_region(paint_region),
	d_surface_occlusion_texture(surface_occlusion_texture),
	d_improve_performance_reduce_quality_hint(improve_performance_reduce_quality_hint)
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

	// Get the view frustum planes.
	d_frustum_planes = boost::in_place(
			renderer.gl_get_matrix(GL_MODELVIEW),
			renderer.gl_get_matrix(GL_PROJECTION));

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
					d_scale,
					d_surface_occlusion_texture);

	// We no longer have a layer painter.
	d_layer_painter = boost::none;

	// Frustum planes are no longer relevant.
	d_frustum_planes = boost::none;

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

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_polyline_on_sphere.get_polyline_on_sphere();

	if (rendered_polyline_on_sphere.get_is_filled())
	{
		GPlatesOpenGL::GLFilledPolygonsGlobeView::filled_drawables_type &filled_polygons =
				d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons_globe_view();

		// Modulate with the fill modulate colour.
		const Colour fill_colour = Colour::modulate(
				colour.get(),
				rendered_polyline_on_sphere.get_fill_modulate_colour());

		// Add the filled polygon at the current location (if any) in the rendered geometries spatial partition.
		filled_polygons.add_filled_polygon(
				*polyline_on_sphere,
				fill_colour,
				d_current_spatial_partition_location.get());

		return;
	}

	const float line_width =
			rendered_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for lines of the current line width.
	stream_primitives_type &stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(line_width);

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
		GPlatesOpenGL::GLFilledPolygonsGlobeView::filled_drawables_type &filled_polygons =
				d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons_globe_view();

		// Modulate with the fill modulate colour.
		const Colour fill_colour = Colour::modulate(
				colour.get(),
				rendered_polygon_on_sphere.get_fill_modulate_colour());

		// Add the filled polygon at the current location (if any) in the rendered geometries spatial partition.
		filled_polygons.add_filled_polygon(
				*polygon_on_sphere,
				fill_colour,
				d_current_spatial_partition_location.get());

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
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_coloured_edge_surface_mesh(
	const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh &rendered_coloured_edge_surface_mesh)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

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

		const GPlatesMaths::GreatCircleArc edge[1] =
		{
				GPlatesMaths::GreatCircleArc::create(
						mesh_vertices[mesh_edge.vertex_indices[0]],
						mesh_vertices[mesh_edge.vertex_indices[1]])
		};

		// Paint the current single great circle arc edge (it might get tessellated into smaller arcs).
		paint_great_circle_arcs(edge, edge + 1, colour.get(), stream);
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_coloured_triangle_surface_mesh(
		const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh &rendered_coloured_triangle_surface_mesh)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	GPlatesOpenGL::GLFilledPolygonsGlobeView::filled_drawables_type &filled_polygons =
			d_layer_painter->translucent_drawables_on_the_sphere.get_filled_polygons_globe_view();

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

		// Add the current filled triangle.
		filled_polygons.add_filled_triangle_to_mesh(
				mesh_vertices[mesh_triangle.vertex_indices[0]],
				mesh_vertices[mesh_triangle.vertex_indices[1]],
				mesh_vertices[mesh_triangle.vertex_indices[2]],
				colour.get());
	}

	// Add the filled mesh at the current location (if any) in the rendered geometries spatial partition.
	filled_polygons.end_filled_triangle_mesh(d_current_spatial_partition_location.get());
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_resolved_raster(
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
					rendered_resolved_raster.get_raster_modulate_colour(),
					rendered_resolved_raster.get_normal_map_height_field_scale_factor()));
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_resolved_scalar_field_3d(
		const GPlatesViewOperations::RenderedResolvedScalarField3D &rendered_resolved_scalar_field)
{
	if (d_paint_region != PAINT_SUB_SURFACE)
	{
		return;
	}

	GPlatesViewOperations::ScalarField3DRenderParameters render_params(
			rendered_resolved_scalar_field.get_render_parameters());

	// If we have been hinted to improve performance and the user has enabled this for the
	// current layer then reduce the sampling rate (at the cost of quality).
	// This is typically done during globe rotation when the mouse is dragged.
	if (d_improve_performance_reduce_quality_hint)
	{
		GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance quality_performance =
				render_params.get_quality_performance();
		if (quality_performance.enable_reduce_rate_during_drag_globe)
		{
			quality_performance.sampling_rate /= quality_performance.reduce_rate_during_drag_globe;

			// Keep the sampling rate above a minimum threshold.
			if (quality_performance.sampling_rate < 10)
			{
				quality_performance.sampling_rate = 10;
			}
		}
		render_params.set_quality_performance(quality_performance);
	}

	// Queue the scalar field for painting.
	d_layer_painter->scalar_fields.push_back(
			LayerPainter::ScalarField3DDrawable(
					rendered_resolved_scalar_field.get_resolved_scalar_field_3d(),
					render_params));
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_radial_arrow(
		const GPlatesViewOperations::RenderedRadialArrow &rendered_radial_arrow)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	const GPlatesMaths::Vector3D start(
			rendered_radial_arrow.get_position().position_vector());

	// Calculate position from start point along radial/normal direction to
	// end point off the globe. The length of the arrow in world space
	// is inversely proportional to the zoom or magnification.
	const GPlatesMaths::Vector3D end = (1 +
			d_inverse_zoom_factor * rendered_radial_arrow.get_arrow_projected_length()) * start;

	const GPlatesMaths::Vector3D arrowline = end - start;
	const GPlatesMaths::real_t arrowline_length = arrowline.magnitude();

	// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
	if (arrowline_length == 0)
	{
		return;
	}

	// Cull the rendered arrow if it's outside the view frustum.
	// This helps a lot for high zoom levels where the zoom-dependent binning of rendered arrows
	// creates a large number of arrow bins to such an extent that CPU profiling shows the drawing
	// of each arrow (setting up the arrow geometry) to consume most of the rendering time.
	// Note that the zoom-dependent binning cannot take advantage of the rendered geometries
	// spatial partition (because arrows are off the sphere and also arrow length is not known
	// ahead of time so bounds cannot be determined for placement in spatial partition) and hence
	// is not affected by our hierarchical view frustum culling in 'get_visible_rendered_geometries()'.
	//
	// Use a bounding sphere around the arrow - we also know that the arrowhead will always fit
	// within this bounding sphere because it's axis is never longer than the arrow line and
	// the angle of its cone (relative to the arrow line) is 45 degrees, and the maximum arrowhead
	// length is typically limited to half the arrowline length.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_frustum_planes,
			GPLATES_ASSERTION_SOURCE);
	if (!GPlatesOpenGL::GLIntersect::intersect_sphere_frustum(
		GPlatesOpenGL::GLIntersect::Sphere(
				start + 0.5 * arrowline/*arrow midpoint*/,
				0.5 * arrowline_length/*arrow radius*/),
		d_frustum_planes->get_planes(),
		GPlatesOpenGL::GLFrustum::ALL_PLANES_ACTIVE_MASK))
	{
		return;
	}

	//
	// Render the arrow.
	//

	boost::optional<Colour> arrow_colour = rendered_radial_arrow.get_arrow_colour().get_colour(d_colour_scheme);
	if (!arrow_colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_arrow_colour = Colour::to_rgba8(*arrow_colour);

	const GPlatesMaths::UnitVector3D &arrowline_unit_vector =
			rendered_radial_arrow.get_position().position_vector();

	GPlatesMaths::real_t arrowhead_size =
			d_inverse_zoom_factor * rendered_radial_arrow.get_arrowhead_projected_size();

	const GPlatesMaths::real_t arrowline_width =
			d_inverse_zoom_factor * rendered_radial_arrow.get_projected_arrowline_width();
	
	paint_arrow(
			start,
			end,
			arrowline_unit_vector,
			arrowline_width,
			arrowhead_size,
			rgba8_arrow_colour,
			d_layer_painter->drawables_off_the_sphere.get_axially_symmetric_mesh_triangles_stream());

	//
	// Render the symbol.
	//

    boost::optional<Colour> symbol_colour = rendered_radial_arrow.get_symbol_colour().get_colour(d_colour_scheme);
    if (!symbol_colour)
    {
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_symbol_colour = Colour::to_rgba8(*symbol_colour);

	// The symbol is a small circle with diameter equal to the arrowline width.
	const GPlatesMaths::real_t small_circle_radius = 0.5 * arrowline_width;
	const GPlatesMaths::SmallCircle small_circle = GPlatesMaths::SmallCircle::create_colatitude(
			rendered_radial_arrow.get_position().position_vector(),
			small_circle_radius);

	std::vector<GPlatesMaths::PointOnSphere> small_circle_points;
	tessellate(small_circle_points, small_circle, SMALL_CIRCLE_ANGULAR_INCREMENT);

	// Draw the small circle outline.
	// We do this even if we're filling the small circle because it makes it nicely visible around
	// the base of the arrow body because it extends out past the cylinder of the arrow body.

	// The factor of 2 makes the small circle nicely visible around the base of the arrow body.
	const float small_circle_line_width = 2.0f * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the stream for the small circle lines.
	stream_primitives_type &small_circle_line_stream =
			d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(small_circle_line_width);

	// Used to add a line loop to the stream.
	stream_primitives_type::LineLoops stream_small_circle_line_loops(small_circle_line_stream);
	stream_small_circle_line_loops.begin_line_loop();
	for (unsigned int i = 0; i < small_circle_points.size(); ++i)
	{
		stream_small_circle_line_loops.add_vertex(
				coloured_vertex_type(small_circle_points[i].position_vector(), rgba8_symbol_colour));
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
					coloured_vertex_type(rendered_radial_arrow.get_position().position_vector(), rgba8_symbol_colour));

		// Add small circle points.
		for (unsigned int i = 0; i < small_circle_points.size(); ++i)
		{
			stream_triangle_fans.add_vertex(
					coloured_vertex_type(small_circle_points[i].position_vector(), rgba8_symbol_colour));
		}

		stream_triangle_fans.end_triangle_fan();
	}

	// Draw the small circle centre point.
	if (rendered_radial_arrow.get_symbol_type() == GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_POINT)
	{
		// Factor of 2 makes the makes the point more visible when the cylinder of the arrow body
		// is transparent.
		const float point_size = 2.0f * POINT_SIZE_ADJUSTMENT * d_scale;
		stream_primitives_type &point_stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_points_stream(point_size);

		stream_primitives_type::Points stream_points(point_stream);
		stream_points.begin_points();
		stream_points.add_vertex(
				coloured_vertex_type(rendered_radial_arrow.get_position().position_vector(), rgba8_symbol_colour));
		stream_points.end_points();
	}

	// Draw a cross in the small circle.
	if (rendered_radial_arrow.get_symbol_type() == GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS)
	{
		// Create tangent space where 'y' direction points to the north pole.
		const GPlatesMaths::Vector3D tangent_space_z(rendered_radial_arrow.get_position().position_vector());
		const GPlatesMaths::Vector3D tangent_space_x_dir =
				cross(GPlatesMaths::UnitVector3D::zBasis(), tangent_space_z);
		const GPlatesMaths::Vector3D tangent_space_x =
				(tangent_space_x_dir.magSqrd() > 0)
				? GPlatesMaths::Vector3D(tangent_space_x_dir.get_normalisation())
				: GPlatesMaths::Vector3D(
						generate_perpendicular(rendered_radial_arrow.get_position().position_vector()));
		const GPlatesMaths::Vector3D tangent_space_y = cross(tangent_space_z, tangent_space_x);

		// The factor of 1.5 ensures the cross is not too fat.
		const float cross_line_width = 1.5f * LINE_WIDTH_ADJUSTMENT * d_scale;

		// Get the stream for the cross lines.
		stream_primitives_type &cross_line_stream =
				d_layer_painter->translucent_drawables_on_the_sphere.get_lines_stream(cross_line_width);

		stream_primitives_type::LineStrips stream_cross_line_strips(cross_line_stream);

		stream_cross_line_strips.begin_line_strip();
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(tangent_space_z - small_circle_radius * tangent_space_x, rgba8_symbol_colour));
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(tangent_space_z + small_circle_radius * tangent_space_x, rgba8_symbol_colour));
		stream_cross_line_strips.end_line_strip();

		stream_cross_line_strips.begin_line_strip();
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(tangent_space_z - small_circle_radius * tangent_space_y, rgba8_symbol_colour));
		stream_cross_line_strips.add_vertex(
				coloured_vertex_type(tangent_space_z + small_circle_radius * tangent_space_y, rgba8_symbol_colour));
		stream_cross_line_strips.end_line_strip();
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_tangential_arrow(
		const GPlatesViewOperations::RenderedTangentialArrow &rendered_tangential_arrow)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

	if (!d_render_settings.show_arrows())
	{
		return;
	}

	const GPlatesMaths::Vector3D start(
			rendered_tangential_arrow.get_start_position().position_vector());

	// Calculate position from start point along tangent direction to
	// end point off the globe. The length of the arrow in world space
	// is inversely proportional to the zoom or magnification.
	const GPlatesMaths::Vector3D end = GPlatesMaths::Vector3D(start) +
			d_inverse_zoom_factor * rendered_tangential_arrow.get_arrow_direction();

	const GPlatesMaths::Vector3D arrowline = end - start;
	const GPlatesMaths::real_t arrowline_length = arrowline.magnitude();

	// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
	if (arrowline_length == 0)
	{
		return;
	}

	// Cull the rendered arrow if it's outside the view frustum.
	// This helps a lot for high zoom levels where the zoom-dependent binning of rendered arrows
	// creates a large number of arrow bins to such an extent that CPU profiling shows the drawing
	// of each arrow (setting up the arrow geometry) to consume most of the rendering time.
	// Note that the zoom-dependent binning cannot take advantage of the rendered geometries
	// spatial partition (because arrows are off the sphere and also arrow length is not known
	// ahead of time so bounds cannot be determined for placement in spatial partition) and hence
	// is not affected by our hierarchical view frustum culling in 'get_visible_rendered_geometries()'.
	//
	// Use a bounding sphere around the arrow - we also know that the arrowhead will always fit
	// within this bounding sphere because it's axis is never longer than the arrow line and
	// the angle of its cone (relative to the arrow line) is 45 degrees, and the maximum arrowhead
	// length is typically limited to half the arrowline length.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_frustum_planes,
			GPLATES_ASSERTION_SOURCE);
	if (!GPlatesOpenGL::GLIntersect::intersect_sphere_frustum(
		GPlatesOpenGL::GLIntersect::Sphere(
				start + 0.5 * arrowline/*arrow midpoint*/,
				0.5 * arrowline_length/*arrow radius*/),
		d_frustum_planes->get_planes(),
		GPlatesOpenGL::GLFrustum::ALL_PLANES_ACTIVE_MASK))
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_tangential_arrow);
	if (!colour)
	{
		return;
	}

	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(*colour);

	const GPlatesMaths::UnitVector3D arrowline_unit_vector((1.0 / arrowline_length) * arrowline);

	GPlatesMaths::real_t arrowhead_size =
			d_inverse_zoom_factor * rendered_tangential_arrow.get_arrowhead_projected_size();

	const float max_ratio_arrowhead_to_arrowline_length =
			rendered_tangential_arrow.get_max_ratio_arrowhead_to_arrowline_length();

	// We want to keep the projected arrowhead size constant regardless of the
	// the length of the arrowline, except...
	//
	// ...if the ratio of arrowhead size to arrowline length is large enough then
	// we need to start scaling the arrowhead size by the arrowline length so
	// that the arrowhead disappears as the arrowline disappears.
	if (arrowhead_size > max_ratio_arrowhead_to_arrowline_length * arrowline_length)
	{
		arrowhead_size = max_ratio_arrowhead_to_arrowline_length * arrowline_length;
	}

	const GPlatesMaths::real_t arrowline_width =
			rendered_tangential_arrow.get_globe_view_ratio_arrowline_width_to_arrowhead_size() * arrowhead_size;
	
	// Render the arrow.
	paint_arrow(
			start,
			end,
			arrowline_unit_vector,
			arrowline_width,
			arrowhead_size,
			rgba8_color,
			d_layer_painter->drawables_off_the_sphere.get_axially_symmetric_mesh_triangles_stream());
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

	// Based on the "visit_rendered_tangential_arrow" code 

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

			paint_arrow_head_2D(
					gca.end_point().position_vector(),
					arrowline_unit_vector,
					arrowhead_size,
					rgba8_colour,
					d_layer_painter->translucent_drawables_on_the_sphere.get_triangles_stream());
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
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_strain_marker_symbol(
	const GPlatesViewOperations::RenderedStrainMarkerSymbol &rendered_strain_marker_symbol)
{
	if (d_paint_region != PAINT_SURFACE)
	{
		return;
	}

    boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_strain_marker_symbol);
    if (!colour)
    {
	    return;
    }

    // Define the square in the tangent plane at the north pole,
    // then rotate down to required latitude, and
    // then east/west to required longitude.
    //
    // (Two rotations are required to maintain the north-alignment).
    //
    // Can I use a new render node to do this rotation more efficiently?
    //
    // Reminder about coordinate system:
    // x is out of the screen as we look at the globe on startup.
    // y is towards right (east) as we look at the globe on startup.
    // z is up...

    // Get the point position.
    const GPlatesMaths::PointOnSphere &pos =
		    rendered_strain_marker_symbol.get_centre();

    GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);


	// rotations to locate the symbol
    static const GPlatesMaths::UnitVector3D axis1 = GPlatesMaths::UnitVector3D(0.,1.,0.);
    GPlatesMaths::Rotation r1 =
		GPlatesMaths::Rotation::create(axis1,GPlatesMaths::HALF_PI - GPlatesMaths::convert_deg_to_rad(llp.latitude()));

    static const GPlatesMaths::UnitVector3D axis2 = GPlatesMaths::UnitVector3D(0.,0.,1.);
    GPlatesMaths::Rotation r2 =
	    GPlatesMaths::Rotation::create(axis2,GPlatesMaths::convert_deg_to_rad(llp.longitude()));

    GPlatesMaths::Rotation r3 = r2*r1;


	// a rotation to orient relative to projected tanget x,y plane
	GPlatesMaths::Real angle( rendered_strain_marker_symbol.get_angle() );
//qDebug() << "visit_rendered_strain_marker_symbol: angle = " << angle; 

    static const GPlatesMaths::UnitVector3D axis_z = GPlatesMaths::UnitVector3D(0.,0.,1.);
    GPlatesMaths::Rotation r_orient =
	    GPlatesMaths::Rotation::create(axis_z,angle);

	// fairly arbitrary initial half-altitude for testing.
    double d = 0.1 * d_inverse_zoom_factor * rendered_strain_marker_symbol.get_size(); 

    // Set up the vertices of a cross with centre (0,0,1).
   	double x = rendered_strain_marker_symbol.get_scale_x();
   	double y = rendered_strain_marker_symbol.get_scale_y();
	
//qDebug() << "BEFOR: x = " << x << "; y =" << y; 

	// scale the lenght of the cross
	x = x * d;
	y = y * d;

//qDebug() << "AFTER: x = " << x << "; y =" << y; 

	// Define a scaled cross symbol at the north pole
    GPlatesMaths::Vector3D v3d_a(0.,y,1.);
    GPlatesMaths::Vector3D v3d_b(0,-y,1.);
    GPlatesMaths::Vector3D v3d_c(-x,0.,1.);
    GPlatesMaths::Vector3D v3d_d(x,0.,1.);

	// First rotate the strain marker cross to the correct angle
    v3d_a = r_orient * v3d_a;
    v3d_b = r_orient * v3d_b;
    v3d_c = r_orient * v3d_c;
    v3d_d = r_orient * v3d_d;

    // Rotate to desired location.
    v3d_a = r3 * v3d_a;
    v3d_b = r3 * v3d_b;
    v3d_c = r3 * v3d_c;
    v3d_d = r3 * v3d_d;


    coloured_vertex_type va(v3d_a.x().dval(), v3d_a.y().dval(),v3d_a.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vb(v3d_b.x().dval(), v3d_b.y().dval(),v3d_b.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vc(v3d_c.x().dval(), v3d_c.y().dval(),v3d_c.z().dval(),Colour::to_rgba8(*colour));
    coloured_vertex_type vd(v3d_d.x().dval(), v3d_d.y().dval(),v3d_d.z().dval(),Colour::to_rgba8(*colour));

    const float line_width = rendered_strain_marker_symbol.get_line_width_hint() * STRAIN_LINE_WIDTH_ADJUSTMENT * d_scale;

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
	// Get the spatial partition of rendered geometries.
	rendered_geometries_spatial_partition_type::non_null_ptr_to_const_type
			rendered_geometries_spatial_partition = d_rendered_geometry_layer.get_rendered_geometries();

	// Get rendered geometries and associated information.
	// The information is separated into two vectors in order to minimise copying during sorting.
	std::vector<RenderedGeometryInfo> rendered_geometry_infos;
	rendered_geometry_infos.reserve(rendered_geometries_spatial_partition->size());
	std::vector<RenderedGeometryOrder> rendered_geometry_orders;
	rendered_geometry_orders.reserve(rendered_geometries_spatial_partition->size());

	// Visit the spatial partition to do view-frustum culling and collect the visible rendered geometries
	// (the geometries completely outside the view frustum are not rendered).
	get_visible_rendered_geometries(
			renderer,
			rendered_geometry_infos,
			rendered_geometry_orders,
			*rendered_geometries_spatial_partition);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			rendered_geometry_infos.size() == rendered_geometry_orders.size(),
			GPLATES_ASSERTION_SOURCE);

	// We need to visit (and hence render) the rendered geometries in their render order.
	std::sort(
			rendered_geometry_orders.begin(),
			rendered_geometry_orders.end(),
			RenderedGeometryOrder::SortRenderOrder());

	// Visit the rendered geometries in render order.
	const unsigned int num_visible_rendered_geoms = rendered_geometry_infos.size();
	for (unsigned int visible_rendered_geom_index = 0;
		visible_rendered_geom_index < num_visible_rendered_geoms;
		++visible_rendered_geom_index)
	{
		const unsigned int rendered_geometry_info_index =
				rendered_geometry_orders[visible_rendered_geom_index].rendered_geometry_info_index;
		const RenderedGeometryInfo &rendered_geom_info =
				rendered_geometry_infos[rendered_geometry_info_index];

		// Visit the rendered geometry and let it know its location in the spatial partition.
		d_current_spatial_partition_location = rendered_geom_info.spatial_partition_location;
		rendered_geom_info.rendered_geometry.accept_visitor(*this);
		d_current_spatial_partition_location = boost::none;
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::get_visible_rendered_geometries(
		GPlatesOpenGL::GLRenderer &renderer,
		std::vector<RenderedGeometryInfo> &rendered_geometry_infos,
		std::vector<RenderedGeometryOrder> &rendered_geometry_orders,
		const rendered_geometries_spatial_partition_type &rendered_geometries_spatial_partition)
{
	// Visit the rendered geometries in the root of the cube quad tree.
	// These are unpartitioned and hence must be rendered regardless of the view frustum.
	// Start out at the root of the cube quad tree.
	rendered_geometries_spatial_partition_type::element_const_iterator root_elements_iter =
			rendered_geometries_spatial_partition.begin_root_elements();
	rendered_geometries_spatial_partition_type::element_const_iterator root_elements_end =
			rendered_geometries_spatial_partition.end_root_elements();
	for ( ; root_elements_iter != root_elements_end; ++root_elements_iter)
	{
		const GPlatesViewOperations::RenderedGeometryLayer::PartitionedRenderedGeometry &
				root_rendered_geom = *root_elements_iter;

		// Associate rendered geometry with its spatial partition location and render order.
		rendered_geometry_orders.push_back(
				RenderedGeometryOrder(rendered_geometry_infos.size(), root_rendered_geom.render_order));
		rendered_geometry_infos.push_back(
				RenderedGeometryInfo(root_rendered_geom.rendered_geometry));
	}

	// Create a loose oriented-bounding-box cube quad tree cache so we can do view-frustum culling
	// as we traverse the spatial partition of rendered geometries.
	// We're only visiting each subdivision node once so there's no need for caching.
	cube_subdivision_cache_type::non_null_ptr_type cube_subdivision_cache =
			cube_subdivision_cache_type::create(
					GPlatesOpenGL::GLCubeSubdivision::create());

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_frustum_planes,
			GPLATES_ASSERTION_SOURCE);

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

		get_visible_rendered_geometries_from_spatial_partition_quad_tree(
				rendered_geometry_infos,
				rendered_geometry_orders,
				root_cube_quad_tree_node_location,
				loose_quad_tree_root_node,
				*cube_subdivision_cache,
				cube_subdivision_cache_root_node,
				d_frustum_planes.get(),
				// There are six frustum planes initially active
				GPlatesOpenGL::GLFrustum::ALL_PLANES_ACTIVE_MASK);
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::get_visible_rendered_geometries_from_spatial_partition_quad_tree(
		std::vector<RenderedGeometryInfo> &rendered_geometry_infos,
		std::vector<RenderedGeometryOrder> &rendered_geometry_orders,
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
	rendered_geometries_spatial_partition_type::const_node_reference_type::element_iterator_type
			node_elements_iter = rendered_geometries_quad_tree_node.begin();
	rendered_geometries_spatial_partition_type::const_node_reference_type::element_iterator_type
			node_elements_end = rendered_geometries_quad_tree_node.end();
	for ( ; node_elements_iter != node_elements_end; ++node_elements_iter)
	{
		const GPlatesViewOperations::RenderedGeometryLayer::PartitionedRenderedGeometry &
				node_rendered_geom = *node_elements_iter;

		// Associate rendered geometry with its spatial partition location and render order.
		rendered_geometry_orders.push_back(
				RenderedGeometryOrder(rendered_geometry_infos.size(), node_rendered_geom.render_order));
		rendered_geometry_infos.push_back(
				RenderedGeometryInfo(node_rendered_geom.rendered_geometry, cube_quad_tree_node_location));
	}


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

			get_visible_rendered_geometries_from_spatial_partition_quad_tree(
					rendered_geometry_infos,
					rendered_geometry_orders,
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
		GreatCircleArcForwardIter begin_arcs,
		GreatCircleArcForwardIter end_arcs,
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
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_arrow(
		const GPlatesMaths::Vector3D &start,
		const GPlatesMaths::Vector3D &end,
		const GPlatesMaths::UnitVector3D &arrow_axis,
		const GPlatesMaths::real_t &arrowline_width,
		const GPlatesMaths::real_t &arrowhead_size,
		rgba8_t rgba8_color,
		axially_symmetric_mesh_stream_primitives_type &triangles_stream)
{
	// Find an orthonormal basis using 'arrow_axis'.
	const GPlatesMaths::UnitVector3D &arrow_z_axis = arrow_axis;
	const GPlatesMaths::UnitVector3D arrow_y_axis = generate_perpendicular(arrow_z_axis);
	const GPlatesMaths::UnitVector3D arrow_x_axis( cross(arrow_y_axis, arrow_z_axis) );

	//
	// Render the arrow head.
	//
	// We render the arrow head first because, for transparent arrows, it looks better.
	//

	paint_arrow_head_3D(
			end,
			arrow_x_axis,
			arrow_y_axis,
			arrow_z_axis,
			arrowhead_size,
			rgba8_color,
			triangles_stream);

	//
	// Render the arrow body.
	//

	const GPlatesMaths::Vector3D &centre_start_circle = start;
	const GPlatesMaths::Vector3D centre_end_circle = end - arrowhead_size * arrow_axis;

	const GPlatesMaths::real_t arrowline_half_width = 0.5 * arrowline_width;

	static const int NUM_VERTICES_IN_UNIT_CIRCLE = 8;
	static const double s_vertex_angle =
			2 * GPlatesMaths::PI / NUM_VERTICES_IN_UNIT_CIRCLE;
	static const GPlatesMaths::real_t s_unit_circle[][2] =
	{
		{ GPlatesMaths::cos(0 * s_vertex_angle), GPlatesMaths::sin(0 * s_vertex_angle) },
		{ GPlatesMaths::cos(1 * s_vertex_angle), GPlatesMaths::sin(1 * s_vertex_angle) },
		{ GPlatesMaths::cos(2 * s_vertex_angle), GPlatesMaths::sin(2 * s_vertex_angle) },
		{ GPlatesMaths::cos(3 * s_vertex_angle), GPlatesMaths::sin(3 * s_vertex_angle) },
		{ GPlatesMaths::cos(4 * s_vertex_angle), GPlatesMaths::sin(4 * s_vertex_angle) },
		{ GPlatesMaths::cos(5 * s_vertex_angle), GPlatesMaths::sin(5 * s_vertex_angle) },
		{ GPlatesMaths::cos(6 * s_vertex_angle), GPlatesMaths::sin(6 * s_vertex_angle) },
		{ GPlatesMaths::cos(7 * s_vertex_angle), GPlatesMaths::sin(7 * s_vertex_angle) }
	};
	BOOST_STATIC_ASSERT(NUM_VERTICES_IN_UNIT_CIRCLE == (sizeof(s_unit_circle) / sizeof(s_unit_circle[0])));

	// Generate the cylinder start vertices in the frame of reference of the arrow's axis.
	const GPlatesMaths::Vector3D start_circle[] =
	{
		centre_start_circle + arrowline_half_width * (s_unit_circle[0][0] * arrow_x_axis + s_unit_circle[0][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[1][0] * arrow_x_axis + s_unit_circle[1][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[2][0] * arrow_x_axis + s_unit_circle[2][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[3][0] * arrow_x_axis + s_unit_circle[3][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[4][0] * arrow_x_axis + s_unit_circle[4][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[5][0] * arrow_x_axis + s_unit_circle[5][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[6][0] * arrow_x_axis + s_unit_circle[6][1] * arrow_y_axis),
		centre_start_circle + arrowline_half_width * (s_unit_circle[7][0] * arrow_x_axis + s_unit_circle[7][1] * arrow_y_axis)
	};
	BOOST_STATIC_ASSERT(NUM_VERTICES_IN_UNIT_CIRCLE == (sizeof(start_circle) / sizeof(start_circle[0])));

	// Generate the cylinder end vertices in the frame of reference of the arrow's axis.
	const GPlatesMaths::Vector3D end_circle[] =
	{
		centre_end_circle + arrowline_half_width * (s_unit_circle[0][0] * arrow_x_axis + s_unit_circle[0][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[1][0] * arrow_x_axis + s_unit_circle[1][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[2][0] * arrow_x_axis + s_unit_circle[2][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[3][0] * arrow_x_axis + s_unit_circle[3][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[4][0] * arrow_x_axis + s_unit_circle[4][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[5][0] * arrow_x_axis + s_unit_circle[5][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[6][0] * arrow_x_axis + s_unit_circle[6][1] * arrow_y_axis),
		centre_end_circle + arrowline_half_width * (s_unit_circle[7][0] * arrow_x_axis + s_unit_circle[7][1] * arrow_y_axis)
	};
	BOOST_STATIC_ASSERT(NUM_VERTICES_IN_UNIT_CIRCLE == (sizeof(end_circle) / sizeof(end_circle[0])));

	//
	// Render the cylinder.
	//

	axially_symmetric_mesh_stream_primitives_type::Primitives stream_cylinder_mesh(triangles_stream);

	bool ok = stream_cylinder_mesh.begin_primitive(
			2 * NUM_VERTICES_IN_UNIT_CIRCLE/*max_num_vertices*/,
			3 * 2 * NUM_VERTICES_IN_UNIT_CIRCLE/*max_num_vertex_elements*/);

	// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			ok,
			GPLATES_ASSERTION_SOURCE);

	// Add the cylinder start vertices.
	for (int n = 0; n < NUM_VERTICES_IN_UNIT_CIRCLE; ++n)
	{
		stream_cylinder_mesh.add_vertex(
				axially_symmetric_mesh_vertex_type(
						start_circle[n],
						rgba8_color,
						arrow_x_axis,
						arrow_y_axis,
						arrow_z_axis,
						s_unit_circle[n][0].dval()/*model_space_x_position*/,
						s_unit_circle[n][1].dval()/*model_space_y_position*/,
						1/*radial_normal_weight*/,
						0/*axial_normal_weight*/));
	}

	// Add the cylinder end vertices.
	for (int n = 0; n < NUM_VERTICES_IN_UNIT_CIRCLE; ++n)
	{
		stream_cylinder_mesh.add_vertex(
				axially_symmetric_mesh_vertex_type(
						end_circle[n],
						rgba8_color,
						arrow_x_axis,
						arrow_y_axis,
						arrow_z_axis,
						s_unit_circle[n][0].dval()/*model_space_x_position*/,
						s_unit_circle[n][1].dval()/*model_space_y_position*/,
						1/*radial_normal_weight*/,
						0/*axial_normal_weight*/));
	}

	// Add the cylinder vertex elements.
	// Make outward facing triangles counter-clockwise (this is the default front-facing OpenGL mode).
	for (int n = 0; n < NUM_VERTICES_IN_UNIT_CIRCLE - 1; ++n)
	{
		// First triangle of current quad.
		stream_cylinder_mesh.add_vertex_element(n);
		stream_cylinder_mesh.add_vertex_element(n + 1);
		stream_cylinder_mesh.add_vertex_element(n + NUM_VERTICES_IN_UNIT_CIRCLE);

		// Second triangle of current quad.
		stream_cylinder_mesh.add_vertex_element(n + 1 + NUM_VERTICES_IN_UNIT_CIRCLE);
		stream_cylinder_mesh.add_vertex_element(n + NUM_VERTICES_IN_UNIT_CIRCLE);
		stream_cylinder_mesh.add_vertex_element(n + 1);
	}

	// Wrap-around cylinder vertex elements.
	// First triangle of current quad.
	stream_cylinder_mesh.add_vertex_element(NUM_VERTICES_IN_UNIT_CIRCLE - 1);
	stream_cylinder_mesh.add_vertex_element(0);
	stream_cylinder_mesh.add_vertex_element(2 * NUM_VERTICES_IN_UNIT_CIRCLE - 1);
	// Second triangle of current quad.
	stream_cylinder_mesh.add_vertex_element(NUM_VERTICES_IN_UNIT_CIRCLE);
	stream_cylinder_mesh.add_vertex_element(2 * NUM_VERTICES_IN_UNIT_CIRCLE - 1);
	stream_cylinder_mesh.add_vertex_element(0);

	stream_cylinder_mesh.end_primitive();

	//
	// Render the cap to close off the start of the cylinder.
	//
	// We don't need one at the end of the cylinder because the arrow head closes it off for us
	// (because the cylinder abuts the arrow head cone).
	//

	axially_symmetric_mesh_stream_primitives_type::Primitives stream_start_cap_mesh(triangles_stream);

	ok = stream_start_cap_mesh.begin_primitive(
			NUM_VERTICES_IN_UNIT_CIRCLE + 1/*max_num_vertices*/,
			3 * NUM_VERTICES_IN_UNIT_CIRCLE/*max_num_vertex_elements*/);

	// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			ok,
			GPLATES_ASSERTION_SOURCE);

	// Add the triangle fan vertex at the centre of the start cap.
	stream_start_cap_mesh.add_vertex(
			axially_symmetric_mesh_vertex_type(
					start,
					rgba8_color,
					arrow_x_axis,
					arrow_y_axis,
					arrow_z_axis,
					0/*model_space_x_position*/,
					0/*model_space_y_position*/,
					0/*radial_normal_weight*/,
					-1/*axial_normal_weight*/));

	// Add the start cap vertices.
	// Note that we can't share the cylinder vertices because the radial/axial normal weights are different.
	for (int n = 0; n < NUM_VERTICES_IN_UNIT_CIRCLE; ++n)
	{
		stream_start_cap_mesh.add_vertex(
				axially_symmetric_mesh_vertex_type(
						start_circle[n],
						rgba8_color,
						arrow_x_axis,
						arrow_y_axis,
						arrow_z_axis,
						s_unit_circle[n][0].dval()/*model_space_x_position*/,
						s_unit_circle[n][1].dval()/*model_space_y_position*/,
						0/*radial_normal_weight*/,
						-1/*axial_normal_weight*/));
	}

	// Add the start cap vertex elements.
	// Make outward facing triangles counter-clockwise (this is the default front-facing OpenGL mode).
	for (int n = 0; n < NUM_VERTICES_IN_UNIT_CIRCLE - 1; ++n)
	{
		stream_start_cap_mesh.add_vertex_element(0); // Fan centre.
		stream_start_cap_mesh.add_vertex_element(n + 2);
		stream_start_cap_mesh.add_vertex_element(n + 1);
	}
	// Wrap-around start circle vertex elements.
	stream_start_cap_mesh.add_vertex_element(0); // Fan centre.
	stream_start_cap_mesh.add_vertex_element(1);
	stream_start_cap_mesh.add_vertex_element(NUM_VERTICES_IN_UNIT_CIRCLE);

	stream_start_cap_mesh.end_primitive();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_arrow_head_3D(
		const GPlatesMaths::Vector3D &apex,
		const GPlatesMaths::UnitVector3D &cone_x_axis,
		const GPlatesMaths::UnitVector3D &cone_y_axis,
		const GPlatesMaths::UnitVector3D &cone_z_axis,
		const GPlatesMaths::real_t &cone_axis_mag,
		rgba8_t rgba8_color,
		axially_symmetric_mesh_stream_primitives_type &triangles_stream)
{
	const GPlatesMaths::Vector3D cone_axis = cone_axis_mag * cone_z_axis;
	const GPlatesMaths::Vector3D centre_base_circle = apex - cone_axis;

	static const int NUM_VERTICES_IN_BASE_UNIT_CIRCLE = 12;
	static const double s_vertex_angle =
			2 * GPlatesMaths::PI / NUM_VERTICES_IN_BASE_UNIT_CIRCLE;
	static const GPlatesMaths::real_t s_base_unit_circle[][2] =
	{
		{ GPlatesMaths::cos(0 * s_vertex_angle), GPlatesMaths::sin(0 * s_vertex_angle) },
		{ GPlatesMaths::cos(1 * s_vertex_angle), GPlatesMaths::sin(1 * s_vertex_angle) },
		{ GPlatesMaths::cos(2 * s_vertex_angle), GPlatesMaths::sin(2 * s_vertex_angle) },
		{ GPlatesMaths::cos(3 * s_vertex_angle), GPlatesMaths::sin(3 * s_vertex_angle) },
		{ GPlatesMaths::cos(4 * s_vertex_angle), GPlatesMaths::sin(4 * s_vertex_angle) },
		{ GPlatesMaths::cos(5 * s_vertex_angle), GPlatesMaths::sin(5 * s_vertex_angle) },
		{ GPlatesMaths::cos(6 * s_vertex_angle), GPlatesMaths::sin(6 * s_vertex_angle) },
		{ GPlatesMaths::cos(7 * s_vertex_angle), GPlatesMaths::sin(7 * s_vertex_angle) },
		{ GPlatesMaths::cos(8 * s_vertex_angle), GPlatesMaths::sin(8 * s_vertex_angle) },
		{ GPlatesMaths::cos(9 * s_vertex_angle), GPlatesMaths::sin(9 * s_vertex_angle) },
		{ GPlatesMaths::cos(10 * s_vertex_angle), GPlatesMaths::sin(10 * s_vertex_angle) },
		{ GPlatesMaths::cos(11 * s_vertex_angle), GPlatesMaths::sin(11 * s_vertex_angle) }
	};
	BOOST_STATIC_ASSERT(NUM_VERTICES_IN_BASE_UNIT_CIRCLE == (sizeof(s_base_unit_circle) / sizeof(s_base_unit_circle[0])));

	// Radius of cone base circle is proportional to the distance from the apex to
	// the centre of the base circle.
	const GPlatesMaths::real_t radius_cone_circle = ARROWHEAD_BASE_HEIGHT_RATIO * cone_axis_mag;

	// Generate the cone vertices in the frame of reference of the cone axis.
	const GPlatesMaths::Vector3D cone_base_circle[] =
	{
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[0][0] * cone_x_axis + s_base_unit_circle[0][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[1][0] * cone_x_axis + s_base_unit_circle[1][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[2][0] * cone_x_axis + s_base_unit_circle[2][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[3][0] * cone_x_axis + s_base_unit_circle[3][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[4][0] * cone_x_axis + s_base_unit_circle[4][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[5][0] * cone_x_axis + s_base_unit_circle[5][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[6][0] * cone_x_axis + s_base_unit_circle[6][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[7][0] * cone_x_axis + s_base_unit_circle[7][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[8][0] * cone_x_axis + s_base_unit_circle[8][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[9][0] * cone_x_axis + s_base_unit_circle[9][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[10][0] * cone_x_axis + s_base_unit_circle[10][1] * cone_y_axis),
		centre_base_circle + radius_cone_circle * (s_base_unit_circle[11][0] * cone_x_axis + s_base_unit_circle[11][1] * cone_y_axis)
	};
	BOOST_STATIC_ASSERT(NUM_VERTICES_IN_BASE_UNIT_CIRCLE == (sizeof(cone_base_circle) / sizeof(cone_base_circle[0])));

	const GLfloat radial_normal_weight = COSINE_ARROWHEAD_BASE_HEIGHT_RATIO;
	const GLfloat axial_normal_weight = SINE_ARROWHEAD_BASE_HEIGHT_RATIO;

	//
	// Render the curved surface of the cone.
	//

	axially_symmetric_mesh_stream_primitives_type::Primitives stream_cone_surface_mesh(triangles_stream);

	bool ok = stream_cone_surface_mesh.begin_primitive(
			NUM_VERTICES_IN_BASE_UNIT_CIRCLE + 1/*max_num_vertices*/,
			3 * NUM_VERTICES_IN_BASE_UNIT_CIRCLE/*max_num_vertex_elements*/);

	// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			ok,
			GPLATES_ASSERTION_SOURCE);

	// Add the cone apex vertex.
	stream_cone_surface_mesh.add_vertex(
			axially_symmetric_mesh_vertex_type(
					apex,
					rgba8_color,
					cone_x_axis,
					cone_y_axis,
					cone_z_axis,
					0/*model_space_x_position*/,
					0/*model_space_y_position*/,
					radial_normal_weight,
					axial_normal_weight));

	// Add the cone base circle vertices.
	for (int n = 0; n < NUM_VERTICES_IN_BASE_UNIT_CIRCLE; ++n)
	{
		stream_cone_surface_mesh.add_vertex(
				axially_symmetric_mesh_vertex_type(
						cone_base_circle[n],
						rgba8_color,
						cone_x_axis,
						cone_y_axis,
						cone_z_axis,
						s_base_unit_circle[n][0].dval()/*model_space_x_position*/,
						s_base_unit_circle[n][1].dval()/*model_space_y_position*/,
						radial_normal_weight,
						axial_normal_weight));
	}

	// Add the cone surface vertex elements.
	// Make outward facing triangles counter-clockwise (this is the default front-facing OpenGL mode).
	for (int n = 0; n < NUM_VERTICES_IN_BASE_UNIT_CIRCLE - 1; ++n)
	{
		stream_cone_surface_mesh.add_vertex_element(0); // Fan centre.
		stream_cone_surface_mesh.add_vertex_element(n + 1);
		stream_cone_surface_mesh.add_vertex_element(n + 2);
	}
	// Wrap-around cone base circle vertex elements.
	stream_cone_surface_mesh.add_vertex_element(0); // Fan centre.
	stream_cone_surface_mesh.add_vertex_element(NUM_VERTICES_IN_BASE_UNIT_CIRCLE);
	stream_cone_surface_mesh.add_vertex_element(1);

	stream_cone_surface_mesh.end_primitive();

	//
	// Render the cone cap to close off the cone.
	//

	axially_symmetric_mesh_stream_primitives_type::Primitives stream_cone_cap_mesh(triangles_stream);

	ok = stream_cone_cap_mesh.begin_primitive(
			NUM_VERTICES_IN_BASE_UNIT_CIRCLE + 1/*max_num_vertices*/,
			3 * NUM_VERTICES_IN_BASE_UNIT_CIRCLE/*max_num_vertex_elements*/);

	// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			ok,
			GPLATES_ASSERTION_SOURCE);

	// Add the triangle fan vertex at the centre of the cone base circle.
	stream_cone_cap_mesh.add_vertex(
			axially_symmetric_mesh_vertex_type(
					centre_base_circle,
					rgba8_color,
					cone_x_axis,
					cone_y_axis,
					cone_z_axis,
					0/*model_space_x_position*/,
					0/*model_space_y_position*/,
					0/*radial_normal_weight*/,
					-1/*axial_normal_weight*/));

	// Add the cone cap vertices.
	// Note that we can't share the cone base circles vertices from the cone surface above
	// because the radial/axial normal weights are different.
	for (int n = 0; n < NUM_VERTICES_IN_BASE_UNIT_CIRCLE; ++n)
	{
		stream_cone_cap_mesh.add_vertex(
				axially_symmetric_mesh_vertex_type(
						cone_base_circle[n],
						rgba8_color,
						cone_x_axis,
						cone_y_axis,
						cone_z_axis,
						s_base_unit_circle[n][0].dval()/*model_space_x_position*/,
						s_base_unit_circle[n][1].dval()/*model_space_y_position*/,
						0/*radial_normal_weight*/,
						-1/*axial_normal_weight*/));
	}

	// Add the cone cap vertex elements.
	// Make outward facing triangles counter-clockwise (this is the default front-facing OpenGL mode).
	for (int n = 0; n < NUM_VERTICES_IN_BASE_UNIT_CIRCLE - 1; ++n)
	{
		stream_cone_cap_mesh.add_vertex_element(0); // Fan centre.
		stream_cone_cap_mesh.add_vertex_element(n + 2);
		stream_cone_cap_mesh.add_vertex_element(n + 1);
	}
	// Wrap-around cone base circle vertex elements.
	stream_cone_cap_mesh.add_vertex_element(0); // Fan centre.
	stream_cone_cap_mesh.add_vertex_element(1);
	stream_cone_cap_mesh.add_vertex_element(NUM_VERTICES_IN_BASE_UNIT_CIRCLE);

	stream_cone_cap_mesh.end_primitive();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_arrow_head_2D(
		const GPlatesMaths::UnitVector3D &apex,
		const GPlatesMaths::UnitVector3D &direction,
		const GPlatesMaths::real_t &size,
		rgba8_t rgba8_color,
		stream_primitives_type &triangles_stream)
{
	// A vector perpendicular to the arrow direction, for forming the base of the triangle.
	const GPlatesMaths::UnitVector3D perpendicular_direction =
			GPlatesMaths::cross(direction, apex).get_normalisation();

	const GPlatesMaths::Vector3D base = GPlatesMaths::Vector3D(apex) - size * direction;
	const GPlatesMaths::Vector3D base_corners[2] =
	{
		base - ARROWHEAD_BASE_HEIGHT_RATIO * size * perpendicular_direction,
		base + ARROWHEAD_BASE_HEIGHT_RATIO * size * perpendicular_direction
	};

	stream_primitives_type::Triangles stream_triangles(triangles_stream);

	stream_triangles.begin_triangles();

	stream_triangles.add_vertex(coloured_vertex_type(apex, rgba8_color));
	stream_triangles.add_vertex(coloured_vertex_type(base_corners[0], rgba8_color));
	stream_triangles.add_vertex(coloured_vertex_type(base_corners[1], rgba8_color));

	stream_triangles.end_triangles();
}
