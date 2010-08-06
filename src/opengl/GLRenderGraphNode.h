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
 
#ifndef GPLATES_OPENGL_GLRENDERGRAPHNODE_H
#define GPLATES_OPENGL_GLRENDERGRAPHNODE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLRenderGraphVisitor.h"
#include "GLStateSet.h"
#include "GLTransform.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Base class representing a node in the OpenGL render graph.
	 *
	 * It optionally contains a graphics state set and a transform.
	 */
	class GLRenderGraphNode :
			public GPlatesUtils::ReferenceCount<GLRenderGraphNode>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderGraphNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderGraphNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderGraphNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderGraphNode> non_null_ptr_to_const_type;


		virtual
		~GLRenderGraphNode()
		{  }


		/**
		 * Sets an optional state set for this node (a state set is typically a part
		 * of the OpenGL state required to render a drawable - the rest of the OpenGL
		 * state being distributed across the parent render graph nodes).
		 *
		 * The render graph will sort nodes containing drawables according to state set
		 * under the following conditions:
		 * - the nodes must point to the same state set, and
		 * - the nodes must have the same chain of ancestor state sets going back to the
		 *   root render graph node, and
		 * - the nodes must be in the same render layer.
		 * If all the above conditions hold for a group of nodes then their drawables
		 * will be drawn in sequence with the same OpenGL state thus minimising
		 * potentially expensive OpenGL state changes.
		 * The change in OpenGL state between draw calls is what's important.
		 * If you change the state and then reset it before a subsequent draw call it's not
		 * so important because the OpenGL driver should batch up changes between draw calls
		 * and determine the net change itself anyway. It's really the fact that there is
		 * a net change itself that's important as this has the potential to stall
		 * (or partially stall) the graphics pipeline. You can limit that by batching drawables
		 * that use the same OpenGL state (effectively removing a net change between some
		 * drawables). You can do that by giving them the same state set pointer and positioning
		 * them in the render graph such that they have the same ancestor state sets.
		 */
		void
		set_state_set(
				const GLStateSet::non_null_ptr_to_const_type &state_set)
		{
			d_state_set = state_set;
		}

		//! Returns the optional state set.
		const boost::optional<GLStateSet::non_null_ptr_to_const_type> &
		get_state_set() const
		{
			return d_state_set;
		}


		/**
		 * Sets the optional transform to be applied for this node.
		 *
		 * Any transforms set on child nodes are relative to this transform.
		 * That is the transforms are hierarchical.
		 */
		void
		set_transform(
				const GLTransform::non_null_ptr_to_const_type &transform)
		{
			d_transform = transform;
		}

		//! Returns the optional transform.
		const boost::optional<GLTransform::non_null_ptr_to_const_type> &
		get_transform() const
		{
			return d_transform;
		}


		/**
		 * Accept a ConstGLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstGLRenderGraphVisitor &visitor) const = 0;


		/**
		 * Accept a GLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				GLRenderGraphVisitor &visitor) = 0;

	private:
		boost::optional<GLStateSet::non_null_ptr_to_const_type> d_state_set;
		boost::optional<GLTransform::non_null_ptr_to_const_type> d_transform;
	};
}

#endif // GPLATES_OPENGL_GLRENDERGRAPHNODE_H
