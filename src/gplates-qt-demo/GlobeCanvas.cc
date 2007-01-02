/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include "GlobeCanvas.h"

#include <vector>
#include <cmath>

#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPointConversions.h"

#include "global/Exception.h"
#include "state/Layout.h"

#include "QtGui/QMouseEvent"

static const GLfloat FRAMING_RATIO = 1.07;

static const GLfloat EYE_X = 0.0, EYE_Y = 0.0, EYE_Z = -5.0;

namespace 
{

	static
	GPlatesMaths::real_t
	calc_globe_pos_discrim(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z) 
	{
		return (y * y + z * z);
	}
	
	static
	bool
	is_on_globe(
			const GPlatesMaths::real_t &discrim) 
	{
		return (discrim <= 1.0);
	}

	static
	const GPlatesMaths::PointOnSphere
	on_globe(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z,
			const GPlatesMaths::real_t &discrim) 
	{
		using namespace GPlatesMaths;

		real_t x = sqrt(1.0 - discrim);

		return PointOnSphere(UnitVector3D(x, y, z));
	}

	static
	const GPlatesMaths::PointOnSphere
	at_intersection_with_globe(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z,
			const GPlatesMaths::real_t &discrim) 
	{
		using namespace GPlatesMaths;

		real_t norm_reciprocal = 1.0 / sqrt(discrim);

		return PointOnSphere(
			UnitVector3D(0.0, y * norm_reciprocal, z * norm_reciprocal));
	}

	static
	const GPlatesMaths::PointOnSphere
	virtual_globe_position(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z) 
	{
		GPlatesMaths::real_t discrim = calc_globe_pos_discrim(y, z);
		
		if (is_on_globe(discrim)) {
			return on_globe(y, z, discrim);
		}
		
		return at_intersection_with_globe(y, z, discrim);
	}
	
}

GPlatesGui::GlobeCanvas::GlobeCanvas(
		QWidget *widget_parent):
	QGLWidget(widget_parent) 
{
	handle_zoom_change();
}

void
GPlatesGui::GlobeCanvas::draw_polyline(
		const GPlatesMaths::PolylineOnSphere &polyline)
{
	GPlatesState::Layout::InsertLineDataPos(NULL, polyline);
}

void
GPlatesGui::GlobeCanvas::draw_point(
		const GPlatesMaths::PointOnSphere &point)
{
	GPlatesState::Layout::InsertPointDataPos(NULL, point);
}

void
GPlatesGui::GlobeCanvas::update_canvas()
{
	updateGL();
}

void
GPlatesGui::GlobeCanvas::clear_data()
{
	GPlatesState::Layout::Clear();
}

void
GPlatesGui::GlobeCanvas::zoom_in() 
{
	unsigned zoom_percent = d_viewport_zoom.zoom_percent();

	d_viewport_zoom.zoom_in();
	
	if (zoom_percent != d_viewport_zoom.zoom_percent()) {
		handle_zoom_change();
	}
}

void
GPlatesGui::GlobeCanvas::zoom_out() 
{
	unsigned zoom_percent = d_viewport_zoom.zoom_percent();

	d_viewport_zoom.zoom_out();
	
	if (zoom_percent != d_viewport_zoom.zoom_percent()) {
		handle_zoom_change();
	}
}

void
GPlatesGui::GlobeCanvas::zoom_reset() 
{
	d_viewport_zoom.reset_zoom();
	handle_zoom_change();
}

void 
GPlatesGui::GlobeCanvas::initializeGL() 
{
	glEnable(GL_DEPTH_TEST);
	
	// FIXME: Enable polygon offset here or in Globe?
	
	clear_canvas();
}

void 
GPlatesGui::GlobeCanvas::resizeGL(
		int new_width,
		int new_height) 
{
	try {
		set_view();
	} catch (const GPlatesGlobal::Exception &e) {
		// FIXME: Use new exception system which doesn't involve strings.
	}
}

void 
GPlatesGui::GlobeCanvas::paintGL() 
{
	try {
		clear_canvas();
		
		glLoadIdentity();
		glTranslatef(EYE_X, EYE_Y, EYE_Z);

		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glRotatef(-90.0, 0.0, 0.0, 1.0);

		// FIXME: Globe uses wrong naming convention for methods.
		d_globe.Paint();

	} catch (const GPlatesGlobal::Exception &e) {
		// FIXME: Use new exception system which doesn't involve strings.
	}
}

void
GPlatesGui::GlobeCanvas::mousePressEvent(
		QMouseEvent *press_event) 
{
	d_mouse_x = press_event->x();
	d_mouse_y = press_event->y();
	
	switch (press_event->button()) {
	case Qt::LeftButton:
		handle_left_mouse_down();
		break;
		
	case Qt::RightButton:
		handle_right_mouse_down();
		break;
		
	default:
		break;
	}
}

void
GPlatesGui::GlobeCanvas::mouseMoveEvent(
		QMouseEvent *move_event) 
{
	d_mouse_x = move_event->x();
	d_mouse_y = move_event->y();
	
	if (move_event->buttons() & Qt::RightButton) {
		handle_right_mouse_drag();
	}
}

void 
GPlatesGui::GlobeCanvas::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	if (release_event->button() == Qt::LeftButton) {
		emit left_mouse_button_clicked();
	}
}

void GPlatesGui::GlobeCanvas::wheelEvent(
		QWheelEvent *wheel_event) 
{
	handle_wheel_rotation(wheel_event->delta());
}

void
GPlatesGui::GlobeCanvas::handle_zoom_change() 
{
	emit current_zoom_changed(d_viewport_zoom.zoom_percent());

	set_view();
	update();
	
	handle_mouse_motion();
}
		
void
GPlatesGui::GlobeCanvas::set_view() 
{
	static const GLdouble depth_near_clipping = 0.5;

	get_dimensions();
	glViewport(0, 0, static_cast<GLsizei>(d_width), static_cast<GLsizei>(d_height));
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLdouble smaller_dim_clipping =
		FRAMING_RATIO / d_viewport_zoom.zoom_factor();

	GLdouble dim_ratio = d_larger_dim / d_smaller_dim;
	GLdouble larger_dim_clipping = smaller_dim_clipping * dim_ratio;

	GLdouble depth_far_clipping = fabsf(EYE_Z);

	if (d_width <= d_height) {
		glOrtho(-smaller_dim_clipping, smaller_dim_clipping,
			-larger_dim_clipping, larger_dim_clipping, depth_near_clipping, 
			depth_far_clipping);

	} else {
		glOrtho(-larger_dim_clipping, larger_dim_clipping,
			-smaller_dim_clipping, smaller_dim_clipping, depth_near_clipping, 
			depth_far_clipping);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void
GPlatesGui::GlobeCanvas::get_dimensions() 
{
	d_width = width();
	d_height = height();

	if (d_width <= d_height) {
		d_smaller_dim = static_cast< GLdouble >(d_width);
		d_larger_dim = static_cast< GLdouble >(d_height);
	} else {
		d_smaller_dim = static_cast< GLdouble >(d_height);
		d_larger_dim = static_cast< GLdouble >(d_width);
	}
}
		
void
GPlatesGui::GlobeCanvas::handle_mouse_motion() 
{
	using namespace GPlatesMaths;

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	real_t discrim = calc_globe_pos_discrim(y_pos, z_pos);
	
	if (is_on_globe(discrim)) {
		PointOnSphere p = on_globe(y_pos, z_pos, discrim);

		// FIXME: Globe uses wrong naming convention for methods.
		PointOnSphere rotated_p = d_globe.Orient(p);

		// FIXME: LatLonPoint uses wrong naming convention for functions.
		LatLonPoint llp =
			LatLonPointConversions::convertPointOnSphereToLatLonPoint(rotated_p);

		emit current_global_pos_changed(llp.latitude().dval(),
			llp.longitude().dval());

	} else {
		emit current_global_pos_off_globe();
	}
}
		
void
GPlatesGui::GlobeCanvas::handle_right_mouse_down() 
{
	using namespace GPlatesMaths;

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);
	
	// FIXME: Globe uses wrong naming convention for methods.
	d_globe.SetNewHandlePos(p);
}
		
void
GPlatesGui::GlobeCanvas::handle_left_mouse_down() 
{
#if 0
	using namespace GPlatesState;
	using namespace GPlatesMaths;

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);

	// FIXME: Globe uses wrong naming convention for methods.
	PointOnSphere rotated_p = d_globe.Orient(p);

	GLdouble diameter_ratio =
		4.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor());
		
	double closeness_inclusion_threshold =
		cos(static_cast< double >(diameter_ratio));

	std::priority_queue< Layout::CloseDatum > sorted_results;
	
	Layout::find_close_data(sorted_results, rotated_p,
		closeness_inclusion_threshold);
		
	if (sorted_results.size() > 0) {
		std::vector< line_header_type > items;
		
		while (!sorted_results.empty()) {
			const Layout::CloseDatum &item = sorted_results.top();
			GPlatesGeo::DrawableData *datum = item.m_datum;

			items.push_back(
				line_header_type(datum->FirstHeaderLine(), datum->SecondHeaderLine()));
				
			sorted_results.pop();
		}

		emit items_selected(items);
	} else {
		emit no_items_selected_by_click();
	}
#endif
}
		
void
GPlatesGui::GlobeCanvas::handle_right_mouse_drag() 
{
	using namespace GPlatesMaths;

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);
	
	// FIXME: Globe uses wrong naming convention for methods.
	d_globe.UpdateHandlePos(p);
	
	updateGL();
}
		
void
GPlatesGui::GlobeCanvas::handle_wheel_rotation(
		int delta) 
{
	const int num_degrees = delta / 8;
	
	int num_steps = num_degrees / 15;

	if (num_steps > 0) {
		while (num_steps--) {
			zoom_in();
		}
	} else {
		while (num_steps++) {
			zoom_out();
		}
	}
}
				
GPlatesMaths::real_t
GPlatesGui::GlobeCanvas::get_universe_coord_y(
		int screen_x) 
{
	GPlatesMaths::real_t y_pos = (2.0 * screen_x - d_width) / d_smaller_dim;

	return (y_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}
				
GPlatesMaths::real_t
GPlatesGui::GlobeCanvas::get_universe_coord_z(
		int screen_y) 
{
	GPlatesMaths::real_t z_pos = (d_height - 2.0 * screen_y) / d_smaller_dim;
	
	return (z_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}
				
void
GPlatesGui::GlobeCanvas::clear_canvas(
		const QColor& c) 
{
	glClearColor(c.red(), c.green(), c.blue(), c.alpha());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
