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
#include <iostream>
#include <QtGlobal>
#include <QDebug>

#include "GlobeAndMapCanvas.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "gui/Colour.h"
#include "gui/GlobeCamera.h"
#include "gui/MapCamera.h"
#include "gui/MapProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/MathsUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLViewport.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::GlobeAndMapCanvas::GlobeAndMapCanvas(
		GPlatesPresentation::ViewState &view_state) :
	QOpenGLWindow(),
	d_view_state(view_state),
	d_gl_context(
			GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QOpenGLWindowImpl(*this)))),
	d_initialised_gl(false),
	// The following unit-vector initialisation value is arbitrary.
	d_mouse_position_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_is_on_globe(false),
	d_scene(GPlatesGui::Scene::create(view_state, devicePixelRatio())),
	d_scene_view(GPlatesGui::SceneView::create(view_state)),
	d_scene_overlays(GPlatesGui::SceneOverlays::create(view_state)),
	d_scene_renderer(GPlatesGui::SceneRenderer::create(view_state))
{
	// Update our canvas whenever the RenderedGeometryCollection gets updated.
	// This will cause 'paintGL()' to be called which will visit the rendered
	// geometry collection and redraw it.
	QObject::connect(
			&d_view_state.get_rendered_geometry_collection(),
			SIGNAL(collection_was_updated(
					GPlatesViewOperations::RenderedGeometryCollection &,
					GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
			this,
			SLOT(update_canvas()));

	// Update our canvas whenever the scene view changes.
	//
	// Note that the scene view changes when the globe/map cameras change.
	QObject::connect(
			&*d_scene_view, SIGNAL(view_changed()),
			this, SLOT(update_canvas()));

	//
	// NOTE: This window is not yet initialised (eg, calling width() and height() might return undefined values).
	//       This might be due to it being managed by "QWidget::createWindowContainer()" (see GlobeAndMapWidget).
	// 
}


GPlatesQtWidgets::GlobeAndMapCanvas::~GlobeAndMapCanvas()
{
	// Note that when our data members that contain OpenGL resources (like 'd_scene') are destroyed they don't actually
	// destroy the *native* OpenGL resources. Instead the native resource *wrappers* get destroyed (see GLObjectResource)
	// which just queue the native resources for deallocation with our resource managers (see GLObjectResourceManager).
	// But when the resource managers get destroyed (when our 'd_gl_context' is destroyed) they also don't destroy the
	// native resources. Instead, the native resources (queued for destruction) only get destroyed when 'GLContext::begin_render()'
	// and 'GLContext::end_render()' get called (which only happens when we're actually going to render something).
	//
	// As a result, the native resources only get destroyed when the *native* OpenGL context itself is destroyed
	// (this is taken care of by our base class QOpenGLWindow destructor).
	//
	// Also we connect to the 'QOpenGLContext::aboutToBeDestroyed' signal to destroy our resources before the OpenGL context is destroyed.
}


double
GPlatesQtWidgets::GlobeAndMapCanvas::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_point) const
{
	// The viewport should be in device *independent* coordinates.
	// This way if a user has a high DPI display (like Apple Retina) the higher pixel resolution
	// does not force them to have more accurate mouse clicks.
	const GPlatesOpenGL::GLViewport viewport(0, 0, width(), height());

	return d_scene_view->current_proximity_inclusion_threshold(click_point, viewport);
}


QSize
GPlatesQtWidgets::GlobeAndMapCanvas::get_viewport_size() const
{
	return QSize(width(), height());
}


QImage
GPlatesQtWidgets::GlobeAndMapCanvas::render_to_image(
		const QSize &image_size_in_device_independent_pixels,
		const GPlatesGui::Colour &image_clear_colour)
{
	// Make sure the OpenGL context is current, and then initialise OpenGL if we haven't already.
	//
	// Note: We're not called from 'paintEvent()' so we can't be sure the OpenGL context is current.
	//       And we also don't know if 'initializeGL()' has been called yet.
	makeCurrent();
	if (!d_initialised_gl)
	{
		initialize_gl();
	}

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// The image to render/copy the scene into.
	//
	// Handle high DPI displays (eg, Apple Retina) by rendering image in high-res device pixels.
	// The image will still be it's original size in device *independent* pixels.
	//
	// TODO: We're using the device pixel ratio of current canvas since we're rendering into that and
	// then copying into image. This might not be ideal if this canvas is displayed on one monitor and
	// the QImage (eg, Colouring previews) will be displayed on another with a different device pixel ratio.
	const QSize image_size_in_device_pixels(
			image_size_in_device_independent_pixels.width() * devicePixelRatio(),
			image_size_in_device_independent_pixels.height() * devicePixelRatio());
	QImage image(image_size_in_device_pixels, QImage::Format_ARGB32);
	image.setDevicePixelRatio(devicePixelRatio());

	if (image.isNull())
	{
		// Most likely a memory allocation failure - return the null image.
		return QImage();
	}

	// Fill the image with the clear colour in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(image_clear_colour).rgba());

	// Viewport zoom.
	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();

	// Render the scene into the image.
	d_scene_renderer->render_to_image(
			image, *gl,
			*d_scene, *d_scene_overlays, *d_scene_view,
			viewport_zoom_factor, image_clear_colour);

	return image;
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_canvas()
{
	update();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::initializeGL() 
{
	// Initialise OpenGL.
	//
	// Note: The OpenGL context is already current.
	initialize_gl();
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::initialize_gl()
{
	// Call 'shutdown_gl()' when the QOpenGLContext is about to be destroyed.
	QObject::connect(
			context(), SIGNAL(aboutToBeDestroyed()),
			this, SLOT(shutdown_gl()));

	// Initialise our context-like object first.
	d_gl_context->initialise_gl();

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// NOTE: We should not perform any operation that affects the default framebuffer (such as 'glClear()')
	//       because it's possible the default framebuffer (associated with this GLWidget) is not yet
	//       set up correctly despite its OpenGL context being the current rendering context.

	// Initialise the scene.
	d_scene->initialise_gl(*gl);

	// Initialise the scene renderer.
	d_scene_renderer->initialise_gl(*gl);

	// 'initializeGL()' should only be called once.
	d_initialised_gl = true;
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::shutdown_gl() 
{
	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	{
		GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
		GPlatesOpenGL::GL::RenderScope render_scope(*gl);

		// Shutdown the scene.
		d_scene->shutdown_gl(*gl);

		// Shutdown the scene renderer.
		d_scene_renderer->shutdown_gl(*gl);
	}

	// Shutdown our context-like object last.
	d_gl_context->shutdown_gl();

	d_initialised_gl = false;

	// Disconnect 'shutdown_gl()' now that the QOpenGLContext is about to be destroyed.
	QObject::disconnect(
			context(), SIGNAL(aboutToBeDestroyed()),
			this, SLOT(shutdown_gl()));
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::paintGL()
{
	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// The viewport is in device pixels since that is what OpenGL will use to render the scene.
	const GPlatesOpenGL::GLViewport viewport(0, 0, width(), height());

	// Viewport zoom.
	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	// Clear colour buffer of the main framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This appears to be needed on
	// Mac with Qt5 (alpha=0 is fine on Qt5 Windows/Ubuntu, and on Qt4 for all platforms). Perhaps because
	// QGLWidget rendering (on Qt5 Mac) is first done to a framebuffer object which is then blended into the
	// window framebuffer (where having a source alpha of zero would result in the black background not showing).
	// Or, more likely, maybe a framebuffer object is used on all platforms but the window framebuffer is white on Mac
	// but already black on Windows/Ubuntu. That was using a QOpenGLWidget and we turned off background rendering with
	// "setAutoFillBackground(false)" and "setAttribute(Qt::WA_NoSystemBackground)"). Now we use QOpenGLWindow
	// which does not support those features.
	const GPlatesGui::Colour clear_colour(0, 0, 0, 1);


	// Render the scene into the canvas.
	d_scene_renderer->render(
			*gl, *d_scene, *d_scene_overlays, *d_scene_view,
			viewport, viewport_zoom_factor, clear_colour, devicePixelRatio());
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::paintEvent(
		QPaintEvent *paint_event)
{
	QOpenGLWindow::paintEvent(paint_event);

	// If d_mouse_press_info is not boost::none, then mouse is down.
	Q_EMIT repainted(static_cast<bool>(d_mouse_press_info));
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::mousePressEvent(
		QMouseEvent *press_event) 
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (press_event->button() != Qt::LeftButton)
	{
		return;
	}

	update_mouse_screen_position(press_event);

	d_mouse_press_info =
			MousePressInfo(
					d_mouse_screen_position,
					d_mouse_position_on_map_plane,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					press_event->button(),
					press_event->modifiers());

	if (is_globe_active())
	{
		Q_EMIT mouse_pressed_when_globe_active(
				width(),
				height(),
				d_mouse_press_info->d_mouse_screen_position,
				d_mouse_press_info->d_mouse_position_on_globe,
				d_mouse_press_info->d_mouse_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
	else
	{
		Q_EMIT mouse_pressed_when_map_active(
				width(),
				height(),
				d_mouse_press_info->d_mouse_screen_position,
				d_mouse_press_info->d_mouse_map_position,
				d_mouse_press_info->d_mouse_position_on_globe,
				d_mouse_press_info->d_mouse_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::mouseMoveEvent(
	QMouseEvent *move_event)
{
	update_mouse_screen_position(move_event);

	if (d_mouse_press_info)
	{
		if (is_mouse_in_drag())
		{
			d_mouse_press_info->d_is_mouse_drag = true;
		}

		if (d_mouse_press_info->d_is_mouse_drag)
		{
			if (is_globe_active())
			{
				Q_EMIT mouse_dragged_when_globe_active(
						width(),
						height(),
						d_mouse_press_info->d_mouse_screen_position,
						d_mouse_press_info->d_mouse_position_on_globe,
						d_mouse_press_info->d_mouse_is_on_globe,
						d_mouse_screen_position,
						d_mouse_position_on_globe,
						d_mouse_is_on_globe,
						centre_of_viewport(),
						d_mouse_press_info->d_button,
						d_mouse_press_info->d_modifiers);
			}
			else
			{
				Q_EMIT mouse_dragged_when_map_active(
						width(),
						height(),
						d_mouse_press_info->d_mouse_screen_position,
						d_mouse_press_info->d_mouse_map_position,
						d_mouse_press_info->d_mouse_position_on_globe,
						d_mouse_press_info->d_mouse_is_on_globe,
						d_mouse_screen_position,
						d_mouse_position_on_map_plane,
						d_mouse_position_on_globe,
						d_mouse_is_on_globe,
						centre_of_viewport(),
						d_mouse_press_info->d_button,
						d_mouse_press_info->d_modifiers);
			}
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
		if (is_globe_active())
		{
			Q_EMIT mouse_moved_without_drag_when_globe_active(
					width(),
					height(),
					d_mouse_screen_position,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport());
		}
		else
		{
			Q_EMIT mouse_moved_without_drag_when_map_active(
					width(),
					height(),
					d_mouse_screen_position,
					d_mouse_position_on_map_plane,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport());
		}
	}
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (release_event->button() != Qt::LeftButton)
	{
		return;
	}

	if (!d_mouse_press_info)
	{
		// Somehow we received this left-mouse release event without having first received the
		// corresponding left-mouse press event.
		//
		// Note: With the map view (in older versions of GPlates) a reasonably fast double left mouse click
		//       on the canvas resulted in this (for some reason). However, according to the Qt docs, a
		//       double-click should still produce a mouse press, then release, then a second press
		//       and then a second release.
		return;
	}

	update_mouse_screen_position(release_event);

	if (is_mouse_in_drag())
	{
		d_mouse_press_info->d_is_mouse_drag = true;
	}

	if (d_mouse_press_info->d_is_mouse_drag)
	{
		if (is_globe_active())
		{
			Q_EMIT mouse_released_after_drag_when_globe_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_screen_position,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport(),
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
		else
		{
			Q_EMIT mouse_released_after_drag_when_map_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_map_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_screen_position,
					d_mouse_position_on_map_plane,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport(),
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
	}
	else
	{
		if (is_globe_active())
		{
			Q_EMIT mouse_clicked_when_globe_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
		else
		{
			Q_EMIT mouse_clicked_when_map_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_map_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
	}

	d_mouse_press_info = boost::none;

	// Emit repainted signal with mouse_down = false so that those listeners who
	// didn't care about intermediate repaints can now deal with the repaint.
	Q_EMIT repainted(false);
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::keyPressEvent(
		QKeyEvent *key_event)
{
	// Note that the arrow keys are handled here instead of being set as shortcuts
	// to the corresponding actions in ViewportWindow because when they were set as
	// shortcuts, they were interfering with the arrow keys on other widgets.
	switch (key_event->key())
	{
		case Qt::Key_Up:
			get_active_camera().pan_up();
			break;

		case Qt::Key_Down:
			get_active_camera().pan_down();
			break;

		case Qt::Key_Left:
			get_active_camera().pan_left();
			break;

		case Qt::Key_Right:
			get_active_camera().pan_right();
			break;

		default:
			QOpenGLWindow::keyPressEvent(key_event);
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::wheelEvent(
		QWheelEvent *wheel_event)
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


GPlatesMaths::PointOnSphere
GPlatesQtWidgets::GlobeAndMapCanvas::centre_of_viewport() const
{
	// The point on the globe which corresponds to the centre of the viewport.
	//
	// Note that, for the map view, the map camera look-at position (on map plane) is restricted
	// to be inside the map projection boundary, so this always returns a valid position on the globe.
	return get_active_camera().get_look_at_position_on_globe();
}


bool
GPlatesQtWidgets::GlobeAndMapCanvas::is_mouse_in_drag() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_press_info,
			GPLATES_ASSERTION_SOURCE);

	// Call it a drag if the mouse moved at least 4 pixels in any direction.
	//
	// Otherwise, the user just has shaky hands or a very high-res screen.
	const qreal x_dist = d_mouse_screen_position.x() - d_mouse_press_info->d_mouse_screen_position.x();
	const qreal y_dist = d_mouse_screen_position.y() - d_mouse_press_info->d_mouse_screen_position.y();

	return x_dist*x_dist + y_dist*y_dist > 4;
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_mouse_screen_position(
		QMouseEvent *mouse_event)
{
	d_mouse_screen_position = mouse_event->
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			position()
#else
			localPos()
#endif
			;

	update_mouse_position_on_globe_or_map();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_mouse_position_on_globe_or_map()
{
	// Note that OpenGL and Qt y-axes are the reverse of each other.
	const double mouse_window_y = height() - d_mouse_screen_position.y();
	const double mouse_window_x = d_mouse_screen_position.x();

	// Project screen coordinates into a ray into 3D scene.
	const GPlatesOpenGL::GLIntersect::Ray camera_ray =
			get_active_camera().get_camera_ray_at_window_coord(mouse_window_x, mouse_window_y, width(), height());

	// Determine where/if the camera ray intersects globe.
	//
	// When the map is active (ie, when globe is inactive) the camera ray is considered to intersect
	// the globe if it intersects the map plane at a position that is inside the map projection boundary.
	if (is_globe_active())
	{
		update_mouse_position_on_globe(camera_ray);
	}
	else
	{
		update_mouse_position_on_map(camera_ray);
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_mouse_position_on_globe(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray)
{
	const GPlatesGui::GlobeCamera &globe_camera = d_view_state.get_globe_camera();

	// See if camera ray intersects the globe.
	boost::optional<GPlatesMaths::PointOnSphere> new_position_on_globe =
			globe_camera.get_position_on_globe_at_camera_ray(camera_ray);

	bool is_now_on_globe;
	if (new_position_on_globe)
	{
		// Camera ray, at screen coordinates, intersects the globe.
		is_now_on_globe = true;
	}
	else
	{
		// Camera ray, at screen coordinates, does NOT intersect the globe.
		is_now_on_globe = false;

		// Instead get the nearest point on the globe horizon (visible circumference) to the camera ray.
		new_position_on_globe = globe_camera.get_nearest_globe_horizon_position_at_camera_ray(camera_ray);
	}

	// Update if changed.
	if (new_position_on_globe.get() != d_mouse_position_on_globe ||
		is_now_on_globe != d_mouse_is_on_globe)
	{
		d_mouse_position_on_globe = new_position_on_globe.get();
		d_mouse_is_on_globe = is_now_on_globe;

		Q_EMIT mouse_position_on_globe_changed(d_mouse_position_on_globe, d_mouse_is_on_globe);
	}

	// Position on map plane (z=0) is not used when the globe is active (ie, when map is inactive).
	d_mouse_position_on_map_plane = boost::none;
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_mouse_position_on_map(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray)
{
	const GPlatesGui::MapCamera &map_camera = d_view_state.get_map_camera();
	const GPlatesGui::MapProjection &map_projection = d_view_state.get_map_projection();

	// See if camera ray at screen coordinates intersects the 2D map plane (z=0).
	//
	// In perspective view it's possible for a screen pixel ray emanating from the camera eye to
	// miss the map plane entirely (even though the map plane is infinite).
	//
	// Given the camera ray, calculate a position on the map *plane* (2D plane with z=0), or
	// none if screen view ray (at screen position) does not intersect the map plane.
	d_mouse_position_on_map_plane = map_camera.get_position_on_map_plane_at_camera_ray(camera_ray);

	// Get the position on the globe.
	boost::optional<GPlatesMaths::LatLonPoint> new_lat_lon_position_on_globe;
	bool is_now_on_globe;
	if (d_mouse_position_on_map_plane)
	{
		// Mouse position is on map plane, so see if it's also inside the map projection boundary.
		new_lat_lon_position_on_globe = map_projection.inverse_transform(d_mouse_position_on_map_plane.get());
		if (new_lat_lon_position_on_globe)
		{
			// Mouse position is inside the map projection boundary (so it is also on the globe).
			is_now_on_globe = true;
		}
		else
		{
			// Mouse position is NOT inside the map projection boundary (so it is not on the globe).
			is_now_on_globe = false;

			// Camera ray at screen pixel intersects the map plane but not *within* the map projection boundary.
			//
			// So get the intersection of line segment (from origin to intersection on map plane) with map projection boundary.
			// We'll use that to get a new position on the globe (it can be inverse map projected onto the globe).
			const QPointF map_boundary_point = map_projection.get_map_boundary_position(
					QPointF(0, 0),  // map origin
					d_mouse_position_on_map_plane.get());
			new_lat_lon_position_on_globe = map_projection.inverse_transform(map_boundary_point);

			// The map boundary position is guaranteed to be invertible (onto the globe) in the map projection.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					new_lat_lon_position_on_globe,
					GPLATES_ASSERTION_SOURCE);
		}
	}
	else
	{
		// Mouse position is NOT on the map plane (so it is not on the globe).
		is_now_on_globe = false;

		// Camera ray at screen pixel does not intersect the map plane.
		//
		// So get the intersection of 2D ray, from map origin in direction of camera ray (projected onto 2D map plane),
		// with map projection boundary.
		const QPointF ray_direction(
				camera_ray.get_direction().x().dval(),
				camera_ray.get_direction().y().dval());
		const QPointF ray_origin(0, 0);  // map origin

		const boost::optional<QPointF> map_boundary_point =
				map_camera.get_position_on_map_boundary_intersected_by_2d_camera_ray(ray_direction, ray_origin);
		if (map_boundary_point)
		{
			new_lat_lon_position_on_globe = map_projection.inverse_transform(map_boundary_point.get());

			// The map boundary position is guaranteed to be invertible (onto the globe) in the map projection.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					new_lat_lon_position_on_globe,
					GPLATES_ASSERTION_SOURCE);
		}
		else
		{
			// The 3D camera ray direction points straight down (ie, camera ray x and y are zero).
			//
			// We shouldn't really get here for a valid camera ray since we already know it did not intersect the
			// 2D map plane and so if it points straight down then it would have intersected the map plane (z=0).
			// However it's possible that at 90 degree tilt the camera eye (in perspective viewing) dips just below
			// the map plane (z=0) due to numerical tolerance and hence just misses the map plane.
			// But even then the camera view direction would be horizontal and with a field-of-view of 90 degrees or less
			// there wouldn't be any screen pixel in the view frustum that could look straight down.
			// So it really should never happen.
			//
			// Arbitrarily choose the North pole (again, we shouldn't get here).
			new_lat_lon_position_on_globe = GPlatesMaths::LatLonPoint(90, 0);
		}
	}

	// Convert inverse-map-projected lat-lon position to new position on the globe.
	const GPlatesMaths::PointOnSphere new_position_on_globe = make_point_on_sphere(new_lat_lon_position_on_globe.get());

	// Update if changed.
	if (new_position_on_globe != d_mouse_position_on_globe ||
		is_now_on_globe != d_mouse_is_on_globe)
	{
		d_mouse_position_on_globe = new_position_on_globe;
		d_mouse_is_on_globe = is_now_on_globe;

		Q_EMIT mouse_position_on_globe_changed(d_mouse_position_on_globe, d_mouse_is_on_globe);
	}
}
