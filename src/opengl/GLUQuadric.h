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

#ifndef _GPLATES_OPENGL_GLUQUADRIC_H_
#define _GPLATES_OPENGL_GLUQUADRIC_H_

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLDrawable.h"

#include "gui/Colour.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A wrapper around the GLU quadric type.
	 */
	class GLUQuadric :
			public GPlatesUtils::ReferenceCount<GLUQuadric>
	{
		public:
			//! A convenience typedef for a shared pointer to a non-const @a GLUQuadric.
			typedef GPlatesUtils::non_null_intrusive_ptr<GLUQuadric> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a GLUQuadric.
			typedef GPlatesUtils::non_null_intrusive_ptr<const GLUQuadric> non_null_ptr_to_const_type;

			//! Typedef for a shared pointer to a 'GLUquadricObj'.
			typedef boost::shared_ptr<GLUquadricObj> glu_quadric_obj_type;


			/**
			 * Creates a @a GLUQuadric object.
			 */
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new GLUQuadric());
			}


			/**
			 * Parameters that determine the appearance of a quadric.
			 */
			struct Parameters
			{
				//! Constructor sets parameters to GLU defaults.
				Parameters();

				GLenum normals;
				GLboolean texture_coords;
				GLenum orientation;
				GLenum draw_style;
			};


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
			 * - GLU_NONE: No normals are generated.  This is the default.
			 * - GLU_FLAT: One normal is generated for every facet
			 *    of a quadric.
			 * - GLU_SMOOTH: One normal is generated for every
			 *    vertex of a quadric.
			 */
			void
			set_normals(GLenum normals)
			{
				d_current_parameters.normals = normals;
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
				d_current_parameters.texture_coords = texture_coords;
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
				d_current_parameters.orientation = orientation;
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
			 *    This is the default.
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
				d_current_parameters.draw_style = draw_style;
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
			GLDrawable::non_null_ptr_to_const_type
			draw_sphere(
					GLdouble radius,
					GLint num_slices,
					GLint num_stacks,
					const GPlatesGui::Colour &colour);


			/**
			 * Draw a quadric disk.
			 *
			 * A disk is rendered on the z = 0 plane. The disk has a radius of outer, and
			 * contains a concentric circular hole with a radius of inner. If inner is 0,
			 * then no hole is generated. The disk is subdivided around the z axis into
			 * slices (like pizza slices), and also about the z axis into rings (as
			 * specified by slices and loops, respectively).
			 *
			 * If the orientation is set to 'GLU_OUTSIDE' (with @a set_orientation), then
			 * any normals generated point along the +z axis. Otherwise, they point along
			 * the -z axis.
			 */
			GLDrawable::non_null_ptr_to_const_type
			draw_disk(
					GLdouble inner,
					GLdouble outer,
					GLint num_slices,
					GLint num_loops,
					const GPlatesGui::Colour &colour);

		private:
			/**
			 * GLU quadrics object
			 */
			glu_quadric_obj_type d_quadric;

			Parameters d_current_parameters;


			//! Constructor.
			GLUQuadric()
			{  }

			/**
			 * Creates our 'GLUquadricObj' in 'd_quadric'.
			 *
			 * Creation is delayed until something is drawn because when something
			 * is drawn we know the OpenGL context is current.
			 */
			void
			create_quadric_obj();
	};
}

#endif /* _GPLATES_OPENGL_GLUQUADRIC_H_ */
