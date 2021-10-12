/* $Id$ */

/**
 * \file
 * Contains the implementation of the GlobeAndMapWidget class.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "GlobeAndMapWidget.h"
#include "GlobeCanvas.h"
#include "MapCanvas.h"
#include "MapView.h"
#include "gui/ColourScheme.h"
#include "gui/MapTransform.h"
#include "gui/ViewportProjection.h"
#include "presentation/ViewState.h"

#include <QHBoxLayout>

// Fresh GlobeAndMapWidget
GPlatesQtWidgets::GlobeAndMapWidget::GlobeAndMapWidget(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_view_state(view_state),
	d_globe_canvas_ptr(
			new GlobeCanvas(
				d_view_state,
				d_view_state.get_colour_scheme(),
				this)),
	d_map_canvas_ptr(
			new MapCanvas(
				d_view_state.get_rendered_geometry_collection(),
				d_view_state.get_render_settings(),
				d_view_state.get_viewport_zoom(),
				d_view_state.get_colour_scheme(),
				d_view_state,
				this)),
	d_map_view_ptr(
			new MapView(
				d_view_state,
				d_map_canvas_ptr,
				this))
{
	// Globe is the active view by default.
	d_active_view_ptr = d_globe_canvas_ptr;
	d_map_view_ptr->hide();

	init();
}

// For cloning GlobeAndMapWidget
GPlatesQtWidgets::GlobeAndMapWidget::GlobeAndMapWidget(
		GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
		QWidget *parent_) :
	QWidget(parent_),
	d_view_state(existing_globe_and_map_widget_ptr->d_view_state),
	d_globe_canvas_ptr(
			existing_globe_and_map_widget_ptr->d_globe_canvas_ptr->clone(
				colour_scheme,
				this)),
	d_map_canvas_ptr(
			new MapCanvas(
				d_view_state.get_rendered_geometry_collection(),
				d_view_state.get_render_settings(),
				d_view_state.get_viewport_zoom(),
				colour_scheme,
				d_view_state,
				this)),
	d_map_view_ptr(
			new MapView(
				d_view_state,
				d_map_canvas_ptr,
				this))
{
	// Copy which of globe and map is active.
	if (existing_globe_and_map_widget_ptr->is_globe_active())
	{
		d_active_view_ptr = d_globe_canvas_ptr;
		d_map_view_ptr->hide();
	}
	else
	{
		d_active_view_ptr = d_map_view_ptr;
		d_globe_canvas_ptr->hide();
	}

	init();
}

void
GPlatesQtWidgets::GlobeAndMapWidget::init()
{
	d_map_canvas_ptr->set_map_view_ptr(d_map_view_ptr);

	// Add the globe and the map to this widget.
	QHBoxLayout *globe_and_map_layout = new QHBoxLayout(this);
	globe_and_map_layout->setSpacing(0);
	globe_and_map_layout->setContentsMargins(0, 0, 0, 0);
	globe_and_map_layout->addWidget(d_globe_canvas_ptr);
	globe_and_map_layout->addWidget(d_map_view_ptr);

	// Make sure the cursor is always an arrow.
	d_globe_canvas_ptr->setCursor(Qt::ArrowCursor);
	d_map_view_ptr->setCursor(Qt::ArrowCursor);

	// Set up signals and slots.
	make_signal_slot_connections();
}

GPlatesQtWidgets::GlobeAndMapWidget::~GlobeAndMapWidget()
{
	delete d_globe_canvas_ptr;

	// Delete the view first because the view depends on the canvas.
	delete d_map_view_ptr;
	delete d_map_canvas_ptr;
}

bool
GPlatesQtWidgets::GlobeAndMapWidget::is_globe_active() const
{
	return d_active_view_ptr == d_globe_canvas_ptr;
}

bool
GPlatesQtWidgets::GlobeAndMapWidget::is_map_active() const
{
	return d_active_view_ptr == d_map_view_ptr;
}

QSize
GPlatesQtWidgets::GlobeAndMapWidget::sizeHint() const
{
	return d_globe_canvas_ptr->sizeHint();
}

void
GPlatesQtWidgets::GlobeAndMapWidget::make_signal_slot_connections()
{
	// Handle signals for change in zoom.
	GPlatesGui::ViewportZoom &vzoom = d_view_state.get_viewport_zoom();
	QObject::connect(
			&vzoom,
			SIGNAL(zoom_changed()),
			this,
			SLOT(handle_zoom_change()));

	// Handle changes in the projection.
	GPlatesGui::ViewportProjection &vprojection = d_view_state.get_viewport_projection();
	QObject::connect(
			&vprojection,
			SIGNAL(projection_type_changed(const GPlatesGui::ViewportProjection &)),
			this,
			SLOT(change_projection(const GPlatesGui::ViewportProjection &)));
	QObject::connect(
			&vprojection,
			SIGNAL(central_meridian_changed(const GPlatesGui::ViewportProjection &)),
			this,
			SLOT(change_projection(const GPlatesGui::ViewportProjection &)));

	// Get notified when globe and map get repainted.
	QObject::connect(
			d_globe_canvas_ptr,
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));
	QObject::connect(
			d_map_view_ptr,
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));
}

void
GPlatesQtWidgets::GlobeAndMapWidget::handle_globe_or_map_repainted(
		bool mouse_down)
{
	emit repainted(mouse_down);
}

void
GPlatesQtWidgets::GlobeAndMapWidget::change_projection(
		const GPlatesGui::ViewportProjection &view_projection)
{
	// Update the map canvas's projection.
	d_map_canvas_ptr->map().set_projection_type(
		view_projection.get_projection_type());
	d_map_canvas_ptr->map().set_central_meridian(
		view_projection.get_central_meridian());

	// Save the existing camera llp.
	boost::optional<GPlatesMaths::LatLonPoint> camera_llp = get_camera_llp();

	if (view_projection.get_projection_type() == GPlatesGui::ORTHOGRAPHIC)
	{
		// Switch to globe.
		d_active_view_ptr = d_globe_canvas_ptr;
		d_globe_canvas_ptr->update_canvas();
		if (camera_llp)
		{
			d_globe_canvas_ptr->set_camera_viewpoint(*camera_llp);
		}
		d_globe_canvas_ptr->show();
		d_map_view_ptr->hide();
	}
	else
	{
		// Switch to map.
		d_active_view_ptr = d_map_view_ptr;
		d_map_view_ptr->set_view();
		d_map_view_ptr->update_canvas();
		if (camera_llp)
		{
			d_map_view_ptr->set_camera_viewpoint(*camera_llp);	
		}
		d_globe_canvas_ptr->hide();
		d_map_view_ptr->show();
	}
	
	emit update_tools_and_status_message();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::handle_zoom_change()
{
	d_active_view_ptr->handle_zoom_change();
}

GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_globe_canvas() const
{
	return *d_globe_canvas_ptr;
}

GPlatesQtWidgets::MapCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_map_canvas() const
{
	return *d_map_canvas_ptr;
}

GPlatesQtWidgets::MapView &
GPlatesQtWidgets::GlobeAndMapWidget::get_map_view() const
{
	return *d_map_view_ptr;
}

GPlatesQtWidgets::SceneView &
GPlatesQtWidgets::GlobeAndMapWidget::get_active_view() const
{
	return *d_active_view_ptr;
}

boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::GlobeAndMapWidget::get_camera_llp() const
{
	return d_active_view_ptr->camera_llp();
}

void
GPlatesQtWidgets::GlobeAndMapWidget::set_mouse_wheel_enabled(
		bool enabled)
{
	d_globe_canvas_ptr->set_mouse_wheel_enabled(enabled);
	d_map_view_ptr->set_mouse_wheel_enabled(enabled);
}

void
GPlatesQtWidgets::GlobeAndMapWidget::resizeEvent(
		QResizeEvent *resize_event)
{
	emit resized(resize_event->size().width(), resize_event->size().height());
}

QImage
GPlatesQtWidgets::GlobeAndMapWidget::grab_frame_buffer()
{
	return d_active_view_ptr->grab_frame_buffer();
}

void
GPlatesQtWidgets::GlobeAndMapWidget::update_canvas()
{
	d_active_view_ptr->update_canvas();
}

