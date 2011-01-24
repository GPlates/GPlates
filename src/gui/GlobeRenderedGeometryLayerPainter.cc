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
#include <boost/foreach.hpp>
#include <opengl/OpenGL.h>

#include "ColourScheme.h"
#include "GlobeRenderedGeometryLayerPainter.h"

#include "RasterColourPalette.h"

#include "maths/EllipseGenerator.h"
#include "maths/Real.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/MathsUtils.h"

#include "opengl/GLBlendState.h"
#include "opengl/GLCompositeDrawable.h"
#include "opengl/GLCompositeStateSet.h"
#include "opengl/GLDrawable.h"
#include "opengl/GLFragmentTestStates.h"
#include "opengl/GLMaskBuffersState.h"
#include "opengl/GLPointLinePolygonState.h"
#include "opengl/GLRenderGraphDrawableNode.h"
#include "opengl/GLText3DNode.h"
#include "opengl/GLUNurbsRenderer.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexElementArray.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/XsString.h"

#include "view-operations/RenderedArrowedPolyline.h"
#include "view-operations/RenderedDirectionArrow.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"
#include "view-operations/RenderedGeometryUtils.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedResolvedRaster.h"
#include "view-operations/RenderedString.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"

namespace 
{
	/**
	 * We will draw a NURBS if the two endpoints of a great circle arc are
	 * more than PI/36 radians (= 5 degrees) apart.
	 */
	const double GCA_DISTANCE_THRESHOLD_DOT = std::cos(GPlatesMaths::PI / 36.0);
	const double TWO_PI = 2. * GPlatesMaths::PI;
} // anonymous namespace


const float GPlatesGui::GlobeRenderedGeometryLayerPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::GlobeRenderedGeometryLayerPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;


template <typename GreatCircleArcForwardIter>
void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_great_circle_arcs(
		const GreatCircleArcForwardIter &begin_arcs,
		const GreatCircleArcForwardIter &end_arcs,
		const Colour &colour,
		LineDrawables &line_drawables,
		GPlatesOpenGL::GLUNurbsRenderer &nurbs_renderer)
{
	// Convert colour from floats to bytes to use less vertex memory.
	const rgba8_t rgba8_color = Colour::to_rgba8(colour);

	// Used to add line strips to the stream.
	GPlatesOpenGL::GLStreamLineStrips<vertex_type> stream_line_strips(*line_drawables.stream);

	stream_line_strips.begin_line_strip();

	// Iterate over the great circle arcs.
	for (GreatCircleArcForwardIter gca_iter = begin_arcs ; gca_iter != end_arcs; ++gca_iter)
	{
		const GPlatesMaths::GreatCircleArc &gca = *gca_iter;

		// Draw a NURBS if the two endpoints of the arc are far enough apart.
		if (gca.dot_of_endpoints() < GCA_DISTANCE_THRESHOLD_DOT)
		{
			// We've interrupted our regular line strip so we need to end any current strip.
			stream_line_strips.end_line_strip();

			line_drawables.nurbs_drawables.push_back(
					d_nurbs_renderer->draw_great_circle_arc(
							gca.start_point(), gca.end_point(), colour));

			// Start a new line strip.
			stream_line_strips.begin_line_strip();
		}
		else
		{
			if (stream_line_strips.is_start_of_strip())
			{
				// Get the start position of the great circle arc.
				const GPlatesMaths::UnitVector3D &start = gca.start_point().position_vector();

				// Vertex representing the start point's position and colour.
				const vertex_type start_vertex = {
						start.x().dval(), start.y().dval(), start.z().dval(), rgba8_color };

				stream_line_strips.add_vertex(start_vertex);
			}

			// Get the end position of the great circle arc.
			const GPlatesMaths::UnitVector3D &end = gca.end_point().position_vector();

			// Vertex representing the end point's position and colour.
			const vertex_type end_vertex = {
					end.x().dval(), end.y().dval(), end.z().dval(), rgba8_color };

			stream_line_strips.add_vertex(end_vertex);
		}
	}

	stream_line_strips.end_line_strip();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &rendered_layer_node)
{
	//
	// Create one render graph node to parent all primitives whose geometry is constrained
	// to the sphere (regardless of whether they are opaque or translucent).
	// Create another render graph node to parent any primitives whose geometry is not
	// constrained to the sphere.
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
	// or the polyline should be drawn on top in its entirely depending on the order they are
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

	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type non_raster_primitives_on_the_sphere_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type raster_primitives_on_the_sphere_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type primitives_off_the_sphere_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();

	// Only setting for non-raster primitives.
	set_state_for_non_raster_primitives_on_the_sphere(*non_raster_primitives_on_the_sphere_node);

	// Only setting for raster primitives.
	set_state_for_raster_primitives_on_the_sphere(*raster_primitives_on_the_sphere_node);

	set_state_for_primitives_off_the_sphere(*primitives_off_the_sphere_node);

	// Add each node as a child of the render layer node.
	// The order they are rendered does not matter.
	rendered_layer_node->add_child_node(primitives_off_the_sphere_node);
	rendered_layer_node->add_child_node(non_raster_primitives_on_the_sphere_node);
	rendered_layer_node->add_child_node(raster_primitives_on_the_sphere_node);


	//
	// To further complicate matters we also separate the non-raster primitives *on* the sphere into
	// two groups, opaque and translucent. This is because they have different alpha-blending and
	// point/line anti-aliasing states. By adding primitives to each group (render graph node)
	// we minimise changing OpenGL state back and forth (which can be costly).
	//
	// We don't need two groups for the primitives *off* the sphere because they should
	// consist only of opaque primitives (see comments above).
	//

	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type opaque_primitives_on_the_sphere_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type translucent_primitives_on_the_sphere_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();

	set_translucent_state(*translucent_primitives_on_the_sphere_node);

	// Add each node as a child of the primitives *on* the sphere node.
	// The order they are rendered does not matter.
	non_raster_primitives_on_the_sphere_node->add_child_node(opaque_primitives_on_the_sphere_node);
	non_raster_primitives_on_the_sphere_node->add_child_node(translucent_primitives_on_the_sphere_node);

	// Create a special node for adding 3D text.
	// This is because the text is converted from 3D space to 2D window coordinates and hence
	// is effectively *off* the sphere but it can't have depth writes enabled (because we don't
	// know the depth since its rendered as 2D).
	// We add it last so it gets drawn last for this layer which should put it on top.
	// However if another rendered layer is drawn after this one then the text will be overwritten
	// and not appear to hover in 3D space - currently is looks like the only layer that uses
	// text is the Measure Distance tool layer and that's the last layer.
	// Also it depends on how the text is meant to interact with other *off* the sphere geometries
	// such as rendered arrows (should it be on top or interleave depending on depth).
	// FIXME: We might be able to draw text as 3D and turn depth writes on (however the
	// alpha-blending could cause some visual artifacts as described above).
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type text_off_the_sphere_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();
	rendered_layer_node->add_child_node(text_off_the_sphere_node);

	set_state_for_text_off_the_sphere(*text_off_the_sphere_node);

	// Initialise our paint parameters so our visit methods can access them.
	d_paint_params = PaintParams(
			raster_primitives_on_the_sphere_node,
			text_off_the_sphere_node);

	// Visit the rendered geometries in the rendered layer.
	visit_rendered_geoms();

	//
	// Collect the point, line and polygon drawables and add them to the render graph
	// with the appropriate state (such as point size, line width).
	//
	d_paint_params->drawables_off_the_sphere.add_drawables(
			*primitives_off_the_sphere_node);
	d_paint_params->opaque_drawables_on_the_sphere.add_drawables(
			*opaque_primitives_on_the_sphere_node);
	d_paint_params->translucent_drawables_on_the_sphere.add_drawables(
			*translucent_primitives_on_the_sphere_node);

	// These parameters are only used for the duration of this 'paint()' method.
	d_paint_params = boost::none;
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_geoms()
{
	// Visit each RenderedGeometry.
	std::for_each(
		d_rendered_geometry_layer.rendered_geometry_begin(),
		d_rendered_geometry_layer.rendered_geometry_end(),
		boost::bind(&GPlatesViewOperations::RenderedGeometry::accept_visitor, _1, boost::ref(*this)));
}


template <class T>
boost::optional<GPlatesGui::Colour>
GPlatesGui::GlobeRenderedGeometryLayerPainter::get_colour_of_rendered_geometry(
		const T &geom)
{
	return geom.get_colour().get_colour(d_colour_scheme);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_point_on_sphere(
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
	GPlatesOpenGL::GLStreamPrimitives<vertex_type> &stream =
			d_paint_params->translucent_drawables_on_the_sphere.get_point_drawables(point_size);

	// Get the point position.
	const GPlatesMaths::UnitVector3D &pos =
			rendered_point_on_sphere.get_point_on_sphere().position_vector();

	// Vertex representing the point's position and colour.
	// Convert colour from floats to bytes to use less vertex memory.
	const vertex_type vertex = {
			pos.x().dval(), pos.y().dval(), pos.z().dval(), Colour::to_rgba8(*colour) };

	// Used to add points to the stream.
	GPlatesOpenGL::GLStreamPoints<vertex_type> stream_points(stream);

	stream_points.begin_points();
	stream_points.add_vertex(vertex);
	stream_points.end_points();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_multi_point_on_sphere(
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
	GPlatesOpenGL::GLStreamPrimitives<vertex_type> &stream =
			d_paint_params->translucent_drawables_on_the_sphere.get_point_drawables(point_size);

	// Used to add points to the stream.
	GPlatesOpenGL::GLStreamPoints<vertex_type> stream_points(stream);

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
		const vertex_type vertex = { pos.x().dval(), pos.y().dval(), pos.z().dval(), rgba8_color };

		stream_points.add_vertex(vertex);
	}

	stream_points.end_points();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_polyline_on_sphere(
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

	// Get the drawables for lines of the current line width.
	LineDrawables &line_drawables =
			d_paint_params->translucent_drawables_on_the_sphere.get_line_drawables(line_width);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_polyline_on_sphere.get_polyline_on_sphere();

	paint_great_circle_arcs(
			polyline_on_sphere->begin(),
			polyline_on_sphere->end(),
			colour.get(),
			line_drawables,
			*d_nurbs_renderer);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_polygon_on_sphere(
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

	const float line_width =
			rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the drawables for lines of the current line width.
	LineDrawables &line_drawables =
			d_paint_params->translucent_drawables_on_the_sphere.get_line_drawables(line_width);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
			rendered_polygon_on_sphere.get_polygon_on_sphere();

	paint_great_circle_arcs(
			polygon_on_sphere->begin(),
			polygon_on_sphere->end(),
			colour.get(),
			line_drawables,
			*d_nurbs_renderer);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_resolved_raster(
		const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster)
{
	RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
		rendered_resolved_raster.get_raster_colour_palette();

	// Look up the raster band name selected by the user and
	// find a match in the list of band names in the raster.
	// Use this to find the correct proxied raster.
	unsigned int source_proxied_raster_index = 0;
	const GPlatesPropertyValues::TextContent &raster_band_name =
		rendered_resolved_raster.get_raster_band_name();

	unsigned int band_name_index = 0;
	BOOST_FOREACH(
			const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type &band_name,
			rendered_resolved_raster.get_raster_band_names())
	{
		if (band_name->value() == raster_band_name)
		{
			source_proxied_raster_index = band_name_index;
			break;
		}
		++band_name_index;
	}

	// Get the proxied rasters.
	// We'll look these up using a raster band name from the colouring options.
	const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters =
			rendered_resolved_raster.get_proxied_rasters();
	if (source_proxied_raster_index >= proxied_rasters.size())
	{
		// Cannot index into raster array so nothing we can do.
		return;
	}
	const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster =
			proxied_rasters[source_proxied_raster_index];

	// Look for the age grid proxied raster if there's an age grid.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> age_grid_raster;
	if (rendered_resolved_raster.get_age_grid_raster_band_names())
	{
		static const GPlatesPropertyValues::TextContent AGE_BAND_NAME(GPlatesUtils::UnicodeString("age"));

		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
				age_grid_proxied_rasters = rendered_resolved_raster.get_age_grid_proxied_rasters();
		if (age_grid_proxied_rasters)
		{
			unsigned int age_grid_band_name_index = 0;
			BOOST_FOREACH(
					const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type &band_name,
					rendered_resolved_raster.get_age_grid_raster_band_names().get())
			{
				if (band_name->value() == AGE_BAND_NAME)
				{
					if (age_grid_band_name_index < age_grid_proxied_rasters->size())
					{
						age_grid_raster = age_grid_proxied_rasters.get()[age_grid_band_name_index];
					}
					break;
				}
				++age_grid_band_name_index;
			}
		}
	}


	// We don't want to rebuild the OpenGL structures that render the raster each frame
	// so those structures need to persist from one render to the next.
	boost::optional<GPlatesOpenGL::GLRenderGraphNode::non_null_ptr_type> raster_render_graph_node =
			d_persistent_opengl_objects->get_list_objects().get_raster_render_graph_node(
					rendered_resolved_raster.get_layer(),
					raster_colour_palette,
					rendered_resolved_raster.get_reconstruction_time(),
					rendered_resolved_raster.get_georeferencing(),
					raster,
					rendered_resolved_raster.get_reconstruct_raster_polygons(),
					rendered_resolved_raster.get_age_grid_georeferencing(),
					age_grid_raster);
	if (!raster_render_graph_node)
	{
		return;
	}

	d_paint_params->raster_primitives_on_the_sphere_node->add_child_node(raster_render_graph_node.get());
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_direction_arrow(
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
	if (arrowline_length > 0)
	{
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
		
		const GPlatesMaths::Vector3D arrowline_unit_vector = (1.0 / arrowline_length) * arrowline;

		// Specify end of arrowhead and direction of arrow.
		paint_cone(
				end,
				arrowhead_size * arrowline_unit_vector,
				rgba8_color,
				d_paint_params->drawables_off_the_sphere.get_triangle_drawables());
	}


	const float line_width =
			rendered_direction_arrow.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the drawables for lines of the current line width.
	LineDrawables &line_drawables =
			d_paint_params->drawables_off_the_sphere.get_line_drawables(line_width);

	// Render a single line segment for the arrow body.

	// Used to add lines to the stream.
	GPlatesOpenGL::GLStreamLines<vertex_type> stream_lines(*line_drawables.stream);

	stream_lines.begin_lines();

	// Vertex representing the start and end point's position and colour.
	const vertex_type start_vertex = { start.x().dval(), start.y().dval(), start.z().dval(), rgba8_color };
	const vertex_type end_vertex = { end.x().dval(), end.y().dval(), end.z().dval(), rgba8_color };

	stream_lines.add_vertex(start_vertex);
	stream_lines.add_vertex(end_vertex);

	stream_lines.end_lines();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (!d_render_settings.show_strings())
	{
		return;
	}

	if (d_visibility_tester.is_point_visible(rendered_string.get_point_on_sphere()))
	{
		const GPlatesMaths::UnitVector3D &uv = rendered_string.get_point_on_sphere().position_vector();

		// render drop shadow, if any
		boost::optional<Colour> shadow_colour = rendered_string.get_shadow_colour().get_colour(
				d_colour_scheme);
		if (shadow_colour)
		{
			GPlatesOpenGL::GLText3DNode::non_null_ptr_type shadow_text_3d_node =
					GPlatesOpenGL::GLText3DNode::create(
						d_text_renderer_ptr,
						uv.x().dval(),
						uv.y().dval(),
						uv.z().dval(),
						rendered_string.get_string(),
						*shadow_colour,
						rendered_string.get_x_offset() + 1, // right 1px
						rendered_string.get_y_offset() - 1, // down 1px
						rendered_string.get_font(),
						d_scale);

			d_paint_params->text_off_the_sphere_node->add_child_node(shadow_text_3d_node);
		}

		// render main text
		boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_string);
		if (colour)
		{
			GPlatesOpenGL::GLText3DNode::non_null_ptr_type text_3d_node =
					GPlatesOpenGL::GLText3DNode::create(
						d_text_renderer_ptr,
						uv.x().dval(),
						uv.y().dval(),
						uv.z().dval(),
						rendered_string.get_string(),
						*colour,
						rendered_string.get_x_offset(),
						rendered_string.get_y_offset(),
						rendered_string.get_font(),
						d_scale);

			d_paint_params->text_off_the_sphere_node->add_child_node(text_3d_node);
		}
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle);
	if (!colour)
	{
		return;
	}

	const float line_width =
			rendered_small_circle.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the drawables for lines of the current line width.
	LineDrawables &line_drawables =
			d_paint_params->translucent_drawables_on_the_sphere.get_line_drawables(line_width);

	line_drawables.nurbs_drawables.push_back(
			d_nurbs_renderer->draw_small_circle(
					rendered_small_circle.get_centre(),
					rendered_small_circle.get_radius_in_radians(),
					colour.get()));
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_small_circle_arc(
		const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle_arc);
	if (!colour)
	{
		return;
	}

	const float line_width =
			rendered_small_circle_arc.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the drawables for lines of the current line width.
	LineDrawables &line_drawables =
			d_paint_params->translucent_drawables_on_the_sphere.get_line_drawables(line_width);

	line_drawables.nurbs_drawables.push_back(
			d_nurbs_renderer->draw_small_circle_arc(
					rendered_small_circle_arc.get_centre(),
					rendered_small_circle_arc.get_start_point(),
					rendered_small_circle_arc.get_arc_length_in_radians(),
					colour.get()));
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_ellipse);
	if (!colour)
	{
		return;
	}

	const float line_width =
			rendered_ellipse.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	// Get the drawables for lines of the current line width.
	LineDrawables &line_drawables =
			d_paint_params->translucent_drawables_on_the_sphere.get_line_drawables(line_width);

	paint_ellipse(rendered_ellipse, colour.get(), line_drawables);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
		const Colour &colour,
		LineDrawables &line_drawables)
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
	GPlatesOpenGL::GLStreamLineLoops<vertex_type> stream_line_loops(*line_drawables.stream);

	stream_line_loops.begin_line_loop();

	for (double i = 0 ; i < TWO_PI ; i += dt)
	{
		GPlatesMaths::UnitVector3D uv = ellipse_generator.get_point_on_ellipse(i);

		// Vertex representing the ellipse point position and colour.
		const vertex_type vertex = { uv.x().dval(), uv.y().dval(), uv.z().dval(), rgba8_color };

		stream_line_loops.add_vertex(vertex);
	}

	stream_line_loops.end_line_loop();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint_cone(
		const GPlatesMaths::Vector3D &apex,
		const GPlatesMaths::Vector3D &cone_axis,
		rgba8_t rgba8_color,
		GPlatesOpenGL::GLStreamPrimitives<vertex_type> &stream)
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
	GPlatesOpenGL::GLStreamTriangleFans<vertex_type> stream_triangle_fans(stream);

	stream_triangle_fans.begin_triangle_fan();

	const vertex_type apex_vertex = { apex.x().dval(), apex.y().dval(), apex.z().dval(), rgba8_color };
	stream_triangle_fans.add_vertex(apex_vertex);

	for (int vertex_index = 0;
		vertex_index < NUM_VERTICES_IN_BASE_UNIT_CIRCLE;
		++vertex_index)
	{
		const GPlatesMaths::Vector3D &boundary = cone_base_circle[vertex_index];
		const vertex_type boundary_vertex = {
				boundary.x().dval(), boundary.y().dval(), boundary.z().dval(), rgba8_color };
		stream_triangle_fans.add_vertex(boundary_vertex);
	}
	const GPlatesMaths::Vector3D &last_circle = cone_base_circle[0];
	const vertex_type last_circle_vertex = {
			last_circle.x().dval(), last_circle.y().dval(), last_circle.z().dval(), rgba8_color };
	stream_triangle_fans.add_vertex(last_circle_vertex);

	stream_triangle_fans.end_triangle_fan();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::set_state_for_primitives_off_the_sphere(
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node)
{
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Turn on depth testing.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state_set =
			GPlatesOpenGL::GLDepthTestState::create();
	depth_test_state_set->gl_enable(GL_TRUE);
	state_set->add_state_set(depth_test_state_set);

	// Turn on depth writes.
	GPlatesOpenGL::GLMaskBuffersState::non_null_ptr_type depth_mask_state_set =
			GPlatesOpenGL::GLMaskBuffersState::create();
	depth_mask_state_set->gl_depth_mask(GL_TRUE);
	state_set->add_state_set(depth_mask_state_set);

	render_graph_node.set_state_set(state_set);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::set_state_for_non_raster_primitives_on_the_sphere(
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node)
{
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Turn on depth testing.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state_set =
			GPlatesOpenGL::GLDepthTestState::create();
	depth_test_state_set->gl_enable(GL_TRUE);
	state_set->add_state_set(depth_test_state_set);

	// Turn off depth writes.
	GPlatesOpenGL::GLMaskBuffersState::non_null_ptr_type depth_mask_state_set =
			GPlatesOpenGL::GLMaskBuffersState::create();
	depth_mask_state_set->gl_depth_mask(GL_FALSE);
	state_set->add_state_set(depth_mask_state_set);

	render_graph_node.set_state_set(state_set);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::set_state_for_raster_primitives_on_the_sphere(
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node)
{
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Set the alpha-blend state in case raster is semi-transparent.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state_set->add_state_set(blend_state);

	// Set the alpha-test state to reject pixels where alpha is zero (they make no
	// change or contribution to the framebuffer) - this is an optimisation.
	GPlatesOpenGL::GLAlphaTestState::non_null_ptr_type alpha_test_state =
			GPlatesOpenGL::GLAlphaTestState::create();
	alpha_test_state->gl_enable(GL_TRUE).gl_alpha_func(GL_GREATER, GLclampf(0));
	state_set->add_state_set(alpha_test_state);

	//
	// Note that we set the depth testing/writing state here rather than inside the
	// raster rendering machinery because here we know we are rendering to the scene
	// and hence have a depth buffer attachment to the main framebuffer.
	// In the raster rendering code there are certain paths that use render targets which
	// currently don't have a depth buffer attachment (because it's not needed) and
	// hence enabling depth testing in these paths can give corrupt results.
	//

	// Turn on depth testing.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state_set =
			GPlatesOpenGL::GLDepthTestState::create();
	depth_test_state_set->gl_enable(GL_TRUE);
	state_set->add_state_set(depth_test_state_set);

	// Turn off depth writes.
	GPlatesOpenGL::GLMaskBuffersState::non_null_ptr_type depth_mask_state_set =
			GPlatesOpenGL::GLMaskBuffersState::create();
	depth_mask_state_set->gl_depth_mask(GL_FALSE);
	state_set->add_state_set(depth_mask_state_set);

	render_graph_node.set_state_set(state_set);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::set_state_for_text_off_the_sphere(
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node)
{
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Turn on depth testing.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state_set =
			GPlatesOpenGL::GLDepthTestState::create();
	depth_test_state_set->gl_enable(GL_TRUE);
	state_set->add_state_set(depth_test_state_set);

	// Turn off depth writes.
	GPlatesOpenGL::GLMaskBuffersState::non_null_ptr_type depth_mask_state_set =
			GPlatesOpenGL::GLMaskBuffersState::create();
	depth_mask_state_set->gl_depth_mask(GL_FALSE);
	state_set->add_state_set(depth_mask_state_set);

	render_graph_node.set_state_set(state_set);
}

void
GPlatesGui::GlobeRenderedGeometryLayerPainter::set_translucent_state(
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node)
{
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type translucent_state =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Set the alpha-blend state.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	translucent_state->add_state_set(blend_state);

	// Set the anti-aliased point state.
	GPlatesOpenGL::GLPointState::non_null_ptr_type point_state = GPlatesOpenGL::GLPointState::create();
	point_state->gl_enable_point_smooth(GL_TRUE).gl_hint_point_smooth(GL_NICEST);
	translucent_state->add_state_set(point_state);

	// Set the anti-aliased line state.
	GPlatesOpenGL::GLLineState::non_null_ptr_type line_state = GPlatesOpenGL::GLLineState::create();
	line_state->gl_enable_line_smooth(GL_TRUE).gl_hint_line_smooth(GL_NICEST);
	translucent_state->add_state_set(line_state);

	render_graph_node.set_state_set(translucent_state);
}


GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::PointLinePolygonDrawables() :
	d_triangle_drawables(GPlatesOpenGL::create_stream<vertex_type>()),
	d_quad_drawables(GPlatesOpenGL::create_stream<vertex_type>())
{
}


GPlatesOpenGL::GLStreamPrimitives<GPlatesGui::GlobeRenderedGeometryLayerPainter::vertex_type> &
GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::get_point_drawables(
		float point_size)
{
	// Get the stream for points of the current point size.
	point_size_to_drawables_map_type::iterator stream_iter = d_point_drawables_map.find(point_size);
	if (stream_iter != d_point_drawables_map.end())
	{
		return *stream_iter->second;
	}

	// Create a new stream.
	GPlatesOpenGL::GLStreamPrimitives<vertex_type>::non_null_ptr_type stream = GPlatesOpenGL::create_stream<vertex_type>();
	
	d_point_drawables_map.insert(
			point_size_to_drawables_map_type::value_type(point_size, stream));

	return *stream;
}


GPlatesGui::GlobeRenderedGeometryLayerPainter::LineDrawables &
GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::get_line_drawables(
		float line_width)
{
	// Get the stream for lines of the current line width.
	line_width_to_drawables_map_type::iterator stream_iter = d_line_drawables_map.find(line_width);
	if (stream_iter != d_line_drawables_map.end())
	{
		return stream_iter->second;
	}

	// Create a new stream.
	GPlatesOpenGL::GLStreamPrimitives<vertex_type>::non_null_ptr_type stream = GPlatesOpenGL::create_stream<vertex_type>();

	std::pair<line_width_to_drawables_map_type::iterator, bool> insert_result =
			d_line_drawables_map.insert(
					line_width_to_drawables_map_type::value_type(
							line_width, LineDrawables(stream)));

	return insert_result.first->second;
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::add_drawables(
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_parent_node)
{
	//
	// Get the drawables representing all point primitives (if any).
	//

	// Iterate over the point size groups and add a render graph node for each.
	BOOST_FOREACH(point_size_to_drawables_map_type::value_type &point_size_entry, d_point_drawables_map)
	{
		GPlatesOpenGL::GLStreamPrimitives<vertex_type> &points_stream = *point_size_entry.second;
		boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> points_drawable =
				points_stream.get_drawable();
		if (points_drawable)
		{
			const float point_size = point_size_entry.first.dval();

			add_points_drawable(point_size, points_drawable.get(), render_graph_parent_node);
		}
	}

	//
	// Get the drawables representing all line primitives (if any).
	//

	// Iterate over the line width groups and add a render graph node for each.
	BOOST_FOREACH(line_width_to_drawables_map_type::value_type &line_width_entry, d_line_drawables_map)
	{
		LineDrawables &line_drawables = line_width_entry.second;

		// Get a drawable representing all regular lines and nurbs line curves combined.
		boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> all_lines_drawable =
				get_lines_drawable(line_drawables);

		if (all_lines_drawable)
		{
			const float line_width = line_width_entry.first.dval();

			add_lines_drawable(line_width, all_lines_drawable.get(), render_graph_parent_node);
		}
	}

	//
	// Get the drawable representing all triangle primitives (if any).
	//

	boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> triangle_drawable =
			d_triangle_drawables->get_drawable();
	if (triangle_drawable)
	{
		// Create a drawable render graph node and add it to the parent node.
		GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type triangle_drawable_node =
				GPlatesOpenGL::GLRenderGraphDrawableNode::create(triangle_drawable.get());
		// No state set needed for polygons - the default state is sufficient.

		render_graph_parent_node.add_child_node(triangle_drawable_node);
	}

	//
	// Get the drawable representing all quad primitives (if any).
	//

	boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> quad_drawable =
			d_quad_drawables->get_drawable();
	if (quad_drawable)
	{
		// Create a drawable render graph node and add it to the parent node.
		GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type quad_drawable_node =
				GPlatesOpenGL::GLRenderGraphDrawableNode::create(quad_drawable.get());
		// No state set needed for polygons - the default state is sufficient.

		render_graph_parent_node.add_child_node(quad_drawable_node);
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::add_points_drawable(
		float point_size,
		const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &points_drawable,
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_parent_node)
{
	// Create a drawable render graph node and add it to the parent node.
	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type points_drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(points_drawable.get());

	// Create a state set for the current point size.
	GPlatesOpenGL::GLPointState::non_null_ptr_type point_state = GPlatesOpenGL::GLPointState::create();
	point_state->gl_point_size(point_size);
	points_drawable_node->set_state_set(point_state);

	render_graph_parent_node.add_child_node(points_drawable_node);
}


boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>
GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::get_lines_drawable(
		LineDrawables &line_drawables)
{
	boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> non_nurbs_line_drawable =
			line_drawables.stream->get_drawable();
	if (non_nurbs_line_drawable)
	{
		// If there's no nurbs drawables then there's only the single non-nurbs drawable
		// so we can just return it.
		if (line_drawables.nurbs_drawables.empty())
		{
			return non_nurbs_line_drawable.get();
		}

		// Create a composite drawable to hold the nurbs and non-nurbs drawables.
		GPlatesOpenGL::GLCompositeDrawable::non_null_ptr_type composite_drawable =
				GPlatesOpenGL::GLCompositeDrawable::create();
		composite_drawable->add_drawable(non_nurbs_line_drawable.get());

		BOOST_FOREACH(
				const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &nurbs_drawable,
				line_drawables.nurbs_drawables)
		{
			composite_drawable->add_drawable(nurbs_drawable);
		}

		return static_cast<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>(composite_drawable);
	}
	// If we get here then there was no non-nurbs drawable.

	if (line_drawables.nurbs_drawables.empty())
	{
		return boost::none;
	}

	// If there's a single nurbs drawable then return it.
	if (line_drawables.nurbs_drawables.size() == 1)
	{
		return line_drawables.nurbs_drawables[0];
	}

	// Create a composite drawable to hold the nurbs drawables.
	GPlatesOpenGL::GLCompositeDrawable::non_null_ptr_type composite_drawable =
			GPlatesOpenGL::GLCompositeDrawable::create();

	BOOST_FOREACH(
			const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &nurbs_drawable,
			line_drawables.nurbs_drawables)
	{
		composite_drawable->add_drawable(nurbs_drawable);
	}

	return static_cast<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>(composite_drawable);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::PointLinePolygonDrawables::add_lines_drawable(
		float line_width,
		const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &lines_drawable,
		GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_parent_node)
{
	// Create a drawable render graph node and add it to the parent node.
	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type lines_drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(lines_drawable.get());

	// Create a state set for the current line width.
	GPlatesOpenGL::GLLineState::non_null_ptr_type line_state = GPlatesOpenGL::GLLineState::create();
	line_state->gl_line_width(line_width);
	lines_drawable_node->set_state_set(line_state);

	render_graph_parent_node.add_child_node(lines_drawable_node);
}


GPlatesGui::GlobeRenderedGeometryLayerPainter::LineDrawables::LineDrawables(
		const GPlatesOpenGL::GLStreamPrimitives<vertex_type>::non_null_ptr_type &stream_) :
	stream(stream_)
{
}

void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_arrowed_polyline(
		const GPlatesViewOperations::RenderedArrowedPolyline &rendered_arrowed_polyline)
{
	// Based on the "visit_rendered_direction_arrow" code 

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_arrowed_polyline);
	if (!colour)
	{
		return;
	}

	const rgba8_t rgba8_colour = Colour::to_rgba8(*colour);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type points =
			rendered_arrowed_polyline.get_polyline_on_sphere();

	GPlatesMaths::PolylineOnSphere::const_iterator 
			iter = points->begin(),
			end = points->end();

	for (; iter != end ; ++iter)
	{	
		GPlatesMaths::real_t arrowhead_size =
			d_inverse_zoom_factor * rendered_arrowed_polyline.get_arrowhead_projected_size();

		//qDebug() << "arrowhead_size:" << arrowhead_size.dval();

		const float MAX_ARROWHEAD_SIZE = rendered_arrowed_polyline.get_max_arrowhead_size();

		if (arrowhead_size >MAX_ARROWHEAD_SIZE)
		{
			arrowhead_size = MAX_ARROWHEAD_SIZE;
		}

		// For the direction of the arrow, we really want the tangent to the curve at
		// the end of the curve. The curve will ultimately be a small circle arc; the 
		// current implementation uses a great circle arc. 
		if (!(iter->is_zero_length()))
		{
			GPlatesMaths::Vector3D tangent_direction = GPlatesMaths::cross(
				iter->rotation_axis(),iter->end_point().position_vector());
			GPlatesMaths::UnitVector3D arrowline_unit_vector(tangent_direction);			
			paint_cone(GPlatesMaths::Vector3D(iter->end_point().position_vector()),
				arrowhead_size*arrowline_unit_vector,
				rgba8_colour,
				d_paint_params->drawables_off_the_sphere.get_triangle_drawables());
		}
	}

	const float line_width =
		rendered_arrowed_polyline.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT * d_scale;

	LineDrawables &line_drawables =
		d_paint_params->translucent_drawables_on_the_sphere.get_line_drawables(line_width);


	paint_great_circle_arcs(
		points->begin(),
		points->end(),
		colour.get(),
		line_drawables,
		*d_nurbs_renderer);
}

