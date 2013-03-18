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

#include <exception>
#include <iostream>
#include <map>
#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <QDebug>
#include <QPaintEngine>

#include "GLRenderer.h"

#include "GLStateStore.h"
#include "GLUtils.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


using namespace GPlatesOpenGL::GLRendererImpl;


const GLenum GPlatesOpenGL::GLRenderer::DEFAULT_BLEND_EQUATION = GL_FUNC_ADD_EXT;


GPlatesOpenGL::GLRenderer::GLRenderer(
		const GLContext::non_null_ptr_type &context,
		const GLStateStore::shared_ptr_type &state_store) :
	d_context(context),
	d_state_store(state_store),
	d_default_state(d_state_store->allocate_state()),
	d_last_applied_state(d_state_store->allocate_state()),
	d_current_frame_buffer_draw_count(0)
{
}


void
GPlatesOpenGL::GLRenderer::begin_render(
		const GLViewport &default_viewport)
{
	// Start a rendering frame.
	d_context->begin_render();

	// We should have no render target blocks at this stage.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			d_render_target_block_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_RENDER_TARGET_BLOCKS);

	// The viewport of the window currently attached to the OpenGL context.
	d_default_viewport = default_viewport;
	d_default_state->set_viewport(get_context().get_capabilities(), default_viewport, default_viewport);
	d_default_state->set_scissor(get_context().get_capabilities(), default_viewport, default_viewport);

	// Start a new render target block with its first state block set to the default state.
	// This render target block represents the main framebuffer.
	begin_render_target_block_internal(true/*reset_to_default_state*/);

	// NOTE: We are explicitly setting OpenGL state here (which is unusual since it's all meant to be
	// wrapped by GLState and the GLStateSet derivations) because there isn't really any default
	// viewport since the 'default' is different depending on the window the OpenGL context is
	// attached to (which is why we asked the caller to pass it in to us).
	// So we 'initialise' the pseudo-default viewport state here (same applies to the scissor rectangle).
	// Later when GLState applies its state to OpenGL it filters redundant state changes and will likely
	// filter out a subsequent viewport setting if the viewport rectangle is the same.
	// If we didn't call 'glViewport' here then OpenGL would be left with the viewport of the last
	// window that the current OpenGL context was attached to (which is different than the current window).
	glViewport(default_viewport.x(), default_viewport.y(), default_viewport.width(), default_viewport.height());
	glScissor(default_viewport.x(), default_viewport.y(), default_viewport.width(), default_viewport.height());

	// Use the GL_EXT_framebuffer_object extension for render targets if it's available.
	if (get_context().get_capabilities().framebuffer.gl_EXT_framebuffer_object)
	{
		d_framebuffer_object = d_context->get_non_shared_state()->acquire_frame_buffer_object(*this);
	}

	// Apply the default vertex array state to the default vertex array object (resource handle zero).
	// Since we haven't bound any vertex array objects yet then the default object (zero) is currently bound.
	// This is not necessary but improves efficiency of filtering redundant vertex array state since
	// simple pointer (GLStateSet) comparisons only are needed to filter redundant vertex array state.
	//
	// NOTE: This is done at the very end of 'begin_render' to ensure everything is set up for
	// rendering before we start using 'this' renderer.
	const GLCompiledDrawState::non_null_ptr_to_const_type default_vertex_array_state =
			create_unbound_vertex_array_compiled_draw_state(*this);
	apply_compiled_draw_state(*default_vertex_array_state);
}


void
GPlatesOpenGL::GLRenderer::begin_render(
		QPainter &qpainter,
		bool paint_device_is_framebuffer)
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!d_qpainter_info,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_ACTIVE_QPAINTER);

	d_qpainter_info = boost::in_place(boost::ref(qpainter), paint_device_is_framebuffer);

	// The QPainter should currently be active.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			d_qpainter_info->qpainter.isActive(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_ACTIVE_QPAINTER);

	// The viewport and modelview/projection matrices set by QPainter.
	GLViewport qpainter_viewport;
	GLMatrix qpainter_model_view_matrix;
	GLMatrix qpainter_projection_matrix;

	// Suspend the QPainter so we can start making calls directly to OpenGL without interfering with
	// the QPainter's OpenGL state (if it uses an OpenGL paint engine).
	suspend_qpainter(qpainter_viewport, qpainter_model_view_matrix, qpainter_projection_matrix);

	begin_render(qpainter_viewport);

	// We're not really in the default OpenGL state so we need to track the current
	// modelview and projection matrices set by QPainter.
	// Easiest way to do that is simply to load them.
	gl_load_matrix(GL_MODELVIEW, qpainter_model_view_matrix);
	gl_load_matrix(GL_PROJECTION, qpainter_projection_matrix);

	// This is one of the rare cases where we need to apply the OpenGL state encapsulated in
	// GLRenderer directly to OpenGL - in this case we need to make sure our last applied state
	// actually represents the state of OpenGL - which it may not because QPainter may have
	// changed the model-view and projection matrices (if it uses an OpenGL paint engine).
	apply_current_state_to_opengl();
}


void
GPlatesOpenGL::GLRenderer::end_render()
{
	// Finish the current render target block that represented the main framebuffer.
	end_render_target_block_internal();

	// We should now have no render target blocks.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			d_render_target_block_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_RENDER_TARGET_BLOCKS);

	// We no longer have a default viewport.
	d_default_viewport = boost::none;

	// No longer need a framebuffer object for render targets.
	d_framebuffer_object = boost::none;

	// We should be at the default OpenGL state but it has not necessarily been applied
	// directly to OpenGL yet. So we do this now.
	// This is because we're finished rendering and should leave OpenGL in the default state.
	d_default_state->apply_state(get_capabilities(), *d_last_applied_state);

	// End a rendering frame.
	d_context->end_render();

	// If a QPainter (using OpenGL) was specified in 'begin_render' then resume it so the
	// client can continue using the QPainter for rendering.
	// NOTE: We are currently in the default OpenGL state which is required before we can resume the QPainter.
	if (d_qpainter_info)
	{
		// The QPainter should currently be active - it should not have become inactive between
		// 'begin_render' and 'end_render' or switched paint engines.
		GPlatesGlobal::Assert<GLRendererAPIError>(
				d_qpainter_info->qpainter.isActive(),
				GPLATES_ASSERTION_SOURCE,
				GLRendererAPIError::SHOULD_HAVE_ACTIVE_QPAINTER);

		// NOTE: We don't need to reset to the default state (and apply it) because that was just
		// done above (that's the state we leave OpenGL in when we're finished rendering).
		resume_qpainter();

		d_qpainter_info = boost::none;
	}
}


bool
GPlatesOpenGL::GLRenderer::rendering_to_context_framebuffer() const
{
	// Should be between 'begin_render()' and 'end_render()' - should have a render target block.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!d_render_target_block_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TARGET_BLOCK);

	return d_qpainter_info ? d_qpainter_info->paint_device_is_framebuffer : true;
}


boost::optional<QPainter &>
GPlatesOpenGL::GLRenderer::begin_qpainter_block()
{
	if (d_qpainter_info)
	{
		// Reset to the default OpenGL state as that's what QPainter expects when it resumes painting
		// (if it uses an OpenGL paint engine).
		begin_state_block(true/*reset_to_default_state*/);

		// This is one of the rare cases where we need to apply the OpenGL state encapsulated in
		// GLRenderer directly to OpenGL so that Qt can see it. When we're rendering exclusively using
		// GLRenderer we don't need this because the next draw call will flush the state to OpenGL for us.
		apply_current_state_to_opengl();

		resume_qpainter();

		return d_qpainter_info->qpainter;
	}

	return boost::none;
}


void
GPlatesOpenGL::GLRenderer::end_qpainter_block(
		bool restore_model_view_projection_transforms)
{
	if (d_qpainter_info)
	{
		// The viewport and modelview/projection matrices set by QPainter.
		GLViewport qpainter_viewport;
		GLMatrix qpainter_model_view_matrix;
		GLMatrix qpainter_projection_matrix;

		// Suspend the QPainter so we can start making calls directly to OpenGL without
		// interfering with the QPainter's OpenGL state (if it uses an OpenGL paint engine).
		suspend_qpainter(qpainter_viewport, qpainter_model_view_matrix, qpainter_projection_matrix);

		// Restore the OpenGL state to what it was before 'begin_qpainter_block' was called.
		end_state_block();

		if (restore_model_view_projection_transforms)
		{
			// While the QPainter was used it may have altered its transform so we should update
			// the modelview and projection matrices set by QPainter.
			// Easiest way to do that is simply to load them.
			gl_load_matrix(GL_MODELVIEW, qpainter_model_view_matrix);
			gl_load_matrix(GL_PROJECTION, qpainter_projection_matrix);
		}

		// This is one of the rare cases where we need to apply the OpenGL state encapsulated in
		// GLRenderer directly to OpenGL - in this case we need to make sure our last applied state
		// actually represents the state of OpenGL - which it may not because QPainter may have
		// changed the model-view and projection matrices (if it uses an OpenGL paint engine).
		apply_current_state_to_opengl();
	}
}


GPlatesOpenGL::GLRenderer::QPainterBlockScope::QPainterBlockScope(
		GLRenderer &renderer,
		bool restore_model_view_projection_transforms) :
	d_renderer(renderer),
	d_qpainter(renderer.begin_qpainter_block()),
	d_restore_model_view_projection_transforms(restore_model_view_projection_transforms)
{
}


GPlatesOpenGL::GLRenderer::QPainterBlockScope::~QPainterBlockScope()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		d_renderer.end_qpainter_block(d_restore_model_view_projection_transforms);
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during QPainter block scope: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during QPainter block scope: Unknown error";
	}
}


bool
GPlatesOpenGL::GLRenderer::supports_floating_point_render_target_2D() const
{
	// We need framebuffer object support otherwise we would end up rendering to the main
	// framebuffer but it is a fixed-point target.
	return get_context().get_capabilities().framebuffer.gl_EXT_framebuffer_object;
}


void
GPlatesOpenGL::GLRenderer::get_max_dimensions_render_target_2D(
		unsigned int &max_render_target_width,
		unsigned int &max_render_target_height) const
{
	// If using framebuffer objects for render-targets...
	if (d_framebuffer_object)
	{
		const GLCapabilities &capabilities = get_context().get_capabilities();

		// The minimum of the maximum texture width and maximum viewport width.
		max_render_target_width = capabilities.texture.gl_max_texture_size;
		if (max_render_target_width > capabilities.viewport.gl_max_viewport_width)
		{
			max_render_target_width = capabilities.viewport.gl_max_viewport_width;
		}
		// Should already be a power-of-two - but just in case.
		max_render_target_width = GPlatesUtils::Base2::previous_power_of_two(max_render_target_width);

		// The minimum of the maximum texture height and maximum viewport height.
		max_render_target_height = capabilities.texture.gl_max_texture_size;
		if (max_render_target_height > capabilities.viewport.gl_max_viewport_height)
		{
			max_render_target_height = capabilities.viewport.gl_max_viewport_height;
		}
		// Should already be a power-of-two - but just in case.
		max_render_target_height = GPlatesUtils::Base2::previous_power_of_two(max_render_target_height);
	}
	else // ...using main framebuffer as a render-target...
	{
		GPlatesGlobal::Assert<OpenGLException>(
				d_default_viewport,
				GPLATES_ASSERTION_SOURCE,
				"Must call 'GLRenderer::get_max_dimensions_render_target_2D' between begin_render/end_render.");

		// Round down to the nearest power-of-two.
		// This is because the client will be using power-of-two texture dimensions.
		max_render_target_width = GPlatesUtils::Base2::previous_power_of_two(d_default_viewport->width());
		max_render_target_height = GPlatesUtils::Base2::previous_power_of_two(d_default_viewport->height());
	}
}


void
GPlatesOpenGL::GLRenderer::begin_render_target_2D(
		const GLTexture::shared_ptr_to_const_type &texture,
		boost::optional<GLViewport> render_texture_viewport,
		const double &max_point_size_and_line_width,
		GLint level,
		bool reset_to_default_state)
{
	//PROFILE_FUNC();

	// The texture must be initialised with a width and a height.
	// If not then its either a 1D texture or it has not been initialised with
	// GLTexture::gl_tex_image_2D or GLTexture::gl_tex_image_3D.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture->get_width() && texture->get_height(),
			GPLATES_ASSERTION_SOURCE);

	// Set the default render-target viewport if it wasn't specified.
	if (!render_texture_viewport)
	{
		// The default is the entire texture.
		// Note that the texture width is for level 0 so we need to adjust if not level 0.
		render_texture_viewport = GLViewport(
				0, 0,
				texture->get_width().get() >> level,
				texture->get_height().get() >> level);
	}

	// Push a new render target block.
	begin_render_target_block_internal(
			reset_to_default_state,
			RenderTextureTarget(render_texture_viewport.get(), texture, level));

	// The current render texture target.
	// NOTE: This must reference directly into the structure stored on the render target block stack
	// since it can get modified below.
	RenderTextureTarget &render_texture_target =
			get_current_render_target_block().render_texture_target.get();

	// Disable depth writing for render targets.
	// If using framebuffer objects (as render targets) then it doesn't really matter but if using
	// the main framebuffer then its depth buffer would get corrupted if depth writes were enabled.
	gl_depth_mask(GL_FALSE);

	// Begin the current render texture target.
	if (d_framebuffer_object)
	{
		// Use framebuffer object for rendering to texture unless the driver is not supporting
		// the configuration for some reason.
		if (!begin_framebuffer_object_2D(render_texture_target))
		{
			// Start using the main framebuffer instead (for rendering to texture).
			begin_rgba8_main_framebuffer_2D(render_texture_target, max_point_size_and_line_width);
		}
	}
	else
	{
		begin_rgba8_main_framebuffer_2D(render_texture_target, max_point_size_and_line_width);
	}
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLRenderer::begin_tile_render_target_2D(
		bool save_restore_state,
		GLViewport *tile_render_target_viewport,
		GLViewport *tile_render_target_scissor_rect)
{
	// The current render texture target.
	boost::optional<RenderTextureTarget> &render_texture_target =
			get_current_render_target_block().render_texture_target;

	// Should always have a render texture target when tiling a render target 2D.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			render_texture_target,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TEXTURE_TARGET);

	// Save the current OpenGL state if requested.
	if (save_restore_state)
	{
		begin_state_block(false/*reset_to_default_state*/);
	}
	render_texture_target->tile_save_restore_state = save_restore_state;

	if (render_texture_target->main_frame_buffer)
	{
		return begin_tile_rgba8_main_framebuffer_2D(
				render_texture_target.get(),
				tile_render_target_viewport,
				tile_render_target_scissor_rect);
	}
	else
	{
		return begin_tile_framebuffer_object_2D(
				render_texture_target.get(),
				tile_render_target_viewport,
				tile_render_target_scissor_rect);
	}
}


bool
GPlatesOpenGL::GLRenderer::end_tile_render_target_2D()
{
	// The current render texture target.
	boost::optional<RenderTextureTarget> &render_texture_target =
			get_current_render_target_block().render_texture_target;

	// Should always have a render texture target when tiling a render target 2D.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			render_texture_target,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TEXTURE_TARGET);

	// End the current tile.
	const bool continue_to_next_tile = render_texture_target->main_frame_buffer
			? end_tile_rgba8_main_framebuffer_2D(render_texture_target.get())
			: end_tile_framebuffer_object_2D(render_texture_target.get());

	// Restore the OpenGL state if requested.
	if (render_texture_target->tile_save_restore_state)
	{
		end_state_block();
	}
	render_texture_target->tile_save_restore_state = false;

	return continue_to_next_tile;
}


void
GPlatesOpenGL::GLRenderer::end_render_target_2D()
{
	//PROFILE_FUNC();

	// The current render texture target.
	boost::optional<RenderTextureTarget> &render_texture_target =
			get_current_render_target_block().render_texture_target;

	// Should always have a render texture target when tiling a render target 2D.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			render_texture_target,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TEXTURE_TARGET);

	// End the current render texture target.
	if (render_texture_target->main_frame_buffer)
	{
		end_rgba8_main_framebuffer_2D(render_texture_target.get());

		// End the current render target block.
		end_render_target_block_internal();
	}
	else
	{
		// End the current render target block.
		//
		// FIXME: This is risky because we are implicitly ending a stack block here
		// before calling 'end_framebuffer_object_2D()' which could itself set some state.
		// We really want to end the render target block last so it restores all state.
		// Right now we get away with it because 'end_framebuffer_object_2D()' doesn't
		// set any global state (it only modifies the framebuffer object's state - ie, local state).
		// To fix: change std::stack to something where can access second from top element (not just top).
		end_render_target_block_internal();

		// Is there a parent render texture target (ie, not back to the main framebuffer yet).
		boost::optional<RenderTextureTarget> &parent_render_texture_target =
				get_current_render_target_block().render_texture_target;

		end_framebuffer_object_2D(parent_render_texture_target);
	}
}


void
GPlatesOpenGL::GLRenderer::begin_state_block(
		bool reset_to_default_state)
{
	// Begin a new state block.
	// Note that a new state is always created/cloned so subsequent state setting methods
	// don't modify the previous state block (or the default state block).
	//
	if (reset_to_default_state)
	{
		// We're starting out in the default OpenGL state so it doesn't matter if we're
		// currently nested inside a compiled draw state block or not.
		begin_state_block_internal(StateBlock(d_default_state->clone()));
	}
	else if (get_current_render_target_block().compile_draw_state_nest_count)
	{
		// We *are* nested a state block inside a compiled draw state block (or a nested group
		// of compiled draw state blocks or another state block that itself is nested in one or
		// more compiled draw state blocks, etc).
		// So we start out with an empty state that is relative (ie, a state *change*) to the
		// state just before this new state block.
		// This ensures that if any state is applied to OpenGL during this state block it will
		// be the full state (ie, begin state + state change).
		begin_state_block_internal(
				StateBlock(
						// Empty state...
						d_state_store->allocate_state(),
						// 'begin_state_to_apply'...
						get_current_state_block().get_state_to_apply()));
	}
	else
	{
		// We're *not* nested inside a compiled draw state block so the current state is the *full* state.
		begin_state_block_internal(StateBlock(clone_current_state()));
	}
}


void
GPlatesOpenGL::GLRenderer::end_state_block()
{
	RenderTargetBlock &current_render_target_block = get_current_render_target_block();

	// There should be at least the state block pushed in the current render target block.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!current_render_target_block.state_block_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_STATE_BLOCK);

	// Pop the current state block - the previous state block is now the current state block.
	current_render_target_block.state_block_stack.pop();

	// NOTE: We don't apply the current state directly to OpenGL, instead we wait for a draw
	// call before doing this - this is so we can minimise redundant state changes between draw calls.
}


void
GPlatesOpenGL::GLRenderer::begin_render_queue_block()
{
	// Push a new render queue.
	begin_render_queue_block_internal(RenderQueue::create());
}


void
GPlatesOpenGL::GLRenderer::end_render_queue_block()
{
	const RenderQueue::non_null_ptr_type render_queue = end_render_queue_block_internal();

	// If there are drawables in the render queue then attempt to render them now.
	// If drawables are still being queued (due to nested scopes) then they could get queued again.
	BOOST_FOREACH(const RenderOperation &render_operation, render_queue->render_operations)
	{
		draw(render_operation);
	}
}


void
GPlatesOpenGL::GLRenderer::begin_compile_draw_state(
		boost::optional<GLCompiledDrawState::non_null_ptr_type> compiled_draw_state)
{
	RenderTargetBlock &current_render_target_block = get_current_render_target_block();

	// We are compiling/recording draw state so flag that.
	++current_render_target_block.compile_draw_state_nest_count;

	// Create a new compiled draw state if one hasn't been passed in by the client.
	if (!compiled_draw_state)
	{
		// We create a new compiled draw state with a GLState that has no state sets.
		// This is important because when the compiled draw state is eventually applied we are going to apply
		// it as a state *change* to the OpenGL state that is current when the compiled draw state is applied.
		// To do this we can't have any state sets other than what the client compiles in.
		compiled_draw_state = create_empty_compiled_draw_state();
	}

	// All states during draw state compilation are now relative to the current state and
	// reflect the state change since the beginning of draw state compilation.
	// Save the current state in case we're asked to apply state during the middle of compilation
	// (because that's what the compiled state *changes* are relative to).
	const StateBlock state_block(
			compiled_draw_state.get(),
			get_current_state_block().get_state_to_apply());

	// Save the current state before we continue so we can restore it after this draw state
	// has been compiled.
	begin_state_block_internal(state_block);

	// Start a new render queue.
	begin_render_queue_block_internal(compiled_draw_state.get()->d_render_queue);
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLRenderer::end_compile_draw_state()
{
	RenderTargetBlock &current_render_target_block = get_current_render_target_block();

	// Get the compiled draw state.
	// NOTE: We need to retrieve this *before* ending the current state block.
	const boost::optional<GLCompiledDrawState::non_null_ptr_type> compiled_draw_state =
			get_current_state_block().get_compiled_draw_state();

	// We should be in a state block that is compiling draw state.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			(current_render_target_block.compile_draw_state_nest_count > 0) &&
				compiled_draw_state,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_COMPILE_DRAW_STATE_BLOCK);

	// End the compiled draw state's render queue.
	//
	// NOTE: Instead of attempting to render the queued drawables (like a render queue block would)
	// we store the render queue in the compiled draw state - this prevents the drawables from being
	// rendered until the compiled draw state is explicitly applied by the client.
	// Also note that the render queue is already stored in the compiled draw state.
	end_render_queue_block_internal();

	// Now that we've finished compiling state changes we can end the state block.
	// Also note that the current state is already stored/referenced in the compiled draw state.
	end_state_block();

	// We were compiling/recording draw state so flag that.
	--current_render_target_block.compile_draw_state_nest_count;

	return compiled_draw_state.get();
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLRenderer::create_empty_compiled_draw_state()
{
	return GLCompiledDrawState::non_null_ptr_type(
			new GLCompiledDrawState(
					// Empty state...
					d_state_store->allocate_state(),
					// Empty render queue...
					RenderQueue::create()));
}


void
GPlatesOpenGL::GLRenderer::apply_compiled_draw_state(
		const GLCompiledDrawState &compiled_draw_state)
{
	const GLState::shared_ptr_type current_state = get_current_state();

	// If there are drawables in the compiled draw state's render queue then attempt to render them now.
	// If drawables are still being queued (due to nested scopes or a draw state being compiled)
	// then they could get queued again.
	BOOST_FOREACH(const RenderOperation &render_operation, compiled_draw_state.d_render_queue->render_operations)
	{
		// Make sure the compiled state works with the current OpenGL context.
		//
		// NOTE: We only update the state *change* in the compiled draw state and not the full
		// state (the state after the state change is merged into the current state).
		// This minimises the amount of updating that we need to do.
		// For example, there's no point updating a vertex array that's *not* in the compiled state
		// because we already know it will work with the current OpenGL context.
		update_compiled_draw_state_for_current_context(*render_operation.state);

		// The states in the compiled draw state are state changes (relative to the beginning of draw
		// state compilation) and must be applied, in the form of state changes, to the current state.
		const GLState::shared_ptr_type merged_state = current_state->clone();
		merged_state->merge_state_change(*render_operation.state);

		draw(RenderOperation(merged_state, render_operation.drawable, render_operation.modifies_frame_buffer));
	}

	// Make sure the compiled state works with the current OpenGL context.
	//
	// NOTE: We only update the state *change* in the compiled draw state and not the full
	// state (the state after the state change is merged into the current state).
	// This minimises the amount of updating that we need to do.
	// For example, there's no point updating a vertex array that's *not* in the compiled state
	// because we already know it will work with the current OpenGL context.
	update_compiled_draw_state_for_current_context(*compiled_draw_state.d_state_change);

	// Apply the compiled draw state's state change to the current state.
	// Note that it's possible there were state changes but either:
	//  * no render operations, or
	//  * state changes were set *after* the draw calls (render operations).
	current_state->merge_state_change(*compiled_draw_state.d_state_change);
}


void
GPlatesOpenGL::GLRenderer::update_compiled_draw_state_for_current_context(
		GLState &compiled_state_change)
{
	// Extra care needs to be taken with vertex array objects because they cannot be shared
	// across contexts. It's possible that the client compiled some draw state in one OpenGL
	// context and is using it in another (we allow this to make it easier for clients).
	// When the vertex array object was compiled - the native vertex array object resource
	// (created in the OpenGL context that was active at compile time) was stored in the 'GLState'.
	// However we might currently be in a different OpenGL context so we might need to replace
	// the native object. We do this by getting the native object for the current OpenGL
	// context and setting that on the merged 'GLState'.
	boost::optional<GLVertexArrayObject::shared_ptr_to_const_type> bound_vertex_array_object_opt =
			compiled_state_change.get_bind_vertex_array_object();
	// There's a vertex array object bound then make sure it works with the current OpenGL context.
	if (bound_vertex_array_object_opt)
	{
		const GLVertexArrayObject::shared_ptr_to_const_type &bound_vertex_array_object =
				bound_vertex_array_object_opt.get();

		// This gets the native vertex array object appropriate for the current context.
		//
		// NOTE: 'GLVertexArrayObject::get_vertex_array_resource()' may in turn call
		// 'GLRenderer::gl_bind_vertex_array_object_internal()' if it needs to set up buffer
		// bindings on a new vertex array object (for a new OpenGL context).
		// So we have to be careful of re-entrant issues in this method.
		// Basically it's possible that a bunch of bind state could get applied before we
		// return from the current method.
		GLVertexArrayObject::resource_handle_type resource_handle;
		GLState::shared_ptr_type current_resource_state;
		GLState::shared_ptr_to_const_type target_resource_state;
		bound_vertex_array_object->get_vertex_array_resource(
				*this, resource_handle, current_resource_state, target_resource_state);

		// This overrides whatever the previous bind state was.
		compiled_state_change.set_bind_vertex_array_object(
				resource_handle, current_resource_state, target_resource_state, bound_vertex_array_object);
	}
}


void
GPlatesOpenGL::GLRenderer::gl_clear(
		GLbitfield clear_mask)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct ClearDrawable :
			public Drawable
	{
		explicit
		ClearDrawable(
				GLbitfield clear_mask_) :
			clear_mask(clear_mask_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply only the subset of state needed by 'glClear'.
			state_to_apply.apply_state_used_by_gl_clear(capabilities, last_applied_state);

			glClear(clear_mask);
		}

		GLbitfield clear_mask;
	};

	const Drawable::non_null_ptr_to_const_type drawable(
			new ClearDrawable(clear_mask));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_draw_elements(
		GLenum mode,
		GLsizei count,
		GLenum type,
		GLint indices_offset)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct DrawElementsDrawable :
			public Drawable
	{
		DrawElementsDrawable(
				GLenum mode_,
				GLsizei count_,
				GLenum type_,
				GLint indices_offset_) :
			mode(mode_),
			count(count_),
			type(type_),
			indices_offset(indices_offset_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			glDrawElements(mode, count, type, GPLATES_OPENGL_BUFFER_OFFSET(indices_offset));
		}

		GLenum mode;
		GLsizei count;
		GLenum type;
		GLint indices_offset;
	};

	const Drawable::non_null_ptr_to_const_type drawable(
			new DrawElementsDrawable(mode, count, type, indices_offset));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_draw_elements(
		GLenum mode,
		GLsizei count,
		GLenum type,
		GLint indices_offset,
		const GLBufferImpl::shared_ptr_to_const_type &vertex_element_buffer_impl)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct DrawElementsDrawable :
			public Drawable
	{
		DrawElementsDrawable(
				GLenum mode_,
				GLsizei count_,
				GLenum type_,
				GLint indices_offset_,
				const GLBufferImpl::shared_ptr_to_const_type &vertex_element_buffer_impl_) :
			mode(mode_),
			count(count_),
			type(type_),
			indices_offset(indices_offset_),
			vertex_element_buffer_impl(vertex_element_buffer_impl_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			// The client memory indices pointer.
			// NOTE: By getting the indices resource pointer here (at the OpenGL draw call) we allow
			// the buffer to be updated *after* the draw is submitted (eg, a compiled draw state).
			// This emulates how buffer objects work.
			const GLvoid *indices = vertex_element_buffer_impl->get_buffer_resource() + indices_offset;

			glDrawElements(mode, count, type, indices);
		}

		GLenum mode;
		GLsizei count;
		GLenum type;
		GLint indices_offset;
		GLBufferImpl::shared_ptr_to_const_type vertex_element_buffer_impl;
	};

	const Drawable::non_null_ptr_to_const_type drawable(
			new DrawElementsDrawable(mode, count, type, indices_offset, vertex_element_buffer_impl));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_draw_range_elements(
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		GLint indices_offset)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct DrawRangeElementsDrawable :
			public Drawable
	{
		DrawRangeElementsDrawable(
				GLenum mode_,
				GLuint start_,
				GLuint end_,
				GLsizei count_,
				GLenum type_,
				GLint indices_offset_) :
			mode(mode_),
			start(start_),
			end(end_),
			count(count_),
			type(type_),
			indices_offset(indices_offset_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			// NOTE: When using gDEBugger you'll need to change this to 'glDrawRangeElements' if you
			// want to break on it since it doesn't allow you to break on 'glDrawRangeElementsEXT'.
			glDrawRangeElementsEXT(mode, start, end, count, type, GPLATES_OPENGL_BUFFER_OFFSET(indices_offset));
		}

		GLenum mode;
		GLuint start;
		GLuint end;
		GLsizei count;
		GLenum type;
		GLint indices_offset;
	};

	// Requires GL_EXT_draw_range_elements extension.
	if (!GLEW_EXT_draw_range_elements)
	{
		// Revert to glDrawElements if extension not present.
		gl_draw_elements(mode, count, type, indices_offset);
		return;
	}

	const Drawable::non_null_ptr_to_const_type drawable(
			new DrawRangeElementsDrawable(mode, start, end, count, type, indices_offset));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_draw_range_elements(
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		GLint indices_offset,
		const GLBufferImpl::shared_ptr_to_const_type &vertex_element_buffer_impl)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct DrawRangeElementsDrawable :
			public Drawable
	{
		DrawRangeElementsDrawable(
				GLenum mode_,
				GLuint start_,
				GLuint end_,
				GLsizei count_,
				GLenum type_,
				GLint indices_offset_,
				const GLBufferImpl::shared_ptr_to_const_type &vertex_element_buffer_impl_) :
			mode(mode_),
			start(start_),
			end(end_),
			count(count_),
			type(type_),
			indices_offset(indices_offset_),
			vertex_element_buffer_impl(vertex_element_buffer_impl_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			// The client memory indices pointer.
			// NOTE: By getting the indices resource pointer here (at the OpenGL draw call) we allow
			// the buffer to be updated *after* the draw is submitted (eg, a compiled draw state).
			// This emulates how buffer objects work.
			const GLvoid *indices = vertex_element_buffer_impl->get_buffer_resource() + indices_offset;

			// NOTE: When using gDEBugger you'll need to change this to 'glDrawRangeElements' if you
			// want to break on it since it doesn't allow you to break on 'glDrawRangeElementsEXT'.
			glDrawRangeElementsEXT(mode, start, end, count, type, indices);
		}

		GLenum mode;
		GLuint start;
		GLuint end;
		GLsizei count;
		GLenum type;
		GLint indices_offset;
		GLBufferImpl::shared_ptr_to_const_type vertex_element_buffer_impl;
	};

	// Requires GL_EXT_draw_range_elements extension.
	if (!GLEW_EXT_draw_range_elements)
	{
		// Revert to glDrawElements if extension not present.
		gl_draw_elements(mode, count, type, indices_offset, vertex_element_buffer_impl);
		return;
	}

	const Drawable::non_null_ptr_to_const_type drawable(
			new DrawRangeElementsDrawable(mode, start, end, count, type, indices_offset, vertex_element_buffer_impl));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_read_pixels(
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLint offset)
{
	// We're using pixel buffers objects in this version of 'gl_read_pixels'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_pixel_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	// Wrap up the read pixels call in case it gets added to a render queue.
	struct ReadPixelsDrawable :
			public Drawable
	{
		ReadPixelsDrawable(
				GLint x_,
				GLint y_,
				GLsizei width_,
				GLsizei height_,
				GLenum format_,
				GLenum type_,
				GLint offset_) :
			x(x_),
			y(y_),
			width(width_),
			height(height_),
			format(format_),
			type(type_),
			offset(offset_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply only the subset of state needed by 'glReadPixels'.
			state_to_apply.apply_state_used_by_gl_read_pixels(capabilities, last_applied_state);

			glReadPixels(x, y, width, height, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
		}

		GLint x;
		GLint y;
		GLsizei width;
		GLsizei height;
		GLenum format;
		GLenum type;
		GLint offset;
	};

	const Drawable::non_null_ptr_to_const_type drawable(
			new ReadPixelsDrawable(x, y, width, height, format, type, offset));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_read_pixels(
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLint offset,
		const GLBufferImpl::shared_ptr_type &pixel_buffer_impl)
{
	// Wrap up the read pixels call in case it gets added to a render queue.
	struct ReadPixelsDrawable :
			public Drawable
	{
		ReadPixelsDrawable(
				GLint x_,
				GLint y_,
				GLsizei width_,
				GLsizei height_,
				GLenum format_,
				GLenum type_,
				GLint offset_,
				const GLBufferImpl::shared_ptr_type &pixel_buffer_impl_) :
			x(x_),
			y(y_),
			width(width_),
			height(height_),
			format(format_),
			type(type_),
			offset(offset_),
			pixel_buffer_impl(pixel_buffer_impl_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply only the subset of state needed by 'glReadPixels'.
			state_to_apply.apply_state_used_by_gl_read_pixels(capabilities, last_applied_state);

			// The client memory pixel data pointer.
			// NOTE: By getting the pixel data resource pointer here (at the OpenGL read pixels call)
			// we allow the buffer to be updated *after* the read pixels call is submitted
			// (eg, a compiled draw state).
			// This emulates how buffer objects work.
			GLvoid *pixels = pixel_buffer_impl->get_buffer_resource() + offset;

			glReadPixels(x, y, width, height, format, type, pixels);
		}

		GLint x;
		GLint y;
		GLsizei width;
		GLsizei height;
		GLenum format;
		GLenum type;
		GLint offset;
		GLBufferImpl::shared_ptr_type pixel_buffer_impl;
	};

	const Drawable::non_null_ptr_to_const_type drawable(
			new ReadPixelsDrawable(x, y, width, height, format, type, offset, pixel_buffer_impl));

	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable));
}


void
GPlatesOpenGL::GLRenderer::gl_copy_tex_sub_image_1D(
		GLenum texture_unit,
		GLenum texture_target,
		GLint level,
		GLint xoffset,
		GLint x,
		GLint y,
		GLsizei width)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct CopyTexSubImage1DDrawable :
			public Drawable
	{
		CopyTexSubImage1DDrawable(
				GLenum texture_target_,
				GLint level_,
				GLint xoffset_,
				GLint x_,
				GLint y_,
				GLsizei width_) :
			texture_target(texture_target_),
			level(level_),
			xoffset(xoffset_),
			x(x_),
			y(y_),
			width(width_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			glCopyTexSubImage1D(texture_target, level, xoffset, x, y, width);
		}

		GLenum texture_target;
		GLint level;
		GLint xoffset;
		GLint x;
		GLint y;
		GLsizei width;
	};

	// Set the active texture unit - glCopyTexSubImage1D targets the texture bound to it.
	// The client is expected to have bound the target texture to 'texture_unit'.
	gl_active_texture(texture_unit);

	const Drawable::non_null_ptr_to_const_type drawable(
			new CopyTexSubImage1DDrawable(texture_target, level, xoffset, x, y, width));

	// Since it's copying *from* the framebuffer to a texture it does not modify the framebuffer.
	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable, false/*modifies_frame_buffer*/));
}


void
GPlatesOpenGL::GLRenderer::gl_copy_tex_sub_image_2D(
		GLenum texture_unit,
		GLenum texture_target,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct CopyTexSubImage2DDrawable :
			public Drawable
	{
		CopyTexSubImage2DDrawable(
				GLenum texture_target_,
				GLint level_,
				GLint xoffset_,
				GLint yoffset_,
				GLint x_,
				GLint y_,
				GLsizei width_,
				GLsizei height_) :
			texture_target(texture_target_),
			level(level_),
			xoffset(xoffset_),
			yoffset(yoffset_),
			x(x_),
			y(y_),
			width(width_),
			height(height_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			glCopyTexSubImage2D(texture_target, level, xoffset, yoffset, x, y, width, height);
		}

		GLenum texture_target;
		GLint level;
		GLint xoffset;
		GLint yoffset;
		GLint x;
		GLint y;
		GLsizei width;
		GLsizei height;
	};

	// Set the active texture unit - glCopyTexSubImage2D targets the texture bound to it.
	// The client is expected to have bound the target texture to 'texture_unit'.
	gl_active_texture(texture_unit);

	const Drawable::non_null_ptr_to_const_type drawable(
			new CopyTexSubImage2DDrawable(texture_target, level, xoffset, yoffset, x, y, width, height));

	// Since it's copying *from* the framebuffer to a texture it does not modify the framebuffer.
	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable, false/*modifies_frame_buffer*/));
}


void
GPlatesOpenGL::GLRenderer::gl_copy_tex_sub_image_3D(
		GLenum texture_unit,
		GLenum texture_target,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLint zoffset,
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height)
{
	// Wrap up the draw call in case it gets added to a render queue.
	struct CopyTexSubImage3DDrawable :
			public Drawable
	{
		CopyTexSubImage3DDrawable(
				GLenum texture_target_,
				GLint level_,
				GLint xoffset_,
				GLint yoffset_,
				GLint zoffset_,
				GLint x_,
				GLint y_,
				GLsizei width_,
				GLsizei height_) :
			texture_target(texture_target_),
			level(level_),
			xoffset(xoffset_),
			yoffset(yoffset_),
			zoffset(zoffset_),
			x(x_),
			y(y_),
			width(width_),
			height(height_)
		{  }

		virtual
		void
		draw(
				const GLCapabilities &capabilities,
				const GLState &state_to_apply,
				GLState &last_applied_state) const
		{
			// Apply all state - not just a subset.
			state_to_apply.apply_state(capabilities, last_applied_state);

			glCopyTexSubImage3D(texture_target, level, xoffset, yoffset, zoffset, x, y, width, height);
		}

		GLenum texture_target;
		GLint level;
		GLint xoffset;
		GLint yoffset;
		GLint zoffset;
		GLint x;
		GLint y;
		GLsizei width;
		GLsizei height;
	};

	// Previously we checked for the GL_EXT_copy_texture extension but on MacOS this is not exposed
	// so we use the core OpenGL 1.2 function instead.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_VERSION_1_2),
			GPLATES_ASSERTION_SOURCE);

	// Set the active texture unit - glCopyTexSubImage2D targets the texture bound to it.
	// The client is expected to have bound the target texture to 'texture_unit'.
	gl_active_texture(texture_unit);

	const Drawable::non_null_ptr_to_const_type drawable(
			new CopyTexSubImage3DDrawable(texture_target, level, xoffset, yoffset, zoffset, x, y, width, height));

	// Since it's copying *from* the framebuffer to a texture it does not modify the framebuffer.
	// NOTE: The cloning of the current state is necessary so that when we render the drawable
	// later it doesn't apply state that's been modified between now and then.
	draw(RenderOperation(clone_current_state(), drawable, false/*modifies_frame_buffer*/));
}


void
GPlatesOpenGL::GLRenderer::gl_mult_matrix(
		GLenum mode,
		const GLMatrix &matrix)
{
	// Post-multiply the currently loaded matrix by the caller's matrix.
	GLMatrix post_multiplied_matrix(gl_get_matrix(mode));
	post_multiplied_matrix.gl_mult_matrix(matrix);

	// Load the post-multiplied matrix.
	gl_load_matrix(mode, post_multiplied_matrix);
}


void
GPlatesOpenGL::GLRenderer::gl_mult_texture_matrix(
		GLenum texture_unit,
		const GLMatrix &texture_matrix)
{
	// Post-multiply the currently loaded texture matrix by the caller's texture matrix.
	GLMatrix post_multiplied_texture_matrix(gl_get_texture_matrix(texture_unit));
	post_multiplied_texture_matrix.gl_mult_matrix(texture_matrix);

	// Load the post-multiplied texture matrix.
	gl_load_texture_matrix(texture_unit, post_multiplied_texture_matrix);
}


const GPlatesOpenGL::GLViewport &
GPlatesOpenGL::GLRenderer::gl_get_viewport(
		unsigned int viewport_index) const
{
	// Get the current viewport at index 'viewport_index'.
	const boost::optional<const GLViewport &> current_viewport =
			get_current_state()->get_viewport(get_capabilities(), viewport_index);

	// If we're between 'begin_render' and 'end_render' then should have a valid viewport.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			current_viewport,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TARGET_BLOCK);

	return current_viewport.get();
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLRenderer::gl_get_matrix(
		GLenum mode) const
{
	// Get the current matrix.
	const boost::optional<const GLMatrix &> current_matrix =
			get_current_state()->get_load_matrix(mode);

	// Return the current matrix, or the identity matrix if not loaded.
	return current_matrix ? current_matrix.get() : GLMatrix::IDENTITY;
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLRenderer::gl_get_texture_matrix(
		GLenum texture_unit) const
{
	// Get the current texture matrix.
	const boost::optional<const GLMatrix &> current_texture_matrix =
			get_current_state()->get_load_texture_matrix(texture_unit);

	// Return the current texture matrix, or the identity matrix if not loaded.
	return current_texture_matrix ? current_texture_matrix.get() : GLMatrix::IDENTITY;
}


void
GPlatesOpenGL::GLRenderer::apply_current_state_to_opengl()
{
	get_current_state_block().get_state_to_apply()->apply_state(
			get_capabilities(),
			*d_last_applied_state);
}


void
GPlatesOpenGL::GLRenderer::gl_bind_vertex_array_object(
		const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object)
{
	// Get the vertex array object for the current OpenGL context.
	//
	// Vertex array objects cannot be shared across contexts so the vertex array may
	// generate a new native vertex array object for the current OpenGL context.
	//
	GLVertexArrayObject::resource_handle_type resource_handle;
	GLState::shared_ptr_type current_resource_state;
	GLState::shared_ptr_to_const_type target_resource_state;
	vertex_array_object->get_vertex_array_resource(
			*this, resource_handle, current_resource_state, target_resource_state);

	// Bind the native vertex array object resource.
	get_current_state()->set_bind_vertex_array_object(
			resource_handle,
			current_resource_state,
			target_resource_state,
			vertex_array_object);
}


void
GPlatesOpenGL::GLRenderer::gl_bind_vertex_array_object_and_apply(
		const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object)
{
	// Get the vertex array object for the current OpenGL context.
	//
	// Vertex array objects cannot be shared across contexts so the vertex array may
	// generate a new native vertex array object for the current OpenGL context.
	//
	GLVertexArrayObject::resource_handle_type resource_handle;
	GLState::shared_ptr_type current_resource_state;
	GLState::shared_ptr_to_const_type target_resource_state;
	vertex_array_object->get_vertex_array_resource(
			*this, resource_handle, current_resource_state, target_resource_state);

	// Bind the native vertex array object resource.
	get_current_state()->set_bind_vertex_array_object_and_apply(
			get_capabilities(),
			resource_handle,
			current_resource_state,
			target_resource_state,
			vertex_array_object,
			*d_last_applied_state);
}


void
GPlatesOpenGL::GLRenderer::draw(
		const RenderOperation &render_operation)
{
	RenderTargetBlock &current_render_target_block = get_current_render_target_block();

	// If we're in a render queue block then we've been requested to delay rendering
	// of drawables and instead put them in a render queue.
	if (!current_render_target_block.render_queue_stack.empty())
	{
		// Add the drawable to the current render queue...
		current_render_target_block.render_queue_stack.top()->render_operations.push_back(render_operation);

		return;
	}
	// Otherwise just render the drawable now...

	// If we're in a render texture target then we can't have depth/stencil tests enabled
	// because we either don't have a depth/stencil buffer FBO attachment or
	// don't want to overwrite the depth/stencil buffer of the main framebuffer.
	// We also disallow depth writes in case the main framebuffer is being used as a render target
	// otherwise its depth buffer would get corrupted.
	if (current_render_target_block.render_texture_target)
	{
		GPlatesGlobal::Assert<GLRendererAPIError>(
				!render_operation.state->get_depth_mask() &&
					!render_operation.state->get_enable(GL_DEPTH_TEST) &&
					!render_operation.state->get_enable(GL_STENCIL_TEST),
				GPLATES_ASSERTION_SOURCE,
				GLRendererAPIError::CANNOT_ENABLE_DEPTH_STENCIL_TEST_IN_RGBA8_RENDER_TARGETS);
	}

	// Shouldn't be able to get here if we're currently compiling draw state because
	// all drawables should be queued into the compiled draw state.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			current_render_target_block.compile_draw_state_nest_count == 0,
			GPLATES_ASSERTION_SOURCE);

	// The draw command should apply any state sets that have not yet been applied (and that
	// are required for it to complete its draw command).
	//
	// NOTE: Aside from 'end_render()' this is the only place we apply the current state directly to OpenGL.
	// This is because a draw call is where the current OpenGL state comes into effect
	// (eg, which textures are used, is blending enabled, which framebuffer is targeted etc).
	// And by only applying when drawing (except as mentioned above) we can remove
	// redundant state changes made between draw calls.
	//
	// Render the drawable.
	render_operation.drawable->draw(get_capabilities(), *render_operation.state, *d_last_applied_state);

	// If the draw operation modifies the framebuffer then increment the draw count.
	//
	// NOTE: The draw count is only used (for the 2D RGBA render targets) if the main framebuffer
	// is used to simulate them (ie, if GL_EXT_framebuffer_object extension is *not* available).
	if (render_operation.modifies_frame_buffer)
	{
		++d_current_frame_buffer_draw_count;
	}
}


void
GPlatesOpenGL::GLRenderer::suspend_qpainter(
		GLViewport &qpainter_viewport,
		GLMatrix &qpainter_model_view_matrix,
		GLMatrix &qpainter_projection_matrix)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_qpainter_info,
			GPLATES_ASSERTION_SOURCE);

#if QT_VERSION >= 0x040600
	// From Qt 4.6, the default OpenGL paint engine is QPaintEngine::OpenGL2.
	// And it needs protection if we're mixing painter calls with our own native OpenGL calls.
	//
	// Get the paint engine to reset to the default OpenGL state.
	// Actually it still sets the modelview and projection matrices as if you were using
	// the 1.x paint engine (so it's not exactly the default OpenGL state).
	//
	// NOTE: This is a no-operation if the paint engine does not use OpenGL.
	d_qpainter_info->qpainter.beginNativePainting();
#else
	if (d_qpainter_info->qpainter.paintEngine()->type() == QPaintEngine::OpenGL)
	{
		// Our GLRenderer assumes it's entered in the default OpenGL state and when it exits it leaves
		// OpenGL in the default state.
		// However QPainter using the OpenGL version 1 paint engine (QPaintEngine::OpenGL) expects
		// us to restore the modelview (and projection?) matrices if we've modified them.
		// So we push the modelview and projection matrices onto the OpenGL matrix stack so that we
		// modify copies of them and we restore the originals when resuming the QPainter later.
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW); // The default matrix mode.
	}
#endif

	//
	// Retrieve the viewport, set by QPainter, from OpenGL.
	//
	// NOTE: It is *not* a good idea to retrieve state *from* OpenGL because, in the worst case,
	// this has the potential to stall the graphics pipeline - and in general it's not recommended.
	//
	// UPDATE: We can't get the viewport via OpenGL anyway because the QPainter
	// might not be using an OpenGL paint engine.
	//
#if 0
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
#endif

	// The QPainter's paint device.
	QPaintDevice *qpaint_device = d_qpainter_info->qpainter.device();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			qpaint_device,
			GPLATES_ASSERTION_SOURCE);

	// Get the viewport from the QPainter.
	const QRect viewport = d_qpainter_info->qpainter.viewport();
	// Return to the caller.
	qpainter_viewport.set_viewport(
			viewport.x(),
			// Qt and OpenGL have inverted 'y' viewport components relative to each other...
			qpaint_device->height() - viewport.y() - viewport.height(),
			viewport.width(),
			viewport.height());

	//
	// Retrieve the current modelview/projection matrices from QPainter.
	//
	// NOTE: It is *not* a good idea to retrieve state *from* OpenGL because, in the worst case,
	// this has the potential to stall the graphics pipeline - and in general it's not recommended.
	// Profiling revealed 300msec (ie, huge!) for "glGetDoublev(GL_MODELVIEW_MATRIX, ...)" when
	// rendering rasters with age grid smoothing (ie, a deep GPU pipeline to stall).
	//
	// UPDATE: We can't get the modelview/projection matrices via OpenGL anyway because the QPainter
	// might not be using an OpenGL paint engine.
	//
#if 0
	GLMatrix mvm, pm;
	glGetDoublev(GL_MODELVIEW_MATRIX, mvm.get_matrix());
	glGetDoublev(GL_PROJECTION_MATRIX, pm.get_matrix());
#endif

	// The reason for retrieving this is we track the OpenGL state and we normally assume it starts
	// out in the default state (which is the case if QPainter isn't used) but is not the case here.

	// The model-view matrix.
    const QTransform &model_view_transform = d_qpainter_info->qpainter.worldTransform();
	const GLdouble model_view_matrix[16] =
	{
        model_view_transform.m11(), model_view_transform.m12(),         0, model_view_transform.m13(),
        model_view_transform.m21(), model_view_transform.m22(),         0, model_view_transform.m23(),
		                         0,                          0,         1,                          0,
         model_view_transform.dx(),  model_view_transform.dy(),         0, model_view_transform.m33()
    };
	qpainter_model_view_matrix.gl_load_matrix(model_view_matrix);

	// The projection matrix.
	qpainter_projection_matrix.gl_load_identity();
	qpainter_projection_matrix.gl_ortho(0, qpaint_device->width(), qpaint_device->height(), 0, -999999, 999999);
}


void
GPlatesOpenGL::GLRenderer::resume_qpainter()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_qpainter_info,
			GPLATES_ASSERTION_SOURCE);

#if QT_VERSION >= 0x040600
	// From Qt 4.6, the default OpenGL paint engine is QPaintEngine::OpenGL2.
	// And it needs protection if we're mixing painter calls with our own native OpenGL calls.
	//
	// NOTE: At this point we must have restored the OpenGL state to the default state !
	// Otherwise we will stuff up the painter's OpenGL state - this is because the painter only
	// restores the state that it sets - any other state it assumes is in the default state.
	//
	// Get the paint engine to restore its OpenGL state (to what it was before 'beginNativePainting').
	//
	// NOTE: This is a no-operation if the paint engine does not use OpenGL.
	d_qpainter_info->qpainter.endNativePainting();
#else
	if (d_qpainter_info->qpainter.paintEngine()->type() == QPaintEngine::OpenGL)
	{
		// We are now in the default OpenGL state but we need to return the QPainter to the state it was in.
		// For the QPainter OpenGL2 paint engine this is not necessary but it is for the OpenGL1 paint engine.
		// So we pop the modelview and projection matrices that we pushed onto the OpenGL matrix stack.
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW); // The default matrix mode.
	}
#endif
}


void
GPlatesOpenGL::GLRenderer::begin_rgba8_main_framebuffer_2D(
		RenderTextureTarget &render_texture_target,
		const double &max_point_size_and_line_width)
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!render_texture_target.main_frame_buffer,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_RENDER_TEXTURE_MAIN_FRAME_BUFFERS);

	// Set up for tiling into the main framebuffer.
	const GLTileRender tile_render(
			d_default_viewport->width()/*render_target_width*/,
			d_default_viewport->height()/*render_target_height*/,
			render_texture_target.texture_viewport/*destination_viewport*/,
			// The border is half the point size or line width, rounded up to nearest pixel.
			// NOTE: It is important that 'max_point_size_and_line_width = 0' maps to 'border = 0'...
			static_cast<unsigned int>(0.5 * max_point_size_and_line_width + 1-1e-5));

	// Get the maximum render target dimensions needed for tiling.
	unsigned int max_tile_render_target_width;
	unsigned int max_tile_render_target_height;
	tile_render.get_max_tile_render_target_dimensions(
			max_tile_render_target_width,
			max_tile_render_target_height);

	// Keep track of the save restore texture and tiling state.
	render_texture_target.main_frame_buffer =
			boost::in_place(max_tile_render_target_width, max_tile_render_target_height, tile_render);

	// Save the portion of the main framebuffer used as a render target so we can restore it later.
	render_texture_target.main_frame_buffer->save_restore_frame_buffer.save(*this);

	// Start at the first tile.
	render_texture_target.main_frame_buffer->tile_render.first_tile();
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLRenderer::begin_tile_rgba8_main_framebuffer_2D(
		RenderTextureTarget &render_texture_target,
		GLViewport *tile_render_target_viewport,
		GLViewport *tile_render_target_scissor_rect)
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			render_texture_target.main_frame_buffer,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TEXTURE_MAIN_FRAME_BUFFER);

	GLViewport current_tile_render_target_viewport;
	render_texture_target.main_frame_buffer->tile_render.get_tile_render_target_viewport(
			current_tile_render_target_viewport);

	GLViewport current_tile_render_target_scissor_rect;
	render_texture_target.main_frame_buffer->tile_render.get_tile_render_target_scissor_rectangle(
			current_tile_render_target_scissor_rect);

	// Mask off rendering outside the current tile region in case the tile is smaller than the
	// render target. Note that the tile's viewport is slightly larger than the tile itself
	// (the scissor rectangle) in order that fat points and wide lines just outside the tile
	// have pixels rasterised inside the tile (the projection transform has also been expanded slightly).
	//
	// This includes 'gl_clear()' calls which clear the entire main framebuffer.
	gl_enable(GL_SCISSOR_TEST);
	gl_scissor(
			current_tile_render_target_scissor_rect.x(),
			current_tile_render_target_scissor_rect.y(),
			current_tile_render_target_scissor_rect.width(),
			current_tile_render_target_scissor_rect.height());
	gl_viewport(
			current_tile_render_target_viewport.x(),
			current_tile_render_target_viewport.y(),
			current_tile_render_target_viewport.width(),
			current_tile_render_target_viewport.height());

	// If caller requested the tile render target viewport.
	if (tile_render_target_viewport)
	{
		*tile_render_target_viewport = current_tile_render_target_viewport;
	}
	// If caller requested the tile render target scissor rectangle.
	if (tile_render_target_scissor_rect)
	{
		*tile_render_target_scissor_rect = current_tile_render_target_scissor_rect;
	}

	// Return the projection transform for the current tile.
	return render_texture_target.main_frame_buffer->tile_render.get_tile_projection_transform();
}


bool
GPlatesOpenGL::GLRenderer::end_tile_rgba8_main_framebuffer_2D(
		RenderTextureTarget &render_texture_target)
{
	//
	// Copy the main framebuffer (the part used for render tile) to the render target texture
	// (the part where the tile slots in).
	//

	GPlatesGlobal::Assert<GLRendererAPIError>(
			render_texture_target.main_frame_buffer,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TEXTURE_MAIN_FRAME_BUFFER);

	GLViewport tile_source_viewport;
	render_texture_target.main_frame_buffer->tile_render.get_tile_source_viewport(tile_source_viewport);

	GLViewport tile_destination_viewport;
	render_texture_target.main_frame_buffer->tile_render.get_tile_destination_viewport(tile_destination_viewport);

	// We don't want any state changes made here to interfere with the client's state changes.
	// So save the current state and revert back to it at the end of this scope.
	// We don't need to reset to the default OpenGL state because very little state
	// affects glCopyTexSubImage2D so it doesn't matter what the current OpenGL state is.
	StateBlockScope save_restore_state(*this);

	// Bind the render-target texture so we can copy the main framebuffer to it.
	gl_bind_texture(render_texture_target.texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// Copy the portion of the main framebuffer used as a render target to the render-target texture.
	gl_copy_tex_sub_image_2D(
			GL_TEXTURE0,
			GL_TEXTURE_2D,
			render_texture_target.level,
			tile_destination_viewport.x(),
			tile_destination_viewport.y(),
			tile_source_viewport.x(),
			tile_source_viewport.y(),
			tile_source_viewport.width(),
			tile_source_viewport.height());

	// Proceed to the next tile (if any).
	render_texture_target.main_frame_buffer->tile_render.next_tile();
	return !render_texture_target.main_frame_buffer->tile_render.finished();
}


void
GPlatesOpenGL::GLRenderer::end_rgba8_main_framebuffer_2D(
		RenderTextureTarget &render_texture_target)
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			render_texture_target.main_frame_buffer,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TEXTURE_MAIN_FRAME_BUFFER);

	// Restore the portion of the main framebuffer used as a render target.
	render_texture_target.main_frame_buffer->save_restore_frame_buffer.restore(*this);
}


bool
GPlatesOpenGL::GLRenderer::begin_framebuffer_object_2D(
		RenderTextureTarget &render_texture_target)
{
	// Attach the texture to the framebuffer object.
	d_framebuffer_object.get()->gl_attach_texture_2D(
			*this,
			GL_TEXTURE_2D,
			render_texture_target.texture,
			render_texture_target.level,
			GL_COLOR_ATTACHMENT0_EXT);

	// Revert to using the main framebuffer as a render-target if the framebuffer object status is invalid.
	if (!check_framebuffer_object_2D_completeness(render_texture_target.texture->get_internal_format().get()))
	{
		// Detach the texture from the framebuffer object before we return it to the framebuffer object cache.
		d_framebuffer_object.get()->gl_detach(*this, GL_COLOR_ATTACHMENT0_EXT);

		return false;
	}

	// Bind the framebuffer object to make it the active framebuffer.
	gl_bind_frame_buffer(d_framebuffer_object.get());

	return true;
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLRenderer::begin_tile_framebuffer_object_2D(
		RenderTextureTarget &render_texture_target,
		GLViewport *tile_render_target_viewport,
		GLViewport *tile_render_target_scissor_rect)
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!render_texture_target.main_frame_buffer,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_RENDER_TEXTURE_MAIN_FRAME_BUFFERS);

	// Mask off rendering outside the render target viewport (eg, due to wide lines and fat points)
	// in case the viewport specified is smaller than the render target.
	// This includes 'gl_clear()' calls which clear the entire framebuffer.
	// So set the scissor rectangle to match the render target viewport.
	gl_enable(GL_SCISSOR_TEST);
	gl_scissor(
			render_texture_target.texture_viewport.x(),
			render_texture_target.texture_viewport.y(),
			render_texture_target.texture_viewport.width(),
			render_texture_target.texture_viewport.height());
	gl_viewport(
			render_texture_target.texture_viewport.x(),
			render_texture_target.texture_viewport.y(),
			render_texture_target.texture_viewport.width(),
			render_texture_target.texture_viewport.height());

	// If caller requested the tile render target viewport.
	if (tile_render_target_viewport)
	{
		*tile_render_target_viewport = render_texture_target.texture_viewport;
	}
	// If caller requested the tile render target scissor rectangle.
	if (tile_render_target_scissor_rect)
	{
		*tile_render_target_scissor_rect = render_texture_target.texture_viewport;
	}

	// Return the identity projection transform.
	// There's only one tile covering the entire destination viewport.
	return GLTransform::create();
}


bool
GPlatesOpenGL::GLRenderer::end_tile_framebuffer_object_2D(
		RenderTextureTarget &render_texture_target)
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!render_texture_target.main_frame_buffer,
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_RENDER_TEXTURE_MAIN_FRAME_BUFFERS);

	// There's only one tile covering the entire destination viewport - so we're finished.
	return false;
}


void
GPlatesOpenGL::GLRenderer::end_framebuffer_object_2D(
		boost::optional<RenderTextureTarget> &parent_render_texture_target)
{
	// If there's no parent then we've returned to rendering to the *main* framebuffer.
	if (!parent_render_texture_target)
	{
		// Detach the texture from the current framebuffer object.
		// We're finished using the framebuffer object for now so it's good to leave it in a default
		// state so it doesn't prevent us releasing the texture resource if we need to.
		d_framebuffer_object.get()->gl_detach(*this, GL_COLOR_ATTACHMENT0_EXT);

		// We don't need to bind the *main* framebuffer because the end of the current render target
		// block also ends an implicit state block which will revert the bind state for us.

		return;
	}

	// The parent render target is now the active render target.
	// Attach the texture, of the parent render target, to the framebuffer object.
	d_framebuffer_object.get()->gl_attach_texture_2D(
			*this,
			GL_TEXTURE_2D,
			parent_render_texture_target->texture,
			parent_render_texture_target->level,
			GL_COLOR_ATTACHMENT0_EXT);

	// We don't need to bind the framebuffer object because the end of the current render target
	// block also ends an implicit state block which will revert the bind state for us.
	// Doesn't really matter though because we only use the one framebuffer object.
}


bool
GPlatesOpenGL::GLRenderer::check_framebuffer_object_2D_completeness(
		GLint render_texture_internal_format)
{
	//
	// Now that we've attached the texture to the framebuffer object we need to check for
	// framebuffer completeness.
	//

	// Checking the framebuffer status can sometimes be expensive even if called once per frame.
	// One profile measured 142msec for a single check - not sure if that was due to the check
	// or somehow the driver needed to wait for some reason and happened at that call.
	//
	// In any case we cache the results in a static variable to ensure the same check is not
	// repeated every frame (a new GLRenderer is created every frame).

	//! Typedef for a mapping of render texture internal format to 'glCheckFramebufferStatus' result.
	typedef std::map<GLint/*texture internal format*/, bool> frame_buffer_state_to_status_map_type;

	// Cache results of 'glCheckFramebufferStatus' as an optimisation since it's expensive to call.
	static frame_buffer_state_to_status_map_type framebuffer_object_status_map;

	// See if we've already cached the framebuffer completeness status for the texture internal format.
	frame_buffer_state_to_status_map_type::iterator framebuffer_status_iter =		
			framebuffer_object_status_map.find(render_texture_internal_format);
	if (framebuffer_status_iter == framebuffer_object_status_map.end())
	{
		const bool framebuffer_status = d_framebuffer_object.get()->gl_check_frame_buffer_status(*this);

		if (!framebuffer_status)
		{
			// This only emits one warning (per texture internal format) since the status map is a static variable.
			qWarning() << "Unable to render using framebuffer object due to its lack of support for "
				"texture internal format: " << render_texture_internal_format;

			// Also emit a warning if the texture is floating-point.
			// This is because we will fallback to using the main framebuffer as a render target,
			// but the main framebuffer is fixed-point.
			if (GLTexture::is_format_floating_point(render_texture_internal_format))
			{
				qWarning() << "...incorrect results likely in floating-point render-texture since "
					"falling back to using (fixed-point) main framebuffer as render-target.";
			}
		}

		framebuffer_object_status_map[render_texture_internal_format] = framebuffer_status;

		return framebuffer_status;
	}

	return framebuffer_status_iter->second;
}


void
GPlatesOpenGL::GLRenderer::begin_render_target_block_internal(
		bool reset_to_default_state,
		const boost::optional<RenderTextureTarget> &render_texture_target)
{
	// To start things off create a new render target block that contains a new state block.
	// Note that a (state) clone is always created so subsequent state setting methods don't modify
	// the default state block.

	if (reset_to_default_state)
	{
		// Push a new render target block.
		d_render_target_block_stack.push(RenderTargetBlock(render_texture_target));

		// We're starting out in the default OpenGL state so it doesn't matter if we're
		// currently nested inside a compiled draw state block or not.
		begin_state_block_internal(StateBlock(d_default_state->clone()));
	}
	else
	{
		// NOTE: Here there must already exist a render target block before we push a new one.
		// This means 'begin_render()' must use a 'reset_to_default_state' that is 'true'.
		if (get_current_render_target_block().compile_draw_state_nest_count)
		{
			// NOTE: We must query the current state block *before* pushing a new render target block.
			GLState::shared_ptr_to_const_type begin_state_to_apply = get_current_state_block().get_state_to_apply();

			// Push a new render target block.
			d_render_target_block_stack.push(RenderTargetBlock(render_texture_target));

			// We *are* nested a state block inside a compiled draw state block (or a nested group
			// of compiled draw state blocks or another state block that itself is nested in one or
			// more compiled draw state blocks, etc).
			// So we start out with an empty state that is relative (ie, a state *change*) to the
			// state just before this new state block.
			// This ensures that if any state is applied to OpenGL during this state block it will
			// be the full state (ie, begin state + state change).
			begin_state_block_internal(StateBlock(d_state_store->allocate_state(), begin_state_to_apply));
		}
		else
		{
			// NOTE: We clone the current state *before* pushing a new render target block.
			GLState::shared_ptr_type cloned_current_state = clone_current_state();

			// Push a new render target block.
			d_render_target_block_stack.push(RenderTargetBlock(render_texture_target));

			// We're *not* nested inside a compiled draw state block so the current state is the *full* state.
			begin_state_block_internal(StateBlock(cloned_current_state));
		}
	}
}


void
GPlatesOpenGL::GLRenderer::end_render_target_block_internal()
{
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!d_render_target_block_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_TARGET_BLOCK);

	RenderTargetBlock &current_render_target_block = d_render_target_block_stack.top();

	GPlatesGlobal::Assert<GLRendererAPIError>(
			current_render_target_block.render_queue_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_RENDER_QUEUE_BLOCKS);

	// End the state block for the current render target block about to be popped.
	end_state_block();

	GPlatesGlobal::Assert<GLRendererAPIError>(
			current_render_target_block.state_block_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_NO_STATE_BLOCKS);

	// Pop the current render target block.
	d_render_target_block_stack.pop();
}


void
GPlatesOpenGL::GLRenderer::begin_state_block_internal(
		const StateBlock &state_block)
{
	// Push the state block onto the stack.
	get_current_render_target_block().state_block_stack.push(state_block);

	// NOTE: We don't apply the current state directly to OpenGL, instead we wait for a draw
	// call before doing this - this is so we can minimise redundant state changes between draw calls.
}


void
GPlatesOpenGL::GLRenderer::begin_render_queue_block_internal(
		const RenderQueue::non_null_ptr_type &render_queue)
{
	// Push the render queue.
	get_current_render_target_block().render_queue_stack.push(render_queue);
}


GPlatesOpenGL::GLRendererImpl::RenderQueue::non_null_ptr_type
GPlatesOpenGL::GLRenderer::end_render_queue_block_internal()
{
	RenderTargetBlock &current_render_target_block = get_current_render_target_block();

	// There should be at least the state block pushed in the current render target block.
	GPlatesGlobal::Assert<GLRendererAPIError>(
			!current_render_target_block.render_queue_stack.empty(),
			GPLATES_ASSERTION_SOURCE,
			GLRendererAPIError::SHOULD_HAVE_A_RENDER_QUEUE_BLOCK);

	// Pop the current render queue block.
	// The previous render queue block is now the current render queue block.
	RenderQueue::non_null_ptr_type render_queue = current_render_target_block.render_queue_stack.top();
	// Pop the current render queue.
	current_render_target_block.render_queue_stack.pop();

	return render_queue;
}


GPlatesOpenGL::GLRenderer::RenderScope::RenderScope(
		GLRenderer &renderer,
		const GLViewport &default_viewport) :
	d_renderer(renderer),
	d_called_end_render(false)
{
	d_renderer.begin_render(default_viewport);
}


GPlatesOpenGL::GLRenderer::RenderScope::RenderScope(
		GLRenderer &renderer,
		QPainter &qpainter,
		bool paint_device_is_framebuffer) :
	d_renderer(renderer),
	d_called_end_render(false)
{
	d_renderer.begin_render(qpainter, paint_device_is_framebuffer);
}


GPlatesOpenGL::GLRenderer::RenderScope::~RenderScope()
{
	if (!d_called_end_render)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_renderer.end_render();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GLRenderer: exception thrown during compile draw state scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "GLRenderer: exception thrown during compile draw state scope: Unknown error";
		}
	}
}


void
GPlatesOpenGL::GLRenderer::RenderScope::end_render()
{
	if (!d_called_end_render)
	{
		d_renderer.end_render();
		d_called_end_render = true;
	}
}


GPlatesOpenGL::GLRenderer::RenderTarget2DScope::RenderTarget2DScope(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &texture,
		boost::optional<GLViewport> render_target_viewport,
		const double &max_point_size_and_line_width,
		GLint level,
		bool reset_to_default_state) :
	d_renderer(renderer),
	d_called_end_tile(true),
	d_called_end_render(false)
{
	d_renderer.begin_render_target_2D(
			texture, render_target_viewport, max_point_size_and_line_width, level, reset_to_default_state);
}


GPlatesOpenGL::GLRenderer::RenderTarget2DScope::~RenderTarget2DScope()
{
	if (!d_called_end_render)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			end_render();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GLRenderer: exception thrown during render target scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "GLRenderer: exception thrown during render target scope: Unknown error";
		}
	}
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLRenderer::RenderTarget2DScope::begin_tile(
		bool save_restore_state,
		GLViewport *tile_render_target_viewport,
		GLViewport *tile_render_target_scissor_rect)
{
	d_called_end_tile = false;
	return d_renderer.begin_tile_render_target_2D(
			save_restore_state,
			tile_render_target_viewport,
			tile_render_target_scissor_rect);
}


bool
GPlatesOpenGL::GLRenderer::RenderTarget2DScope::end_tile()
{
	d_called_end_tile = true;
	return d_renderer.end_tile_render_target_2D();
}


void
GPlatesOpenGL::GLRenderer::RenderTarget2DScope::end_render()
{
	if (!d_called_end_tile)
	{
		end_tile();
	}

	d_renderer.end_render_target_2D();
	d_called_end_render = true;
}


GPlatesOpenGL::GLRenderer::StateBlockScope::StateBlockScope(
		GLRenderer &renderer,
		bool reset_to_default_state) :
	d_renderer(renderer),
	d_called_end_state_block(false)
{
	d_renderer.begin_state_block(reset_to_default_state);
}


GPlatesOpenGL::GLRenderer::StateBlockScope::~StateBlockScope()
{
	if (!d_called_end_state_block)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_renderer.end_state_block();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GLRenderer: exception thrown during state block scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "GLRenderer: exception thrown during state block scope: Unknown error";
		}
	}
}


void
GPlatesOpenGL::GLRenderer::StateBlockScope::end_state_block()
{
	if (!d_called_end_state_block)
	{
		d_renderer.end_state_block();
		d_called_end_state_block = true;
	}
}


GPlatesOpenGL::GLRenderer::RenderQueueBlockScope::RenderQueueBlockScope(
		GLRenderer &renderer) :
	d_renderer(renderer)
{
	d_renderer.begin_render_queue_block();
}


GPlatesOpenGL::GLRenderer::RenderQueueBlockScope::~RenderQueueBlockScope()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		d_renderer.end_render_queue_block();
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during render queue scope: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during render queue scope: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::CompileDrawStateScope::CompileDrawStateScope(
		GLRenderer &renderer,
		boost::optional<GLCompiledDrawState::non_null_ptr_type> compiled_draw_state) :
	d_renderer(renderer)
{
	d_renderer.begin_compile_draw_state(compiled_draw_state);
}


GPlatesOpenGL::GLRenderer::CompileDrawStateScope::~CompileDrawStateScope()
{
	if (!d_compiled_draw_state)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_renderer.end_compile_draw_state();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GLRenderer: exception thrown during compile draw state scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "GLRenderer: exception thrown during compile draw state scope: Unknown error";
		}
	}
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLRenderer::CompileDrawStateScope::get_compiled_draw_state()
{
	if (!d_compiled_draw_state)
	{
		d_compiled_draw_state = d_renderer.end_compile_draw_state();
	}

	return d_compiled_draw_state.get();
}


GPlatesOpenGL::GLRenderer::GLRendererAPIError::GLRendererAPIError(
		const GPlatesUtils::CallStack::Trace &exception_source,
		ErrorType error_type) :
	GPlatesGlobal::PreconditionViolationError(exception_source),
	d_error_type(error_type)
{
}


const char *
GPlatesOpenGL::GLRenderer::GLRendererAPIError::exception_name() const
{
	return "GLRendererAPIError";
}


void
GPlatesOpenGL::GLRenderer::GLRendererAPIError::write_message(
		std::ostream &os) const
{
	switch (d_error_type)
	{
	case SHOULD_HAVE_NO_ACTIVE_QPAINTER:
		os << "expected no active QPainter";
		break;
	case SHOULD_HAVE_ACTIVE_QPAINTER:
		os << "expected an active QPainter";
		break;
	case SHOULD_HAVE_NO_STATE_BLOCKS:
		os << "expected no state blocks";
		break;
	case SHOULD_HAVE_A_STATE_BLOCK:
		os << "expected a state block";
		break;
	case SHOULD_HAVE_NO_RENDER_TARGET_BLOCKS:
		os << "expected no render-target blocks";
		break;
	case SHOULD_HAVE_A_RENDER_TARGET_BLOCK:
		os << "expected a render-target block";
		break;
	case SHOULD_HAVE_NO_RENDER_TEXTURE_TARGETS:
		os << "expected no render-texture targets";
		break;
	case SHOULD_HAVE_A_RENDER_TEXTURE_TARGET:
		os << "expected a render-texture target";
		break;
	case SHOULD_HAVE_NO_RENDER_TEXTURE_MAIN_FRAME_BUFFERS:
		os << "expected no render-texture main frame buffers";
		break;
	case SHOULD_HAVE_A_RENDER_TEXTURE_MAIN_FRAME_BUFFER:
		os << "expected a render-texture main frame buffer";
		break;
	case SHOULD_HAVE_NO_RENDER_QUEUE_BLOCKS:
		os << "expected no render-queue blocks";
		break;
	case SHOULD_HAVE_A_RENDER_QUEUE_BLOCK:
		os << "expected a render-queue block";
		break;
	case SHOULD_HAVE_NO_COMPILE_DRAW_STATE_BLOCKS:
		os << "expected no compile draw state blocks";
		break;
	case SHOULD_HAVE_A_COMPILE_DRAW_STATE_BLOCK:
		os << "expected a compile draw state block";
		break;
	case CANNOT_ENABLE_DEPTH_STENCIL_TEST_IN_RGBA8_RENDER_TARGETS:
		os << "cannot enable depth or stencil tests when using render targets";
		break;
	default:
		os << "unspecified error";
		break;
	}
}


GPlatesOpenGL::GLRenderer::BindFrameBufferAndApply::~BindFrameBufferAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_frame_buffer_object)
		{
			d_renderer.gl_bind_frame_buffer(d_prev_frame_buffer_object.get());
		}
		else
		{
			d_renderer.gl_unbind_frame_buffer();
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindFrameBufferAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindFrameBufferAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::UnbindFrameBufferAndApply::~UnbindFrameBufferAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_frame_buffer_object)
		{
			d_renderer.gl_bind_frame_buffer(d_prev_frame_buffer_object.get());
		}
		else
		{
			d_renderer.gl_unbind_frame_buffer();
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindFrameBufferAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindFrameBufferAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::BindProgramObjectAndApply::~BindProgramObjectAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_program_object)
		{
			d_renderer.gl_bind_program_object(d_prev_program_object.get());
		}
		else
		{
			d_renderer.gl_unbind_program_object();
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindProgramObjectAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindProgramObjectAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::UnbindProgramObjectAndApply::~UnbindProgramObjectAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_program_object)
		{
			d_renderer.gl_bind_program_object(d_prev_program_object.get());
		}
		else
		{
			d_renderer.gl_unbind_program_object();
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindProgramObjectAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindProgramObjectAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::BindTextureAndApply::~BindTextureAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_texture_object)
		{
			d_renderer.gl_bind_texture(d_prev_texture_object.get(), d_texture_unit, d_texture_target);
		}
		else
		{
			d_renderer.gl_unbind_texture(d_texture_unit, d_texture_target);
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindTextureAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindTextureAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::UnbindTextureAndApply::~UnbindTextureAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_texture_object)
		{
			d_renderer.gl_bind_texture(d_prev_texture_object.get(), d_texture_unit, d_texture_target);
		}
		else
		{
			d_renderer.gl_unbind_texture(d_texture_unit, d_texture_target);
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during UnbindTextureAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during UnbindTextureAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::BindVertexArrayObjectAndApply::~BindVertexArrayObjectAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_vertex_array_object)
		{
			d_renderer.gl_bind_vertex_array_object(d_prev_vertex_array_object.get());
		}
		else
		{
			d_renderer.gl_unbind_vertex_array_object();
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindVertexArrayObjectAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindVertexArrayObjectAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::BindBufferObjectAndApply::~BindBufferObjectAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_buffer_object)
		{
			d_renderer.gl_bind_buffer_object(d_prev_buffer_object.get(), d_target);
		}
		else
		{
			d_renderer.gl_unbind_buffer_object(d_target);
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during BindBufferObjectAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during BindBufferObjectAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLRenderer::UnbindBufferObjectAndApply::~UnbindBufferObjectAndApply()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		if (d_prev_buffer_object)
		{
			d_renderer.gl_bind_buffer_object(d_prev_buffer_object.get(), d_target);
		}
		else
		{
			d_renderer.gl_unbind_buffer_object(d_target);
		}
	}
	catch (std::exception &exc)
	{
		qWarning() << "GLRenderer: exception thrown during UnbindBufferObjectAndApply: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GLRenderer: exception thrown during UnbindBufferObjectAndApply: Unknown error";
	}
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::create_unbound_vertex_array_compiled_draw_state(
		GLRenderer &renderer)
{
	GLCompiledDrawState::non_null_ptr_to_const_type unbound_vertex_array_compiled_draw_state =	
			renderer.get_context().get_shared_state()->get_unbound_vertex_array_compiled_draw_state(renderer);

	// Compile a new draw state to return to the caller.
	// The compiled draw state above is not modifiable - we need to return one that is to the caller.
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

	// Copy pre-compiled draw state into the compiled draw state to return to the caller.
	renderer.apply_compiled_draw_state(*unbound_vertex_array_compiled_draw_state.get());

	return compile_draw_state_scope.get_compiled_draw_state();
}
