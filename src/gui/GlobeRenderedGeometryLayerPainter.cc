/* $Id$ */

/**
 * \file Draws rendered geometries in a specific @a RenderedGeometryLayer onto 3d orthographic
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "GlobeRenderedGeometryLayerPainter.h"
#include "NurbsRenderer.h"
#include "OpenGL.h"

#include "maths/EllipseGenerator.h"
#include "maths/Real.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"

#include "view-operations/RenderedDirectionArrow.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedString.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"
#include "view-operations/RenderedGeometryUtils.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"



namespace 
{
	/**
	 * We will draw a NURBS if the two endpoints of a great circle arc are
	 * more than PI/36 radians (= 5 degrees) apart.
	 */
	const double GCA_DISTANCE_THRESHOLD_DOT = std::cos(GPlatesMaths::PI/36.0);
	const double TWO_PI = 2.*GPlatesMaths::PI;

	inline
	void
	draw_vertex(
			const GPlatesMaths::PointOnSphere &p)
	{
		const GPlatesMaths::UnitVector3D &uv = p.position_vector();
		glVertex3d(uv.x().dval(), uv.y().dval(), uv.z().dval());
	}


	void
	draw_points_for_multi_point(
			const GPlatesMaths::MultiPointOnSphere::const_iterator &begin,
			const GPlatesMaths::MultiPointOnSphere::const_iterator &end)
	{
		GPlatesMaths::MultiPointOnSphere::const_iterator iter = begin;

		glBegin(GL_POINTS);
			for ( ; iter != end; ++iter) {
				draw_vertex(*iter);
			}
		glEnd();
	}


	void
	draw_great_circle_arc(
			const GPlatesMaths::PointOnSphere &start,
			const GPlatesMaths::PointOnSphere &end,
			const GPlatesMaths::real_t &dot_of_endpoints,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		// Draw a NURBS if the two endpoints of the arc are far enough apart.
		if (dot_of_endpoints < GCA_DISTANCE_THRESHOLD_DOT)
		{
			nurbs_renderer.draw_great_circle_arc(start, end);
		}
		else
		{
			glBegin(GL_LINES);
				draw_vertex(start);
				draw_vertex(end);
			glEnd();
		}
	}


	inline
	void
	draw_great_circle_arc(
			const GPlatesMaths::PointOnSphere &start,
			const GPlatesMaths::PointOnSphere &end,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		draw_great_circle_arc(start, end,
				dot(start.position_vector(), end.position_vector()),
				nurbs_renderer);
	}


	inline
	void
	draw_great_circle_arc(
			const GPlatesMaths::GreatCircleArc &gca,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		draw_great_circle_arc(gca.start_point(), gca.end_point(),
				gca.dot_of_endpoints(), nurbs_renderer);
	}


	void
	draw_arcs_for_polygon(
			const GPlatesMaths::PolygonOnSphere::const_iterator &begin, 
			const GPlatesMaths::PolygonOnSphere::const_iterator &end,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		GPlatesMaths::PolygonOnSphere::const_iterator iter = begin;

		for ( ; iter != end; ++iter)
		{
			draw_great_circle_arc(*iter, nurbs_renderer);
		}
	}


	void
	draw_arcs_for_polyline(
			const GPlatesMaths::PolylineOnSphere::const_iterator &begin, 
			const GPlatesMaths::PolylineOnSphere::const_iterator &end,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		GPlatesMaths::PolylineOnSphere::const_iterator iter = begin;

		for ( ; iter != end; ++iter)
		{
			draw_great_circle_arc(*iter, nurbs_renderer);
		}
	}


	void
	draw_cone(
			const GPlatesMaths::Vector3D &apex,
			const GPlatesMaths::Vector3D &cone_axis)
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

		glBegin(GL_TRIANGLE_FAN);
			glVertex3d(apex.x().dval(), apex.y().dval(), apex.z().dval());

			for (int vertex_index = 0;
				vertex_index < NUM_VERTICES_IN_BASE_UNIT_CIRCLE;
				++vertex_index)
			{
				const GPlatesMaths::Vector3D &vertex = cone_base_circle[vertex_index];
				glVertex3d(vertex.x().dval(), vertex.y().dval(), vertex.z().dval());
			}
			const GPlatesMaths::Vector3D &last_circle_vertex = cone_base_circle[0];
			glVertex3d(last_circle_vertex.x().dval(), last_circle_vertex.y().dval(),
					last_circle_vertex.z().dval());
		glEnd();
	}
	
	void
	draw_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
		const double &inverse_zoom_factor)
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

		GPlatesMaths::EllipseGenerator ellipse_generator(
			rendered_ellipse.get_centre(),
			rendered_ellipse.get_semi_major_axis_radians(),
			rendered_ellipse.get_semi_minor_axis_radians(),
			rendered_ellipse.get_axis());
					
		glBegin(GL_LINE_LOOP);
		
		for (double i = 0 ; i < TWO_PI ; i += dt)
		{

			GPlatesMaths::UnitVector3D uv = ellipse_generator.get_point_on_ellipse(i);
			glVertex3d(uv.x().dval(),uv.y().dval(),uv.z().dval());
		}
		glEnd();
	
	}
	
}


const float GPlatesGui::GlobeRenderedGeometryLayerPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::GlobeRenderedGeometryLayerPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::paint()
{
	// First pass over rendered geometry layer draws opaque primitives such
	// as 3d meshes.
	d_draw_opaque_primitives = true;
	// Enable depth buffer writes.
	glDepthMask(GL_TRUE);
	visit_rendered_geoms();

	// Second pass over rendered geometry layer draws the transparent primitives
	// such as antialiased lines and points. These must be drawn second to
	// avoid blending artifacts due to the depth buffer such as opaque primitives
	// having transparent lines through them caused by drawing an antialiased line
	// first and a filled polygon second (where the polygon is at a greater depth
	// than the line).
	d_draw_opaque_primitives = false;
	// Disable depth buffer writes to avoid blending artifacts caused if
	// a transparent primitive writes to the depth buffer and the next primitive
	// is behind it and hence fails the depth test leaving a partially transparent
	// pixel in the colour buffer (if pixel is near edge of first primitive where
	// transparency blending shows through).
	glDepthMask(GL_FALSE);
	visit_rendered_geoms();
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


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_point_on_sphere(
		const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
{
	if (!d_render_settings.show_points)
	{
		return;
	}

	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_point_on_sphere.get_colour());
	glPointSize(rendered_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT);
	glBegin(GL_POINTS);
		draw_vertex(rendered_point_on_sphere.get_point_on_sphere());
	glEnd();
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_multi_point_on_sphere(
		const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)
{
	if (!d_render_settings.show_multipoints)
	{
		return;
	}

	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_multi_point_on_sphere.get_colour());
	glPointSize(rendered_multi_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT);

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
		rendered_multi_point_on_sphere.get_multi_point_on_sphere();

	draw_points_for_multi_point(
			multi_point_on_sphere->begin(),
			multi_point_on_sphere->end());
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_polyline_on_sphere(
		const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
{
	if (!d_render_settings.show_lines)
	{
		return;
	}

	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_polyline_on_sphere.get_colour());
	glLineWidth(rendered_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
		rendered_polyline_on_sphere.get_polyline_on_sphere();

	draw_arcs_for_polyline(
			polyline_on_sphere->begin(),
			polyline_on_sphere->end(),
			*d_nurbs_renderer);
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_polygon_on_sphere(
		const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere)
{
	if (!d_render_settings.show_polygons)
	{
		return;
	}

	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_polygon_on_sphere.get_colour());
	glLineWidth(rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
		rendered_polygon_on_sphere.get_polygon_on_sphere();

	draw_arcs_for_polygon(
			polygon_on_sphere->begin(),
			polygon_on_sphere->end(),
			*d_nurbs_renderer);

}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_direction_arrow(
		const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow)
{
	if (!d_render_settings.show_arrows)
	{
		return;
	}

	const GPlatesMaths::Vector3D start(
			rendered_direction_arrow.get_start_position().position_vector());

	// Calculate position from start point along tangent direction to
	// end point off the globe. The length of the arrow in world space
	// is inversely proportional to the zoom or magnification.
	const GPlatesMaths::Vector3D end = GPlatesMaths::Vector3D(start) +
			d_inverse_zoom_factor * rendered_direction_arrow.get_arrow_direction();

	glColor3fv(rendered_direction_arrow.get_colour());

	if (d_draw_opaque_primitives)
	{
		const GPlatesMaths::Vector3D arrowline = end - start;
		const GPlatesMaths::real_t arrowline_length = arrowline.magnitude();

		// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
		if (arrowline_length == 0)
		{
			return;
		}

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
		draw_cone(
				end,
				arrowhead_size * arrowline_unit_vector);
	}
	else
	{
		glLineWidth(rendered_direction_arrow.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT);

		// Render a single line segment for the arrow body.
		glBegin(GL_LINES);
			glVertex3d(start.x().dval(), start.y().dval(), start.z().dval());
			glVertex3d(end.x().dval(), end.y().dval(), end.z().dval());
		glEnd();
	}
}

void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (!d_render_settings.show_strings)
	{
		return;
	}

	if(d_visibility_tester.is_point_visible(rendered_string.get_point_on_sphere()))
	{
		const GPlatesMaths::UnitVector3D &uv = rendered_string.get_point_on_sphere().position_vector();

		// render drop shadow, if any
		if (rendered_string.get_shadow_colour())
		{
			d_text_renderer_ptr->render_text(
					uv.x().dval(),
					uv.y().dval(),
					uv.z().dval(),
					rendered_string.get_string(),
					*rendered_string.get_shadow_colour(),
					rendered_string.get_x_offset() + 1, // right 1px
					rendered_string.get_y_offset() - 1, // down 1px
					rendered_string.get_font());
		}

		// render main text
		d_text_renderer_ptr->render_text(
				uv.x().dval(),
				uv.y().dval(),
				uv.z().dval(),
				rendered_string.get_string(),
				rendered_string.get_colour(),
				rendered_string.get_x_offset(),
				rendered_string.get_y_offset(),
				rendered_string.get_font());
	}
}


void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle)
{

	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_small_circle.get_colour());
	glLineWidth(rendered_small_circle.get_line_width_hint() * POINT_SIZE_ADJUSTMENT);

	d_nurbs_renderer->draw_small_circle(
		rendered_small_circle.get_centre(),
		rendered_small_circle.get_radius_in_radians());
}

void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_small_circle_arc(
		const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc)
{

	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_small_circle_arc.get_colour());
	glLineWidth(rendered_small_circle_arc.get_line_width_hint() * POINT_SIZE_ADJUSTMENT);

	d_nurbs_renderer->draw_small_circle_arc(
			rendered_small_circle_arc.get_centre(),
			rendered_small_circle_arc.get_start_point(),
			rendered_small_circle_arc.get_arc_length_in_radians());	
}

void
GPlatesGui::GlobeRenderedGeometryLayerPainter::visit_rendered_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse)
{
	if (d_draw_opaque_primitives)
	{
		return;
	}

	glColor3fv(rendered_ellipse.get_colour());
	glLineWidth(rendered_ellipse.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT);	
	
	draw_ellipse(rendered_ellipse,d_inverse_zoom_factor);
}


