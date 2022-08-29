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

#include "GlobeAndMapCanvas.h"

#include "opengl/GLContext.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::GlobeAndMapWidget::GlobeAndMapWidget(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_globe_and_map_canvas_ptr(new GlobeAndMapCanvas(view_state, this))
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


QSize
GPlatesQtWidgets::GlobeAndMapWidget::get_viewport_size() const
{
	return d_globe_and_map_canvas_ptr->get_viewport_size();
}


QImage
GPlatesQtWidgets::GlobeAndMapWidget::render_to_qimage(
		const QSize &image_size_in_device_independent_pixels,
		const GPlatesGui::Colour &image_clear_colour)
{
	return d_globe_and_map_canvas_ptr->render_to_qimage(image_size_in_device_independent_pixels, image_clear_colour);
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
