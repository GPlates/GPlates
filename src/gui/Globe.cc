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

#include <QDebug>

#include "Globe.h"

#include "PlatesColourTable.h"
#include "Texture.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::Globe::Globe(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection) :
d_rendered_geom_collection(&rendered_geom_collection),
d_sphere( OpaqueSphereFactory(Colour(0.35f, 0.35f, 0.35f)) ),
d_grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON),
d_rendered_geom_collection_painter(rendered_geom_collection)
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

		// Draw the rendered geometries in the depth range [0, 0.7].
		d_rendered_geom_collection_painter.paint(
				viewport_zoom_factor,
				*d_nurbs_renderer,
				0.0,
				0.7);

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

		// Draw the rendered geometries in the depth range [0, 0.7].
		d_rendered_geom_collection_painter.paint(
				viewport_zoom_factor,
				*d_nurbs_renderer,
				0.0,
				0.7);

		// Restore previous rendered layer active state.
		d_rendered_geom_collection->restore_main_layer_active_state(
			prev_rendered_layer_active_state);

	glPopMatrix();
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

