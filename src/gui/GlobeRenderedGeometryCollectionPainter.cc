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
#include <opengl/OpenGL.h>

#include "GlobeRenderedGeometryCollectionPainter.h"

#include "GlobeRenderedGeometryLayerPainter.h"

#include "opengl/GLRenderer.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesGui::GlobeRenderedGeometryCollectionPainter::GlobeRenderedGeometryCollectionPainter(
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_persistent_opengl_objects(persistent_opengl_objects),
	d_visual_layers(visual_layers),
	d_render_settings(render_settings),
	d_text_renderer_ptr(text_renderer_ptr),
	d_visibility_tester(visibility_tester),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f),
	d_visual_layers_reversed(false)
{  }


GPlatesGui::GlobeRenderedGeometryCollectionPainter::cache_handle_type
GPlatesGui::GlobeRenderedGeometryCollectionPainter::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_globe_state_scope(renderer);

	// Initialise our paint parameters so our visit methods can access them.
	d_paint_params = PaintParams(renderer, viewport_zoom_factor);

	// Draw the layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// Get the cache handle for all the rendered layers.
	const cache_handle_type cache_handle = d_paint_params->d_cache_handle;

	// These parameters are only used for the duration of this 'paint()' method.
	d_paint_params = boost::none;

	return cache_handle;
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

	// Draw the current rendered geometry layer.
	GlobeRenderedGeometryLayerPainter rendered_geom_layer_painter(
			rendered_geometry_layer,
			d_persistent_opengl_objects,
			d_paint_params->d_inverse_viewport_zoom_factor,
			d_render_settings,
			d_text_renderer_ptr,
			d_visibility_tester,
			d_colour_scheme);
	rendered_geom_layer_painter.set_scale(d_scale);

	// Paint the layer.
	const cache_handle_type layer_cache =
			rendered_geom_layer_painter.paint(*d_paint_params->d_renderer, d_layer_painter);

	// Cache the layer's painting.
	d_paint_params->d_cache_handle->push_back(layer_cache);

	// We've already visited the rendered geometry layer so don't visit its rendered geometries.
	return false;
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::set_scale(
		float scale)
{
	d_scale = scale;
}


boost::optional<GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>
GPlatesGui::GlobeRenderedGeometryCollectionPainter::get_custom_child_layers_order(
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType parent_layer)
{
	if (parent_layer == GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)
	{
		if (d_visual_layers_reversed)
		{
			return GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type(
					d_visual_layers.get_layer_order().rbegin(),
					d_visual_layers.get_layer_order().rend());
		}
		else
		{
			return d_visual_layers.get_layer_order();
		}
	}
	else
	{
		return boost::none;
	}
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::set_visual_layers_reversed(
		bool reversed)
{
	d_visual_layers_reversed = reversed;
}

