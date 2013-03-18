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
#include <QPaintEngine>
#include <QPainter>
#include <opengl/OpenGL.h>

#include "MapCanvas.h"
#include "MapView.h"

#include "global/GPlatesException.h"

#include "gui/Map.h"
#include "gui/MapProjection.h"
#include "gui/MapTransform.h"
#include "gui/RenderSettings.h"

#include "opengl/GLContext.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLRenderTarget.h"
#include "opengl/GLSaveRestoreFrameBuffer.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLViewport.h"

#include "presentation/ViewState.h"

#include "utils/Profile.h"


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
	d_text_renderer(GPlatesGui::QPainterTextRenderer::create()),
	d_text_overlay(new GPlatesGui::TextOverlay(view_state.get_application_state(), d_text_renderer)),
	d_map(
			view_state,
			gl_visual_layers,
			rendered_geometry_collection,
			view_state.get_visual_layers(),
			render_settings,
			viewport_zoom,
			colour_scheme,
			d_text_renderer),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_disable_update(false)
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
GPlatesQtWidgets::MapCanvas::set_disable_update(
		bool b)
{
	d_disable_update = b;
}

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
			GPlatesOpenGL::GLViewport(0, 0, d_map_view_ptr->width(), d_map_view_ptr->height()));

	// Initialise those parts of map that require a valid OpenGL context to be bound.
	d_map.initialiseGL(*renderer);
}


GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene(
		GPlatesOpenGL::GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Let the text renderer know of the QPainter object (indirectly via GLRenderer) so it can use
	// it to render text with.
	// We need to do this before any text rendering can occur (and it can inside 'Map::paint').
	GPlatesGui::TextRenderer::RenderScope text_render_scope(*d_text_renderer, &renderer);

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

	// Draw the optional text overlay.
	d_text_overlay->paint(
			d_view_state.get_text_overlay_settings(),
			d_map_view_ptr->width(),
			d_map_view_ptr->height(),
			scale);

	// Finished text rendering.
	text_render_scope.end_render();

	return frame_cache_handle;
}


void
GPlatesQtWidgets::MapCanvas::drawBackground(
		QPainter *painter,
		const QRectF &/*exposed_rect*/)
{
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
	// This also let's GLRenderer know of the modelview/projection transforms set by the QPainter.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, *painter, true/*paint_device_is_framebuffer*/);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(*renderer);

	// End the render scope so that the OpenGL state is now back to the default state
	// before we emit the repainted signal.
	render_scope.end_render();

	Q_EMIT repainted();
}

void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	if (!d_disable_update)
	{
		update();
	}
}

QImage
GPlatesQtWidgets::MapCanvas::render_to_qimage(
		QGLWidget *paint_device,
		const QTransform &viewport_transform,
		const QSize &image_size)
{
	// Make sure our OpenGL context is the currently active context.
	d_gl_context->make_current();

	// By default the QPainter will set up the OpenGL projection matrix to fit the canvas using
	// an orthographic projection.
	QPainter painter(paint_device);

	// Set the painter transform to match the current view.
	painter.setWorldTransform(viewport_transform);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	//
	// By default the current render target of 'renderer' is the main frame buffer (of the window).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	// This also let's GLRenderer know of the modelview/projection transforms set by the QPainter.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(
			*renderer,
			painter,
			true/*paint_device_is_framebuffer*/);

	// Determine the tile render target dimensions.
	// Use power-of-two tile render-target dimensions to enhance re-use of render-target...
	unsigned int tile_render_target_width = 1024;
	unsigned int tile_render_target_height = 1024;

	// Acquire a render target to use for rendering the scene into tiles.
	boost::optional<GPlatesOpenGL::GLRenderTarget::shared_ptr_type> tile_render_target =
			d_gl_context->get_shared_state()->acquire_render_target(
					*renderer,
					GL_RGBA8,
					// We don't need a depth buffer for 2D map rendering...
					false/*include_depth_buffer*/,
					tile_render_target_width,
					tile_render_target_height);
	// If a render target is not supported then use the main framebuffer as a render target...
	if (!tile_render_target)
	{
		tile_render_target_width = d_map_view_ptr->width();
		tile_render_target_height = d_map_view_ptr->height();
	}

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int tile_border = 10;
	// Set up for rendering the scene into tiles.
	GPlatesOpenGL::GLTileRender tile_render(
			tile_render_target_width,
			tile_render_target_height,
			GPlatesOpenGL::GLViewport(
					0,
					0,
					image_size.width(),
					image_size.height())/*destination_viewport*/,
			tile_border);

	// The image to render the scene into.
	QImage image(image_size, QImage::Format_ARGB32);

	// Fill the image with transparent black in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(0,0,0,0).rgba());

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (tile_render.first_tile(); !tile_render.finished(); tile_render.next_tile())
	{
		// If we have a render target then render to it, otherwise use the main framebuffer.
		if (tile_render_target)
		{
			// Render to render target instead of main framebuffer.
			GPlatesOpenGL::GLRenderTarget::RenderScope tile_render_target_scope(
					*tile_render_target.get(),
					*renderer);

			cache_handle_type tile_cache_handle = render_scene_tile_into_image(*renderer, tile_render, image);
			frame_cache_handle->push_back(tile_cache_handle);
		}
		else if (paint_device->doubleBuffer())
		{
			// We have a double buffer main framebuffer and we are rendering to the back buffer.
			// So the front buffer (which is being displayed) won't get disturbed. And when this
			// widget paints itself it will clear and re-draw the back buffer and then swap it so
			// it becomes the front buffer.
			// So for these reasons we do not need to save and restore the main framebuffer.
			cache_handle_type tile_cache_handle = render_scene_tile_into_image(*renderer, tile_render, image);
			frame_cache_handle->push_back(tile_cache_handle);
		}
		else
		{
			// We only have a front buffer so we need to save and restore the main (colour)
			// framebuffer in order not to disturb the display of the globe canvas painted widget.
			GPlatesOpenGL::GLSaveRestoreFrameBuffer save_restore_main_framebuffer(
					tile_render_target_width,
					tile_render_target_height);

			save_restore_main_framebuffer.save(*renderer);
			cache_handle_type tile_cache_handle = render_scene_tile_into_image(*renderer, tile_render, image);
			frame_cache_handle->push_back(tile_cache_handle);
			save_restore_main_framebuffer.restore(*renderer);
		}
	}

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = frame_cache_handle;

	return image;
}

GPlatesQtWidgets::MapCanvas::cache_handle_type
GPlatesQtWidgets::MapCanvas::render_scene_tile_into_image(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLTileRender &tile_render,
		QImage &image)
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

	// The projection matrix adjusted for the current tile.
	GPlatesOpenGL::GLMatrix projection_matrix(tile_projection_transform->get_matrix());
	projection_matrix.gl_ortho(0, image.width(), image.height(), 0, -999999, 999999);

	renderer.gl_load_matrix(GL_PROJECTION, projection_matrix);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(renderer);

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
		QPaintDevice &paint_device,
		const QTransform &viewport_transform)
{
	// Make sure our OpenGL context is the currently active context.
	d_gl_context->make_current();

	// Note that we're not rendering to the OpenGL canvas here.
	// The OpenGL rendering gets redirected into the QPainter (using OpenGL feedback) and
	// ends up in the specified paint device.
	QPainter painter(&paint_device);

	// Set the painter transform to match the current view.
	painter.setWorldTransform(viewport_transform);

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
	// This also let's GLRenderer know of the modelview/projection transforms set by the QPainter.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, painter, false/*paint_device_is_framebuffer*/);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(*renderer);
}

float
GPlatesQtWidgets::MapCanvas::calculate_scale()
{
	int min_dimension = (std::min)(d_map_view_ptr->width(), d_map_view_ptr->height());
	return static_cast<float>(min_dimension) /
		static_cast<float>(d_view_state.get_main_viewport_min_dimension());
}

