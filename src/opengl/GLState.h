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
 
#ifndef GPLATES_OPENGL_GLSTATE_H
#define GPLATES_OPENGL_GLSTATE_H

#include <memory> // For std::unique_ptr
#include <stack>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL1.h>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLFramebuffer.h"
#include "GLSampler.h"
#include "GLStateSetKeys.h"
#include "GLStateSetStore.h"
#include "GLStateStore.h"
#include "GLVertexArray.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"

#include "utils/ObjectPool.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Shadows the current global state of an OpenGL 3.3 (core profile) context.
	 *
	 * A container of @a GLStateSet objects each representing a different part of the global state.
	 *
	 * Any @a GLStateSet slots that are not set represent the default OpenGL state for those slots.
	 *
	 * Note that only commonly used global state is shadowed here (and accessible via class @a GL).
	 * Global state that is not shadowed will need to be modified directly via OpenGL, and hence will
	 * have no save/restore ability.
	 */
	class GLState :
			public GPlatesUtils::ReferenceCount<GLState>
	{
	public:

		//! A convenience typedef for a shared pointer to a @a GLState.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLState object.
		 */
		static
		non_null_ptr_type
		create(
				const GLCapabilities &capabilities,
				const GLStateStore::non_null_ptr_type &state_store)
		{
			return non_null_ptr_type(new GLState(capabilities, state_store));
		}


		/**
		 * Reset the current state to the default OpenGL state.
		 */
		void
		reset_to_default();


		/**
		 * Saves a snapshot of the current state, so it can be restored later with @a restore.
		 *
		 * Each save should be paired with a @a restore, and nesting of save/restore pairs is allowed.
		 */
		void
		save() const;


		/**
		 * Restores the current state to the snapshot saved by the most recent @a save.
		 *
		 * @throws PreconditionViolationError if this method has been called more times than @a save
		 *         (in other words each @a restore should be paired with a corresponding @a save).
		 */
		void
		restore();


		//
		// Set state methods (modifies individual @a GLStateSet objects).
		//


		void
		active_texture(
				GLenum active_texture_)
		{
			set_and_apply_state_set(
					d_state_set_store->active_texture_state_sets,
					GLStateSetKeys::KEY_ACTIVE_TEXTURE,
					boost::in_place(boost::cref(d_capabilities), active_texture_));
		}

		//! Binds the buffer object (at the specified target) to the active OpenGL context.
		void
		bind_buffer(
				GLenum target,
				boost::optional<GLBuffer::shared_ptr_type> buffer)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_buffer_state_sets,
					d_state_set_keys->get_bind_buffer_key(target),
					boost::in_place(target, buffer));
		}

		/**
		 * Binds the target to the framebuffer object on the active OpenGL context
		 * (none and 0 mean bind default framebuffer).
		 */
		void
		bind_framebuffer(
				GLenum target,
				boost::optional<GLFramebuffer::shared_ptr_type> framebuffer,
				// Framebuffer resource handle associated with the current OpenGL context...
				GLuint framebuffer_resource);

		//! Binds the renderbuffer object (at the specified target, must be GL_RENDERBUFFER) to the active OpenGL context.
		void
		bind_renderbuffer(
				GLenum target,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_renderbuffer_state_sets,
					d_state_set_keys->get_bind_renderbuffer_key(target),
					boost::in_place(target, renderbuffer));
		}

		//! Binds the sampler object (at the specified texture unit) to the active OpenGL context.
		void
		bind_sampler(
				GLuint unit,
				boost::optional<GLSampler::shared_ptr_type> sampler)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_sampler_state_sets,
					d_state_set_keys->get_bind_sampler_key(unit),
					boost::in_place(boost::cref(d_capabilities), unit, sampler));
		}

		//! Binds the texture object (at the specified target and texture unit) to the active OpenGL context.
		void
		bind_texture(
				GLenum texture_target,
				GLenum texture_unit,
				boost::optional<GLTexture::shared_ptr_type> texture)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_texture_state_sets,
					d_state_set_keys->get_bind_texture_key(texture_target, texture_unit),
					boost::in_place(boost::cref(d_capabilities), texture_target, texture_unit, texture));
		}

		//! Binds the vertex array object to the active OpenGL context (none and 0 mean unbind).
		void
		bind_vertex_array(
				boost::optional<GLVertexArray::shared_ptr_type> array,
				// Array resource handle associated with the current OpenGL context...
				GLuint array_resource)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_vertex_array_state_sets,
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY,
					boost::in_place(array, array_resource));
		}

		void
		blend_color(
				GLclampf red,
				GLclampf green,
				GLclampf blue,
				GLclampf alpha)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_color_state_sets,
					GLStateSetKeys::KEY_BLEND_COLOR,
					boost::in_place(red, green, blue, alpha));
		}

		void
		blend_equation(
				GLenum mode)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(mode));
		}

		void
		blend_equation_separate(
				GLenum mode_RGB,
				GLenum mode_alpha)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(mode_RGB, mode_alpha));
		}

		void
		blend_func(
				GLenum src,
				GLenum dst)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_func_state_sets,
					GLStateSetKeys::KEY_BLEND_FUNC,
					boost::in_place(GLBlendFuncStateSet::Func{src, dst}));
		}

		void
		blend_func_separate(
				GLenum src_RGB,
				GLenum dst_RGB,
				GLenum src_alpha,
				GLenum dst_alpha)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_func_state_sets,
					GLStateSetKeys::KEY_BLEND_FUNC,
					boost::in_place(
							GLBlendFuncStateSet::Func{src_RGB, dst_RGB},
							GLBlendFuncStateSet::Func{src_alpha, dst_alpha}));
		}

		void
		clamp_color(
				GLenum target,
				GLenum clamp)
		{
			set_and_apply_state_set(
					d_state_set_store->clamp_color_state_sets,
					d_state_set_keys->get_clamp_color_key(target),
					boost::in_place(target, clamp));
		}

		void
		clear_color(
				GLclampf red,
				GLclampf green,
				GLclampf blue,
				GLclampf alpha)
		{
			set_and_apply_state_set(
					d_state_set_store->clear_color_state_sets,
					GLStateSetKeys::KEY_CLEAR_COLOR,
					boost::in_place(red, green, blue, alpha));
		}

		void
		clear_depth(
				GLclampd depth)
		{
			set_and_apply_state_set(
					d_state_set_store->clear_depth_state_sets,
					GLStateSetKeys::KEY_CLEAR_DEPTH,
					boost::in_place(depth));
		}

		void
		clear_stencil(
				GLint stencil)
		{
			set_and_apply_state_set(
					d_state_set_store->clear_stencil_state_sets,
					GLStateSetKeys::KEY_CLEAR_STENCIL,
					boost::in_place(stencil));
		}

		void
		color_mask(
				GLboolean red,
				GLboolean green,
				GLboolean blue,
				GLboolean alpha)
		{
			set_and_apply_state_set(
					d_state_set_store->color_mask_state_sets,
					GLStateSetKeys::KEY_COLOR_MASK,
					boost::in_place(
							boost::cref(d_capabilities),
							GLColorMaskStateSet::Mask{red, green, blue, alpha}));
		}

		void
		color_maski(
				GLuint buf,
				GLboolean red,
				GLboolean green,
				GLboolean blue,
				GLboolean alpha);

		void
		depth_func(
				GLenum func)
		{
			set_and_apply_state_set(
					d_state_set_store->depth_func_state_sets,
					GLStateSetKeys::KEY_DEPTH_FUNC,
					boost::in_place(func));
		}

		void
		cull_face(
				GLenum mode)
		{
			set_and_apply_state_set(
					d_state_set_store->cull_face_state_sets,
					GLStateSetKeys::KEY_CULL_FACE,
					boost::in_place(mode));
		}

		void
		depth_mask(
				GLboolean flag)
		{
			set_and_apply_state_set(
					d_state_set_store->depth_mask_state_sets,
					GLStateSetKeys::KEY_DEPTH_MASK,
					boost::in_place(flag));
		}

		void
		depth_range(
				GLclampd n,
				GLclampd f)
		{
			set_and_apply_state_set(
					d_state_set_store->depth_range_state_sets,
					GLStateSetKeys::KEY_DEPTH_RANGE,
					boost::in_place(boost::cref(d_capabilities), n, f));
		}

		void
		draw_buffer(
				GLenum buf,
				GLenum default_draw_buffer)
		{
			set_and_apply_state_set(
					d_state_set_store->draw_buffers_state_sets,
					GLStateSetKeys::KEY_DRAW_BUFFERS,
					boost::in_place(buf, default_draw_buffer));
		}

		void
		draw_buffers(
				const std::vector<GLenum> &bufs,
				GLenum default_draw_buffer)
		{
			set_and_apply_state_set(
					d_state_set_store->draw_buffers_state_sets,
					GLStateSetKeys::KEY_DRAW_BUFFERS,
					boost::in_place(bufs, default_draw_buffer));
		}

		//! Handles both glEnable and glDisable with @a enable_ parameter.
		void
		enable(
				GLenum cap,
				bool enable_);

		//! Handles both glEnablei and glDisablei with @a enable_ parameter.
		void
		enablei(
				GLenum cap,
				GLuint index,
				bool enable_);

		void
		front_face(
				GLenum dir)
		{
			set_and_apply_state_set(
					d_state_set_store->front_face_state_sets,
					GLStateSetKeys::KEY_FRONT_FACE,
					boost::in_place(dir));
		}

		void
		hint(
				GLenum target,
				GLenum hint_)
		{
			set_and_apply_state_set(
					d_state_set_store->hint_state_sets,
					d_state_set_keys->get_hint_key(target),
					boost::in_place(target, hint_));
		}

		void
		line_width(
				GLfloat width)
		{
			set_and_apply_state_set(
					d_state_set_store->line_width_state_sets,
					GLStateSetKeys::KEY_LINE_WIDTH,
					boost::in_place(width));
		}

		void
		pixel_storef(
				GLenum pname,
				GLfloat param)
		{
			set_and_apply_state_set(
					d_state_set_store->pixel_store_state_sets,
					d_state_set_keys->get_pixel_store_key(pname),
					boost::in_place(pname, param));
		}

		void
		pixel_storei(
				GLenum pname,
				GLint param)
		{
			set_and_apply_state_set(
					d_state_set_store->pixel_store_state_sets,
					d_state_set_keys->get_pixel_store_key(pname),
					boost::in_place(pname, param));
		}

		void
		point_size(
				GLfloat size)
		{
			set_and_apply_state_set(
					d_state_set_store->point_size_state_sets,
					GLStateSetKeys::KEY_POINT_SIZE,
					boost::in_place(size));
		}

		/**
		 * glPolygonMode.
		 *
		 * NOTE: OpenGL 3.3 core requires 'face' (parameter of glPolygonMode) to be 'GL_FRONT_AND_BACK'.
		 */
		void
		polygon_mode(
				GLenum mode)
		{
			set_and_apply_state_set(
					d_state_set_store->polygon_mode_state_sets,
					GLStateSetKeys::KEY_POLYGON_MODE_FRONT_AND_BACK,
					boost::in_place(mode));
		}

		void
		polygon_offset(
				GLfloat factor,
				GLfloat units)
		{
			set_and_apply_state_set(
					d_state_set_store->polygon_offset_state_sets,
					GLStateSetKeys::KEY_POLYGON_OFFSET,
					boost::in_place(factor, units));
		}

		void
		primitive_restart_index(
				GLuint index)
		{
			set_and_apply_state_set(
					d_state_set_store->primitive_restart_index_state_sets,
					GLStateSetKeys::KEY_PRIMITIVE_RESTART_INDEX,
					boost::in_place(index));
		}

		void
		read_buffer(
				GLenum src,
				GLenum default_draw_buffer)
		{
			set_and_apply_state_set(
					d_state_set_store->read_buffer_state_sets,
					GLStateSetKeys::KEY_READ_BUFFER,
					boost::in_place(src, default_draw_buffer));
		}

		void
		sample_coverage(
				GLclampf value,
				GLboolean invert)
		{
			set_and_apply_state_set(
					d_state_set_store->sample_coverage_state_sets,
					GLStateSetKeys::KEY_SAMPLE_COVERAGE,
					boost::in_place(value, invert));
		}

		void
		sample_maski(
				GLuint mask_number,
				GLbitfield mask);

		void
		scissor(
				const GLViewport &scissor,
				// The default viewport is passed in since it can change when the window is resized...
				const GLViewport &default_viewport)
		{
			set_and_apply_state_set(
					d_state_set_store->scissor_state_sets,
					GLStateSetKeys::KEY_SCISSOR,
					boost::in_place(scissor, default_viewport));
		}

		void
		stencil_func(
				GLenum func,
				GLint ref,
				GLuint mask)
		{
			set_and_apply_state_set(
					d_state_set_store->stencil_func_state_sets,
					GLStateSetKeys::KEY_STENCIL_FUNC,
					boost::in_place(GLStencilFuncStateSet::Func{func, ref, mask}));
		}

		void
		stencil_func_separate(
				GLenum face,
				GLenum func,
				GLint ref,
				GLuint mask);

		void
		stencil_mask(
				GLuint mask)
		{
			set_and_apply_state_set(
					d_state_set_store->stencil_mask_state_sets,
					GLStateSetKeys::KEY_STENCIL_MASK,
					boost::in_place(mask/*front and back*/));
		}

		void
		stencil_mask_separate(
				GLenum face,
				GLuint mask);

		void
		stencil_op(
				GLenum sfail,
				GLenum dpfail,
				GLenum dppass)
		{
			set_and_apply_state_set(
					d_state_set_store->stencil_op_state_sets,
					GLStateSetKeys::KEY_STENCIL_OP,
					boost::in_place(GLStencilOpStateSet::Op{sfail, dpfail, dppass}));
		}

		void
		stencil_op_separate(
				GLenum face,
				GLenum sfail,
				GLenum dpfail,
				GLenum dppass);

		void
		use_program(
				boost::optional<GLProgram::shared_ptr_type> program)
		{
			set_and_apply_state_set(
					d_state_set_store->use_program_state_sets,
					GLStateSetKeys::KEY_USE_PROGRAM,
					boost::in_place(program));
		}

		void
		viewport(
				const GLViewport &viewport,
				// The default viewport is passed in since it can change when the window is resized...
				const GLViewport &default_viewport)
		{
			set_and_apply_state_set(
					d_state_set_store->viewport_state_sets,
					GLStateSetKeys::KEY_VIEWPORT,
					boost::in_place(viewport, default_viewport));
		}


		//
		// Get state methods (queries individual @a GLStateSet objects).
		//
		// Note: Unlike the set state methods, these are only provided where needed by the OpenGL framework.
		//


		//! Returns the active texture unit.
		GLenum
		get_active_texture() const
		{
			const boost::optional<GLenum> active_texture_ =
					query_state_set<GLenum>(
							GLStateSetKeys::KEY_ACTIVE_TEXTURE,
							&GLActiveTextureStateSet::d_active_texture);
			// The default of no active texture unit means the default unit GL_TEXTURE0 is active.
			return active_texture_ ? active_texture_.get() : GLCapabilities::gl_TEXTURE0;
		}

		//! Returns the bound buffer object, or boost::none if no object bound.
		boost::optional<GLBuffer::shared_ptr_type>
		get_bind_buffer(
				GLenum target) const
		{
			return query_state_set<GLBuffer::shared_ptr_type>(
					d_state_set_keys->get_bind_buffer_key(target),
					&GLBindBufferStateSet::d_buffer);
		}

		//! Returns the currently bound framebuffer object - boost::none implies the default framebuffer.
		boost::optional<GLFramebuffer::shared_ptr_type>
		get_bind_framebuffer(
				GLenum target) const;

		//! Returns the sampler object bound on the specified texture unit - boost::none implies the default no binding.
		boost::optional<GLSampler::shared_ptr_type>
		get_bind_sampler(
				GLuint unit) const
		{
			return query_state_set<GLSampler::shared_ptr_type>(
					d_state_set_keys->get_bind_sampler_key(unit),
					&GLBindSamplerStateSet::d_sampler);
		}

		//! Returns the texture object bound on the specified target and texture unit - boost::none implies the default no binding.
		boost::optional<GLTexture::shared_ptr_type>
		get_bind_texture(
				GLenum texture_target,
				GLenum texture_unit) const
		{
			return query_state_set<GLTexture::shared_ptr_type>(
					d_state_set_keys->get_bind_texture_key(texture_target, texture_unit),
					&GLBindTextureStateSet::d_texture);
		}

		//! Returns the currently bound vertex array object - boost::none implies the default no binding.
		boost::optional<GLVertexArray::shared_ptr_type>
		get_bind_vertex_array() const
		{
			return query_state_set<GLVertexArray::shared_ptr_type>(
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY,
					&GLBindVertexArrayStateSet::d_array);
		}

		//! Returns the current scissor rectangle.
		const GLViewport &
		get_scissor(
				// The default viewport is passed in since it can change when the window is resized...
				const GLViewport &default_viewport) const
		{
			const boost::optional<const GLViewport &> scissor =
					query_state_set<const GLViewport &>(
							GLStateSetKeys::KEY_SCISSOR,
							&GLScissorStateSet::d_scissor_rectangle);
			return scissor ? scissor.get() : default_viewport;
		}

		//! Returns the program object currently in use - boost::none implies no program in use.
		boost::optional<GLProgram::shared_ptr_type>
		get_use_program() const
		{
			return query_state_set<GLProgram::shared_ptr_type>(
					GLStateSetKeys::KEY_USE_PROGRAM,
					&GLUseProgramStateSet::d_program);
		}

		//! Returns the current viewport.
		const GLViewport &
		get_viewport(
				// The default viewport is passed in since it can change when the window is resized...
				const GLViewport &default_viewport) const
		{
			const boost::optional<const GLViewport &> viewport =
					query_state_set<const GLViewport &>(
							GLStateSetKeys::KEY_VIEWPORT,
							&GLViewportStateSet::d_viewport);
			return viewport ? viewport.get() : default_viewport;
		}

		/**
		 * Returns true if the specified capability is currently enabled
		 * (ie, state associated with glEnable/glDisable and glEnablei/glDisablei).
		 *
		 * If the capability is indexed (eg, GL_BLEND) then @a index can be non-zero.
		 */
		bool
		is_capability_enabled(
				GLenum cap,
				GLuint index = 0) const;

	private:

		/**
		 * Typedef for a group of 32 boolean flags indicating if 32 state set slots have been initialised.
		 */
		typedef boost::uint32_t state_set_slot_flag32_type;

		/**
		 * Typedef for a set of flags indicating a set of state set slots.
		 *
		 * NOTE: This used to be a boost::dynamic_bitset<> but the searching for the next non-zero
		 * flag was consuming too much CPU.
		 * Next a std::vector was used instead because blasting through all ~200 state-sets ended up
		 * being noticeably faster which is important since applying state shows high on the CPU profile
		 * for some rendering paths such as reconstructing rasters with age-grid smoothing.
		 * However the number of slots jumped from ~250 to ~750 when switching from 'old-style'
		 * texture units to 'new-style' texture units (for shaders) increasing the max number of
		 * texture units from 4 to 32 (on latest nVidia hardware).
		 * So next switched to a packed bit flags approach similar to boost::dynamic_bitset but blasting
		 * through our own collection of 32-bit integers instead of the slow searching through the bit
		 * flags done by boost::dynamic_bitset. This enables us to rapidly skip past up to 32 slots
		 * in one test (which is useful considering most of those 32 texture units typically are not
		 * used by clients and hence are empty slots).
		 */
		typedef std::vector<state_set_slot_flag32_type> state_set_slot_flags_type;

		//! Typedef for a state set key.
		typedef GLStateSetKeys::key_type state_set_key_type;

		/**
		 * Typedef for a shared pointer to an immutable @a GLStateSet.
		 *
		 * NOTE: There is only a pointer-to-const since we're treating @a GLStateSet objects
		 * as immutable once created.
		 */
		typedef boost::shared_ptr<const GLStateSet> state_set_ptr_type;

		//! Typedef for a sequence of immutable @a GLStateSet pointers.
		typedef std::vector<state_set_ptr_type> state_set_seq_type;

		/**
		 * A snapshot of the OpenGL state.
		 *
		 * This represents either the current or the state recorded at a @a save.
		 */
		struct Snapshot :
				private boost::noncopyable
		{
		public:
			//
			// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
			// is so these objects can be used with GPlatesUtils::ObjectCache.
			//

			//! A convenience typedef for a shared pointer to a @a Snapshot.
			typedef boost::shared_ptr<Snapshot> shared_ptr_type;
			typedef boost::shared_ptr<const Snapshot> shared_ptr_to_const_type;

			//! A convenience typedef for a weak pointer to a @a Snapshot.
			typedef boost::weak_ptr<Snapshot> weak_ptr_type;
			typedef boost::weak_ptr<const Snapshot> weak_ptr_to_const_type;


			static
			shared_ptr_type
			create(
					const GLStateSetKeys &state_set_keys)
			{
				return shared_ptr_type(new Snapshot(state_set_keys));
			}


			/**
			 * Contains the actual state sets indexed by @a state_set_key_type.
			 *
			 * NOTE: Unused state set slots have NULL pointers.
			 */
			state_set_seq_type state_sets;

			/**
			 * A flag for each state set (indexed by state set key).
			 *
			 * A flag value of true indicates non-null entries in @a d_state_sets.
			 *
			 * WARNING: The number of flags is rounded up to the nearest multiple of 32, so care
			 * should be taken to ensure @a d_state_sets isn't dereferenced beyond the total number
			 * of state-set slots.
			 * We could have added extra code to take care of the last 32 state-set slots but it's
			 * easier to just treat them as flags that are always null.
			 */
			state_set_slot_flags_type state_set_slots;

			/**
			 * The state set slots that have been changed since the last snapshot
			 * (either at the last @a save or the default startup state or no saved snapshots).
			 */
			state_set_slot_flags_type state_set_slots_changed_since_last_snapshot;

		private:

			explicit
			Snapshot(
					const GLStateSetKeys &state_set_keys) :
				state_sets(state_set_keys.get_num_state_set_keys()),
				state_set_slots(GLState::get_num_state_set_slot_flag32s(state_set_keys)),
				state_set_slots_changed_since_last_snapshot(GLState::get_num_state_set_slot_flag32s(state_set_keys))
			{  }
		};

		//! Typedef for a stack of save/restore snapshots.
		typedef std::stack<Snapshot::shared_ptr_type> save_restore_stack_type;


		const GLCapabilities &d_capabilities;

		GLStateSetStore::non_null_ptr_type d_state_set_store;
		GLStateSetKeys::non_null_ptr_to_const_type d_state_set_keys;

		/**
		 * Snapshot representing the current OpenGL state.
		 */
		Snapshot::shared_ptr_type d_current_state;

		/**
		 * Stack of save/restore snapshots.
		 */
		mutable save_restore_stack_type d_save_restore_state;

		/**
		 * Snapshot representing the default OpenGL state.
		 *
		 * Note that the default state is represented by null state set for all slots.
		 */
		Snapshot::shared_ptr_to_const_type d_default_state;


		GLState(
				const GLCapabilities &capabilities,
				const GLStateStore::non_null_ptr_type &state_store);

		/**
		 * Sets a derived @a GLStateSet type at the specified state set key slot, and applies the state.
		 *
		 * The constructor arguments of the derived @a GLStateSet type are passed in
		 * @a state_set_constructor_args and it is created in the @a state_set_pool object pool.
		 */
		template <class GLStateSetType, class InPlaceFactoryType>
		void
		set_and_apply_state_set(
				GPlatesUtils::ObjectPool<GLStateSetType> &state_set_pool,
				state_set_key_type state_set_key,
				const InPlaceFactoryType &state_set_constructor_args)
		{
			state_set_ptr_type current_state_set = d_current_state->state_sets[state_set_key];

			// Create a new GLStateSet of appropriate derived type and store as an immutable state set.
			// If there's an existing state set in the current slot then it gets thrown out.
			//
			// NOTE: The use of boost::in_place_factory means the derived state set object is created
			// directly in the object pool.
			state_set_ptr_type new_state_set = state_set_pool.add_with_auto_release(state_set_constructor_args);

			// Apply the new state set.
			if (current_state_set)
			{
				// Both state sets exist - this is a transition from an existing state to a new (possibly different) state.
				if (!new_state_set->apply_state(d_capabilities, *current_state_set, *this/*current_state*/))
				{
					// Return early if no change was detected (ie, new state matches existing state).
					return;
				}
			}
			else
			{
				// Only the new state set exists - get it to apply its internal state.
				// This is a transition from the default state to a new state.
				if (!new_state_set->apply_from_default_state(d_capabilities, *this/*current_state*/))
				{
					// Return early if no change was detected (ie, new state represents the default state).
					return;
				}
			}

			//
			// If we get here then the new state is different from the existing state, and we need to record this.
			//

			// Store the new state set.
			d_current_state->state_sets[state_set_key] = new_state_set;

			// Mark the state set slot as not empty.
			set_state_set_slot_flag(d_current_state->state_set_slots, state_set_key);
			// Also mark that the state set has changed since the last snapshot.
			set_state_set_slot_flag(d_current_state->state_set_slots_changed_since_last_snapshot, state_set_key);
		}


		/**
		 * Returns a derived @a GLStateSet type at the specified state set key slot.
		 *
		 * This query method returns a const-reference to the GLStateSetType.
		 */
		template <class GLStateSetType>
		boost::optional<const GLStateSetType &>
		query_state_set(
				state_set_key_type state_set_key) const
		{
			// If no state set on the key slot then it means that state is the default state.
			if (!is_state_set_slot_set(d_current_state->state_set_slots, state_set_key))
			{
				return boost::none;
			}

			const GLStateSetType *state_set =
					dynamic_cast<const GLStateSetType *>(d_current_state->state_sets[state_set_key].get());

			// The state set derived type should match the state set slot.
			// If not then there's a programming error somewhere.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					state_set,
					GPLATES_ASSERTION_SOURCE);

			return *state_set;
		}


		/**
		 * Returns a derived @a GLStateSet type at the specified state set key slot.
		 *
		 * This query method accepts a pointer-to-data-member (of class GLStateSetType).
		 */
		template <typename QueryReturnType, class GLStateSetType, class QueryMemberDataType>
		boost::optional<QueryReturnType>
		query_state_set(
				state_set_key_type state_set_key,
				QueryMemberDataType GLStateSetType::*query_member) const
		{
			// If no state set on the key slot then it means that state is the default state.
			if (!is_state_set_slot_set(d_current_state->state_set_slots, state_set_key))
			{
				return boost::none;
			}

			const GLStateSetType *state_set =
					dynamic_cast<const GLStateSetType *>(d_current_state->state_sets[state_set_key].get());

			// The state set derived type should match the state set slot.
			// If not then there's a programming error somewhere.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					state_set,
					GPLATES_ASSERTION_SOURCE);

			return state_set->*query_member;
		}


		/**
		 * Returns a derived @a GLStateSet type at the specified state set key slot.
		 *
		 * This query method accepts a query function (that in turn accepts a 'const GLStateSetType &' argument).
		 */
		template <typename QueryReturnType, class GLStateSetType, class QueryFunctionType>
		boost::optional<QueryReturnType>
		query_state_set(
				state_set_key_type state_set_key,
				const QueryFunctionType &query_function) const
		{
			// If no state set on the key slot then it means that state is the default state.
			if (!is_state_set_slot_set(d_current_state->state_set_slots, state_set_key))
			{
				return boost::none;
			}

			const GLStateSetType *state_set =
					dynamic_cast<const GLStateSetType *>(d_current_state->state_sets[state_set_key].get());

			// The state set derived type should match the state set slot.
			// If not then there's a programming error somewhere.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					state_set,
					GPLATES_ASSERTION_SOURCE);

			return query_function(*state_set);
		}


		/**
		 * Applies @a new_state so that it becomes the current OpenGL state.
		 *
		 * @a state_set_slots_changed identifies which slots have changed between the current and new states.
		 * It is used as an optimisation to avoid checking all slots.
		 */
		void
		apply_state(
				const Snapshot &new_state,
				const state_set_slot_flags_type &state_set_slots_changed);

		/**
		 * Returns an index into @a state_set_slot_flags_type for the specified state set slot.
		 */
		static
		constexpr
		unsigned int
		state_set_slot_flag32_index(
				state_set_key_type state_set_key)
		{
			return state_set_key >> 5;
		}

		/**
		 * Returns an index into @a state_set_slot_flags_type for the specified state set slot.
		 */
		static
		constexpr
		state_set_slot_flag32_type
		state_set_slot_flag32(
				state_set_key_type state_set_key)
		{
			return 1 << (state_set_key & 31);
		}

		/**
		 * Returns true if specified state set slot flag is set.
		 */
		static
		inline
		bool
		is_state_set_slot_set(
				state_set_slot_flags_type &state_set_slots,
				state_set_key_type state_set_key)
		{
			return (state_set_slots[state_set_slot_flag32_index(state_set_key)] & state_set_slot_flag32(state_set_key)) != 0;
		}

		/**
		 * Sets the specified state set slot flag.
		 */
		static
		inline
		void
		set_state_set_slot_flag(
				state_set_slot_flags_type &state_set_slots,
				state_set_key_type state_set_key)
		{
			state_set_slots[state_set_slot_flag32_index(state_set_key)] |= state_set_slot_flag32(state_set_key);
		}

		/**
		 * Clears the specified state set slot flag.
		 */
		static
		inline
		void
		clear_state_set_slot_flag(
				state_set_slot_flags_type &state_set_slots,
				state_set_key_type state_set_key)
		{
			state_set_slots[state_set_slot_flag32_index(state_set_key)] &= ~state_set_slot_flag32(state_set_key);
		}

		/**
		 * Returns the number of groups of 32 state-set slots required.
		 *
		 * Note that this is rounded up to the nearest multiple of 32.
		 */
		static
		unsigned int
		get_num_state_set_slot_flag32s(
				const GLStateSetKeys &state_set_keys)
		{
			// Slot flags go into groups of 32 (since using 32-bit integer bitmasks)...
			return (state_set_keys.get_num_state_set_keys() >> 5) +
				((state_set_keys.get_num_state_set_keys() & 31) != 0 ? 1 : 0);
		}
	};
}

#endif // GPLATES_OPENGL_GLSTATE_H
