/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSCREENRENDERTARGET_H
#define GPLATES_OPENGL_GLSCREENRENDERTARGET_H

#include "GLFrameBufferObject.h"
#include "GLRenderBufferObject.h"
#include "GLTexture.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * Used to render to a screen-size RGBA8 texture (with associated hardware depth buffer).
	 *
	 * Your rendering is done between @a begin_render and @a end_render.
	 */
	class GLScreenRenderTarget
	{
	public:

		//! Returns true if supported by the runtime system.
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Constructor creates the texture and depth buffer resources but doesn't allocate them yet.
		 */
		explicit
		GLScreenRenderTarget(
				GLRenderer &renderer);


		/**
		 * Ensures internal RGBA texture and depth render buffer have a storage allocation of the
		 * specified dimensions and binds the internal framebuffer for rendering to them.
		 */
		void
		begin_render(
				GLRenderer &renderer,
				unsigned int render_target_width,
				unsigned int render_target_height);


		/**
		 * Returns viewport-size RGBA render texture and binds to the main framebuffer.
		 */
		GLTexture::shared_ptr_to_const_type
		end_render(
				GLRenderer &renderer);

	private:

		GLFrameBufferObject::shared_ptr_type d_framebuffer;
		GLTexture::shared_ptr_type d_texture;
		GLRenderBufferObject::shared_ptr_type d_depth_buffer;

		/**
		 * Is false if we've not yet allocated storage for the texture and depth buffer.
		 */
		bool d_allocated_storage;
	};
}

#endif // GPLATES_OPENGL_GLSCREENRENDERTARGET_H
