/* $Id$ */

/**
 * \file Draws @a RenderedGeometry objects onto the globe (orthographic view).
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "GlobeRenderedGeometryCollectionPainter.h"

#include "OpenGL.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesGui::GlobeRenderedGeometryCollectionPainter::GlobeRenderedGeometryCollectionPainter(
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const RenderSettings &render_settings,
		boost::shared_ptr<TextRenderer> text_renderer_ptr) :
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_current_layer_far_depth(0),
	d_depth_range_per_layer(0),
	d_render_settings(render_settings),
	d_text_renderer_ptr(text_renderer_ptr)
{
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::paint(
		const double &viewport_zoom_factor,
		GPlatesGui::NurbsRenderer &nurbs_renderer,
		double depth_range_near,
		double depth_range_far)
{
	// Initialise our paint parameters so other methods can access them.
	d_paint_params = PaintParams(viewport_zoom_factor, nurbs_renderer);

	// Count the number of layers that we're going to draw.
	const unsigned num_layers_to_draw =
			GPlatesViewOperations::RenderedGeometryUtils::get_num_active_non_empty_layers(
					d_rendered_geometry_collection);

	// Divide our given depth range equally amongst the layers.
	// Draw layers back to front as we visit them.
	d_depth_range_per_layer = (depth_range_far - depth_range_near) / num_layers_to_draw;
	d_current_layer_far_depth = depth_range_far;

	// Draw the layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// Re-enable writes to the depth buffer.
	// This is might have been turned off when rendering the individual layers.
	glDepthMask(GL_TRUE);
}


bool
GPlatesGui::GlobeRenderedGeometryCollectionPainter::visit_rendered_geometry_layer(
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
{
	// If layer is not active then we don't want to visit it.
	if (!rendered_geometry_layer.is_active())
	{
		return false;
	}

	if (rendered_geometry_layer.is_empty())
	{
		return false;
	}

	// Set the depth range for the current rendered geometry layer.
	move_to_next_rendered_layer_depth_range_and_set();

	// Draw the current rendered geometry layer.
	GlobeRenderedGeometryLayerPainter rendered_geom_layer_painter(
			rendered_geometry_layer,
			d_paint_params->d_inverse_viewport_zoom_factor,
			*d_paint_params->d_nurbs_renderer,
			d_render_settings,
			d_text_renderer_ptr);
	rendered_geom_layer_painter.paint();

	// We've already visited the rendered geometry layer so don't visit its rendered geometries.
	return false;
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::move_to_next_rendered_layer_depth_range_and_set()
{
	// Since the layer is not empty we have allocated a depth range for it.
	const double far_depth = d_current_layer_far_depth;
	double near_depth = far_depth - d_depth_range_per_layer;
	if (near_depth < 0) near_depth = 0;

	glDepthRange(near_depth, far_depth);

	d_current_layer_far_depth -= d_depth_range_per_layer;
}
