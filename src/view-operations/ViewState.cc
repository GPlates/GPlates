/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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


#include "ViewState.h"

#include "gui/FeatureFocus.h"


GPlatesViewOperations::ViewState::ViewState(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::FeatureFocus &feature_focus) :
	d_rendered_geometry_collection(rendered_geom_collection),
	/*d_active_view_ptr(NULL)*/
	d_viewport_projection(GPlatesGui::ORTHOGRAPHIC),
	d_geometry_focus_highlight(rendered_geom_collection)
{
#if 0
	// Create the GlobeCanvas.
	d_globe_canvas_ptr.reset(new GlobeCanvas(d_rendered_geometry_collection));
	// Create the MapCanvas.
	d_map_canvas_ptr.reset(new MapCanvas(d_rendered_geometry_collection));

	// Create the MapView.
	d_map_view_ptr.reset(new MapView(d_map_canvas_ptr.get()));

	// Set the active_view_ptr to the globe view.
	d_active_view_ptr = static_cast<SceneView*>(d_globe_canvas_ptr.get());

	// FIXME: we can do this in the view's constructor.
	d_map_view_ptr->setScene(d_map_canvas_ptr.get());

	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			this, SLOT(recalc_camera_position()));
	QObject::connect(d_map_view_ptr, SIGNAL(view_changed()),
			this, SLOT(recalc_camera_position()));
#endif

	// Connect the geometry-focus highlight to the feature focus.
	QObject::connect(&feature_focus, SIGNAL(focus_changed(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			&d_geometry_focus_highlight, SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));
	QObject::connect(&feature_focus, SIGNAL(focused_feature_modified(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
			&d_geometry_focus_highlight, SLOT(set_focus(
					GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));

	// Handle zoom changes.
	QObject::connect(&d_viewport_zoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));

	// Handle view projection changes.
	QObject::connect(
			&d_viewport_projection, SIGNAL(projection_type_changed(const GPlatesViewOperations::ViewportProjection &)),
			this, SLOT(handle_projection_type_change(const GPlatesViewOperations::ViewportProjection &)));
	QObject::connect(
			&d_viewport_projection, SIGNAL(central_meridian_changed(const GPlatesViewOperations::ViewportProjection &)),
			this, SLOT(handle_central_meridian_change(const GPlatesViewOperations::ViewportProjection &)));

#if 0
	QObject::connect(this,SIGNAL(update_tools_and_status_message()),
			&view_state,SLOT(update_tools_and_status_message()));

	recalc_camera_position();
#endif
}


GPlatesViewOperations::ViewState::~ViewState()
{
	// boost::scoped_ptr destructor requires complete type.
}


void
GPlatesViewOperations::ViewState::handle_zoom_change()
{
	d_rendered_geometry_collection.set_viewport_zoom_factor(d_viewport_zoom.zoom_factor());

#if 0
	d_active_view_ptr->handle_zoom_change();
#endif
}


void
GPlatesViewOperations::ViewState::handle_projection_type_change(
		const GPlatesViewOperations::ViewportProjection &viewport_projection)
{
#if 0
	// Update the map canvas' projection
	d_map_canvas_ptr->set_projection_type(
		viewport_projection.get_projection_type());

	switch(viewport_projection.get_projection_type())
	{
	case GPlatesGui::ORTHOGRAPHIC:
		d_active_view_ptr = d_globe_canvas_ptr;
		d_globe_canvas_ptr->update_canvas();
		if (d_camera_llp)
		{
			d_globe_canvas_ptr->set_camera_viewpoint(*d_camera_llp);
		}
		d_map_view_ptr->hide();
		d_globe_canvas_ptr->show();
		break;

	default:
		d_active_view_ptr = d_map_view_ptr;
		d_map_view_ptr->set_view();
		d_map_view_ptr->update_canvas();
		if (d_camera_llp)
		{
			d_map_view_ptr->set_camera_viewpoint(*d_camera_llp);	
		}
		d_globe_canvas_ptr->hide();
		d_map_view_ptr->show();
		break;
	}
	
	recalc_camera_position();
	emit update_tools_and_status_message();
#endif
}


void
GPlatesViewOperations::ViewState::handle_central_meridian_change(
		const GPlatesViewOperations::ViewportProjection &viewport_projection)
{
#if 0
	// Update the map canvas.
	d_map_canvas_ptr->set_central_meridian(
		viewport_projection.get_central_meridian());
#endif
}
