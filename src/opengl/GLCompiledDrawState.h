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

#ifndef GPLATES_OPENGL_GLCOMPILEDDRAWSTATE_H
#define GPLATES_OPENGL_GLCOMPILEDDRAWSTATE_H

#include <boost/shared_ptr.hpp>

#include "global/PointerTraits.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLState;

	namespace GLRendererImpl
	{
		class StateBlock;
		struct RenderQueue;
	}

	/**
	 * A compiled draw state contains a set of state changes and optionally a sequence of draw calls.
	 *
	 * It is similar in design to the OpenGL display list object.
	 *
	 * The implementation of compiled draw states in @a GLRenderer is such that they can be used
	 * across different OpenGL contexts - this is primarily due to the way @a GLVertexArrayObject is
	 * implemented (normally native OpenGL vertex array objects cannot be shared across contexts).
	 */
	class GLCompiledDrawState :
			public GPlatesUtils::ReferenceCount<GLCompiledDrawState>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLCompiledDrawState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLCompiledDrawState> non_null_ptr_to_const_type;

		~GLCompiledDrawState();


		/**
		 * Returns the 'const' state compiled into this draw state.
		 *
		 * Note that the state will change if more state is subsequently compiled in.
		 *
		 * NOTE: This should only be used to help implement the render framework.
		 * General clients shouldn't need to use this.
		 */
		boost::shared_ptr<const GLState>
		get_state() const
		{
			return d_state_change;
		}

	private:
		//! The net state change across the scope of the compiled draw state.
		boost::shared_ptr<GLState> d_state_change;

		//! Optional sequence of draw calls - depends whether any were compiled into draw state.
		GPlatesGlobal::PointerTraits<GLRendererImpl::RenderQueue>::non_null_ptr_type d_render_queue;


		//! Constructor.
		GLCompiledDrawState(
				const boost::shared_ptr<GLState> &state_change,
				const GPlatesGlobal::PointerTraits<GLRendererImpl::RenderQueue>::non_null_ptr_type &render_queue);


		/**
		 * So can create and access contents.
		 *
		 * This is really just an implementation class of @a GLRenderer.
		 * It doesn't have an interface and just gets passed around as an opaque object.
		 */
		friend class GLRenderer;
		friend class GLRendererImpl::StateBlock;
	};
}

#endif // GPLATES_OPENGL_GLCOMPILEDDRAWSTATE_H
