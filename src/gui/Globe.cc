/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "Globe.h"
#include "PlatesColourTable.h"
#include "NurbsRenderer.h"
#include "Texture.h"

#include "maths/Real.h"
#include "maths/UnitVector3D.h"
#include "maths/ConstGeometryOnSphereVisitor.h"

#include "view-operations/RenderedDirectionArrow.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"
#include "view-operations/RenderedGeometryUtils.h"


namespace 
{
	/**
	 * We will draw a NURBS if the two endpoints of a great circle arc are
	 * more than PI/36 radians (= 5 degrees) apart.
	 */
	const double GCA_DISTANCE_THRESHOLD_DOT = std::cos(GPlatesMaths::PI/36.0);


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

		// highlight the polygon vertices
		iter = begin;
		for ( ; iter != end; ++iter) {
			glPointSize(5.0f);
			glBegin(GL_POINTS);
				draw_vertex(iter->start_point());
			glEnd();
			glPointSize(4.0f);
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
			const GPlatesMaths::Vector3D &centre_base_circle,
			const GPlatesMaths::Vector3D &apex)
	{
		const GPlatesMaths::Vector3D cone_axis = apex - centre_base_circle;

		const GPlatesMaths::real_t cone_axis_mag = cone_axis.magnitude();

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


	/**
	 * This is a visitor to draw rendered geometries on-screen using OpenGL.
	 */
	class PaintGeometry :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor
	{
	public:
		explicit
		PaintGeometry(
				GPlatesGui::NurbsRenderer &nurbs_renderer,
				const double &viewport_zoom_factor):
			d_nurbs_renderer(&nurbs_renderer),
			d_globe_ptr(globe),
			d_current_layer_far_depth(0),
			d_depth_range_per_layer(0),
			d_inverse_zoom_factor(1.0 / viewport_zoom_factor)
		{  }

		virtual
		~PaintGeometry()
		{  }

		virtual
		void
		paint(
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				double depth_range_near,
				double depth_range_far)
		{
			// Count the number of layers that we're going to draw.
			const unsigned num_layers_to_draw =
					GPlatesViewOperations::RenderedGeometryUtils::get_num_active_non_empty_layers(
							rendered_geometry_collection);

			// Divide our given depth range equally amongst the layers.
			// Draw layers back to front as we visit them.
			d_depth_range_per_layer = (depth_range_far - depth_range_near) / num_layers_to_draw;
			d_current_layer_far_depth = depth_range_far;

			// Draw the layers.
			rendered_geometry_collection.accept_visitor(*this);
		}

	private:
		virtual
		bool
		visit_rendered_geometry_layer(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
		{
			// If layer is not active then we don't want to visit it.
			if (!rendered_geometry_layer.is_active())
			{
				return false;
			}

			if (rendered_geometry_layer.is_empty())
			{
				return false;
			}

			// First pass over rendered geometry layer gathers 

			// If layer is not empty then we have allocated a depth range for it.
			const double far_depth = d_current_layer_far_depth;
			double near_depth = far_depth - d_depth_range_per_layer;
			if (near_depth < 0) near_depth = 0;

			glDepthRange(near_depth, far_depth);

			d_current_layer_far_depth -= d_depth_range_per_layer;

			// We want to visit the RenderedGeometry objects in this layer to draw them.
			return true;
		}

		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
		{
			if ( ! d_globe_ptr->d_show_point ) { return; }
			glColor3fv(rendered_point_on_sphere.get_colour());
			glPointSize(rendered_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT);
			glBegin(GL_POINTS);
				draw_vertex(rendered_point_on_sphere.get_point_on_sphere());
			glEnd();
		}

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)
		{
			if ( ! d_globe_ptr->d_show_multipoint ) { return; }
			glColor3fv(rendered_multi_point_on_sphere.get_colour());
			glPointSize(rendered_multi_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT);

			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
				rendered_multi_point_on_sphere.get_multi_point_on_sphere();

			draw_points_for_multi_point(
					multi_point_on_sphere->begin(),
					multi_point_on_sphere->end());
		}

		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
		{
			if ( !d_globe_ptr->d_show_line ) { return; }
			glColor3fv(rendered_polyline_on_sphere.get_colour());
			glLineWidth(rendered_polyline_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT);

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
				rendered_polyline_on_sphere.get_polyline_on_sphere();

			draw_arcs_for_polyline(
					polyline_on_sphere->begin(),
					polyline_on_sphere->end(),
					*d_nurbs_renderer);
		}

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere)
		{
			if ( ! d_globe_ptr->d_show_polygon ) { return; }

			glColor3fv(rendered_polygon_on_sphere.get_colour());
			glLineWidth(rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT);

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
				rendered_polygon_on_sphere.get_polygon_on_sphere();

			draw_arcs_for_polygon(
					polygon_on_sphere->begin(),
					polygon_on_sphere->end(),
					*d_nurbs_renderer);

		}

		virtual
		void
		visit_rendered_direction_arrow(
				const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow)
		{
			const GPlatesMaths::Vector3D start(
					rendered_direction_arrow.get_start_position().position_vector());

			// Calculate position from start point along tangent direction to
			// end point off the globe. The length of the arrow in world space
			// is inversely proportional to the zoom or magnification.
			const GPlatesMaths::Vector3D end = GPlatesMaths::Vector3D(start) +
					d_inverse_zoom_factor * rendered_direction_arrow.get_arrow_direction();

			glColor3fv(rendered_direction_arrow.get_colour());
			glLineWidth(rendered_direction_arrow.get_arrowline_width_hint() * LINE_WIDTH_ADJUSTMENT);

			const float ratio_arrowhead_to_body =
					rendered_direction_arrow.get_ratio_size_arrowhead_to_arrowline();

			// Specify start and end of arrowhead - the apex is at the end.
			draw_cone(
					ratio_arrowhead_to_body * start + (1 - ratio_arrowhead_to_body) * end,
					end);

			// Render a single line segment for the arrow body.
			glBegin(GL_LINES);
				glVertex3d(start.x().dval(), start.y().dval(), start.z().dval());
				glVertex3d(end.x().dval(), end.y().dval(), end.z().dval());
			glEnd();
		}

	private:
		GPlatesGui::NurbsRenderer *const d_nurbs_renderer;
		GPlatesGui::Globe *const d_globe_ptr;
		double d_current_layer_far_depth;
		double d_depth_range_per_layer;
		double d_inverse_zoom_factor;

		//! Multiplying factor to get point size of 1.0f to look like one screen-space pixel.
		static const float POINT_SIZE_ADJUSTMENT;
		//! Multiplying factor to get line width of 1.0f to look like one screen-space pixel.
		static const float LINE_WIDTH_ADJUSTMENT;
	};

	const float PaintGeometry::POINT_SIZE_ADJUSTMENT = 1.0f;
	const float PaintGeometry::LINE_WIDTH_ADJUSTMENT = 1.0f;


	void
	paint_rendered_geometries(
			const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
			GPlatesGui::NurbsRenderer &nurbs_renderer,
			GPlatesGui::Globe *globe,
			double depth_range_near,
			double depth_range_far,
			const double &viewport_zoom_factor)
	{
		PaintGeometry paint_geometry(nurbs_renderer, viewport_zoom_factor);

		paint_geometry.paint(
				rendered_geom_collection, depth_range_near, depth_range_far);
	}
}


GPlatesGui::Globe::Globe(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection) :
d_rendered_geom_collection(&rendered_geom_collection),
d_sphere( OpaqueSphereFactory(Colour(0.35f, 0.35f, 0.35f)) ),
d_grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON)
{
	//FIXME: be sure to sychonize with ViewportWidgetUi.ui
	d_show_point = true;
	d_show_line = true;
	d_show_polygon = true;
	d_show_topology = true;
	d_show_multipoint = true;
}

void
GPlatesGui::Globe::SetNewHandlePos(
		const GPlatesMaths::PointOnSphere &pos)
{
	d_globe_orientation.set_new_handle_at_pos(pos);
}


void
GPlatesGui::Globe::UpdateHandlePos(
		const GPlatesMaths::PointOnSphere &pos)
{
	d_globe_orientation.move_handle_to_pos(pos);
}


const GPlatesMaths::PointOnSphere
GPlatesGui::Globe::Orient(
		const GPlatesMaths::PointOnSphere &pos)
{
	return d_globe_orientation.reverse_orient_point(pos);
}


void
GPlatesGui::Globe::paint(
		const double &viewport_zoom_factor)
{
	// Enable smoothing.
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		GPlatesMaths::UnitVector3D axis = d_globe_orientation.rotation_axis();
		GPlatesMaths::real_t angle_in_deg =
				GPlatesMaths::radiansToDegrees(d_globe_orientation.rotation_angle());
		glRotatef(angle_in_deg.dval(),
		           axis.x().dval(), axis.y().dval(), axis.z().dval());
		
		// The glDepthRange(near_plane, far_plane) call pushes the sphere back in the depth
		// buffer a bit, to avoid Z-fighting.
		glDepthRange(0.9, 1.0);
		d_sphere->paint();

		// Draw the texture slightly in front of the grey sphere, otherwise we get little
		// bits of the sphere sticking out. 
		glDepthRange(0.8, 0.9);
		d_texture->paint();
		
		glDepthRange(0.7, 0.8);
		d_grid.paint(Colour::get_silver());

		paint_rendered_geometries(
				*d_rendered_geom_collection,
				*d_nurbs_renderer,
				this,
				0.0,
				0.7,
				viewport_zoom_factor);

	glPopMatrix();
}

void
GPlatesGui::Globe::paint_vector_output(
		const double &viewport_zoom_factor)
{
	d_grid.paint_circumference(GPlatesGui::Colour::get_grey());

	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		GPlatesMaths::UnitVector3D axis = d_globe_orientation.rotation_axis();
		GPlatesMaths::real_t angle_in_deg =
				GPlatesMaths::radiansToDegrees(d_globe_orientation.rotation_angle());
		glRotatef(angle_in_deg.dval(),
		           axis.x().dval(), axis.y().dval(), axis.z().dval());

		// The glDepthRange(near_plane, far_plane) call pushes the grid back in the depth
		// buffer a bit, to avoid Z-fighting.
		glDepthRange(0.7, 0.8);
		d_grid.paint(Colour::get_grey());

		// Get current rendered layer active state so we can restore later.
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState
			prev_rendered_layer_active_state =
				d_rendered_geom_collection->capture_main_layer_active_state();

		// Turn off rendering of digitisation layer.
		d_rendered_geom_collection->set_main_layer_active(
				GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER,
				false);

		// Turn off rendering of mouse movement layer.
		d_rendered_geom_collection->set_main_layer_active(
				GPlatesViewOperations::RenderedGeometryCollection::MOUSE_MOVEMENT_LAYER,
				false);

		paint_rendered_geometries(
				*d_rendered_geom_collection,
				*d_nurbs_renderer,
				this,
				0.0,
				0.7,
				viewport_zoom_factor);

		// Restore previous rendered layer active state.
		d_rendered_geom_collection->restore_main_layer_active_state(
			prev_rendered_layer_active_state);

	glPopMatrix();
}

void
GPlatesGui::Globe::initialise_texture()
{
//	d_texture->generate_test_texture2();
}

void
GPlatesGui::Globe::toggle_raster_image()
{
	d_texture->toggle();

}

void
GPlatesGui::Globe::enable_raster_display()
{
	d_texture->set_enabled(true);
}

void
GPlatesGui::Globe::disable_raster_display()
{
	d_texture->set_enabled(false);
}

