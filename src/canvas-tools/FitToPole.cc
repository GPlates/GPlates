/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 Geological Survey of Norway
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

#include "qt-widgets/HellingerDialog.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "FitToPole.h"


GPlatesCanvasTools::FitToPole::FitToPole(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::HellingerDialog &hellinger_dialog) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection_ptr(&rendered_geom_collection),
	d_fit_to_pole_layer_ptr(rendered_geom_collection.get_main_rendered_layer(main_rendered_layer_type)),
	d_hellinger_dialog_ptr(&hellinger_dialog)
{
#if 0
	QObject::connect(d_small_circle_widget_ptr,SIGNAL(clear_geometries()),
			this,SLOT(handle_clear_geometries()));
#endif
}

void
GPlatesCanvasTools::FitToPole::handle_activation()
{
    set_status_bar_message(QT_TR_NOOP("Click to mark the centre and radius. Shift+click to add more radii."));

	d_fit_to_pole_layer_ptr->set_active();
	//d_small_circle_widget_ptr->setEnabled(true);
}

void
GPlatesCanvasTools::FitToPole::handle_deactivation()
{
	d_fit_to_pole_layer_ptr->set_active(false);
	//d_small_circle_widget_ptr->setEnabled(false);
}


void
GPlatesCanvasTools::FitToPole::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
#if 0
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
#endif
}



void
GPlatesCanvasTools::FitToPole::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{

#if 0
	if (d_circle_is_being_drawn)
	{
		d_point_on_radius.reset(point_on_sphere);
		GPlatesMaths::SmallCircle circle = GPlatesMaths::SmallCircle::create(d_centre->position_vector(),*d_point_on_radius);
		d_small_circle_widget_ptr->update_radii(circle.colatitude().dval());
	}
	paint();
#endif
}

void
GPlatesCanvasTools::FitToPole::handle_shift_left_click(
	const GPlatesMaths::PointOnSphere &point_on_sphere,
	bool is_on_earth,
	double proximity_inclusion_threshold)
{
#if 0
	if (d_circle_is_being_drawn)
	{
		d_point_on_radius.reset(point_on_sphere);
		GPlatesMaths::SmallCircle circle = GPlatesMaths::SmallCircle::create(d_centre->position_vector(),*d_point_on_radius);
		d_small_circle_collection_ref.push_back(circle);
		d_small_circle_widget_ptr->update_radii();
	}
	paint();
#endif
}

void
GPlatesCanvasTools::FitToPole::paint()
{
#if 0
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
#endif
}

void
GPlatesCanvasTools::FitToPole::handle_clear_geometries()
{

}


