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

#include "opengl/GLMatrix.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLViewport.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::MapCanvas::MapCanvas(
		GPlatesPresentation::ViewState &view_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		MapView *map_view_ptr,
		const GPlatesOpenGL::GLContext::non_null_ptr_type &gl_context,
		const GPlatesGui::PersistentOpenGLObjects::non_null_ptr_type &gl_persistent_objects,
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
			gl_persistent_objects,
			rendered_geometry_collection,
			view_state.get_visual_layers(),
			render_settings,
			viewport_zoom,
			colour_scheme,
			d_text_renderer),
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

void
GPlatesQtWidgets::MapCanvas::set_disable_update(
		bool b)
{
	if(b)
	{
		QObject::disconnect(d_rendered_geometry_collection,
		SIGNAL(collection_was_updated(
				GPlatesViewOperations::RenderedGeometryCollection &,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
		this,
		SLOT(update_canvas()));
	}
	else
	{
		QObject::connect(d_rendered_geometry_collection,
		SIGNAL(collection_was_updated(
				GPlatesViewOperations::RenderedGeometryCollection &,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
		this,
		SLOT(update_canvas()));
	}
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
			GPlatesOpenGL::GLViewport(0, 0, d_map_view_ptr->width(), d_map_view_ptr->height()));

	// Initialise those parts of map that require a valid OpenGL context to be bound.
	d_map.initialiseGL(*renderer);
}

void
GPlatesQtWidgets::MapCanvas::drawBackground(
		QPainter *painter,
		const QRectF &/*rect*/)
{
	try
	{
		PROFILE_BLOCK("MapCanvas::drawBackground: render map");

		// Create a render for all our OpenGL rendering work.
		// Note that nothing will happen until we enter a rendering scope.
		GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

		// Start a begin_render/end_render scope.
		//
		// By default the current render target of 'renderer' is the main frame buffer (of the window).
		//
		// We're currently in an active OpenGL QPainter so we need to let the GLRenderer know about that.
		// This also let's GLRenderer know of the modelview/projection transforms set by the QPainter.
		GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, painter);

		// Let the text renderer know of the QPainter object (indirectly via GLRenderer) so it can use
		// it to render text with.
		// We need to do this before any text rendering can occur (and it can inside 'Map::paint').
		GPlatesGui::TextRenderer::RenderScope text_render_scope(*d_text_renderer, renderer.get());

		// Render the map.
		const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
		const float scale = calculate_scale();
		//
		// Paint the map and its contents.
		//
		// NOTE: We hold onto the previous frame's cached resources *while* generating the current frame
		// and then release our hold on the previous frame (by assigning the current frame's cache).
		// This just prevents a render frame from re-using cached resources of the previous frame
		// in order to avoid regenerating the same cached resources unnecessarily each frame.
		// Since the view direction usually differs little from one frame to the next there is a lot
		// of overlap that we want to reuse (and not recalculate).
		//
		d_gl_frame_cache_handle = d_map.paint(*renderer, viewport_zoom_factor, scale);

		// Draw the optional text overlay.
		d_text_overlay->paint(
				d_view_state.get_text_overlay_settings(),
				d_map_view_ptr->width(),
				d_map_view_ptr->height(),
				scale);

		// Finished text rendering.
		text_render_scope.end_render();

		// At scope exit OpenGL should now be back in the default OpenGL state...
	}
	catch (const GPlatesGlobal::Exception &e)
	{
		qWarning() << e;
	}

	emit repainted();
}

void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	update();
}

void
GPlatesQtWidgets::MapCanvas::draw_svg_output()
{
	// Switch off unwanted layers - taken from the Globe's painting routines. 

	// Get current rendered layer active state so we can restore later.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState
		prev_rendered_layer_active_state =
		d_rendered_geometry_collection->capture_main_layer_active_state();

	// Turn off rendering of digitisation layer.
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER,
		false);

	update();

	// Force the update signal to be processed so that 
	// content is drawn to the OpenGL feedback buffer. 
	QApplication::processEvents();


	// Restore previous rendered layer active state.
	d_rendered_geometry_collection->restore_main_layer_active_state(
		prev_rendered_layer_active_state);

}

float
GPlatesQtWidgets::MapCanvas::calculate_scale()
{
	int min_dimension = (std::min)(d_map_view_ptr->width(), d_map_view_ptr->height());
	return static_cast<float>(min_dimension) /
		static_cast<float>(d_view_state.get_main_viewport_min_dimension());
}

