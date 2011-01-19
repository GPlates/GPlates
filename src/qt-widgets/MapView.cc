/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include <cmath>
#include <iostream>
#include <QGraphicsView>
#include <QtOpenGL/qgl.h>
#include <QPaintEngine>
#include <QScrollBar>

#include "MapView.h"

#include "MapCanvas.h"

#include "gui/MapTransform.h"
#include "gui/ProjectionException.h"
#include "gui/SvgExport.h"
#include "gui/QGLWidgetTextRenderer.h"

#include "maths/InvalidLatLonException.h"

#include "presentation/ViewState.h"


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
}


GPlatesQtWidgets::MapView::MapView(
		GPlatesPresentation::ViewState &view_state,
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
		QWidget *parent_) :
	d_gl_widget_ptr(
			new QGLWidget(
				QGLFormat(QGL::SampleBuffers),
				this)),
	d_map_canvas_ptr(
			new MapCanvas(
				view_state,
				view_state.get_rendered_geometry_collection(),
				this,
				view_state.get_render_settings(),
				view_state.get_viewport_zoom(),
				colour_scheme,
				GPlatesGui::QGLWidgetTextRenderer::create(d_gl_widget_ptr),
				this)),
	d_map_transform(view_state.get_map_transform())
{
#if QT_VERSION >= 0x040600
	// From Qt 4.6, the default paint engine is QPaintEngine::OpenGL2. This
	// causes the map view in GPlates to not function correctly.
	// 
	// This is documented in the Qt 4.6 changelog
	// (http://qt.nokia.com/developer/changes/changes-4.6.0):
	//
	//		The default engine used to draw onto OpenGL buffers has changed in
	//		Qt 4.6. The QPaintEngine::OpenGL2 engine is now used as the default
	//		engine. This *may* cause compatibility problems for applications
	//		that use a mix of QPainter and native OpenGL calls to draw into a GL
	//		buffer. Use the QGL::setPreferredPaintEngine() function to enforce
	//		usage of the old GL paint engine.
	//
	// FIXME: It may be desirable to fix the Map rendering code so that the new
	// OpenGL2 paint engine can be used.
	//
	// Note that the 3D Globe view is not affected by the change to OpenGL2.
	QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
	// setOptimizationFlag(QGraphicsView::IndirectPainting);
#endif

	// QWidget::setMouseTracking:
	//   If mouse tracking is disabled (the default), the widget only receives mouse move
	//   events when at least one mouse button is pressed while the mouse is being moved.
	//
	//   If mouse tracking is enabled, the widget receives mouse move events even if no buttons
	//   are pressed.
	//    -- http://doc.trolltech.com/4.3/qwidget.html#mouseTracking-prop
	d_gl_widget_ptr->setMouseTracking(true);

	setViewport(d_gl_widget_ptr);
	setScene(d_map_canvas_ptr.get());

	setViewportUpdateMode(
		QGraphicsView::MinimalViewportUpdate);
	setInteractive(false);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Get rid of the border.
	setFrameShape(QFrame::NoFrame);

	// Set initial rotation and translation.
	setTransformationAnchor(QGraphicsView::NoAnchor);
	handle_transform_changed(d_map_transform);

	make_signal_slot_connections();
}


GPlatesQtWidgets::MapView::~MapView()
{  }


void
GPlatesQtWidgets::MapView::make_signal_slot_connections()
{
	// Handle map transforms.
	QObject::connect(
			&d_map_transform,
			SIGNAL(transform_changed(const GPlatesGui::MapTransform &)),
			this,
			SLOT(handle_transform_changed(const GPlatesGui::MapTransform &)));

	// Pass up repainted signals from MapCanvas.
	QObject::connect(
			d_map_canvas_ptr.get(),
			SIGNAL(repainted()),
			this,
			SLOT(handle_map_canvas_repainted()));
}


void
GPlatesQtWidgets::MapView::handle_transform_changed(
		const GPlatesGui::MapTransform &map_transform)
{
	static const double FRAMING_RATIO = 1.07;
	double scale_factor = map_transform.get_zoom_factor() * width() /
		(GPlatesGui::MapTransform::MAX_CENTRE_OF_VIEWPORT_X -
		 GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_X) / FRAMING_RATIO;

	QMatrix m;
	m.scale(scale_factor, -scale_factor);
	m.rotate(map_transform.get_rotation());

	// For the translation, we see where the centre of viewport (in scene
	// coordinates) would have ended up (in window coordinates) if we hadn't done
	// any translation. Then we apply a translation such that the centre of
	// viewport will end up in the middle of the screen (in window coordinates).
	// Note that QMatrix::translate() translates along the (already rotated) axes,
	// so we do it manually, by modifying the dx and dy parameters of the matrix.
	const GPlatesGui::MapTransform::point_type &centre = map_transform.get_centre_of_viewport();
	double transformed_centre_x, transformed_centre_y;
	m.map(centre.x(), centre.y(), &transformed_centre_x, &transformed_centre_y);
	double offset_x = static_cast<double>(width()) / 2.0 - transformed_centre_x;
	double offset_y = static_cast<double>(height()) / 2.0 - transformed_centre_y;
	setTransformationAnchor(QGraphicsView::NoAnchor);
	setMatrix(
			QMatrix(m.m11(), m.m12(), m.m21(), m.m22(),
				m.dx() + offset_x,
				m.dy() + offset_y));

	// Even though the scroll bars are hidden, the QGraphicsView is still
	// scrollable, and it has a habit of scrolling around if you have panned to the
	// extremes of the map - which means that if you later recentre on llp (0, 0),
	// that point doesn't appear in the centre of the map anymore. The following
	// cause the QGraphicsView to recentre itself where possible.
	horizontalScrollBar()->setValue(0);
	verticalScrollBar()->setValue(0);

	handle_mouse_pointer_pos_change();
}


void
GPlatesQtWidgets::MapView::handle_map_canvas_repainted()
{
	// If d_mouse_press_info is not boost::none, then mouse is down.
	emit repainted(d_mouse_press_info);
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
					
	emit mouse_pressed(
			d_mouse_press_info->d_mouse_pointer_scene_coords,
			d_mouse_press_info->d_is_on_surface,
			d_mouse_press_info->d_button,
			d_mouse_press_info->d_modifiers);
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

	// Emit repainted signal with mouse_down = false so that those listeners who
	// didn't care about intermediate repaints can now deal with the repaint.
	emit repainted(false);
}


void
GPlatesQtWidgets::MapView::mouseDoubleClickEvent(
		QMouseEvent *mouse_event)
{
	mousePressEvent(mouse_event);
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


	llp = d_map_canvas_ptr->map().projection().inverse_transform(x_mouse_pos,y_mouse_pos);

	if (!llp)
	{
		return llp;
	}
		
	// Forward transform the lat-lon point and see where it would end up. 
	double x_scene_pos = llp->longitude();
	double y_scene_pos = llp->latitude();
	d_map_canvas_ptr->map().projection().forward_transform(x_scene_pos,y_scene_pos);

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


const GPlatesQtWidgets::MapCanvas &
GPlatesQtWidgets::MapView::map_canvas() const
{
	return *d_map_canvas_ptr;
}


GPlatesQtWidgets::MapCanvas &
GPlatesQtWidgets::MapView::map_canvas()
{
	return *d_map_canvas_ptr;
}


void
GPlatesQtWidgets::MapView::set_camera_viewpoint(
	const GPlatesMaths::LatLonPoint &desired_centre)
{
	// Convert the llp to canvas coordinates.
	double x_pos = desired_centre.longitude();
	double y_pos = desired_centre.latitude();

	try{
		d_map_canvas_ptr->map().projection().forward_transform(x_pos,y_pos);
	}
	catch(GPlatesGui::ProjectionException &e)
	{
		std::cerr << "Caught exception converting lat-long to scene coordinates." << std::endl;
		e.write(std::cerr);
	}

	// Centre the view on this point.
	d_map_transform.set_centre_of_viewport(
			GPlatesGui::MapTransform::point_type(x_pos, y_pos));
}


boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::MapView::camera_llp() const
{
	const GPlatesGui::MapTransform::point_type &centre_of_viewport = d_map_transform.get_centre_of_viewport();
	double x_pos = centre_of_viewport.x();
	double y_pos = centre_of_viewport.y();

	// This stores the x screen coordinate, for comparison with the forward-transformed longitude.  
	double screen_x = x_pos;

	// Tolerance for comparing forward transformed longitude with screen longitude. 
	double tolerance = 1.;

	boost::optional<GPlatesMaths::LatLonPoint> llp =
		d_map_canvas_ptr->map().projection().inverse_transform(x_pos,y_pos);
		
	if (!llp)
	{
		return llp;
	}

	// Forward transform the lat-lon point and see where it would end up. 
	double x_scene_pos = llp->longitude();
	double y_scene_pos = llp->latitude();
	d_map_canvas_ptr->map().projection().forward_transform(x_scene_pos,y_scene_pos);

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
	handle_transform_changed(d_map_transform);
}


void
GPlatesQtWidgets::MapView::wheelEvent(
		QWheelEvent *wheel_event)
{
	// This is necessary otherwise the base implementation in QGraphicsView
	// will cause the view to scroll up and down.
	// Zooming is handled by our parent widget, GlobeAndMapWidget.
	wheel_event->ignore();
}


void
GPlatesQtWidgets::MapView::enable_raster_display()
{
	// Do nothing because we can't draw rasters in map view yet.
}


void
GPlatesQtWidgets::MapView::disable_raster_display()
{
	// Do nothing because we can't draw rasters in map view yet.
}


QPointF
GPlatesQtWidgets::MapView::mouse_pointer_scene_coords()
{
	return mapToScene(d_mouse_pointer_screen_pos);
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
GPlatesQtWidgets::MapView::move_camera(
		double dx,
		double dy)
{
	// Position of new centre in window coordinates.
	double win_x = static_cast<double>(width()) / 2.0 + dx;
	double win_y = static_cast<double>(height()) / 2.0 + dy;

	// Turn that into scene coordinates.
	double scene_x, scene_y;
	matrix().inverted().map(win_x, win_y, &scene_x, &scene_y);
	d_map_transform.set_centre_of_viewport(
			GPlatesGui::MapTransform::point_type(scene_x, scene_y));
}


void
GPlatesQtWidgets::MapView::move_camera_up()
{
	// This translation will be zoom-dependent, as it's based on view coordinates. 
	move_camera(0, -5);
}


void
GPlatesQtWidgets::MapView::move_camera_down()
{
	// See comments under "move_camera_up" above. 
	move_camera(0, 5);
}


void
GPlatesQtWidgets::MapView::move_camera_left()
{
	// See comments under "move_camera_up" above. 
	move_camera(-5, 0);
}


void
GPlatesQtWidgets::MapView::move_camera_right()
{
	// See comments under "move_camera_up" above.
	move_camera(5, 0);
}


void
GPlatesQtWidgets::MapView::rotate_camera_clockwise()
{
	d_map_transform.rotate(-5.0);
}


void
GPlatesQtWidgets::MapView::rotate_camera_anticlockwise()
{
	d_map_transform.rotate(5.0);
}


void
GPlatesQtWidgets::MapView::reset_camera_orientation()
{
	d_map_transform.set_rotation(0);
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
		map_canvas().map().projection().inverse_transform(x_, y_);

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


QImage
GPlatesQtWidgets::MapView::grab_frame_buffer()
{
	return d_gl_widget_ptr->grabFrameBuffer();
}


int
GPlatesQtWidgets::MapView::width() const
{
	return d_gl_widget_ptr->width();
}


int
GPlatesQtWidgets::MapView::height() const
{
	return d_gl_widget_ptr->height();
}

