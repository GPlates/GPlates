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

#include "GLState.h"

#include "GLCapabilities.h"
#include "GLContext.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLState::GLState(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		const GLStateStore::non_null_ptr_type &state_store) :
	d_opengl_functions(opengl_functions),
	d_capabilities(capabilities),
	d_state_set_keys(state_store->get_state_set_keys()),
	d_state_set_store(state_store->get_state_set_store()),
	// The root state scope is initially empty (which represents the default OpenGL state)...
	d_root_state_scope(d_state_scope_pool.add_with_auto_release(boost::in_place())),
	// The current state scope is initially the root state scope (until a GL::StateScope is entered)...
	d_current_state_scope(d_root_state_scope)
{
}


void
GPlatesOpenGL::GLState::reset_to_default()
{
	PROFILE_FUNC();

	// Reset the current state to the default OpenGL state.
	d_current_state_scope->apply_default_state(*this/*current_state*/, d_capabilities);
}


void
GPlatesOpenGL::GLState::save()
{
	PROFILE_FUNC();

	// Allocate an empty state scope as the new current state scope.
	if (d_save_restore_state.empty())
	{
		// Parent is the root state scope (since there are no saved state scopes).
		d_current_state_scope = d_state_scope_pool.add_with_auto_release(
				boost::in_place(d_root_state_scope));
	}
	else
	{
		// Parent is the most recently saved/pushed state scope.
		d_current_state_scope = d_state_scope_pool.add_with_auto_release(
				boost::in_place(d_save_restore_state.top()));
	}

	// Push the new current state scope onto the save/restore stack.
	d_save_restore_state.push(d_current_state_scope);
}


void
GPlatesOpenGL::GLState::restore()
{
	PROFILE_FUNC();

	// There should be at least one saved state on the stack - if not then save/restore
	// has been used incorrectly by the caller.
	// Also the top of the save/restore stack should be the current state scope.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_save_restore_state.empty() &&
				d_current_state_scope == d_save_restore_state.top(),
			GPLATES_ASSERTION_SOURCE);

	// Revert the state changed since the start of the current scope.
	d_current_state_scope->apply_state_at_scope_start(
			*this/*current_state*/,
			d_capabilities);

	// Pop the current state scope off the save/restore stack.
	d_save_restore_state.pop();

	// The new current state scope is the parent state scope.
	if (d_save_restore_state.empty())
	{
		// Parent is the root state scope (since there are no more saved state scopes).
		d_current_state_scope = d_root_state_scope;
	}
	else
	{
		// Parent is the next most recently saved/pushed state scope.
		d_current_state_scope = d_save_restore_state.top();
	}
}


void
GPlatesOpenGL::GLState::bind_buffer(
		GLenum target,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	if (target == GL_UNIFORM_BUFFER ||
		target == GL_TRANSFORM_FEEDBACK_BUFFER ||
		target == GL_SHADER_STORAGE_BUFFER ||
		target == GL_ATOMIC_COUNTER_BUFFER)
	{
		//
		// Indexed buffer target.
		//

		const state_set_key_type state_set_key = d_state_set_keys->get_bind_buffer_key(target);

		// Get the current state.
		boost::optional<const GLBindBufferIndexedStateSet &> current_state_set =
				query_state_set<GLBindBufferIndexedStateSet>(state_set_key);

		// Copy the current state.
		boost::shared_ptr<GLBindBufferIndexedStateSet> new_state_set;
		if (current_state_set)
		{
			new_state_set = create_state_set(
					d_state_set_store->bind_buffer_indexed_state_sets,
					// Copy construction...
					boost::in_place(boost::cref(current_state_set.get())));
		}
		else
		{
			new_state_set = create_state_set(
					d_state_set_store->bind_buffer_indexed_state_sets,
					// Default state...
					boost::in_place(target));
		}

		// Set the requested state (in copy of current state).
		//
		// Bind/unbind the general binding point - but leave the indexed binding points as they are.
		if (buffer)
		{
			new_state_set->d_general_buffer = buffer.get();
			new_state_set->d_general_buffer_resource = buffer.get()->get_resource_handle();
		}
		else // unbind
		{
			new_state_set->d_general_buffer = boost::none;
			new_state_set->d_general_buffer_resource = 0;
		}

		// Apply modified copy of current state.
		apply_state_set(state_set_key, new_state_set);
	}
	else
	{
		//
		// Non-indexed buffer target.
		//
		apply_state_set(
				d_state_set_store->bind_buffer_state_sets,
				d_state_set_keys->get_bind_buffer_key(target),
				boost::in_place(target, buffer));
	}
}


void
GPlatesOpenGL::GLState::bind_buffer_base(
		GLenum target,
		GLuint index,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	// Should only be called for an *indexed* target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			target == GL_UNIFORM_BUFFER ||
				target == GL_TRANSFORM_FEEDBACK_BUFFER ||
				target == GL_SHADER_STORAGE_BUFFER ||
				target == GL_ATOMIC_COUNTER_BUFFER,
			GPLATES_ASSERTION_SOURCE);

	const state_set_key_type state_set_key = d_state_set_keys->get_bind_buffer_key(target);

	// Get the current state.
	boost::optional<const GLBindBufferIndexedStateSet &> current_state_set =
			query_state_set<GLBindBufferIndexedStateSet>(state_set_key);

	// Copy the current state.
	boost::shared_ptr<GLBindBufferIndexedStateSet> new_state_set;
	if (current_state_set)
	{
		new_state_set = create_state_set(
				d_state_set_store->bind_buffer_indexed_state_sets,
				// Copy construction...
				boost::in_place(boost::cref(current_state_set.get())));
	}
	else
	{
		new_state_set = create_state_set(
				d_state_set_store->bind_buffer_indexed_state_sets,
				// Default state...
				boost::in_place(target));
	}

	// Set the requested state (in copy of current state).
	//
	// Bind/unbind the general binding point as well as the indexed binding point at 'index'.
	if (buffer)
	{
		new_state_set->d_general_buffer = buffer.get();
		new_state_set->d_general_buffer_resource = buffer.get()->get_resource_handle();

		new_state_set->d_indexed_buffers[index] = { buffer.get(), buffer.get()->get_resource_handle(), boost::none };
	}
	else // unbind
	{
		new_state_set->d_general_buffer = boost::none;
		new_state_set->d_general_buffer_resource = 0;

		new_state_set->d_indexed_buffers.erase(index);
	}

	// Apply modified copy of current state.
	apply_state_set(state_set_key, new_state_set);
}


void
GPlatesOpenGL::GLState::bind_buffer_range(
		GLenum target,
		GLuint index,
		boost::optional<GLBuffer::shared_ptr_type> buffer,
		GLintptr offset,
		GLsizeiptr size)
{
	// Should only be called for an *indexed* target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			target == GL_UNIFORM_BUFFER ||
				target == GL_TRANSFORM_FEEDBACK_BUFFER ||
				target == GL_SHADER_STORAGE_BUFFER ||
				target == GL_ATOMIC_COUNTER_BUFFER,
			GPLATES_ASSERTION_SOURCE);

	const state_set_key_type state_set_key = d_state_set_keys->get_bind_buffer_key(target);

	// Get the current state.
	boost::optional<const GLBindBufferIndexedStateSet &> current_state_set =
			query_state_set<GLBindBufferIndexedStateSet>(state_set_key);

	// Copy the current state.
	boost::shared_ptr<GLBindBufferIndexedStateSet> new_state_set;
	if (current_state_set)
	{
		new_state_set = create_state_set(
				d_state_set_store->bind_buffer_indexed_state_sets,
				// Copy construction...
				boost::in_place(boost::cref(current_state_set.get())));
	}
	else
	{
		new_state_set = create_state_set(
				d_state_set_store->bind_buffer_indexed_state_sets,
				// Default state...
				boost::in_place(target));
	}

	// Set the requested state (in copy of current state).
	//
	// Bind/unbind the general binding point as well as the indexed binding point at 'index'.
	if (buffer)
	{
		new_state_set->d_general_buffer = buffer.get();
		new_state_set->d_general_buffer_resource = buffer.get()->get_resource_handle();

		new_state_set->d_indexed_buffers[index] = {
				buffer.get(),
				buffer.get()->get_resource_handle(),
				GLBindBufferIndexedStateSet::Range{ offset, size } };
	}
	else // unbind
	{
		new_state_set->d_general_buffer = boost::none;
		new_state_set->d_general_buffer_resource = 0;

		new_state_set->d_indexed_buffers.erase(index);
	}

	// Apply modified copy of current state.
	apply_state_set(state_set_key, new_state_set);
}


void
GPlatesOpenGL::GLState::bind_framebuffer(
		GLenum target,
		boost::optional<GLFramebuffer::shared_ptr_type> framebuffer,
		// Default framebuffer resource (might not be zero, eg, each QOpenGLWindow has its own framebuffer object)...
		GLuint default_framebuffer_resource)
{
	// Default framebuffer (bound to zero).
	boost::optional<GLFramebuffer::shared_ptr_type> draw_framebuffer;
	boost::optional<GLFramebuffer::shared_ptr_type> read_framebuffer;

	if (target == GL_FRAMEBUFFER)
	{
		// Framebuffer is used for both draw and read targets.
		draw_framebuffer = framebuffer;
		read_framebuffer = framebuffer;
	}
	else if (target == GL_DRAW_FRAMEBUFFER)
	{
		// Framebuffer is used for draw target only.
		draw_framebuffer = framebuffer;

		// For read target use framebuffer currently bound to it.
		boost::optional<const GLBindFramebufferStateSet &> bind_framebuffer_state_set =
				query_state_set<GLBindFramebufferStateSet>(GLStateSetKeys::KEY_BIND_FRAMEBUFFER);
		if (bind_framebuffer_state_set)
		{
			read_framebuffer = bind_framebuffer_state_set->d_read_framebuffer;
		}
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				target == GL_READ_FRAMEBUFFER,
				GPLATES_ASSERTION_SOURCE);

		// Framebuffer is used for read target only.
		read_framebuffer = framebuffer;

		// For draw target use framebuffer currently bound to it.
		boost::optional<const GLBindFramebufferStateSet &> bind_framebuffer_state_set =
				query_state_set<GLBindFramebufferStateSet>(GLStateSetKeys::KEY_BIND_FRAMEBUFFER);
		if (bind_framebuffer_state_set)
		{
			draw_framebuffer = bind_framebuffer_state_set->d_draw_framebuffer;
		}
	}

	apply_state_set(
			d_state_set_store->bind_framebuffer_state_sets,
			GLStateSetKeys::KEY_BIND_FRAMEBUFFER,
			boost::in_place(
					draw_framebuffer, read_framebuffer,
					default_framebuffer_resource));
}


void
GPlatesOpenGL::GLState::color_maski(
		GLuint buf,
		GLboolean red,
		GLboolean green,
		GLboolean blue,
		GLboolean alpha)
{
	// Get the current state.
	boost::optional<const GLColorMaskStateSet &> current_state_set =
			query_state_set<GLColorMaskStateSet>(GLStateSetKeys::KEY_COLOR_MASK);

	// Copy the current state.
	boost::shared_ptr<GLColorMaskStateSet> new_state_set;
	if (current_state_set)
	{
		new_state_set = create_state_set(
				d_state_set_store->color_mask_state_sets,
				// Copy construction...
				boost::in_place(boost::cref(current_state_set.get())));
	}
	else
	{
		new_state_set = create_state_set(
				d_state_set_store->color_mask_state_sets,
				// Default state...
				boost::in_place(boost::cref(d_capabilities), boost::cref(GLColorMaskStateSet::DEFAULT_MASK)));
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			buf < d_capabilities.gl_max_draw_buffers,
			GPLATES_ASSERTION_SOURCE);

	// Set the requested state (in copy of current state).
	new_state_set->d_masks[buf] = {red, green, blue, alpha};
	new_state_set->d_all_masks_equal = false;

	// Apply modified copy of current state.
	apply_state_set(GLStateSetKeys::KEY_COLOR_MASK, new_state_set);
}


void
GPlatesOpenGL::GLState::enable(
		GLenum cap,
		bool enable_)
{
	const state_set_key_type state_set_key = d_state_set_keys->get_capability_key(cap);

	if (d_state_set_keys->is_capability_indexed(cap))
	{
		// According to the 3.3 core spec:
		//
		//   In general, passing an indexed capability to glEnable or glDisable will enable or disable
		//   that capability for all indices, respectively.

		// Apply to indexed state.
		apply_state_set(
				d_state_set_store->enable_indexed_state_sets,
				state_set_key,
				boost::in_place(cap, enable_, d_state_set_keys->get_num_capability_indices(cap)));
	}
	else // not an indexed capability ...
	{
		// Apply to non-indexed state.
		apply_state_set(
				d_state_set_store->enable_state_sets,
				state_set_key,
				boost::in_place(cap, enable_));
	}
}


void
GPlatesOpenGL::GLState::enablei(
		GLenum cap,
		GLuint index,
		bool enable_)
{
	const state_set_key_type state_set_key = d_state_set_keys->get_capability_key(cap);

	if (d_state_set_keys->is_capability_indexed(cap))
	{
		const GLuint num_capability_indices = d_state_set_keys->get_num_capability_indices(cap);

		// Get the current state.
		boost::optional<const GLEnableIndexedStateSet &> current_state_set =
				query_state_set<GLEnableIndexedStateSet>(state_set_key);

		// Copy the current state.
		boost::shared_ptr<GLEnableIndexedStateSet> new_state_set;
		if (current_state_set)
		{
			new_state_set = create_state_set(
					d_state_set_store->enable_indexed_state_sets,
					// Copy construction...
					boost::in_place(boost::cref(current_state_set.get())));
		}
		else
		{
			new_state_set = create_state_set(
					d_state_set_store->enable_indexed_state_sets,
					// Default state...
					boost::in_place(
							cap,
							GLEnableStateSet::get_default(cap),
							num_capability_indices));
		}

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				index < num_capability_indices,
				GPLATES_ASSERTION_SOURCE);

		// Set the requested state (in copy of current state).
		new_state_set->d_indices[index] = enable_;
		new_state_set->d_all_indices_equal = false;

		// Apply modified copy of current state.
		apply_state_set(state_set_key, new_state_set);
	}
	else // not an indexed capability ...
	{
		// According to the 3.3 core spec:
		//
		//   Any token accepted by glEnable or glDisable is also accepted by glEnablei and glDisablei,
		//   but if the capability is not indexed, the maximum value that index may take is zero.

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				index == 0,
				GPLATES_ASSERTION_SOURCE);

		// Apply to non-indexed state.
		apply_state_set(
				d_state_set_store->enable_state_sets,
				state_set_key,
				boost::in_place(cap, enable_));
	}
}


void
GPlatesOpenGL::GLState::sample_maski(
		GLuint mask_number,
		GLbitfield mask)
{
	// Get the current state.
	boost::optional<const GLSampleMaskStateSet &> current_state_set =
			query_state_set<GLSampleMaskStateSet>(GLStateSetKeys::KEY_SAMPLE_MASK);

	// Copy the current state.
	boost::shared_ptr<GLSampleMaskStateSet> new_state_set;
	if (current_state_set)
	{
		new_state_set = create_state_set(
				d_state_set_store->sample_mask_state_sets,
				// Copy construction...
				boost::in_place(boost::cref(current_state_set.get())));
	}
	else
	{
		new_state_set = create_state_set(
				d_state_set_store->sample_mask_state_sets,
				// Default state...
				boost::in_place(boost::cref(d_capabilities), boost::cref(GLSampleMaskStateSet::DEFAULT_MASK)));
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_number < d_capabilities.gl_max_sample_mask_words,
			GPLATES_ASSERTION_SOURCE);

	// Set the requested state (in copy of current state).
	new_state_set->d_masks[mask_number] = mask;

	// Apply modified copy of current state.
	apply_state_set(GLStateSetKeys::KEY_SAMPLE_MASK, new_state_set);
}


void
GPlatesOpenGL::GLState::stencil_func_separate(
		GLenum face,
		GLenum func,
		GLint ref,
		GLuint mask)
{
	// Stencil func to set.
	const GLStencilFuncStateSet::Func stencil_func{ func, ref, mask };

	// Front and back state (starts out uninitialized).
	GLStencilFuncStateSet::Func front_stencil_func;
	GLStencilFuncStateSet::Func back_stencil_func;

	// Set the requested state (in copy of current state).
	if (face == GL_FRONT)
	{
		front_stencil_func = stencil_func;

		// Get the current state.
		boost::optional<const GLStencilFuncStateSet &> stencil_func_state_set =
				query_state_set<GLStencilFuncStateSet>(GLStateSetKeys::KEY_STENCIL_FUNC);
		if (stencil_func_state_set)
		{
			back_stencil_func = stencil_func_state_set->d_back_func;
		}
		else // default state
		{
			back_stencil_func = GLStencilFuncStateSet::DEFAULT_FUNC;
		}
	}
	else if (face == GL_BACK)
	{
		back_stencil_func = stencil_func;

		// Get the current state.
		boost::optional<const GLStencilFuncStateSet &> stencil_func_state_set =
				query_state_set<GLStencilFuncStateSet>(GLStateSetKeys::KEY_STENCIL_FUNC);
		if (stencil_func_state_set)
		{
			front_stencil_func = stencil_func_state_set->d_front_func;
		}
		else // default state
		{
			front_stencil_func = GLStencilFuncStateSet::DEFAULT_FUNC;
		}
	}
	else // GL_FRONT_AND_BACK
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				face == GL_FRONT_AND_BACK,
				GPLATES_ASSERTION_SOURCE);

		front_stencil_func = stencil_func;
		back_stencil_func = stencil_func;
	}

	// Apply modified copy of current state.
	apply_state_set(
			d_state_set_store->stencil_func_state_sets,
			GLStateSetKeys::KEY_STENCIL_FUNC,
			boost::in_place(front_stencil_func, back_stencil_func));
}


void
GPlatesOpenGL::GLState::stencil_mask_separate(
		GLenum face,
		GLuint mask)
{
	// Front and back state (starts out uninitialized).
	GLuint front_mask;
	GLuint back_mask;

	// Set the requested state (in copy of current state).
	if (face == GL_FRONT)
	{
		front_mask = mask;

		// Get the current state.
		boost::optional<const GLStencilMaskStateSet &> stencil_mask_state_set =
				query_state_set<GLStencilMaskStateSet>(GLStateSetKeys::KEY_STENCIL_MASK);
		if (stencil_mask_state_set)
		{
			back_mask = stencil_mask_state_set->d_back_mask;
		}
		else // default state
		{
			back_mask = GLStencilMaskStateSet::DEFAULT_MASK;
		}
	}
	else if (face == GL_BACK)
	{
		back_mask = mask;

		// Get the current state.
		boost::optional<const GLStencilMaskStateSet &> stencil_mask_state_set =
				query_state_set<GLStencilMaskStateSet>(GLStateSetKeys::KEY_STENCIL_MASK);
		if (stencil_mask_state_set)
		{
			front_mask = stencil_mask_state_set->d_front_mask;
		}
		else // default state
		{
			front_mask = GLStencilMaskStateSet::DEFAULT_MASK;
		}
	}
	else // GL_FRONT_AND_BACK
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				face == GL_FRONT_AND_BACK,
				GPLATES_ASSERTION_SOURCE);

		front_mask = mask;
		back_mask = mask;
	}

	// Apply modified copy of current state.
	apply_state_set(
			d_state_set_store->stencil_mask_state_sets,
			GLStateSetKeys::KEY_STENCIL_MASK,
			boost::in_place(front_mask, back_mask));
}


void
GPlatesOpenGL::GLState::stencil_op_separate(
		GLenum face,
		GLenum sfail,
		GLenum dpfail,
		GLenum dppass)
{
	// Stencil op to set.
	const GLStencilOpStateSet::Op stencil_op{ sfail, dpfail, dppass };

	// Front and back state (starts out uninitialized).
	GLStencilOpStateSet::Op front_stencil_op;
	GLStencilOpStateSet::Op back_stencil_op;

	// Set the requested state (in copy of current state).
	if (face == GL_FRONT)
	{
		front_stencil_op = stencil_op;

		// Get the current state.
		boost::optional<const GLStencilOpStateSet &> stencil_op_state_set =
				query_state_set<GLStencilOpStateSet>(GLStateSetKeys::KEY_STENCIL_OP);
		if (stencil_op_state_set)
		{
			back_stencil_op = stencil_op_state_set->d_back_op;
		}
		else // default state
		{
			back_stencil_op = GLStencilOpStateSet::DEFAULT_OP;
		}
	}
	else if (face == GL_BACK)
	{
		back_stencil_op = stencil_op;

		// Get the current state.
		boost::optional<const GLStencilOpStateSet &> stencil_op_state_set =
				query_state_set<GLStencilOpStateSet>(GLStateSetKeys::KEY_STENCIL_OP);
		if (stencil_op_state_set)
		{
			front_stencil_op = stencil_op_state_set->d_front_op;
		}
		else // default state
		{
			front_stencil_op = GLStencilOpStateSet::DEFAULT_OP;
		}
	}
	else // GL_FRONT_AND_BACK
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				face == GL_FRONT_AND_BACK,
				GPLATES_ASSERTION_SOURCE);

		front_stencil_op = stencil_op;
		back_stencil_op = stencil_op;
	}

	// Apply modified copy of current state.
	apply_state_set(
			d_state_set_store->stencil_op_state_sets,
			GLStateSetKeys::KEY_STENCIL_OP,
			boost::in_place(front_stencil_op, back_stencil_op));
}


boost::optional<GPlatesOpenGL::GLBuffer::shared_ptr_type>
GPlatesOpenGL::GLState::get_bind_buffer(
		GLenum target) const
{
	if (target == GL_UNIFORM_BUFFER ||
		target == GL_TRANSFORM_FEEDBACK_BUFFER ||
		target == GL_SHADER_STORAGE_BUFFER ||
		target == GL_ATOMIC_COUNTER_BUFFER)
	{
		// Indexed buffer target.
		return query_state_set<GLBuffer::shared_ptr_type>(
				d_state_set_keys->get_bind_buffer_key(target),
				&GLBindBufferIndexedStateSet::d_general_buffer);
	}
	else
	{
		// Non-indexed buffer target.
		return query_state_set<GLBuffer::shared_ptr_type>(
				d_state_set_keys->get_bind_buffer_key(target),
				&GLBindBufferStateSet::d_buffer);
	}
}


boost::optional<GPlatesOpenGL::GLFramebuffer::shared_ptr_type>
GPlatesOpenGL::GLState::get_bind_framebuffer(
		GLenum target) const
{
	// Select the framebuffer bound to the draw or read target.
	boost::optional<GLFramebuffer::shared_ptr_type> GLBindFramebufferStateSet::*framebuffer = nullptr;

	if (target == GL_FRAMEBUFFER ||
		target == GL_DRAW_FRAMEBUFFER)
	{
		// Return the GL_DRAW_FRAMEBUFFER framebuffer.
		framebuffer = &GLBindFramebufferStateSet::d_draw_framebuffer;
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				target == GL_READ_FRAMEBUFFER,
				GPLATES_ASSERTION_SOURCE);

		// Return the GL_READ_FRAMEBUFFER framebuffer.
		framebuffer = &GLBindFramebufferStateSet::d_read_framebuffer;
	}

	return query_state_set<GLFramebuffer::shared_ptr_type>(
			GLStateSetKeys::KEY_BIND_FRAMEBUFFER,
			framebuffer);
}


bool
GPlatesOpenGL::GLState::is_capability_enabled(
		GLenum cap,
		GLuint index) const
{
	const state_set_key_type state_set_key = d_state_set_keys->get_capability_key(cap);

	if (d_state_set_keys->is_capability_indexed(cap))
	{
		boost::optional<const GLEnableIndexedStateSet &> enable_indexed_state_set =
				query_state_set<GLEnableIndexedStateSet>(state_set_key);
		if (enable_indexed_state_set)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					index < d_state_set_keys->get_num_capability_indices(cap),
					GPLATES_ASSERTION_SOURCE);

			return enable_indexed_state_set->d_indices[index];
		}
		else
		{
			// Return default.
			return GLEnableStateSet::get_default(cap);
		}
	}
	else // not an indexed capability ...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				index == 0,
				GPLATES_ASSERTION_SOURCE);

		boost::optional<const GLEnableStateSet &> enable_state_set =
				query_state_set<GLEnableStateSet>(state_set_key);
		if (enable_state_set)
		{
			return enable_state_set->d_enable;
		}
		else
		{
			// Return default.
			return GLEnableStateSet::get_default(cap);
		}
	}
}


GPlatesOpenGL::GLState::StateScope::StateScope(
		shared_ptr_to_const_type parent_state_scope) :
	d_state_at_scope_start(parent_state_scope->d_state_at_scope_start)
{
	// The state at the start of the new scope is the state at the start of the parent scope
	// plus the state that's changed *during* the parent scope.
	for (const auto &parent_state_set_key_value : parent_state_scope->d_state_changed_since_scope_start)
	{
		const state_set_key_type state_set_key = parent_state_set_key_value.first;
		const boost::optional<state_set_ptr_type> &parent_state_set = parent_state_set_key_value.second;

		if (parent_state_set)
		{
			d_state_at_scope_start[state_set_key] = parent_state_set.get();
		}
		else
		{
			// Parent state set is in the default state, which is the equivalent of no state set,
			// so make sure there's no state set recorded at the start of the new scope.
			d_state_at_scope_start.erase(state_set_key);
		}
	}
}


boost::optional<GPlatesOpenGL::GLState::state_set_ptr_type>
GPlatesOpenGL::GLState::StateScope::get_current_state_set(
		state_set_key_type state_set_key) const
{
	// First search the state that *changed* since the start of the scope.
	auto search_state_changed = d_state_changed_since_scope_start.find(state_set_key);
	if (search_state_changed != d_state_changed_since_scope_start.end())
	{
		const boost::optional<state_set_ptr_type> &state_set = search_state_changed->second;

		// Either the state set exists or is none (ie, has been explicitly set to the default state
		// since the start of scope). Either way it's the current state, so return it.
		return state_set;
	}

	// Next search the state *at* the start of the scope.
	auto search_state_at_start = d_state_at_scope_start.find(state_set_key);
	if (search_state_at_start != d_state_at_scope_start.end())
	{
		const state_set_ptr_type &state_set = search_state_at_start->second;

		return state_set;
	}

	// State set not found at all, so its absence means it's in the default OpenGL state.
	return boost::none;
}


void
GPlatesOpenGL::GLState::StateScope::apply_state_set(
		const GLState &current_state,
		const GLCapabilities &capabilities,
		state_set_key_type state_set_key,
		boost::optional<state_set_ptr_type> state_set_ptr)
{
	// Get the current state.
	boost::optional<state_set_ptr_type> current_state_set = get_current_state_set(state_set_key);

	if (state_set_ptr && current_state_set)
	{
		// Both state sets exist - this is a transition from the current state to the new state.
		if (state_set_ptr.get()->apply_state(current_state.d_opengl_functions, capabilities, *current_state_set.get(), current_state))
		{
			// The new state is different than the current state, so record as the new current state.
			d_state_changed_since_scope_start[state_set_key] = state_set_ptr;
		}
	}
	else if (state_set_ptr)
	{
		// Only the new state set exists - get it to apply its internal state.
		// This is a transition from the current *default* state to the new state.
		if (state_set_ptr.get()->apply_from_default_state(current_state.d_opengl_functions, capabilities, current_state))
		{
			// The new state is different than the *default* state, so record as the new current state.
			d_state_changed_since_scope_start[state_set_key] = state_set_ptr;
		}
	}
	else if (current_state_set)
	{
		// Only the current state set exists - get it to apply the default state.
		// This is a transition from the current state to the *default* state.
		if (current_state_set.get()->apply_to_default_state(current_state.d_opengl_functions, capabilities, current_state))
		{
			// The *default* state is different than the current state, so record as the new current state.
			d_state_changed_since_scope_start[state_set_key] = boost::none;
		}
	}
	// else neither state exists (so no state change since both are the default state).
}


void
GPlatesOpenGL::GLState::StateScope::apply_default_state(
		const GLState &current_state,
		const GLCapabilities &capabilities)
{
	// First iterate over the state that *changed* since the start of the scope.
	//
	// This represents the part of the current state that has changed since the start of the scope.
	for (auto &state_changed : d_state_changed_since_scope_start)
	{
		boost::optional<state_set_ptr_type> &state_set = state_changed.second;
		if (state_set)
		{
			// A state set exists (meaning it's *not* the default state).
			//
			// Change the state from the current state to the default state.
			state_set.get()->apply_to_default_state(current_state.d_opengl_functions, capabilities, current_state);

			// Record the state as the default state.
			state_set = boost::none;
		}
		// else already in default state.
	}

	// Next iterate over the state *at* the start of the scope
	// (but ignore state already set to default in the loop above).
	//
	// This represents the part of the current state *at* the start of the scope that has *not*
	// changed since the start of the scope.
	for (const auto &state_at_start : d_state_at_scope_start)
	{
		const state_set_key_type state_set_key = state_at_start.first;
		const state_set_ptr_type &state_set = state_at_start.second;

		// Search for the equivalent state set (with same key) in the state that *changed* since the start of the scope.
		auto insert_state_change = d_state_changed_since_scope_start.insert({state_set_key, state_set});
		// If the insertion was successful then it means the current state set has not changed since start of the scope.
		if (insert_state_change.second)
		{
			// Change the state from the current state to the default state.
			state_set->apply_to_default_state(current_state.d_opengl_functions, capabilities, current_state);

			// Record the state as the default state.
			insert_state_change.first->second = boost::none;
		}
	}
}


void
GPlatesOpenGL::GLState::StateScope::apply_state_at_scope_start(
		const GLState &current_state,
		const GLCapabilities &capabilities)
{
	// Iterate over the state sets representing state that changed since the start of the scope and
	// revert them (by comparing with the state at the start of the scope).
	for (const auto &changed_state : d_state_changed_since_scope_start)
	{
		const state_set_key_type state_set_key = changed_state.first;
		const boost::optional<state_set_ptr_type> &changed_state_set = changed_state.second;

		// Search for the equivalent state set (with same key) in the state *at* the start of the scope.
		auto search_state_at_start = d_state_at_scope_start.find(state_set_key);
		if (search_state_at_start != d_state_at_scope_start.end())
		{
			// A state set existed at the start of the scope (meaning it's *not* in the default state).
			const state_set_ptr_type &state_set_at_start = search_state_at_start->second;

			if (changed_state_set)
			{
				// Change the state from the current state to the state at the start of the scope.
				state_set_at_start->apply_state(current_state.d_opengl_functions, capabilities, *changed_state_set.get(), current_state);
			}
			else
			{
				// No state set exists for the current state (meaning it's the default state).
				//
				// Change the state from the default state to the state at the start of the scope.
				state_set_at_start->apply_from_default_state(current_state.d_opengl_functions, capabilities, current_state);
			}
		}
		else // in default state at start of scope...
		{
			if (changed_state_set)
			{
				// No state set existed at the start of the scope (meaning it's the default state).
				//
				// Change the state from the current state to the default state (at the start of the scope).
				changed_state_set.get()->apply_to_default_state(current_state.d_opengl_functions, capabilities, current_state);
			}
			// else neither state exists (so no state change since both are the default state). 
		}
	}

	// Since we're reverted to the state at the start of the scope, there's no state changes since
	// the start of the scope.
	d_state_changed_since_scope_start.clear();
}
