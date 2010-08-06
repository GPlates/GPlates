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
 
#ifndef GPLATES_OPENGL_GLVIEWPORTNODE_H
#define GPLATES_OPENGL_GLVIEWPORTNODE_H

#include <opengl/OpenGL.h>

#include "GLRenderGraphInternalNode.h"
#include "GLViewport.h"


namespace GPlatesOpenGL
{
	/**
	 * A render graph node for setting the OpenGL viewport.
	 * All child nodes of this node are rendered into the specified viewport.
	 */
	class GLViewportNode :
			public GLRenderGraphInternalNode
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLViewportNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLViewportNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLViewportNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLViewportNode> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLViewportNode object.
		 */
		static
		non_null_ptr_type
		create(
				const GLViewport &viewport)
		{
			return non_null_ptr_type(new GLViewportNode(viewport));
		}


		/**
		 * Sets the viewport parameters.
		 *
		 * Useful if you want to change the viewport parameters after constructor.
		 *
		 * NOTE: This does not call OpenGL directly.
		 */
		void
		gl_viewport(
				const GLViewport &viewport)
		{
			d_viewport = viewport;
		}


		//! Returns the viewport parameters.
		const GLViewport &
		get_viewport() const
		{
			return d_viewport;
		}


		/**
		 * Accept a ConstGLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstGLRenderGraphVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		/**
		 * Accept a GLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				GLRenderGraphVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

	private:
		GLViewport d_viewport;


		//! Constructor.
		explicit
		GLViewportNode(
				const GLViewport &viewport) :
			d_viewport(viewport)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLVIEWPORTNODE_H
