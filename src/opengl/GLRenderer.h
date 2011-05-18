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
#include <boost/optional.hpp>

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
		 * Enumerates the ways in which a render target can be used.
		 *
		 *
		 * SERIAL:
		 * A render target about to be pushed will get drawn immediately after
		 * its parent render target (the one at the top of the current render
		 * target stack before the new target is pushed) and when it is popped the parent
		 * render target will resume drawing...
		 *
		 * Push render target A
		 *   Draw D1
		 *   Push render target B
		 *     Draw D2
		 *   Pop render target B
		 *   Draw D3
		 *   Push render target B
		 *     Draw D4
		 *   Pop render target B
		 *   Draw D5
		 * Pop render target A
		 *
		 * ...here D1 is drawn to A, D2 to B, D3 to A, D4 to B, and D5 to A, in that order.
		 *
		 * Note that although D1, D3 and D5 are drawn to the same render target A they *cannot*
		 * be reordered if they share the same state (the rendering engine will normally do that
		 * unless explicitly requested not to) - this is because render targets B and C force
		 * a serialisation or barrier between D1 and D3 and between D3 and D5.
		 *
		 *
		 * PARALLEL:
		 * A render target about to be pushed will get drawn in a separate
		 * render pass than its parent render target (the one at the top of the current render
		 * target stack before the new target is pushed).
		 *
		 * Push render target A
		 *   Draw D1
		 *   Push render target B
		 *     Draw D2
		 *   Pop render target B
		 *   Draw D3
		 *   Push render target C
		 *     Draw D4
		 *   Pop render target C
		 *   Draw D5
		 * Pop render target A
		 *
		 * ...here D2 is drawn to B, D4 to C, and (D1,D3,D5) to A, in that order.
		 *
		 * Note that render targets B and C, being at the same level of the push hierarchy, will
		 * get added to the same render pass and drawn serially with respect to each other.
		 * Also note that, unlike the serial case, D1, D3 and D5 can be reordered relative to
		 * each other if they share the same state - this is because render targets B and C
		 * do *not* form a serialisation or barrier between D1, D3 and D5.
		 *
		 *
		 * 'SERIAL' is best used when a single render target (such as a texture) is rendered to
		 * multiple times per frame. In contract 'PARALLEL' is best used when a single render
		 * target is only rendered to at most once per frame - so 'PARALLEL' is useful when
		 * you want to cache rendering results over multiple frames to avoid having to
		 * render the results each frame if the results don't change. 'PARALLEL' also has the
		 * advantage of requiring fewer render target switches between the main framebuffer and
		 * the render targets (in the examples above you can picture render target A as the main
		 * framebuffer). Switches between the main framebuffer and the render targets can be costly
		 * on some graphics systems (for example those that use pbuffers as they require a full
		 * OpenGL context switch which is costly) so it makes sense to minimise them where possible.
		 * A disadvantage of 'PARALLEL' is that it tends to require more memory as the above
		 * examples show that 'PARALLEL' requires render targets B and C whereas 'SERIAL' only
		 * requires render target B. In both examples you can imagine the rendered results of
		 * D2 being used by D3 (as a texture lookup) and D4 being used by D5. In 'PARALLEL'
		 * we're rendering both D2 and D4 before D1, D3 and D5 but in 'SERIAL' we're rendering
		 * in strictly serial order D1, D2, D3, D4 and D5 and so D2 and D4 can be rendered to
		 * the same render target because the results of D2 are used by D3 before D4 is rendered
		 * and hence D4 can overwrite the results of D2 (ie, render to the same render target).
		 */
		enum RenderTargetUsageType
		{
			RENDER_TARGET_USAGE_SERIAL,
			RENDER_TARGET_USAGE_PARALLEL
		};


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
				const GLRenderTargetType::non_null_ptr_type &render_target_type,
				RenderTargetUsageType render_target_usage);

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
					RenderTargetUsageType render_target_usage);


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
			 * The type of render target (frame buffer or texture target).
			 */
			GLRenderTargetType::non_null_ptr_type d_render_target_type;

			/**
			 * Whether this render target should be used in serial or parallel.
			 */
			RenderTargetUsageType d_render_target_usage;

			/**
			 * All render operations are added to this target.
			 *
			 * It is optional because we only want to create it if the client
			 * adds a drawable to the target.
			 */
			boost::optional<GLRenderOperationsTarget::non_null_ptr_type> d_render_operations_target;
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
