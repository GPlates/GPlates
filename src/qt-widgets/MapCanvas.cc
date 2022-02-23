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

#include <QApplication>
#include <QDebug>
#include <QGLWidget>
#include <QGraphicsView>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>

#include "MapCanvas.h"
#include "MapView.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
#include "global/PreconditionViolationError.h"

#include "gui/Map.h"
#include "gui/MapProjection.h"
#include "gui/MapTransform.h"
#include "gui/TextOverlay.h"
#include "gui/VelocityLegendOverlay.h"

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
		/**
		 * Gets the equivalent OpenGL model-view matrix from the 2D world transform.
		 */
		void
		get_model_view_matrix_from_2D_world_transform(
				GPlatesOpenGL::GLMatrix &model_view_matrix,
				const QTransform &world_transform)
		{
			const GLdouble model_view_matrix_array[16] =
			{
				world_transform.m11(), world_transform.m12(),        0, world_transform.m13(),
				world_transform.m21(), world_transform.m22(),        0, world_transform.m23(),
				                    0,                     0,        1,                     0,
				 world_transform.dx(),  world_transform.dy(),        0, world_transform.m33()
			};
			model_view_matrix.gl_load_matrix(model_view_matrix_array);
		}


		/**
		 * Gets the orthographic OpenGL projection matrix from the specified dimensions.
		 */
		GPlatesOpenGL::GLMatrix
		get_ortho_projection_matrix_from_dimensions(
				int scene_width,
				int scene_height)
		{
			GPlatesOpenGL::GLMatrix projection_matrix_scene;

			// NOTE: Use bottom=height instead of top=height inverts the y-axis which
			// converts from Qt coordinate system to OpenGL coordinate system.
			projection_matrix_scene.gl_ortho(0, scene_width, scene_height, 0, -999999, 999999);

			return projection_matrix_scene;
		}
	}
}


GPlatesQtWidgets::MapCanvas::MapCanvas(
		GPlatesPresentation::ViewState &view_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		MapView *map_view_ptr,
		QGLWidget *gl_widget,
		const GPlatesOpenGL::GLContext::non_null_ptr_type &gl_context,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		GPlatesGui::ViewportZoom &viewport_zoom,
		QWidget *parent_) :
	QGraphicsScene(parent_),
	d_view_state(view_state),
	d_map_view_ptr(map_view_ptr),
	d_gl_context(gl_context),
	d_make_context_current(*d_gl_context),
	d_off_screen_render_target_dimension(OFF_SCREEN_RENDER_TARGET_DIMENSION),
	d_text_overlay(new GPlatesGui::TextOverlay(view_state.get_application_state())),
	d_velocity_legend_overlay(new GPlatesGui::VelocityLegendOverlay()),
	d_map(
			view_state,
			gl_visual_layers,
			rendered_geometry_collection,
			view_state.get_visual_layers(),
			viewport_zoom,
			gl_widget->devicePixelRatio()),
	d_rendered_geometry_collection(&rendered_geometry_collection)
{
	// Do some OpenGL initialisation.
	// Because of 'd_make_context_current' we know the OpenGL context is currently active.
	initializeGL(gl_widget);

	// Give the scene a rectangle that's big enough to guarantee that the map view,
	// even after rotations and translations, won't go outside these boundaries.
	// (Note that the centre of the map, in scene coordinates, is constrained by
	// the MapTransform class.)
	static const int FACTOR = 3;
	setSceneRect(QRect(
				GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_X * FACTOR,
				GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_Y * FACTOR,
				(GPlatesGui::MapTransform::MAX_CENTRE_OF_VIEWPORT_X -
				 GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_X) * FACTOR,
				(GPlatesGui::MapTransform::MAX_CENTRE_OF_VIEWPORT_Y -
				 GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_Y) * FACTOR));

	QObject::connect(d_rendered_geometry_collection,
		SIGNAL(collection_was_updated(
			GPlatesViewOperations::RenderedGeometryCollection &,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
		this,
		SLOT(update_canvas()));
}

GPlatesQtWidgets::MapCanvas::~MapCanvas()
{  }


void 
GPlatesQtWidgets::MapCanvas::initializeGL(
		QGLWidget *gl_widget) 
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
GPlatesQtWidgets::MapCanvas::drawBackground(
		QPainter *painter,
		const QRectF &/*exposed_rect*/)
{
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
}

void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	update();
}

void
GPlatesQtWidgets::MapCanvas::set_viewport_transform(
		const QTransform &viewport_transform)
{
	d_viewport_transform = viewport_transform;
}

QImage
GPlatesQtWidgets::MapCanvas::render_to_qimage(
		QPaintDevice &map_canvas_paint_device,
		const QTransform &viewport_transform,
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
		QPaintDevice &map_canvas_paint_device,
		const QTransform &viewport_transform,
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
	const double feedback_paint_device_aspect_ratio =
			double(feedback_paint_device_pixel_width) / feedback_paint_device_pixel_height;

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
