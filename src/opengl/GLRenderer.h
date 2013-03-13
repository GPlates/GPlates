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
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>
#include <QPainter>

#include "GLBufferImpl.h"
#include "GLBufferObject.h"
#include "GLCapabilities.h"
#include "GLCompiledDrawState.h"
#include "GLContext.h"
#include "GLDepthRange.h"
#include "GLFrameBufferObject.h"
#include "GLMatrix.h"
#include "GLPixelBufferObject.h"
#include "GLProgramObject.h"
#include "GLRendererImpl.h"
#include "GLState.h"
#include "GLTexture.h"
#include "GLVertexArrayObject.h"
#include "GLVertexElementBufferObject.h"
#include "GLViewport.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLStateStore;

	/**
	 * Handles OpenGL rendering to render targets (and the main framebuffer).
	 *
	 * All OpenGL rendering should go through this interface since it keeps track of OpenGL state
	 * and allows queuing of draw calls (and OpenGL state associated with each draw call).
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
		 * Reports an error using the begin/end block interface of @a GLRenderer.
		 *
		 * This indicates a programming error (like @a AssertionFailureException).
		 */
		class GLRendererAPIError :
				public GPlatesGlobal::PreconditionViolationError
		{
		public:
			enum ErrorType
			{
				SHOULD_HAVE_NO_ACTIVE_QPAINTER,
				SHOULD_HAVE_ACTIVE_QPAINTER,
				SHOULD_HAVE_NO_STATE_BLOCKS,
				SHOULD_HAVE_A_STATE_BLOCK,
				SHOULD_HAVE_NO_RENDER_TARGET_BLOCKS,
				SHOULD_HAVE_A_RENDER_TARGET_BLOCK,
				SHOULD_HAVE_NO_RENDER_TEXTURE_TARGETS,
				SHOULD_HAVE_A_RENDER_TEXTURE_TARGET,
				SHOULD_HAVE_NO_RENDER_TEXTURE_MAIN_FRAME_BUFFERS,
				SHOULD_HAVE_A_RENDER_TEXTURE_MAIN_FRAME_BUFFER,
				SHOULD_HAVE_NO_RENDER_QUEUE_BLOCKS,
				SHOULD_HAVE_A_RENDER_QUEUE_BLOCK,
				SHOULD_HAVE_NO_COMPILE_DRAW_STATE_BLOCKS,
				SHOULD_HAVE_A_COMPILE_DRAW_STATE_BLOCK,
				CANNOT_ENABLE_DEPTH_STENCIL_TEST_IN_RGBA8_RENDER_TARGETS
			};

			GLRendererAPIError(
					const GPlatesUtils::CallStack::Trace &exception_source,
					ErrorType error_type);

			~GLRendererAPIError() throw() { }

		protected:
			virtual
			const char *
			exception_name() const;

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:
			ErrorType d_error_type;
		};


		/**
		 * Creates a @a GLRenderer object
		 *
		 * NOTE: Call GLContext::create_renderer instead of calling this directly.
		 */
		static
		non_null_ptr_type
		create(
				const GLContext::non_null_ptr_type &context,
				const boost::shared_ptr<GLStateStore> &state_store)
		{
			return non_null_ptr_type(new GLRenderer(context, state_store));
		}


		/**
		 * Returns the @a GLContext passed into the constructor.
		 *
		 * Note that a shared pointer is not returned to avoid possibility of cycle shared references
		 * leading to memory leaks (@a GLContext owns a few resources which should not own it).
		 */
		GLContext &
		get_context() const
		{
			return *d_context;
		}


		/**
		 * Returns the OpenGL implementation-dependent capabilities and parameters.
		 */
		const GLCapabilities &
		get_capabilities() const
		{
			return d_context->get_capabilities();
		}


		/**
		 * Call this method before using 'this' renderer.
		 * Note: This method calls GLContext::begin_render.
		 *
		 * NOTE: This should only be called when you know the full OpenGL state is set to the default OpenGL state.
		 * This is the assumption that this renderer is making.
		 *
		 * @a default_viewport represents the viewport of the window currently attached to the OpenGL context.
		 *
		 * It's possible for one or more windows to share an OpenGL context.
		 * However the window dimensions will differ and so, in a sense, the default viewport
		 * is not really the default OpenGL state (like other default states) but is the default
		 * for the current @a begin_render / @a end_render pair in which all rendering, in that scope,
		 * is direct at *one* of the windows.
		 */
		void
		begin_render(
				const GLViewport &default_viewport);


		/**
		 * An alternative version of @a begin_render that accepts a QPainter.
		 *
		 * NOTE: The QPainter must currently be active and must exist until @a end_render is called.
		 *
		 * The QPainter is useful for rendering things like text to the framebuffer (or render target).
		 * As such the QPainter is typically attached to the *same* framebuffer (viewport widget) as this
		 * GLRenderer so that they both interleave drawing to the same target. In this case the QPainter
		 * will be using an OpenGL paint engine (ie, QPaintEngine::OpenGL or QPaintEngine::OpenGL2).
		 * An example is a QGraphicsView that has a QGLWidget viewport - here the method
		 * QGraphicsScene::drawBackground, that you override, passes an active QPainter that uses OpenGL.
		 * In this case this method assumes the Qt paint engine is in the default OpenGL state (aside from the
		 * modelview and projection matrices which it sets according to the QPainter's current transform state).
		 *
		 * However it is possible for the QPainter to target a paint device that is not the OpenGL
		 * framebuffer. In this case @a paint_device_is_framebuffer should be set to false because
		 * any rendering done using the QPainter will not end up in the OpenGL framebuffer.
		 * An example of this is rendering to SVG where the client also uses OpenGL feedback to
		 * collect projected vector primitives to write them to SVG.
		 *
		 * Upon returning, the OpenGL state will be the default state except for the modelview and
		 * projection transforms which will be set to the transform state of @a qpainter and
		 * orthographic projection.
		 *
		 * The default viewport is also obtained from @a qpainter.
		 */
		void
		begin_render(
				QPainter &qpainter,
				// Does the QPainter render to the framebuffer or some other paint device ? ...
				bool paint_device_is_framebuffer = true);

		/**
		 * Call this method after using 'this' renderer and before it is destroyed.
		 * Note: This method calls GLContext::end_render.
		 *
		 * This sets the OpenGL state back to the default OpenGL state.
		 *
		 * NOTE: If a QPainter was specified in @a begin_render and it uses an OpenGL paint engine then
		 * it will be restored to its OpenGL state (to what it was before 'begin_render' was called).
		 *
		 * @throws PreconditionViolationError if:
		 *  - the number of begin calls of a particular type (such as state blocks) does not equal
		 *    the number of end calls.
		 */
		void
		end_render();

		/**
		 * RAII class to call @a begin_render and @a end_render over a scope.
		 */
		class RenderScope :
				private boost::noncopyable
		{
		public:
			RenderScope(
					GLRenderer &renderer,
					const GLViewport &default_viewport);

			RenderScope(
					GLRenderer &renderer,
					QPainter &qpainter,
					bool paint_device_is_framebuffer = true);

			~RenderScope();

			//! Opportunity to end rendering before the scope exits (when destructor is called).
			void
			end_render();

		private:
			GLRenderer &d_renderer;
			bool d_called_end_render;
		};


		/**
		 * Returns true if we are rendering to the framebuffer of the OpenGL context.
		 *
		 * This is true unless @a begin_render was called with a QPainter whose paint device is not
		 * the framebuffer (eg, if rendering to SVG).
		 *
		 * Note that this should be called between @a begin_render and @a end_render.
		 */
		bool
		rendering_to_context_framebuffer() const;


		/**
		 * Begins rendering to a QPainter (and suspends rendering using GLRenderer).
		 *
		 * The QPainter returned is the one passed into @a begin_render, or boost::none if none passed.
		 * It can be used to make QPainter calls.
		 *
		 * @throws @a GLRendererAPIError if @a begin_render not yet called.
		 */
		boost::optional<QPainter &>
		begin_qpainter_block();

		/**
		 * Ends the current QPainter block.
		 *
		 * Upon returning no further QPainter calls can be made.
		 *
		 * If @a restore_model_view_projection_transforms is true then the modelview and projection
		 * matrices are restored using the current transform state of the QPainter.
		 * This is useful if the transform state of the QPainter was altered during usage.
		 *
		 * @throws @a GLRendererAPIError if there is no current QPainter block.
		 */
		void
		end_qpainter_block(
				bool restore_model_view_projection_transforms = false);

		/**
		 * RAII class to call @a begin_qpainter_block and @a end_qpainter_block over a scope.
		 */
		class QPainterBlockScope :
				private boost::noncopyable
		{
		public:
			explicit
			QPainterBlockScope(
					GLRenderer &renderer,
					bool restore_model_view_projection_transforms = false);

			~QPainterBlockScope();

			//! Returns the QPainter passed into GLRenderer::begin_render or boost::none if none passed in.
			boost::optional<QPainter &>
			get_qpainter() const
			{
				return d_qpainter;
			}

		private:
			GLRenderer &d_renderer;
			boost::optional<QPainter &> d_qpainter;
			bool d_restore_model_view_projection_transforms;
		};


		////////////////////////////////////////////////////////////////////
		// METHODS TO CONTROL SCOPING (SUCH AS SAVE/RESTORE STATE BLOCKS) //
		////////////////////////////////////////////////////////////////////


		/**
		 * Returns true if @a begin_render_target_2D / @a end_render_target_2D
		 * support rendering to floating-point textures.
		 *
		 * If false is returned then only the fixed-point format can be rendered to.
		 *
		 * This is effectively a test for support of the 'GL_EXT_framebuffer_object' extension.
		 */
		bool
		supports_floating_point_render_target_2D() const;


		/**
		 * Returns the maximum render target dimensions when using @a begin_render_target_2D /
		 * @a end_render_target_2D (values returned as power-of-two dimensions).
		 *
		 * This is mainly in case the 'GL_EXT_framebuffer_object' extension is not available and
		 * we fallback to the main framebuffer as a render-target. In which case the dimensions
		 * of the main framebuffer become the limit (rounded down to the nearest power-of-two).
		 *
		 * If 'GL_EXT_framebuffer_object' is supported then this effectively becomes the minimum of
		 * the maximum texture dimensions and maximum viewport dimensions.
		 *
		 * NOTE: Must be called between @a begin_render and @a end_render but not necessarily
		 * between @a begin_render_target_2D and @a end_render_target_2D.
		 *
		 * NOTE: The dimensions can change from one render to the next (if main framebuffer used
		 * as render target and the window is resized). So ideally this should be called every render.
		 */
		void
		get_max_dimensions_render_target_2D(
				unsigned int &max_render_target_width,
				unsigned int &max_render_target_height) const;


		/**
		 * Begin a new 2D render target to render into a texture (without using a depth/stencil buffer).
		 *
		 * If @a supports_floating_point_render_target_2D returns true then the target texture can
		 * be either fixed-point or floating-point. Otherwise it must be fixed-point because then
		 * rendering to the main framebuffer will occur and the main framebuffer is fixed-point.
		 *
		 * The texture should have been initialised with GLTexture::gl_tex_image_2D or GLTexture::gl_tex_image_3D.
		 * Rendering is by default to the level 0 mipmap of the texture.
		 *
		 * @a max_point_size_and_line_width specifies the maximum line width and point size of any lines
		 * and/or points to be rendered into the render target. This is only used if tiling is needed
		 * to render to the render-texture (ie, if using main framebuffer as render-target and it is
		 * smaller than the render-texture).
		 * The default value of zero can be used if no points or lines are rendered.
		 *
		 * @a render_texture_viewport specifies the region of the render-texture that you will limit
		 * rendering to - by default this is the entire texture..
		 *
		 * This uses the texture target GL_TEXTURE_2D. If you want other texture targets then use
		 * @a GLFrameBufferObject instead (along with @a gl_bind_frame_buffer to make it active).
		 *
		 * NOTE: A depth/stencil buffer is not provided and so depth and stencil test must be turned off.
		 * This is because if a framebuffer object is used internally it won't have a depth/stencil buffer.
		 * And if the main framebuffer is used then we don't want to overwrite its depth/stencil buffer.
		 * In general GPlates surface layer rendering does not need a depth buffer.
		 * If you want an off-screen render target with a depth buffer then use @a GLFrameBufferObject.
		 * And you can use @a gl_bind_frame_buffer to make it active.
		 *
		 * This begin / end render target block is also an implicit state block.
		 * See @a begin_state_block / @a end_state_block (they are called internally by this render target block).
		 *
		 * If @a reset_to_default_state is true then this block starts off in the default OpenGL state.
		 * However, when the matching @a end_render_target_2D is called then the state will be
		 * restored to what it was before @a begin_render_target_2D was called.
		 * Unlike state blocks, the default value for @a reset_to_default_state is to reset to the default state.
		 * This is because the state required for render targets is usually fairly independent of the
		 * state outside the render target scope.
		 *
		 * NOTE: Render target 2D blocks can be nested.
		 *
		 * NOTE: Render target 2D blocks can also be nested in other blocks types (such as state blocks,
		 * render queue blocks, compiled draw state blocks) and vice versa.
		 * However if a render target 2D block is nested *inside* a render queue block then any
		 * rendering done within the render target 2D block is *not* queued (instead it is rendered
		 * immediately - unless the render target 2D block, in turn, has a render queue block within it).
		 * This, in turn, means a render target 2D block nested *inside* a compile draw state block
		 * will *not* compile its draw commands into the compiled draw state.
		 *
		 * @throws GLRendererAPIError if:
		 *  - @a texture has not been initialised, or
		 *  - @a depth or stencil testing is enabled.
		 */
		void
		begin_render_target_2D(
				const GLTexture::shared_ptr_to_const_type &texture,
				boost::optional<GLViewport> render_texture_viewport = boost::none,
				const double &max_point_size_and_line_width = 0,
				GLint level = 0,
				bool reset_to_default_state = true);

		/**
		 * Begins a tile (sub-region) of the current 2D render target.
		 *
		 * The returned transform is an adjustment to the projection transform normally used to
		 * render to the render target. The adjustment should be pre-multiplied with the normal
		 * projection transform and the result used as the actual projection transform. This ensures
		 * only the tile region of the view frustum is rendered to.
		 *
		 * @a tile_render_target_viewport and @a tile_render_target_scissor_rect are the rectangles
		 * specified internally to @a gl_viewport and @a gl_scissor, respectively.
		 * The viewport contains the tile border required to prevent clipping of wide lines and fat points.
		 * A stencil rectangle prevents rasterization of pixels outside the actual tile region.
		 * NOTE: You do not need to call @a gl_viewport or @a gl_scissor (they are done internally).
		 *
		 * NOTE: @a begin_tile_render_target_2D and @a end_tile_render_target_2D
		 * do *not* save / restore the OpenGL state.
		 *
		 * Note: If the 'GL_EXT_framebuffer_object' extension is supported then only one tile is
		 * needed and it covers the entire render target. Otherwise the main framebuffer is being
		 * used as a render target and more than one tile rendering might be needed if the framebuffer
		 * is smaller than the render target texture. But this is all opaque in this interface.
		 *
		 * Must be called inside a @a begin_render_target_2D / @a end_render_target_2D pair.
		 */
		GLTransform::non_null_ptr_to_const_type
		begin_tile_render_target_2D(
				GLViewport *tile_render_target_viewport = NULL,
				GLViewport *tile_render_target_scissor_rect = NULL);

		/**
		 * Ends the current tile (sub-region) of the current 2D render target.
		 *
		 * Returns true if another tile needs to be rendered in which case another
		 * @a begin_tile_render_target_2D / @a end_tile_render_target_2D pair must be rendered.
		 * For example:
		 *    renderer.begin_render_target_2D(...);
		 *    do
		 *    {
		 *        renderer.begin_tile_render_target_2D();
		 *        ... // Render scene.
		 *    } while (!renderer.end_tile_render_target_2D());
		 *    renderer.end_render_target_2D();
		 *
		 * NOTE: @a begin_tile_render_target_2D and @a end_tile_render_target_2D
		 * do *not* save / restore the OpenGL state.
		 *
		 * Must be called inside a @a begin_render_target_2D / @a end_render_target_2D pair.
		 */
		bool
		end_tile_render_target_2D();

		/**
		 * Ends the current 2D render target.
		 *
		 * Note that render target blocks can be nested.
		 *
		 * @throws @a GLRendererAPIError if:
		 *  - there is no current render target block, or
		 *  - there are render-queue and state blocks that were not nested properly.
		 */
		void
		end_render_target_2D();


		/**
		 * RAII class to call @a begin_render_target_2D and @a end_render_target_2D over a scope.
		 */
		class RenderTarget2DScope :
				private boost::noncopyable
		{
		public:
			RenderTarget2DScope(
					GLRenderer &renderer,
					const GLTexture::shared_ptr_to_const_type &texture,
					boost::optional<GLViewport> render_target_viewport = boost::none,
					const double &max_point_size_and_line_width = 0,
					GLint level = 0,
					bool reset_to_default_state = true);

			~RenderTarget2DScope();

			/**
			 * Begins a tile of the current 2D render target - see 'begin_tile_render_target_2D()'.
			 */
			GLTransform::non_null_ptr_to_const_type
			begin_tile(
					GLViewport *tile_render_target_viewport = NULL,
					GLViewport *tile_render_target_scissor_rect = NULL);

			/**
			 * Ends the current tile of the current 2D render target - see 'end_tile_render_target_2D()'.
			 */
			bool
			end_tile();

		private:
			GLRenderer &d_renderer;
		};


		/**
		 * Begins a new state block.
		 *
		 * All state set between a @a begin_state_block and @a end_state_block pair will
		 * be reverted when @a end_state_block is called.
		 *
		 * If @a reset_to_default_state is true then this state block starts off in the default OpenGL state.
		 * However, when the matching @a end_state_block is called, the state will be restored to
		 * what it was before @a begin_state_block was called.
		 * The default value for @a reset_to_default_state is *not* to reset to the default state.
		 *
		 * NOTE: State blocks can be nested.
		 *
		 * NOTE: State blocks can also be nested in other blocks types (such as render target blocks,
		 * render queue blocks, compiled draw state blocks) and vice versa.
		 *
		 * @throws @a GLRendererAPIError if @a begin_render not yet called.
		 */
		void
		begin_state_block(
				bool reset_to_default_state = false);

		/**
		 * Ends the current state block.
		 *
		 * All state set between a @a begin_state_block and @a end_state_block pair will be reverted
		 * when this method returns. In other words, the state will be what it was just before
		 * the matching @a begin_state_block call was made.
		 *
		 * NOTE: State blocks can be nested.
		 *
		 * @throws @a GLRendererAPIError if there is no current state block.
		 */
		void
		end_state_block();

		/**
		 * RAII class to call @a begin_state_block and @a end_state_block over a scope.
		 */
		class StateBlockScope :
				private boost::noncopyable
		{
		public:
			explicit
			StateBlockScope(
					GLRenderer &renderer,
					bool reset_to_default_state = false);

			~StateBlockScope();

		private:
			GLRenderer &d_renderer;
		};


		/**
		 * Begins queueing draw calls to a new render queue.
		 *
		 * This queues all draw calls (except in nested render target 2D blocks - see above) into
		 * a queue that is emitted when @a end_render_queue_block is called.
		 *
		 * The primary reason for having render queues is to avoid costly main framebuffer updates
		 * when the GL_EXT_framebuffer_object extension is not available and the main framebuffer is
		 * used to simulate render targets.
		 *
		 * For example:
		 * 
		 *   begin_render_queue_block
		 *   
		 *     begin_render_target_2D A
		 *       draw to render target (immediately)
		 *     end_render_target_2D
		 *     
		 *     draw X using render texture A (queued)
		 *     
		 *     begin_render_target_2D B
		 *       draw to render target (immediately)
		 *     end_render_target_2D
		 *     
		 *     draw Y using render texture B (queued)
		 *     
		 *   end_render_queue_block (both X and Y are drawn now)
		 *
		 * ...in the above the queueing of draw calls X and Y means render targets A and B do not
		 * need to save/restore the main framebuffer when they've finished using it as a render target.
		 * Each render target will still copy from the main framebuffer to the render texture but
		 * there doesn't need to be a save of the main framebuffer before the render target or a
		 * restore after the render target.
		 *
		 * NOTE: Render queue blocks can be nested.
		 * In this case (aside from render target 2D blocks) the queued draw calls are emitted
		 * when the outermost render queue block (of the current render target block) exits scope.
		 *
		 * NOTE: Render queue blocks can also be nested in other blocks types (such as state blocks,
		 * compile draw state blocks, render target blocks) and vice versa.
		 *
		 * However a render target 2D block nested *inside* a render queue block will *not*
		 * queue its draw commands into the render queue.
		 * This is by design since a render target block could stream vertices through a single
		 * vertex buffer and emit multiple draw calls as it does this - if we queued those draw
		 * calls then they'd all draw the last set of vertices (currently) in the vertex buffer.
		 * Granted that this can also happen in situations that don't use a render target block.
		 *
		 * WARNING: So in general, as noted above, this is something to be careful of. That is mixing
		 * streaming to vertex buffers with queued rendering can lead to unexpected or undesired results.
		 *
		 * @throws @a GLRendererAPIError if @a begin_render not yet called.
		 */
		void
		begin_render_queue_block();

		/**
		 * Ends the current render queue block.
		 *
		 * @throws @a GLRendererAPIError if there is no current render queue block.
		 */
		void
		end_render_queue_block();

		/**
		 * RAII class to call @a begin_render_queue_block and @a end_render_queue_block over a scope.
		 */
		class RenderQueueBlockScope :
				private boost::noncopyable
		{
		public:
			explicit
			RenderQueueBlockScope(
					GLRenderer &renderer);

			~RenderQueueBlockScope();

		private:
			GLRenderer &d_renderer;
		};


		/**
		 * Begins compiling a draw state.
		 *
		 * All renderer set state calls and draw calls will be compiled to a draw state
		 * instead of rendered to OpenGL.
		 *
		 * If a valid @a compiled_draw_state is passed in then compilation will continue into it,
		 * otherwise a new compiled draw state will be created.
		 * Note that @a end_compile_draw_state will return the @a compiled_draw_state object
		 * (in the former case) or the newly created one (in the latter case).
		 *
		 * All state set between a @a begin_compile_draw_state and @a end_compile_draw_state pair will
		 * be reverted when @a end_compile_draw_state is called (so that compilation doesn't
		 * interfere with any state set outside of compilation).
		 * Also all draw calls are not rendered but instead compiled into the draw state.
		 *
		 * NOTE: Only state that is set while compiling the draw state is stored.
		 * Any currently active state (ie, before @a begin_compile_draw_state) is not compiled into the draw state.
		 * So whenever the compiled draw state is *applied* (see @a apply_compiled_draw_state)
		 * it effectively adds to state that is already active at the time it is applied and hence
		 * depends on the context in which it is applied.
		 *
		 * NOTE: Compile draw state blocks can be nested.
		 *
		 * NOTE: Compile draw state blocks can also be nested in other blocks types (such as state blocks,
		 * render queue blocks, render target blocks) and vice versa.
		 * However a render target 2D block nested *inside* a compile draw state block will *not*
		 * compile its draw commands into the compiled draw state.
		 * Also a compile draw state block can be nested in a render queue block without any effect
		 * on the compiled draw state.
		 *
		 * NOTE: It is also possible to call @a apply_compiled_draw_state while compiling another
		 * draw state in order to generate hierarchical compiled draw states.
		 *
		 * @throws @a GLRendererAPIError if @a begin_render not yet called.
		 */
		void
		begin_compile_draw_state(
				boost::optional<GLCompiledDrawState::non_null_ptr_type> compiled_draw_state = boost::none);

		/**
		 * Ends compilation of a draw state and returns the compiled draw state (which is either
		 * a new compile draw state or the compile draw state passed into @a begin_compile_draw_state).
		 *
		 * All renderer set state class and draws calls since @a begin_compile_draw_state are
		 * compiled into the returned compiled draw state.
		 *
		 * The returned compiled draw state can be applied by calling @a apply_compiled_draw_state.
		 *
		 * NOTE: The implementation of compiled draw states is such that they can be used across
		 * different OpenGL contexts - this is primarily due to the way @a GLVertexArrayObject is
		 * implemented (normally native OpenGL vertex array objects cannot be shared across contexts).
		 */
		GLCompiledDrawState::non_null_ptr_type
		end_compile_draw_state();

		/**
		 * RAII class to call @a begin_compile_draw_state and @a end_compile_draw_state over a scope.
		 */
		class CompileDrawStateScope :
				private boost::noncopyable
		{
		public:
			explicit
			CompileDrawStateScope(
					GLRenderer &renderer,
					boost::optional<GLCompiledDrawState::non_null_ptr_type> compiled_draw_state = boost::none);

			~CompileDrawStateScope();

			/**
			 * Returns the compiled draw state (which is also the one passed into constructor if
			 * one was passed in, otherwise a new compiled draw state).
			 *
			 * This method will also call 'GLRenderer::end_compile_draw_state()' if necessary.
			 */
			GLCompiledDrawState::non_null_ptr_type
			get_compiled_draw_state();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLCompiledDrawState::non_null_ptr_type> d_compiled_draw_state;
		};

		/**
		 * Creates an empty compiled draw state that contains no state changes or draw commands.
		 *
		 * This is useful when you want to postpone compilation (or do incremental compilation).
		 * In other words you start out with an empty compiled draw state and then later ask
		 * a renderer to compile some state into it or modify some state in it.
		 *
		 * NOTE: It is *not* necessary to call this method to compile a draw state.
		 * You can just call @a begin_compile_draw_state and @a end_compile_draw_state since
		 * @a end_compile_draw_state returns a compiled draw state.
		 */
		GLCompiledDrawState::non_null_ptr_type
		create_empty_compiled_draw_state();


		///////////////////////////////////////////////////////////////////////////////////
		// METHODS THAT READ OR WRITE TO THE CURRENT FRAMEBUFFER AND/OR SET OPENGL STATE //
		///////////////////////////////////////////////////////////////////////////////////


		/**
		 * Applies the current renderer state directly to OpenGL.
		 *
		 * Only differences in state between the current state and @a d_last_applied_state need be applied.
		 */


		/**
		 * Applies the specified compiled draw state.
		 *
		 * This renders draw calls compiled into the compiled draw state (if any) and applies
		 * any state changes (compiled into draw state).
		 *
		 * NOTE: This applies only the OpenGL states that were compiled - any OpenGL states not compiled
		 * will use the currently active OpenGL state (eg, if the blend state was compiled but not
		 * depth state then the current depth state is used and the compiled blend state is used).
		 *
		 * NOTE: This method can be called while compiling another draw state in order to
		 * generate hierarchical compiled draw states.
		 *
		 * NOTE: A compiled draw state does not need any referenced objects (such as bound buffers, textures,
		 * etc) to remain alive when it is applied because they are kept alive by the compiled draw state.
		 * For example, if a compiled draw state binds a texture but then all (other) references to
		 * that texture are destroyed then the texture will still exist and will still be bound when
		 * the compiled draw state is applied.
		 */
		void
		apply_compiled_draw_state(
				const GLCompiledDrawState &compiled_draw_state);

		/**
		 * Clears the current framebuffer.
		 *
		 * @a clear_mask is the same as the argument to the OpenGL function 'glClear()'.
		 * That is a bitwise combination of GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, etc.
		 */
		void
		gl_clear(
				GLbitfield clear_mask);

		/**
		 * Performs same function as the glCopyTexSubImage1D OpenGL function.
		 *
		 * NOTE: The target of the copy is the texture bound to texture unit @a texture_unit.
		 * So you first need to bind a texture on that unit using @a gl_bind_texture.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_copy_tex_sub_image_1D(
				GLenum texture_unit,
				GLenum texture_target,
				GLint level,
				GLint xoffset,
				GLint x,
				GLint y,
				GLsizei width);

		/**
		 * Performs same function as the glCopyTexSubImage2D OpenGL function.
		 *
		 * NOTE: The target of the copy is the texture bound to texture unit @a texture_unit.
		 * So you first need to bind a texture on that unit using @a gl_bind_texture.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_copy_tex_sub_image_2D(
				GLenum texture_unit,
				GLenum texture_target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);

		/**
		 * Performs same function as the glCopyTexSubImage3D OpenGL function.
		 *
		 * NOTE: OpenGL 1.2 must be available.
		 *
		 * NOTE: The target of the copy is the texture bound to texture unit @a texture_unit.
		 * So you first need to bind a texture on that unit using @a gl_bind_texture.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_copy_tex_sub_image_3D(
				GLenum texture_unit,
				GLenum texture_target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLint zoffset,
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);


		///////////////////////////////////
		// METHODS THAT SET OPENGL STATE //
		///////////////////////////////////


		/**
		 * Binds the specified framebuffer object - requires the GL_EXT_framebuffer_object extension.
		 */
		void
		gl_bind_frame_buffer(
				const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object)
		{
			get_current_state()->set_bind_frame_buffer(frame_buffer_object);
		}

		/**
		 * Unbinds any currently bound framebuffer object - the *main* framebuffer is now targeted.
		 */
		void
		gl_unbind_frame_buffer()
		{
			get_current_state()->set_unbind_frame_buffer();
		}


		/**
		 * Binds the specified shader program object - requires the GL_ARB_shader_objects extension.
		 */
		void
		gl_bind_program_object(
				const GLProgramObject::shared_ptr_to_const_type &program_object)
		{
			get_current_state()->set_bind_program_object(program_object);
		}

		/**
		 * Unbinds any currently bound shader program object - the fixed-function pipeline is now used exclusively.
		 */
		void
		gl_unbind_program_object()
		{
			get_current_state()->set_unbind_program_object();
		}


		/**
		 * Binds a texture to the specified texture unit.
		 *
		 * NOTE: @a texture must be initialised.
		 */
		void
		gl_bind_texture(
				const GLTexture::shared_ptr_to_const_type &texture_object,
				GLenum texture_unit,
				GLenum texture_target)
		{
			get_current_state()->set_bind_texture(
					get_capabilities(), texture_object, texture_unit, texture_target);
		}

		/**
		 * Unbinds any currently bound texture object on the specified texture unit and texture target.
		 */
		void
		gl_unbind_texture(
				GLenum texture_unit,
				GLenum texture_target)
		{
			get_current_state()->set_unbind_texture(get_capabilities(), texture_unit, texture_target);
		}


		//! Sets the OpenGL colour mask.
		void
		gl_color_mask(
				GLboolean red = GL_TRUE,
				GLboolean green = GL_TRUE,
				GLboolean blue = GL_TRUE,
				GLboolean alpha = GL_TRUE)
		{
			get_current_state()->set_color_mask(red, green, blue, alpha);
		}

		//! Sets the OpenGL depth mask.
		void
		gl_depth_mask(
				GLboolean flag = GL_TRUE)
		{
			get_current_state()->set_depth_mask(flag);
		}

		//! Sets the OpenGL stencil mask.
		void
		gl_stencil_mask(
				GLuint stencil = ~GLuint(0)/*all ones*/)
		{
			get_current_state()->set_stencil_mask(stencil);
		}

		//! Sets the OpenGL clear colour.
		void
		gl_clear_color(
				GLclampf red = GLclampf(0.0),
				GLclampf green = GLclampf(0.0),
				GLclampf blue = GLclampf(0.0),
				GLclampf alpha = GLclampf(0.0))
		{
			get_current_state()->set_clear_color(red, green, blue, alpha);
		}

		//! Sets the OpenGL clear depth value.
		void
		gl_clear_depth(
				GLclampd depth = GLclampd(1.0))
		{
			get_current_state()->set_clear_depth(depth);
		}

		//! Sets the OpenGL clear stencil value.
		void
		gl_clear_stencil(
				GLint stencil = 0)
		{
			get_current_state()->set_clear_stencil(stencil);
		}

		/**
		 * Sets the current scissor rectangle (GL_SCISSOR_TEST also needs to be enabled).
		 *
		 * If the GL_ARB_viewport_array extension is supported then all scissor rectangles are set
		 * to the same parameters in accordance with that extension.
		 * For hardware that does not support this extension there is only one scissor rectangle.
		 */
		void
		gl_scissor(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height)
		{
			get_current_state()->set_scissor(
					get_capabilities(), GLViewport(x, y, width, height), d_default_viewport.get());
		}

		/**
		 * Sets all scissor rectangles to the parameters specified in @a all_scissor_rectangles.
		 *
		 * NOTE: @a all_scissor_rectangles must contain exactly 'context.get_capabilities().viewport.gl_max_viewports' rectangles.
		 *
		 * If the GL_ARB_viewport_array extension is not available then only one scissor rectangle is available.
		 *
		 * @throws PreconditionViolationError if:
		 *  all_scissor_rectangles.size() != context.get_capabilities().viewport.gl_max_viewports
		 */
		void
		gl_scissor_array(
				const std::vector<GLViewport> &all_scissor_rectangles)
		{
			get_current_state()->set_scissor_array(
					get_capabilities(), all_scissor_rectangles, d_default_viewport.get());
		}

		/**
		 * Sets the current viewport.
		 *
		 * If the GL_ARB_viewport_array extension is supported then all viewports are set to
		 * the same parameters in accordance with that extension.
		 * For hardware that does not support this extension there is only one viewport.
		 */
		void
		gl_viewport(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height)
		{
			get_current_state()->set_viewport(
					get_capabilities(), GLViewport(x, y, width, height), d_default_viewport.get());
		}

		/**
		 * Sets all viewports to the parameters specified in @a all_viewports.
		 *
		 * NOTE: @a all_viewports must contain exactly 'context.get_capabilities().viewport.gl_max_viewports' viewports.
		 *
		 * If the GL_ARB_viewport_array extension is not available then only one viewport is available.
		 *
		 * NOTE: With the GL_ARB_viewport_array the viewport parameters can be *floating-point* but
		 * we keep them as integers here for simplicity (for now).
		 *
		 * @throws PreconditionViolationError if:
		 *  all_viewports.size() != context.get_capabilities().viewport.gl_max_viewports
		 */
		void
		gl_viewport_array(
				const std::vector<GLViewport> &all_viewports)
		{
			get_current_state()->set_viewport_array(
					get_capabilities(), all_viewports, d_default_viewport.get());
		}

		/**
		 * Sets the depth range for the current viewport.
		 *
		 * If the GL_ARB_viewport_array extension is supported then all viewports are set to
		 * the same parameters in accordance with that extension.
		 * For hardware that does not support this extension there is only one viewport.
		 */
		void
		gl_depth_range(
				GLclampd zNear = 0.0,
				GLclampd zFar = 1.0)
		{
			get_current_state()->set_depth_range(get_capabilities(), GLDepthRange(zNear, zFar));
		}

		/**
		 * Sets depth ranges of all viewports to the parameters specified in @a all_depth_ranges.
		 *
		 * NOTE: @a all_depth_ranges must contain exactly 'context.get_capabilities().viewport.gl_max_viewports' viewports.
		 *
		 * If the GL_ARB_viewport_array extension is not available then only one viewport/depth-range is available.
		 *
		 * @throws PreconditionViolationError if:
		 *  all_depth_ranges.size() != context.get_capabilities().viewport.gl_max_viewports
		 */
		void
		gl_depth_range_array(
				const std::vector<GLDepthRange> &all_depth_ranges)
		{
			get_current_state()->set_depth_range_array(get_capabilities(), all_depth_ranges);
		}

		/**
		 * Enable/disable the specified capability.
		 *
		 * NOTE: For enabling/disabling texturing use @a gl_enable_texture instead.
		 *
		 * NOTE: All capabilities are disabled by default except two (GL_DITHER and GL_MULTISAMPLE).

		 *
		 * Also note that only a subset of the capabilities are currently supported in the
		 * implementation although it is easy to add more as needed (see 'GLStateSetKeys::get_enable_key').
		 */
		void
		gl_enable(
				GLenum cap,
				bool enable = true)
		{
			get_current_state()->set_enable(cap, enable);
		}

		/**
		 * Enable/disable texturing for the specified target and texture unit.
		 *
		 * NOTE: All texture unit are disabled by default.
		 */
		void
		gl_enable_texture(
				GLenum texture_unit,
				GLenum texture_target,
				bool enable = true)
		{
			get_current_state()->set_enable_texture(texture_unit, texture_target, enable);
		}

		//! Sets the point size.
		void
		gl_point_size(
				GLfloat size = GLfloat(1))
		{
			get_current_state()->set_point_size(size);
		}

		//! Sets the line width.
		void
		gl_line_width(
				GLfloat width = GLfloat(1))
		{
			get_current_state()->set_line_width(width);
		}

		void
		gl_polygon_mode(
				GLenum face = GL_FRONT_AND_BACK,
				GLenum mode = GL_FILL)
		{
			get_current_state()->set_polygon_mode(face, mode);
		}

		void
		gl_front_face(
				GLenum mode = GL_CCW)
		{
			get_current_state()->set_front_face(mode);
		}

		void
		gl_cull_face(
				GLenum mode = GL_BACK)
		{
			get_current_state()->set_cull_face(mode);
		}

		void
		gl_polygon_offset(
				GLfloat factor = GLfloat(0),
				GLfloat units = GLfloat(0))
		{
			get_current_state()->set_polygon_offset(factor, units);
		}

		/**
		 * Specify a hint.
		 *
		 * Also note that only a subset of the hints are currently supported in the
		 * implementation although it is easy to add more as needed (see 'GLStateSetKeys::get_hint_key').
		 */
		void
		gl_hint(
				GLenum target,
				GLenum mode)
		{
			get_current_state()->set_hint(target, mode);
		}

		//! Sets the alpha test function.
		void
		gl_alpha_func(
				GLenum func = GL_ALWAYS,
				GLclampf ref = GLclampf(0))
		{
			get_current_state()->set_alpha_func(func, ref);
		}

		//! Sets the depth function.
		void
		gl_depth_func(
				GLenum func = GL_LESS)
		{
			get_current_state()->set_depth_func(func);
		}

		/**
		 * Sets the alpha-blend equation (NOTE: you'll also want to enable blending).
		 *
		 * NOTE: This requires the GL_EXT_blend_minmax extension.
		 */
		void
		gl_blend_equation(
				GLenum mode = DEFAULT_BLEND_EQUATION)
		{
			get_current_state()->set_blend_equation(mode);
		}

		/**
		 * Sets the alpha-blend equation separately for RGB and Alpha (NOTE: you'll also want to enable blending).
		 *
		 * NOTE: This requires the GL_EXT_blend_equation_separate extension.
		 */
		void
		gl_blend_equation_separate(
				GLenum modeRGB = DEFAULT_BLEND_EQUATION,
				GLenum modeAlpha = DEFAULT_BLEND_EQUATION)
		{
			get_current_state()->set_blend_equation_separate(modeRGB, modeAlpha);
		}

		//! Sets the alpha-blend function (NOTE: you'll also want to enable blending).
		void
		gl_blend_func(
				GLenum sfactor = GL_ONE,
				GLenum dfactor = GL_ZERO)
		{
			get_current_state()->set_blend_func(sfactor, dfactor);
		}

		/**
		 * Sets the alpha-blend function separately for RGB and Alpha (NOTE: you'll also want to enable blending).
		 *
		 * NOTE: This requires the GL_EXT_blend_func_separate extension.
		 */
		void
		gl_blend_func_separate(
				GLenum sfactorRGB = GL_ONE,
				GLenum dfactorRGB = GL_ZERO,
				GLenum sfactorAlpha = GL_ONE,
				GLenum dfactorAlpha = GL_ZERO)
		{
			get_current_state()->set_blend_func_separate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
		}

		/**
		 * Sets the specified texture environment state to the specified parameter on the specified texture unit.
		 *
		 * NOTE: 'ParamType' should be one of GLint, GLfloat, std::vector<GLint> or std::vector<GLfloat>.
		 * These correspond to the OpenGL functions 'glTexEnvi', 'glTexEnvf', 'glTexEnviv' and 'glTexEnvfv'.
		 */
		template <typename ParamType>
		void
		gl_tex_env(
				GLenum texture_unit,
				GLenum target,
				GLenum pname,
				const ParamType &param)
		{
			get_current_state()->set_tex_env(texture_unit, target, pname, param);
		}

		/**
		 * Sets the specified texture coordinate generation state to the specified parameter on the specified texture unit.
		 *
		 * NOTE: 'ParamType' should be one of GLint, GLfloat, GLdouble, std::vector<GLint>, std::vector<GLfloat> or std::vector<GLdouble>.
		 * These correspond to the OpenGL functions 'glTexGeni', 'glTexGenf', 'glTexGend', 'glTexGeniv', 'glTexGenfv' and 'glTexGendv'.
		 */
		template <typename ParamType>
		void
		gl_tex_gen(
				GLenum texture_unit,
				GLenum coord,
				GLenum pname,
				const ParamType &param)
		{
			get_current_state()->set_tex_gen(texture_unit, coord, pname, param);
		}

		/**
		 * Loads the specified matrix into the specified matrix mode.
		 *
		 * Valid values for @a mode are GL_MODELVIEW and GL_PROJECTION.
		 *
		 * NOTE: Use @a gl_load_texture_matrix for texture matrices (GL_TEXTURE matrix mode).
		 */
		void
		gl_load_matrix(
				GLenum mode,
				const GLMatrix &matrix)
		{
			get_current_state()->set_load_matrix(mode, matrix);
		}

		/**
		 * Loads the specified matrix into the texture matrix (GL_TEXTURE) for the specified texture unit.
		 */
		void
		gl_load_texture_matrix(
				GLenum texture_unit,
				const GLMatrix &texture_matrix)
		{
			get_current_state()->set_load_texture_matrix(texture_unit, texture_matrix);
		}

		/**
		 * Post-multiplies the current matrix (for @a mode) by @a matrix.
		 *
		 * If there is no current matrix loaded this is the same as calling @a gl_load_matrix.
		 *
		 * Note that you can use @a StateBlockScope to push/pop render state (including matrix state).
		 */
		void
		gl_mult_matrix(
				GLenum mode,
				const GLMatrix &matrix);

		/**
		 * Post-multiplies the current texture matrix (GL_TEXTURE) on the specified texture unit by @a texture_matrix.
		 *
		 * If there is no current matrix loaded this is the same as calling @a gl_load_texture_matrix.
		 *
		 * Note that you can use @a StateBlockScope to push/pop render state (including matrix state).
		 */
		void
		gl_mult_texture_matrix(
				GLenum texture_unit,
				const GLMatrix &texture_matrix);


		/////////////////////////////////////////////////////////////////////////////////////////////
		//                                                                                         //
		// The following methods are 'get' methods.                                                //
		//                                                                                         //
		// NOTE: Mostly only those required by the renderer framework implementation are provided. //
		//                                                                                         //
		/////////////////////////////////////////////////////////////////////////////////////////////


		/**
		 * Returns the current viewport at index @a viewport_index (default index is zero).
		 *
		 * If there's only one viewport set (via @a gl_viewport) then use the default index of zero.
		 *
		 * NOTE: @a viewport_index must be less than 'context.get_capabilities().viewport.gl_max_viewports'.
		 * NOTE: And must be called between @a begin_render and @a end_render as usual.
		 */
		const GLViewport &
		gl_get_viewport(
				unsigned int viewport_index = 0) const;


		/**
		 * Returns the current matrix for the specified matrix mode, or identity if not set.
		 *
		 * Valid values for @a mode are GL_MODELVIEW and GL_PROJECTION.
		 *
		 * NOTE: Use @a gl_get_texture_matrix for texture matrices (GL_TEXTURE matrix mode).
		 */
		const GLMatrix &
		gl_get_matrix(
				GLenum mode) const;

		/**
		 * Returns the current texture matrix for the specified texture unit, or identity if not set.
		 */
		const GLMatrix &
		gl_get_texture_matrix(
				GLenum texture_unit) const;

	private:

		/**
		 * Information about the QPainter, if any, that is active at the beginning of rendering.
		 */
		struct QPainterInfo
		{
			explicit
			QPainterInfo(
					QPainter &qpainter_,
					bool paint_device_is_framebuffer_ = true) :
				qpainter(qpainter_),
				paint_device_is_framebuffer(paint_device_is_framebuffer_)
			{  }

			QPainter &qpainter;

			//! Does the QPainter render to the framebuffer or some other paint device ?
			bool paint_device_is_framebuffer;
		};


		/**
		 * The default blend equation for 'glBlendEquation()'.
		 *
		 * This is here because 'GL_FUNC_ADD' is only in GLEW.h and that cannot be included in
		 * header files because GLEW.h must be included before GL.h and that's too difficult unless
		 * GLEW.h is only included at top of '.cc' files.
		 */
		static const GLenum DEFAULT_BLEND_EQUATION;

		/**
		 * Is valid if a QPainter is active during rendering (when @a begin_render is called).
		 */
		boost::optional<QPainterInfo> d_qpainter_info;

		/**
		 * Used to begin/end rendering and manage framebuffer objects.
		 */
		GLContext::non_null_ptr_type d_context;

		/**
		 * The viewport of the window currently attached to the OpenGL context.
		 */
		boost::optional<GLViewport> d_default_viewport;

		/**
		 * Used to efficiently allocate @a GLState objects.
		 */
		boost::shared_ptr<GLStateStore> d_state_store;

		/**
		 * Represents the default OpenGL state.
		 *
		 * Typically it won't have any states set which means it's the default state.
		 * One exception is the default viewport which represents the dimensions of the window
		 * currently attached to the OpenGL context.
		 *
		 * NOTE: This must be declared after @a d_state_set_store and @a d_state_set_keys.
		 */
		GLState::shared_ptr_type d_default_state;

		/**
		 * Represents the actual OpenGL state (as it was last applied to OpenGL).
		 *
		 * NOTE: This must be declared after @a d_state_set_store and @a d_state_set_keys.
		 */
		GLState::shared_ptr_type d_last_applied_state;

		/**
		 * The current draw count for draw calls that modify any framebuffers.
		 *
		 * Only used when the GL_EXT_framebuffer_object extension is not available and hence
		 * the main framebuffer is being used for render targets.
		 */
		GLRendererImpl::frame_buffer_draw_count_type d_current_frame_buffer_draw_count;

		/**
		 * The framebuffer object to use for render targets (if GL_EXT_framebuff_object supported).
		 */
		boost::optional<GLFrameBufferObject::shared_ptr_type> d_framebuffer_object;

		/**
		 * Stack of currently render target blocks.
		 *
		 * The first block pushed always represents the main framebuffer.
		 * Subsequent blocks represent render-to-texture targets.
		 */
		GLRendererImpl::render_target_block_stack_type d_render_target_block_stack;


		//! Constructor.
		GLRenderer(
				const GLContext::non_null_ptr_type &context,
				const boost::shared_ptr<GLStateStore> &state_store);


		/**
		 * Returns the current 'const' render target block.
		 */
		const GLRendererImpl::RenderTargetBlock &
		get_current_render_target_block() const
		{
			// There should be at least the state block pushed in 'begin_render'.
			GPlatesGlobal::Assert<GLRendererAPIError>(
					!d_render_target_block_stack.empty(),
					GPLATES_ASSERTION_SOURCE,
					GLRendererAPIError::SHOULD_HAVE_A_RENDER_TARGET_BLOCK);

			return d_render_target_block_stack.top();
		}

		/**
		 * Returns the current 'non-const' render target block.
		 */
		GLRendererImpl::RenderTargetBlock &
		get_current_render_target_block()
		{
			// There should be at least the state block pushed in 'begin_render'.
			GPlatesGlobal::Assert<GLRendererAPIError>(
					!d_render_target_block_stack.empty(),
					GPLATES_ASSERTION_SOURCE,
					GLRendererAPIError::SHOULD_HAVE_A_RENDER_TARGET_BLOCK);

			return d_render_target_block_stack.top();
		}


		/**
		 * Returns the current 'const' state block.
		 */
		const GLRendererImpl::StateBlock &
		get_current_state_block() const
		{
			const GLRendererImpl::RenderTargetBlock &current_render_target_block = get_current_render_target_block();

			// There should be at least the state block pushed in the current render target block.
			GPlatesGlobal::Assert<GLRendererAPIError>(
					!current_render_target_block.state_block_stack.empty(),
					GPLATES_ASSERTION_SOURCE,
					GLRendererAPIError::SHOULD_HAVE_A_STATE_BLOCK);

			return current_render_target_block.state_block_stack.top();
		}

		/**
		 * Returns the current 'non-const' state block.
		 */
		GLRendererImpl::StateBlock &
		get_current_state_block()
		{
			GLRendererImpl::RenderTargetBlock &current_render_target_block = get_current_render_target_block();

			// There should be at least the state block pushed in the current render target block.
			GPlatesGlobal::Assert<GLRendererAPIError>(
					!current_render_target_block.state_block_stack.empty(),
					GPLATES_ASSERTION_SOURCE,
					GLRendererAPIError::SHOULD_HAVE_A_STATE_BLOCK);

			return current_render_target_block.state_block_stack.top();
		}


		/**
		 * Returns the 'const' state of the current state block of the current render target block.
		 */
		inline
		GLState::shared_ptr_to_const_type
		get_current_state() const
		{
			return get_current_state_block().get_current_state();
		}

		/**
		 * Returns the 'non-const' state of the current state block of the current render target block.
		 */
		inline
		const GLState::shared_ptr_type &
		get_current_state()
		{
			return get_current_state_block().get_current_state();
		}


		/**
		 * Clones the state of the current state block (top of stack).
		 */
		inline
		GLState::shared_ptr_type
		clone_current_state()
		{
			return get_current_state()->clone();
		}

		/**
		 * When compiled draw state is applied it may need to be updated to work with the current
		 * OpenGL context if it is a different context than when the draw state was compiled.
		 */
		void
		update_compiled_draw_state_for_current_context(
				GLState &compiled_state_change);

		/**
		 * Renders a render operation containing a drawable and a state.
		 *
		 * The render operation can be queued for later rendering in certain situations.
		 */
		void
		draw(
				const GLRendererImpl::RenderOperation &render_operation);

		void
		suspend_qpainter(
				GLViewport &qpainter_viewport,
				GLMatrix &qpainter_model_view_matrix,
				GLMatrix &qpainter_projection_matrix);

		//! NOTE: OpenGL must be in the default state before this is called.
		void
		resume_qpainter();

		void
		begin_rgba8_main_framebuffer_2D(
				GLRendererImpl::RenderTextureTarget &render_texture_target,
				const double &max_point_size_and_line_width);

		GLTransform::non_null_ptr_to_const_type
		begin_tile_rgba8_main_framebuffer_2D(
				GLRendererImpl::RenderTextureTarget &render_texture_target,
				GLViewport *tile_render_target_viewport,
				GLViewport *tile_render_target_scissor_rect);

		bool
		end_tile_rgba8_main_framebuffer_2D(
				GLRendererImpl::RenderTextureTarget &render_texture_target);

		void
		end_rgba8_main_framebuffer_2D(
				const GLRendererImpl::RenderTextureTarget &render_texture_target);

		bool
		begin_framebuffer_object_2D(
				GLRendererImpl::RenderTextureTarget &render_texture_target);

		GLTransform::non_null_ptr_to_const_type
		begin_tile_framebuffer_object_2D(
				GLRendererImpl::RenderTextureTarget &render_texture_target,
				GLViewport *tile_render_target_viewport,
				GLViewport *tile_render_target_scissor_rect);

		bool
		end_tile_framebuffer_object_2D(
				GLRendererImpl::RenderTextureTarget &render_texture_target);

		void
		end_framebuffer_object_2D(
				const boost::optional<GLRendererImpl::RenderTextureTarget> &parent_render_texture_target);

		bool
		check_framebuffer_object_2D_completeness(
				GLint render_texture_internal_format);

		void
		begin_render_target_block_internal(
				bool reset_to_default_state,
				const boost::optional<GLRendererImpl::RenderTextureTarget> &render_texture_target = boost::none);

		void
		end_render_target_block_internal();

		void
		begin_state_block_internal(
				const GLRendererImpl::StateBlock &state_block);

		void
		begin_render_queue_block_internal(
				const GLRendererImpl::RenderQueue::non_null_ptr_type &render_queue);

		GLRendererImpl::RenderQueue::non_null_ptr_type
		end_render_queue_block_internal();

		/**
		 * Performs same function as the glDrawElements OpenGL function.
		 *
		 * This overload uses the bound vertex element buffer object for obtaining vertex indices.
		 */
		void
		gl_draw_elements(
				GLenum mode,
				GLsizei count,
				GLenum type,
				GLint indices_offset);

		/**
		 * Performs same function as the glDrawElements OpenGL function.
		 *
		 * This overload uses client memory (no bound objects) for obtaining vertex indices.
		 */
		void
		gl_draw_elements(
				GLenum mode,
				GLsizei count,
				GLenum type,
				GLint indices_offset,
				const GLBufferImpl::shared_ptr_to_const_type &vertex_element_buffer_impl);

	public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// NOTE: This public grouping is placed at the bottom of the class so that it's less noticeable.
	// These functions either:
	//  1) aren't generally needed by general clients (only needed to help build an OpenGL framework), or
	//  2) are handled better (more transparently) using OpenGL framework objects outside of GLRenderer
	//     such as GLVertexArray which, in turn, calls some of these GLRenderer methods.
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////


		/**
		 * Applies the current renderer state immediately to OpenGL instead of waiting until the next
		 * un-queued draw command (or any command that writes to, or reads from, the current framebuffer).
		 *
		 * Normally it's better to *not* apply immediately because it allows the opportunity to
		 * remove redundant state changes between draw calls.
		 *
		 * NOTE: This method should *not* be used in general rendering.
		 * It should only be used to help implement the renderer framework.
		 * Another case is when interacting with code beyond our control that directly
		 * calls OpenGL such as Qt (eg, during text rendering).
		 */
		void
		apply_current_state_to_opengl();


		/**
		 * Performs same function as the glDrawRangeElements OpenGL function.
		 *
		 * This overload uses the bound vertex element buffer object for obtaining vertex indices.
		 *
		 * NOTE: If the GL_EXT_draw_range_elements extension is not present then reverts to
		 * using @a gl_draw_elements instead (ie, @a start and @a end are effectively ignored).
		 */
		void
		gl_draw_range_elements(
				GLenum mode,
				GLuint start,
				GLuint end,
				GLsizei count,
				GLenum type,
				GLint indices_offset);

		/**
		 * Performs same function as the glDrawRangeElements OpenGL function.
		 *
		 * This overload uses client memory (no bound objects) for obtaining vertex indices.
		 *
		 * NOTE: If the GL_EXT_draw_range_elements extension is not present then reverts to
		 * using @a gl_draw_elements instead (ie, @a start and @a end are effectively ignored).
		 */
		void
		gl_draw_range_elements(
				GLenum mode,
				GLuint start,
				GLuint end,
				GLsizei count,
				GLenum type,
				GLint indices_offset,
				const GLBufferImpl::shared_ptr_to_const_type &vertex_element_buffer_impl);


		/**
		 * Performs the equivalent of the OpenGL command 'glReadPixels'.
		 *
		 * This overload uses the bound pixel buffer object (on the *pack* target) to write pixel data to.
		 */
		void
		gl_read_pixels(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				GLint offset);


		/**
		 * Performs the equivalent of the OpenGL command 'glReadPixels'.
		 *
		 * This overload uses client memory (no bound objects) to write pixel data to.
		 */
		void
		gl_read_pixels(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				GLint offset,
				const GLBufferImpl::shared_ptr_type &pixel_buffer_impl);


		/**
		 * Sets the currently active texture unit.
		 *
		 * NOTE: This is a detail that clients shouldn't have to worry about - in other words
		 * they should be able to specify the texture unit in any public methods that involves
		 * textures and not have to explicitly set the active texture unit beforehand.
		 *
		 * However there are some situations where it needs to be set such as specifying a texture
		 * coordinate array for a texture unit or calling @a gl_copy_tex_image_2D (that
		 * copies to the texture currently bound on the active texture unit).
		 *
		 * @throws PreconditionViolationError if @a active_texture is not in the half-open range:
		 *   [GL_TEXTURE0,
		 *    GL_TEXTURE0 + context.get_capabilities().texture.gl_max_texture_units_ARB).
		 */
		void
		gl_active_texture(
				GLenum active_texture)
		{
			get_current_state()->set_active_texture(get_capabilities(), active_texture);
		}

		/**
		 * Sets the currently active texture unit targeted by vertex texture coordinate arrays.
		 *
		 * NOTE: This is a detail that clients shouldn't have to worry about - in other words
		 * they should be able to specify the texture unit in any public methods that involves
		 * textures and not have to explicitly set the active texture unit beforehand.
		 *
		 * @throws PreconditionViolationError if @a client_active_texture is not in the half-open range:
		 *   [GL_TEXTURE0,
		 *    GL_TEXTURE0 + context.get_capabilities().texture.gl_max_texture_units_ARB).
		 */
		void
		gl_client_active_texture(
				GLenum client_active_texture)
		{
			get_current_state()->set_client_active_texture(get_capabilities(), client_active_texture);
		}

		/**
		 * Specifies which matrix stack is the target for matrix operations.
		 */
		void
		gl_matrix_mode(
				GLenum mode)
		{
			get_current_state()->set_matrix_mode(mode);
		}

		/**
		 * Binds the specified framebuffer object and applies directly to OpenGL - requires the GL_EXT_framebuffer_object extension.
		 *
		 * NOTE: Should only be used by the implementation of @a GLFrameBufferObject - since it makes *direct*
		 * calls to OpenGL. Ensures the object is bound immediately in OpenGL instead of the next draw call.
		 */
		void
		gl_bind_frame_buffer_and_apply(
				const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object)
		{
			get_current_state()->set_bind_frame_buffer_and_apply(
					get_capabilities(), frame_buffer_object, *d_last_applied_state);
		}

		/**
		 * RAII class to bind, and apply, to OpenGL over a scope (reverts bind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class BindFrameBufferAndApply :
				private boost::noncopyable
		{
		public:
			BindFrameBufferAndApply(
					GLRenderer &renderer,
					const GLFrameBufferObject::shared_ptr_to_const_type &frame_buffer_object) :
				d_renderer(renderer),
				d_prev_frame_buffer_object(renderer.get_current_state()->get_bind_frame_buffer())
			{
				renderer.gl_bind_frame_buffer_and_apply(frame_buffer_object);
			}

			~BindFrameBufferAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> d_prev_frame_buffer_object;
		};

		/**
		 * Unbinds any currently bound framebuffer object and applies directly to OpenGL -
		 * requires the GL_EXT_framebuffer_object extension.
		 *
		 * NOTE: Should only be used by the implementation of @a GLFrameBufferObject - since it makes *direct*
		 * calls to OpenGL. Ensures an object is unbound immediately in OpenGL instead of the next draw call.
		 */
		void
		gl_unbind_frame_buffer_and_apply()
		{
			get_current_state()->set_unbind_frame_buffer_and_apply(get_capabilities(), *d_last_applied_state);
		}

		/**
		 * RAII class to unbind, and apply, to OpenGL over a scope (reverts bind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class UnbindFrameBufferAndApply :
				private boost::noncopyable
		{
		public:
			explicit
			UnbindFrameBufferAndApply(
					GLRenderer &renderer) :
				d_renderer(renderer),
				d_prev_frame_buffer_object(renderer.get_current_state()->get_bind_frame_buffer())
			{
				renderer.gl_unbind_frame_buffer_and_apply();
			}

			~UnbindFrameBufferAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> d_prev_frame_buffer_object;
		};

		/**
		 * Binds the specified shader program object and applies directly to OpenGL - requires the GL_ARB_shader_objects extension.
		 *
		 * NOTE: Should only be used by the implementation of @a GLProgramObject - since it makes *direct*
		 * calls to OpenGL. Ensures the object is bound immediately in OpenGL instead of the next draw call.
		 */
		void
		gl_bind_program_object_and_apply(
				const GLProgramObject::shared_ptr_to_const_type &program_object)
		{
			get_current_state()->set_bind_program_object_and_apply(
					get_capabilities(), program_object, *d_last_applied_state);
		}

		/**
		 * RAII class to bind, and apply, to OpenGL over a scope (reverts bind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class BindProgramObjectAndApply :
				private boost::noncopyable
		{
		public:
			BindProgramObjectAndApply(
					GLRenderer &renderer,
					const GLProgramObject::shared_ptr_to_const_type &program_object) :
				d_renderer(renderer),
				d_prev_program_object(renderer.get_current_state()->get_bind_program_object())
			{
				renderer.gl_bind_program_object_and_apply(program_object);
			}

			~BindProgramObjectAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLProgramObject::shared_ptr_to_const_type> d_prev_program_object;
		};

		/**
		 * Same as @a gl_unbind_program_object but also applies binding directly to OpenGL.
		 */
		void
		gl_unbind_program_object_and_apply()
		{
			get_current_state()->set_unbind_program_object_and_apply(get_capabilities(), *d_last_applied_state);
		}

		/**
		 * RAII class to unbind, and apply, to OpenGL over a scope (reverts unbind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know that no object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class UnbindProgramObjectAndApply :
				private boost::noncopyable
		{
		public:
			explicit
			UnbindProgramObjectAndApply(
					GLRenderer &renderer) :
				d_renderer(renderer),
				d_prev_program_object(renderer.get_current_state()->get_bind_program_object())
			{
				renderer.gl_unbind_program_object_and_apply();
			}

			~UnbindProgramObjectAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLProgramObject::shared_ptr_to_const_type> d_prev_program_object;
		};

		/**
		 * Binds a texture to the specified texture unit and applies directly to OpenGL.
		 *
		 * NOTE: Should only be used by the implementation of @a GLTexture - since it makes *direct*
		 * calls to OpenGL. Ensures the object is bound immediately in OpenGL instead of the next draw call.
		 *
		 * NOTE: @a texture must be initialised.
		 */
		void
		gl_bind_texture_and_apply(
				const GLTexture::shared_ptr_to_const_type &texture_object,
				GLenum texture_unit,
				GLenum texture_target)
		{
			get_current_state()->set_bind_texture_and_apply(
					get_capabilities(), texture_object, texture_unit, texture_target, *d_last_applied_state);
		}

		/**
		 * RAII class to bind, and apply, to OpenGL over a scope (reverts bind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class BindTextureAndApply :
				private boost::noncopyable
		{
		public:
			BindTextureAndApply(
					GLRenderer &renderer,
					const GLTexture::shared_ptr_to_const_type &texture_object,
					GLenum texture_unit,
					GLenum texture_target) :
				d_renderer(renderer),
				d_prev_texture_object(renderer.get_current_state()->get_bind_texture(texture_unit, texture_target)),
				d_texture_unit(texture_unit),
				d_texture_target(texture_target)
			{
				renderer.gl_bind_texture_and_apply(texture_object, texture_unit, texture_target);
			}

			~BindTextureAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLTexture::shared_ptr_to_const_type> d_prev_texture_object;
			GLenum d_texture_unit;
			GLenum d_texture_target;
		};

		/**
		 * Same as @a gl_unbind_texture but also applies binding directly to OpenGL.
		 */
		void
		gl_unbind_texture_and_apply(
				GLenum texture_unit,
				GLenum texture_target)
		{
			get_current_state()->set_unbind_texture_and_apply(
					get_capabilities(), texture_unit, texture_target, *d_last_applied_state);
		}

		/**
		 * RAII class to unbind, and apply, to OpenGL over a scope (reverts unbind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know that no object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class UnbindTextureAndApply :
				private boost::noncopyable
		{
		public:
			UnbindTextureAndApply(
					GLRenderer &renderer,
					GLenum texture_unit,
					GLenum texture_target) :
				d_renderer(renderer),
				d_prev_texture_object(renderer.get_current_state()->get_bind_texture(texture_unit, texture_target)),
				d_texture_unit(texture_unit),
				d_texture_target(texture_target)
			{
				renderer.gl_unbind_texture_and_apply(texture_unit, texture_target);
			}

			~UnbindTextureAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLTexture::shared_ptr_to_const_type> d_prev_texture_object;
			GLenum d_texture_unit;
			GLenum d_texture_target;
		};

		/**
		 * Binds a vertex array object - requires the GL_ARB_vertex_array_object extension.
		 */
		void
		gl_bind_vertex_array_object(
				const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object);

		/**
		 * Same as @a gl_bind_vertex_array_object but also applies binding directly to OpenGL.
		 *
		 * NOTE: Should only be used by the implementation of @a GLVertexArrayObject - since it makes *direct*
		 * calls to OpenGL. Ensures the object is bound immediately in OpenGL instead of the next draw call.
		 */
		void
		gl_bind_vertex_array_object_and_apply(
				const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object);

		/**
		 * RAII class to bind, and apply, to OpenGL over a scope (reverts bind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class BindVertexArrayObjectAndApply :
				private boost::noncopyable
		{
		public:
			BindVertexArrayObjectAndApply(
					GLRenderer &renderer,
					const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object) :
				d_renderer(renderer),
				d_prev_vertex_array_object(renderer.get_current_state()->get_bind_vertex_array_object())
			{
				renderer.gl_bind_vertex_array_object_and_apply(vertex_array_object);
			}

			~BindVertexArrayObjectAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLVertexArrayObject::shared_ptr_to_const_type> d_prev_vertex_array_object;
		};

		/**
		 * Unbinds any currently bound vertex array object.
		 */
		void
		gl_unbind_vertex_array_object()
		{
			get_current_state()->set_unbind_vertex_array_object();
		}

		/**
		 * Binds a vertex element buffer object - requires the GL_ARB_vertex_buffer_object extension.
		 */
		void
		gl_bind_vertex_element_buffer_object(
				const GLVertexElementBufferObject::shared_ptr_to_const_type &vertex_element_buffer_object)
		{
			gl_bind_buffer_object(
					vertex_element_buffer_object->get_buffer_object(),
					GLVertexElementBufferObject::get_target_type());
		}

		/**
		 * Unbinds any currently bound vertex element buffer object.
		 */
		void
		gl_unbind_vertex_element_buffer_object()
		{
			gl_unbind_buffer_object(GLVertexElementBufferObject::get_target_type());
		}

		/**
		 * Binds a vertex buffer object - requires the GL_ARB_vertex_buffer_object extension.
		 */
		void
		gl_bind_vertex_buffer_object(
				const GLVertexBufferObject::shared_ptr_to_const_type &vertex_buffer_object)
		{
			gl_bind_buffer_object(
					vertex_buffer_object->get_buffer_object(),
					GLVertexBufferObject::get_target_type());
		}

		/**
		 * Unbinds any currently bound vertex buffer object.
		 */
		void
		gl_unbind_vertex_buffer_object()
		{
			gl_unbind_buffer_object(GLVertexBufferObject::get_target_type());
		}

		/**
		 * Binds a pixel buffer object on the *unpack* target - requires the GL_ARB_pixel_buffer_object extension.
		 */
		void
		gl_bind_pixel_unpack_buffer_object(
				const GLPixelBufferObject::shared_ptr_to_const_type &pixel_buffer_object)
		{
			gl_bind_buffer_object(
					pixel_buffer_object->get_buffer_object(),
					GLPixelBufferObject::get_unpack_target_type());
		}

		/**
		 * Unbinds any currently bound pixel buffer object on the *unpack* target.
		 */
		void
		gl_unbind_pixel_unpack_buffer_object()
		{
			gl_unbind_buffer_object(GLPixelBufferObject::get_unpack_target_type());
		}

		/**
		 * Binds a pixel buffer object on the *pack* target - requires the GL_ARB_pixel_buffer_object extension.
		 */
		void
		gl_bind_pixel_pack_buffer_object(
				const GLPixelBufferObject::shared_ptr_to_const_type &pixel_buffer_object)
		{
			gl_bind_buffer_object(
					pixel_buffer_object->get_buffer_object(),
					GLPixelBufferObject::get_pack_target_type());
		}

		/**
		 * Unbinds any currently bound pixel buffer object on the *pack* target.
		 */
		void
		gl_unbind_pixel_pack_buffer_object()
		{
			gl_unbind_buffer_object(GLPixelBufferObject::get_pack_target_type());
		}

		/**
		 * Binds a buffer object to the specified target - requires the GL_ARB_vertex_buffer_object extension.
		 */
		void
		gl_bind_buffer_object(
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum target)
		{
			get_current_state()->set_bind_buffer_object(buffer_object, target);
		}

		/**
		 * Same as @a gl_bind_buffer_object but also applies binding directly to OpenGL.
		 */
		void
		gl_bind_buffer_object_and_apply(
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum target)
		{
			get_current_state()->set_bind_buffer_object_and_apply(
					get_capabilities(), buffer_object, target, *d_last_applied_state);
		}

		/**
		 * RAII class to bind, and apply, to OpenGL over a scope (reverts bind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class BindBufferObjectAndApply :
				private boost::noncopyable
		{
		public:
			BindBufferObjectAndApply(
					GLRenderer &renderer,
					const GLBufferObject::shared_ptr_to_const_type &buffer_object,
					GLenum target) :
				d_renderer(renderer),
				d_prev_buffer_object(renderer.get_current_state()->get_bind_buffer_object(target)),
				d_target(target)
			{
				renderer.gl_bind_buffer_object_and_apply(buffer_object, target);
			}

			~BindBufferObjectAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLBufferObject::shared_ptr_to_const_type> d_prev_buffer_object;
			GLenum d_target;
		};

		/**
		 * Unbinds any buffer object currently bound to the specified target.
		 */
		void
		gl_unbind_buffer_object(
				GLenum target)
		{
			get_current_state()->set_unbind_buffer_object(target);
		}

		/**
		 * Same as @a gl_unbind_buffer_object but also applies binding directly to OpenGL.
		 */
		void
		gl_unbind_buffer_object_and_apply(
				GLenum target)
		{
			get_current_state()->set_unbind_buffer_object_and_apply(
					get_capabilities(), target, *d_last_applied_state);
		}

		/**
		 * RAII class to unbind, and apply, to OpenGL over a scope (reverts unbind, but not apply, at scope exit).
		 *
		 * The apply is only needed for the scope duration since client needs to know that no object is bound directly to OpenGL.
		 * This class is a lot more efficient than using @a StateBlockScope to save/restore all state.
		 */
		class UnbindBufferObjectAndApply :
				private boost::noncopyable
		{
		public:
			UnbindBufferObjectAndApply(
					GLRenderer &renderer,
					GLenum target) :
				d_renderer(renderer),
				d_prev_buffer_object(renderer.get_current_state()->get_bind_buffer_object(target)),
				d_target(target)
			{
				renderer.gl_unbind_buffer_object_and_apply(target);
			}

			~UnbindBufferObjectAndApply();

		private:
			GLRenderer &d_renderer;
			boost::optional<GLBufferObject::shared_ptr_to_const_type> d_prev_buffer_object;
			GLenum d_target;
		};

		/**
		 * Enables the specified (@a array) vertex array (in the fixed-function pipeline).
		 *
		 * @a array should be one of GL_VERTEX_ARRAY, GL_COLOR_ARRAY, or GL_NORMAL_ARRAY.
		 *
		 * NOTE: Use @a gl_enable_client_texture_state for GL_TEXTURE_COORD_ARRAY.
		 */
		void
		gl_enable_client_state(
				GLenum array,
				bool enable = true)
		{
			get_current_state()->set_enable_client_state(array, enable);
		}

		/**
		 * Enables the vertex attribute array GL_TEXTURE_COORD_ARRAY (in the fixed-function pipeline)
		 * on the specified texture unit.
		 */
		void
		gl_enable_client_texture_state(
				GLenum texture_unit,
				bool enable = true)
		{
			get_current_state()->set_enable_client_texture_state(texture_unit, enable);
		}

		/**
		 * Specify the source of vertex position data (from a buffer object).
		 */
		void
		gl_vertex_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object)
		{
			get_current_state()->set_vertex_pointer(
					size, type, stride, offset, vertex_buffer_object);
		}

		/**
		 * Specify the source of vertex position data (from client memory).
		 */
		void
		gl_vertex_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl)
		{
			get_current_state()->set_vertex_pointer(
					size, type, stride, offset, vertex_buffer_impl);
		}

		/**
		 * Specify the source of vertex color data (from a buffer object).
		 */
		void
		gl_color_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object)
		{
			get_current_state()->set_color_pointer(size, type, stride, offset, vertex_buffer_object);
		}

		/**
		 * Specify the source of vertex color data (from client memory).
		 */
		void
		gl_color_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl)
		{
			get_current_state()->set_color_pointer(size, type, stride, offset, vertex_buffer_impl);
		}

		/**
		 * Specify the source of vertex normal data (from a buffer object).
		 */
		void
		gl_normal_pointer(
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object)
		{
			get_current_state()->set_normal_pointer(type, stride, offset, vertex_buffer_object);
		}

		/**
		 * Specify the source of vertex normal data (from client memory).
		 */
		void
		gl_normal_pointer(
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl)
		{
			get_current_state()->set_normal_pointer(type, stride, offset, vertex_buffer_impl);
		}

		/**
		 * Specify the source of vertex texture coordinate data (from a buffer object).
		 */
		void
		gl_tex_coord_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object,
				GLenum texture_unit)
		{
			get_current_state()->set_tex_coord_pointer(size, type, stride, offset, vertex_buffer_object, texture_unit);
		}

		/**
		 * Specify the source of vertex texture coordinate data (from client memory).
		 */
		void
		gl_tex_coord_pointer(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl,
				GLenum texture_unit)
		{
			get_current_state()->set_tex_coord_pointer(size, type, stride, offset, vertex_buffer_impl, texture_unit);
		}

		/**
		 * Enables the specified *generic* vertex attribute array (for use in a shader program).
		 *
		 * These *generic* attribute arrays can be used in addition to the non-generic arrays or instead of.
		 * These *generic* attributes can only be accessed by shader programs (see @a GLProgramObject).
		 * The non-generic arrays can be accessed by both the fixed-function pipeline and shader programs.
		 * Although starting with OpenGL 3 the non-generic arrays are deprecated/removed from the core OpenGL profile.
		 * But graphics vendors support compatibility profiles so using them in a pre version 3 way is still ok.
		 *
		 * Note that, as dictated by OpenGL, @a attribute_index must be in the half-closed range
		 * [0, GL_MAX_VERTEX_ATTRIBS_ARB).
		 * You can get GL_MAX_VERTEX_ATTRIBS_ARB from 'context.get_capabilities().shader.gl_max_vertex_attribs'.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		void
		gl_enable_vertex_attrib_array(
				GLuint attribute_index,
				bool enable = true)
		{
			get_current_state()->set_enable_vertex_attrib_array(attribute_index, enable);
		}

		/**
		 * Specify the source of *generic* vertex attribute array (for use in a shader program "from a buffer object").
		 *
		 * See comment in @a gl_enable_vertex_attrib_array for details and restrictions.
		 */
		void
		gl_vertex_attrib_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object)
		{
			get_current_state()->set_vertex_attrib_pointer(
					attribute_index, size, type, normalized, stride, offset, vertex_buffer_object);
		}

		/**
		 * Specify the source of *generic* vertex attribute array (for use in a shader program "from client memory").
		 *
		 * See comment in @a gl_enable_vertex_attrib_array for details and restrictions.
		 */
		void
		gl_vertex_attrib_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl)
		{
			get_current_state()->set_vertex_attrib_pointer(
					attribute_index, size, type, normalized, stride, offset, vertex_buffer_impl);
		}

		/**
		 * Same as @a gl_vertex_attrib_pointer except used to specify attributes mapping to *integer* shader variables.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4' OpenGL extension.
		 */
		void
		gl_vertex_attrib_i_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object)
		{
			get_current_state()->set_vertex_attrib_i_pointer(
					attribute_index, size, type, stride, offset, vertex_buffer_object);
		}

		/**
		 * Same as @a gl_vertex_attrib_pointer except used to specify attributes mapping to *integer* shader variables.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4' OpenGL extension.
		 */
		void
		gl_vertex_attrib_i_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl)
		{
			get_current_state()->set_vertex_attrib_i_pointer(
					attribute_index, size, type, stride, offset, vertex_buffer_impl);
		}

		/**
		 * Same as @a gl_vertex_attrib_pointer except used to specify attributes mapping to *double* shader variables.
		 *
		 * NOTE: Requires 'GL_ARB_vertex_attrib_64bit' OpenGL extension.
		 */
		void
		gl_vertex_attrib_l_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferObject::shared_ptr_to_const_type vertex_buffer_object)
		{
			get_current_state()->set_vertex_attrib_l_pointer(
					attribute_index, size, type, stride, offset, vertex_buffer_object);
		}

		/**
		 * Same as @a gl_vertex_attrib_pointer except used to specify attributes mapping to *double* shader variables.
		 *
		 * NOTE: Requires 'GL_ARB_vertex_attrib_64bit' OpenGL extension.
		 */
		void
		gl_vertex_attrib_l_pointer(
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				GLBufferImpl::shared_ptr_to_const_type vertex_buffer_impl)
		{
			get_current_state()->set_vertex_attrib_l_pointer(
					attribute_index, size, type, stride, offset, vertex_buffer_impl);
		}
	};


	/**
	 * Creates a compiled draw state that specifies no bound vertex element buffer, no bound
	 * vertex attribute arrays (vertex buffers) and no enabled client vertex attribute state.
	 *
	 * This function also makes state change detection in the renderer more efficient when
	 * binding vertex arrays (implementation detail: because the individual immutable GLStateSet
	 * objects, that specify unbound/disabled state, are shared by multiple vertex arrays resulting
	 * in a simple pointer comparison versus a virtual function call with comparison logic).
	 *
	 * So this function is built into @a GLVertexArray.
	 */
	GLCompiledDrawState::non_null_ptr_type
	create_unbound_vertex_array_compiled_draw_state(
			GLRenderer &renderer);
}

#endif // GPLATES_OPENGL_GLRENDERER_H
