/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2020 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSAMPLER_H
#define GPLATES_OPENGL_GLSAMPLER_H

#include <memory> // For std::unique_ptr
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL1.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GL;
	class OpenGLFunctions;

	/**
	 * Wrapper around an OpenGL sampler object.
	 */
	class GLSampler :
			public GLObject,
			public boost::enable_shared_from_this<GLSampler>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLSampler.
		typedef boost::shared_ptr<GLSampler> shared_ptr_type;
		typedef boost::shared_ptr<const GLSampler> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLSampler.
		typedef boost::weak_ptr<GLSampler> weak_ptr_type;
		typedef boost::weak_ptr<const GLSampler> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLSampler object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl)
		{
			return shared_ptr_type(new GLSampler(gl));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLSampler>
		create_unique(
				GL &gl)
		{
			return std::unique_ptr<GLSampler>(new GLSampler(gl));
		}


		/**
		 * Returns the sampler resource handle.
		 */
		GLuint
		get_resource_handle() const;


	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL shader objects.
		 */
		class Allocator
		{
		public:
			GLuint
			allocate(
					OpenGLFunctions &opengl_functions,
					const GLCapabilities &capabilities);

			void
			deallocate(
					OpenGLFunctions &opengl_functions,
					GLuint);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<GLuint, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<GLuint, Allocator> resource_manager_type;

	private:
		resource_type::non_null_ptr_to_const_type d_resource;


		//! Constructor.
		explicit
		GLSampler(
				GL &gl);
	};
}

#endif // GPLATES_OPENGL_GLSAMPLER_H
