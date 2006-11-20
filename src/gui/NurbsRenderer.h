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

#ifndef _GPLATES_GUI_NURBSRENDERER_H_
#define _GPLATES_GUI_NURBSRENDERER_H_

#include "OpenGL.h"

namespace GPlatesGui
{
	/**
	 * A wrapper around the GLU nurbs renderer type.
	 *
	 * Performs resource management and provides a nice class interface.
	 */
	class NurbsRenderer
	{
		public:
			NurbsRenderer();

			~NurbsRenderer() {

				gluDeleteNurbsRenderer(_nr);
			}

		private:
			/*
			 * These two member functions intentionally declared
			 * private to avoid object copying/assignment.
			 * [There doesn't seem to be a way to duplicate
			 * a GLUnurbsObj resource.]
			 */ 
			NurbsRenderer(const NurbsRenderer &other);

			NurbsRenderer &operator=(const NurbsRenderer &other);

		public:
			/**
			 * Draw a NURBS curve.
			 *
			 * The six parameters to this function match the last
			 * six parameters to the GLU function 'gluNurbsCurve'.
			 *
			 * @param num_knots the number of knots in @a knots.
			 * @param knots an array of non-decreasing knot values.
			 * @param stride the offset between successive curve
			 *   control points.
			 * @param ctrl_pts an array of control points.
			 * @param order the order of the NURBS curve.
			 * @param curve_type the type of the curve.
			 *
			 * The <i>order</i> of the NURBS curve equals
			 * (<i>degree</i> + 1).  Thus, a cubic curve has an
			 * order of 4.  The number of knots equals the order
			 * of the curve plus the number of control points
			 * (the length of the array @a ctrl_pts).
			 */
			void
			drawCurve(GLint num_knots,
			          GLfloat *knots,
			          GLint stride,
			          GLfloat *ctrl_pts,
			          GLint order,
			          GLenum curve_type) {

				gluBeginCurve(_nr);
				gluNurbsCurve(_nr, num_knots, knots, stride,
				 ctrl_pts, order, curve_type);
				gluEndCurve(_nr);
			}

		private:
			/**
			 * GLU nurbs renderer object
			 */
			GLUnurbsObj *_nr;
	};
}

#endif /* _GPLATES_GUI_NURBSRENDERER_H_ */
