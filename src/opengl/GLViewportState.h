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
 
#ifndef GPLATES_OPENGL_GLVIEWPORTSTATE_H
#define GPLATES_OPENGL_GLVIEWPORTSTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"
#include "GLViewport.h"


namespace GPlatesOpenGL
{
	/**
	 * Used to set and restore the OpenGL viewport state.
	 */
	class GLViewportState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLViewportState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLViewportState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLViewportState object.
		 */
		static
		non_null_ptr_type
		create(
				const boost::optional<GLViewport> &old_viewport,
				const GLViewport &new_viewport)
		{
			return non_null_ptr_type(new GLViewportState(old_viewport, new_viewport));
		}


		virtual
		void
		enter_state_set() const
		{
			// Set the new viewport.
			glViewport(
					d_new_viewport.x(),
					d_new_viewport.y(),
					d_new_viewport.width(),
					d_new_viewport.height());
		}


		virtual
		void
		leave_state_set() const
		{
			// Set the old viewport back again.
			if (d_old_viewport)
			{
				glViewport(
						d_old_viewport->x(),
						d_old_viewport->y(),
						d_old_viewport->width(),
						d_old_viewport->height());
			}
		}

	private:
		boost::optional<GLViewport> d_old_viewport;
		GLViewport d_new_viewport;


		//! Constructor
		GLViewportState(
				const boost::optional<GLViewport> &old_viewport,
				const GLViewport &new_viewport) :
			d_old_viewport(old_viewport),
			d_new_viewport(new_viewport)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLVIEWPORTSTATE_H
