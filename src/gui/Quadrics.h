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

#ifndef _GPLATES_GUI_QUADRICS_H_
#define _GPLATES_GUI_QUADRICS_H_

#include <boost/noncopyable.hpp>

#include "OpenGL.h"

namespace GPlatesGui
{
	/**
	 * A wrapper around the GLU quadrics type.
	 *
	 * Performs resource management and provides a nice class interface.
	 */
	class Quadrics :
		private boost::noncopyable
	{
		public:
			Quadrics();

			~Quadrics()
			{
				gluDeleteQuadric(d_quadric);
			}

			/**
			 * Specify what kind of normals are desired for
			 * quadrics rendered by an instance of this type.
			 *
			 * The parameter to this function matches the latter
			 * parameter to the GLU function 'gluQuadricNormals'.
			 *
			 * @param normals the desired type of normal.
			 *
			 * Valid values for @a normals are as follows:
			 * - GLU_NONE: No normals are generated.
			 * - GLU_FLAT: One normal is generated for every facet
			 *    of a quadric.
			 * - GLU_SMOOTH: One normal is generated for every
			 *    vertex of a quadric.  This is the default.
			 */
			void
			set_normals(GLenum normals)
			{

				gluQuadricNormals(d_quadric, normals);
			}


			/**
			 * Specify whether texture coordinates should be
			 * generated for quadrics rendered by an instance
			 * of this type.
			 *
			 * The parameter to this function matches the latter
			 * parameter to the GLU function 'set_generate_texture'.
			 *
			 * @param texture_coords a flag indicating whether
			 *   texture coordinates should be generated.
			 *
			 * If the value of @a texture_coords is:
			 * - GL_TRUE: texture coordinates are generated.
			 * - GL_FALSE: texture coordinates are not generated.
			 *    This is the default.
			 */
			void
			set_generate_texture(GLboolean texture_coords)
			{

				gluQuadricTexture(d_quadric, texture_coords);
			}


			/**
			 * Specify what kind of orientation is desired for
			 * quadrics rendered by an instance of this type.
			 *
			 * The parameter to this function matches the latter
			 * parameter to the GLU function
			 * 'gluQuadricOrientation'.
			 *
			 * @param orientation the desired orientation.
			 *
			 * Valid values for @a orientation are as follows:
			 * - GLU_OUTSIDE: quadrics are drawn with normals
			 *    pointing outward.  This is the default.
			 * - GLU_INSIDE: normals point inward.
			 */
			void
			set_orientation(GLenum orientation)
			{

				gluQuadricOrientation(d_quadric, orientation);
			}


			/**
			 * Specify the draw style for quadrics rendered by
			 * an instance of this type.
			 *
			 * The parameter to this function matches the latter
			 * parameter to the GLU function 'gluQuadricDrawStyle'.
			 *
			 * @param draw_style the draw style.
			 *
			 * Valid values for @a draw_style are as follows:
			 * - GLU_FILL: quadrics are rendered with polygon
			 *    primitives.  The polygons are drawn in a
			 *    counterclockwise fashion with respect to
			 *    their normals (as defined by @a set_orientation).
			 * - GLU_LINE: quadrics are rendered as a set of lines.
			 * - GLU_SILHOUETTE: quadrics are rendered as a set of
			 *    lines, except that edges separating coplanar
			 *    faces will not be drawn.
			 * - GLU_POINT: quadrics are rendered as a set of
			 *    points.
			 */
			void
			set_draw_style(GLenum draw_style)
			{

				gluQuadricDrawStyle(d_quadric, draw_style);
			}


			/**
			 * Draw a quadric sphere.
			 *
			 * A sphere of the specified radius is drawn, centred
			 * on the origin.  The sphere is subdivided about the
			 * the <i>z</i> axis into slices and along the <i>z</i>
			 * axis into stacks (similar to lines of longitude and
			 * latitude, respectively).
			 *
			 * The three parameters to this function match the last
			 * three parameters to the GLU function 'gluSphere'.
			 *
			 * @param radius the radius of the sphere.
			 * @param num_slices the number of subdivisions about
			 *   the <i>z</i> axis.
			 * @param num_stacks the number of subdivisions along
			 *   the <i>z</i> axis.
			 *
			 * If the orientation is set to 'GLU_OUTSIDE' (with
			 * @a set_orientation), then any normals generated
			 * point away from the center of the sphere.
			 * Otherwise, they point toward the center of the
			 * sphere.
			 */
			void
			draw_sphere(GLdouble radius,
					GLint num_slices,
					GLint num_stacks)
			{

				gluSphere(d_quadric, radius, num_slices, num_stacks);
			}

		private:
			/**
			 * GLU quadrics object
			 */
			GLUquadricObj *d_quadric;
	};
}

#endif /* _GPLATES_GUI_QUADRICS_H_ */
