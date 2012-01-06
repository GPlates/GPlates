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

#include <boost/foreach.hpp>
#include <QHBoxLayout>

#include "GlobeAndMapWidget.h"
#ifdef GPLATES_PINCH_ZOOM_ENABLED // Defined in .h file.
#include <QPinchGesture>
#endif

#include "GlobeCanvas.h"
#include "MapCanvas.h"
#include "MapView.h"

#include "gui/ColourScheme.h"
#include "gui/MapTransform.h"
#include "gui/SimpleGlobeOrientation.h"
#include "gui/ViewportProjection.h"
#include "gui/ViewportZoom.h"

#include "presentation/ViewState.h"


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
	d_map_view_ptr(
			new MapView(
				d_view_state,
				d_view_state.get_colour_scheme(),
				this,
				d_globe_canvas_ptr.get(),
				d_globe_canvas_ptr->get_gl_context(),
				d_globe_canvas_ptr->get_persistent_opengl_objects())),
	d_layout(new QStackedLayout(this)),
	d_active_view_ptr(d_globe_canvas_ptr.get()),
	d_zoom_enabled(true)
{
	init();

	// Globe is the active view by default.
	d_layout->setCurrentWidget(d_globe_canvas_ptr.get());
}


// For cloning GlobeAndMapWidget
GPlatesQtWidgets::GlobeAndMapWidget::GlobeAndMapWidget(
		const GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
		QWidget *parent_) :
	QWidget(parent_),
	d_view_state(existing_globe_and_map_widget_ptr->d_view_state),
	d_globe_canvas_ptr(
			existing_globe_and_map_widget_ptr->d_globe_canvas_ptr->clone(
				colour_scheme,
				this)),
	d_map_view_ptr(
			new MapView(
				d_view_state,
				colour_scheme,
				this,
				d_globe_canvas_ptr.get(),
				d_globe_canvas_ptr->get_gl_context(),
				d_globe_canvas_ptr->get_persistent_opengl_objects())),
	d_layout(new QStackedLayout(this)),
	d_active_view_ptr(
			existing_globe_and_map_widget_ptr->is_globe_active()
			? static_cast<SceneView *>(d_globe_canvas_ptr.get())
			: static_cast<SceneView *>(d_map_view_ptr.get())),
	d_zoom_enabled(existing_globe_and_map_widget_ptr->d_zoom_enabled)
{
	init();

	// Copy which of globe and map is active.
	if (existing_globe_and_map_widget_ptr->is_globe_active())
	{
		d_layout->setCurrentWidget(d_globe_canvas_ptr.get());
	}
	else
	{
		d_layout->setCurrentWidget(d_map_view_ptr.get());
	}
}


void
GPlatesQtWidgets::GlobeAndMapWidget::init()
{
	// Add the globe and the map to this widget.
	d_layout->addWidget(d_globe_canvas_ptr.get());
	d_layout->addWidget(d_map_view_ptr.get());

	// Make sure the cursor is always an arrow.
	d_globe_canvas_ptr->setCursor(Qt::ArrowCursor);
	d_map_view_ptr->setCursor(Qt::ArrowCursor);

	// Set up signals and slots.
	make_signal_slot_connections();

#ifdef GPLATES_PINCH_ZOOM_ENABLED
	grabGesture(Qt::PinchGesture);
#endif
}


GPlatesQtWidgets::GlobeAndMapWidget::~GlobeAndMapWidget()
{  }


bool
GPlatesQtWidgets::GlobeAndMapWidget::is_globe_active() const
{
	return d_active_view_ptr == d_globe_canvas_ptr.get();
}


bool
GPlatesQtWidgets::GlobeAndMapWidget::is_map_active() const
{
	return d_active_view_ptr == d_map_view_ptr.get();
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
			d_globe_canvas_ptr.get(),
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));
	QObject::connect(
			d_map_view_ptr.get(),
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
	d_map_view_ptr->map_canvas().map().set_projection_type(
		view_projection.get_projection_type());
	d_map_view_ptr->map_canvas().map().set_central_meridian(
		view_projection.get_central_meridian());

	// Save the existing camera llp.
	boost::optional<GPlatesMaths::LatLonPoint> camera_llp = get_camera_llp();

	if (view_projection.get_projection_type() == GPlatesGui::MapProjection::ORTHOGRAPHIC)
	{
		// Switch to globe.
		d_active_view_ptr = d_globe_canvas_ptr.get();
		d_globe_canvas_ptr->update_canvas();
		if (camera_llp)
		{
			d_globe_canvas_ptr->set_camera_viewpoint(*camera_llp);
		}
		d_layout->setCurrentWidget(d_globe_canvas_ptr.get());
	}
	else
	{
		// Switch to map.
		d_active_view_ptr = d_map_view_ptr.get();
		// d_map_view_ptr->set_view();
		d_map_view_ptr->update_canvas();
		if (camera_llp)
		{
			d_map_view_ptr->set_camera_viewpoint(*camera_llp);	
		}
		d_layout->setCurrentWidget(d_map_view_ptr.get());
	}
	
	emit update_tools_and_status_message();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::handle_zoom_change()
{
	d_active_view_ptr->handle_zoom_change();
}


GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_globe_canvas()
{
	return *d_globe_canvas_ptr;
}


const GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_globe_canvas() const
{
	return *d_globe_canvas_ptr;
}


GPlatesQtWidgets::MapView &
GPlatesQtWidgets::GlobeAndMapWidget::get_map_view()
{
	return *d_map_view_ptr;
}


const GPlatesQtWidgets::MapView &
GPlatesQtWidgets::GlobeAndMapWidget::get_map_view() const
{
	return *d_map_view_ptr;
}


GPlatesQtWidgets::SceneView &
GPlatesQtWidgets::GlobeAndMapWidget::get_active_view()
{
	return *d_active_view_ptr;
}


const GPlatesQtWidgets::SceneView &
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


void
GPlatesQtWidgets::GlobeAndMapWidget::repaint_canvas()
{
	d_active_view_ptr->repaint_canvas();
}


double
GPlatesQtWidgets::GlobeAndMapWidget::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe) const
{
	return d_active_view_ptr->current_proximity_inclusion_threshold(click_pos_on_globe);
}


void
GPlatesQtWidgets::GlobeAndMapWidget::set_zoom_enabled(
		bool enabled)
{
	d_zoom_enabled = enabled;
}


void
GPlatesQtWidgets::GlobeAndMapWidget::wheelEvent(
		QWheelEvent *wheel_event) 
{
	if (d_zoom_enabled)
	{
		int delta = wheel_event->delta();
		if (delta == 0)
		{
			return;
		}

		GPlatesGui::ViewportZoom &viewport_zoom = d_view_state.get_viewport_zoom();

		// The number 120 is derived from the Qt docs for QWheelEvent:
		// http://doc.trolltech.com/4.3/qwheelevent.html#delta
		static const int NUM_UNITS_PER_STEP = 120;

		double num_levels = static_cast<double>(std::abs(delta)) / NUM_UNITS_PER_STEP;
		if (delta > 0)
		{
			viewport_zoom.zoom_in(num_levels);
		}
		else
		{
			viewport_zoom.zoom_out(num_levels);
		}
	}
	else
	{
		wheel_event->ignore();
	}
}


#ifdef GPLATES_PINCH_ZOOM_ENABLED
#include <QDebug>
bool
GPlatesQtWidgets::GlobeAndMapWidget::event(
		QEvent *ev)
{
	if (ev->type() == QEvent::Gesture)
	{
		if (!d_zoom_enabled)
		{
			return false;
		}

		QGestureEvent *gesture_ev = static_cast<QGestureEvent *>(ev);
		bool pinch_gesture_found = false;

		BOOST_FOREACH(QGesture *gesture, gesture_ev->activeGestures())
		{
			if (gesture->gestureType() == Qt::PinchGesture)
			{
				gesture_ev->accept(gesture);
				pinch_gesture_found = true;

				QPinchGesture *pinch_gesture = static_cast<QPinchGesture *>(gesture);

				// Handle the scaling component of the pinch gesture.
				GPlatesGui::ViewportZoom &viewport_zoom = d_view_state.get_viewport_zoom();
				if (pinch_gesture->state() == Qt::GestureStarted)
				{
					viewport_zoom_at_start_of_pinch = viewport_zoom.zoom_percent();
				}

				viewport_zoom.set_zoom_percent(*viewport_zoom_at_start_of_pinch *
						pinch_gesture->scaleFactor());

				if (pinch_gesture->state() == Qt::GestureFinished)
				{
					viewport_zoom_at_start_of_pinch = boost::none;
				}

				// Handle the rotation component of the pinch gesture.
				double angle = pinch_gesture->rotationAngle() - pinch_gesture->lastRotationAngle();
				if (is_globe_active())
				{
					GPlatesGui::SimpleGlobeOrientation &orientation = d_globe_canvas_ptr->globe().orientation();
					orientation.rotate_camera(-angle);
				}
				else
				{
					d_view_state.get_map_transform().rotate(-angle);
				}
			}
		}

		return pinch_gesture_found;
	}
	else
	{
		return QWidget::event(ev);
	}
}
#endif

