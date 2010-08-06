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
 
#ifndef GPLATES_OPENGL_GLRENDEROPERATIONSTARGET_H
#define GPLATES_OPENGL_GLRENDEROPERATIONSTARGET_H

#include <cstddef> // For std::size_t
#include <map>
#include <vector>

#include "GLRenderOperation.h"
#include "GLRenderTarget.h"
#include "GLStateGraph.h"
#include "GLStateGraphNode.h"
#include "GLStateSet.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLState;

	/**
	 * All render operations that contribute to a render target as added to this class.
	 */
	class GLRenderOperationsTarget :
			public GPlatesUtils::ReferenceCount<GLRenderOperationsTarget>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderOperationsTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderOperationsTarget> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderOperationsTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderOperationsTarget> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderOperationsTarget object.
		 *
		 * Also sets the default render group (to that of the root state graph node).
		 *
		 * The state graph is stored here to keep the nodes in the state graph
		 * alive until @a draw is called.
		 *
		 * Drawing will be done to @a render_target when @a draw is called.
		 */
		static
		non_null_ptr_type
		create(
				const GLRenderTarget::non_null_ptr_type &render_target,
				const GLStateGraph::non_null_ptr_type &state_graph)
		{
			return non_null_ptr_type(new GLRenderOperationsTarget(render_target, state_graph));
		}


		/**
		 * Sets the state that subsequently added render operations will be rendered with.
		 */
		void
		set_state(
				const GLStateGraphNode::non_null_ptr_to_const_type &state);


		/**
		 * Adds a render operation to this target.
		 *
		 * The render operation will be drawn using the current state set by @a set_state,
		 * but the rendering will happen when @a draw is called.
		 */
		void
		add_render_operation(
				const GLRenderOperation::non_null_ptr_to_const_type &render_operation);


		/**
		 * Draws all render operations added to this target.
		 *
		 * Render operations belong to render groups and the render groups are drawn
		 * in the order of their integer render group number.
		 */
		void
		draw(
				GLState &state);


	private:
		//! Typedef for a sequence of render operations.
		typedef std::vector<GLRenderOperation::non_null_ptr_to_const_type> render_operation_seq_type;

		//! Used to associate a state graph node with its render operations.
		struct RenderSequence
		{
			RenderSequence(
					const GLStateGraphNode::non_null_ptr_to_const_type &state_) :
				state(state_)
			{  }

			GLStateGraphNode::non_null_ptr_to_const_type state;
			render_operation_seq_type render_operations;
		};

		//! Typedef for a sequence of render sequence pointers.
		typedef std::vector<RenderSequence *> render_sequence_ptr_seq_type;

		/**
		 * Typedef for mapping state graph nodes to render sequences (groups of render operations).
		 * This is within a single render group.
		 */
		typedef std::map<
				GLStateGraphNode::non_null_ptr_to_const_type,
				RenderSequence> render_sequence_map_type;

		//! Used to keep a unique sequence of state graph nodes sorted by order they were added.
		struct RenderGroup
		{
			render_sequence_map_type render_sequences;
			//! Contains pointers into @a render_sequences and is ordered by draw order.
			render_sequence_ptr_seq_type render_sequence_draw_order;
		};

		/**
		 * Typedef for a mapping between render group ordering integers and render groups.
		 */
		typedef std::map<GLStateSet::render_group_type, RenderGroup> render_group_map_type;


		/**
		 * The render target destination into which render operations are drawn.
		 */
		GLRenderTarget::non_null_ptr_type d_render_target;

		/**
		 * This state graph contains the state graph nodes referenced by the render operations.
		 */
		GLStateGraph::non_null_ptr_type d_state_graph;

		/**
		 * Groups of render operations ordered by draw order.
		 */
		render_group_map_type d_render_groups;

		/**
		 * The current render group id.
		 */
		GLStateSet::render_group_type d_current_render_group_id;

		/**
		 * The current render group that render operations are added to.
		 *
		 * This should always reflect @a d_current_render_group_id.
		 */
		RenderGroup *d_current_render_group;

		/**
		 * The current render sequence that subsequently added render operations
		 * should be added to.
		 */
		RenderSequence *d_current_render_sequence;

		/**
		 * The current state (state graph node) that subsequently added render operations
		 * should be drawn using.
		 */
		const GLStateGraphNode *d_current_render_state;


		//! Constructor.
		GLRenderOperationsTarget(
				const GLRenderTarget::non_null_ptr_type &render_target,
				const GLStateGraph::non_null_ptr_type &state_graph);
	};
}

#endif // GPLATES_OPENGL_GLRENDEROPERATIONSTARGET_H
