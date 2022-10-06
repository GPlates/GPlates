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
#include <unordered_map>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <qopengl.h>  // For OpenGL constants and typedefs.

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
	class OpenGLFunctions;

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
				OpenGLFunctions &opengl_functions,
				const GLCapabilities &capabilities,
				const GLStateStore::non_null_ptr_type &state_store)
		{
			return non_null_ptr_type(new GLState(opengl_functions, capabilities, state_store));
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
		save();


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


		//! Binds the buffer object (at the specified target) to the active OpenGL context.
		void
		bind_buffer(
				GLenum target,
				boost::optional<GLBuffer::shared_ptr_type> buffer);

		//! Binds the entire buffer object (at the specified *indexed* target) to the active OpenGL context.
		void
		bind_buffer_base(
				GLenum target,
				GLuint index,
				boost::optional<GLBuffer::shared_ptr_type> buffer);

		//! Binds a sub-range of the buffer object (at the specified *indexed* target) to the active OpenGL context.
		void
		bind_buffer_range(
				GLenum target,
				GLuint index,
				boost::optional<GLBuffer::shared_ptr_type> buffer,
				GLintptr offset,
				GLsizeiptr size);

		/**
		 * Binds the framebuffer object (at the specified target) to the active OpenGL context
		 * (none means bind default framebuffer).
		 */
		void
		bind_framebuffer(
				GLenum target,
				boost::optional<GLFramebuffer::shared_ptr_type> framebuffer,
				// Default framebuffer resource (might not be zero, eg, each QOpenGLWindow has its own framebuffer object)...
				GLuint default_framebuffer_resource);

		//! Binds the texture object (at the specified image unit) to the active OpenGL context.
		void
		bind_image_texture(
				GLuint image_unit,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLboolean layered,
				GLint layer,
				GLenum access,
				GLenum format)
		{
			apply_state_set(
					d_state_set_store->bind_image_texture_state_sets,
					d_state_set_keys->get_bind_image_texture_key(image_unit),
					boost::in_place(boost::cref(d_capabilities), image_unit, texture, level, layered, layer, access, format));
		}

		//! Binds the renderbuffer object (at the specified target, must be GL_RENDERBUFFER) to the active OpenGL context.
		void
		bind_renderbuffer(
				GLenum target,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
		{
			apply_state_set(
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
			apply_state_set(
					d_state_set_store->bind_sampler_state_sets,
					d_state_set_keys->get_bind_sampler_key(unit),
					boost::in_place(boost::cref(d_capabilities), unit, sampler));
		}

		//! Binds the texture object (at the specified texture unit) to the active OpenGL context.
		void
		bind_texture_unit(
				GLuint texture_unit,
				boost::optional<GLTexture::shared_ptr_type> texture);

		//! Binds the vertex array object to the active OpenGL context.
		void
		bind_vertex_array(
				boost::optional<GLVertexArray::shared_ptr_type> array)
		{
			apply_state_set(
					d_state_set_store->bind_vertex_array_state_sets,
					GLStateSetKeys::KEY_BIND_VERTEX_ARRAY,
					boost::in_place(array));
		}

		void
		blend_color(
				GLclampf red,
				GLclampf green,
				GLclampf blue,
				GLclampf alpha)
		{
			apply_state_set(
					d_state_set_store->blend_color_state_sets,
					GLStateSetKeys::KEY_BLEND_COLOR,
					boost::in_place(red, green, blue, alpha));
		}

		void
		blend_equation(
				GLenum mode)
		{
			apply_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(mode));
		}

		void
		blend_equation_separate(
				GLenum mode_RGB,
				GLenum mode_alpha)
		{
			apply_state_set(
					d_state_set_store->blend_equation_state_sets,
					GLStateSetKeys::KEY_BLEND_EQUATION,
					boost::in_place(mode_RGB, mode_alpha));
		}

		void
		blend_func(
				GLenum src,
				GLenum dst)
		{
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
					d_state_set_store->clear_color_state_sets,
					GLStateSetKeys::KEY_CLEAR_COLOR,
					boost::in_place(red, green, blue, alpha));
		}

		void
		clear_depth(
				GLclampd depth)
		{
			apply_state_set(
					d_state_set_store->clear_depth_state_sets,
					GLStateSetKeys::KEY_CLEAR_DEPTH,
					boost::in_place(depth));
		}

		void
		clear_stencil(
				GLint stencil)
		{
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
					d_state_set_store->depth_func_state_sets,
					GLStateSetKeys::KEY_DEPTH_FUNC,
					boost::in_place(func));
		}

		void
		cull_face(
				GLenum mode)
		{
			apply_state_set(
					d_state_set_store->cull_face_state_sets,
					GLStateSetKeys::KEY_CULL_FACE,
					boost::in_place(mode));
		}

		void
		depth_mask(
				GLboolean flag)
		{
			apply_state_set(
					d_state_set_store->depth_mask_state_sets,
					GLStateSetKeys::KEY_DEPTH_MASK,
					boost::in_place(flag));
		}

		void
		depth_range(
				GLclampd n,
				GLclampd f)
		{
			apply_state_set(
					d_state_set_store->depth_range_state_sets,
					GLStateSetKeys::KEY_DEPTH_RANGE,
					boost::in_place(boost::cref(d_capabilities), n, f));
		}

		void
		draw_buffer(
				GLenum buf,
				GLenum default_draw_buffer)
		{
			apply_state_set(
					d_state_set_store->draw_buffers_state_sets,
					GLStateSetKeys::KEY_DRAW_BUFFERS,
					boost::in_place(buf, default_draw_buffer));
		}

		void
		draw_buffers(
				const std::vector<GLenum> &bufs,
				GLenum default_draw_buffer)
		{
			apply_state_set(
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
			apply_state_set(
					d_state_set_store->front_face_state_sets,
					GLStateSetKeys::KEY_FRONT_FACE,
					boost::in_place(dir));
		}

		void
		hint(
				GLenum target,
				GLenum hint_)
		{
			apply_state_set(
					d_state_set_store->hint_state_sets,
					d_state_set_keys->get_hint_key(target),
					boost::in_place(target, hint_));
		}

		void
		line_width(
				GLfloat width)
		{
			apply_state_set(
					d_state_set_store->line_width_state_sets,
					GLStateSetKeys::KEY_LINE_WIDTH,
					boost::in_place(width));
		}

		void
		pixel_storef(
				GLenum pname,
				GLfloat param)
		{
			apply_state_set(
					d_state_set_store->pixel_store_state_sets,
					d_state_set_keys->get_pixel_store_key(pname),
					boost::in_place(pname, param));
		}

		void
		pixel_storei(
				GLenum pname,
				GLint param)
		{
			apply_state_set(
					d_state_set_store->pixel_store_state_sets,
					d_state_set_keys->get_pixel_store_key(pname),
					boost::in_place(pname, param));
		}

		void
		point_size(
				GLfloat size)
		{
			apply_state_set(
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
			apply_state_set(
					d_state_set_store->polygon_mode_state_sets,
					GLStateSetKeys::KEY_POLYGON_MODE_FRONT_AND_BACK,
					boost::in_place(mode));
		}

		void
		polygon_offset(
				GLfloat factor,
				GLfloat units)
		{
			apply_state_set(
					d_state_set_store->polygon_offset_state_sets,
					GLStateSetKeys::KEY_POLYGON_OFFSET,
					boost::in_place(factor, units));
		}

		void
		primitive_restart_index(
				GLuint index)
		{
			apply_state_set(
					d_state_set_store->primitive_restart_index_state_sets,
					GLStateSetKeys::KEY_PRIMITIVE_RESTART_INDEX,
					boost::in_place(index));
		}

		void
		read_buffer(
				GLenum src,
				GLenum default_draw_buffer)
		{
			apply_state_set(
					d_state_set_store->read_buffer_state_sets,
					GLStateSetKeys::KEY_READ_BUFFER,
					boost::in_place(src, default_draw_buffer));
		}

		void
		sample_coverage(
				GLclampf value,
				GLboolean invert)
		{
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
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
			apply_state_set(
					d_state_set_store->viewport_state_sets,
					GLStateSetKeys::KEY_VIEWPORT,
					boost::in_place(viewport, default_viewport));
		}


		//
		// Get state methods (queries individual @a GLStateSet objects).
		//
		// Note: Unlike the set state methods, these are only provided where needed by the OpenGL framework.
		//


		//! Returns the bound buffer object, or boost::none if no object bound.
		boost::optional<GLBuffer::shared_ptr_type>
		get_bind_buffer(
				GLenum target) const;

		//! Returns the currently bound framebuffer object - boost::none implies the default framebuffer.
		boost::optional<GLFramebuffer::shared_ptr_type>
		get_bind_framebuffer(
				GLenum target) const;

		//! Returns the texture object bound on the specified image unit - boost::none implies the default no binding.
		boost::optional<GLTexture::shared_ptr_type>
		get_bind_image_texture(
				GLuint image_unit) const
		{
			return query_state_set<GLTexture::shared_ptr_type>(
					d_state_set_keys->get_bind_image_texture_key(image_unit),
					&GLBindImageTextureStateSet::d_texture);
		}

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
				GLuint texture_unit) const
		{
			boost::optional<const GLBindTextureStateSet &> state_set =
					query_state_set<GLBindTextureStateSet>(d_state_set_keys->get_bind_texture_key(texture_unit));

			boost::optional<GLTexture::shared_ptr_type> texture;
			if (!state_set)
			{
				return boost::none;
			}

			return state_set->d_target_textures[GLBindTextureStateSet::get_texture_target_index(texture_target)];
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

		//! Typedef for a state set key.
		typedef GLStateSetKeys::key_type state_set_key_type;

		/**
		 * Typedef for a shared pointer to an immutable @a GLStateSet.
		 *
		 * Note: There is only a pointer-to-const since we're treating @a GLStateSet objects
		 *       as immutable once created.
		 */
		typedef boost::shared_ptr<const GLStateSet> state_set_ptr_type;

		/**
		 * A snapshot of the OpenGL state at the start of a scope and the changes made within the scope.
		 *
		 * The implicit root scope starts at the default OpenGL state whereas an explicit scope (GL::StateScope)
		 * starts when @a save is called (at the start of a GL::StateScope).
		 */
		struct StateScope :
				private boost::noncopyable
		{
		public:
			//
			// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
			// is so these objects can be used with GPlatesUtils::ObjectCache.
			//

			//! A convenience typedef for a shared pointer to a @a StateScope.
			typedef boost::shared_ptr<StateScope> shared_ptr_type;
			typedef boost::shared_ptr<const StateScope> shared_ptr_to_const_type;


			/**
			 * Create the root state scope (that starts with the default OpenGL state, ie, no state sets).
			 */
			StateScope()
			{  }

			/**
			 * Create a child state scope within a parent state scope.
			 */
			explicit
			StateScope(
					shared_ptr_to_const_type parent_state_scope);

			/**
			 * Returns the current state set for the specified state set key.
			 *
			 * Returns none if in the default state.
			 */
			boost::optional<state_set_ptr_type>
			get_current_state_set(
					state_set_key_type state_set_key) const;

			/**
			 * Applies the specified state set (associated with the specified state set key).
			 *
			 * Applies state change to OpenGL if a state change is detected.
			 *
			 * Specify none to apply the default state.
			 */
			void
			apply_state_set(
					const GLState &current_state,
					const GLCapabilities &capabilities,
					state_set_key_type state_set_key,
					boost::optional<state_set_ptr_type> state_set_ptr = boost::none);

			/**
			 * Applies the default state (for all non-default state sets).
			 *
			 * Applies state changes to OpenGL where state changes are detected.
			 */
			void
			apply_default_state(
					const GLState &current_state,
					const GLCapabilities &capabilities);

			/**
			 * Applies the state *at* the start of the scope.
			 *
			 * This is achieved by reverting all state changes since the start of the scope.
			 *
			 * Applies reverted state changes to OpenGL.
			 */
			void
			apply_state_at_scope_start(
					const GLState &current_state,
					const GLCapabilities &capabilities);

		private:
			/**
			 * The state *at* the start of this scope.
			 *
			 * NOTE: The absence of a state set here means the default OpenGL state (for that state set).
			 *       This is unlike 'd_state_changed_since_scope_start' where absence means no state *changed*
			 *       (since the start of the scope).
			 *
			 * Note: For the root state scope this is empty and hence represents the default OpenGL state.
			 */
			std::unordered_map<state_set_key_type, state_set_ptr_type> d_state_at_scope_start;

			/**
			 * The state that's *changed* since the start of this scope.
			 *
			 * NOTE: The absence of a state set here means there was no state *change* since the start of scope (for that state set).
			 *       This is unlike 'd_state_at_scope_start' where absence means the default OpenGL state.
			 *
			 * Note: The state sets are boost::optional here (unlike the state sets at scope start) so that
			 *       a state change can register a change *to* the default state (as boost::none).
			 *       This way we know to revert from the default state when restoring state, otherwise
			 *       we wouldn't know the state had been explicitly changed to the default state
			 *       (ie, we would have just assumed that the state did not change at all).
			 */
			std::unordered_map<state_set_key_type, boost::optional<state_set_ptr_type>> d_state_changed_since_scope_start;
		};

		//! Typedef for a stack of state scopes to save (push) and restore (pop).
		typedef std::stack<StateScope::shared_ptr_type> save_restore_stack_type;


		OpenGLFunctions &d_opengl_functions;

		const GLCapabilities &d_capabilities;

		/**
		 * Used to assign a separate slot for each @a GLStateSet derived state.
		 */
		GLStateSetKeys::non_null_ptr_to_const_type d_state_set_keys;

		/**
		 * Manages allocation of derived @a GLStateSet instances using a separate object pool for each derived type.
		 *
		 * Note: This should be declared *before* anything that stores @a GLStateSet shared pointers
		 *       because they are allocated from object pools (in this state set store) and should
		 *       therefore be destroyed before the object pools (in this state set store) are destroyed.
		 *       Note that this is possible because @a GLStateSet shared pointers should not be able to escape @a GLstate.
		 */
		GLStateSetStore::non_null_ptr_type d_state_set_store;

		/**
		 * Used to allocate @a StateScope objects.
		 */
		GPlatesUtils::ObjectPool<StateScope> d_state_scope_pool;

		/**
		 * Stack of state scopes to save (push) and restore (pop).
		 */
		save_restore_stack_type d_save_restore_state;

		/**
		 * The (implicit) root state scope.
		 *
		 * This is an implicit state scope that is the current state scope when we're not inside any GL::StateScope scopes.
		 * The state at the start of its scope is the default OpenGL state.
		 */
		StateScope::shared_ptr_type d_root_state_scope;

		/**
		 * The current state scope.
		 *
		 * This is the implicit root state scope if we're *not* inside any GL::StateScope scopes, otherwise
		 * it's the top of the save/restore stack (ie, the most recently pushed state scope associated with
		 * the GL::StateScope that we're currently inside).
		 */
		StateScope::shared_ptr_type d_current_state_scope;


		GLState(
				OpenGLFunctions &opengl_functions,
				const GLCapabilities &capabilities,
				const GLStateStore::non_null_ptr_type &state_store);

		/**
		 * Create a new GLStateSet of appropriate derived type and store as an immutable state set.
		 *
		 * Note: The use of boost::in_place_factory means the derived state set object is created
		 *       directly in the object pool.
		 */
		template <class GLStateSetType, class InPlaceFactoryType>
		boost::shared_ptr<GLStateSetType>
		create_state_set(
				GPlatesUtils::ObjectPool<GLStateSetType> &state_set_pool,
				const InPlaceFactoryType &state_set_constructor_args)
		{
			return state_set_pool.add_with_auto_release(state_set_constructor_args);
		}

		/**
		 * Applies the specified state set to OpenGL if a state change is detected.
		 */
		void
		apply_state_set(
				state_set_key_type state_set_key,
				const state_set_ptr_type &state_set)
		{
			// Apply the new current state set to OpenGL (if a state change is detected).
			d_current_state_scope->apply_state_set(*this/*current_state*/, d_capabilities, state_set_key, state_set);
		}

		/**
		 * Creates a new state set and applies it to OpenGL if a state change is detected.
		 *
		 * The constructor arguments of the derived @a GLStateSet type are passed in
		 * @a state_set_constructor_args and it is created in the @a state_set_pool object pool.
		 */
		template <class GLStateSetType, class InPlaceFactoryType>
		void
		apply_state_set(
				GPlatesUtils::ObjectPool<GLStateSetType> &state_set_pool,
				state_set_key_type state_set_key,
				const InPlaceFactoryType &state_set_constructor_args)
		{
			// Create a new state set and apply to OpenGL (if a state change is detected).
			apply_state_set(state_set_key, create_state_set(state_set_pool, state_set_constructor_args));
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
			// Get the current state.
			boost::optional<state_set_ptr_type> state_set = d_current_state_scope->get_current_state_set(state_set_key);
			if (!state_set)
			{
				// In the default OpenGL state (for the requested state set key).
				return boost::none;
			}

			const GLStateSetType *derived_state_set = dynamic_cast<const GLStateSetType *>(state_set->get());

			return *derived_state_set;
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
			// Get the current state.
			boost::optional<state_set_ptr_type> state_set = d_current_state_scope->get_current_state_set(state_set_key);
			if (!state_set)
			{
				// In the default OpenGL state (for the requested state set key).
				return boost::none;
			}

			const GLStateSetType* derived_state_set = dynamic_cast<const GLStateSetType*>(state_set->get());

			return derived_state_set->*query_member;
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
			// Get the current state.
			boost::optional<state_set_ptr_type> state_set = d_current_state_scope->get_current_state_set(state_set_key);
			if (!state_set)
			{
				// In the default OpenGL state (for the requested state set key).
				return boost::none;
			}

			const GLStateSetType* derived_state_set = dynamic_cast<const GLStateSetType*>(state_set->get());

			return query_function(*derived_state_set);
		}
	};
}

#endif // GPLATES_OPENGL_GLSTATE_H
