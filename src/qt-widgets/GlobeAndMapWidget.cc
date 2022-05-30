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

#include "gui/Globe.h"
#include "gui/GlobeCamera.h"
#include "gui/MapCamera.h"
#include "gui/Projection.h"
#include "gui/ViewportZoom.h"

#include "opengl/GLContext.h"

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
				this)),
	d_map_canvas_ptr(
			new MapCanvas(
				d_view_state,
				*d_globe_canvas_ptr,
				this)),
	d_layout(new QStackedLayout(this)),
	d_active_view_ptr(d_globe_canvas_ptr.get()),
	d_zoom_enabled(true)
{
	// Add the globe and the map to this widget.
	d_layout->addWidget(d_globe_canvas_ptr.get());
	d_layout->addWidget(d_map_canvas_ptr.get());

	// Make sure the cursor is always an arrow.
	d_globe_canvas_ptr->setCursor(Qt::ArrowCursor);
	d_map_canvas_ptr->setCursor(Qt::ArrowCursor);

	// Set up signals and slots.
	make_signal_slot_connections();

#ifdef GPLATES_PINCH_ZOOM_ENABLED
	grabGesture(Qt::PinchGesture);
#endif

	// Globe is the active view by default.
	d_layout->setCurrentWidget(d_globe_canvas_ptr.get());
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
	return d_active_view_ptr == d_map_canvas_ptr.get();
}


QSize
GPlatesQtWidgets::GlobeAndMapWidget::sizeHint() const
{
	return d_globe_canvas_ptr->sizeHint();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::make_signal_slot_connections()
{
	// Handle changes in the projection.
	GPlatesGui::Projection &projection = d_view_state.get_projection();
	QObject::connect(
			&projection,
			SIGNAL(globe_map_projection_about_to_change(const GPlatesGui::Projection &)),
			this,
			SLOT(about_to_change_globe_map_projection(const GPlatesGui::Projection &)));
	QObject::connect(
			&projection,
			SIGNAL(projection_changed(const GPlatesGui::Projection &)),
			this,
			SLOT(change_projection(const GPlatesGui::Projection &)));

	// Get notified when globe and map get repainted.
	QObject::connect(
			d_globe_canvas_ptr.get(),
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));
	QObject::connect(
			d_map_canvas_ptr.get(),
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));
}


void
GPlatesQtWidgets::GlobeAndMapWidget::handle_globe_or_map_repainted(
		bool mouse_down)
{
	Q_EMIT repainted(mouse_down);
}


void
GPlatesQtWidgets::GlobeAndMapWidget::about_to_change_globe_map_projection(
		const GPlatesGui::Projection &projection)
{
	// Save the camera position of the currently active view before we potentially change
	// to a different view (eg, globe to map view or vice versa).
	d_active_camera_viewpoint = get_active_camera().get_look_at_position_on_globe();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::change_projection(
		const GPlatesGui::Projection &projection)
{
	const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection = projection.get_globe_map_projection();
	const GPlatesGui::Projection::viewport_projection_type viewport_projection = projection.get_viewport_projection();

	if (globe_map_projection.is_viewing_globe_projection())  // viewing globe projection...
	{
		// Switch to globe.
		d_layout->setCurrentWidget(d_globe_canvas_ptr.get());
		d_active_view_ptr = d_globe_canvas_ptr.get();
	}
	else // viewing map projection...
	{
		// Switch to map.
		d_layout->setCurrentWidget(d_map_canvas_ptr.get());
		d_active_view_ptr = d_map_canvas_ptr.get();

		// Update the map canvas's map projection.
		d_view_state.get_map_projection().set_projection_type(
				globe_map_projection.get_map_projection_type());
		d_view_state.get_map_projection().set_central_meridian(
				globe_map_projection.get_map_central_meridian());
	}

	// Set the camera viewport projection (orthographic/perspective).
	d_active_view_ptr->get_camera().set_viewport_projection(viewport_projection);

	// Set the camera look-at position.
	if (d_active_camera_viewpoint)
	{
		d_active_view_ptr->get_camera().move_look_at_position_on_globe(d_active_camera_viewpoint.get());
	}

	d_active_view_ptr->update_canvas();

	Q_EMIT update_tools_and_status_message();
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


GPlatesQtWidgets::MapCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_map_canvas()
{
	return *d_map_canvas_ptr;
}


const GPlatesQtWidgets::MapCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_map_canvas() const
{
	return *d_map_canvas_ptr;
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


GPlatesGui::Camera &
GPlatesQtWidgets::GlobeAndMapWidget::get_active_camera()
{
	return d_active_view_ptr->get_camera();
}


const GPlatesGui::Camera &
GPlatesQtWidgets::GlobeAndMapWidget::get_active_camera() const
{
	return d_active_view_ptr->get_camera();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::resizeEvent(
		QResizeEvent *resize_event)
{
	Q_EMIT resized(resize_event->size().width(), resize_event->size().height());
}


QSize
GPlatesQtWidgets::GlobeAndMapWidget::get_viewport_size() const
{
	return d_active_view_ptr->get_viewport_size();
}


QImage
GPlatesQtWidgets::GlobeAndMapWidget::render_to_qimage(
		const QSize &image_size_in_device_independent_pixels)
{
	return d_active_view_ptr->render_to_qimage(image_size_in_device_independent_pixels);
}


GPlatesOpenGL::GLContext::non_null_ptr_type
GPlatesQtWidgets::GlobeAndMapWidget::get_active_gl_context()
{
	if (d_active_view_ptr == d_globe_canvas_ptr.get())
	{
		return d_globe_canvas_ptr->get_gl_context();
	}

	return d_map_canvas_ptr->get_gl_context();
}


GPlatesOpenGL::GLVisualLayers::non_null_ptr_type
GPlatesQtWidgets::GlobeAndMapWidget::get_active_gl_visual_layers()
{
	if (d_active_view_ptr == d_globe_canvas_ptr.get())
	{
		return d_globe_canvas_ptr->get_gl_visual_layers();
	}

	return d_map_canvas_ptr->get_gl_visual_layers();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::update_canvas()
{
	d_active_view_ptr->update_canvas();
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
		int delta = wheel_event->angleDelta().y();
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
					GPlatesGui::GlobeCamera &globe_camera = d_view_state.get_globe_camera();
					// We want to rotate the globe clockwise which means rotating the camera anticlockwise.
					globe_camera.rotate_anticlockwise(angle);
				}
				else
				{
					GPlatesGui::MapCamera &map_camera = d_view_state.get_map_camera();
					// We want to rotate the map clockwise which means rotating the camera anticlockwise.
					map_camera.rotate_anticlockwise(angle);
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

