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
 
#ifndef GPLATES_OPENGL_GLRENDERGRAPHDRAWABLENODE_H
#define GPLATES_OPENGL_GLRENDERGRAPHDRAWABLENODE_H

#include "GLDrawable.h"
#include "GLRenderGraphNode.h"


namespace GPlatesOpenGL
{
	class GLRenderGraphDrawableNode :
			public GLRenderGraphNode
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderGraphDrawableNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderGraphDrawableNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderGraphDrawableNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderGraphDrawableNode> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderGraphDrawableNode object.
		 */
		static
		non_null_ptr_type
		create(
				const GLDrawable::non_null_ptr_to_const_type &drawable)
		{
			return non_null_ptr_type(new GLRenderGraphDrawableNode(drawable));
		}


		void
		set_drawable(
				const GLDrawable::non_null_ptr_to_const_type &drawable)
		{
			d_drawable = drawable;
		}

		GLDrawable::non_null_ptr_to_const_type
		get_drawable() const
		{
			return d_drawable;
		}


		/**
		 * Accept a ConstGLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstGLRenderGraphVisitor &visitor) const;


		/**
		 * Accept a GLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				GLRenderGraphVisitor &visitor);

	private:
		GLDrawable::non_null_ptr_to_const_type d_drawable;


		GLRenderGraphDrawableNode(
				const GLDrawable::non_null_ptr_to_const_type &drawable) :
			d_drawable(drawable)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRENDERGRAPHDRAWABLENODE_H
