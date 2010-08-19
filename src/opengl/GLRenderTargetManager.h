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
 
#ifndef GPLATES_OPENGL_GLRENDERTARGETMANAGER_H
#define GPLATES_OPENGL_GLRENDERTARGETMANAGER_H

#include <map>
#include <utility>
#include <boost/optional.hpp>

#include "GLContext.h"
#include "GLRenderTarget.h"
#include "GLRenderTargetFactory.h"
#include "GLTexture.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Manages and creates render targets.
	 */
	class GLRenderTargetManager :
			public GPlatesUtils::ReferenceCount<GLRenderTargetManager>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderTargetManager.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderTargetManager> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderTargetManager.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderTargetManager> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderTargetManager object.
		 */
		static
		non_null_ptr_type
		create(
				const GLContext::non_null_ptr_type &context)
		{
			return non_null_ptr_type(new GLRenderTargetManager(context));
		}


		/**
		 * This is simply used when rendering to the main frame buffer.
		 *
		 * An OpenGL context must be active and GLContext must been initialised.
		 */
		GLFrameBufferRenderTarget::non_null_ptr_type
		get_frame_buffer_render_target();


		/**
		 * Gets, or creates, a render target for rendering to textures.
		 *
		 * An OpenGL context must be active and GLContext must been initialised.
		 */
		GLTextureRenderTarget::non_null_ptr_type
		get_texture_render_target(
				unsigned int texture_width,
				unsigned int texture_height);

	private:
		//! Typedef for dimensions of a render target.
		typedef std::pair<unsigned int, unsigned int> dimensions_type;

		//! Typedef for mapping render target dimensions to texture render targets.
		typedef std::map<dimensions_type, GLTextureRenderTarget::non_null_ptr_type>
				texture_render_target_map_type;


		/**
		 * The context of the main frame buffer.
		 */
		GLContext::non_null_ptr_type d_context;

		/**
		 * The sole frame buffer render target for the main frame buffer.
		 */
		boost::optional<GLFrameBufferRenderTarget::non_null_ptr_type> d_frame_buffer_render_target;

		texture_render_target_map_type d_texture_render_targets;


		//! Constructor.
		explicit
		GLRenderTargetManager(
				const GLContext::non_null_ptr_type &context) :
			d_context(context)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRENDERTARGETMANAGER_H
