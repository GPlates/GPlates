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
 
#ifndef GPLATES_OPENGL_GLUQUADRICDRAWABLE_H
#define GPLATES_OPENGL_GLUQUADRICDRAWABLE_H

#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLDrawable.h"
#include "GLUQuadric.h"

#include "gui/Colour.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Interface for drawing the different geometry types supported by GLU quadrics.
	 */
	class GLUQuadricGeometry
	{
	public:
		virtual
		~GLUQuadricGeometry()
		{  }


		virtual
		void
		draw(
				GLUquadricObj *quadric) const = 0;
	};


	/**
	 * Draws a quadric sphere.
	 */
	class GLUQuadricSphere :
			public GLUQuadricGeometry
	{
	public:
		GLUQuadricSphere(
				GLdouble radius,
				GLint num_slices,
				GLint num_stacks);


		virtual
		void
		draw(
				GLUquadricObj *quadric) const
		{
			gluSphere(quadric, d_radius, d_num_slices, d_num_stacks);
		}

	private:
		GLdouble d_radius;
		GLint d_num_slices;
		GLint d_num_stacks;
	};


	class GLUQuadricDisk :
			public GLUQuadricGeometry
	{
	public:
		GLUQuadricDisk(
				GLdouble inner,
				GLdouble outer,
				GLint num_slices,
				GLint num_loops);


		virtual
		void
		draw(
				GLUquadricObj *quadric) const
		{
			gluDisk(quadric, d_inner, d_outer, d_num_slices, d_num_loops);
		}

	private:
		GLdouble d_inner;
		GLdouble d_outer;
		GLint d_num_slices;
		GLint d_num_loops;
	};


	/**
	 * A drawable using a GLU quadric.
	 */
	class GLUQuadricDrawable :
			public GLDrawable
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLUQuadricDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLUQuadricDrawable> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLUQuadricDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLUQuadricDrawable> non_null_ptr_to_const_type;

		/**
		 * Creates a @a GLUQuadricDrawable object.
		 */
		static
		non_null_ptr_type
		create(
				const GLUQuadric::glu_quadric_obj_type &glu_quadric_obj,
				const boost::shared_ptr<GLUQuadricGeometry> &glu_quadric_geometry,
				const GLUQuadric::Parameters &glu_quadric_params,
				const GPlatesGui::Colour &colour)
		{
			return non_null_ptr_type(
					new GLUQuadricDrawable(
							glu_quadric_obj, glu_quadric_geometry, glu_quadric_params, colour));
		}


		virtual
		void
		bind() const;


		virtual
		void
		draw() const;

	private:
		GLUQuadric::glu_quadric_obj_type d_glu_quadric_obj;
		boost::shared_ptr<GLUQuadricGeometry> d_glu_quadric_geometry;
		GLUQuadric::Parameters d_glu_quadric_params;
		GPlatesGui::Colour d_colour;


		//! Constructor.
		GLUQuadricDrawable(
				const GLUQuadric::glu_quadric_obj_type &glu_quadric_obj,
				const boost::shared_ptr<GLUQuadricGeometry> &glu_quadric_geometry,
				const GLUQuadric::Parameters &glu_quadric_params,
				const GPlatesGui::Colour &colour) :
			d_glu_quadric_obj(glu_quadric_obj),
			d_glu_quadric_geometry(glu_quadric_geometry),
			d_glu_quadric_params(glu_quadric_params),
			d_colour(colour)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLUQUADRICDRAWABLE_H
