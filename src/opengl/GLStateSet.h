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
 
#ifndef GPLATES_OPENGL_GLSTATESET_H
#define GPLATES_OPENGL_GLSTATESET_H

#include <boost/optional.hpp>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLStateSet :
			public GPlatesUtils::ReferenceCount<GLStateSet>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLStateSet.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateSet> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLStateSet.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateSet> non_null_ptr_to_const_type;

		/**
		 * Typedef for a render group.
		 *
		 * The draw order of render groups are sorted using this type (integer).
		 *
		 * A render graph subtree belongs to the render group defined by the state set
		 * attached to the root node of the subtree - except any child nodes that override
		 * the render group. So all nodes with the same render group integer will be drawn
		 * together.
		 */
		typedef int render_group_type;

		/**
		 * The default render group is the global group.
		 *
		 * This is '-1' so that 0, 1, 2, ... can be used by clients
		 * to define their own render groups.
		 */
		static const render_group_type GLOBAL = -1;


		/**
		 * Creates a base @a GLStateSet object that does not set any state.
		 *
		 * @param render_group the render group assigned to this state set (if any) -
		 *        if no render group is assigned then the parent's render group is inherited -
		 *        where parent refers to render graph nodes (which state sets can be attached to).
		 * @param render_sub_group whether to create a render sub group within the
		 *        parent render group - multiple render graph nodes can reference
		 *        the same state set instance which means their draw order can be changed
		 *        (within the parent render group) to minimise state changes -
		 *        setting this parameter to true prevents this from happening - this
		 *        is useful if you want to maintain a draw order without assigning
		 *        render group integers (ie, all inherit the GLOBAL render group and enable
		 *        render sub groups).
		 *
		 * Normally you would create a derived type but you might want to create
		 * a @a GLStateSet if you only want to set the render group (and not set any state).
		 */
		static
		non_null_ptr_type
		create(
				const boost::optional<render_group_type> &render_group = boost::none,
				bool enable_render_sub_group = false)
		{
			return non_null_ptr_type(new GLStateSet(render_group, enable_render_sub_group));
		}


		virtual
		~GLStateSet()
		{  }


		/**
		 * Override this method to set some OpenGL state by making direct calls to
		 * the OpenGL library.
		 *
		 * This method is typically used to setup some OpenGL state that will be used
		 * for a subsequent draw call.
		 * And typically only state that differs from the default OpenGL state needs
		 * to be set (the default state is the state that OpenGL is in when a new OpenGL
		 * context is first created).
		 *
		 * Since a state set is attached to a render graph node (and the render graph
		 * is a tree structure) you can incrementally build up OpenGL state over several
		 * state sets rather than setting it all in one state set. In this case the
		 * state sets should be designed to work together orthogonally.
		 */
		virtual
		void
		enter_state_set() const
		{  }


		/**
		 * Override this method to reset some OpenGL state by making calls to
		 * the OpenGL library.
		 *
		 * This method is typically used to restore the states changed in @a enter_state_set
		 * to the OpenGL default values for those states.
		 * This doesn't have to be done - you might prefer to leave that up to a parent
		 * state set (a state set attached to a render graph node that is the parent of
		 * the render graph node that 'this' state set is attached to).
		 * The class @a GLEnterOrLeaveStateSet can be used to prevent the default state
		 * of a state set being set by preventing its @a leave_state_set method from being called.
		 *
		 * In any case when we leave the state set of the root render graph node it is
		 * preferable to have OpenGL be in its default state (that is, the state that OpenGL
		 * is in when a new OpenGL context is first created). This means other state sets
		 * only need to set state that is different that the default state.
		 */
		virtual
		void
		leave_state_set() const
		{  }


		/**
		 * Assigns a render group to this state set.
		 */
		void
		set_render_group(
				render_group_type render_group)
		{
			d_render_group = render_group;
		}

		/**
		 * Returns the render group assigned to this state set, otherwise
		 * returns false if none assigned.
		 */
		const boost::optional<render_group_type> &
		get_render_group() const
		{
			return d_render_group;
		}


		/**
		 * Determines whether multiple render graph nodes (referencing 'this'
		 * state set) can have their draw order changed to minimise state changes
		 * during rendering.
		 *
		 * Set to true to enable a render sub group (ie, disable draw order changes).
		 */
		void
		set_enable_render_sub_group(
				bool enable_render_sub_group = true)
		{
			d_enable_render_sub_group = enable_render_sub_group;
		}

		/**
		 * Returns whether a render sub group is enabled for this state set.
		 */
		bool
		get_enable_render_sub_group() const
		{
			return d_enable_render_sub_group;
		}

	protected:
		boost::optional<render_group_type> d_render_group;
		bool d_enable_render_sub_group;


		//! Constructor.
		explicit
		GLStateSet(
				const boost::optional<render_group_type> &render_group = boost::none,
				bool enable_render_sub_group = false) :
			d_render_group(render_group),
			d_enable_render_sub_group(enable_render_sub_group)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLSTATESET_H
