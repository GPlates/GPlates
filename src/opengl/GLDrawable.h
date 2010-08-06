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
 
#ifndef GPLATES_OPENGL_GLDRAWABLE_H
#define GPLATES_OPENGL_GLDRAWABLE_H

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Interface for anything that modifies the OpenGL frame buffers.
	 *
	 * This is usually some kind of geometry such as a mesh or a sets of quads
	 * used to render font for example represented as vertices.
	 *
	 * It doesn't typically include any OpenGL state such as alpha-blending or
	 * texture state for example - although some attributes that determine visual
	 * appearance can exist in the vertex data itself such as colour and texture coordinates.
	 */
	class GLDrawable :
			public GPlatesUtils::ReferenceCount<GLDrawable>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLDrawable> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLDrawable> non_null_ptr_to_const_type;


		virtual
		~GLDrawable()
		{  }


		/**
		 * Makes this drawable the active drawable for subsequent draw calls.
		 */
		virtual
		void
		bind() const = 0;


		/**
		 * Draws this drawable - it should be the currently bound drawable (see @a bind).
		 *
		 * The reason for separate @a bind and @a draw methods is to allow a drawable
		 * to be bound once and draw multiple times.
		 * A situation where you might @a bind once and @a draw multiple times
		 * is a multi-pass effect where the same drawable is drawn with two or more
		 * different OpenGL states (such as different textures) to achieve some effect.
		 */
		virtual
		void
		draw() const = 0;


		/**
		 * Convenience method to @a bind and then @a draw.
		 *
		 * Useful if only drawing 'this' drawable once.
		 */
		void
		bind_and_draw() const
		{
			bind();
			draw();
		}
	};
}

#endif // GPLATES_OPENGL_GLDRAWABLE_H
