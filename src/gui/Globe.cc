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

#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"
#include "view-operations/RenderedGeometryUtils.h"


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
	 * This is a visitor to draw rendered geometries on-screen using OpenGL.
	 */
	class PaintGeometry :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor
	{
	public:
		explicit
		PaintGeometry(
				GPlatesGui::NurbsRenderer &nurbs_renderer):
			d_nurbs_renderer(&nurbs_renderer),
			d_current_layer_far_depth(0),
			d_depth_range_per_layer(0)
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
			const unsigned num_layers_to_draw = get_num_active_non_empty_layers(
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

			// If layer is not empty then we have allocated a depth range for it.
			if (!rendered_geometry_layer.is_empty())
			{
				const double far_depth = d_current_layer_far_depth;
				double near_depth = far_depth - d_depth_range_per_layer;
				if (near_depth < 0) near_depth = 0;

				glDepthRange(near_depth, far_depth);

				d_current_layer_far_depth -= d_depth_range_per_layer;
			}

			// We want to visit the RenderedGeometry objects in this layer to draw them.
			return true;
		}

		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
		{
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
			glColor3fv(rendered_polygon_on_sphere.get_colour());
			glLineWidth(rendered_polygon_on_sphere.get_line_width_hint() * LINE_WIDTH_ADJUSTMENT);

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
				rendered_polygon_on_sphere.get_polygon_on_sphere();

			draw_arcs_for_polygon(
					polygon_on_sphere->begin(),
					polygon_on_sphere->end(),
					*d_nurbs_renderer);
		}

	private:
		GPlatesGui::NurbsRenderer *const d_nurbs_renderer;
		double d_current_layer_far_depth;
		double d_depth_range_per_layer;

		//! Multiplying factor to get default point size of 1.0f to look like one screen-space pixel.
		static const float POINT_SIZE_ADJUSTMENT;
		//! Multiplying factor to get default line width of 1.0f to look like one screen-space pixel.
		static const float LINE_WIDTH_ADJUSTMENT;
	};

	const float PaintGeometry::POINT_SIZE_ADJUSTMENT = 1.5f;
	const float PaintGeometry::LINE_WIDTH_ADJUSTMENT = 1.5f;


	void
	paint_rendered_geometries(
			const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
			GPlatesGui::NurbsRenderer &nurbs_renderer,
			double depth_range_near,
			double depth_range_far)
	{
		PaintGeometry paint_geometry(nurbs_renderer);

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
GPlatesGui::Globe::paint()
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
		d_grid.paint(Colour::SILVER);

		paint_rendered_geometries(
				*d_rendered_geom_collection,
				*d_nurbs_renderer,
				0.0,
				0.7);

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
				0.0,
				0.7);

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

