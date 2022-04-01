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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <cmath>
#include <iostream>
#include <QApplication>
#include <QDebug>
#include <QGLWidget>
#include <QGraphicsView>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>

#include "GlobeCanvas.h"
#include "MapCanvas.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "gui/Map.h"
#include "gui/MapProjection.h"
#include "gui/MapTransform.h"
#include "gui/ProjectionException.h"
#include "gui/TextOverlay.h"
#include "gui/VelocityLegendOverlay.h"
#include "gui/ViewportZoom.h"

#include "opengl/GL.h"
#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLViewport.h"
#include "opengl/OpenGLException.h"

#include "presentation/ViewState.h"

#include "utils/Profile.h"


namespace GPlatesQtWidgets
{
	namespace
	{
		double
		distance_between_qpointfs(
				const QPointF& p1,
				const QPointF& p2)
		{
			QPointF difference = p1 - p2;

			return sqrt(difference.x()*difference.x() + difference.y()*difference.y());
		}
	}
}


// Public constructor
GPlatesQtWidgets::MapCanvas::MapCanvas(
		GPlatesPresentation::ViewState &view_state,
		GlobeCanvas &globe_canvas,
		QWidget *parent_):
	QGLWidget(
			GPlatesOpenGL::GLContext::get_qgl_format_to_create_context_with(),
			parent_,
			// Share texture objects, vertex buffer objects, etc...
			&globe_canvas),
	d_view_state(view_state),
	d_gl_context(
			// Mirror the sharing of OpenGL context state (if sharing).
			// Both GLWidgets should be sharing since they were created with the same QGLFormat...
			globe_canvas.isSharing()
			? GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QGLWidgetImpl(*this)),
					*globe_canvas.get_gl_context())
			: GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QGLWidgetImpl(*this)))),
	d_make_context_current(*d_gl_context),
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
	// Since we're using a QPainter inside 'paintEvent()' or more specifically 'paintGL()'
	// (which is called from 'paintEvent()') then we turn off automatic swapping of the OpenGL
	// front and back buffers after each 'paintGL()' call. This is because QPainter::end(),
	// or QPainter's destructor, automatically calls QGLWidget::swapBuffers() if auto buffer swap
	// is enabled - and this results in two calls to QGLWidget::swapBuffers() - one from QPainter
	// and one from 'paintEvent()'. So we disable auto buffer swapping and explicitly call it ourself.
	//
	// Also we don't want to swap buffers when we're just rendering to a QImage (using OpenGL)
	// and not rendering to the QGLWidget itself, otherwise the widget will have the wrong content.
	setAutoBufferSwap(false);

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
	QObject::connect(
			&d_map_camera, SIGNAL(camera_changed()),
			this, SLOT(force_mouse_pointer_pos_change()));

	handle_camera_change();

	setAttribute(Qt::WA_NoSystemBackground);
}


GPlatesQtWidgets::MapCanvas::~MapCanvas()
{  }


double
GPlatesQtWidgets::MapCanvas::current_proximity_inclusion_threshold(
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

	boost::optional<GPlatesMaths::LatLonPoint> llp =  map().projection().inverse_transform(x_, y_);

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


void 
GPlatesQtWidgets::MapCanvas::initializeGL_if_necessary()
{
	// Return early if we've already initialised OpenGL.
	// This is now necessary because it's not only 'paintEvent()' and other QGLWidget methods
	// that call our 'initializeGL()' method - it's now also when a client wants to render the
	// scene to an image (instead of render/update the QGLWidget itself).
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
GPlatesQtWidgets::MapCanvas::initializeGL() 
{
	// Initialise our context-like object first.
	d_gl_context->initialise();

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
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, d_off_screen_render_target_dimension, d_off_screen_render_target_dimension);

	// Initialise offscreen depth/stencil renderbuffer.
	// Note that (in OpenGL 3.3 core) an OpenGL implementation is only *required* to provide stencil if a
	// depth/stencil format is requested, and furthermore GL_DEPTH24_STENCIL8 is a specified required format.
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_off_screen_depth_stencil_renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, d_off_screen_render_target_dimension, d_off_screen_render_target_dimension);

	// Bind the framebuffer that'll we subsequently attach the renderbuffers to.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_off_screen_framebuffer);

	// Bind the colour renderbuffer to framebuffer's first colour attachment.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, d_off_screen_colour_renderbuffer);

	// Bind the depth/stencil renderbuffer to framebuffer's depth/stencil attachment.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, d_off_screen_depth_stencil_renderbuffer);

	const GLenum completeness = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<GPlatesOpenGL::OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering tiles in globe filled polygons.");
}


GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels,
		int map_canvas_paint_device_width_in_device_independent_pixels,
		int map_canvas_paint_device_height_in_device_independent_pixels)
{
	PROFILE_FUNC();

	// Clear the colour and depth buffers of the main framebuffer.
	//
	// NOTE: We don't use the depth buffer in the map view but clear it anyway so that we can
	// use common layer painting code with the 3D globe rendering that enables depth testing.
	// In our case the depth testing will always return true - depth testing is very fast
	// in modern graphics hardware so we don't need to optimise it away.
	// We also clear the stencil buffer in case it is used - also it's usually interleaved
	// with depth so it's more efficient to clear both depth and stencil.
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
	gl.ClearStencil();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			map_canvas_paint_device_width_in_device_independent_pixels,
			map_canvas_paint_device_height_in_device_independent_pixels);

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

	// Draw the optional text overlay.
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


void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	update();
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
#if 1
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
#else
	// Restore the QPainter's transform after our rendering because we overwrite it during our
	// text rendering (where we set it to the identity transform).
	const QTransform qpainter_world_transform = painter->worldTransform();

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->create_gl();
	GPlatesOpenGL::GL::RenderScope render_scope(*gl);

	// Get the model-view matrix.
	GPlatesOpenGL::GLMatrix model_view_matrix;
	get_model_view_matrix_from_2D_world_transform(model_view_matrix, d_viewport_transform);

	// The QPainter's paint device.
	const QPaintDevice *qpaint_device = painter->device();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			qpaint_device,
			GPLATES_ASSERTION_SOURCE);

	// Get the projection matrix for the QPainter's paint device.
	const GPlatesOpenGL::GLMatrix projection_matrix_scene = 
			get_ortho_projection_matrix_from_dimensions(
					// Using device-independent pixels (eg, widget dimensions)...
					qpaint_device->width(),
					qpaint_device->height());

	// GLContext returns the current width and height of this GLWidget canvas.
	//
	// Note: This includes the device-pixel ratio since dimensions, in OpenGL, are in device pixels
	//       (not the device independent pixels used for widget sizes).
	const unsigned int canvas_width = d_gl_context->get_width();
	const unsigned int canvas_height = d_gl_context->get_height();

	const GPlatesOpenGL::GLViewProjection view_projection(
			GPlatesOpenGL::GLViewport(0, 0, canvas_width, canvas_height),
			model_view_matrix/*view*/,
			projection_matrix_scene/*projection*/);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*gl,
			view_projection,
			// Using device-independent pixels (eg, widget dimensions)...
			qpaint_device->width(),
			qpaint_device->height(),
			qpaint_device->width(),
			qpaint_device->height());

	// Restore the QPainter's original world transform in case we modified it during rendering.
	painter->setWorldTransform(qpainter_world_transform);
#endif
}


void
GPlatesQtWidgets::MapCanvas::paintEvent(
		QPaintEvent *paint_event)
{
	QGLWidget::paintEvent(paint_event);

	// Explicitly swap the OpenGL front and back buffers.
	// Note that we have already disabled auto buffer swapping because otherwise both the QPainter
	// in 'paintGL()' and 'QGLWidget::paintEvent()' will call 'QGLWidget::swapBuffers()'
	// essentially canceling each other out (or causing flickering).
	if (doubleBuffer() && !autoBufferSwap())
	{
		swapBuffers();
	}

	// If d_mouse_press_info is not boost::none, then mouse is down.
	Q_EMIT repainted(static_cast<bool>(d_mouse_press_info));
}


void
GPlatesQtWidgets::MapCanvas::mousePressEvent(
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
					
	Q_EMIT mouse_pressed(
			d_mouse_press_info->d_mouse_pointer_scene_coords,
			d_mouse_press_info->d_is_on_surface,
			d_mouse_press_info->d_button,
			d_mouse_press_info->d_modifiers);
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

	if (abs(release_event->x() - d_mouse_press_info->d_mouse_pointer_screen_pos_x) > 3 &&
			abs(release_event->y() - d_mouse_press_info->d_mouse_pointer_screen_pos_y) > 3) {
		d_mouse_press_info->d_is_mouse_drag = true;
	}
	if ((d_mouse_press_info->d_is_mouse_drag))
	{

		Q_EMIT mouse_released_after_drag(
				d_mouse_press_info->d_mouse_pointer_scene_coords,
				d_mouse_press_info->d_is_on_surface,
				mouse_pointer_scene_coords(),
				mouse_pointer_is_on_surface(),
				QPointF(),
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);

	} else {
		Q_EMIT mouse_clicked(
				d_mouse_press_info->d_mouse_pointer_scene_coords,
				d_mouse_press_info->d_is_on_surface,
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
GPlatesQtWidgets::MapCanvas::mouseMoveEvent(
	QMouseEvent *move_event)
{
	QPointF translation = mapToScene(move_event->pos()) - 
		mapToScene(d_last_mouse_view_coords);

	d_last_mouse_view_coords = move_event->pos();

	update_mouse_pointer_pos(move_event);

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
			Q_EMIT mouse_dragged(
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
		Q_EMIT mouse_moved_without_drag(
				mouse_pointer_scene_coords(),
				mouse_pointer_is_on_surface(),
				translation);
	}
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
			move_camera_up();
			break;

		case Qt::Key_Down:
			move_camera_down();
			break;

		case Qt::Key_Left:
			move_camera_left();
			break;

		case Qt::Key_Right:
			move_camera_right();
			break;

		default:
			QGraphicsView::keyPressEvent(key_event);
	}
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
	// Set up a QPainter to help us with OpenGL text rendering.
	QPainter painter(&map_canvas_paint_device);

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
			image_size_in_device_independent_pixels.width() * map_canvas_paint_device.devicePixelRatio(),
			image_size_in_device_independent_pixels.height() * map_canvas_paint_device.devicePixelRatio());
	QImage image(image_size_in_device_pixels, QImage::Format_ARGB32);
	image.setDevicePixelRatio(map_canvas_paint_device.devicePixelRatio());

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

	// Get the model-view matrix from the 2D world transform.
	GPlatesOpenGL::GLMatrix image_view_transform;
	get_model_view_matrix_from_2D_world_transform(image_view_transform, viewport_transform);

	// Get the projection matrix for the image dimensions.
	// It'll get adjusted per tile (that the scene is rendered to).
	const GPlatesOpenGL::GLMatrix image_projection_transform =
			get_ortho_projection_matrix_from_dimensions(
					// Using device-independent pixels (eg, widget dimensions)...
					image_size_in_device_independent_pixels.width(),
					image_size_in_device_independent_pixels.height());

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int tile_border = 10;
	// Set up for rendering the scene into tiles.
	// The tile render target dimensions match the framebuffer dimensions.
	GPlatesOpenGL::GLTileRender image_tile_render(
			d_off_screen_render_target_dimension/*tile_render_target_width*/,
			d_off_screen_render_target_dimension/*tile_render_target_height*/,
			image_viewport/*destination_viewport*/,
			tile_border);

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (image_tile_render.first_tile(); !image_tile_render.finished(); image_tile_render.next_tile())
	{
		// Render the scene to the current tile.
		// Hold onto the previous frame's cached resources *while* generating the current frame.
		const cache_handle_type tile_cache_handle = render_scene_tile_into_image(
				*gl,
				image_view_transform,
				image_projection_transform,
				image_tile_render,
				image,
				map_canvas_paint_device);
		frame_cache_handle->push_back(tile_cache_handle);
	}

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = frame_cache_handle;

	return image;
}

GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene_tile_into_image(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLMatrix &image_view_transform,
		const GPlatesOpenGL::GLMatrix &image_projection_transform,
		const GPlatesOpenGL::GLTileRender &image_tile_render,
		QImage &image,
		const QPaintDevice &map_canvas_paint_device)
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
			image.height() / image.devicePixelRatio(),
			map_canvas_paint_device.width(),
			map_canvas_paint_device.height());

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

void
GPlatesQtWidgets::MapCanvas::render_opengl_feedback_to_paint_device(
		QPaintDevice &feedback_paint_device)
{
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

	const GPlatesOpenGL::GLViewport feedback_paint_device_viewport(
			0, 0,
			feedback_paint_device_pixel_width,
			feedback_paint_device_pixel_height);

	// Set the viewport (and scissor rectangle) to the size of the feedback paint device
	// (instead of the map canvas) since we're rendering to it (via transform feedback).
	gl->Viewport(
			feedback_paint_device_viewport.x(), feedback_paint_device_viewport.y(),
			feedback_paint_device_viewport.width(), feedback_paint_device_viewport.height());
	gl->Scissor(
			feedback_paint_device_viewport.x(), feedback_paint_device_viewport.y(),
			feedback_paint_device_viewport.width(), feedback_paint_device_viewport.height());

	// Get the model-view matrix from the 2D world transform.
	GPlatesOpenGL::GLMatrix model_view_matrix;
	get_model_view_matrix_from_2D_world_transform(model_view_matrix, viewport_transform);

	// Get the projection matrix for the feedback paint device.
	const GPlatesOpenGL::GLMatrix projection_matrix_scene =
			get_ortho_projection_matrix_from_dimensions(
					// Using device-independent pixels (eg, widget dimensions)...
					feedback_paint_device.width(),
					feedback_paint_device.height());

	const GPlatesOpenGL::GLViewProjection feedback_paint_device_view_projection(
			feedback_paint_device_viewport,
			model_view_matrix/*view*/,
			projection_matrix_scene/*projection*/);

	// Render the scene to the feedback paint device.
	// This will use the main framebuffer for intermediate rendering in some cases.
	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*gl,
			feedback_paint_device_view_projection,
			// Using device-independent pixels (eg, widget dimensions)...
			feedback_paint_device.width(),
			feedback_paint_device.height(),
			map_canvas_paint_device.width(),
			map_canvas_paint_device.height());
}


boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::MapCanvas::get_camera_viewpoint() const
{
#if 1
	return GPlatesMaths::make_lat_lon_point(d_map_camera.get_look_at_position());
#else
	const GPlatesGui::MapTransform::point_type &centre_of_viewport = d_map_transform.get_centre_of_viewport();
	double x_pos = centre_of_viewport.x();
	double y_pos = centre_of_viewport.y();

	// This stores the x screen coordinate, for comparison with the forward-transformed longitude.  
	double screen_x = x_pos;

	// Tolerance for comparing forward transformed longitude with screen longitude. 
	double tolerance = 1.;

	boost::optional<GPlatesMaths::LatLonPoint> llp = map().projection().inverse_transform(x_pos,y_pos);
		
	if (!llp)
	{
		return boost::none;
	}

	// Forward transform the lat-lon point and see where it would end up. 
	double x_scene_pos = llp->longitude();
	double y_scene_pos = llp->latitude();
	map().projection().forward_transform(x_scene_pos,y_scene_pos);

	// If we don't end up at the same point, we're off the map. 
	if (std::fabs(x_scene_pos - screen_x) > tolerance)
	{
		return boost::none;
	}
		
	return llp;
#endif
}


void
GPlatesQtWidgets::MapCanvas::set_camera_viewpoint(
		const GPlatesMaths::LatLonPoint &camera_viewpoint)
{
#if 1
	d_map_camera.move_look_at_position(
			GPlatesMaths::make_point_on_sphere(camera_viewpoint));
#else
	// Convert the llp to canvas coordinates.
	double x_pos = camera_viewpoint.longitude();
	double y_pos = camera_viewpoint.latitude();

	try
	{
		map().projection().forward_transform(x_pos,y_pos);
	}
	catch(GPlatesGui::ProjectionException &e)
	{
		qWarning() << "Caught exception converting lat-long to scene coordinates.";
		qWarning() << e;
	}

	// Centre the view on this point.
	d_map_transform.set_centre_of_viewport(
			GPlatesGui::MapTransform::point_type(x_pos, y_pos));
#endif
}


void
GPlatesQtWidgets::MapCanvas::set_orientation(
		const GPlatesMaths::Rotation &rotation
		/*bool should_emit_external_signal */)
{
	GPlatesMaths::LatLonPoint llp(0,0);
	GPlatesMaths::PointOnSphere centre = GPlatesMaths::make_point_on_sphere(llp);

	GPlatesMaths::Rotation rev = rotation.get_reverse();

	GPlatesMaths::PointOnSphere desired_centre = rev * centre;
	GPlatesMaths::LatLonPoint desired_llp = GPlatesMaths::make_lat_lon_point(desired_centre);


	// Convert the llp to canvas coordinates.
	double x_pos = desired_llp.longitude();
	double y_pos = desired_llp.latitude();

	try
	{
		map().projection().forward_transform(x_pos, y_pos);
	}
	catch (GPlatesGui::ProjectionException &e)
	{
		qWarning() << "Caught exception converting lat-long to scene coordinates.";
		qWarning() << e;
	}

	// Centre the view on this point.
	d_map_transform.set_centre_of_viewport(
		GPlatesGui::MapTransform::point_type(x_pos, y_pos)
		/*should_emit_external_signal */);

}


void
GPlatesQtWidgets::MapCanvas::update_mouse_pointer_pos(
		QMouseEvent *mouse_event)
{
	d_mouse_pointer_screen_pos = mouse_event->pos();

	handle_mouse_pointer_pos_change();
}


void
GPlatesQtWidgets::MapCanvas::handle_mouse_pointer_pos_change()
{
	boost::optional<GPlatesMaths::LatLonPoint> llp = mouse_pointer_llp();

	const bool is_on_surface = static_cast<bool>(llp);

	Q_EMIT mouse_pointer_position_changed(llp, is_on_surface);
}


boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::MapCanvas::mouse_pointer_llp()
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


	llp = map().projection().inverse_transform(x_mouse_pos,y_mouse_pos);

	if (!llp)
	{
		return boost::none;
	}
		
	// Forward transform the lat-lon point and see where it would end up. 
	double x_scene_pos = llp->longitude();
	double y_scene_pos = llp->latitude();
	map().projection().forward_transform(x_scene_pos,y_scene_pos);

	// If we don't end up at the same point, we're off the map. 

	if (std::fabs(x_scene_pos - screen_x) > tolerance)
	{
		return boost::none;
	}

	// If we reach here, we should be on the map, with valid lat,lon.
	return llp;
	
}


QPointF
GPlatesQtWidgets::MapCanvas::mouse_pointer_scene_coords()
{
	return mapToScene(d_mouse_pointer_screen_pos);
}


bool
GPlatesQtWidgets::MapCanvas::mouse_pointer_is_on_surface()
{
	return static_cast<bool>(mouse_pointer_llp());
}


void
GPlatesQtWidgets::MapCanvas::move_camera(
		double dx,
		double dy)
{
	// Position of new centre in window coordinates.
	double win_x = static_cast<double>(width()) / 2.0 + dx;
	double win_y = static_cast<double>(height()) / 2.0 + dy;

	// Turn that into scene coordinates.
	double scene_x, scene_y;
	transform().inverted().map(win_x, win_y, &scene_x, &scene_y);
	d_map_transform.set_centre_of_viewport(
			GPlatesGui::MapTransform::point_type(scene_x, scene_y));
}


void
GPlatesQtWidgets::MapCanvas::move_camera_up()
{
	// This translation will be zoom-dependent, as it's based on view coordinates. 
	move_camera(0, -5);
}


void
GPlatesQtWidgets::MapCanvas::move_camera_down()
{
	// See comments under "move_camera_up" above. 
	move_camera(0, 5);
}


void
GPlatesQtWidgets::MapCanvas::move_camera_left()
{
	// See comments under "move_camera_up" above. 
	move_camera(-5, 0);
}


void
GPlatesQtWidgets::MapCanvas::move_camera_right()
{
	// See comments under "move_camera_up" above.
	move_camera(5, 0);
}


void
GPlatesQtWidgets::MapCanvas::rotate_camera_clockwise()
{
	d_map_transform.rotate(-5.0);
}


void
GPlatesQtWidgets::MapCanvas::rotate_camera_anticlockwise()
{
	d_map_transform.rotate(5.0);
}


void
GPlatesQtWidgets::MapCanvas::reset_camera_orientation()
{
	d_map_transform.set_rotation(0);
}


float
GPlatesQtWidgets::MapCanvas::calculate_scale(
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels,
		int map_canvas_paint_device_width_in_device_independent_pixels,
		int map_canvas_paint_device_height_in_device_independent_pixels)
{
	// Note that we use regular device *independent* sizes not high-DPI device pixels
	// (ie, not using device pixel ratio) to calculate scale because font sizes, etc, are
	// based on these coordinates (it's only OpenGL, really, that deals with device pixels).
	const int paint_device_dimension = (std::min)(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels);
	const int min_viewport_dimension = (std::min)(
			map_canvas_paint_device_width_in_device_independent_pixels,
			map_canvas_paint_device_height_in_device_independent_pixels);

	// If paint device is larger than the viewport then don't scale - this avoids having
	// too large point/line sizes when exporting large screenshots.
	if (paint_device_dimension >= min_viewport_dimension)
	{
		return 1.0f;
	}

	// This is useful when rendering the small colouring previews - avoids too large point/line sizes.
	return static_cast<float>(paint_device_dimension) / static_cast<float>(min_viewport_dimension);
}
