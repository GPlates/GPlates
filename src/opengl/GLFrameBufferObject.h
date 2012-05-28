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

#ifndef GPLATES_OPENGL_GLFRAMEBUFFEROBJECT_H
#define GPLATES_OPENGL_GLFRAMEBUFFEROBJECT_H

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLTexture.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A wrapper around a framebuffer object.
	 *
	 * Framebuffer objects is an OpenGL extension "GL_EXT_framebuffer_object" for rendering
	 * to off-screen framebuffers.
	 *
	 * Check "GLContext::get_parameters().framebuffer.gl_EXT_framebuffer_object" to see if supported.
	 *
	 * NOTE: There's also the more recent "GL_ARB_framebuffer_object" extension however it
	 * has extra features and does not have the wide support that "GL_EXT_framebuffer_object" does.
	 */
	class GLFrameBufferObject :
			public GLObject,
			public boost::enable_shared_from_this<GLFrameBufferObject>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLFrameBufferObject.
		typedef boost::shared_ptr<GLFrameBufferObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLFrameBufferObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLFrameBufferObject.
		typedef boost::weak_ptr<GLFrameBufferObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLFrameBufferObject> weak_ptr_to_const_type;


		/**
		 * Policy class to allocate and deallocate OpenGL framebuffer objects.
		 */
		class Allocator
		{
		public:
			GLint
			allocate();

			void
			deallocate(
					GLuint);
		};

		//! Typedef for a resource handle.
		typedef GLuint resource_handle_type;

		//! Typedef for a resource.
		typedef GLObjectResource<resource_handle_type, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<resource_handle_type, Allocator> resource_manager_type;



		/**
		 * Creates a shared pointer to a @a GLFrameBufferObject object.
		 *
		 * The internal framebuffer resource is allocated by @a framebuffer_resource_manager.
		 */
		static
		shared_ptr_type
		create(
				const resource_manager_type::shared_ptr_type &framebuffer_resource_manager)
		{
			return shared_ptr_type(
					new GLFrameBufferObject(
							resource_type::create(framebuffer_resource_manager)));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLFrameBufferObject>
		create_as_auto_ptr(
				const resource_manager_type::shared_ptr_type &framebuffer_resource_manager)
		{
			return std::auto_ptr<GLFrameBufferObject>(
					new GLFrameBufferObject(
							resource_type::create(framebuffer_resource_manager)));
		}


		/**
		 * Performs same function as the glFramebufferTexture1D OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a colour_attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments_EXT).
		 */
		void
		gl_attach_1D(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLenum colour_attachment);


		/**
		 * Performs same function as the glFramebufferTexture2D OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a colour_attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments_EXT).
		 */
		void
		gl_attach_2D(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLenum colour_attachment);


		/**
		 * Performs same function as the glFramebufferTexture3D OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a colour_attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments_EXT).
		 */
		void
		gl_attach_3D(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLint zoffset,
				GLenum colour_attachment);


		/**
		 * Detaches specified colour attachment point.
		 *
		 * @throws PreconditionViolationError if @a colour_attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments_EXT).
		 */
		void
		gl_detach(
				GLRenderer &renderer,
				GLenum colour_attachment);


		/**
		 * Detaches any currently attached attachment points.
		 */
		void
		gl_detach_all(
				GLRenderer &renderer);


		/**
		 * Effectively does the same as 'glCheckFramebufferStatusEXT' and returns true if the status is
		 * 'GL_FRAMEBUFFER_COMPLETE_EXT' or false is the status is 'GL_FRAMEBUFFER_UNSUPPORTED_EXT'.
		 *
		 * If the status is neither 'GL_FRAMEBUFFER_COMPLETE_EXT' nor 'GL_FRAMEBUFFER_UNSUPPORTED_EXT'
		 * then an assertion/exception is triggered as this represents a programming error.
		 *
		 * NOTE: 'glCheckFramebufferStatusEXT' appears to be an expensive call so it might pay to
		 * only call it when you know that the framebuffer configuration has changed.
		 * Configuration detection could be placed inside this class but it's unclear what outside state
		 * also affects framebuffer completeness (such as 'glDrawBuffers') - so leaving it up to the client.
		 * On two systems it took 40usec and 100usec per call (compared with say a couple of usec
		 * for a draw call) and ended up very high on the CPU profile for certain framebuffer
		 * intensive rendering paths such as:
		 *  (1) reconstructed rasters with age-grid smoothing, and
		 *  (2) filled polygons (they're actually rendered like a multi-resolution raster).
		 */
		bool
		gl_check_frame_buffer_status(
				GLRenderer &renderer) const;


		/**
		 * Returns the framebuffer handle.
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		resource_handle_type
		get_frame_buffer_resource_handle() const
		{
			return d_resource->get_resource_handle();
		}

	private:
		/**
		 * Information for a framebuffer object colour attachment point.
		 *
		 * Note that we obtain a write lock on the texture attached.
		 */
		struct ColourAttachment
		{
			GLenum attachment;
			GLenum framebuffer_texture_type; // GL_TEXTURE_1D, GL_TEXTURE_2D or GL_TEXTURE_3D_EXT
			GLenum texture_target;
			GLTexture::shared_ptr_to_const_type texture;
			GLint level;
			boost::optional<GLint> zoffset; // Only used for glFramebufferTexture3DEXT.
		};

		//! Typedef for a sequence of colour attachments.
		typedef std::vector<boost::optional<ColourAttachment> > colour_attachment_seq_type;


		resource_type::non_null_ptr_to_const_type d_resource;

		/**
		 * Up to "GLContext::get_parameters().framebuffer.gl_max_color_attachments_EXT"
		 * colour attachment points.
		 */
		colour_attachment_seq_type d_colour_attachments;


		//! Constructor.
		explicit
		GLFrameBufferObject(
				const resource_type::non_null_ptr_to_const_type &resource);
	};
}
#endif // GPLATES_OPENGL_GLFRAMEBUFFEROBJECT_H
