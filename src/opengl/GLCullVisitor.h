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

#include <stack>

#include "GLRenderGraphVisitor.h"

#include "GLRenderQueue.h"
#include "GLRenderTarget.h"
#include "GLStateGraph.h"
#include "GLStateGraphBuilder.h"
#include "GLStateSet.h"
#include "GLTransformState.h"


namespace GPlatesOpenGL
{
	class GLRenderGraphNode;
	class GLRenderQueue;

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
				const GPlatesUtils::non_null_intrusive_ptr<text_3d_node_type> &);

	private:
		/**
		 * Keeps track of all state used to render into a render target.
		 */
		struct RenderTargetState
		{
			//! Constructor.
			RenderTargetState(
					const GLRenderTarget::non_null_ptr_type &render_target,
					GLRenderQueue &render_queue);


			/**
			 * Used to keep track of the transformation state (includes
			 * model-view, projection and texture matrices and the viewport).
			 *
			 * These help us with culling and level-of-detail selection.
			 *
			 * Also allows us to calculate matrices with double-precision and output the
			 * final concatenated matrix for each transform node in the render graph.
			 */
			GLTransformState::non_null_ptr_type d_transform_state;

			/**
			 * Used to assemble the OpenGL state of visible nodes in the render graph.
			 *
			 * The state is not applied directly to OpenGL yet - that is done by the
			 * render queue when it is rendered.
			 */
			GLStateGraphBuilder::non_null_ptr_type d_state_graph_builder;

			/**
			 * The state graph that will get stored with a @a GLRenderTarget since
			 * the render target has @a GLRenderOperation objects associated with it that
			 * reference state graph nodes.
			 */
			GLStateGraph::non_null_ptr_type d_state_graph;

			/**
			 * All render operations are added to this target.
			 */
			GLRenderOperationsTarget::non_null_ptr_type d_render_operations_target;
		};

		//! Typedef for a stack of render target states.
		typedef std::stack<RenderTargetState> render_target_state_stack_type;


		/**
		 * Stack of currently pushed render targets.
		 */
		render_target_state_stack_type d_render_target_state_stack;

		/**
		 * References the current top of the render target state stack.
		 */
		RenderTargetState *d_current_render_target_state;

		/**
		 * The output of our visitation goes here and eventually gets returned to the caller.
		 */
		GLRenderQueue::non_null_ptr_type d_render_queue;


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


		/**
		 * Push a new render target and associated state.
		 */
		void
		push_render_target(
				const GLRenderTarget::non_null_ptr_type &render_target);

		/**
		 * Pop most recently pushed render target and associated state.
		 */
		void
		pop_render_target();


		/**
		 * Pushes a state set and sets the new state on the current render operations target.
		 */
		void
		push_state_set(
				const GLStateSet::non_null_ptr_to_const_type &state_set);


		/**
		 * Pops most recent state set and sets the new state on the current render operations target.
		 */
		void
		pop_state_set();


		/**
		 * Adds a render operation to the current render operations target.
		 */
		void
		add_render_operation(
				const GLRenderOperation::non_null_ptr_to_const_type &render_operation)
		{
			d_current_render_target_state->d_render_operations_target->add_render_operation(
					render_operation);
		}
	};
}

#endif // GPLATES_OPENGL_GLCULLVISITOR_H
