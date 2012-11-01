/* $Id$ */

/**
 * @file 
 *
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#include <iterator>
#include <QObject>

#include "maths/SmallCircle.h"
#include "qt-widgets/SmallCircleWidget.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "CreateSmallCircle.h"


GPlatesCanvasTools::CreateSmallCircle::CreateSmallCircle(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::SmallCircleWidget &small_circle_widget) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection_ptr(&rendered_geom_collection),
	d_small_circle_layer_ptr(rendered_geom_collection.get_main_rendered_layer(main_rendered_layer_type)),
	d_small_circle_widget_ptr(&small_circle_widget),
	d_small_circle_collection_ref(small_circle_widget.small_circle_collection()),
	d_circle_is_being_drawn(false)
{

	QObject::connect(d_small_circle_widget_ptr,SIGNAL(clear_geometries()),
			this,SLOT(handle_clear_geometries()));
}

void
GPlatesCanvasTools::CreateSmallCircle::handle_activation()
{
    set_status_bar_message(QT_TR_NOOP("Click to mark the centre and radius. Shift+click to add more radii."));

	d_small_circle_layer_ptr->set_active();
	d_small_circle_widget_ptr->setEnabled(true);
}

void
GPlatesCanvasTools::CreateSmallCircle::handle_deactivation()
{
	d_small_circle_layer_ptr->set_active(false);
	d_small_circle_widget_ptr->setEnabled(false);
}


void
GPlatesCanvasTools::CreateSmallCircle::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	if (!is_on_earth)
	{
		return;
	}


	if (!d_circle_is_being_drawn)
	{
		d_circle_is_being_drawn = true;
		d_centre.reset(point_on_sphere);
		d_point_on_radius.reset();
		d_small_circle_collection_ref.clear();
		d_small_circle_widget_ptr->update_current_centre(*d_centre);
	}
	else
	{
		d_circle_is_being_drawn = false;
		d_point_on_radius.reset(point_on_sphere);
		GPlatesMaths::SmallCircle circle = GPlatesMaths::SmallCircle::create(d_centre->position_vector(),*d_point_on_radius);
		d_small_circle_collection_ref.push_back(circle);
		d_small_circle_widget_ptr->update_radii();
	}
	paint();
}



void
GPlatesCanvasTools::CreateSmallCircle::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{


	if (d_circle_is_being_drawn)
	{
		d_point_on_radius.reset(point_on_sphere);
		GPlatesMaths::SmallCircle circle = GPlatesMaths::SmallCircle::create(d_centre->position_vector(),*d_point_on_radius);
		d_small_circle_widget_ptr->update_radii(circle.colatitude().dval());
	}
	paint();
}

void
GPlatesCanvasTools::CreateSmallCircle::handle_shift_left_click(
	const GPlatesMaths::PointOnSphere &point_on_sphere,
	bool is_on_earth,
	double proximity_inclusion_threshold)
{



	if (d_circle_is_being_drawn)
	{
		d_point_on_radius.reset(point_on_sphere);
		GPlatesMaths::SmallCircle circle = GPlatesMaths::SmallCircle::create(d_centre->position_vector(),*d_point_on_radius);
		d_small_circle_collection_ref.push_back(circle);
		d_small_circle_widget_ptr->update_radii();
	}
	paint();
}

void
GPlatesCanvasTools::CreateSmallCircle::paint()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Draw any circles in the widget's collection.
	d_small_circle_widget_ptr->update_small_circle_layer();

	// Draw the tool's current circle.
	if (d_centre)
	{
		GPlatesViewOperations::RenderedGeometry rendered_point = 
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(*d_centre);

			d_small_circle_layer_ptr->add_rendered_geometry(rendered_point);
	}

	if (d_centre && d_point_on_radius)
	{	
		GPlatesViewOperations::RenderedGeometry rendered_circle =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
					GPlatesMaths::SmallCircle::create(d_centre->position_vector(),*d_point_on_radius));

		d_small_circle_layer_ptr->add_rendered_geometry(rendered_circle);
	}

}

void
GPlatesCanvasTools::CreateSmallCircle::handle_clear_geometries()
{
	d_centre.reset();
	d_point_on_radius.reset();
	d_circle_is_being_drawn = false;
}


