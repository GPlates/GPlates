/* $Id$ */

/**
 * @file 
 * Contains the implementation of the Map class.
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

#include <opengl/OpenGL.h>

#include "Colour.h"
#include "Map.h"
#include "MapCanvasPainter.h"

#include "maths/LatLonPoint.h"


namespace
{
	void
	set_opengl_flags()
	{
		glShadeModel(GL_SMOOTH);
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
	}
}


GPlatesGui::Map::Map(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		ViewportZoom &viewport_zoom,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_visual_layers(visual_layers),
	d_render_settings(render_settings),
	d_viewport_zoom(viewport_zoom),
	d_colour_scheme(colour_scheme)
{
}


GPlatesGui::MapProjection &
GPlatesGui::Map::projection()
{
	return d_projection;
}


const GPlatesGui::MapProjection &
GPlatesGui::Map::projection() const
{
	return d_projection;
}


GPlatesGui::ProjectionType
GPlatesGui::Map::projection_type() const
{
	return d_projection.projection_type();
}


void
GPlatesGui::Map::set_projection_type(
		GPlatesGui::ProjectionType projection_type_)
{
	d_projection.set_projection_type(projection_type_);
}


double
GPlatesGui::Map::central_meridian()
{
	return d_projection.central_llp().longitude();
}


void
GPlatesGui::Map::set_central_meridian(
		double central_meridian_)
{
	GPlatesMaths::LatLonPoint llp(0.0, central_meridian_);
	d_projection.set_central_llp(llp);
}


void
GPlatesGui::Map::set_text_renderer(
		TextRenderer::ptr_to_const_type text_renderer_ptr)
{
	d_text_renderer_ptr = text_renderer_ptr;
}


void
GPlatesGui::Map::draw_grid_lines()
{
	static const int NUM_LAT_LINES = 7;
	static const int NUM_LON_LINES = 13;
	static const int NUM_LAT_VERTICES = 100;
	static const int NUM_LON_VERTICES = 200;

	// LAT_MARGIN represents the number of degrees above -90 and below 90 between
	// which each line of longitude will be drawn. This gives you a little space
	// at the top and bottom, so that grid lines which converge at the poles don't
	// look too busy. 

	//static const double LAT_MARGIN = 1.5;
	static const double LAT_MARGIN = 0.;

	double lat_line_spacing = 180./(NUM_LAT_LINES-1);
	double lon_line_spacing = 360./(NUM_LON_LINES-1);

	double lat_vertex_spacing = 360./NUM_LAT_VERTICES;
	double lon_vertex_spacing = (180.- 2.*LAT_MARGIN)/NUM_LON_VERTICES;

	double lat_0 = -90.;
	double lon_0 = d_projection.central_llp().longitude()-180.;

	double lat,lon;
	double screen_x,screen_y;

	glColor3fv(GPlatesGui::Colour::get_silver());
	glLineWidth(1.0f);

	// Lines of latitude.
	lat = lat_0;
	for (int count1 = 0; count1 < NUM_LAT_LINES ; count1++)
	{
		glBegin(GL_LINE_STRIP);
		lon = lon_0;
		screen_x = lon;
		screen_y = lat;
		d_projection.forward_transform(screen_x,screen_y);
		glVertex2d(screen_x,screen_y);
		for (int count2 = 0; count2 < NUM_LAT_VERTICES; count2++)
		{
			lon += lat_vertex_spacing;
			screen_x = lon;
			screen_y = lat;
			d_projection.forward_transform(screen_x,screen_y);
			glVertex2d(screen_x,screen_y);
		}
		glEnd();
		lat += lat_line_spacing;
	}


	// Lines of longitude.
	lon = lon_0;
	for (int count1 = 0; count1 < NUM_LON_LINES ; count1++)
	{
		glBegin(GL_LINE_STRIP);
		lat = lat_0 + LAT_MARGIN;
		screen_x = lon;
		screen_y = lat;
		d_projection.forward_transform(screen_x,screen_y);
		glVertex2d(screen_x,screen_y);
		for (int count2 = 0; count2 < NUM_LON_VERTICES; count2++)
		{
			lat += lon_vertex_spacing;
			screen_x = lon;
			screen_y = lat;
			d_projection.forward_transform(screen_x,screen_y);
			glVertex2d(screen_x,screen_y);
		}
		glEnd();
		lon += lon_line_spacing;
	}	

}


void
GPlatesGui::Map::draw_background()
{
	set_opengl_flags();
	
	glClearColor(0.35f,0.35f,0.35f,1.0f);

	glClear(GL_COLOR_BUFFER_BIT);

	try
	{
		draw_grid_lines();
	}
	catch(const GPlatesGui::ProjectionException &e)
	{
		std::cerr << e << std::endl;
	}
}


void
GPlatesGui::Map::paint(
		float scale)
{
	try
	{
		set_opengl_flags();

		double inverse_zoom_factor = 1.0 / d_viewport_zoom.zoom_factor();
		GPlatesGui::MapCanvasPainter map_canvas_painter(
				*this,
				d_visual_layers,
				d_render_settings,
				d_text_renderer_ptr,
				d_update_type,
				inverse_zoom_factor,
				d_colour_scheme);
		map_canvas_painter.set_scale(scale);
		d_rendered_geometry_collection->accept_visitor(map_canvas_painter);
	}
	catch (const GPlatesGlobal::Exception &e)
	{
		std::cerr << e << std::endl;
	}
}


void
GPlatesGui::Map::set_update_type(
		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type)
{
	d_update_type = update_type;
}

