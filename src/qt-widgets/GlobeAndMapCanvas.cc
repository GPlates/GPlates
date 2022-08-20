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
#include <QDebug>

#include "GlobeAndMapCanvas.h"
#ifdef GPLATES_PINCH_ZOOM_ENABLED // Defined in GlobeAndMapCanvas.h
#include <QPinchGesture>
#endif

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "gui/GlobeCamera.h"
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


GPlatesQtWidgets::GlobeAndMapCanvas::GlobeAndMapCanvas(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QOpenGLWidget(parent_),
	d_view_state(view_state),
	d_gl_context(
			GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QOpenGLWidgetImpl(*this)))),
	d_initialisedGL(false),
	d_view_projection(
			GPlatesOpenGL::GLViewport(0, 0, d_gl_context->get_width(), d_gl_context->get_height()),
			// Use identity transforms for now, these will get updated when the camera changes...
			GPlatesOpenGL::GLMatrix::IDENTITY,
			GPlatesOpenGL::GLMatrix::IDENTITY),
	d_off_screen_render_target_dimension(OFF_SCREEN_RENDER_TARGET_DIMENSION),
	d_gl_visual_layers(
			GPlatesOpenGL::GLVisualLayers::create(
					d_gl_context,
					view_state.get_application_state())),
	// The following unit-vector initialisation value is arbitrary.
	d_mouse_position_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_is_on_globe(false),
	d_zoom_enabled(true),
	d_projection(d_view_state.get_projection()),
	d_globe(
			view_state,
			d_gl_visual_layers,
			view_state.get_rendered_geometry_collection(),
			view_state.get_visual_layers(),
			devicePixelRatio()),
	d_map(
			view_state,
			d_gl_visual_layers,
			view_state.get_rendered_geometry_collection(),
			view_state.get_visual_layers(),
			devicePixelRatio()),
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
	
	// Ensure the globe/map will always expand to fill available space.
	// A minumum size and non-collapsibility is set on the globe/map basically so users
	// can't obliterate it and then wonder where their globe/map went.
	QSizePolicy globe_and_map_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	globe_and_map_size_policy.setHorizontalStretch(255);
	setSizePolicy(globe_and_map_size_policy);
	setFocusPolicy(Qt::StrongFocus);
	setMinimumSize(100, 100);

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

	// Handle changes in the projection. This includes globe and map projections as well as
	// view projections (switching between orthographic and perspective).
	QObject::connect(
			&d_projection,
			SIGNAL(globe_map_projection_changed(
					const GPlatesGui::Projection::globe_map_projection_type &,
					const GPlatesGui::Projection::globe_map_projection_type &)),
			this,
			SLOT(handle_globe_map_projection_changed(
					const GPlatesGui::Projection::globe_map_projection_type &,
					const GPlatesGui::Projection::globe_map_projection_type &)));
	// Now handle changes to just the viewport projection.
	QObject::connect(
			&d_projection,
			SIGNAL(viewport_projection_changed(
					GPlatesGui::Projection::viewport_projection_type,
					GPlatesGui::Projection::viewport_projection_type)),
			this,
			SLOT(handle_viewport_projection_changed(
					GPlatesGui::Projection::viewport_projection_type,
					GPlatesGui::Projection::viewport_projection_type)));

	// Update our view whenever the globe and map cameras change.
	//
	// Note that the cameras are updated when the zoom changes.

	// Globe camera.
	QObject::connect(
			&d_view_state.get_globe_camera(), SIGNAL(camera_changed()),
			this, SLOT(handle_camera_change()));
	// Map camera.
	QObject::connect(
			&d_view_state.get_map_camera(), SIGNAL(camera_changed()),
			this, SLOT(handle_camera_change()));

	handle_camera_change();

#ifdef GPLATES_PINCH_ZOOM_ENABLED
	grabGesture(Qt::PinchGesture);
#endif

	setAttribute(Qt::WA_NoSystemBackground);
}


GPlatesQtWidgets::GlobeAndMapCanvas::~GlobeAndMapCanvas()
{
	// Note that when our data members that contain OpenGL resources (like 'd_globe' and 'd_map') are destroyed they don't actually
	// destroy the *native* OpenGL resources. Instead the native resource *wrappers* get destroyed (see GLObjectResource)
	// which just queue the native resources for deallocation with our resource managers (see GLObjectResourceManager).
	// But when the resource managers get destroyed (when our 'd_gl_context' is destroyed) they also don't destroy the
	// native resources. Instead, the native resources (queued for destruction) only get destroyed when 'GLContext::begin_render()'
	// and 'GLContext::end_render()' get called (which only happens when we're actually going to render something).
	//
	// As a result, the native resources only get destroyed when the *native* OpenGL context itself is destroyed
	// (this is taken care of by our base class QOpenGLWidget destructor).
	//
	// Also note we could connect to the 'QOpenGLContext::aboutToBeDestroyed' signal, but that also is unnecessary
	// for us since we never re-parent GlobeAndMapCanvas to a different top-level window and hence are not required
	// to destroy our resources before rebuilding them again in 'initializeGL()'. The resources only need to be destroyed
	// once (when GPlates shuts down).
}


double
GPlatesQtWidgets::GlobeAndMapCanvas::current_proximity_inclusion_threshold(
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

	//
	// Limit the maximum angular distance on unit sphere.
	//
	// Globe view: When the click point is at the circumference of the visible globe, a one viewport pixel variation
	//             can result in a large traversal on the globe since the globe surface is tangential to the view there.
	//
	// Map view: When the map view is tilted the click point can intersect the map plane (z=0) at an acute angle such
	//           that one viewport pixel can cover a large area on the map.
	//           Additionally the map projection itself (eg, Rectangular, Mollweide, etc) can further stretch the
	//           viewport pixel (already projected onto map plane z=0) when it's inverse transformed back onto the globe.
	//
	// As such, a small mouse-pointer displacement on-screen can result in significantly different mouse-pointer
	// displacements on the surface of the globe depending on the location of the click point.
	//
	// To take this into account we use the current view and projection transforms (and viewport) to project
	// one screen pixel area onto the globe and find the maximum deviation of this area projected onto the globe
	// (in terms of angular distance on the globe).
	//

	const double max_distance_inclusion_threshold = GPlatesMaths::convert_deg_to_rad(5.0);

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

	// If we're viewing the map (instead of globe) then we also need the map projection.
	//
	// This is because, for the map view, we need to project one screen pixel area onto the map plane (z=0) and
	// then inverse transform from the map plane onto the globe (using the map projection, eg, Rectangular, Mollweide, etc).
	// This finds the maximum deviation of this area projected onto the globe (in terms of angular distance on the globe).
	boost::optional<const GPlatesGui::MapProjection &> map_projection;
	if (is_map_active())
	{
		map_projection = d_view_state.get_map_projection();
	}

	// Calculate the maximum distance on the unit-sphere subtended by one viewport pixel projected onto it.
	boost::optional< std::pair<double/*min*/, double/*max*/> > min_max_device_independent_pixel_size =
			gl_view_projection.get_min_max_pixel_size_on_globe(click_point, map_projection);
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
GPlatesQtWidgets::GlobeAndMapCanvas::get_viewport_size() const
{
	return QSize(width(), height());
}


QImage
GPlatesQtWidgets::GlobeAndMapCanvas::render_to_qimage(
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
	const GPlatesOpenGL::GLMatrix image_view_transform =
			get_active_camera().get_view_transform();
	const GPlatesOpenGL::GLMatrix image_projection_transform =
			get_active_camera().get_projection_transform(image_aspect_ratio);

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
GPlatesQtWidgets::GlobeAndMapCanvas::render_opengl_feedback_to_paint_device(
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
	const GPlatesOpenGL::GLMatrix feedback_paint_device_view_transform =
			get_active_camera().get_view_transform();
	const GPlatesOpenGL::GLMatrix feedback_paint_device_projection_transform =
			get_active_camera().get_projection_transform(feedback_paint_device_aspect_ratio);

	const GPlatesOpenGL::GLViewProjection feedback_paint_device_view_projection(
			feedback_paint_device_viewport,
			feedback_paint_device_view_transform,
			feedback_paint_device_projection_transform);

	// Set the viewport (and scissor rectangle) to the size of the feedback paint device
	// (instead of the globe/map canvas) since we're rendering to it (via transform feedback).
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
GPlatesQtWidgets::GlobeAndMapCanvas::get_active_camera() const
{
	return is_globe_active()
			? static_cast<GPlatesGui::Camera &>(d_view_state.get_globe_camera())
			: static_cast<GPlatesGui::Camera &>(d_view_state.get_map_camera());
}


GPlatesGui::Camera &
GPlatesQtWidgets::GlobeAndMapCanvas::get_active_camera()
{
	return is_globe_active()
			? static_cast<GPlatesGui::Camera &>(d_view_state.get_globe_camera())
			: static_cast<GPlatesGui::Camera &>(d_view_state.get_map_camera());
}


bool
GPlatesQtWidgets::GlobeAndMapCanvas::is_globe_active() const
{
	return d_projection.get_globe_map_projection().is_viewing_globe_projection();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::set_zoom_enabled(
		bool enabled)
{
	d_zoom_enabled = enabled;
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_canvas()
{
	update();
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::initializeGL() 
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

	// Initialise those parts of globe and map that require a valid OpenGL context to be bound.
	d_globe.initialiseGL(*gl);
	d_map.initialiseGL(*gl);

	// 'initializeGL()' should only be called once.
	d_initialisedGL = true;
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::resizeGL(
		int new_width,
		int new_height) 
{
	// The canvas dimensions have changed and this affects the projection transform of the view.
	set_view();
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::paintGL()
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
GPlatesQtWidgets::GlobeAndMapCanvas::paintEvent(
		QPaintEvent *paint_event)
{
	QOpenGLWidget::paintEvent(paint_event);

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
			QOpenGLWidget::keyPressEvent(key_event);
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::wheelEvent(
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
bool
GPlatesQtWidgets::GlobeAndMapCanvas::event(
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


void
GPlatesQtWidgets::GlobeAndMapCanvas::handle_camera_change()
{
	// The active camera has been modified and this affects the view-projection transform of the view.
	set_view();

	// The active camera has been modified so make sure our mouse position on globe/map is up-to-date.
	update_mouse_position_on_globe_or_map();

	// QWidget::update:
	//   Updates the widget unless updates are disabled or the widget is hidden.
	//
	//   This function does not cause an immediate repaint; instead it schedules a paint event
	//   for processing when Qt returns to the main event loop.
	//    -- http://doc.trolltech.com/4.3/qwidget.html#update
	update_canvas();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::handle_globe_map_projection_changed(
		const GPlatesGui::Projection::globe_map_projection_type &old_globe_map_projection,
		const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection)
{
	//
	// We could be switching from the globe camera to map camera (or vice versa).
	//
	// If so, then get the camera view orientation, tilt and viewport projection of the old camera
	// (before the projection change) and set them on the new camera (after the projection change).
	//
	// The view orientation is the combined camera look-at position and the orientation rotation around that look-at position.
	//
	// Note: Switching between globe and map cameras (transferring view orientation, tilt and viewport projection)
	//       doesn't necessarily cause the switched-to camera to emit a 'camera_changed' signal.
	//       This is because the view orientation, tilt and viewport projection might not have changed.
	//       This can happen if the user is simply switching back and forth between the globe and map views.
	//       So we'll detect if the 'camera_changed' signal was NOT emitted and essentially handle it ourself
	//       (by directly calling our 'handle_camera_changed' slot).
	//

	// If switching from map to globe projection...
	if (old_globe_map_projection.is_viewing_map_projection() &&
		globe_map_projection.is_viewing_globe_projection())
	{
		const GPlatesGui::MapCamera &map_camera = d_view_state.get_map_camera();
		GPlatesGui::GlobeCamera &globe_camera = d_view_state.get_globe_camera();

		// Get *map* camera view orientation, tilt and viewport projection.
		const GPlatesMaths::Rotation map_camera_view_orientation = map_camera.get_view_orientation();
		const GPlatesMaths::real_t map_camera_tilt_angle = map_camera.get_tilt_angle();
		const GPlatesGui::Projection::viewport_projection_type map_viewport_projection = map_camera.get_viewport_projection();

		bool emitted_camera_change_signal = false;

		// Set the *globe* camera view orientation, tilt and viewport projection.
		// Also detect if the 'camera_change' signal was emitted.
		if (map_camera_view_orientation.quat() != globe_camera.get_view_orientation().quat())
		{
			globe_camera.set_view_orientation(map_camera_view_orientation);
			emitted_camera_change_signal = true;
		}
		if (map_camera_tilt_angle != globe_camera.get_tilt_angle())
		{
			globe_camera.set_tilt_angle(map_camera_tilt_angle);
			emitted_camera_change_signal = true;
		}
		if (map_viewport_projection != globe_camera.get_viewport_projection())
		{
			globe_camera.set_viewport_projection(map_viewport_projection);
			emitted_camera_change_signal = true;
		}

		if (!emitted_camera_change_signal)
		{
			// The globe camera didn't actually change (since the last time it was active).
			// But we've switched from the map camera. That's a camera change, so we need to handle it.
			handle_camera_change();
		}
	}
	// else if switching from globe to map projection...
	else if (old_globe_map_projection.is_viewing_globe_projection() &&
			globe_map_projection.is_viewing_map_projection())
	{
		const GPlatesGui::GlobeCamera &globe_camera = d_view_state.get_globe_camera();
		GPlatesGui::MapCamera &map_camera = d_view_state.get_map_camera();

		// Get *globe* camera view orientation, tilt and viewport projection.
		const GPlatesMaths::Rotation globe_camera_view_orientation = globe_camera.get_view_orientation();
		const GPlatesMaths::real_t globe_camera_tilt_angle = globe_camera.get_tilt_angle();
		const GPlatesGui::Projection::viewport_projection_type globe_viewport_projection = globe_camera.get_viewport_projection();

		bool emitted_camera_change_signal = false;

		// Set the *map* camera view orientation, tilt and viewport projection.
		// Also detect if the 'camera_change' signal was emitted.
		if (globe_camera_view_orientation.quat() != map_camera.get_view_orientation().quat())
		{
			map_camera.set_view_orientation(globe_camera_view_orientation);
			emitted_camera_change_signal = true;
		}
		if (globe_camera_tilt_angle != map_camera.get_tilt_angle())
		{
			map_camera.set_tilt_angle(globe_camera_tilt_angle);
			emitted_camera_change_signal = true;
		}
		if (globe_viewport_projection != map_camera.get_viewport_projection())
		{
			map_camera.set_viewport_projection(globe_viewport_projection);
			emitted_camera_change_signal = true;
		}

		// Update the map projection.
		//
		// It shouldn't have changed since the last time the map camera was active, but just in case.
		//
		// Note: This doesn't emit a 'camera_changed' signal.
		d_view_state.get_map_projection().set_projection_type(
				globe_map_projection.get_map_projection_type());
		d_view_state.get_map_projection().set_central_meridian(
				globe_map_projection.get_map_central_meridian());

		if (!emitted_camera_change_signal)
		{
			// The map camera didn't actually change (since the last time it was active).
			// But we've switched from the globe camera. That's a camera change, so we need to handle it.
			handle_camera_change();
		}
	}
	else // switching between two map projections and/or changing central meridian in one map projection...
	{
		// Update the map projection.
		d_view_state.get_map_projection().set_projection_type(
				globe_map_projection.get_map_projection_type());
		d_view_state.get_map_projection().set_central_meridian(
				globe_map_projection.get_map_central_meridian());

		// Something changed in the map projection (otherwise we wouldn't be here).
		// So we need to handle that.
		handle_camera_change();
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::handle_viewport_projection_changed(
		GPlatesGui::Projection::viewport_projection_type old_viewport_projection,
		GPlatesGui::Projection::viewport_projection_type viewport_projection)
{
	// Change the viewport projection (orthographic or perspective) of the active camera.
	//
	// Note: This will cause the active camera to emit the 'camera_changed' signal which
	//       will call our 'handle_camera_change' slot.
	get_active_camera().set_viewport_projection(viewport_projection);
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::initializeGL_if_necessary()
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
GPlatesQtWidgets::GlobeAndMapCanvas::initialize_off_screen_render_target(
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
GPlatesQtWidgets::GlobeAndMapCanvas::set_view()
{
	// GLContext returns the current width and height of this GLWidget canvas.
	//
	// Note: This includes the device-pixel ratio since dimensions, in OpenGL, are in device pixels
	//       (not the device independent pixels used for widget sizes).
	const unsigned int canvas_width = d_gl_context->get_width();
	const unsigned int canvas_height = d_gl_context->get_height();
	const double canvas_aspect_ratio = double(canvas_width) / canvas_height;

	// Get the view-projection transform.
	const GPlatesOpenGL::GLMatrix view_transform = get_active_camera().get_view_transform();
	const GPlatesOpenGL::GLMatrix projection_transform = get_active_camera().get_projection_transform(canvas_aspect_ratio);

	d_view_projection = GPlatesOpenGL::GLViewProjection(
			GPlatesOpenGL::GLViewport(0, 0, canvas_width, canvas_height),
			view_transform,
			projection_transform);
}


GPlatesQtWidgets::GlobeAndMapCanvas::cache_handle_type
GPlatesQtWidgets::GlobeAndMapCanvas::render_scene(
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
	// Note that we clear the colour to (0,0,0,0) and not (0,0,0,1) because we want any transparent
	// parts of the scene (parts that we don't render) to have an alpha of zero.
	// This is because this code is used not only to render the viewport window but also for exporting images of the
	// viewport window, and we want image formats supporting transparency (like PNG) to have a transparent background.
	//
	// Previously we had this as (0,0,0,1) because alpha=1 it appeared to be needed on macOS with Qt5.
	// Perhaps because QGLWidget rendering (on Qt5 Mac) was first done to a framebuffer object which was then blended
	// into the window framebuffer (where having a source alpha of zero would result in the black background not showing).
	// Or, more likely, maybe a framebuffer object is used on all platforms but the window framebuffer is white on Mac
	// but already black on Windows/Ubuntu (maybe because we turned off background rendering with
	// 	"setAutoFillBackground(false)" and "setAttribute(Qt::WA_NoSystemBackground)").
	//
	// Since switching to QOpenGLWidget (from QGLWidget) it doesn't appear to be an issue anymore.
	// But we are now switching again to QVulkanWindow (all our OpenGL rendering will go through Vulkan).
	//
	// TODO: Check that alpha=0 works with the QVulkanWindow that we now use (instead of QOpenGLWidget).
	//
	gl.ClearColor(); // Clear colour to (transparent) black
	gl.ClearDepth(); // Clear depth to 1.0
	gl.ClearStencil(); // Clear stencil to 0
	gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels);

	//
	// Paint the globe or map (and its contents) depending on whether the globe or map is currently active.
	//
	// NOTE: We hold onto the previous frame's cached resources *while* generating the current frame
	// and then release our hold on the previous frame (by assigning the current frame's cache).
	// This just prevents a render frame from invalidating cached resources of the previous frame
	// in order to avoid regenerating the same cached resources unnecessarily each frame.
	// Since the view direction usually differs little from one frame to the next there is a lot
	// of overlap that we want to reuse (and not recalculate).
	//
	cache_handle_type frame_cache_handle;
	if (is_globe_active())
	{
		frame_cache_handle = d_globe.paint(
				gl,
				view_projection,
				d_view_state.get_globe_camera().get_front_globe_horizon_plane(),
				viewport_zoom_factor,
				scale);
	}
	else
	{
		frame_cache_handle = d_map.paint(
				gl,
				view_projection,
				viewport_zoom_factor,
				scale);
	}

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

	// Paint the velocity legend overlay
	d_velocity_legend_overlay->paint(
			gl,
			d_view_state.get_velocity_legend_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			scale);

	return frame_cache_handle;
}


GPlatesQtWidgets::GlobeAndMapCanvas::cache_handle_type
GPlatesQtWidgets::GlobeAndMapCanvas::render_scene_tile_into_image(
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
	// This includes 'glClear()' calls which are bounded by the scissor rectangle.
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
	d_mouse_screen_position = mouse_event->localPos();

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


float
GPlatesQtWidgets::GlobeAndMapCanvas::calculate_scale(
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
