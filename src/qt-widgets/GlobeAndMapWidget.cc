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
#include "gui/ViewportZoom.h"

#include "opengl/GLContext.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::GlobeAndMapWidget::GlobeAndMapWidget(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_globe_and_map_canvas_ptr(new GlobeAndMapCanvas(view_state))
#ifdef GPLATES_PINCH_ZOOM_ENABLED
	, d_viewport_zoom(view_state.get_viewport_zoom())
#endif
{
	// The globe-and-map canvas is a QWindow (QOpenGLWindow) so wrap it in a QWidget container.
	QWidget *globe_and_map_canvas_container =
			QWidget::createWindowContainer(d_globe_and_map_canvas_ptr.get(), this);

	// Add the globe-and-map canvas container to this widget.
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(globe_and_map_canvas_container);
	layout->setContentsMargins(0, 0, 0, 0);

	// Ensure the globe/map will always expand to fill available space.
	// A minumum size and non-collapsibility is set on the globe/map basically so users
	// can't obliterate it and then wonder where their globe/map went.
	QSizePolicy globe_and_map_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	globe_and_map_size_policy.setHorizontalStretch(255);
	setSizePolicy(globe_and_map_size_policy);
	setFocusPolicy(Qt::StrongFocus);
	// Pass focus to the globe-and-map container when we get focus.
	setFocusProxy(globe_and_map_canvas_container);
	setMinimumSize(100, 100);

	// Make sure the cursor is always an arrow.
	setCursor(Qt::ArrowCursor);

	// Get notified when globe and map get repainted.
	QObject::connect(
			d_globe_and_map_canvas_ptr.get(),
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_or_map_repainted(bool)));

#ifdef GPLATES_PINCH_ZOOM_ENABLED
	// Gestures are handled in this class (GlobeAndMapWidget) because this class inherits from
	// QWidget (which has the QWidget::grabGesture() method) whereas GlobeAndMapCanvas inherits from
	// QWindow via QOpenGLWindow (which does not have this method).
	grabGesture(Qt::PinchGesture);
#endif
}


GPlatesQtWidgets::GlobeAndMapWidget::~GlobeAndMapWidget()
{  }


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


QSize
GPlatesQtWidgets::GlobeAndMapWidget::get_viewport_size() const
{
	return d_globe_and_map_canvas_ptr->get_viewport_size();
}


QImage
GPlatesQtWidgets::GlobeAndMapWidget::render_to_image(
		const QSize &image_size_in_device_independent_pixels,
		const GPlatesGui::Colour &image_clear_colour)
{
	return d_globe_and_map_canvas_ptr->render_to_image(image_size_in_device_independent_pixels, image_clear_colour);
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


#ifdef GPLATES_PINCH_ZOOM_ENABLED
bool
GPlatesQtWidgets::GlobeAndMapWidget::event(
		QEvent *ev)
{
	if (ev->type() == QEvent::Gesture)
	{
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
				if (pinch_gesture->state() == Qt::GestureStarted)
				{
					d_viewport_zoom_at_start_of_pinch = d_viewport_zoom.zoom_percent();
				}

				d_viewport_zoom.set_zoom_percent(d_viewport_zoom_at_start_of_pinch.get() *
						pinch_gesture->scaleFactor());

				if (pinch_gesture->state() == Qt::GestureFinished)
				{
					d_viewport_zoom_at_start_of_pinch = boost::none;
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
