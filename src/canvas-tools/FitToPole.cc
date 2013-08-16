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
#include "view-operations/RenderedGeometryProximity.h"
#include "FitToPole.h"


GPlatesCanvasTools::FitToPole::FitToPole(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::HellingerDialog &hellinger_dialog) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection_ptr(&rendered_geom_collection),
	d_hellinger_dialog_ptr(&hellinger_dialog)
{
}

void
GPlatesCanvasTools::FitToPole::handle_activation()
{
	set_status_bar_message(QT_TR_NOOP(""));
}

void
GPlatesCanvasTools::FitToPole::handle_deactivation()
{

}


void
GPlatesCanvasTools::FitToPole::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	qDebug() << "Left click in fit-to-pole tool";

	if (!is_on_earth)
	{
		return;
	}


	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;
	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_hellinger_dialog_ptr->get_pick_layer()))
	{
		const unsigned int index = sorted_hits.front().d_rendered_geom_index;
		d_hellinger_dialog_ptr->highlight_selected_pick(index);
	}
	else
	{
		d_hellinger_dialog_ptr->clear_selection_layer();
	}
}



void
GPlatesCanvasTools::FitToPole::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;
	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_hellinger_dialog_ptr->get_pick_layer()))
	{
		const unsigned int index = sorted_hits.front().d_rendered_geom_index;
		d_hellinger_dialog_ptr->highlight_hovered_pick(index);
	}
	else
	{
		d_hellinger_dialog_ptr->clear_highlight_layer();
	}
}

void
GPlatesCanvasTools::FitToPole::paint()
{
#if 0
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
#endif
}



