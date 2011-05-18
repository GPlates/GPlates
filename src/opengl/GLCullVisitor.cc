/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>

#include "GLCullVisitor.h"

#include "GLMultiResolutionRaster.h"
#include "GLMultiResolutionRasterNode.h"
#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"
#include "GLMultiResolutionReconstructedRasterNode.h"
#include "GLRenderOperation.h"
#include "GLRenderGraph.h"
#include "GLRenderGraphDrawableNode.h"
#include "GLRenderGraphInternalNode.h"
#include "GLRenderTargetType.h"
#include "GLText3DNode.h"
#include "GLViewportNode.h"
#include "GLViewportState.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLCullVisitor::GLCullVisitor() :
	d_render_queue(GLRenderQueue::create()),
	d_renderer(GLRenderer::create(d_render_queue))
{
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<render_graph_type> &render_graph)
{
	// Push a render target corresponding to the frame buffer (of the window).
	// This will be the render target that the main scene is rendered to.
	// It doesn't really matter whether the render target usage is serial or parallel
	// because the render target is the framebuffer and we're not using the results
	// of rendering to it (like we would a render texture).
	d_renderer->push_render_target(
			GLFrameBufferRenderTargetType::create(),
			GLRenderer::RENDER_TARGET_USAGE_SERIAL);

	render_graph->get_root_node().accept_visitor(*this);

	d_renderer->pop_render_target();
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<render_graph_internal_node_type> &internal_node)
{
	preprocess_node(*internal_node);

	// Visit the child nodes.
	internal_node->visit_child_nodes(*this);

	postprocess_node(*internal_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<viewport_node_type> &viewport_node)
{
	preprocess_node(*viewport_node);

	// Get the old viewport.
	boost::optional<GLViewport> old_viewport =
			d_renderer->get_transform_state().get_current_viewport();

	// Get the new viewport.
	const GLViewport &new_viewport = viewport_node->get_viewport();

	// Create a state set to set and restore the viewport.
	GLViewportState::non_null_ptr_type viewport_state = GLViewportState::create(
			new_viewport, old_viewport);

	// Push the viewport state set.
	d_renderer->push_state_set(viewport_state);

	// Let the transform state know of the new viewport.
	// This is necessary since it is used to determine pixel projections in world space
	// which are in turn used for level-of-detail selection for rasters.
	d_renderer->get_transform_state().set_viewport(new_viewport);

	// Visit the child nodes.
	viewport_node->visit_child_nodes(*this);

	// Restore the old viewport if there was one.
	if (old_viewport)
	{
		d_renderer->get_transform_state().set_viewport(old_viewport.get());
	}

	// Pop the viewport state set.
	d_renderer->pop_state_set();

	postprocess_node(*viewport_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<render_graph_drawable_node_type> &drawable_node)
{
	preprocess_node(*drawable_node);

	// Add the drawable.
	d_renderer->add_drawable(drawable_node->get_drawable());

	postprocess_node(*drawable_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_raster_node_type> &raster_node)
{
	preprocess_node(*raster_node);

	// Render the multi-resolution raster.
	raster_node->get_multi_resolution_raster()->render(*d_renderer);

	postprocess_node(*raster_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_reconstructed_raster_node_type> &
				reconstructed_raster_node)
{
	preprocess_node(*reconstructed_raster_node);

	// Render the multi-resolution raster.
	reconstructed_raster_node->get_multi_resolution_reconstructed_raster()->render(*d_renderer);

	postprocess_node(*reconstructed_raster_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<text_3d_node_type> &text_3d_node)
{
	preprocess_node(*text_3d_node);

	// Add the drawable.
	d_renderer->add_drawable(
			// Converts 3D text position to 2D window coordinates...
			text_3d_node->get_drawable(
					d_renderer->get_transform_state()));

	postprocess_node(*text_3d_node);
}


void
GPlatesOpenGL::GLCullVisitor::preprocess_node(
		const GLRenderGraphNode &node)
{
	// If the current node has some state to set then push it.
	if (node.get_state_set())
	{
		d_renderer->push_state_set(node.get_state_set().get());
	}

	// If the current node has a transform then push it.
	if (node.get_transform())
	{
		const GLTransform &node_transform = *node.get_transform().get();

		d_renderer->push_transform(node_transform);
	}
}


void
GPlatesOpenGL::GLCullVisitor::postprocess_node(
		const GLRenderGraphNode &node)
{
	// If the current node pushed a transform then pop it.
	if (node.get_transform())
	{
		d_renderer->pop_transform();
	}

	// If the current node pushed some state then pop it.
	if (node.get_state_set())
	{
		d_renderer->pop_state_set();
	}
}
