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

#include <boost/noncopyable.hpp>
	

namespace GPlatesOpenGL
{
	class GLState;

	/**
	 * Base class for setting any OpenGL *global* state - together all the individual state sets
	 * form the complete OpenGL global state.
	 *
	 * Note that all OpenGL global state should ideally be set by the derived classes of @a GLStateSet.
	 *
	 * The state stored in OpenGL objects (such as texture objects, vertex buffer objects, etc)
	 * is not handled here. Those states are manipulated by setting state directly on those objects
	 * (although this does need the object to be bound to the OpenGL context which means setting
	 * a global state - the binding state for the type of object - since only one object of any
	 * type, or target, can be bound to an OpenGL context at any particular time).
	 */
	class GLStateSet :
			private boost::noncopyable
	{
	public:
		virtual
		~GLStateSet()
		{  }


		/**
		 * Applies the internal state (of derived class instance) directly to OpenGL if a state change
		 * is detected when compared to @a last_applied_state_set.
		 *
		 * If it is difficult or costly (or otherwise doesn't serve any gain) to detect if the
		 * state set has changed then simply apply the internal state without comparison.
		 * In that case 'this' state set will get applied - so if it hasn't changed then the worst
		 * is a redundant state is set to OpenGL (which logically does nothing).
		 *
		 * @a last_applied_state_set can be downcast to the derived class type of 'this'.
		 * The caller guarantees they are of the same type.
		 */
		virtual
		void
		apply_state(
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const = 0;


		/**
		 * Applies the internal state (of derived class instance) directly to OpenGL *from* the
		 * default OpenGL state.
		 *
		 * The default state is what OpenGL considers to be the default state for the particular
		 * OpenGL state represented by the derived class.
		 * For example, for a state set representing whether blending is enabled (GL_BLEND) the
		 * default state is disabled.
		 *
		 * The caller guarantees that, for this particular @a GLStateSet, that OpenGL is currently
		 * in the default state before this method is called.
		 *
		 * @a last_applied_state is the container for all state sets (including 'this' actually).
		 * It represents the current OpenGL state (as applied to the OpenGL context).
		 * It is only provided in case 'this' state set needs to modify state other than itself
		 * in order to achieve its goal.
		 * For example changing the active texture unit is required in order to bind a texture
		 * to a specific texture unit (and the active texture unit is managed by another @a GLStateSet).
		 *
		 * Note that this method is 'const' since 'this' object should not change because if it
		 * gets called again later it should apply the same state to OpenGL.
		 */
		virtual
		void
		apply_from_default_state(
				GLState &last_applied_state) const = 0;


		/**
		 * Applies the *default* state directly to OpenGL *from* the internal state (of this
		 * derived class instance).
		 *
		 * The default state is what OpenGL considers to be the default state for the particular
		 * OpenGL state represented by the derived class.
		 * For example, for a state set representing whether blending is enabled (GL_BLEND) the
		 * default state is disabled.
		 *
		 * The caller guarantees that, for this particular @a GLStateSet, that OpenGL is currently
		 * in the state represented by 'this' object before this method is called.
		 *
		 * Note that this method is 'const' since 'this' object should not change because if it
		 * gets called again later it should apply the same state to OpenGL.
		 *
		 * NOTE: If the internal state of 'this' instance is already in the default state then
		 * nothing needs to be applied.
		 */
		virtual
		void
		apply_to_default_state(
				GLState &last_applied_state) const = 0;
	};
}

#endif // GPLATES_OPENGL_GLSTATESET_H
