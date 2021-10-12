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
 
#ifndef GPLATES_OPENGL_GLANTIALIASPRIMITIVESTATE_H
#define GPLATES_OPENGL_GLANTIALIASPRIMITIVESTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * State affecting the display of points.
	 */
	class GLPointState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLPointState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLPointState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLPointState object with no state.
		 *
		 * Call methods of this class initialise the state.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLPointState());
		}

		GLPointState &
		gl_enable_point_smooth(
				GLboolean enable)
		{
			d_point_smooth = enable;
			return *this;
		}

		GLPointState &
		gl_hint_point_smooth(
				GLenum mode = GL_DONT_CARE)
		{
			d_point_smooth_hint = mode;
			return *this;
		}

		GLPointState &
		gl_point_size(
				GLfloat size = 1.0f)
		{
			d_point_size = size;
			return *this;
		}


		virtual
		void
		enter_state_set() const;

		virtual
		void
		leave_state_set() const;

	private:
		boost::optional<GLboolean> d_point_smooth;
		boost::optional<GLenum> d_point_smooth_hint;
		boost::optional<GLfloat> d_point_size;

		//! Constructor.
		GLPointState()
		{  }
	};


	/**
	 * State affecting the display of lines.
	 */
	class GLLineState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLLineState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLLineState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLLineState object with no state.
		 *
		 * Call methods of this class initialise the state.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLLineState());
		}

		GLLineState &
		gl_enable_line_smooth(
				GLboolean enable)
		{
			d_line_smooth = enable;
			return *this;
		}

		GLLineState &
		gl_hint_line_smooth(
				GLenum mode = GL_DONT_CARE)
		{
			d_line_smooth_hint = mode;
			return *this;
		}

		GLLineState &
		gl_line_width(
				GLfloat width = 1.0f)
		{
			d_line_width = width;
			return *this;
		}


		virtual
		void
		enter_state_set() const;

		virtual
		void
		leave_state_set() const;

	private:
		boost::optional<GLboolean> d_line_smooth;
		boost::optional<GLenum> d_line_smooth_hint;
		boost::optional<GLfloat> d_line_width;

		//! Constructor.
		GLLineState()
		{  }
	};


	/**
	 * State affecting the display of polygons (including triangles and quads).
	 */
	class GLPolygonState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLPolygonState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLPolygonState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLPolygonState object with no state.
		 *
		 * Call methods of this class initialise the state.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLPolygonState());
		}

		GLPolygonState &
		gl_enable_polygon_smooth(
				GLboolean enable)
		{
			d_polygon_smooth = enable;
			return *this;
		}

		GLPolygonState &
		gl_hint_polygon_smooth(
				GLenum mode = GL_DONT_CARE)
		{
			d_polygon_smooth_hint = mode;
			return *this;
		}

		GLPolygonState &
		gl_polygon_mode(
				GLenum face = GL_FRONT_AND_BACK,
				GLenum mode = GL_FILL)
		{
			const PolygonMode polygon_mode = { face, mode };
			d_polygon_mode = polygon_mode;
			return *this;
		}

		GLPolygonState &
		gl_front_face(
				GLenum mode = GL_CCW)
		{
			d_front_face = mode;
			return *this;
		}

		GLPolygonState &
		gl_enable_cull_face(
				GLboolean enable)
		{
			d_enable_cull_face = enable;
			return *this;
		}

		GLPolygonState &
		gl_cull_face(
				GLenum mode = GL_BACK)
		{
			d_cull_face = mode;
			return *this;
		}

		GLPolygonState &
		gl_enable_polygon_offset_point(
				GLboolean enable)
		{
			d_enable_polygon_offset_point = enable;
			return *this;
		}

		GLPolygonState &
		gl_enable_polygon_offset_line(
				GLboolean enable)
		{
			d_enable_polygon_offset_line = enable;
			return *this;
		}

		GLPolygonState &
		gl_enable_polygon_offset_fill(
				GLboolean enable)
		{
			d_enable_polygon_offset_fill = enable;
			return *this;
		}

		GLPolygonState &
		gl_polygon_offset(
				GLfloat factor,
				GLfloat units)
		{
			const PolygonOffset polygon_offset = { factor, units };
			d_polygon_offset = polygon_offset;
			return *this;
		}


		virtual
		void
		enter_state_set() const;

		virtual
		void
		leave_state_set() const;

	private:
		struct PolygonMode
		{
			GLenum face;
			GLenum mode;
		};
		struct PolygonOffset
		{
			GLfloat factor;
			GLfloat units;
		};

		boost::optional<GLboolean> d_polygon_smooth;
		boost::optional<GLenum> d_polygon_smooth_hint;
		boost::optional<PolygonMode> d_polygon_mode;
		boost::optional<GLenum> d_front_face;
		boost::optional<GLboolean> d_enable_cull_face;
		boost::optional<GLenum> d_cull_face;
		boost::optional<GLboolean> d_enable_polygon_offset_point;
		boost::optional<GLboolean> d_enable_polygon_offset_line;
		boost::optional<GLboolean> d_enable_polygon_offset_fill;
		boost::optional<PolygonOffset> d_polygon_offset;

		//! Constructor.
		GLPolygonState()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLANTIALIASPRIMITIVESTATE_H
