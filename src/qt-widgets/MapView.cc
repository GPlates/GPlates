/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway.
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

#include <cmath>
#include <QDebug>
#include <QGraphicsView>

#include "gui/ProjectionException.h"
#include "gui/SvgExport.h"
#include "maths/InvalidLatLonException.h"
#include "MapCanvas.h"
#include "ViewportWindow.h"

#include "MapView.h"

namespace
{

	double
	distance_between_qpointfs(
		const QPointF &p1,
		const QPointF &p2)
	{
		QPointF difference = p1 - p2;

		return sqrt(difference.x()*difference.x() + difference.y()*difference.y());
	}

	QPointF
	get_scene_translation_from_view_translation(
		QGraphicsView *view,
		const int &x,
		const int &y)
	{
		QPointF zero = view->mapToScene(0,0);
		QPointF translation = view->mapToScene(x,y);

		return (translation - zero);
	}

}


GPlatesQtWidgets::MapView::MapView(
	GPlatesQtWidgets::ViewportWindow &view_state,
	QWidget *parent_,
	GPlatesGui::ViewportZoom *viewport_zoom_,
	GPlatesQtWidgets::MapCanvas *map_canvas_):
	d_viewport_zoom(viewport_zoom_),
	d_map_canvas_ptr(map_canvas_),
	d_centre_of_viewport(0.,0.),
	d_scene_rect(-180,-90,360,180)
{

	setViewport(new QGLWidget(
		QGLFormat(QGL::SampleBuffers)));

	setViewportUpdateMode(
		QGraphicsView::MinimalViewportUpdate);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Flip the vertical axis so that positive latitude is upwards. 
	scale(1.,-1.);

	fitInView(d_scene_rect,Qt::KeepAspectRatio);

	setInteractive(false);
}

void
GPlatesQtWidgets::MapView::handle_zoom_change()
{
	// Record the camera position so we can reset it after the zoom. 
	boost::optional<GPlatesMaths::LatLonPoint> llp	= camera_llp();

	set_view();
	
	if (llp)
	{
		set_camera_viewpoint(*llp);
	}

	update();
	handle_mouse_pointer_pos_change();

	update_centre_of_viewport();
	emit view_changed();
	
}

void
GPlatesQtWidgets::MapView::set_view()
{
	// This resets the scale, but also resets any x-y offsets. 
	fitInView(d_scene_rect,Qt::KeepAspectRatio);	

	// Get the zoom level from the GlobeCanvas' ViewportZoom.
	double zoom = d_viewport_zoom->zoom_percent()/100.;

	scale(zoom,zoom);

	// Reinstate the original offset.
	centerOn(d_centre_of_viewport);
}

void
GPlatesQtWidgets::MapView::update_mouse_pointer_pos(
		QMouseEvent *mouse_event)
{
	d_mouse_pointer_screen_pos = mouse_event->pos();

	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::handle_mouse_pointer_pos_change()
{
	boost::optional<GPlatesMaths::LatLonPoint> llp = mouse_pointer_llp();

	d_mouse_pointer_is_on_surface = llp;

	emit mouse_pointer_position_changed(llp, d_mouse_pointer_is_on_surface);
}


void
GPlatesQtWidgets::MapView::mousePressEvent(
		QMouseEvent *press_event) 
{

	update_mouse_pointer_pos(press_event);

	// Let's ignore all mouse buttons except the left mouse button.
	if (press_event->button() != Qt::LeftButton) {
		return;
	}

	d_last_mouse_view_coords = press_event->pos();

	d_mouse_press_info =
			MousePressInfo(
					press_event->x(),
					press_event->y(),
					mouse_pointer_scene_coords(),
					mouse_pointer_llp(),
					mouse_pointer_is_on_surface(),
					press_event->button(),
					press_event->modifiers());

}



void 
GPlatesQtWidgets::MapView::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (release_event->button() != Qt::LeftButton) 
	{
		return;
	}

	// Let's do our best to avoid crash-inducing Boost assertions.
	if ( ! d_mouse_press_info) {
		// OK, something strange happened:  Our boost::optional MousePressInfo is not
		// initialised.  Rather than spontaneously crashing with a Boost assertion error,
		// let's log a warning on the console and NOT crash.
		
		// A reasonably fast double left mouse click on the map is resulting (for some 
		// reason) in an uninitialised mouse_press_info structure, so the following message 
		// gets output quite easily. So I'll silence it for now. 
#if 0	
		std::cerr << "Warning (MapCanvas::mouseReleaseEvent, "
				<< __FILE__
				<< " line "
				<< __LINE__
				<< "):\nUninitialised mouse press info!"
				<< std::endl;
#endif				
		return;
	}

	if (abs(release_event->x() - d_mouse_press_info->d_mouse_pointer_screen_pos_x) > 3 &&
			abs(release_event->y() - d_mouse_press_info->d_mouse_pointer_screen_pos_y) > 3) {
		d_mouse_press_info->d_is_mouse_drag = true;
	}
	if ((d_mouse_press_info->d_is_mouse_drag))
	{

		emit mouse_released_after_drag(
				d_mouse_press_info->d_mouse_pointer_scene_coords,
				d_mouse_press_info->d_is_on_surface,
				mouse_pointer_scene_coords(),
				mouse_pointer_is_on_surface(),
				QPointF(),
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);

	} else {
		emit mouse_clicked(
				d_mouse_press_info->d_mouse_pointer_scene_coords,
				d_mouse_press_info->d_is_on_surface,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
	d_mouse_press_info = boost::none;
}



void
GPlatesQtWidgets::MapView::mouseMoveEvent(
	QMouseEvent *move_event)
{

	QPointF translation = mapToScene(move_event->pos()) - 
		mapToScene(d_last_mouse_view_coords);

	d_last_mouse_view_coords = move_event->pos();

	if (d_mouse_press_info)
	{
		int x_dist = move_event->x() - d_mouse_press_info->d_mouse_pointer_screen_pos_x;
		int y_dist = move_event->y() - d_mouse_press_info->d_mouse_pointer_screen_pos_y;
		if (x_dist*x_dist + y_dist*y_dist > 4)
		{
			d_mouse_press_info->d_is_mouse_drag = true;
		}

		if (d_mouse_press_info->d_is_mouse_drag)
		{

			emit mouse_dragged(
				d_mouse_press_info->d_mouse_pointer_scene_coords,
				d_mouse_press_info->d_is_on_surface,
				mouse_pointer_scene_coords(),
				mouse_pointer_is_on_surface(),
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers,
				translation);

			update_centre_of_viewport();

			//	Signal the ReconstructionViewWidget to update the camera llp display.
			emit view_changed();
		}

	}
	else
	{
		//
		// The mouse has moved but the left mouse button is not currently pressed.
		// This could mean no mouse buttons are currently pressed or it could mean a
		// button other than the left mouse button is currently pressed.
		// Either way it is an mouse movement that is not currently invoking a
		// canvas tool operation.
		//
		emit mouse_moved_without_drag(
			mouse_pointer_scene_coords(),
			mouse_pointer_is_on_surface(),
			translation);
	}


	update_mouse_pointer_pos(move_event);

}

void
GPlatesQtWidgets::MapView::wheelEvent(
	  QWheelEvent *wheel_event)
{
	handle_wheel_rotation(wheel_event->delta());	
}

void
GPlatesQtWidgets::MapView::handle_wheel_rotation(
		int delta) 
{
	// These integer values (8 and 15) are copied from the Qt reference docs for QWheelEvent:
	//  http://doc.trolltech.com/4.3/qwheelevent.html#delta
	const int num_degrees = delta / 8;
	int num_steps = num_degrees / 15;

	if (num_steps > 0) {
		while (num_steps--) {
			d_viewport_zoom->zoom_in();
		}
	} else {
		while (num_steps++) {
			d_viewport_zoom->zoom_out();
		}
	}
}

boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::MapView::mouse_pointer_llp()
{

	QPointF canvas_pos = mapToScene(d_mouse_pointer_screen_pos);

	double x_mouse_pos = canvas_pos.x();
	double y_mouse_pos = canvas_pos.y();

	// The proj library returns valid longitudes even when the screen coordinates are 
	// far to the right, or left, of the map itself. To determine if the mouse position is off
	// the map, I'm transforming the returned lat-lon back into screen coordinates. 
	// If this doesn't match our original screen coordinates, then we can assume that we're off the map.
	// I'm going to use the longitude value for comparison. 

	// This stores the x screen coordinate, for comparison with the forward-transformed longitude.  
	double screen_x = x_mouse_pos;

	// I haven't put any great deal of thought into a suitable tolerance here. 
	double tolerance = 1.;

	boost::optional<GPlatesMaths::LatLonPoint> llp;


	llp = d_map_canvas_ptr->projection().inverse_transform(x_mouse_pos,y_mouse_pos);

	if (!llp)
	{
		return llp;
	}
		
	// Forward transform the lat-lon point and see where it would end up. 
	double x_scene_pos = llp->longitude();
	double y_scene_pos = llp->latitude();
	d_map_canvas_ptr->projection().forward_transform(x_scene_pos,y_scene_pos);

	// If we don't end up at the same point, we're off the map. 

	if (std::fabs(x_scene_pos - screen_x) > tolerance)
	{
		return boost::none;
	}

	// If we reach here, we should be on the map, with valid lat,lon.
	return llp;
	
}


void
GPlatesQtWidgets::MapView::create_svg_output(
	QString filename)
{
	GPlatesGui::SvgExport::create_svg_output(filename,this);
}

void
GPlatesQtWidgets::MapView::draw_svg_output()
{
	map_canvas().draw_svg_output();
}

GPlatesQtWidgets::MapCanvas &
GPlatesQtWidgets::MapView::map_canvas() const
{
	return *d_map_canvas_ptr;
}

void
GPlatesQtWidgets::MapView::update_centre_of_viewport()
{
	QPoint view_centre = viewport()->rect().center();
	d_centre_of_viewport = mapToScene(view_centre);
}

void
GPlatesQtWidgets::MapView::set_camera_viewpoint(
	const GPlatesMaths::LatLonPoint &desired_centre)
{
// Convert the llp to canvas coordinates.
	double x_pos = desired_centre.longitude();
	double y_pos = desired_centre.latitude();

	try{
		d_map_canvas_ptr->projection().forward_transform(x_pos,y_pos);
	}
	catch(GPlatesGui::ProjectionException &e)
	{
		std::cerr << "Caught exception converting lat-long to scene coordinates." << std::endl;
		e.write(std::cerr);
	}

	// Centre the view on this point.
	centerOn(x_pos,y_pos);
	d_centre_of_viewport = QPointF(x_pos,y_pos);

	// Tell the ReconstructionView that the camera llp has changed. 
	emit view_changed();
}

boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::MapView::camera_llp()
{
	double x_pos = d_centre_of_viewport.x();
	double y_pos = d_centre_of_viewport.y();

	// This stores the x screen coordinate, for comparison with the forward-transformed longitude.  
	double screen_x = x_pos;

	// Tolerance for comparing forward transformed longitude with screen longitude. 
	double tolerance = 1.;


	
	boost::optional<GPlatesMaths::LatLonPoint> llp =
		d_map_canvas_ptr->projection().inverse_transform(x_pos,y_pos);
		
	if (!llp)
	{
		return llp;
	}

	// Forward transform the lat-lon point and see where it would end up. 
	double x_scene_pos = llp->longitude();
	double y_scene_pos = llp->latitude();
	d_map_canvas_ptr->projection().forward_transform(x_scene_pos,y_scene_pos);

	// If we don't end up at the same point, we're off the map. 
	if (std::fabs(x_scene_pos - screen_x) > tolerance)
	{
		return boost::none;
	}		
		
	return llp;

}

void
GPlatesQtWidgets::MapView::resizeEvent(
	QResizeEvent *resize_event)
{
	set_view();
}

void
GPlatesQtWidgets::MapView::enable_raster_display()
{

}

void
GPlatesQtWidgets::MapView::disable_raster_display()
{

}

QPointF
GPlatesQtWidgets::MapView::mouse_pointer_scene_coords()
{
	return 	mapToScene(d_mouse_pointer_screen_pos);
}

bool
GPlatesQtWidgets::MapView::mouse_pointer_is_on_surface()
{
	return mouse_pointer_llp();
}


void
GPlatesQtWidgets::MapView::update_canvas()
{
	map_canvas().update();
}


void
GPlatesQtWidgets::MapView::move_camera_up()
{
	setTransformationAnchor(QGraphicsView::NoAnchor);

	// This translation will be zoom-dependent, as it's based on view coordinates. 
	// This is slightly different from the globe behaviour, which is always a 5 degree increment, 
	// irrespective of zoom level.

	QPointF translation = get_scene_translation_from_view_translation(this,0,5);
	translate(translation.x(),translation.y());

	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::move_camera_down()
{
	// See comments under "move_camera_up" above. 
	setTransformationAnchor(QGraphicsView::NoAnchor);

	QPointF translation = get_scene_translation_from_view_translation(this,0,-5);
	translate(translation.x(),translation.y());
	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::move_camera_left()
{
	// See comments under "move_camera_up" above. 
	setTransformationAnchor(QGraphicsView::NoAnchor);

	QPointF translation = get_scene_translation_from_view_translation(this,5,0);
	translate(translation.x(),translation.y());
	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::move_camera_right()
{
	// See comments under "move_camera_up" above. 
	setTransformationAnchor(QGraphicsView::NoAnchor);

	QPointF translation = get_scene_translation_from_view_translation(this,-5,0);
	translate(translation.x(),translation.y());
	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::rotate_camera_clockwise()
{
	setTransformationAnchor(QGraphicsView::AnchorViewCenter);

	rotate(-5.);
	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::rotate_camera_anticlockwise()
{
	setTransformationAnchor(QGraphicsView::AnchorViewCenter);

	rotate(5.);
	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

void
GPlatesQtWidgets::MapView::reset_camera_orientation()
{
	// Set the identity matrix as the transform.
	setTransform(QTransform());

	scale(1.,-1.);

	// This will reset the scale to the current zoom value. 
	set_view();

	
	update_centre_of_viewport();
	emit view_changed();
	handle_mouse_pointer_pos_change();
}

double
GPlatesQtWidgets::MapView::current_proximity_inclusion_threshold(
	const GPlatesMaths::PointOnSphere &click_point) const
{


	// See the corresponding GlobeCanvas::current_proximity_inclusion_threshold function for a 
	// justification, and explanation of calculation, of the proximity inclusion threshold. 
	//
	// On the map, the calculation is slightly different to that on the globe. 
	//
	// The ClickGeometry code, which will use the output of this function, requires a 
	// "dot-product-related closeness inclusion threshold".
	// 
	// To evaluate this on the map:
	// 1. Convert the click-point to llp, and to point-on-sphere.
	// 2. Move 3 screen pixels towards the centre of the canvas. 
	// 3. Convert this location to llp and point-on-sphere.
	// 4. Calculate the cosine of the angle between the 2 point-on-spheres.

	QPoint temp_screen_mouse_position = d_mouse_pointer_screen_pos + QPoint(3,0);
	QPointF temp_scene_mouse_position = mapToScene(temp_screen_mouse_position);

	QPointF scene_mouse_position = mapToScene(d_mouse_pointer_screen_pos);

	double scene_proximity_distance = distance_between_qpointfs(scene_mouse_position,temp_scene_mouse_position);

	double angle = atan2(scene_mouse_position.y(),scene_mouse_position.x());
	double x_proximity = scene_proximity_distance * cos(angle);
	double y_proximity = scene_proximity_distance * sin(angle);

	QPointF threshold_point;
	if (scene_mouse_position.x() > 0)
	{
		threshold_point.setX(scene_mouse_position.x() - x_proximity);
	}
	else
	{	
		threshold_point.setX(scene_mouse_position.x() + x_proximity);
	}
#if 1
	if (scene_mouse_position.y() > 0)
	{
		threshold_point.setY(scene_mouse_position.y() - y_proximity);
	}
	else
	{	
		threshold_point.setY(scene_mouse_position.y() + y_proximity);
	}
#else
	threshold_point.setY(scene_mouse_position.y());
#endif
	double x_ = threshold_point.x();
	double y_ = threshold_point.y();

	boost::optional<GPlatesMaths::LatLonPoint> llp = 
		map_canvas().projection().inverse_transform(x_, y_);

	if (!llp)
	{
		return 0.;
	}
	
	GPlatesMaths::PointOnSphere proximity_pos = GPlatesMaths::make_point_on_sphere(*llp);

	double proximity_inclusion_threshold = GPlatesMaths::dot(
			click_point.position_vector(),
			proximity_pos.position_vector()).dval();

#if 0
	double origin_to_scene_distance = distance_between_qpointfs(scene_mouse_position,QPointF());
	qDebug();
	qDebug() << "temp screen: " << temp_screen_mouse_position;
	qDebug() << "temp scene: " << temp_scene_mouse_position;
	qDebug() << "scene: " << scene_mouse_position;
	qDebug() << "scene prox distance: " << scene_proximity_distance;
	qDebug() << "origin to scene: " << origin_to_scene_distance;
	qDebug() << "Threshold point: " << threshold_point;
	qDebug() << "result: " << proximity_inclusion_threshold;
	qDebug(); 
#endif

	return proximity_inclusion_threshold;
}

