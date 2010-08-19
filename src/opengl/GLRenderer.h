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
 
#ifndef GPLATES_OPENGL_GLRENDERER_H
#define GPLATES_OPENGL_GLRENDERER_H

#include <stack>

#include "GLDrawable.h"
#include "GLRenderQueue.h"
#include "GLRenderTargetType.h"
#include "GLStateGraph.h"
#include "GLStateGraphBuilder.h"
#include "GLStateSet.h"
#include "GLTransform.h"
#include "GLTransformState.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Handles rendering to render targets.
	 */
	class GLRenderer :
			public GPlatesUtils::ReferenceCount<GLRenderer>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderer.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderer> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderer.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderer> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderer object.
		 */
		static
		non_null_ptr_type
		create(
				const GLRenderQueue::non_null_ptr_type &render_queue)
		{
			return non_null_ptr_type(new GLRenderer(render_queue));
		}


		/**
		 * Push a new render target.
		 *
		 * NOTE: This creates a new blank state graph and transform state.
		 * So any state set prior to this push no longer applies after the push.
		 */
		void
		push_render_target(
				const GLRenderTargetType::non_null_ptr_type &render_target_type);

		/**
		 * Pop most recently pushed render target and associated state.
		 *
		 * NOTE: The state graph and transform state prior to the most recent push will be restored.
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
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
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
		 */
		void
		pop_state_set();


		/**
		 * Pushes @a transform onto the stack indicated by its @a get_matrix_mode method -
		 * the stack is in the current render operations target.
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
		 */
		void
		push_transform(
				const GLTransform &transform);


		/**
		 * Pops the most recently pushed transform off its corresponding matrix stack in
		 * the current render operations target.
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
		 */
		void
		pop_transform();


		/**
		 * Returns the transform state of the currently pushed render target.
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
		 */
		const GLTransformState &
		get_transform_state() const;

		/**
		 * Returns the transform state of the currently pushed render target.
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
		 */
		GLTransformState &
		get_transform_state();


		/**
		 * Adds a drawable to the current render operations target.
		 *
		 * It will later be drawn using the current state and model-view and projection matrices
		 * (of the transform state of the current render target).
		 *
		 * @throws PreconditionViolationError if there's no currently pushed render target.
		 */
		void
		add_drawable(
				const GLDrawable::non_null_ptr_to_const_type &drawable);

	private:
		/**
		 * Keeps track of all state used to render into a render target.
		 */
		struct RenderTargetState
		{
			//! Constructor.
			RenderTargetState(
					const GLRenderTargetType::non_null_ptr_type &render_target_type,
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


		//! Constructor.
		explicit
		GLRenderer(
				const GLRenderQueue::non_null_ptr_type &render_queue);
	};
}

#endif // GPLATES_OPENGL_GLRENDERER_H
