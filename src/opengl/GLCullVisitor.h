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
 
#ifndef GPLATES_OPENGL_GLCULLVISITOR_H
#define GPLATES_OPENGL_GLCULLVISITOR_H

#include "GLRenderGraphVisitor.h"

#include "GLRenderer.h"
#include "GLRenderQueue.h"


namespace GPlatesOpenGL
{
	class GLRenderGraphNode;

	/**
	 * Visits a @a GLRenderGraph to draw nodes that are visible in the
	 * view frustum and output render leaves to a render queue.
	 *
	 * Well, it culls multi-resolution rasters according to visibility
	 * (where each raster is like an individual spatial tree).
	 * Culling of geometries is not done, and should eventually be done by
	 * by a separate adaptive bounding volume tree anyway (ie, a tree that is
	 * distinct from the render graph tree and not one big merged tree that
	 * is typically called a scene graph that tries to do both with one tree).
	 */
	class GLCullVisitor :
			public ConstGLRenderGraphVisitor
	{
	public:
		//! Constructor.
		GLCullVisitor();


		/**
		 * Returns the render queue created by visiting a render graph.
		 */
		GLRenderQueue::non_null_ptr_type
		get_render_queue() const
		{
			return d_render_queue;
		}


		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<render_graph_type> &);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<render_graph_internal_node_type> &);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<viewport_node_type> &);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<render_graph_drawable_node_type> &);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_raster_node_type> &);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_reconstructed_raster_node_type> &);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<text_3d_node_type> &);

	private:
		/**
		 * The output of our visitation goes here and eventually gets returned to the caller.
		 */
		GLRenderQueue::non_null_ptr_type d_render_queue;

		/**
		 * Used to add render operations and set state on the render queue.
		 */
		GLRenderer::non_null_ptr_type d_renderer;


		/**
		 * Pre process a node before visiting its child nodes.
		 */
		void
		preprocess_node(
				const GLRenderGraphNode &node);

		/**
		 * Post process a node after visiting its child nodes.
		 */
		void
		postprocess_node(
				const GLRenderGraphNode &node);
	};
}

#endif // GPLATES_OPENGL_GLCULLVISITOR_H
