/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway.
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

#include <QApplication>
#include <QDebug>
#include <QGLWidget>
#include <QGraphicsView>
#include <QPaintEngine>
#include <QPainter>

#include "gui/PlatesColourTable.h"
#include "gui/MapCanvasPainter.h"
#include "gui/MapProjection.h"
#include "MapCanvas.h"

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

GPlatesQtWidgets::MapCanvas::MapCanvas(
	GPlatesViewOperations::RenderedGeometryCollection &collection):
	d_rendered_geometry_collection(&collection),
	d_show_point(true),
	d_show_line(true),
	d_show_polygon(true),
	d_show_topology(true),
	d_show_arrows(true)
{

	// Give the scene a nice big rectangle.
	setSceneRect(QRect(-360,-180,720,360));

	QObject::connect(d_rendered_geometry_collection,
		SIGNAL(collection_was_updated(
			GPlatesViewOperations::RenderedGeometryCollection &,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
		this,
		SLOT(update_canvas(
			GPlatesViewOperations::RenderedGeometryCollection &,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)));

}

void
GPlatesQtWidgets::MapCanvas::set_projection_type(
	int projection_type_)
{
	d_map_projection.set_projection_type(projection_type_);
}

int
GPlatesQtWidgets::MapCanvas::projection_type()
{
	return d_map_projection.projection_type();
}

void
GPlatesQtWidgets::MapCanvas::set_central_meridian(
	double central_meridian_)
{
	GPlatesMaths::LatLonPoint llp(0.0, central_meridian_);
	d_map_projection.set_central_llp(llp);
}

double
GPlatesQtWidgets::MapCanvas::central_meridian()
{
	GPlatesMaths::LatLonPoint llp = d_map_projection.central_llp();
	return llp.longitude();
}

void
GPlatesQtWidgets::MapCanvas::drawBackground(
	QPainter *painter,
	const QRectF &rect)
{

	if (painter->paintEngine()->type() != QPaintEngine::OpenGL)
	{
		std::cerr << "Could not find an openGL paint engine." << std::endl;
		return;
	}

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
GPlatesQtWidgets::MapCanvas::draw_grid_lines()
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
	double lon_0 = d_map_projection.central_llp().longitude()-180.;

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
		d_map_projection.forward_transform(screen_x,screen_y);
		glVertex2d(screen_x,screen_y);
		for (int count2 = 0; count2 < NUM_LAT_VERTICES; count2++)
		{
			lon += lat_vertex_spacing;
			screen_x = lon;
			screen_y = lat;
			d_map_projection.forward_transform(screen_x,screen_y);
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
		d_map_projection.forward_transform(screen_x,screen_y);
		glVertex2d(screen_x,screen_y);
		for (int count2 = 0; count2 < NUM_LON_VERTICES; count2++)
		{
			lat += lon_vertex_spacing;
			screen_x = lon;
			screen_y = lat;
			d_map_projection.forward_transform(screen_x,screen_y);
			glVertex2d(screen_x,screen_y);
		}
		glEnd();
		lon += lon_line_spacing;
	}	

}

void
GPlatesQtWidgets::MapCanvas::drawItems(
	QPainter *painter, 
	int numItems, 
	QGraphicsItem *items_[], 
	const QStyleOptionGraphicsItem options[], 
	QWidget *widget)
{
	set_opengl_flags();
	
	GPlatesGui::MapCanvasPainter map_canvas_painter(*this,d_update_type);	
	
	d_rendered_geometry_collection->accept_visitor(map_canvas_painter);
}

#if 0
void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	update();
}
#endif

void
GPlatesQtWidgets::MapCanvas::update_canvas(
	GPlatesViewOperations::RenderedGeometryCollection &collection,
	GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type)
{
	d_update_type = update_type;
	update();
}

void
GPlatesQtWidgets::MapCanvas::draw_svg_output()
{

	// Switch off unwanted layers - taken from the Globe's painting routines. 

	// Get current rendered layer active state so we can restore later.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState
		prev_rendered_layer_active_state =
		d_rendered_geometry_collection->capture_main_layer_active_state();

	// Turn off rendering of digitisation layer.
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER,
		false);

	// Turn off rendering of mouse movement layer.
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::MOUSE_MOVEMENT_LAYER,
		false);


	update();

	// Force the update signal to be processed so that 
	// content is drawn to the openGL feedback buffer. 
	QApplication::processEvents();


	// Restore previous rendered layer active state.
	d_rendered_geometry_collection->restore_main_layer_active_state(
		prev_rendered_layer_active_state);

}
