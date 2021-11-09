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

#ifndef GPLATES_OPENGL_GLFRAMEBUFFER_H
#define GPLATES_OPENGL_GLFRAMEBUFFER_H

#include <memory> // For std::unique_ptr
#include <utility>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL1.h>

#include "GLCapabilities.h"
#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLRenderbuffer.h"
#include "GLTexture.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLContext;

	/**
	 * A wrapper around an OpenGL framebuffer object.
	 *
	 * You can use an instance of this class freely across different OpenGL contexts (eg, globe and map views).
	 * Normally a framebuffer object cannot be shared across OpenGL contexts, so this class internally
	 * creates a native framebuffer object for each context that it encounters. It also remembers the
	 * framebuffer object state (such as renderbuffer/texture attachments/bindings) and sets it on each
	 * new native framebuffer object (for each context encountered).
	 *
	 * According to Nvidia in "The OpenGL Framebuffer Object Extension" at
	 * http://http.download.nvidia.com/developer/presentations/2005/GDC/OpenGL_Day/OpenGL_FrameBuffer_Object.pdf
	 * ...
	 *
	 *   In order of increasing performance:
	 *
	 *	   Multiple FBOs
	 *		   create a separate FBO for each texture you want to render to
	 *		   switch using BindFramebuffer()
	 *		   can be 2x faster than wglMakeCurrent() in beta NVIDIA drivers
	 *	   Single FBO, multiple texture attachments
	 *		   textures should have same format and dimensions
	 *		   use FramebufferTexture() to switch between textures
	 *	   Single FBO, multiple texture attachments
	 *		   attach textures to different color attachments
	 *		   use DrawBuffer() to switch rendering to different color attachments
	 *
	 * ...although that document is perhaps a bit old now.
	 */
	class GLFramebuffer :
			public GLObject,
			public boost::enable_shared_from_this<GLFramebuffer>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLFramebuffer.
		typedef boost::shared_ptr<GLFramebuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLFramebuffer> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLFramebuffer.
		typedef boost::weak_ptr<GLFramebuffer> weak_ptr_type;
		typedef boost::weak_ptr<const GLFramebuffer> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLFramebuffer object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl)
		{
			return shared_ptr_type(new GLFramebuffer(gl));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLFramebuffer>
		create_unique(
				GL &gl)
		{
			return std::unique_ptr<GLFramebuffer>(new GLFramebuffer(gl));
		}


		/**
		 * Returns the framebuffer resource handle associated with the current OpenGL context.
		 *
		 * Since framebuffer objects cannot be shared across OpenGL contexts a separate
		 * framebuffer object resource is created for each context encountered.
		 */
		GLuint
		get_resource_handle(
				GL &gl) const;

	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL framebuffer objects.
		 */
		class Allocator
		{
		public:
			GLuint
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					GLuint);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<GLuint, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<GLuint, Allocator> resource_manager_type;


		/**
		 * Ensure the native framebuffer object associated with the current OpenGL context has
		 * up-to-date internal state.
		 *
		 * It's possible the state of this framebuffer was modified in a different context and
		 * hence a different native framebuffer object was modified (there's a separate one for
		 * each context since they cannot be shared across contexts) and now we're in a different
		 * context so the native framebuffer object of the current context must be updated to match.
		 *
		 * NOTE: This framebuffer object must currently be bound to @a target.
		 *
		 * NOTE: @a target can be GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER or GL_FRAMEBUFFER.
		 *       And while GL_FRAMEBUFFER is equivalent to both GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER
		 *       for glBindFramebuffer, GL_FRAMEBUFFER is equivalent to only GL_DRAW_FRAMEBUFFER for
		 *       glFramebufferRenderbuffer, etc. But this is OK because if GL_FRAMEBUFFER is the target
		 *       for both glBindFramebuffer and glFramebufferRenderbuffer (for example) then the renderbuffer
		 *       will be attached to the framebuffer bound to GL_DRAW_FRAMEBUFFER which will be the
		 *       framebuffer bound to GL_FRAMEBUFFER (since bound to both GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER).
		 */
		void
		synchronise_current_context(
				GL &gl,
				GLenum target);

		void
		draw_buffer(
				GL &gl,
				GLenum buf);

		void
		draw_buffers(
				GL &gl,
				const std::vector<GLenum> &bufs);

		void
		read_buffer(
				GL &gl,
				GLenum src);

		//! Equivalent to glFramebufferRenderbuffer.
		void
		framebuffer_renderbuffer(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum renderbuffertarget,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer);

		//! Equivalent to glFramebufferTexture.
		void
		framebuffer_texture(
				GL &gl,
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		//! Equivalent to glFramebufferTexture1D.
		void
		framebuffer_texture_1D(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		//! Equivalent to glFramebufferTexture2D.
		void
		framebuffer_texture_2D(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		//! Equivalent to glFramebufferTexture3D.
		void
		framebuffer_texture_3D(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLint layer);

		//! Equivalent to glFramebufferTextureLayer.
		void
		framebuffer_texture_layer(
				GL &gl,
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLint layer);

	private:

		/**
		 * Keep track of the framebuffer object state.
		 */
		struct ObjectState
		{
			struct Attachment
			{
				//! Identify the OpenGL call that was used to attach.
				enum Type
				{
					FRAMEBUFFER_RENDERBUFFER,
					FRAMEBUFFER_TEXTURE,
					FRAMEBUFFER_TEXTURE_1D,
					FRAMEBUFFER_TEXTURE_2D,
					FRAMEBUFFER_TEXTURE_3D,
					FRAMEBUFFER_TEXTURE_LAYER
				};

				// Detached state.
				Attachment() :
					renderbuffertarget(GL_NONE),
					textarget(GL_NONE),
					level(0),
					layer(0)
				{  }

				bool
				operator==(
						const Attachment &rhs) const
				{
					return type == rhs.type &&
							renderbuffertarget == rhs.renderbuffertarget &&
							renderbuffer == rhs.renderbuffer &&
							textarget == rhs.textarget &&
							texture == rhs.texture &&
							level == rhs.level &&
							layer == rhs.layer;
				}

				bool
				operator!=(
						const Attachment &rhs) const
				{
					return !(*this == rhs);
				}

				// When none it means detached, and all other attachment state should also
				// be set to detached state (see default constructor).
				boost::optional<Type> type;

				// Renderbuffer parameters.
				GLenum renderbuffertarget;
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer;

				// Texture parameters.
				GLenum textarget;
				boost::optional<GLTexture::shared_ptr_type> texture;
				GLint level;
				GLint layer;
			};

			explicit
			ObjectState(
					GLuint max_color_attachments) :
				// Default draw buffers state is GL_NONE for all buffers except first (which is GL_COLOR_ATTACHMENT0).
				// We only record the number of buffers specified in 'glDrawBuffers' (which is one for 'glDrawBuffer')...
				draw_buffers_(1, GL_COLOR_ATTACHMENT0),
				// Default read buffer state is GL_COLOR_ATTACHMENT0...
				read_buffer_(GL_COLOR_ATTACHMENT0)
			{
				color_attachments.resize(max_color_attachments);
			}

			//  Colour/depth/stencil attachments.
			std::vector<Attachment> color_attachments;
			Attachment depth_attachment;
			Attachment stencil_attachment;

			// Draw/read buffers.
			std::vector<GLenum> draw_buffers_;
			GLenum read_buffer_;
		};

		/**
		 * The framebuffer object state as currently set in each OpenGL context.
		 *
		 * Since framebuffer objects cannot be shared across OpenGL contexts, in contrast to
		 * renderbuffer and texture objects, we create a separate framebuffer object for each context.
		 */
		struct ContextObjectState
		{
			/**
			 * Constructor creates a new framebuffer object resource using the framebuffer object
			 * manager of the specified context.
			 *
			 * If the framebuffer object is destroyed then the resource will be queued for
			 * deallocation when this context is the active context and it is used for rendering.
			 */
			ContextObjectState(
					const GLContext &context_,
					GL &gl);

			/**
			 * The OpenGL context using our framebuffer object.
			 *
			 * NOTE: This should *not* be a shared pointer otherwise it'll create a cyclic shared reference.
			 */
			const GLContext *context;

			//! The framebuffer object resource created in a specific OpenGL context.
			resource_type::non_null_ptr_to_const_type resource;

			/**
			 * The current state of the native framebuffer object in this OpenGL context.
			 *
			 * Note that this might be out-of-date if the native framebuffer in another context has been
			 * updated and then we switched to this context (required this native object to be updated).
			 */
			ObjectState object_state;

			/**
			 * Determines if our context state needs updating.
			 */
			GPlatesUtils::ObserverToken object_state_observer;
		};

		/**
		 * Typedef for a sequence of context object states.
		 *
		 * A 'vector' is fine since we're not expecting many OpenGL contexts so searches should be fast.
		 */
		typedef std::vector<ContextObjectState> context_object_state_seq_type;


		/**
		 * The framebuffer object state for each context that we've encountered.
		 */
		mutable context_object_state_seq_type d_context_object_states;

		/**
		 * The framebuffer object state set by the client.
		 *
		 * Before a native framebuffer object can be used in a particular OpenGL context the state
		 * in that native object must match this state.
		 */
		ObjectState d_object_state;

		/**
		 * Subject token is invalidated when object state is updated, meaning all contexts need updating.
		 */
		GPlatesUtils::SubjectToken d_object_state_subject;


		//! Constructor.
		explicit
		GLFramebuffer(
				GL &gl);

		void
		synchronise_current_context_attachment(
				GL &gl,
				GLenum target,
				GLenum attachment,
				const ObjectState::Attachment &attachment_state,
				ObjectState::Attachment &context_attachment_state);

		ContextObjectState &
		get_object_state_for_current_context(
				GL &gl) const;

		void
		set_attachment(
				GL &gl,
				GLenum attachment,
				const ObjectState::Attachment &attachment_info);
	};
}

#endif // GPLATES_OPENGL_GLFRAMEBUFFER_H
