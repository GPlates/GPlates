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

#ifndef GPLATES_OPENGL_GLOBJECT_H
#define GPLATES_OPENGL_GLOBJECT_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	/**
	 * Base class for any OpenGL object such as texture object, texture buffer object,
	 * vertex buffer object, pixel buffer object, framebuffer object, etc.
	 */
	class GLObject :
			private boost::noncopyable
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLObject.
		typedef boost::shared_ptr<GLObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLObject.
		typedef boost::weak_ptr<GLObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLObject> weak_ptr_to_const_type;


		virtual
		~GLObject()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLOBJECT_H
