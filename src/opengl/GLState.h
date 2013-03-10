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

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>

#include "GLBufferImpl.h"
#include "GLBufferObject.h"
#include "GLCapabilities.h"
#include "GLDepthRange.h"
#include "GLFrameBufferObject.h"
#include "GLStateSetKeys.h"
#include "GLStateSetStore.h"
#include "GLVertexArrayObject.h"
#include "GLVertexBufferObject.h"
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
	 */
	class GLState :
			private boost::noncopyable
	{
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
		 * Constant data that is shared across @a GLState instances.
		 *
		 * This is shared since @a GLState is already reasonably memory intensive due to the
		 * potentially large number of instances that are currently generated.
		 */
		class SharedData
		{
		public:
			typedef boost::shared_ptr<SharedData> shared_ptr_type;
			typedef boost::shared_ptr<const SharedData> shared_ptr_to_const_type;

			/**
			 * Creates a 'const' @a SharedData object.
			 */
			static
			shared_ptr_type
			create(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys,
					const GLState::shared_ptr_type &default_vertex_array_object_current_context_state_)
			{
				return shared_ptr_type(
						new SharedData(
								capabilities,
								state_set_keys,
								default_vertex_array_object_current_context_state_));
			}

		private:
			/**
			 * These slots are used indirectly via other regular GLStateSet's and hence are
			 * treated as special cases.
			 */
			state_set_slot_flags_type dependent_state_set_slots;

			/**
			 * The inverse of @a dependent_state_set_slots (an optimisation).
			 */
			state_set_slot_flags_type inverse_dependent_state_set_slots;

			/**
			 * These slots represent the state that gets recorded into a native vertex array object.
			 */
			state_set_slot_flags_type vertex_array_state_set_slots;

			/**
			 * The inverse of @a vertex_array_state_set_slots (an optimisation).
			 */
			state_set_slot_flags_type inverse_vertex_array_state_set_slots;

			/**
			 * These slots represent states needed by 'glClear'.
			 */
			state_set_slot_flags_type gl_clear_state_set_slots;

			/**
			 * These slots represent states needed by 'glReadPixels'.
			 */
			state_set_slot_flags_type gl_read_pixels_state_set_slots;

			/**
			 * The majority of GLStateSet's are immutable, however a special few are effectively
			 * mutable. Currently this happens if we emulate buffer objects (to make it easier for
			 * clients to use vertex buffers and vertex element buffers since they form the basis
			 * of OpenGL rendering).
			 *
			 * This is boost::none if there are no mutable state set slots.
			 * Currently there are no mutable state sets if 'GL_ARB_vertex_buffer_object' is supported.
			 */
			boost::optional<state_set_slot_flags_type> mutable_state_set_slots;

			/**
			 * The shadowed state of the default vertex array object (resource handle zero).
			 *
			 * This is used to keep track of the vertex array state that OpenGL currently sees.
			 */
			GLState::shared_ptr_type default_vertex_array_object_current_context_state;

			// So can access the shared data.
			friend class GLState;
			friend class GLStateStore;


			explicit
			SharedData(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys,
					const GLState::shared_ptr_type &default_vertex_array_object_current_context_state_);

			//! Initialise state set slots that other state-sets depend upon.
			void
			initialise_dependent_state_set_slots(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys);

			//! Initialise state set slots that represent vertex array state.
			void
			initialise_vertex_array_state_set_slots(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys);

			//! Initialise state set slots representing states needed by 'glClear'.
			void
			initialise_gl_clear_state_set_slots(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys);

			//! Initialise state set slots representing states needed by 'glReadPixels'.
			void
			initialise_gl_read_pixels_state_set_slots(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys);

			/**
			 * Initialise state set slots for those @a GLStateSet objects that are not immutable.
			 *
			 * Most state-sets are immutable objects but a rare few can change state internally and
			 * must be treated as special cases during the state change filtering logic.
			 */
			void
			initialise_mutable_state_set_slots(
					const GLCapabilities &capabilities,
					const GLStateSetKeys &state_set_keys);
		};


		/**
		 * Creates a @a GLState object - call 'GLStateStore::allocate_state()' instead.
		 *
		 * A valid @a state_store enables 'this' object to clone itself more efficiently.
		 */
		static
		shared_ptr_type
		create(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
				const SharedData::shared_ptr_to_const_type &shared_data,
				const boost::weak_ptr<GLStateStore> &state_store = boost::weak_ptr<GLStateStore>())
		{
			return shared_ptr_type(new GLState(state_set_store, state_set_keys, shared_data, state_store));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 *
		 * A valid @a state_store enables 'this' object to clone itself more efficiently.
		 */
		static
		std::auto_ptr<GLState>
		create_as_auto_ptr(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
				const SharedData::shared_ptr_to_const_type &shared_data,
				const boost::weak_ptr<GLStateStore> &state_store = boost::weak_ptr<GLStateStore>())
		{
			return std::auto_ptr<GLState>(new GLState(state_set_store, state_set_keys, shared_data, state_store));
		}


		/**
		 * Creates a copy of this object that shares the same immutable state sets.
		 *
		 * Since the state sets are immutable once created, the clone state cannot be changed
		 * except by calling @a set_state_set (on the cloned @a GLState) to replace an existing
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
		 * Applies the current state (internal to 'this' object).
		 *
		 * @a last_applied_state is the last applied state (ie, it represents the
		 * current state as seen by the OpenGL library).
		 * Only the difference in state needs to be applied to OpenGL.
		 *
		 * Note that upon return, @a last_applied_state will have been modified to be equal to
		 * the internal state of 'this' (which itself is not modified - this is a 'const' method).
		 */
		void
		apply_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		/**
		 * The same as @a apply_state except only those GLStateSet's needed by 'glClear' are applied.
		 */
		void
		apply_state_used_by_gl_clear(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		/**
		 * The same as @a apply_state except only those GLStateSet's needed by 'glReadPixels' are applied.
		 */
		void
		apply_state_used_by_gl_read_pixels(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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


		/**
		 * Copies the state-sets of @a state that represent vertex array state into 'this'.
		 *
		 * This differs from merging in that both non-null *and* null GLStateSet's are copied across
		 * whereas with merging only the non-null GLStateSet's are copied across.
		 * This means if a vertex attribute array pointer GLStateSet, for example, is not present
		 * in @a state then it will also not be present in 'this'.
		 */
		void
		copy_vertex_array_state(
				const GLState &state);


		//
		// The remaining public methods get and set @a GLStateSet type specific state.
		//

		//
		// NOTE: In the state-set specific methods below we are not returning a shared pointer
		// to a @a GLStateSet type because a @a GLStateSet instance cannot live longer than the
		// object pool it was created from (ie, the @a GLStateSetStore we're referencing).
		//

		//! Sets the OpenGL colour mask.
		void
		set_color_mask(
				GLboolean red,
				GLboolean green,
				GLboolean blue,
				GLboolean alpha)
		{
			set_state_set(
					d_state_set_store->color_mask_state_sets,
					GLStateSetKeys::KEY_COLOR_MASK,
					boost::in_place(red, green, blue, alpha));
		}

		//! Sets the OpenGL depth mask.
		void
		set_depth_mask(
				GLboolean flag)
		{
			set_state_set(
					d_state_set_store->depth_mask_state_sets,
					GLStateSetKeys::KEY_DEPTH_MASK,
					boost::in_place(flag));
		}

		//! Returns the current depth mask.
		GLboolean
		get_depth_mask() const
		{
			const boost::optional<GLboolean> depth_mask =
					get_state_set_query<GLboolean>(
							GLStateSetKeys::KEY_DEPTH_MASK,
							&GLDepthMaskStateSet::d_flag);
			return depth_mask ? depth_mask.get() : GL_TRUE/*default*/;
		}

		//! Sets the OpenGL stencil mask.
		void
		set_stencil_mask(
				GLuint stencil)
		{
			set_state_set(
					d_state_set_store->stencil_mask_state_sets,
					GLStateSetKeys::KEY_STENCIL_MASK,
					boost::in_place(stencil));
		}

		//! Sets the OpenGL clear colour.
		void
		set_clear_color(
				GLclampf red,
				GLclampf green,
				GLclampf blue,
				GLclampf alpha)
		{
			set_state_set(
					d_state_set_store->clear_color_state_sets,
					GLStateSetKeys::KEY_CLEAR_COLOR,
					boost::in_place(red, green, blue, alpha));
		}

		//! Sets the OpenGL clear depth value.
		void
		set_clear_depth(
				GLclampd depth)
		{
			set_state_set(
					d_state_set_store->clear_depth_state_sets,
					GLStateSetKeys::KEY_CLEAR_DEPTH,
					boost::in_place(depth));
		}

		//! Sets the OpenGL clear stencil value.
		void
		set_clear_stencil(
				GLint stencil)
		{
			set_state_set(
					d_state_set_store->clear_stencil_state_sets,
					GLStateSetKeys::KEY_CLEAR_STENCIL,
					boost::in_place(stencil));
		}

		//! Sets the framebuffer object to bind to the active OpenGL context.
		void
		set_bind_frame_buffer(
				const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object)
		{
			set_state_set(
					d_state_set_store->bind_frame_buffer_object_state_sets,
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					boost::in_place(frame_buffer_object));
		}

		//! Same as @a set_bind_frame_buffer but also applies directly to OpenGL.
		void
		set_bind_frame_buffer_and_apply(
				const GLCapabilities &capabilities,
				const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object,
				GLState &last_applied_state)
		{
			set_state_set(
					d_state_set_store->bind_frame_buffer_object_state_sets,
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					boost::in_place(frame_buffer_object));
			apply_state(capabilities, last_applied_state, GLStateSetKeys::KEY_BIND_FRAME_BUFFER);
		}

		//! Unbinds any framebuffer object currently bound.
		void
		set_unbind_frame_buffer()
		{
			set_state_set(
					d_state_set_store->bind_frame_buffer_object_state_sets,
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					// Boost 1.34 does not support zero arguments (1.35 does)...
					// Also seems it doesn't like 'boost::none'...
					boost::in_place(boost::optional<GLFrameBufferObject::shared_ptr_to_const_type>()));
		}

		//! Same as @a set_unbind_frame_buffer but also applies directly to OpenGL.
		void
		set_unbind_frame_buffer_and_apply(
				const GLCapabilities &capabilities,
				GLState &last_applied_state)
		{
			set_state_set(
					d_state_set_store->bind_frame_buffer_object_state_sets,
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					// Boost 1.34 does not support zero arguments (1.35 does)...
					// Also seems it doesn't like 'boost::none'...
					boost::in_place(boost::optional<GLFrameBufferObject::shared_ptr_to_const_type>()));
			apply_state(capabilities, last_applied_state, GLStateSetKeys::KEY_BIND_FRAME_BUFFER);
		}

		//! Returns the framebuffer object to bind to the active OpenGL context - boost::none implies default main framebuffer.
		boost::optional<GLFrameBufferObject::shared_ptr_to_const_type>
		get_bind_frame_buffer() const
		{
			return get_state_set_query<GLFrameBufferObject::shared_ptr_to_const_type>(
					GLStateSetKeys::KEY_BIND_FRAME_BUFFER,
					&GLBindFrameBufferObjectStateSet::d_frame_buffer_object);
		}

		//! Sets the shader program object to bind to the active OpenGL context.
		void
		set_bind_program_object(
				const GLProgramObject::shared_ptr_to_const_type &program_object)
		{
			set_state_set(
					d_state_set_store->bind_program_object_state_sets,
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					boost::in_place(program_object));
		}

		//! Same as @a set_bind_program_object but also applies directly to OpenGL.
		void
		set_bind_program_object_and_apply(
				const GLCapabilities &capabilities,
				const GLProgramObject::shared_ptr_to_const_type &program_object,
				GLState &last_applied_state)
		{
			set_state_set(
					d_state_set_store->bind_program_object_state_sets,
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					boost::in_place(program_object));
			apply_state(capabilities, last_applied_state, GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT);
		}

		//! Unbinds any shader program object currently bound.
		void
		set_unbind_program_object()
		{
			set_state_set(
					d_state_set_store->bind_program_object_state_sets,
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					// Boost 1.34 does not support zero arguments (1.35 does)...
					// Also seems it doesn't like 'boost::none'...
					boost::in_place(boost::optional<GLProgramObject::shared_ptr_to_const_type>()));
		}

		//! Same as @a set_unbind_program_object but also applies directly to OpenGL.
		void
		set_unbind_program_object_and_apply(
				const GLCapabilities &capabilities,
				GLState &last_applied_state)
		{
			set_state_set(
					d_state_set_store->bind_program_object_state_sets,
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					// Boost 1.34 does not support zero arguments (1.35 does)...
					// Also seems it doesn't like 'boost::none'...
					boost::in_place(boost::optional<GLProgramObject::shared_ptr_to_const_type>()));
			apply_state(capabilities, last_applied_state, GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT);
		}

		/**
		 * Returns the shader program object bound (or to bind if not yet applied) to the active OpenGL context.
		 *
		 * boost::none implies fixed-function pipeline.
		 */
		boost::optional<GLProgramObject::shared_ptr_to_const_type>
		get_bind_program_object() const
		{
			return get_state_set_query<GLProgramObject::shared_ptr_to_const_type>(
					GLStateSetKeys::KEY_BIND_PROGRAM_OBJECT,
					&GLBindProgramObjectStateSet::d_program_object);
		}

		//! Sets the texture bound on the specified target and texture unit.
		void
		set_bind_texture(
				const GLCapabilities &capabilities,
				const GLTexture::shared_ptr_to_const_type &texture_object,
				GLenum texture_unit,
				GLenum texture_target)
		{
			set_state_set(
					d_state_set_store->bind_texture_state_sets,
					d_state_set_keys->get_bind_texture_key(texture_unit, texture_target),
					boost::in_place(boost::cref(capabilities), texture_object, texture_unit, texture_target));
		}

		//! Same as @a set_bind_texture but also applies directly to OpenGL.
		void
		set_bind_texture_and_apply(
				const GLCapabilities &capabilities,
				const GLTexture::shared_ptr_to_const_type &texture_object,
				GLenum texture_unit,
				GLenum texture_target,
				GLState &last_applied_state)
		{
			const state_set_key_type state_set_key = d_state_set_keys->get_bind_texture_key(texture_unit, texture_target);
			set_state_set(
					d_state_set_store->bind_texture_state_sets,
					state_set_key,
					boost::in_place(boost::cref(capabilities), texture_object, texture_unit, texture_target));
			apply_state(capabilities, last_applied_state, state_set_key);
		}

		//! Unbinds any texture object currently bound to the specified target and texture unit.
		void
		set_unbind_texture(
				const GLCapabilities &capabilities,
				GLenum texture_unit,
				GLenum texture_target)
		{
			set_state_set(
					d_state_set_store->bind_texture_state_sets,
					d_state_set_keys->get_bind_texture_key(texture_unit, texture_target),
					boost::in_place(boost::cref(capabilities), texture_unit, texture_target));
		}

		//! Same as @a set_unbind_texture but also applies directly to OpenGL.
		void
		set_unbind_texture_and_apply(
				const GLCapabilities &capabilities,
				GLenum texture_unit,
				GLenum texture_target,
				GLState &last_applied_state)
		{
			const state_set_key_type state_set_key = d_state_set_keys->get_bind_texture_key(texture_unit, texture_target);
			set_state_set(
					d_state_set_store->bind_texture_state_sets,
					state_set_key,
					boost::in_place(boost::cref(capabilities), texture_unit, texture_target));
			apply_state(capabilities, last_applied_state, state_set_key);
		}

		//! Returns the texture bound on the specified target and texture unit - boost::none implies the default no binding.
		boost::optional<GLTexture::shared_ptr_to_const_type>
		get_bind_texture(
				GLenum texture_unit,
				GLenum texture_target) const
		{
			return get_state_set_query<GLTexture::shared_ptr_to_const_type>(
					d_state_set_keys->get_bind_texture_key(texture_unit, texture_target),
					&GLBindTextureStateSet::d_texture_object);
		}

		//! Binds the vertex array object to the active OpenGL context.
		void
		set_bind_vertex_array_object(
				GLVertexArrayObject::resource_handle_type resource_handle,
				const shared_ptr_type &current_resource_state,
				const shared_ptr_to_const_type &target_resource_state,
				// NOTE: This is *only* here for @a get_bind_vertex_array_object and to keep it alive until rendered.
				// The resource handle is what is used to do the actual binding....
				const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object)
		{
			set_state_set(
					d_state_set_store->bind_vertex_array_object_state_sets,
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT,
					boost::in_place(
							resource_handle,
							current_resource_state,
							d_shared_data->default_vertex_array_object_current_context_state,
							vertex_array_object));
			// Also set the individual state-sets of the buffer bindings, client enable/disable, etc
			// that the vertex array object is supposed to represent which is 'target_resource_state'.
			// What it currently represents is 'current_resource_state'.
			// That way any difference between current and target will get applied if necessary.
			// Most of the time the two states will be in sync and nothing need be done except
			// bind the native vertex array object itself (which is its main purpose).
			merge_state_change(*target_resource_state);
		}

		//! Same as @a set_bind_vertex_array_object but also applies directly to OpenGL.
		void
		set_bind_vertex_array_object_and_apply(
				const GLCapabilities &capabilities,
				GLVertexArrayObject::resource_handle_type resource_handle,
				const shared_ptr_type &current_resource_state,
				const shared_ptr_to_const_type &target_resource_state,
				// NOTE: This is *only* here for @a get_bind_vertex_array_object and to keep it alive until rendered.
				// The resource handle is what is used to do the actual binding....
				const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object,
				GLState &last_applied_state)
		{
			set_bind_vertex_array_object(resource_handle, current_resource_state, target_resource_state, vertex_array_object);
			apply_state(capabilities, last_applied_state, GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT);
		}

		//! Unbinds any vertex array object currently bound.
		void
		set_unbind_vertex_array_object()
		{
			set_state_set(
					d_state_set_store->bind_vertex_array_object_state_sets,
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT,
					boost::in_place(d_shared_data->default_vertex_array_object_current_context_state));
		}

		//! Returns the currently bound vertex array object - boost::none implies the default no binding.
		boost::optional<GLVertexArrayObject::shared_ptr_to_const_type>
		get_bind_vertex_array_object() const
		{
			return get_state_set_query<GLVertexArrayObject::shared_ptr_to_const_type>(
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT,
					&GLBindVertexArrayObjectStateSet::d_vertex_array_object);
		}

		//! Binds the buffer object (at the specified target) to the active OpenGL context.
		void
		set_bind_buffer_object(
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum target)
		{
			set_state_set(
					d_state_set_store->bind_buffer_object_state_sets,
					d_state_set_keys->get_bind_buffer_object_key(target),
					boost::in_place(buffer_object, target));
		}

		//! Same as @a set_bind_buffer_object but also applies directly to OpenGL.
		void
		set_bind_buffer_object_and_apply(
				const GLCapabilities &capabilities,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum target,
				GLState &last_applied_state);

		//! Unbinds any buffer object currently bound at the specified target.
		void
		set_unbind_buffer_object(
				GLenum target)
		{
			set_state_set(
					d_state_set_store->bind_buffer_object_state_sets,
					d_state_set_keys->get_bind_buffer_object_key(target),
					boost::in_place(target));
		}

		//! Same as @a set_unbind_buffer_object but also applies directly to OpenGL.
		void
		set_unbind_buffer_object_and_apply(
				const GLCapabilities &capabilities,
				GLenum target,
				GLState &last_applied_state);

		//! Returns the bound buffer object, or boost::none if no object bound.
		boost::optional<GLBufferObject::shared_ptr_to_const_type>
		get_bind_buffer_object(
				GLenum target) const
		{
			return get_state_set_query<GLBufferObject::shared_ptr_to_const_type>(
					d_state_set_keys->get_bind_buffer_object_key(target),
					&GLBindBufferObjectStateSet::d_buffer_object);
		}

		//! Returns the bound buffer object resource, or boost::none if no object bound.
		boost::optional<GLBufferObject::resource_handle_type>
		get_bind_buffer_object_resource(
				GLenum target) const
		{
			return get_state_set_query<GLBufferObject::resource_handle_type>(
					d_state_set_keys->get_bind_buffer_object_key(target),
					&GLBindBufferObjectStateSet::d_buffer_object_resource);
		}


		/**
		 * Sets all scissor rectangles to the same parameters.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_scissor(
				const GLCapabilities &capabilities,
				const GLViewport &scissor,
				const GLViewport &default_viewport)
		{
			set_state_set(
					d_state_set_store->scissor_state_sets,
					GLStateSetKeys::KEY_SCISSOR,
					boost::in_place(boost::cref(capabilities), scissor, default_viewport));
		}

		/**
		 * Sets all scissor rectangles to the parameters specified in @a all_scissor_rectangles.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_scissor_array(
				const GLCapabilities &capabilities,
				const std::vector<GLViewport> &all_scissor_rectangles,
				const GLViewport &default_viewport)
		{
			set_state_set(
					d_state_set_store->scissor_state_sets,
					GLStateSetKeys::KEY_SCISSOR,
					boost::in_place(boost::cref(capabilities), all_scissor_rectangles, default_viewport));
		}

		/**
		 * Sets all viewports to the same parameters.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_viewport(
				const GLCapabilities &capabilities,
				const GLViewport &viewport,
				const GLViewport &default_viewport)
		{
			set_state_set(
					d_state_set_store->viewport_state_sets,
					GLStateSetKeys::KEY_VIEWPORT,
					boost::in_place(boost::cref(capabilities), viewport, default_viewport));
		}

		/**
		 * Sets all viewports to the parameters specified in @a all_viewports.
		 *
		 * @a default_viewport is the viewport of the window attached to the OpenGL context.
		 */
		void
		set_viewport_array(
				const GLCapabilities &capabilities,
				const std::vector<GLViewport> &all_viewports,
				const GLViewport &default_viewport)
		{
			set_state_set(
					d_state_set_store->viewport_state_sets,
					GLStateSetKeys::KEY_VIEWPORT,
					boost::in_place(boost::cref(capabilities), all_viewports, default_viewport));
		}

		//! Returns the viewport for the specified viewport index.
		boost::optional<const GLViewport &>
		get_viewport(
				const GLCapabilities &capabilities,
				unsigned int viewport_index) const
		{
			const boost::optional<const GLViewport &> viewport =
					get_state_set_query<const GLViewport &, GLViewportStateSet>(
							GLStateSetKeys::KEY_VIEWPORT,
							boost::bind(&GLViewportStateSet::get_viewport,
									_1, boost::cref(capabilities), viewport_index));
			return viewport;
		}


		/**
		 * Sets all depth ranges to the same parameters.
		 */
		void
		set_depth_range(
				const GLCapabilities &capabilities,
				const GLDepthRange &depth_range)
		{
			set_state_set(
					d_state_set_store->depth_range_state_sets,
					GLStateSetKeys::KEY_DEPTH_RANGE,
					boost::in_place(boost::cref(capabilities), depth_range));
		}

		/**
		 * Sets all depth ranges to the parameters specified in @a all_depth_ranges.
		 */
		void
		set_depth_range_array(
				const GLCapabilities &capabilities,
				const std::vector<GLDepthRange> &all_depth_ranges)
		{
			set_state_set(
					d_state_set_store->depth_range_state_sets,
					GLStateSetKeys::KEY_DEPTH_RANGE,
					boost::in_place(boost::cref(capabilities), all_depth_ranges));
		}


		//! Enable/disable a capability - *except* texturing (use @a set_enable_texture for that).
		void
		set_enable(
				GLenum cap,
				bool enable)
		{
			set_state_set(
					d_state_set_store->enable_state_sets,
					d_state_set_keys->get_enable_key(cap),
					boost::in_place(cap, enable));
		}

		//! Returns true if the specified capability is currently enabled.
		bool
		get_enable(
				GLenum cap) const
		{
			const boost::optional<bool> enabled =
					get_state_set_query<bool>(
							d_state_set_keys->get_enable_key(cap),
							&GLEnableStateSet::d_enable);
			return enabled ? enabled.get() : GLEnableStateSet::get_default(cap);
		}


		//! Enable/disable texturing for the specified target and texture unit.
		void
		set_enable_texture(
				GLenum texture_unit,
				GLenum texture_target,
				bool enable)
		{
			set_state_set(
					d_state_set_store->enable_texture_state_sets,
					d_state_set_keys->get_texture_enable_key(texture_unit, texture_target),
					boost::in_place(texture_unit, texture_target, enable));
		}

		//! Specify point size.
		void
		set_point_size(
				GLfloat size)
		{
			set_state_set(
					d_state_set_store->point_size_state_sets,
					GLStateSetKeys::KEY_POINT_SIZE,
					boost::in_place(size));
		}

		//! Specify line width.
		void
		set_line_width(
				GLfloat width)
		{
			set_state_set(
					d_state_set_store->line_width_state_sets,
					GLStateSetKeys::KEY_LINE_WIDTH,
					boost::in_place(width));
		}

		//! Specify polygon mode.
		void
		set_polygon_mode(
				GLenum face,
				GLenum mode)
		{
			if (face == GL_FRONT_AND_BACK)
			{
				// All state sets need to be orthogonal so split into separate front and back states.
				set_state_set(
						d_state_set_store->polygon_mode_state_sets,
						d_state_set_keys->get_polygon_mode_key(GL_FRONT),
						boost::in_place(GL_FRONT, mode));
				set_state_set(
						d_state_set_store->polygon_mode_state_sets,
						d_state_set_keys->get_polygon_mode_key(GL_BACK),
						boost::in_place(GL_BACK, mode));
			}
			else
			{
				set_state_set(
						d_state_set_store->polygon_mode_state_sets,
						d_state_set_keys->get_polygon_mode_key(face),
						boost::in_place(face, mode));
			}
		}

		void
		set_front_face(
				GLenum mode)
		{
			set_state_set(
					d_state_set_store->front_face_state_sets,
					GLStateSetKeys::KEY_FRONT_FACE,
					boost::in_place(mode));
		}

		void
		set_cull_face(
				GLenum mode)
		{
			set_state_set(
					d_state_set_store->cull_face_state_sets,
					GLStateSetKeys::KEY_CULL_FACE,
					boost::in_place(mode));
		}

		void
		set_polygon_offset(
				GLfloat factor,
				GLfloat units)
		{
			set_state_set(
					d_state_set_store->polygon_offset_state_sets,
					GLStateSetKeys::KEY_POLYGON_OFFSET,
					boost::in_place(factor, units));
		}

		//! Specify a hint.
		void
		set_hint(
				GLenum target,
				GLenum mode)
		{
			set_state_set(
					d_state_set_store->hint_state_sets,
					d_state_set_keys->get_hint_key(target),
					boost::in_place(target, mode));
		}

		//! Sets the alpha test function.
		void
		set_alpha_func(
				GLenum func,
				GLclampf ref)
		{
			set_state_set(
					d_state_set_store->alpha_func_state_sets,
					GLStateSetKeys::KEY_ALPHA_FUNC,
					boost::in_place(func, ref));
		}

		//! Sets the alpha-blend equation (glBlendEquation).
		void
		set_blend_equation(
				GLenum mode)
		{
			set_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(mode));
		}

		//! Sets the alpha-blend equation (glBlendEquationSeparate).
		void
		set_blend_equation_separate(
				GLenum modeRGB,
				GLenum modeAlpha)
		{
			set_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(modeRGB, modeAlpha));
		}

		//! Sets the alpha-blend function (glBlendFunc).
		void
		set_blend_func(
				GLenum sfactor,
				GLenum dfactor)
		{
			set_state_set(
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
			set_state_set(
					d_state_set_store->blend_func_state_sets,
					GLStateSetKeys::KEY_BLEND_FUNC,
					boost::in_place(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
		}

		//! Set the depth function.
		void
		set_depth_func(
				GLenum func)
		{
			set_state_set(
					d_state_set_store->depth_func_state_sets,
					GLStateSetKeys::KEY_DEPTH_FUNC,
					boost::in_place(func));
		}


		//! Sets the active texture unit.
		void
		set_active_texture(
				const GLCapabilities &capabilities,
				GLenum active_texture)
		{
			set_state_set(
					d_state_set_store->active_texture_state_sets,
					GLStateSetKeys::KEY_ACTIVE_TEXTURE,
					boost::in_place(boost::cref(capabilities), active_texture));
		}

		//! Returns the active texture unit.
		GLenum
		get_active_texture() const
		{
			const boost::optional<GLenum> active_texture =
					get_state_set_query<GLenum>(
							GLStateSetKeys::KEY_ACTIVE_TEXTURE,
							&GLActiveTextureStateSet::d_active_texture);
			// The default of no active texture unit means the default unit GL_TEXTURE0 is active.
			return active_texture ? active_texture.get() : GLCapabilities::Texture::gl_TEXTURE0;
		}


		//! Sets the client active texture unit.
		void
		set_client_active_texture(
				const GLCapabilities &capabilities,
				GLenum client_active_texture)
		{
			set_state_set(
					d_state_set_store->client_active_texture_state_sets,
					GLStateSetKeys::KEY_CLIENT_ACTIVE_TEXTURE,
					boost::in_place(boost::cref(capabilities), client_active_texture));
		}

		//! Returns the client active texture unit.
		GLenum
		get_client_active_texture() const
		{
			const boost::optional<GLenum> client_active_texture =
					get_state_set_query<GLenum>(
							GLStateSetKeys::KEY_CLIENT_ACTIVE_TEXTURE,
							&GLClientActiveTextureStateSet::d_client_active_texture);
			// The default of no client_active texture unit means the default unit GL_TEXTURE0 is active.
			return client_active_texture ? client_active_texture.get() : GLCapabilities::Texture::gl_TEXTURE0;
		}


		/**
		 * Sets the specified texture environment state to the specified parameter on the specified texture unit.
		 */
		template <typename ParamType>
		void
		set_tex_env(
				GLenum texture_unit,
				GLenum target,
				GLenum pname,
				const ParamType &param)
		{
			set_state_set(
					d_state_set_store->tex_env_state_sets,
					d_state_set_keys->get_tex_env_key(texture_unit, target, pname),
					boost::in_place(texture_unit, target, pname, param));
		}

		/**
		 * Sets the specified texture coordinate generation state to the specified parameter on the specified texture unit.
		 */
		template <typename ParamType>
		void
		set_tex_gen(
				GLenum texture_unit,
				GLenum coord,
				GLenum pname,
				const ParamType &param)
		{
			set_state_set(
					d_state_set_store->tex_gen_state_sets,
					d_state_set_keys->get_tex_gen_key(texture_unit, coord, pname),
					boost::in_place(texture_unit, coord, pname, param));
		}

		/**
		 * Enables the specified (@a array) vertex array (in the fixed-function pipeline).
		 */
		void
		set_enable_client_state(
				GLenum array,
				bool enable)
		{
			set_state_set(
					d_state_set_store->enable_client_state_state_sets,
					d_state_set_keys->get_enable_client_state_key(array),
					boost::in_place(array, enable));
		}

		//! Enables the vertex attribute array GL_TEXTURE_COORD_ARRAY on the specified texture unit.
		void
		set_enable_client_texture_state(
				GLenum texture_unit,
				bool enable)
		{
			set_state_set(
					d_state_set_store->enable_client_texture_state_state_sets,
					d_state_set_keys->get_enable_client_texture_state_key(texture_unit),
					boost::in_place(texture_unit, enable));
		}

		/**
		 * Specify the source of vertex position data (from a buffer object).
		 */
		void
		set_vertex_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object)
		{
			set_state_set(
					d_state_set_store->vertex_pointer_state_sets,
					GLStateSetKeys::KEY_VERTEX_ARRAY_VERTEX_POINTER,
					boost::in_place(size, type, stride, offset, buffer_object));
		}

		/**
		 * Specify the source of vertex position data (from client memory).
		 */
		void
		set_vertex_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl)
		{
			set_state_set(
					d_state_set_store->vertex_pointer_state_sets,
					GLStateSetKeys::KEY_VERTEX_ARRAY_VERTEX_POINTER,
					boost::in_place(size, type, stride, offset, buffer_impl));
		}

		//! Specify the source of vertex color data (from a buffer object).
		void
		set_color_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object)
		{
			set_state_set(
					d_state_set_store->color_pointer_state_sets,
					GLStateSetKeys::KEY_VERTEX_ARRAY_COLOR_POINTER,
					boost::in_place(size, type, stride, offset, buffer_object));
		}

		//! Specify the source of vertex color data (from client memory).
		void
		set_color_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl)
		{
			set_state_set(
					d_state_set_store->color_pointer_state_sets,
					GLStateSetKeys::KEY_VERTEX_ARRAY_COLOR_POINTER,
					boost::in_place(size, type, stride, offset, buffer_impl));
		}

		//! Specify the source of vertex normal data (from a buffer object).
		void
		set_normal_pointer(
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object)
		{
			set_state_set(
					d_state_set_store->normal_pointer_state_sets,
					GLStateSetKeys::KEY_VERTEX_ARRAY_NORMAL_POINTER,
					boost::in_place(type, stride, offset, buffer_object));
		}

		//! Specify the source of vertex normal data (from client memory).
		void
		set_normal_pointer(
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl)
		{
			set_state_set(
					d_state_set_store->normal_pointer_state_sets,
					GLStateSetKeys::KEY_VERTEX_ARRAY_NORMAL_POINTER,
					boost::in_place(type, stride, offset, buffer_impl));
		}

		//! Specify the source of vertex texture coordinate data (from a buffer object).
		void
		set_tex_coord_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum texture_unit)
		{
			set_state_set(
					d_state_set_store->tex_coord_pointer_state_sets,
					d_state_set_keys->get_tex_coord_pointer_state_key(texture_unit),
					boost::in_place(size, type, stride, offset, buffer_object, texture_unit));
		}

		//! Specify the source of vertex texture coordinate data (from client memory).
		void
		set_tex_coord_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl,
				GLenum texture_unit)
		{
			set_state_set(
					d_state_set_store->tex_coord_pointer_state_sets,
					d_state_set_keys->get_tex_coord_pointer_state_key(texture_unit),
					boost::in_place(size, type, stride, offset, buffer_impl, texture_unit));
		}

		//! Enables the specified *generic* vertex attribute array (for use in a shader program).
		void
		set_enable_vertex_attrib_array(
				GLuint attribute_index,
				bool enable)
		{
			set_state_set(
					d_state_set_store->enable_vertex_attrib_array_state_sets,
					d_state_set_keys->get_enable_vertex_attrib_array_key(attribute_index),
					boost::in_place(attribute_index, enable));
		}

		/**
		 * Specify the source of *generic* vertex attribute array (for use in a shader program "from a buffer object").
		 *
		 * Also note that @a set_vertex_pointer maps to @a set_vertex_attrib_pointer with attribute index 0.
		 * So they will both do the same thing.
		 */
		void
		set_vertex_attrib_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object)
		{
			set_state_set(
					d_state_set_store->vertex_attrib_array_state_sets,
					d_state_set_keys->get_vertex_attrib_array_key(attribute_index),
					boost::in_place(
							attribute_index,
							GLVertexAttribPointerStateSet::VERTEX_ATTRIB_POINTER,
							size,
							type,
							normalized,
							stride,
							offset,
							buffer_object));
		}

		/**
		 * Specify the source of *generic* vertex attribute array (for use in a shader program "from client memory").
		 *
		 * Also note that @a set_vertex_pointer maps to @a set_vertex_attrib_pointer with attribute index 0.
		 * So they will both do the same thing.
		 */
		void
		set_vertex_attrib_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl)
		{
			set_state_set(
					d_state_set_store->vertex_attrib_array_state_sets,
					d_state_set_keys->get_vertex_attrib_array_key(attribute_index),
					boost::in_place(
							attribute_index,
							GLVertexAttribPointerStateSet::VERTEX_ATTRIB_POINTER,
							size,
							type,
							normalized,
							stride,
							offset,
							buffer_impl));
		}

		/**
		 * Same as @a set_vertex_attrib_pointer except used to specify attributes mapping to *integer* shader variables.
		 */
		void
		set_vertex_attrib_i_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object)
		{
			set_state_set(
					d_state_set_store->vertex_attrib_array_state_sets,
					d_state_set_keys->get_vertex_attrib_array_key(attribute_index),
					boost::in_place(
							attribute_index,
							GLVertexAttribPointerStateSet::VERTEX_ATTRIB_I_POINTER,
							size,
							type,
							boost::none/*normalized*/,
							stride,
							offset,
							buffer_object));
		}

		/**
		 * Same as @a set_vertex_attrib_pointer except used to specify attributes mapping to *integer* shader variables.
		 */
		void
		set_vertex_attrib_i_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl)
		{
			set_state_set(
					d_state_set_store->vertex_attrib_array_state_sets,
					d_state_set_keys->get_vertex_attrib_array_key(attribute_index),
					boost::in_place(
							attribute_index,
							GLVertexAttribPointerStateSet::VERTEX_ATTRIB_I_POINTER,
							size,
							type,
							boost::none/*normalized*/,
							stride,
							offset,
							buffer_impl));
		}

		/**
		 * Same as @a set_vertex_attrib_pointer except used to specify attributes mapping to *double* shader variables.
		 */
		void
		set_vertex_attrib_l_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object)
		{
			set_state_set(
					d_state_set_store->vertex_attrib_array_state_sets,
					d_state_set_keys->get_vertex_attrib_array_key(attribute_index),
					boost::in_place(
							attribute_index,
							GLVertexAttribPointerStateSet::VERTEX_ATTRIB_L_POINTER,
							size,
							type,
							boost::none/*normalized*/,
							stride,
							offset,
							buffer_object));
		}

		/**
		 * Same as @a set_vertex_attrib_pointer except used to specify attributes mapping to *double* shader variables.
		 */
		void
		set_vertex_attrib_l_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl)
		{
			set_state_set(
					d_state_set_store->vertex_attrib_array_state_sets,
					d_state_set_keys->get_vertex_attrib_array_key(attribute_index),
					boost::in_place(
							attribute_index,
							GLVertexAttribPointerStateSet::VERTEX_ATTRIB_L_POINTER,
							size,
							type,
							boost::none/*normalized*/,
							stride,
							offset,
							buffer_impl));
		}


		//! Specifies which matrix stack is the target for matrix operations.
		void
		set_matrix_mode(
				GLenum mode)
		{
			set_state_set(
					d_state_set_store->matrix_mode_state_sets,
					GLStateSetKeys::KEY_MATRIX_MODE,
					boost::in_place(mode));
		}

		//! Returns the matrix stack targeted for matrix operations.
		GLenum
		get_matrix_mode() const
		{
			const boost::optional<GLenum> mode =
					get_state_set_query<GLenum>(
							GLStateSetKeys::KEY_MATRIX_MODE,
							&GLMatrixModeStateSet::d_mode);
			// The default of no matrix mode means GL_MODELVIEW.
			return mode ? mode.get() : GL_MODELVIEW;
		}

		//! Loads the specified matrix into the specified matrix mode.
		void
		set_load_matrix(
				GLenum mode,
				const GLMatrix &matrix)
		{
			set_state_set(
					d_state_set_store->load_matrix_state_sets,
					d_state_set_keys->get_load_matrix_key(mode),
					boost::in_place(mode, matrix));
		}

		//! Returns the matrix for the specified matrix mode.
		boost::optional<const GLMatrix &>
		get_load_matrix(
				GLenum mode) const
		{
			const boost::optional<const GLMatrix &> matrix =
					get_state_set_query<const GLMatrix &>(
							d_state_set_keys->get_load_matrix_key(mode),
							&GLLoadMatrixStateSet::d_matrix);
			return matrix;
		}

		//! Loads the specified texture matrix into the specified texture unit.
		void
		set_load_texture_matrix(
				GLenum texture_unit,
				const GLMatrix &matrix)
		{
			set_state_set(
					d_state_set_store->load_texture_matrix_state_sets,
					d_state_set_keys->get_load_texture_matrix_key(texture_unit),
					boost::in_place(texture_unit, matrix));
		}

		//! Returns the texture matrix for the specified texture unit.
		boost::optional<const GLMatrix &>
		get_load_texture_matrix(
				GLenum texture_unit) const
		{
			const boost::optional<const GLMatrix &> texture_matrix =
					get_state_set_query<const GLMatrix &>(
							d_state_set_keys->get_load_texture_matrix_key(texture_unit),
							&GLLoadTextureMatrixStateSet::d_matrix);
			return texture_matrix;
		}

	private:
		//! Typedef for a state set key.
		typedef GLStateSetKeys::key_type state_set_key_type;

		/**
		 * Typedef for a shared pointer to an immutable @a GLStateSet.
		 *
		 * NOTE: There is only a pointer-to-const since we're treating @a GLStateSet objects
		 * as immutable once created.
		 */
		typedef boost::shared_ptr<const GLStateSet> immutable_state_set_ptr_type;

		//! Typedef for a sequence of immutable @a GLStateSet pointers.
		typedef std::vector<immutable_state_set_ptr_type> state_set_seq_type;


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
		 * WARNING: The number of flags is rounded up to the nearest multiple of 32 so care
		 * should be taken to ensure @a d_state_sets isn't dereferenced beyond the total number
		 * of state-set slots.
		 * We could have added extra code to take care of the last 32 state-set slots but it's
		 * easier to just treat them as flags that are always null.
		 */
		state_set_slot_flags_type d_state_set_slots;

		/**
		 * Constant data that is shared by all @a GLState instances (allocated by our state store).
		 *
		 * Note that it can't be 'static' because it depends on the number of state set keys which
		 * depends on the OpenGL extensions supported by the runtime system.
		 */
		SharedData::shared_ptr_to_const_type d_shared_data;


		//! Default constructor.
		GLState(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
				const SharedData::shared_ptr_to_const_type &shared_data,
				const boost::weak_ptr<GLStateStore> &state_store);

		/**
		 * Sets a derived @a GLStateSet type at the specified state set key slot.
		 *
		 * The constructor arguments of the derived @a GLStateSet type are passed in
		 * @a state_set_constructor_args and it is created in the @a state_set_pool object pool.
		 */
		template <class GLStateSetType, class InPlaceFactoryType>
		void
		set_state_set(
				GPlatesUtils::ObjectPool<GLStateSetType> &state_set_pool,
				state_set_key_type state_set_key,
				const InPlaceFactoryType &state_set_constructor_args)
		{
			// Create a new GLStateSet of appropriate derived type and store as an immutable state set.
			// If there's an existing state set in the current slot then it gets thrown out.
			//
			// NOTE: The use of boost::in_place_factory means the derived state set object is created
			// directly in the object pool.
			d_state_sets[state_set_key] = state_set_pool.add_with_auto_release(state_set_constructor_args);

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
		get_state_set_query(
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
		get_state_set_query(
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
		 * Applies 'this' state (from @a last_applied_state) for the specified state-set slots.
		 */
		void
		apply_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state,
				const state_set_slot_flags_type &state_set_slots_mask) const;

		/**
		 * Applies 'this' state (from @a last_applied_state) for the specified *single* state-set slot.
		 */
		void
		apply_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state,
				state_set_key_type state_set_slot_to_apply) const;

		/**
		 * Bind (or unbind) a vertex array object if necessary.
		 */
		void
		begin_bind_vertex_array_object(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		/**
		 * Update the shadowed state of the currently bound vertex array object to mirror
		 * any vertex array state set after @a begin_bind_vertex_array_object.
		 */
		void
		end_bind_vertex_array_object(
				GLState &last_applied_state) const;

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
