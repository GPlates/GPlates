/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include <QLocale>


#include "MapClickGeometry.h"

#include "CommonClickGeometry.h"
#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/FeaturePropertiesDialog.h"
#include "gui/MapProjection.h"
#include "gui/ProjectionException.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "global/InternalInconsistencyException.h"


	
void
GPlatesCanvasTools::MapClickGeometry::handle_activation()
{
	if (map_view().isVisible())
	{
		d_view_state_ptr->status_message(QObject::tr(
				"Click a geometry to choose a feature."
				" Shift+click to query immediately."
				" Ctrl+drag to pan the map."));

		// Activate the geometry focus hightlight layer.
		d_rendered_geom_collection->set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);

	}
	
}


void
GPlatesCanvasTools::MapClickGeometry::handle_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	if (!is_on_surface)
	{
		return;
	}

	double x = click_point_on_scene.x();
	double y = click_point_on_scene.y();


	boost::optional<GPlatesMaths::LatLonPoint> llp = 
		map_canvas().projection().inverse_transform(x,y);	

	if (llp) 
	{
		GPlatesMaths::PointOnSphere point_on_sphere = GPlatesMaths::make_point_on_sphere(*llp);

		double proximity_inclusion_threshold = map_view().current_proximity_inclusion_threshold(point_on_sphere);

		CommonClickGeometry::handle_left_click(
			point_on_sphere,
			proximity_inclusion_threshold,
			*d_view_state_ptr,
			*d_clicked_table_model_ptr,
			*d_feature_focus_ptr,
			*d_rendered_geom_collection);
	}

}


void
GPlatesCanvasTools::MapClickGeometry::handle_shift_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{

	handle_left_click(click_point_on_scene, is_on_surface);
	// If there is a feature focused, we'll assume that the user wants to look at it in detail.
	if (d_feature_focus_ptr->is_valid()) {
		fp_dialog().choose_query_widget_and_open();
	}

}
