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
#include <QGLWidget>
#include <QGraphicsView>
#include <QPaintEngine>
#include <QPainter>

#include "MapCanvas.h"
#include "MapView.h"

#include "gui/Map.h"
#include "gui/MapProjection.h"
#include "gui/MapTransform.h"
#include "gui/RenderSettings.h"
#include "gui/TextOverlay.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::MapCanvas::MapCanvas(
		GPlatesPresentation::ViewState &view_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		MapView *map_view_ptr,
		GPlatesGui::RenderSettings &render_settings,
		GPlatesGui::ViewportZoom &viewport_zoom,
		const GPlatesGui::ColourScheme::non_null_ptr_type &colour_scheme,
		const GPlatesGui::TextRenderer::non_null_ptr_to_const_type &text_renderer,
		QWidget *parent_):
	QGraphicsScene(parent_),
	d_view_state(view_state),
	d_map_view_ptr(map_view_ptr),
	d_map(
			view_state,
			rendered_geometry_collection,
			view_state.get_visual_layers(),
			render_settings,
			viewport_zoom,
			colour_scheme,
			text_renderer),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_text_overlay(
			new GPlatesGui::TextOverlay(
				view_state.get_application_state(),
				text_renderer))
{
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
		SLOT(update_canvas(
			GPlatesViewOperations::RenderedGeometryCollection &,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)));

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
GPlatesQtWidgets::MapCanvas::drawBackground(
		QPainter *painter,
		const QRectF &rect)
{
	d_map.draw_background();
}

void
GPlatesQtWidgets::MapCanvas::drawForeground(
		QPainter *painter, 
		const QRectF &rect)
{
	float scale = calculate_scale();
	d_map.paint(scale);

	d_text_overlay->paint(
			d_view_state.get_text_overlay_settings(),
			d_map_view_ptr->width(),
			d_map_view_ptr->height(),
			scale);

	emit repainted();
}

void
GPlatesQtWidgets::MapCanvas::update_canvas()
{
	update();
}

void
GPlatesQtWidgets::MapCanvas::update_canvas(
		GPlatesViewOperations::RenderedGeometryCollection &collection,
		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type)
{
	d_map.set_update_type(update_type);
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

