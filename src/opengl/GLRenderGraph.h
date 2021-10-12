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
 
#ifndef GPLATES_OPENGL_GLRENDERGRAPH_H
#define GPLATES_OPENGL_GLRENDERGRAPH_H

#include <opengl/OpenGL.h>

#include "GLRenderGraphInternalNode.h"
#include "GLRenderGraphVisitor.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Represents the OpenGL graphics state, transform state, texture sampler state
	 * and drawables as a graph.
	 */
	class GLRenderGraph :
			public GPlatesUtils::ReferenceCount<GLRenderGraph>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderGraph.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderGraph> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderGraph.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderGraph> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderGraph object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLRenderGraph());
		}


		/**
		 * The root node of the render graph.
		 *
		 * Contains no state and is only used to visit child nodes.
		 */
		const GLRenderGraphInternalNode &
		get_root_node() const
		{
			return *d_root_node;
		}


		/**
		 * The root node of the render graph.
		 *
		 * Contains no state and is only used to add child nodes to.
		 */
		GLRenderGraphInternalNode &
		get_root_node()
		{
			return *d_root_node;
		}


		/**
		 * Accept a ConstGLRenderGraphVisitor instance.
		 */
		void
		accept_visitor(
				ConstGLRenderGraphVisitor &visitor) const;


		/**
		 * Accept a GLRenderGraphVisitor instance.
		 */
		void
		accept_visitor(
				GLRenderGraphVisitor &visitor);

	private:
		GLRenderGraphInternalNode::non_null_ptr_type d_root_node;


		//! Constructor.
		GLRenderGraph();
	};
}

#endif // GPLATES_OPENGL_GLRENDERGRAPH_H
