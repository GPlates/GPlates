/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_GUI_OPAQUESPHERE_H_
#define _GPLATES_GUI_OPAQUESPHERE_H_

#include <boost/noncopyable.hpp>
#include <opengl/OpenGL.h>

#include "Colour.h"

#include "opengl/GLRenderGraphInternalNode.h"
#include "opengl/GLUQuadric.h"


namespace GPlatesGui
{
	class OpaqueSphere :
		public boost::noncopyable
	{
		public:
			explicit
			OpaqueSphere(
					const Colour &colour);

			~OpaqueSphere()
			{  }


			/**
			 * Creates child render graph node and attaches it to @a render_graph_parent_node.
			 */
			void paint(
					const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &
							render_graph_parent_node);

		private:
			GPlatesOpenGL::GLUQuadric::non_null_ptr_type d_quad;
			Colour d_colour;
	};
}

#endif /* _GPLATES_GUI_OPAQUESPHERE_H_ */
