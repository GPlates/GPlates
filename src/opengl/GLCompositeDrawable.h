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
 
#ifndef GPLATES_OPENGL_GLCOMPOSITEDRAWABLE_H
#define GPLATES_OPENGL_GLCOMPOSITEDRAWABLE_H

#include <vector>
#include <opengl/OpenGL.h>

#include "GLDrawable.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A convenience wrapper around one or more child @a GLDrawable objects.
	 */
	class GLCompositeDrawable :
			public GLDrawable
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLCompositeDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLCompositeDrawable> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCompositeDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLCompositeDrawable> non_null_ptr_to_const_type;

		/**
		 * Creates a @a GLCompositeDrawable object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLCompositeDrawable());
		}


		/**
		 * Adds a @a GLDrawable.
		 */
		void
		add_drawable(
				const GLDrawable::non_null_ptr_to_const_type &drawable)
		{
			d_drawables.push_back(drawable);
		}


		virtual
		void
		bind() const
		{
			// All binding is done in @a draw because there's no opportunity to
			// bind once and draw multiple times with a composite drawable because
			// you cannot bind more than one drawable at a time (as soon as you bind
			// one drawable you effectively unbind the previously bound drawable).
		}


		virtual
		void
		draw() const;

	private:
		//! Typedef for a sequence of @a GLDrawable objects.
		typedef std::vector<GLDrawable::non_null_ptr_to_const_type> drawable_seq_type;

		/**
		 * The drawables to bind and draw.
		 */
		drawable_seq_type d_drawables;


		//! Constructor.
		GLCompositeDrawable()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLCOMPOSITEDRAWABLE_H
