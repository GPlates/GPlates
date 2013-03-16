/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLRENDERERIMPL_H
#define GPLATES_OPENGL_GLRENDERERIMPL_H

#include <stack>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLCompiledDrawState.h"
#include "GLState.h"
#include "GLTexture.h"
#include "GLTileRender.h"

#include "utils/Counter64.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLCapabilities;

	namespace GLRendererImpl
	{
		/**
		 * Keeps track of all state sets within a state block scope.
		 */
		class StateBlock
		{
		public:
			//! Constructor when @a current_state is the *full* state.
			explicit
			StateBlock(
					const GLState::shared_ptr_type &current_state) :
				d_current_state(current_state)
			{  }

			//! Constructor when @a current_state is a state *change* compared to @a begin_state_to_apply.
			StateBlock(
					const GLState::shared_ptr_type &current_state,
					const GLState::shared_ptr_to_const_type &begin_state_to_apply) :
				d_current_state(current_state),
				d_begin_state_to_apply(begin_state_to_apply)
			{  }

			//! Constructor when @a compiled_draw_state is a state *change* compared to @a begin_state_to_apply.
			StateBlock(
					const GLCompiledDrawState::non_null_ptr_type &compiled_draw_state,
					const GLState::shared_ptr_to_const_type &begin_state_to_apply) :
				// NOTE: The current state is actually the compiled state *change*...
				d_current_state(compiled_draw_state->d_state_change),
				d_begin_state_to_apply(begin_state_to_apply),
				d_compiled_draw_state(compiled_draw_state)
			{  }

			/**
			 * Returns the current state of this state block.
			 *
			 * When compiling draw state this represents the state *change* being compiled.
			 *
			 * When *not* compiling draw state this is the same as @a get_state_to_apply.
			 */
			const GLState::shared_ptr_type &
			get_current_state() const
			{
				return d_current_state;
			}

			/**
			 * Returns the current state that can be applied to OpenGL (the full state).
			 *
			 * When compiling draw state this represents the state *change* being compiled *plus*
			 * the begin state of this state block (passed into constructor).
			 *
			 * When *not* compiling draw state this is the same as @a get_current_state.
			 *
			 * NOTE: It's possible that state gets applied while we're compiling draw state.
			 * For example, when buffers get bound they turn on immediate applies and then bind
			 * themselves which causes this method to be called.
			 */
			GLState::shared_ptr_type
			get_state_to_apply() const
			{
				return d_begin_state_to_apply
						? get_state_to_apply_from_state_change()
						: d_current_state;
			}

			/**
			 * Same as @a get_state_to_apply but makes sure a copy/clone is returned.
			 */
			GLState::shared_ptr_type
			get_cloned_state_to_apply() const
			{
				return d_begin_state_to_apply
						? get_state_to_apply_from_state_change()
						: d_current_state->clone();
			}

			/**
			 * Returns the optional compiled draw state (if currently compiling
			 */
			const boost::optional<GLCompiledDrawState::non_null_ptr_type> &
			get_compiled_draw_state() const
			{
				return d_compiled_draw_state;
			}

		private:
			/**
			 * The snapshot of the current OpenGL state for this state block.
			 */
			GLState::shared_ptr_type d_current_state;

			/**
			 * If compiling draw state in this state block...
			 *
			 * The *full* renderer state just before the state block has begun.
			 */
			boost::optional<GLState::shared_ptr_to_const_type> d_begin_state_to_apply;

			/**
			 * If compiling draw state in this state block...
			 *
			 * In this case @a d_current_state represents the current state *change* being compiled.
			 * In other words it doesn't include @a begin_state.
			 */
			boost::optional<GLCompiledDrawState::non_null_ptr_type> d_compiled_draw_state;


			//! Returns state to apply using the compiled draw state.
			inline
			GLState::shared_ptr_type
			get_state_to_apply_from_state_change() const
			{
				// We're compiling draw state so 'd_current_state' is actually a state *change*.
				const GLState::shared_ptr_type state_to_apply = d_begin_state_to_apply.get()->clone();
				state_to_apply->merge_state_change(*d_current_state);

				return state_to_apply;
			}
		};

		//! Typedef for a stack of state blocks.
		typedef std::stack<StateBlock> state_block_stack_type;

		/**
		 * Interface for the various draw calls - so we can wrap them up for a render queue is requested.
		 */
		struct Drawable :
				public GPlatesUtils::ReferenceCount<Drawable>
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<Drawable> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Drawable> non_null_ptr_to_const_type;

			virtual
			~Drawable()
			{  }

			/**
			 * Applies that part of the state in @a state_to_apply (to OpenGL) that is used
			 * by this (derived) draw command.
			 *
			 * The default implementation should be to apply all state as this is typically required
			 * by draw commands such as 'glDrawRangeElements'.
			 *
			 * However some draw commands, such as 'glClear', are only affected by a small subset of
			 * the total state - so by only setting the required state (such as clear colour,
			 * stencil test, etc) this can result in more optimal performance in certain situations
			 * such as...
			 *
			 *  1) Do some regular drawing with the usual state,
			 *  2) Switch to a render target (resets to the default OpenGL state),
			 *  3) Then one of the first commands issued is 'glClear',
			 *  4) Then a bunch of state is set up,
			 *  5) Then a regular draw command is issued to draw something into the render target.
			 *
			 * ...here the difference in state between 1 and 5 might be minimal requiring a small
			 * amount of state changes. However, if the 'glClear' in 3 applies all state instead of
			 * a subset then the difference in state between 1 and 3 can be large (because 3 is close
			 * to the default state) and the same (inverse) between 3 and 5.
			 * The end result is a lot of unnecessary state changes that weren't needed - they were
			 * applied and then immediately undone.
			 */
			virtual
			void
			draw(
					const GLCapabilities &capabilities,
					const GLState &state_to_apply,
					GLState &last_applied_state) const = 0;
		};

		/**
		 * Used when drawables are added to a render queue instead of being rendered immediately.
		 *
		 * Contains the OpenGL state to be set before drawing and the drawable itself.
		 */
		struct RenderOperation
		{
			RenderOperation(
					GLState::shared_ptr_type state_,
					Drawable::non_null_ptr_to_const_type drawable_,
					bool modifies_frame_buffer_ = true) :
				state(state_),
				drawable(drawable_),
				modifies_frame_buffer(modifies_frame_buffer_)
			{  }

			GLState::shared_ptr_type state;
			Drawable::non_null_ptr_to_const_type drawable;
			bool modifies_frame_buffer;
		};

		/**
		 * A sequence of @a RenderOperation objects batched up in a queue for later rendering.
		 */
		struct RenderQueue :
				public GPlatesUtils::ReferenceCount<RenderQueue>
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<RenderQueue> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const RenderQueue> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new RenderQueue());
			}

			std::vector<RenderOperation> render_operations;

		private:
			RenderQueue()
			{  }
		};

		//! Typedef for a stack of render queues.
		typedef std::stack<RenderQueue::non_null_ptr_type> render_queue_stack_type;

		//! Typedef for a counter for the number of draws to any framebuffer.
		typedef GPlatesUtils::Counter64 frame_buffer_draw_count_type;

		/**
		 * Contains information for a render-to-texture target.
		 */
		struct RenderTextureTarget
		{
			RenderTextureTarget(
					const GLViewport &texture_viewport_,
					const GLTexture::shared_ptr_to_const_type &texture_,
					GLint level_) :
				texture_viewport(texture_viewport_),
				texture(texture_),
				level(level_),
				tile_save_restore_state(false)
			{  }

			GLViewport texture_viewport;
			GLTexture::shared_ptr_to_const_type texture;
			GLint level;

			//! Is true if should save restore state within the current begin/end tile in render target.
			bool tile_save_restore_state;

			// The following parameters are not used if GL_EXT_framebuffer_object extension is available...

			//! Using main framebuffer as a render target.
			struct MainFrameBuffer
			{
				MainFrameBuffer(
						const GLTexture::shared_ptr_to_const_type save_restore_texture_,
						const GLTileRender &tile_render_) :
					save_restore_texture(save_restore_texture_),
					tile_render(tile_render_)
				{  }

				//! Used to save/restore the main framebuffer when it's used as a render target.
				GLTexture::shared_ptr_to_const_type save_restore_texture;

				//! Used to tile rendering when the main framebuffer is smaller than the render target.
				GLTileRender tile_render;
			};

			boost::optional<MainFrameBuffer> main_frame_buffer;
		};

		/**
		 * Contains information for a render target block.
		 */
		struct RenderTargetBlock
		{
			//! Constructor.
			explicit
			RenderTargetBlock(
					const boost::optional<RenderTextureTarget> &render_texture_target_) :
				render_texture_target(render_texture_target_),
				compile_draw_state_nest_count(0)
			{  }


			/**
			 * The render-to-texture target, unless this block represents the main framebuffer.
			 */
			boost::optional<RenderTextureTarget> render_texture_target;

			/**
			 * Stack of currently pushed state blocks.
			 *
			 * The currently active state block is at the top of the stack.
			 */
			state_block_stack_type state_block_stack;

			/**
			 * The number of state blocks that are compiling/recording draw state.
			 */
			unsigned int compile_draw_state_nest_count;

			/**
			 * Stack of currently pushed render queues.
			 *
			 * These are activated by @a begin_render_queue_block / @a end_render_queue_block.
			 *
			 * They allow the delaying of draw operations.
			 */
			render_queue_stack_type render_queue_stack;
		};

		//! Typedef for a stack of render targets.
		typedef std::stack<RenderTargetBlock> render_target_block_stack_type;
	}
}

#endif // GPLATES_OPENGL_GLRENDERERIMPL_H
