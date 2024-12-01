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

#include <QApplication>
#include <QDebug>
#include <QGLWidget>
#include <QGraphicsView>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>
#include <opengl/OpenGL.h>

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

#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLRenderTarget.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLViewport.h"

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
		void
		get_ortho_projection_matrices_from_dimensions(
				GPlatesOpenGL::GLMatrix &projection_matrix_scene,
				GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
				int scene_width,
				int scene_height)
		{
			projection_matrix_scene.gl_load_identity();
			projection_matrix_text_overlay.gl_load_identity();

			// NOTE: Use bottom=height instead of top=height inverts the y-axis which
			// converts from Qt coordinate system to OpenGL coordinate system.
			projection_matrix_scene.gl_ortho(0, scene_width, scene_height, 0, -999999, 999999);

			// However the text overlay doesn't need this y-inversion.
			// TODO: Sort out the need for a y-inversion above by fixing the world transform in MapView.
			projection_matrix_text_overlay.gl_ortho(0, scene_width, 0, scene_height, -999999, 999999);
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
		const GPlatesGui::ColourScheme::non_null_ptr_type &colour_scheme,
		QWidget *parent_) :
	QGraphicsScene(parent_),
	d_view_state(view_state),
	d_map_view_ptr(map_view_ptr),
	d_gl_context(gl_context),
	d_make_context_current(*d_gl_context),
	d_text_overlay(new GPlatesGui::TextOverlay(view_state.get_application_state())),
	d_velocity_legend_overlay(new GPlatesGui::VelocityLegendOverlay()),
	d_map(
			view_state,
			gl_visual_layers,
			rendered_geometry_collection,
			view_state.get_visual_layers(),
			viewport_zoom,
			colour_scheme,
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

	// Create the off-screen context that's used when rendering OpenGL outside the paint event.
	d_gl_off_screen_context =
			GPlatesOpenGL::GLOffScreenContext::create(
					GPlatesOpenGL::GLOffScreenContext::QGLWidgetContext(gl_widget, d_gl_context));

	// Get a renderer - it'll be used to initialise some OpenGL objects.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	// Pass in the viewport of the window currently attached to the OpenGL context.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

	// NOTE: We don't actually 'glClear()' the framebuffer because:
	//  1) It's not necessary to do this before calling Map::initialiseGL(), and
	//  2) It appears to generate an OpenGL error when GPlatesOpenGL::GLUtils::assert_no_gl_errors()
	//     is next called ("invalid framebuffer operation") on a Mac-mini system (all other systems
	//     seem fine) - it's possibly due to the main framebuffer not set up correctly yet.

	// Initialise those parts of map that require a valid OpenGL context to be bound.
	d_map.initialiseGL(*renderer);
}


GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
		const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
		const GPlatesGui::Colour &clear_colour,
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels,
		int map_canvas_paint_device_width_in_device_independent_pixels,
		int map_canvas_paint_device_height_in_device_independent_pixels)
{
	PROFILE_FUNC();

	// Clear the colour buffer of the framebuffer.
	// NOTE: We leave the depth clears to class Map.
	renderer.gl_clear_color(clear_colour.red(), clear_colour.green(), clear_colour.blue(), clear_colour.alpha());
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// Set the projection matrix for the scene.
	renderer.gl_load_matrix(GL_PROJECTION, projection_matrix_scene);

	// Render the map.
	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			map_canvas_paint_device_width_in_device_independent_pixels,
			map_canvas_paint_device_height_in_device_independent_pixels);
	const double device_independent_pixel_to_map_space_ratio =
			d_map_view_ptr->get_device_independent_pixel_to_map_space_ratio(
					paint_device_width_in_device_independent_pixels,
					paint_device_height_in_device_independent_pixels,
					viewport_zoom_factor);

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
	const cache_handle_type frame_cache_handle = d_map.paint(
			renderer,
			viewport_zoom_factor,
			device_independent_pixel_to_map_space_ratio,
			scale);

	// The text overlay is rendered in screen window coordinates (ie, no model-view transform needed).
	renderer.gl_load_matrix(GL_MODELVIEW, GPlatesOpenGL::GLMatrix::IDENTITY);
	// Set the projection matrix for the text overlay (it's inverted compared to the scene transform).
	renderer.gl_load_matrix(GL_PROJECTION, projection_matrix_text_overlay);

	// Draw the optional text overlay.
	// We use the paint device dimensions (and not the canvas dimensions) in case the paint device
	// is not the canvas (eg, when rendering to a larger dimension SVG paint device).
	d_text_overlay->paint(
			renderer,
			d_view_state.get_text_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			scale);

	d_velocity_legend_overlay->paint(
			renderer,
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

	// Create a render for all our OpenGL rendering work.
	// Note that nothing will happen until we enter a rendering scope.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	//
	// By default the current render target of 'renderer' is the main frame buffer (of the window).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, *painter);

	// Get the model-view matrix.
	GPlatesOpenGL::GLMatrix model_view_matrix;
	get_model_view_matrix_from_2D_world_transform(model_view_matrix, d_viewport_transform);

	// Set the model-view matrix on the renderer.
	renderer->gl_load_matrix(GL_MODELVIEW, model_view_matrix);

	// The QPainter's paint device.
	const QPaintDevice *qpaint_device = painter->device();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			qpaint_device,
			GPLATES_ASSERTION_SOURCE);

	// Get the projection matrix for the QPainter's paint device.
	GPlatesOpenGL::GLMatrix projection_matrix_scene;
	GPlatesOpenGL::GLMatrix projection_matrix_text_overlay;
	get_ortho_projection_matrices_from_dimensions(
			projection_matrix_scene,
			projection_matrix_text_overlay,
			// Using device-independent pixels (eg, widget dimensions)...
			qpaint_device->width(),
			qpaint_device->height());

	// Clear colour buffer of the main framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This appears to be needed on
	// Mac with Qt5 (alpha=0 is fine on Qt5 Windows/Ubuntu, and on Qt4 for all platforms). Perhaps because
	// QGLWidget rendering (on Qt5 Mac) is first done to a framebuffer object which is then blended into the
	// window framebuffer (where having a source alpha of zero would result in the black background not showing).
	// Or, more likely, maybe a framebuffer object is used on all platforms but the window framebuffer is
	// white on Mac but already black on Windows/Ubuntu.
	const GPlatesGui::Colour clear_colour(0, 0, 0, 1);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			projection_matrix_scene,
			projection_matrix_text_overlay,
			clear_colour,
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
		const QSize &image_size_in_device_independent_pixels,
		const GPlatesGui::Colour &image_clear_colour)
{
	// Set up a QPainter to help us with OpenGL text rendering.
	QPainter painter(&map_canvas_paint_device);

	// Start a render scope.
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// Where possible, force drawing to an off-screen render target.
	// It seems making the OpenGL context current is not enough to prevent Snow Leopard systems
	// with ATI graphics from hanging/crashing - this appears to be due to modifying/accessing the
	// main/default framebuffer (which is intimately tied to the windowing system).
	// Using an off-screen render target appears to avoid this issue.
	//
	// Set the off-screen render target to the size of the QGLWidget main framebuffer.
	// This is because we use QPainter to render text and it sets itself up using the dimensions
	// of the main framebuffer - if we change the dimensions then the text is rendered incorrectly.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLOffScreenContext::RenderScope off_screen_render_scope(
			*d_gl_off_screen_context.get(),
			// Convert from widget size to device pixels (used by OpenGL)...
			map_canvas_paint_device.width() * map_canvas_paint_device.devicePixelRatio(),
			map_canvas_paint_device.height() * map_canvas_paint_device.devicePixelRatio(),
			painter);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = off_screen_render_scope.get_renderer();


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

	// Fill the image with the clear colour in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(image_clear_colour).rgba());

	// Get the frame buffer dimensions (in device pixels).
	const std::pair<unsigned int/*width*/, unsigned int/*height*/> frame_buffer_dimensions =
			renderer->get_current_frame_buffer_dimensions();

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int tile_border = 10;
	// Set up for rendering the scene into tiles.
	// The tile render target dimensions match the frame buffer dimensions.
	GPlatesOpenGL::GLTileRender tile_render(
			frame_buffer_dimensions.first/*tile_render_target_width*/,
			frame_buffer_dimensions.second/*tile_render_target_height*/,
			GPlatesOpenGL::GLViewport(
					0,
					0,
					// Use image size in device pixels (used by OpenGL)...
					image_size_in_device_pixels.width(),
					image_size_in_device_pixels.height())/*destination_viewport*/,
			tile_border);

	// Get the model-view matrix from the 2D world transform.
	GPlatesOpenGL::GLMatrix model_view_matrix;
	get_model_view_matrix_from_2D_world_transform(
			model_view_matrix,
			viewport_transform);

	// Set the model-view matrix on the renderer.
	renderer->gl_load_matrix(GL_MODELVIEW, model_view_matrix);

	// Get the projection matrix for the image dimensions.
	// It'll get adjusted per tile (that the scene is rendered to).
	GPlatesOpenGL::GLMatrix projection_matrix_scene;
	GPlatesOpenGL::GLMatrix projection_matrix_text_overlay;
	get_ortho_projection_matrices_from_dimensions(
			projection_matrix_scene,
			projection_matrix_text_overlay,
			// Using device-independent pixels (eg, widget dimensions)...
			image_size_in_device_independent_pixels.width(),
			image_size_in_device_independent_pixels.height());

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (tile_render.first_tile(); !tile_render.finished(); tile_render.next_tile())
	{
		// Render the scene to the current tile.
		// Hold onto the previous frame's cached resources *while* generating the current frame.
		const cache_handle_type tile_cache_handle = render_scene_tile_into_image(
				*renderer,
				tile_render,
				image_clear_colour,
				image,
				projection_matrix_scene,
				projection_matrix_text_overlay,
				map_canvas_paint_device);
		frame_cache_handle->push_back(tile_cache_handle);
	}

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = frame_cache_handle;

	return image;
}

GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene_tile_into_image(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLTileRender &tile_render,
		const GPlatesGui::Colour &image_clear_colour,
		QImage &image,
		const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
		const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
		const QPaintDevice &map_canvas_paint_device)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	GPlatesOpenGL::GLViewport current_tile_render_target_viewport;
	tile_render.get_tile_render_target_viewport(current_tile_render_target_viewport);

	GPlatesOpenGL::GLViewport current_tile_render_target_scissor_rect;
	tile_render.get_tile_render_target_scissor_rectangle(current_tile_render_target_scissor_rect);

	// Mask off rendering outside the current tile region in case the tile is smaller than the
	// render target. Note that the tile's viewport is slightly larger than the tile itself
	// (the scissor rectangle) in order that fat points and wide lines just outside the tile
	// have pixels rasterised inside the tile (the projection transform has also been expanded slightly).
	//
	// This includes 'gl_clear()' calls which clear the entire framebuffer.
	renderer.gl_enable(GL_SCISSOR_TEST);
	renderer.gl_scissor(
			current_tile_render_target_scissor_rect.x(),
			current_tile_render_target_scissor_rect.y(),
			current_tile_render_target_scissor_rect.width(),
			current_tile_render_target_scissor_rect.height());
	renderer.gl_viewport(
			current_tile_render_target_viewport.x(),
			current_tile_render_target_viewport.y(),
			current_tile_render_target_viewport.width(),
			current_tile_render_target_viewport.height());

	//
	// Adjust the various projection transforms for the current tile.
	//

	const GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type tile_projection_transform =
			tile_render.get_tile_projection_transform();
	const GPlatesOpenGL::GLMatrix &tile_projection_matrix = tile_projection_transform->get_matrix();

	// The scene projection matrix adjusted for the current tile.
	GPlatesOpenGL::GLMatrix tile_projection_matrix_scene(tile_projection_matrix);
	tile_projection_matrix_scene.gl_mult_matrix(projection_matrix_scene);

	// The text overlay projection matrix adjusted for the current tile.
	GPlatesOpenGL::GLMatrix tile_projection_matrix_text_overlay(tile_projection_matrix);
	tile_projection_matrix_text_overlay.gl_mult_matrix(projection_matrix_text_overlay);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			renderer,
			tile_projection_matrix_scene,
			tile_projection_matrix_text_overlay,
			image_clear_colour,
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
	tile_render.get_tile_source_viewport(current_tile_source_viewport);

	GPlatesOpenGL::GLViewport current_tile_destination_viewport;
	tile_render.get_tile_destination_viewport(current_tile_destination_viewport);

	GPlatesOpenGL::GLImageUtils::copy_rgba8_frame_buffer_into_argb32_qimage(
			renderer,
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
	// Note that the OpenGL rendering gets redirected into the QPainter (using OpenGL feedback) and
	// ends up in the feedback paint device.
	QPainter feedback_painter(&feedback_paint_device);

	// Start a render scope.
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// Where possible, force drawing to an off-screen render target.
	// It seems making the OpenGL context current is not enough to prevent Snow Leopard systems
	// with ATI graphics from hanging/crashing - this appears to be due to modifying/accessing the
	// main/default framebuffer (which is intimately tied to the windowing system).
	// Using an off-screen render target appears to avoid this issue.
	//
	// Set the off-screen render target to the size of the QGLWidget main framebuffer.
	// This is because we use QPainter to render text and it sets itself up using the dimensions
	// of the main framebuffer - actually that doesn't apply when painting to a device other than
	// the main framebuffer (in our case the feedback paint device, eg, SVG) - but we'll leave the
	// restriction in for now.
	// TODO: change to a larger size render target for more efficient rendering.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLOffScreenContext::RenderScope off_screen_render_scope(
			*d_gl_off_screen_context.get(),
			// Convert from widget size to device pixels (used by OpenGL)...
			map_canvas_paint_device.width() * map_canvas_paint_device.devicePixelRatio(),
			map_canvas_paint_device.height() * map_canvas_paint_device.devicePixelRatio(),
			feedback_painter,
			false/*paint_device_is_framebuffer*/);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = off_screen_render_scope.get_renderer();

	// Set the viewport (and scissor rectangle) to the size of the feedback paint device instead
	// of the map canvas because OpenGL feedback uses the viewport to generate projected vertices.
	// Also text rendering uses the viewport.
	// And we want all this to be positioned correctly within the feedback paint device.
	renderer->gl_viewport(0, 0,
			// Convert from widget size to device pixels (used by OpenGL)...
			feedback_paint_device.width() * feedback_paint_device.devicePixelRatio(),
			feedback_paint_device.height() * feedback_paint_device.devicePixelRatio());
	renderer->gl_scissor(0, 0,
			// Convert from widget size to device pixels (used by OpenGL)...
			feedback_paint_device.width() * feedback_paint_device.devicePixelRatio(),
			feedback_paint_device.height() * feedback_paint_device.devicePixelRatio());

	// Get the model-view matrix from the 2D world transform.
	GPlatesOpenGL::GLMatrix model_view_matrix;
	get_model_view_matrix_from_2D_world_transform(
			model_view_matrix,
			viewport_transform);

	// Set the model-view matrix on the renderer.
	renderer->gl_load_matrix(GL_MODELVIEW, model_view_matrix);

	// Get the projection matrix for the feedback paint device.
	GPlatesOpenGL::GLMatrix projection_matrix_scene;
	GPlatesOpenGL::GLMatrix projection_matrix_text_overlay;
	get_ortho_projection_matrices_from_dimensions(
			projection_matrix_scene,
			projection_matrix_text_overlay,
			// Using device-independent pixels (eg, widget dimensions)...
			feedback_paint_device.width(),
			feedback_paint_device.height());

	// Clear colour buffer of the framebuffer (set to transparent black).
	const GPlatesGui::Colour clear_colour(0, 0, 0, 0);

	// Render the scene to the feedback paint device.
	// This will use the main framebuffer for intermediate rendering in some cases.
	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			projection_matrix_scene,
			projection_matrix_text_overlay,
			clear_colour,
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
