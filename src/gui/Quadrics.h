/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_QUADRICS_H_
#define _GPLATES_GUI_QUADRICS_H_

#include "OpenGL.h"

namespace GPlatesGui
{
	/**
	 * A wrapper around the GLU quadrics type.
	 *
	 * Performs resource management and provides a nice class interface.
	 */
	class Quadrics
	{
		public:
			Quadrics();

			~Quadrics() {

				gluDeleteQuadric(_q);
			}

		private:
			/*
			 * These two member functions intentionally declared
			 * private to avoid object copying/assignment.
			 * [There doesn't seem to be a way to duplicate
			 * a GLUquadricObj resource.]
			 */ 
			Quadrics(const Quadrics &other);

			Quadrics &operator=(const Quadrics &other);

		public:
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
			setNormals(GLenum normals) {

				gluQuadricNormals(_q, normals);
			}


			/**
			 * Specify whether texture coordinates should be
			 * generated for quadrics rendered by an instance
			 * of this type.
			 *
			 * The parameter to this function matches the latter
			 * parameter to the GLU function 'setGenerateTexture'.
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
			setGenerateTexture(GLboolean texture_coords) {

				gluQuadricTexture(_q, texture_coords);
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
			setOrientation(GLenum orientation) {

				gluQuadricOrientation(_q, orientation);
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
			 *    their normals (as defined by @a setOrientation).
			 * - GLU_LINE: quadrics are rendered as a set of lines.
			 * - GLU_SILHOUETTE: quadrics are rendered as a set of
			 *    lines, except that edges separating coplanar
			 *    faces will not be drawn.
			 * - GLU_POINT: quadrics are rendered as a set of
			 *    points.
			 */
			void
			setDrawStyle(GLenum draw_style) {

				gluQuadricDrawStyle(_q, draw_style);
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
			 * @a setOrientation), then any normals generated
			 * point away from the center of the sphere.
			 * Otherwise, they point toward the center of the
			 * sphere.
			 */
			void
			drawSphere(GLdouble radius,
			           GLint num_slices,
			           GLint num_stacks) {

				gluSphere(_q, radius, num_slices, num_stacks);
			}

		private:
			/**
			 * GLU quadrics object
			 */
			GLUquadricObj *_q;
	};
}

#endif /* _GPLATES_GUI_QUADRICS_H_ */
