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
#include <QGLPixelBuffer>
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
#include "gui/RenderSettings.h"

#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLRenderTarget.h"
#include "opengl/GLSaveRestoreFrameBuffer.h"
#include "opengl/GLScreenRenderTarget.h"
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
			// Get the model-view matrix from the QPainter.
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
				int width,
				int height)
		{
			projection_matrix_scene.gl_load_identity();
			projection_matrix_text_overlay.gl_load_identity();

			// NOTE: Use bottom=height instead of top=height inverts the y-axis which
			// converts from Qt coordinate system to OpenGL coordinate system.
			projection_matrix_scene.gl_ortho(0, width, height, 0, -999999, 999999);

			// However the text overlay doesn't need this y-inversion.
			// TODO: Sort out the need for a y-inversion above by fixing the world transform in MapView.
			projection_matrix_text_overlay.gl_ortho(0, width, 0, height, -999999, 999999);
		}
	}
}


GPlatesQtWidgets::MapCanvas::MapCanvas(
		GPlatesPresentation::ViewState &view_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		MapView *map_view_ptr,
		const GPlatesOpenGL::GLContext::non_null_ptr_type &gl_context,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		GPlatesGui::RenderSettings &render_settings,
		GPlatesGui::ViewportZoom &viewport_zoom,
		const GPlatesGui::ColourScheme::non_null_ptr_type &colour_scheme,
		QWidget *parent_) :
	QGraphicsScene(parent_),
	d_view_state(view_state),
	d_map_view_ptr(map_view_ptr),
	d_gl_context(gl_context),
	d_make_context_current(*d_gl_context),
	d_text_overlay(new GPlatesGui::TextOverlay(view_state.get_application_state())),
	d_map(
			view_state,
			gl_visual_layers,
			rendered_geometry_collection,
			view_state.get_visual_layers(),
			render_settings,
			viewport_zoom,
			colour_scheme),
	d_rendered_geometry_collection(&rendered_geometry_collection)
{
	// Do some OpenGL initialisation.
	// Because of 'd_make_context_current' we know the OpenGL context is currently active.
	initializeGL();

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

	// Update if RenderSettings gets changed
	QObject::connect(
			&render_settings,
			SIGNAL(settings_changed()),
			this,
			SLOT(update_canvas()));
}

GPlatesQtWidgets::MapCanvas::~MapCanvas()
{  }

void 
GPlatesQtWidgets::MapCanvas::initializeGL() 
{
	// Initialise our context-like object first.
	d_gl_context->initialise();

	// Beginning rendering so we can clear the framebuffer.
	// By default the current render target of 'renderer' is the main frame buffer (of the window).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	// Pass in the viewport of the window currently attached to the OpenGL context.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(
			*renderer,
			// Pass in the viewport currently set by QGraphicsView...
			d_map_view_ptr->width(),
			d_map_view_ptr->height());

	// Initialise those parts of map that require a valid OpenGL context to be bound.
	d_map.initialiseGL(*renderer);
}


GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
		const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay,
		int paint_device_width,
		int paint_device_height)
{
	PROFILE_FUNC();

	// Set the projection matrix for the scene.
	renderer.gl_load_matrix(GL_PROJECTION, projection_matrix_scene);

	// Render the map.
	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale();
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
	const cache_handle_type frame_cache_handle = d_map.paint(renderer, viewport_zoom_factor, scale);

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
			paint_device_width,
			paint_device_height,
			scale);

	return frame_cache_handle;
}


void
GPlatesQtWidgets::MapCanvas::drawBackground(
		QPainter *painter,
		const QRectF &/*exposed_rect*/)
{
	// We use the QPainter's world transform to set our OpenGL model-view and projection matrices.
	// And we restore the QPainter's transform after our rendering because we use it for text
	// rendering which sets its transform to identity.
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
	// This also sets the main frame buffer dimensions to the paint device dimensions.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, *painter);

	// Get the model-view matrix from the QPainter's 2D world transform.
	GPlatesOpenGL::GLMatrix model_view_matrix;
	get_model_view_matrix_from_2D_world_transform(
			model_view_matrix,
			qpainter_world_transform);

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
			qpaint_device->width(),
			qpaint_device->height());

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			projection_matrix_scene,
			projection_matrix_text_overlay,
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

QImage
GPlatesQtWidgets::MapCanvas::render_to_qimage(
		QGLWidget *map_canvas_paint_device,
		const QTransform &viewport_transform,
		const QSize &image_size)
{
	// The image to render the scene into.
	QImage image(image_size, QImage::Format_ARGB32);
	if (image.isNull())
	{
		// Most likely a memory allocation failure - return the null image.
		return QImage();
	}

	// Fill the image with transparent black in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(0,0,0,0).rgba());

	//
	// Rendering.
	//

	const QSize frame_buffer_dimensions(
			map_canvas_paint_device->width(),
			map_canvas_paint_device->height());

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int tile_border = 10;
	// Set up for rendering the scene into tiles.
	// The tile render target dimensions match the frame buffer dimensions.
	GPlatesOpenGL::GLTileRender tile_render(
			frame_buffer_dimensions.width()/*tile_render_target_width*/,
			frame_buffer_dimensions.height()/*tile_render_target_height*/,
			GPlatesOpenGL::GLViewport(
					0,
					0,
					image_size.width(),
					image_size.height())/*destination_viewport*/,
			tile_border);

	GPlatesOpenGL::GLContext::non_null_ptr_type render_context = d_gl_context;
	render_context->make_current();

	// Set up a QPainter to help us with OpenGL text rendering.
	QPainter painter(map_canvas_paint_device);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = render_context->create_renderer();

	// Start a begin_render/end_render scope.
	//
	// By default the current render target of 'renderer' is the main frame buffer (of the window).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	// This also sets the main frame buffer dimensions to the paint device dimensions.
	renderer->begin_render(painter);

	// In case we need to preserve the main frame buffer (if not using frame buffer object or pbuffer).
	// We never need to preserve the depth/stencil buffer though (they get cleared before every render).
	GPlatesOpenGL::GLSaveRestoreFrameBuffer save_restore_main_framebuffer(
			renderer->get_capabilities(),
			tile_render.get_max_tile_render_target_width(),
			tile_render.get_max_tile_render_target_height());

	// Where possible, force drawing to an off-screen render target.
	// It seems making the OpenGL context current is not enough to prevent Snow Leopard systems
	// with ATI graphics from hanging/crashing - this appears to be due to modifying/accessing the
	// main/default framebuffer (which is intimately tied to the windowing system).
	// Using an off-screen render target appears to avoid this issue.
	boost::optional<GPlatesOpenGL::GLScreenRenderTarget::shared_ptr_type> screen_render_target =
			render_context->get_shared_state()->acquire_screen_render_target(
					*renderer,
					GL_RGBA8/*texture_internalformat*/,
					true/*include_depth_buffer*/,
					true/*include_stencil_buffer*/);

	// Begin rendering to the off-screen target.
	boost::optional<QGLPixelBuffer> qgl_pixel_buffer;
	if (screen_render_target)
	{
		// Begin rendering to the screen render target.
		//
		// Set the off-screen render target to the size of the main framebuffer.
		// This is because we use QPainter to render text and it sets itself up using the dimensions
		// of the main framebuffer - if we change the dimensions then the text is rendered incorrectly.
		screen_render_target.get()->begin_render(
				*renderer,
				frame_buffer_dimensions.width(),
				frame_buffer_dimensions.height());
	}
	// If we can't get a screen render target (GL_EXT_framebuffer_object) then attempt to
	// obtain a pbuffer off-screen OpenGL context. We normally use either a frame buffer object or
	// the main frame buffer - however in this situation, as mentioned above, we need to avoid
	// the main frame buffer if possible.
	else if (QGLPixelBuffer::hasOpenGLPbuffers())
	{
		// Create a QGLPixelBuffer.
		qgl_pixel_buffer = boost::in_place(
				frame_buffer_dimensions.width(),
				frame_buffer_dimensions.height(),
				// Use the same format as the current rendering context...
				render_context->get_qgl_format(),
				// It's important to share textures, etc, with our regular OpenGL context...
				map_canvas_paint_device/*shareWidget*/);

		// Switch rendering contexts to the QGLPixelBuffer.
		renderer->end_render();
		render_context = GPlatesOpenGL::GLContext::create(
				boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
						new GPlatesOpenGL::GLContextImpl::QGLPixelBufferImpl(qgl_pixel_buffer.get())),
				// Share textures, etc, with the current render context...
				*render_context);
		render_context->make_current();
		renderer = render_context->create_renderer();
		renderer->begin_render(painter);
	}
	else if (!map_canvas_paint_device->doubleBuffer())
	{
		// We only have a front buffer so we need to save and restore the main (colour)
		// framebuffer in order not to disturb the display of the globe canvas painted widget.
		save_restore_main_framebuffer.save(*renderer);
	}
	// ...else we have a double buffer main framebuffer and we are rendering to the back buffer.
	// So the front buffer (which is being displayed) won't get disturbed. And when this
	// widget paints itself it will clear and re-draw the back buffer and then swap it so
	// it becomes the front buffer.
	// So for these reasons we do not need to save and restore the main framebuffer with double-buffering.

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
			image_size.width(),
			image_size.height());

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
				image,
				projection_matrix_scene,
				projection_matrix_text_overlay);
		frame_cache_handle->push_back(tile_cache_handle);
	}

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = frame_cache_handle;

	// End rendering to the off-screen target.
	if (screen_render_target)
	{
		screen_render_target.get()->end_render(*renderer);
	}
	else if (!map_canvas_paint_device->doubleBuffer())
	{
		save_restore_main_framebuffer.restore(*renderer);
	}

	return image;
}

GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene_tile_into_image(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLTileRender &tile_render,
		QImage &image,
		const GPlatesOpenGL::GLMatrix &projection_matrix_scene,
		const GPlatesOpenGL::GLMatrix &projection_matrix_text_overlay)
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

	// The scene projection matrix adjusted for the current tile.
	GPlatesOpenGL::GLMatrix tile_projection_matrix_scene(tile_projection_transform->get_matrix());
	tile_projection_matrix_scene.gl_mult_matrix(projection_matrix_scene);

	// The text overlay projection matrix adjusted for the current tile.
	GPlatesOpenGL::GLMatrix tile_projection_matrix_text_overlay(tile_projection_transform->get_matrix());
	tile_projection_matrix_text_overlay.gl_mult_matrix(projection_matrix_text_overlay);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			renderer,
			tile_projection_matrix_scene,
			tile_projection_matrix_text_overlay,
			image.width(),
			image.height());

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
		QGLWidget *map_canvas_paint_device,
		const QTransform &viewport_transform,
		QPaintDevice &feedback_paint_device)
{
	// Make sure our OpenGL context is the currently active context.
	d_gl_context->make_current();

	// Note that the OpenGL rendering gets redirected into the QPainter (using OpenGL feedback) and
	// ends up in the feedback paint device.
	QPainter feedback_painter(&feedback_paint_device);

	// Create a render for all our OpenGL rendering work.
	// Note that nothing will happen until we enter a rendering scope.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	//
	// By default the current render target of 'renderer' is the main frame buffer (of the map canvas window).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(
			*renderer,
			feedback_painter,
			// The map canvas is not necessarily the same size as the feedback paint device...
			std::make_pair(
					static_cast<unsigned int>(map_canvas_paint_device->width()),
					static_cast<unsigned int>(map_canvas_paint_device->height())));

	// This should be the same as 'map_canvas_paint_device->width()' and 'map_canvas_paint_device->height()'.
	const std::pair<unsigned int/*width*/, unsigned int/*height*/> frame_buffer_dimensions =
			renderer->get_current_frame_buffer_dimensions();

	// Set the viewport (and scissor rectangle) to the size of the feedback paint device instead
	// of the map canvas because OpenGL feedback uses the viewport to generate projected vertices.
	// Also text rendering uses the viewport.
	// And we want all this to be positioned correctly within the feedback paint device.
	renderer->gl_viewport(0, 0, feedback_paint_device.width(), feedback_paint_device.height());
	renderer->gl_scissor(0, 0, feedback_paint_device.width(), feedback_paint_device.height());

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
			feedback_paint_device.width(),
			feedback_paint_device.height());

	// In case we need to preserve the main frame buffer (if not using frame buffer object or pbuffer).
	// We never need to preserve the depth/stencil buffer though.
	GPlatesOpenGL::GLSaveRestoreFrameBuffer save_restore_main_framebuffer(
			renderer->get_capabilities(),
			frame_buffer_dimensions.first/*width*/,
			frame_buffer_dimensions.second/*height*/);

	// Where possible, force drawing to an off-screen render target.
	// It seems making the OpenGL context current is not enough to prevent Snow Leopard systems
	// with ATI graphics from hanging/crashing - this appears to be due to modifying/accessing the
	// main/default framebuffer (which is intimately tied to the windowing system).
	// Using an off-screen render target appears to avoid this issue.
	boost::optional<GPlatesOpenGL::GLScreenRenderTarget::shared_ptr_type> screen_render_target =
			renderer->get_context().get_shared_state()->acquire_screen_render_target(
					*renderer,
					GL_RGBA8/*texture_internalformat*/,
					true/*include_depth_buffer*/,
					true/*include_stencil_buffer*/);

	// Begin rendering to the off-screen target.
	if (screen_render_target)
	{
		// Begin rendering to the screen render target.
		//
		// Set the off-screen render target to the size of the main framebuffer.
		// This is because we use QPainter to render text and it sets itself up using the dimensions
		// of the main framebuffer - actually that doesn't apply when painting to a device other than
		// the main framebuffer (in our case the feedback paint device, eg, SVG) - but we'll leave the
		// restriction in for now.
		// TODO: change to a larger size render target for more efficient rendering.
		screen_render_target.get()->begin_render(
				*renderer,
				frame_buffer_dimensions.first/*width*/,
				frame_buffer_dimensions.second/*height*/);
	}
	// We have a double buffer main framebuffer and we are rendering to the back buffer.
	// So the front buffer (which is being displayed) won't get disturbed. And when this
	// widget paints itself it will clear and re-draw the back buffer and then swap it so
	// it becomes the front buffer.
	// So for these reasons we do not need to save and restore the main framebuffer with double-buffering.
	else if (!map_canvas_paint_device->doubleBuffer())
	{
		// We only have a front buffer so we need to save and restore the main (colour)
		// framebuffer in order not to disturb the display of the globe canvas painted widget.
		save_restore_main_framebuffer.save(*renderer);
	}

	// Render the scene to the feedback paint device.
	// This will use the main framebuffer for intermediate rendering in some cases.
	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			projection_matrix_scene,
			projection_matrix_text_overlay,
			feedback_paint_device.width(),
			feedback_paint_device.height());

	// End rendering to the off-screen target.
	if (screen_render_target)
	{
		screen_render_target.get()->end_render(*renderer);
	}
	else if (!map_canvas_paint_device->doubleBuffer())
	{
		save_restore_main_framebuffer.restore(*renderer);
	}
}

float
GPlatesQtWidgets::MapCanvas::calculate_scale()
{
	int min_dimension = (std::min)(d_map_view_ptr->width(), d_map_view_ptr->height());
	return static_cast<float>(min_dimension) /
		static_cast<float>(d_view_state.get_main_viewport_min_dimension());
}
