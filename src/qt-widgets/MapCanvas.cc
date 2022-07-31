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
#include <QApplication>
#include <QDebug>
#include <QLineF>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>

#include "GlobeCanvas.h"
#include "MapCanvas.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "gui/Map.h"
#include "gui/MapCamera.h"
#include "gui/MapProjection.h"
#include "gui/ProjectionException.h"
#include "gui/TextOverlay.h"
#include "gui/VelocityLegendOverlay.h"
#include "gui/ViewportZoom.h"

#include "maths/MathsUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLViewport.h"
#include "opengl/OpenGLException.h"

#include "presentation/ViewState.h"

#include "utils/Profile.h"


GPlatesQtWidgets::MapCanvas::MapCanvas(
		GPlatesPresentation::ViewState &view_state,
		GlobeCanvas &globe_canvas,
		QWidget *parent_) :
	QOpenGLWidget(parent_),
	d_view_state(view_state),
	d_gl_context(
			GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QOpenGLWidgetImpl(*this)),
					// We're sharing the OpenGL context with GlobeCanvas since both
					// MapCanvas and GlobeCanvas were created with the same QSurfaceFormat...
					*globe_canvas.get_gl_context())),
	d_initialisedGL(false),
	d_view_projection(
			GPlatesOpenGL::GLViewport(0, 0, d_gl_context->get_width(), d_gl_context->get_height()),
			// Use identity transforms for now, these will get updated when the camera changes...
			GPlatesOpenGL::GLMatrix::IDENTITY,
			GPlatesOpenGL::GLMatrix::IDENTITY),
	d_off_screen_render_target_dimension(OFF_SCREEN_RENDER_TARGET_DIMENSION),
	d_gl_visual_layers(
			// Attempt to share OpenGL resources across contexts.
			// This will depend on whether the two 'GLContext's share any state.
			GPlatesOpenGL::GLVisualLayers::create(
					d_gl_context,
					globe_canvas.get_gl_visual_layers(),
					view_state.get_application_state())),
	// The following unit-vector initialisation value is arbitrary.
	d_mouse_position_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_is_on_globe(false),
	d_map(
			view_state,
			d_gl_visual_layers,
			view_state.get_rendered_geometry_collection(),
			view_state.get_visual_layers(),
			devicePixelRatio()),
	d_map_camera(view_state.get_map_camera()),
	d_text_overlay(new GPlatesGui::TextOverlay(view_state.get_application_state())),
	d_velocity_legend_overlay(new GPlatesGui::VelocityLegendOverlay())
{
	// Don't fill the background - we already clear the background using OpenGL in 'render_scene()' anyway.
	//
	// NOTE: Also there's a problem where QPainter (used in 'paintGL()') uses the background role
	// of the canvas widget to fill the background using glClearColor/glClear - but the clear colour
	// does not get reset to black (default OpenGL state) in 'QPainter::beginNativePainting()' which
	// GL requires (the default OpenGL state) and hence it assumes the clear colour is black
	// when it is not - and hence the background (behind the globe) is *not* black.
	setAutoFillBackground(false);

	// QWidget::setMouseTracking:
	//   If mouse tracking is disabled (the default), the widget only receives mouse move
	//   events when at least one mouse button is pressed while the mouse is being moved.
	//
	//   If mouse tracking is enabled, the widget receives mouse move events even if no buttons
	//   are pressed.
	//    -- http://doc.trolltech.com/4.3/qwidget.html#mouseTracking-prop
	setMouseTracking(true);
	
	// Ensure the map will always expand to fill available space.
	// A minumum size and non-collapsibility is set on the map basically so users
	// can't obliterate it and then wonder (read:complain) where their map went.
	QSizePolicy map_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	map_size_policy.setHorizontalStretch(255);
	setSizePolicy(map_size_policy);
	setFocusPolicy(Qt::StrongFocus);
	setMinimumSize(100, 100);

	// Update our canvas whenever the RenderedGeometryCollection gets updated.
	// This will cause 'paintGL()' to be called which will visit the rendered
	// geometry collection and redraw it.
	QObject::connect(
			&(d_view_state.get_rendered_geometry_collection()),
			SIGNAL(collection_was_updated(
					GPlatesViewOperations::RenderedGeometryCollection &,
					GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
			this,
			SLOT(update_canvas()));

	// Update our view whenever the camera changes.
	//
	// Note that the camera is updated when the zoom changes.
	QObject::connect(
			&d_map_camera, SIGNAL(camera_changed()),
			this, SLOT(handle_camera_change()));

	handle_camera_change();

	setAttribute(Qt::WA_NoSystemBackground);
}


GPlatesQtWidgets::MapCanvas::~MapCanvas()
{
	// Note that when our data members that contain OpenGL resources (like 'd_map') are destroyed they don't actually
	// destroy the *native* OpenGL resources. Instead the native resource *wrappers* get destroyed (see GLObjectResource)
	// which just queue the native resources for deallocation with our resource managers (see GLObjectResourceManager).
	// But when the resource managers get destroyed (when our 'd_gl_context' is destroyed) they also don't destroy the
	// native resources. Instead, the native resources (queued for destruction) only get destroyed when 'GLContext::begin_render()'
	// and 'GLContext::end_render()' get called (which only happens when we're actually going to render something).
	//
	// As a result, the native resources only get destroyed when the *native* OpenGL context itself is destroyed
	// (this is taken care of by our base class QOpenGLWidget destructor). And any resources shared between contexts
	// (eg, textures) are destroyed when all contexts (from GlobeCanvas and MapCanvas) are destroyed (and possibly only
	// after the top-level window containing those QOpenGLWidget's is also destroyed).
	//
	// Also note we could connect to the 'QOpenGLContext::aboutToBeDestroyed' signal, but that also is unnecessary
	// for us since we never re-parent GlobeCanvas/MapCanvas to a different top-level window and hence are not required
	// to destroy our resources before rebuilding them again in 'initializeGL()'. The resources only need to be destroyed
	// once (when GPlates shuts down).
}


double
GPlatesQtWidgets::MapCanvas::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_point) const
{
	// Say we pick an epsilon radius of 3 pixels around the click position.
	// The larger this radius, the more relaxed the proximity inclusion threshold.
	//
	// FIXME:  Do we want this constant to instead be a variable set by a per-user preference,
	// to enable users to specify their own epsilon radius?  (For example, users with shaky
	// hands or very high-resolution displays might prefer a larger epsilon radius.)
	//
	// Note: We're specifying device *independent* pixels here.
	//       On high-DPI displays there are more device pixels in the same physical area on screen but we're
	//       more interested in physical area (which is better represented by device *independent* pixels).
	const double device_independent_pixel_inclusion_threshold = 3.0;
	// Limit the maximum angular distance on unit sphere. When the map view is tilted the click point
	// can intersect it at an acute angle such that one viewport pixel can cover a large area on the map.
	// Additionally the map projection itself (eg, Rectangular, Mollweide, etc) can further stretch the
	// viewport pixel (already projected onto map plane z=0) when it's inverse transformed back onto the globe.
	const double max_distance_inclusion_threshold = GPlatesMaths::convert_deg_to_rad(5.0);

	// The map view can be tilted such that the click point can intersect the map plane (z=0) at varying angles.
	// This means one viewport pixel can cover varying areas on the map plane (z=0).
	// Additionally the map projection itself (eg, Rectangular, Mollweide, etc) can further stretch or compress
	// the viewport pixel (already projected onto map plane z=0) when it's inverse transformed back onto the globe.
	//
	// As such, a small mouse-pointer displacement on-screen can result in significantly different mouse-pointer
	// displacements on the surface of the globe depending on the location of the click point.
	//
	// To take this into account we use the current view and projection transforms (and viewport) to project one
	// screen pixel area onto the map plane (z=0) and then inverse transform from the map plane onto the globe
	// (using the map projection, eg, Rectangular, Mollweide, etc).
	// This finds the maximum deviation of this area projected onto the globe (in terms of angular distance on the globe).
	//
	// Calculate the maximum distance on the unit-sphere subtended by one viewport pixel projected onto it.
	GPlatesOpenGL::GLViewProjection gl_view_projection(
			// Note: We don't multiply dimensions by device-pixel-ratio since we want our max pixel size to be
			// in device *independent* coordinates. This way if a user has a high DPI display (like Apple Retina)
			// the higher pixel resolution does not force them to have more accurate mouse clicks...
			GPlatesOpenGL::GLViewport(0, 0, width(), height()),
			d_view_projection.get_view_transform(),
			// Also note that this projection transform is 'orthographic' or 'perspective', and hence is
			// only affected by viewport *aspect ratio*, so it is independent of whether we're using
			// device pixels or device *independent* pixels...
			d_view_projection.get_projection_transform());
	boost::optional< std::pair<double/*min*/, double/*max*/> > min_max_device_independent_pixel_size =
			gl_view_projection.get_min_max_pixel_size_on_globe(click_point, d_view_state.get_map_projection());
	// If unable to determine maximum pixel size then just return the maximum allowed proximity threshold.
	if (!min_max_device_independent_pixel_size)
	{
		return std::cos(max_distance_inclusion_threshold);  // Proximity threshold is expected to be a cosine.
	}

	// Multiply the inclusive distance on unit-sphere (associated with one viewport pixel) by the
	// number of inclusive viewport pixels.
	double distance_inclusion_threshold = device_independent_pixel_inclusion_threshold *
			min_max_device_independent_pixel_size->second/*max*/;

	// Clamp to range to the maximum distance inclusion threshold (if necessary).
	if (distance_inclusion_threshold > max_distance_inclusion_threshold)
	{
		distance_inclusion_threshold = max_distance_inclusion_threshold;
	}

	// Proximity threshold is expected to be a cosine.
	return std::cos(distance_inclusion_threshold);
}


QSize
GPlatesQtWidgets::MapCanvas::get_viewport_size() const
{
	return QSize(width(), height());
}


QImage
GPlatesQtWidgets::MapCanvas::render_to_qimage(
		const QSize &image_size_in_device_independent_pixels)
{
	// Initialise OpenGL if we haven't already.
	initializeGL_if_necessary();

	// Make sure the OpenGL context is currently active.
	d_gl_context->make_current();

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

	// Fill the image with transparent black in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(0,0,0,0).rgba());

	const GPlatesOpenGL::GLViewport image_viewport(
			0, 0,
			// Use image size in device pixels (used by OpenGL)...
			image_size_in_device_pixels.width(),
			image_size_in_device_pixels.height()/*destination_viewport*/);
	const double image_aspect_ratio = double(image_size_in_device_independent_pixels.width()) /
			image_size_in_device_independent_pixels.height();

	// Get the view-projection transform for the image.
	const GPlatesOpenGL::GLMatrix image_view_transform = d_map_camera.get_view_transform();
	const GPlatesOpenGL::GLMatrix image_projection_transform =
			d_map_camera.get_projection_transform(image_aspect_ratio);

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int image_tile_border = 10;
	// Set up for rendering the scene into tiles using the offscreen render target.
	GPlatesOpenGL::GLTileRender image_tile_render(
			d_off_screen_render_target_dimension/*tile_render_target_width*/,
			d_off_screen_render_target_dimension/*tile_render_target_height*/,
			image_viewport/*destination_viewport*/,
			image_tile_border);

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (image_tile_render.first_tile(); !image_tile_render.finished(); image_tile_render.next_tile())
	{
		// Render the scene to current image tile.
		// Hold onto the previous frame's cached resources *while* generating the current frame.
		const cache_handle_type image_tile_cache_handle = render_scene_tile_into_image(
				*gl,
				image_view_transform,
				image_projection_transform,
				image_tile_render,
				image);
		frame_cache_handle->push_back(image_tile_cache_handle);
	}

	// The previous cached resources were kept alive *while* in the rendering loop above.
	d_gl_frame_cache_handle = frame_cache_handle;

	return image;
}


void
GPlatesQtWidgets::MapCanvas::render_opengl_feedback_to_paint_device(
		QPaintDevice &feedback_paint_device)
{
	// Initialise OpenGL if we haven't already.
	initializeGL_if_necessary();

	// Make sure the OpenGL context is currently active.
	d_gl_context->make_current();

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// Convert from paint device size to device pixels (used by OpenGL)...
	const unsigned int feedback_paint_device_pixel_width =
			feedback_paint_device.width() * feedback_paint_device.devicePixelRatio();
	const unsigned int feedback_paint_device_pixel_height =
			feedback_paint_device.height() * feedback_paint_device.devicePixelRatio();
	const double feedback_paint_device_aspect_ratio =
			double(feedback_paint_device_pixel_width) / feedback_paint_device_pixel_height;

	const GPlatesOpenGL::GLViewport feedback_paint_device_viewport(
			0, 0,
			feedback_paint_device_pixel_width,
			feedback_paint_device_pixel_height);

	// Get the view-projection transform.
	const GPlatesOpenGL::GLMatrix feedback_paint_device_view_transform = d_map_camera.get_view_transform();
	const GPlatesOpenGL::GLMatrix feedback_paint_device_projection_transform =
			d_map_camera.get_projection_transform(feedback_paint_device_aspect_ratio);

	const GPlatesOpenGL::GLViewProjection feedback_paint_device_view_projection(
			feedback_paint_device_viewport,
			feedback_paint_device_view_transform,
			feedback_paint_device_projection_transform);

	// Set the viewport (and scissor rectangle) to the size of the feedback paint device
	// (instead of the map canvas) since we're rendering to it (via transform feedback).
	gl->Viewport(
			feedback_paint_device_viewport.x(), feedback_paint_device_viewport.y(),
			feedback_paint_device_viewport.width(), feedback_paint_device_viewport.height());
	gl->Scissor(
			feedback_paint_device_viewport.x(), feedback_paint_device_viewport.y(),
			feedback_paint_device_viewport.width(), feedback_paint_device_viewport.height());

	// Render the scene to the feedback paint device.
	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*gl,
			feedback_paint_device_view_projection,
			// Using device-independent pixels (eg, widget dimensions)...
			feedback_paint_device.width(),
			feedback_paint_device.height());
}


const GPlatesGui::Camera &
GPlatesQtWidgets::MapCanvas::get_camera() const
{
	return d_map_camera;
}


GPlatesGui::Camera &
GPlatesQtWidgets::MapCanvas::get_camera()
{
	return d_map_camera;
}


void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	update();
}


void 
GPlatesQtWidgets::MapCanvas::initializeGL() 
{
	// Initialise our context-like object first.
	d_gl_context->initialiseGL();

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// Create and initialise the offscreen render target.
	initialize_off_screen_render_target(*gl);

	// NOTE: We should not perform any operation that affects the default framebuffer (such as 'glClear()')
	//       because it's possible the default framebuffer (associated with this GLWidget) is not yet
	//       set up correctly despite its OpenGL context being the current rendering context.

	// Initialise those parts of map that require a valid OpenGL context to be bound.
	d_map.initialiseGL(*gl);

	// 'initializeGL()' should only be called once.
	d_initialisedGL = true;
}


void 
GPlatesQtWidgets::MapCanvas::resizeGL(
		int new_width,
		int new_height) 
{
	set_view();
}


void 
GPlatesQtWidgets::MapCanvas::paintGL()
{
	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*gl,
			d_view_projection,
			// Using device-independent pixels (eg, widget dimensions)...
			width(),
			height());
}


void
GPlatesQtWidgets::MapCanvas::paintEvent(
		QPaintEvent *paint_event)
{
	QOpenGLWidget::paintEvent(paint_event);

	// If d_mouse_press_info is not boost::none, then mouse is down.
	Q_EMIT repainted(static_cast<bool>(d_mouse_press_info));
}


void
GPlatesQtWidgets::MapCanvas::mousePressEvent(
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
					
	Q_EMIT mouse_pressed(
			width(),
			height(),
			d_mouse_press_info->d_mouse_screen_position,
			d_mouse_press_info->d_mouse_map_position,
			d_mouse_press_info->d_mouse_position_on_globe,
			d_mouse_press_info->d_mouse_is_on_globe,
			d_mouse_press_info->d_button,
			d_mouse_press_info->d_modifiers);
}


void
GPlatesQtWidgets::MapCanvas::mouseMoveEvent(
	QMouseEvent *move_event)
{
	update_mouse_screen_position(move_event);

	if (d_mouse_press_info)
	{
		const qreal x_dist = d_mouse_screen_position.x() - d_mouse_press_info->d_mouse_screen_position.x();
		const qreal y_dist = d_mouse_screen_position.y() - d_mouse_press_info->d_mouse_screen_position.y();
		if (x_dist * x_dist + y_dist * y_dist > 4)
		{
			d_mouse_press_info->d_is_mouse_drag = true;
		}

		if (d_mouse_press_info->d_is_mouse_drag)
		{
			Q_EMIT mouse_dragged(
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
		//
		// The mouse has moved but the left mouse button is not currently pressed.
		// This could mean no mouse buttons are currently pressed or it could mean a
		// button other than the left mouse button is currently pressed.
		// Either way it is an mouse movement that is not currently invoking a
		// canvas tool operation.
		//
		Q_EMIT mouse_moved_without_drag(
				width(),
				height(),
				d_mouse_screen_position,
				d_mouse_position_on_map_plane,
				d_mouse_position_on_globe,
				d_mouse_is_on_globe,
				centre_of_viewport());
	}
}


void 
GPlatesQtWidgets::MapCanvas::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (release_event->button() != Qt::LeftButton) 
	{
		return;
	}

	// Let's do our best to avoid crash-inducing Boost assertions.
	if ( ! d_mouse_press_info)
	{
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

	update_mouse_screen_position(release_event);

	if (abs(d_mouse_screen_position.x() - d_mouse_press_info->d_mouse_screen_position.x()) > 3 &&
		abs(d_mouse_screen_position.y() - d_mouse_press_info->d_mouse_screen_position.y()) > 3)
	{
		d_mouse_press_info->d_is_mouse_drag = true;
	}
	if (d_mouse_press_info->d_is_mouse_drag)
	{

		Q_EMIT mouse_released_after_drag(
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
	else
	{
		Q_EMIT mouse_clicked(
				width(),
				height(),
				d_mouse_press_info->d_mouse_screen_position,
				d_mouse_press_info->d_mouse_map_position,
				d_mouse_press_info->d_mouse_position_on_globe,
				d_mouse_press_info->d_mouse_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
	d_mouse_press_info = boost::none;

	// Emit repainted signal with mouse_down = false so that those listeners who
	// didn't care about intermediate repaints can now deal with the repaint.
	Q_EMIT repainted(false);
}


void
GPlatesQtWidgets::MapCanvas::mouseDoubleClickEvent(
		QMouseEvent *mouse_event)
{
	mousePressEvent(mouse_event);
}


void
GPlatesQtWidgets::MapCanvas::keyPressEvent(
		QKeyEvent *key_event)
{
	// Note that the arrow keys are handled here instead of being set as shortcuts
	// to the corresponding actions in ViewportWindow because when they were set as
	// shortcuts, they were interfering with the arrow keys on other widgets.
	switch (key_event->key())
	{
		case Qt::Key_Up:
			d_map_camera.pan_up();
			break;

		case Qt::Key_Down:
			d_map_camera.pan_down();
			break;

		case Qt::Key_Left:
			d_map_camera.pan_left();
			break;

		case Qt::Key_Right:
			d_map_camera.pan_right();
			break;

		default:
			QOpenGLWidget::keyPressEvent(key_event);
	}
}


void
GPlatesQtWidgets::MapCanvas::handle_camera_change()
{
	// switch context before we do any GL stuff
	makeCurrent();

	set_view();

	// QWidget::update:
	//   Updates the widget unless updates are disabled or the widget is hidden.
	//
	//   This function does not cause an immediate repaint; instead it schedules a paint event
	//   for processing when Qt returns to the main event loop.
	//    -- http://doc.trolltech.com/4.3/qwidget.html#update
	update_canvas();

	update_mouse_position_on_map();
}


void 
GPlatesQtWidgets::MapCanvas::initializeGL_if_necessary()
{
	// Return early if we've already initialised OpenGL.
	// This is now necessary because it's not only 'paintEvent()' and other QOpenGLWidget methods
	// that call our 'initializeGL()' method - it's now also when a client wants to render the
	// scene to an image (instead of render/update the QOpenGLWidget itself).
	if (d_initialisedGL)
	{
		return;
	}

	// Make sure the OpenGL context is current.
	// We can't use 'd_gl_context' yet because it hasn't been initialised.
	makeCurrent();

	initializeGL();
}


void
GPlatesQtWidgets::MapCanvas::initialize_off_screen_render_target(
		GPlatesOpenGL::GL &gl)
{
	if (d_off_screen_render_target_dimension > gl.get_capabilities().gl_max_texture_size)
	{
		d_off_screen_render_target_dimension = gl.get_capabilities().gl_max_texture_size;
	}

	// Create the framebuffer and its renderbuffers.
	d_off_screen_colour_renderbuffer = GPlatesOpenGL::GLRenderbuffer::create(gl);
	d_off_screen_depth_stencil_renderbuffer = GPlatesOpenGL::GLRenderbuffer::create(gl);
	d_off_screen_framebuffer = GPlatesOpenGL::GLFramebuffer::create(gl);

	// Initialise offscreen colour renderbuffer.
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_off_screen_colour_renderbuffer);
	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, d_off_screen_render_target_dimension, d_off_screen_render_target_dimension);

	// Initialise offscreen depth/stencil renderbuffer.
	// Note that (in OpenGL 3.3 core) an OpenGL implementation is only *required* to provide stencil if a
	// depth/stencil format is requested, and furthermore GL_DEPTH24_STENCIL8 is a specified required format.
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_off_screen_depth_stencil_renderbuffer);
	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, d_off_screen_render_target_dimension, d_off_screen_render_target_dimension);

	// Bind the framebuffer that'll we subsequently attach the renderbuffers to.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_off_screen_framebuffer);

	// Bind the colour renderbuffer to framebuffer's first colour attachment.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, d_off_screen_colour_renderbuffer);

	// Bind the depth/stencil renderbuffer to framebuffer's depth/stencil attachment.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, d_off_screen_depth_stencil_renderbuffer);

	const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<GPlatesOpenGL::OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering tiles in globe filled polygons.");
}


void
GPlatesQtWidgets::MapCanvas::set_view()
{
	// GLContext returns the current width and height of this GLWidget canvas.
	//
	// Note: This includes the device-pixel ratio since dimensions, in OpenGL, are in device pixels
	//       (not the device independent pixels used for widget sizes).
	const unsigned int canvas_width = d_gl_context->get_width();
	const unsigned int canvas_height = d_gl_context->get_height();
	const double canvas_aspect_ratio = double(canvas_width) / canvas_height;

	// Get the view-projection transform.
	const GPlatesOpenGL::GLMatrix view_transform = d_map_camera.get_view_transform();
	const GPlatesOpenGL::GLMatrix projection_transform = d_map_camera.get_projection_transform(canvas_aspect_ratio);

	d_view_projection = GPlatesOpenGL::GLViewProjection(
			GPlatesOpenGL::GLViewport(0, 0, canvas_width, canvas_height),
			view_transform,
			projection_transform);
}


GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels)
{
	PROFILE_FUNC();

	// Clear the colour and depth buffers of the framebuffer currently bound to GL_DRAW_FRAMEBUFFER target.
	// We also clear the stencil buffer in case it is used - also it's usually
	// interleaved with depth so it's more efficient to clear both depth and stencil.
	//
	// NOTE: Depth/stencil writes must be enabled for depth/stencil clears to work.
	//       But these should be enabled by default anyway.
	gl.DepthMask(GL_TRUE);
	gl.StencilMask(~0/*all ones*/);
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This appears to be needed on
	// Mac with Qt5 (alpha=0 is fine on Qt5 Windows/Ubuntu, and on Qt4 for all platforms). Perhaps because
	// QGLWidget rendering (on Qt5 Mac) is first done to a framebuffer object which is then blended into the
	// window framebuffer (where having a source alpha of zero would result in the black background not showing).
	// Or, more likely, maybe a framebuffer object is used on all platforms but the window framebuffer is
	// white on Mac but already black on Windows/Ubuntu.
	gl.ClearColor(0, 0, 0, 1); // Clear colour to opaque black
	gl.ClearDepth(); // Clear depth to 1.0
	gl.ClearStencil(); // Clear stencil to 0
	gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels);

	//
	// Paint the map and its contents.
	//
	// NOTE: We hold onto the previous frame's cached resources *while* generating the current frame
	// and then release our hold on the previous frame (by assigning the current frame's cache).
	// This just prevents a render frame from invalidating cached resources of the previous frame
	// in order to avoid regenerating the same cached resources unnecessarily each frame.
	// Since the view direction usually differs little from one frame to the next there is a lot
	// of overlap that we want to reuse (and not recalculate).
	//
	const cache_handle_type frame_cache_handle = d_map.paint(gl, view_projection, viewport_zoom_factor, scale);

	// Note that the overlays are rendered in screen window coordinates, so no view transform is needed.

	// Paint the text overlay.
	// We use the paint device dimensions (and not the canvas dimensions) in case the paint device
	// is not the canvas (eg, when rendering to a larger dimension SVG paint device).
	d_text_overlay->paint(
			gl,
			d_view_state.get_text_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			scale);

	d_velocity_legend_overlay->paint(
			gl,
			d_view_state.get_velocity_legend_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			scale);

	return frame_cache_handle;
}


GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene_tile_into_image(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLMatrix &image_view_transform,
		const GPlatesOpenGL::GLMatrix &image_projection_transform,
		const GPlatesOpenGL::GLTileRender &image_tile_render,
		QImage &image)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our offscreen framebuffer object for drawing and reading.
	// This directs drawing to and reading from the offscreen colour renderbuffer at the first colour attachment, and
	// its associated depth/stencil renderbuffer at the depth/stencil attachment.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_off_screen_framebuffer);

	GPlatesOpenGL::GLViewport image_tile_render_target_viewport;
	image_tile_render.get_tile_render_target_viewport(image_tile_render_target_viewport);

	GPlatesOpenGL::GLViewport image_tile_render_target_scissor_rect;
	image_tile_render.get_tile_render_target_scissor_rectangle(image_tile_render_target_scissor_rect);

	// Mask off rendering outside the current tile region in case the tile is smaller than the
	// render target. Note that the tile's viewport is slightly larger than the tile itself
	// (the scissor rectangle) in order that fat points and wide lines just outside the tile
	// have pixels rasterised inside the tile (the projection transform has also been expanded slightly).
	//
	// This includes 'gl_clear()' calls which clear the entire framebuffer.
	gl.Enable(GL_SCISSOR_TEST);
	gl.Scissor(
			image_tile_render_target_scissor_rect.x(),
			image_tile_render_target_scissor_rect.y(),
			image_tile_render_target_scissor_rect.width(),
			image_tile_render_target_scissor_rect.height());
	gl.Viewport(
			image_tile_render_target_viewport.x(),
			image_tile_render_target_viewport.y(),
			image_tile_render_target_viewport.width(),
			image_tile_render_target_viewport.height());

	// View transform associated with current image tile is same as for whole image.
	const GPlatesOpenGL::GLMatrix &image_tile_view_transform = image_view_transform;

	// Projection transform associated with current image tile is post-multiplied with the
	// projection transform for the whole image.
	GPlatesOpenGL::GLMatrix image_tile_projection_transform =
			image_tile_render.get_tile_projection_transform()->get_matrix();
	image_tile_projection_transform.gl_mult_matrix(image_projection_transform);

	// The view/projection/viewport for the current image tile.
	const GPlatesOpenGL::GLViewProjection image_tile_view_projection(
			image_tile_render_target_viewport,  // The viewport that is used for rendering tile.
			image_tile_view_transform,
			image_tile_projection_transform);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			gl,
			image_tile_view_projection,
			// Since QImage is just raw pixels its dimensions are in device pixels, but
			// we need device-independent pixels here (eg, widget dimensions)...
			image.width() / image.devicePixelRatio(),
			image.height() / image.devicePixelRatio());

	//
	// Copy the rendered tile into the appropriate sub-rect of the image.
	//

	GPlatesOpenGL::GLViewport current_tile_source_viewport;
	image_tile_render.get_tile_source_viewport(current_tile_source_viewport);

	GPlatesOpenGL::GLViewport current_tile_destination_viewport;
	image_tile_render.get_tile_destination_viewport(current_tile_destination_viewport);

	GPlatesOpenGL::GLImageUtils::copy_rgba8_framebuffer_into_argb32_qimage(
			gl,
			image,
			current_tile_source_viewport,
			current_tile_destination_viewport);

	return tile_cache_handle;
}


GPlatesMaths::PointOnSphere
GPlatesQtWidgets::MapCanvas::centre_of_viewport() const
{
	// The point on the globe which corresponds to the centre of the viewport.
	//
	// Note that the map camera look-at position (on map plane) is restricted to be inside
	// the map projection boundary, so this always returned a valid position on the globe.
	return d_map_camera.get_look_at_position_on_globe();
}


void
GPlatesQtWidgets::MapCanvas::update_mouse_screen_position(
		QMouseEvent *mouse_event)
{
	d_mouse_screen_position = mouse_event->localPos();

	update_mouse_position_on_map();
}


void
GPlatesQtWidgets::MapCanvas::update_mouse_position_on_map()
{
	const GPlatesGui::MapProjection &map_projection = d_view_state.get_map_projection();

	// Note that OpenGL and Qt y-axes are the reverse of each other.
	const double mouse_window_y = height() - d_mouse_screen_position.y();
	const double mouse_window_x = d_mouse_screen_position.x();

	// Project screen coordinates into a ray into 3D scene.
	const GPlatesOpenGL::GLIntersect::Ray camera_ray =
			d_map_camera.get_camera_ray_at_window_coord(mouse_window_x, mouse_window_y, width(), height());

	// See if camera ray at screen coordinates intersects the 2D map plane (z=0).
	//
	// In perspective view it's possible for a screen pixel ray emanating from the camera eye to
	// miss the map plane entirely (even though the map plane is infinite).
	//
	// Given the camera ray, calculate a position on the map *plane* (2D plane with z=0), or
	// none if screen view ray (at screen position) does not intersect the map plane.
	d_mouse_position_on_map_plane = d_map_camera.get_position_on_map_plane_at_camera_ray(camera_ray);

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
				d_map_camera.get_position_on_map_boundary_intersected_by_2d_camera_ray(ray_direction, ray_origin);
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

		Q_EMIT mouse_position_on_map_changed(d_mouse_position_on_globe, d_mouse_is_on_globe);
	}
}


float
GPlatesQtWidgets::MapCanvas::calculate_scale(
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels) const
{
	// Note that we use regular device *independent* sizes not high-DPI device pixels
	// (ie, not using device pixel ratio) to calculate scale because font sizes, etc, are
	// based on these coordinates (it's only OpenGL, really, that deals with device pixels).
	const int paint_device_dimension = (std::min)(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels);
	const int min_viewport_dimension = (std::min)(width(), height());

	// If paint device is larger than the viewport then don't scale - this avoids having
	// too large point/line sizes when exporting large screenshots.
	if (paint_device_dimension >= min_viewport_dimension)
	{
		return 1.0f;
	}

	// This is useful when rendering the small colouring previews - avoids too large point/line sizes.
	return static_cast<float>(paint_device_dimension) / static_cast<float>(min_viewport_dimension);
}
