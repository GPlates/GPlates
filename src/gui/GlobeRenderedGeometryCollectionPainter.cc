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

#include "opengl/GLEnterOrLeaveStateSet.h"
#include "opengl/GLFragmentTestStates.h"
#include "opengl/GLStateSet.h"
#include "opengl/GLUNurbsRenderer.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesGui::GlobeRenderedGeometryCollectionPainter::GlobeRenderedGeometryCollectionPainter(
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		TextRenderer::ptr_to_const_type text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_visual_layers(visual_layers),
	d_render_settings(render_settings),
	d_text_renderer_ptr(text_renderer_ptr),
	d_visibility_tester(visibility_tester),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f)
{
}


void
GPlatesGui::GlobeRenderedGeometryCollectionPainter::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_node,
		const double &viewport_zoom_factor,
		const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs_renderer)
{
	// Initialise our paint parameters so our visit methods can access them.
	d_paint_params = PaintParams(render_graph_node, viewport_zoom_factor, nurbs_renderer);

	// Draw the layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// These parameters are only used for the duration of this 'paint()' method.
	d_paint_params = boost::none;
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

	// Create an internal node to represent the current rendered geometry layer.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type rendered_layer_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();

	// Turn on depth testing - it's off by default.
	//
	// NOTE: We don't set the depth write state here because
	// GlobeRenderedGeometryLayerPainter will do that.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type state_set =
			GPlatesOpenGL::GLDepthTestState::create();
	state_set->gl_enable(GL_TRUE);

	// Create a state set that ensures this rendered layer will form a render sub group
	// that will not get reordered with other layers by the renderer (to minimise state changes).
	state_set->set_enable_render_sub_group();

	rendered_layer_node->set_state_set(state_set);

	// Add the render layer node to the parent render graph node.
	d_paint_params->d_render_collection_node->add_child_node(rendered_layer_node);

	// Draw the current rendered geometry layer.
	GlobeRenderedGeometryLayerPainter rendered_geom_layer_painter(
			rendered_geometry_layer,
			d_paint_params->d_inverse_viewport_zoom_factor,
			d_paint_params->d_nurbs_renderer,
			d_render_settings,
			d_text_renderer_ptr,
			d_visibility_tester,
			d_colour_scheme);
	rendered_geom_layer_painter.set_scale(d_scale);
	rendered_geom_layer_painter.paint(rendered_layer_node);

	// We've already visited the rendered geometry layer so don't visit its rendered geometries.
	return false;
}
