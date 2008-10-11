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


namespace 
{
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
	draw_arcs_for_polygon(
			const GPlatesMaths::PolygonOnSphere::const_iterator &begin, 
			const GPlatesMaths::PolygonOnSphere::const_iterator &end,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		// We will draw a NURBS if the two endpoints of the arc are
		// more than PI/36 radians (= 5 degrees) apart.
		static const double DISTANCE_THRESHOLD = std::cos(GPlatesMaths::PI/36.0);

		GPlatesMaths::PolygonOnSphere::const_iterator iter = begin;

		for ( ; iter != end; ++iter) {
			if (iter->dot_of_endpoints().dval() < DISTANCE_THRESHOLD) {
				nurbs_renderer.draw_great_circle_arc(*iter);
			} else {
				glBegin(GL_LINES);
					draw_vertex(iter->start_point());
					draw_vertex(iter->end_point());
				glEnd();
			}
		}

		// highlight the polygon vertices
		iter = begin;
		for ( ; iter != end; ++iter) {
			glBegin(GL_POINTS);
				draw_vertex(iter->start_point());
			glEnd();
		}
		
	}


	void
	draw_arcs_for_polyline(
			const GPlatesMaths::PolylineOnSphere::const_iterator &begin, 
			const GPlatesMaths::PolylineOnSphere::const_iterator &end,
			GPlatesGui::NurbsRenderer &nurbs_renderer)
	{
		// We will draw a NURBS if the two endpoints of the arc are
		// more than PI/36 radians (= 5 degrees) apart.
		static const double DISTANCE_THRESHOLD = std::cos(GPlatesMaths::PI/36.0);

		GPlatesMaths::PolylineOnSphere::const_iterator iter = begin;

		for ( ; iter != end; ++iter) {
			if (iter->dot_of_endpoints().dval() < DISTANCE_THRESHOLD) {
				nurbs_renderer.draw_great_circle_arc(*iter);
			} else {
				glBegin(GL_LINES);
					draw_vertex(iter->start_point());
					draw_vertex(iter->end_point());
				glEnd();
			}
		}
	}


	/**
	 * This is a visitor to draw geometries on-screen using OpenGL.
	 */
	class PaintGeometry:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		explicit
		PaintGeometry(
				GPlatesGui::NurbsRenderer &nurbs_renderer,
				float gl_line_width,
				GPlatesGui::Globe *globe):
			d_nurbs_renderer_ptr(&nurbs_renderer),
			d_gl_line_width(gl_line_width),
			d_globe_ptr(globe)
		{  }

		virtual
		~PaintGeometry()
		{  }

		void
		operator()(
				const GPlatesGui::RenderedGeometry &rendered_geometry)
		{
			d_colour = rendered_geometry.colour();
			rendered_geometry.geometry()->accept_visitor(*this);
		}

		// Please keep these geometries ordered alphabetically.

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point)
		{
			// FIXME:  We should assert that the boost::optional is not boost::none.
			glColor3fv(**d_colour);
			glPointSize(4.0f);
			if ( d_globe_ptr->d_show_mesh ) { 
				draw_points_for_multi_point(multi_point->begin(), multi_point->end());
			}
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point)
		{
			// FIXME:  We should assert that the boost::optional is not boost::none.
			glColor3fv(**d_colour);
			glPointSize(4.0f);
			glBegin(GL_POINTS);
			if ( d_globe_ptr->d_show_point ) { 
				draw_vertex(*point);
			}
			glEnd();
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon)
		{
			// FIXME:  We should assert that the boost::optional is not boost::none.
			glColor3fv(**d_colour);
			glLineWidth(d_gl_line_width);
			if ( d_globe_ptr->d_show_polygon ) { 
				draw_arcs_for_polygon(polygon->begin(), polygon->end(), *d_nurbs_renderer_ptr);
			}
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
		{
			// FIXME:  We should assert that the boost::optional is not boost::none.
			glColor3fv(**d_colour);
			glLineWidth(d_gl_line_width);
			if ( d_globe_ptr->d_show_line ) {
				draw_arcs_for_polyline(polyline->begin(), polyline->end(), *d_nurbs_renderer_ptr);
			}
		}

	private:
		GPlatesGui::NurbsRenderer *const d_nurbs_renderer_ptr;
		float d_gl_line_width;
		GPlatesGui::Globe *const d_globe_ptr;
		boost::optional<GPlatesGui::PlatesColourTable::const_iterator> d_colour;
	};


	void
	paint_geometries(
			GPlatesGui::Globe *globe,
			const GPlatesGui::RenderedGeometryLayers::rendered_geometry_layer_type &layer,
			GPlatesGui::NurbsRenderer &nurbs_renderer,
			float gl_line_width)
	{
		typedef GPlatesGui::RenderedGeometryLayers::rendered_geometry_layer_type layer_type;
		layer_type::const_iterator iter = layer.begin();
		layer_type::const_iterator end = layer.end();

		for_each(iter, end, PaintGeometry(nurbs_renderer, gl_line_width, globe));
	}
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
GPlatesGui::Globe::Paint()
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
		d_sphere.Paint();

		// Draw the texture slightly in front of the grey sphere, otherwise we get little
		// bits of the sphere sticking out. 
		glDepthRange(0.8, 0.9);
		d_texture.paint();
		
		glDepthRange(0.7, 0.8);
		d_grid.paint(Colour::SILVER);

		glDepthRange(0.6, 0.7);
		paint_geometries(this, rendered_geometry_layers().reconstruction_layer(), d_nurbs_renderer, 1.5f);

		glDepthRange(0.5, 0.6);
		if (rendered_geometry_layers().should_show_digitisation_layer()) {
			paint_geometries(this, rendered_geometry_layers().digitisation_layer(), d_nurbs_renderer, 2.0f);
		}
		if (rendered_geometry_layers().should_show_geometry_focus_layer()) {
			paint_geometries(this, rendered_geometry_layers().geometry_focus_layer(), d_nurbs_renderer, 2.5f);
		}
		if (rendered_geometry_layers().should_show_pole_manipulation_layer()) {
			paint_geometries(this, rendered_geometry_layers().pole_manipulation_layer(), d_nurbs_renderer, 1.5f);
		}

		glDepthRange(0.4, 0.5);
		paint_geometries(this, rendered_geometry_layers().mouse_movement_layer(), d_nurbs_renderer, 1.5f);

	glPopMatrix();
}

void
GPlatesGui::Globe::paint_vector_output()
{
	d_grid.paint_circumference(GPlatesGui::Colour::GREY);

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
		d_grid.paint(Colour::GREY);

		glDepthRange(0.6, 0.7);
		paint_geometries(this, rendered_geometry_layers().reconstruction_layer(), d_nurbs_renderer, 1.5f);

		glDepthRange(0.5, 0.6);
		if (rendered_geometry_layers().should_show_geometry_focus_layer()) {
			paint_geometries(this, rendered_geometry_layers().geometry_focus_layer(), d_nurbs_renderer, 2.5f);
		}
		if (rendered_geometry_layers().should_show_pole_manipulation_layer()) {
			paint_geometries(this, rendered_geometry_layers().pole_manipulation_layer(), d_nurbs_renderer, 1.5f);
		}

	glPopMatrix();
}

void
GPlatesGui::Globe::initialise_texture()
{
//	d_texture.generate_test_texture2();
}

void
GPlatesGui::Globe::toggle_raster_image()
{
	d_texture.toggle();

}

void
GPlatesGui::Globe::enable_raster_display()
{
	d_texture.set_enabled(true);
}

void
GPlatesGui::Globe::disable_raster_display()
{
	d_texture.set_enabled(false);
}

