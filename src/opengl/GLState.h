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
#include <vector>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL1.h>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLDepthRange.h"
#include "GLFrameBufferObject.h"
#include "GLStateSetKeys.h"
#include "GLStateSetStore.h"
#include "GLVertexArray.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"

#include "utils/ObjectPool.h"


namespace GPlatesOpenGL
{
	class GLStateStore;

	/**
	 * Contains a snapshot of the global state of OpenGL.
	 *
	 * This is represented as a container of @a GLStateSet objects each representing a different
	 * part of the global state.
	 *
	 * Any @a GLStateSet slots that are not set represent the default OpenGL state for those slots.
	 *
	 * Note that only commonly used global state is shadowed here (and accessible via class @a GL).
	 * Global state that is not shadowed will need to be modified directly via OpenGL, and hence will
	 * have no save/restore ability (via GL::StateScope).
	 */
	class GLState :
			private boost::noncopyable
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLState.
		typedef boost::shared_ptr<GLState> shared_ptr_type;
		typedef boost::shared_ptr<const GLState> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLState.
		typedef boost::weak_ptr<GLState> weak_ptr_type;
		typedef boost::weak_ptr<const GLState> weak_ptr_to_const_type;


		/**
		 * Creates a copy of this object that shares the same immutable state sets.
		 *
		 * Since the state sets are immutable once created, the clone state cannot be changed except
		 * by calling @a set_and_apply_state_set (on the cloned @a GLState) to replace an existing
		 * @a GLStateSet with a newly created one.
		 */
		shared_ptr_type
		clone() const;


		/**
		 * Clears references to @a GLStateSet objects such that 'this' object behaves the
		 * same as a newly allocated @a GLState object.
		 */
		void
		clear();


		/**
		 * Applies the state in 'this' object.
		 *
		 * @a current_state represents the current state as seen by OpenGL.
		 * Only the difference in state needs to be applied to OpenGL.
		 *
		 * Note that upon return, @a current_state will have been modified to be equal to
		 * the internal state of 'this' (which itself is not modified - this is a 'const' method).
		 */
		void
		apply_state(
				GLState &current_state) const;


		/**
		 * Merges the specified state change into 'this' state.
		 *
		 * Only those states that have been set on @a state_change are merged.
		 * Any states that have not been set on @a state_changed are ignored.
		 * This is consistent with a state *change* rather than a *full* state.
		 */
		void
		merge_state_change(
				const GLState &state_change);


		//
		// Set state methods.
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

		//! Binds the texture object (at the specified target and texture unit) to the active OpenGL context.
		void
		bind_texture(
				GLenum texture_target,
				GLenum texture_unit,
				boost::optional<GLTexture::shared_ptr_type> texture_object)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_texture_state_sets,
					d_state_set_keys->get_bind_texture_key(texture_target, texture_unit),
					boost::in_place(boost::cref(d_capabilities), texture_target, texture_unit, texture_object));
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
					boost::in_place(red, green, blue, alpha));
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
		disable(
				GLenum cap)
		{
			set_and_apply_state_set(
					d_state_set_store->enable_state_sets,
					d_state_set_keys->get_enable_key(cap),
					boost::in_place(cap, false/*enable*/));
		}

		void
		enable(
				GLenum cap)
		{
			set_and_apply_state_set(
					d_state_set_store->enable_state_sets,
					d_state_set_keys->get_enable_key(cap),
					boost::in_place(cap, true/*enable*/));
		}

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
		stencil_mask(
				GLuint mask)
		{
			set_and_apply_state_set(
					d_state_set_store->stencil_mask_state_sets,
					GLStateSetKeys::KEY_STENCIL_MASK,
					boost::in_place(mask));
		}

		//! Sets the framebuffer object to bind to the active OpenGL context.
		void
		set_bind_frame_buffer(
				const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_frame_buffer_object_state_sets,
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					boost::in_place(frame_buffer_object));
		}

		//! Unbinds any framebuffer object currently bound.
		void
		set_unbind_frame_buffer()
		{
			set_and_apply_state_set(
					d_state_set_store->bind_frame_buffer_object_state_sets,
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					// Seems it doesn't like 'boost::none'...
					boost::in_place(boost::optional<GLFrameBufferObject::shared_ptr_to_const_type>()));
		}

		//! Returns the framebuffer object to bind to the active OpenGL context - boost::none implies default main framebuffer.
		boost::optional<GLFrameBufferObject::shared_ptr_to_const_type>
		get_bind_frame_buffer() const
		{
			return query_state_set<GLFrameBufferObject::shared_ptr_to_const_type>(
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					&GLBindFrameBufferObjectStateSet::d_frame_buffer_object);
		}

		//! Sets the shader program object to bind to the active OpenGL context.
		void
		set_bind_program_object(
				const GLProgramObject::shared_ptr_to_const_type &program_object)
		{
			set_and_apply_state_set(
					d_state_set_store->bind_program_object_state_sets,
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					boost::in_place(program_object));
		}

		//! Unbinds any shader program object currently bound.
		void
		set_unbind_program_object()
		{
			set_and_apply_state_set(
					d_state_set_store->bind_program_object_state_sets,
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					// Seems it doesn't like 'boost::none'...
					boost::in_place(boost::optional<GLProgramObject::shared_ptr_to_const_type>()));
		}

		/**
		 * Returns the shader program object bound (or to bind if not yet applied) to the active OpenGL context.
		 *
		 * boost::none implies fixed-function pipeline.
		 */
		boost::optional<GLProgramObject::shared_ptr_to_const_type>
		get_bind_program_object() const
		{
			return query_state_set<GLProgramObject::shared_ptr_to_const_type>(
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					&GLBindProgramObjectStateSet::d_program_object);
		}


		/**
		 * Sets all scissor rectangles to the same parameters.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_scissor(
				const GLViewport &scissor,
				const GLViewport &default_viewport)
		{
			set_and_apply_state_set(
					d_state_set_store->scissor_state_sets,
					GLStateSetKeys::KEY_SCISSOR,
					boost::in_place(boost::cref(d_capabilities), scissor, default_viewport));
		}

		/**
		 * Sets all scissor rectangles to the parameters specified in @a all_scissor_rectangles.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_scissor_array(
				const std::vector<GLViewport> &all_scissor_rectangles,
				const GLViewport &default_viewport)
		{
			set_and_apply_state_set(
					d_state_set_store->scissor_state_sets,
					GLStateSetKeys::KEY_SCISSOR,
					boost::in_place(boost::cref(d_capabilities), all_scissor_rectangles, default_viewport));
		}

		//! Returns the scissor rectangle for the specified viewport index.
		boost::optional<const GLViewport &>
		get_scissor(
				unsigned int viewport_index) const
		{
			const boost::optional<const GLViewport &> scissor =
					query_state_set<const GLViewport &, GLScissorStateSet>(
							GLStateSetKeys::KEY_SCISSOR,
							boost::bind(&GLScissorStateSet::get_scissor,
									_1, boost::cref(d_capabilities), viewport_index));
			return scissor;
		}

		/**
		 * Sets all viewports to the same parameters.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_viewport(
				const GLViewport &viewport,
				const GLViewport &default_viewport)
		{
			set_and_apply_state_set(
					d_state_set_store->viewport_state_sets,
					GLStateSetKeys::KEY_VIEWPORT,
					boost::in_place(boost::cref(d_capabilities), viewport, default_viewport));
		}

		/**
		 * Sets all viewports to the parameters specified in @a all_viewports.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_viewport_array(
				const std::vector<GLViewport> &all_viewports,
				const GLViewport &default_viewport)
		{
			set_and_apply_state_set(
					d_state_set_store->viewport_state_sets,
					GLStateSetKeys::KEY_VIEWPORT,
					boost::in_place(boost::cref(d_capabilities), all_viewports, default_viewport));
		}

		//! Returns the viewport for the specified viewport index.
		boost::optional<const GLViewport &>
		get_viewport(
				unsigned int viewport_index) const
		{
			const boost::optional<const GLViewport &> viewport =
					query_state_set<const GLViewport &, GLViewportStateSet>(
							GLStateSetKeys::KEY_VIEWPORT,
							boost::bind(&GLViewportStateSet::get_viewport,
									_1, boost::cref(d_capabilities), viewport_index));
			return viewport;
		}


		/**
		 * Sets all depth ranges to the same parameters.
		 */
		void
		set_depth_range(
				const GLDepthRange &depth_range)
		{
			set_and_apply_state_set(
					d_state_set_store->depth_range_state_sets,
					GLStateSetKeys::KEY_DEPTH_RANGE,
					boost::in_place(boost::cref(d_capabilities), depth_range));
		}

		/**
		 * Sets all depth ranges to the parameters specified in @a all_depth_ranges.
		 */
		void
		set_depth_range_array(
				const std::vector<GLDepthRange> &all_depth_ranges)
		{
			set_and_apply_state_set(
					d_state_set_store->depth_range_state_sets,
					GLStateSetKeys::KEY_DEPTH_RANGE,
					boost::in_place(boost::cref(d_capabilities), all_depth_ranges));
		}


		/**
		 * Sets the stencil function.
		 */
		void
		set_stencil_func(
				GLenum func,
				GLint ref,
				GLuint mask)
		{
			set_and_apply_state_set(
					d_state_set_store->stencil_func_state_sets,
					GLStateSetKeys::KEY_STENCIL_FUNC,
					boost::in_place(func, ref, mask));
		}


		/**
		 * Sets the stencil operation.
		 */
		void
		set_stencil_op(
				GLenum fail,
				GLenum zfail,
				GLenum zpass)
		{
			set_and_apply_state_set(
					d_state_set_store->stencil_op_state_sets,
					GLStateSetKeys::KEY_STENCIL_OP,
					boost::in_place(fail, zfail, zpass));
		}

		void
		set_polygon_offset(
				GLfloat factor,
				GLfloat units)
		{
			set_and_apply_state_set(
					d_state_set_store->polygon_offset_state_sets,
					GLStateSetKeys::KEY_POLYGON_OFFSET,
					boost::in_place(factor, units));
		}

		//! Sets the alpha-blend equation (glBlendEquation).
		void
		set_blend_equation(
				GLenum mode)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(boost::cref(d_capabilities), mode));
		}

		//! Sets the alpha-blend equation (glBlendEquationSeparate).
		void
		set_blend_equation_separate(
				GLenum modeRGB,
				GLenum modeAlpha)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(boost::cref(d_capabilities), modeRGB, modeAlpha));
		}

		//! Sets the alpha-blend function (glBlendFunc).
		void
		set_blend_func(
				GLenum sfactor,
				GLenum dfactor)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_func_state_sets,
					GLStateSetKeys::KEY_BLEND_FUNC,
					boost::in_place(sfactor, dfactor));
		}

		//! Sets the alpha-blend function (glBlendFuncSeparate).
		void
		set_blend_func_separate(
				GLenum sfactorRGB,
				GLenum dfactorRGB,
				GLenum sfactorAlpha,
				GLenum dfactorAlpha)
		{
			set_and_apply_state_set(
					d_state_set_store->blend_func_state_sets,
					GLStateSetKeys::KEY_BLEND_FUNC,
					boost::in_place(boost::cref(d_capabilities), sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
		}

		//! Set the depth function.
		void
		set_depth_func(
				GLenum func)
		{
			set_and_apply_state_set(
					d_state_set_store->depth_func_state_sets,
					GLStateSetKeys::KEY_DEPTH_FUNC,
					boost::in_place(func));
		}


		//
		// Get state methods.
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
			return active_texture_ ? active_texture_.get() : GLCapabilities::Texture::gl_TEXTURE0;
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

		//! Returns the texture bound on the specified target and texture unit - boost::none implies the default no binding.
		boost::optional<GLTexture::shared_ptr_type>
		get_bind_texture(
				GLenum texture_target,
				GLenum texture_unit) const
		{
			return query_state_set<GLTexture::shared_ptr_type>(
					d_state_set_keys->get_bind_texture_key(texture_target, texture_unit),
					&GLBindTextureStateSet::d_texture_object);
		}

		//! Returns the currently bound vertex array object - boost::none implies the default no binding.
		boost::optional<GLVertexArray::shared_ptr_type>
		get_bind_vertex_array() const
		{
			return query_state_set<GLVertexArray::shared_ptr_type>(
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY,
					&GLBindVertexArrayStateSet::d_array);
		}

		//! Returns true if the specified capability is currently enabled.
		bool
		get_enable(
				GLenum cap) const
		{
			const boost::optional<bool> enabled =
					query_state_set<bool>(
							d_state_set_keys->get_enable_key(cap),
							&GLEnableStateSet::d_enable);
			return enabled ? enabled.get() : GLEnableStateSet::get_default(cap);
		}


	public: // For use by GLStateStore ...

		/**
		 * Creates a @a GLState object - call 'GLStateStore::allocate_state()' instead.
		 *
		 * A valid @a state_store enables 'this' object to clone itself more efficiently.
		 */
		static
		shared_ptr_type
		create(
				const GLCapabilities &capabilities,
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
				const boost::weak_ptr<GLStateStore> &state_store = boost::weak_ptr<GLStateStore>())
		{
			return shared_ptr_type(new GLState(capabilities, state_set_store, state_set_keys, state_store));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 *
		 * A valid @a state_store enables 'this' object to clone itself more efficiently.
		 */
		static
		std::unique_ptr<GLState>
		create_unique(
				const GLCapabilities &capabilities,
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
				const boost::weak_ptr<GLStateStore> &state_store = boost::weak_ptr<GLStateStore>())
		{
			return std::unique_ptr<GLState>(new GLState(capabilities, state_set_store, state_set_keys, state_store));
		}

	private:

		/**
		 * Typedef for a group of 32 boolean flags indicating if 32 state set slots have been initialised.
		 */
		typedef boost::uint32_t state_set_slot_flag32_type;

		/**
		 * Typedef for a set of flags indicating which state set slots have been initialised.
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

		const GLCapabilities &d_capabilities;

		GLStateSetStore::non_null_ptr_type d_state_set_store;
		GLStateSetKeys::non_null_ptr_to_const_type d_state_set_keys;

		/**
		 * Used to efficiently allocate new @a GLState objects when cloning.
		 *
		 * NOTE: It's a weak reference to avoid circular shared pointers (memory island leak).
		 * If we're asked to clone ourself *after* the state store is destroyed then we'll just
		 * allocate on the heap instead of via the state store.
		 */
		boost::weak_ptr<GLStateStore> d_state_store;

		/**
		 * Contains the actual state sets indexed by @a state_set_key_type.
		 *
		 * NOTE: Unused state set slots have NULL pointers.
		 */
		state_set_seq_type d_state_sets;

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
		state_set_slot_flags_type d_state_set_slots;


		//! Default constructor.
		GLState(
				const GLCapabilities &capabilities,
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
				const boost::weak_ptr<GLStateStore> &state_store);

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
			state_set_ptr_type current_state_set = d_state_sets[state_set_key];

			// Create a new GLStateSet of appropriate derived type and store as an immutable state set.
			// If there's an existing state set in the current slot then it gets thrown out.
			//
			// NOTE: The use of boost::in_place_factory means the derived state set object is created
			// directly in the object pool.
			state_set_ptr_type new_state_set = state_set_pool.add_with_auto_release(state_set_constructor_args);

			// Apply the new state set.
			if (current_state_set)
			{
				// Both state sets exist - this is a transition from an existing state to another
				// (possibly different) existing state - if the two states are the same then it's
				// possible for this to do nothing.
				new_state_set->apply_state(d_capabilities, *current_state_set, *this/*current_state*/);
			}
			else
			{
				// Only the new state set exists - get it to apply its internal state.
				// This is a transition from the default state to a new state.
				new_state_set->apply_from_default_state(d_capabilities, *this/*current_state*/);
			}

			// Store the new state set.
			d_state_sets[state_set_key] = new_state_set;

			// Mark the state set slot as not empty.
			// This is a slightly optimised version of 'set_state_set_slot_flag()'.
			d_state_set_slots[state_set_key >> 5] |= (1 << (state_set_key & 31));
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
			// This is a slightly optimised version of 'is_state_set_slot_set()'.
			if ((d_state_set_slots[state_set_key >> 5] & (1 << (state_set_key & 31))) == 0)
			{
				return boost::none;
			}

			const GLStateSetType *state_set =
					dynamic_cast<const GLStateSetType *>(d_state_sets[state_set_key].get());

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
		 * This query method accepts a query function (that in turn accepts a 'GLStateSetType' argument).
		 */
		template <typename QueryReturnType, class GLStateSetType, class QueryFunctionType>
		boost::optional<QueryReturnType>
		query_state_set(
				state_set_key_type state_set_key,
				const QueryFunctionType &query_function) const
		{
			// If no state set on the key slot then it means that state is the default state.
			// This is a slightly optimised version of 'is_state_set_slot_set()'.
			if ((d_state_set_slots[state_set_key >> 5] & (1 << (state_set_key & 31))) == 0)
			{
				return boost::none;
			}

			const GLStateSetType *state_set =
					dynamic_cast<const GLStateSetType *>(d_state_sets[state_set_key].get());

			// The state set derived type should match the state set slot.
			// If not then there's a programming error somewhere.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					state_set,
					GPLATES_ASSERTION_SOURCE);

			return query_function(state_set);
		}


		/**
		 * Applies 'this' state (from @a current_state) for the specified state-set slots (in the mask).
		 */
		void
		apply_state(
				GLState &current_state,
				const state_set_slot_flags_type &state_set_slots_mask) const;

		/**
		 * Returns the number of groups of 32 state-set slots required.
		 *
		 * Note that this is rounded up to the nearest multiple of 32.
		 */
		static
		unsigned int
		get_num_state_set_slot_flag32s(
				const GLStateSetKeys &state_set_keys);

		/**
		 * Returns true if the specified state set slot flag is set.
		 */
		static
		bool
		is_state_set_slot_set(
				state_set_slot_flags_type &state_set_slots,
				state_set_key_type state_set_slot);

		/**
		 * Sets the specified state set slot flag.
		 */
		static
		void
		set_state_set_slot_flag(
				state_set_slot_flags_type &state_set_slots,
				state_set_key_type state_set_slot);

		/**
		 * Clears the specified state set slot flag.
		 */
		static
		void
		clear_state_set_slot_flag(
				state_set_slot_flags_type &state_set_slots,
				state_set_key_type state_set_slot);
	};
}

#endif // GPLATES_OPENGL_GLSTATE_H
