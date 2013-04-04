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
#include <utility>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLRenderBufferObject.h"
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
	 * Check "context.get_capabilities().framebuffer.gl_EXT_framebuffer_object" to see if supported.
	 *
	 * NOTE: There's also the more recent "GL_ARB_framebuffer_object" extension however it has extra features
	 * (or less restrictions) but does not have the wide support that "GL_EXT_framebuffer_object" does.
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
		 * Classifies a frame buffer object to assist with frame buffer switching efficiency.
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
		 *		   use glDrawBuffer() to switch rendering to different color attachments
		 *
		 * ...so we can optimize for the second case above having multiple render targets with
		 * the same texture format and dimensions share a single frame buffer object.
		 * These parameters are specified in this class (@a Classification).
		 *
		 */
		class Classification
		{
		public:

			//! Classification as a boost tuple (so can be used as a key in maps).
			typedef boost::tuple<GLuint, GLuint, GLint, GLint, GLint> tuple_type;


			//! Default classification represents empty (un-attached) frame buffer.
			Classification();

			//! Set dimensions of frame buffer attachable textures/render-buffers.
			void
			set_dimensions(
					GLuint width,
					GLuint height);

			/**
			 * Set texture internal format if one or more textures are to be attached.
			 *
			 * Note that GL_EXT_framebuffer_object requires all textures attached to colour attachments
			 * to have the same internal format so there's no need to specify the attachment point(s).
			 */
			void
			set_texture_internal_format(
					GLint texture_internal_format);

			//! Set depth buffer internal format if a depth buffer is to be attached.
			void
			set_depth_buffer_internal_format(
					GLint depth_buffer_internal_format);

			//! Set stencil buffer internal format if a stencil buffer is to be attached.
			void
			set_stencil_buffer_internal_format(
					GLint stencil_buffer_internal_format);

			//! Return this classification object as a tuple.
			tuple_type
			get_tuple() const;

			//
			// Queries parameters...
			//

			GLuint
			get_width() const;

			GLuint
			get_height() const;

			GLint
			get_texture_internal_format() const;

			GLint
			get_depth_buffer_internal_format() const;

			GLint
			get_stencil_buffer_internal_format() const;

		private:

			GLuint d_width;
			GLuint d_height;
			GLint d_texture_internal_format;
			GLint d_depth_buffer_internal_format;
			GLint d_stencil_buffer_internal_format;
		};


		/**
		 * The default buffer (GL_COLOR_ATTACHMENT0_EXT) for the glDrawBuffer(s)/glReadBuffer state
		 * contained in a framebuffer object.
		 */
		static const GLenum DEFAULT_DRAW_READ_BUFFER;


		/**
		 * Performs same function as the glGenerateMipmap OpenGL function.
		 *
		 * In addition it temporarily binds the specified texture to the specified target
		 * (on, arbitrarily, texture unit zero) such that mipmap generation targets the specified texture.
		 * The texture binding is reverted, before returning, to its original state.
		 * It also temporarily unbinds any active framebuffer object just to be sure there's no conflict
		 * if the texture is currently attached to the active framebuffer object.
		 *
		 * NOTE: This function is defined here instead of in class @a GLTexture since it's part
		 * of the GL_EXT_framebuffer_object extension and ideally (due to potential driver issues)
		 * should only really be used for a texture that has been rendered to using a framebuffer object.
		 */
		static
		void
		gl_generate_mipmap(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture);


		/**
		 * Creates a shared pointer to a @a GLFrameBufferObject object.
		 *
		 * The internal framebuffer resource is allocated by @a framebuffer_resource_manager.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(new GLFrameBufferObject(renderer));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLFrameBufferObject>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLFrameBufferObject>(new GLFrameBufferObject(renderer));
		}


		/**
		 * Performs same function as the glFramebufferTexture1D OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_attach_texture_1D(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLenum attachment);


		/**
		 * Performs same function as the glFramebufferTexture2D OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_attach_texture_2D(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLenum attachment);


		/**
		 * Performs same function as the glFramebufferTexture3D OpenGL function.
		 *
		 * NOTE: Use @a gl_attach_texture_array_layer for 2D array textures.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_attach_texture_3D(
				GLRenderer &renderer,
				GLenum texture_target,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLint zoffset,
				GLenum attachment);


		/**
		 * Performs same function as the glFramebufferTextureLayer OpenGL function - can be used for
		 * 1D and 2D array textures (and regular 3D textures - where it does same as @a gl_attach_texture_3D).
		 *
		 * NOTE: This also requires the GL_EXT_texture_array extension to be supported.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_attach_texture_array_layer(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLint layer,
				GLenum attachment);


		/**
		 * Performs same function as the glFramebufferTexture OpenGL function - can be used for 1D and 2D array textures.
		 *
		 * NOTE: This also requires the GL_EXT_geometry_shader4 extension to be supported.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_attach_texture_array(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &texture,
				GLint level,
				GLenum attachment);


		/**
		 * Performs same function as the glFramebufferRenderbuffer OpenGL function.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_attach_render_buffer(
				GLRenderer &renderer,
				const GLRenderBufferObject::shared_ptr_to_const_type &render_buffer,
				GLenum attachment);


		/**
		 * Detaches specified attachment point.
		 *
		 * @throws PreconditionViolationError if @a attachment is not in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...or is not GL_DEPTH_ATTACHMENT_EXT or GL_STENCIL_ATTACHMENT_EXT.
		 */
		void
		gl_detach(
				GLRenderer &renderer,
				GLenum attachment);


		/**
		 * Detaches any currently attached attachment points.
		 */
		void
		gl_detach_all(
				GLRenderer &renderer);


		/**
		 * Performs same function as the glDrawBuffer/glDrawBuffers OpenGL function.
		 *
		 * Note that since the glDrawBuffer(s) state is stored inside the framebuffer object the
		 * framebuffer object is temporarily bound inside this method (and reverted on return).
		 *
		 * The default state is buffer 0 is GL_COLOR_ATTACHMENT0_EXT with buffers
		 * [1, context.get_capabilities().framebuffer.gl_max_draw_buffers) being GL_NONE.
		 *
		 * There is also a glDrawBuffer(s) state that applies to the default window-system framebuffer
		 * but that is not dealt with here - that would go in the GLRenderer interface - currently
		 * it's not there since the default of GL_BACK (or GL_FRONT if has no back buffer) is fine
		 * for now and, anyway, rendering to multiple render targets should use non-default
		 * application-created framebuffer objects (ie, this interface).
		 *
		 * @throws PreconditionViolationError if 'bufs.size()' is not in the range:
		 *   [1, context.get_capabilities().framebuffer.gl_max_draw_buffers]
		 *
		 * Note that if any values in @a bufs are not either GL_NONE or in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...then OpenGL will generate a GL_INVALID_OPERATION error (see 'GLUtils::assert_no_gl_errors').
		 *
		 * If 'bufs.size()' is greater than one then the GL_ARB_draw_buffers extension must be supported
		 * (an exception is thrown if it is not supported in this case).
		 */
		void
		gl_draw_buffers(
				GLRenderer &renderer,
				const std::vector<GLenum> &bufs = std::vector<GLenum>(1, DEFAULT_DRAW_READ_BUFFER));


		/**
		 * Performs same function as the glReadBuffer OpenGL function.
		 *
		 * Note that since the glReadBuffer state is stored inside the framebuffer object the
		 * framebuffer object is temporarily bound inside this method (and reverted on return).
		 *
		 * The default state is GL_COLOR_ATTACHMENT0_EXT.
		 *
		 * Note that if @a mode is not either GL_NONE or in the half-open range:
		 *   [GL_COLOR_ATTACHMENT0_EXT,
		 *    GL_COLOR_ATTACHMENT0_EXT + context.get_capabilities().framebuffer.gl_max_color_attachments)
		 * ...then OpenGL will generate a GL_INVALID_OPERATION error (see 'GLUtils::assert_no_gl_errors').
		 *
		 * There is also a glReadBuffer state that applies to the default window-system framebuffer
		 * but that is not dealt with here - see @a gl_draw_buffers for more details.
		 */
		void
		gl_read_buffer(
				GLRenderer &renderer,
				GLenum mode = DEFAULT_DRAW_READ_BUFFER);


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
		 * Returns the framebuffer dimensions, or boost::none if no attachments have been specified or
		 * if an attachment has not had its storage specified.
		 *
		 * NOTE: Since we're using GL_EXT_framebuffer_object (and not GL_ARB_framebuffer_object) the
		 * dimensions of all attachment points must be the same (hence no need for client to specify
		 * a specific attachment point for this method).
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		boost::optional< std::pair<GLuint/*width*/, GLuint/*height*/> >
		get_frame_buffer_dimensions() const;


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

		enum AttachmentType
		{
			ATTACHMENT_TEXTURE_1D,
			ATTACHMENT_TEXTURE_2D,
			ATTACHMENT_TEXTURE_3D,
			ATTACHMENT_TEXTURE_ARRAY_LAYER,
			ATTACHMENT_TEXTURE_ARRAY,
			ATTACHMENT_RENDER_BUFFER
		};

		/**
		 * Information for a framebuffer object attachment point.
		 */
		struct AttachmentPoint
		{
			//! Constructor for texture 1D, 2D and 3D attachments.
			AttachmentPoint(
					GLenum attachment_,
					AttachmentType attachment_type_,
					GLenum attachment_target_,
					const GLTexture::shared_ptr_to_const_type &texture_,
					GLint texture_level_,
					boost::optional<GLint> texture_zoffset_ = boost::none);

			//! Constructor for texture array layer attachments.
			AttachmentPoint(
					GLenum attachment_,
					const GLTexture::shared_ptr_to_const_type &texture_,
					GLint texture_level_,
					GLint texture_layer_);

			//! Constructor for texture array attachments.
			AttachmentPoint(
					GLenum attachment_,
					const GLTexture::shared_ptr_to_const_type &texture_,
					GLint texture_level_);

			//! Constructor for render buffer attachments.
			AttachmentPoint(
					GLenum attachment_,
					const GLRenderBufferObject::shared_ptr_to_const_type &render_buffer_);

			GLenum attachment;
			AttachmentType attachment_type;

			// Only used for ATTACHMENT_TEXTURE_1D, ATTACHMENT_TEXTURE_2D or ATTACHMENT_TEXTURE_3D...
			boost::optional<GLenum> attachment_target;

			// Not used for ATTACHMENT_RENDER_BUFFER...
			boost::optional<GLTexture::shared_ptr_to_const_type> texture; // Keep texture alive while attached.

			// Not used for ATTACHMENT_RENDER_BUFFER...
			boost::optional<GLint> texture_level;

			// Only used for ATTACHMENT_TEXTURE_3D and ATTACHMENT_TEXTURE_ARRAY_LAYER...
			boost::optional<GLint> texture_zoffset;

			// Only used for ATTACHMENT_RENDER_BUFFER...
			boost::optional<GLRenderBufferObject::shared_ptr_to_const_type> render_buffer;
		};

		//! Typedef for a sequence of attachments.
		typedef std::vector<boost::optional<AttachmentPoint> > attachment_point_seq_type;


		resource_type::non_null_ptr_to_const_type d_resource;

		/**
		 * All attachment points.
		 *
		 * Unused slots will be boost::none.
		 */
		attachment_point_seq_type d_attachment_points;


		//! Constructor.
		explicit
		GLFrameBufferObject(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLFRAMEBUFFEROBJECT_H
