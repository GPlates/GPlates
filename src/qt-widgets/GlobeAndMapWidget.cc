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
#include <QHBoxLayout>

#include "GlobeAndMapWidget.h"
#ifdef GPLATES_PINCH_ZOOM_ENABLED // Defined in GlobeAndMapWidget.h
#include <QPinchGesture>
#endif

#include "GlobeAndMapCanvas.h"

#include "gui/Camera.h"
#include "gui/Projection.h"
#include "gui/ViewportZoom.h"

#include "opengl/GLContext.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::GlobeAndMapWidget::GlobeAndMapWidget(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_view_state(view_state),
	d_globe_and_map_canvas_ptr(new GlobeAndMapCanvas(d_view_state, this)),
	d_zoom_enabled(true)
{
	// Add the globe and map to this widget.
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(d_globe_and_map_canvas_ptr.get());
	layout->setContentsMargins(0, 0, 0, 0);

	// Make sure the cursor is always an arrow.
	d_globe_and_map_canvas_ptr->setCursor(Qt::ArrowCursor);

	// Get notified when globe and map get repainted.
	QObject::connect(
			d_globe_and_map_canvas_ptr.get(),
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));

#ifdef GPLATES_PINCH_ZOOM_ENABLED
	grabGesture(Qt::PinchGesture);
#endif
}


GPlatesQtWidgets::GlobeAndMapWidget::~GlobeAndMapWidget()
{  }


QSize
GPlatesQtWidgets::GlobeAndMapWidget::sizeHint() const
{
	return d_globe_and_map_canvas_ptr->sizeHint();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::handle_globe_or_map_repainted(
		bool mouse_down)
{
	Q_EMIT repainted(mouse_down);
}


GPlatesQtWidgets::GlobeAndMapCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_globe_and_map_canvas()
{
	return *d_globe_and_map_canvas_ptr;
}


const GPlatesQtWidgets::GlobeAndMapCanvas &
GPlatesQtWidgets::GlobeAndMapWidget::get_globe_and_map_canvas() const
{
	return *d_globe_and_map_canvas_ptr;
}


GPlatesGui::Camera &
GPlatesQtWidgets::GlobeAndMapWidget::get_active_camera()
{
	return d_globe_and_map_canvas_ptr->get_active_camera();
}


const GPlatesGui::Camera &
GPlatesQtWidgets::GlobeAndMapWidget::get_active_camera() const
{
	return d_globe_and_map_canvas_ptr->get_active_camera();
}


bool
GPlatesQtWidgets::GlobeAndMapWidget::is_globe_active() const
{
	return d_globe_and_map_canvas_ptr->is_globe_active();
}


bool
GPlatesQtWidgets::GlobeAndMapWidget::is_map_active() const
{
	return d_globe_and_map_canvas_ptr->is_map_active();
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
	return d_globe_and_map_canvas_ptr->get_viewport_size();
}


QImage
GPlatesQtWidgets::GlobeAndMapWidget::render_to_qimage(
		const QSize &image_size_in_device_independent_pixels)
{
	return d_globe_and_map_canvas_ptr->render_to_qimage(image_size_in_device_independent_pixels);
}


void
GPlatesQtWidgets::GlobeAndMapWidget::render_opengl_feedback_to_paint_device(
		QPaintDevice &feedback_paint_device)
{
	d_globe_and_map_canvas_ptr->render_opengl_feedback_to_paint_device(feedback_paint_device);
}


GPlatesOpenGL::GLContext::non_null_ptr_type
GPlatesQtWidgets::GlobeAndMapWidget::get_gl_context()
{
	return d_globe_and_map_canvas_ptr->get_gl_context();
}


GPlatesOpenGL::GLVisualLayers::non_null_ptr_type
GPlatesQtWidgets::GlobeAndMapWidget::get_gl_visual_layers()
{
	return d_globe_and_map_canvas_ptr->get_gl_visual_layers();
}


void
GPlatesQtWidgets::GlobeAndMapWidget::update_canvas()
{
	d_globe_and_map_canvas_ptr->update_canvas();
}


double
GPlatesQtWidgets::GlobeAndMapWidget::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe) const
{
	return d_globe_and_map_canvas_ptr->current_proximity_inclusion_threshold(click_pos_on_globe);
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

		for (QGesture *gesture : gesture_ev->activeGestures())
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
				// We want to rotate the globe or map clockwise which means rotating the camera anticlockwise.
				get_active_camera().rotate_anticlockwise(angle);
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

