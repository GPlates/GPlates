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
 
#ifndef GPLATES_OPENGL_GLUNURBSRENDERERDRAWABLE_H
#define GPLATES_OPENGL_GLUNURBSRENDERERDRAWABLE_H

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLDrawable.h"
#include "GLUNurbsRenderer.h"

#include "gui/Colour.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Interface for drawing the different geometry types supported by GLU nurbs renderer.
	 */
	class GLUNurbsGeometry
	{
	public:
		virtual
		~GLUNurbsGeometry()
		{  }


		virtual
		void
		draw(
				GLUnurbsObj *nurbs) const = 0;
	};


	/**
	 * Draws a nurbs curve.
	 */
	class GLUNurbsCurve :
			public GLUNurbsGeometry
	{
	public:
		GLUNurbsCurve(
				GLint num_knots,
				const boost::shared_array<GLfloat> &knots,
				GLint stride,
				const boost::shared_array<GLfloat> &ctrl_pts,
				GLint order,
				GLenum curve_type);


		virtual
		void
		draw(
				GLUnurbsObj *nurbs) const;

	private:
		GLint d_num_knots;
		boost::shared_array<GLfloat> d_knots;
		GLint d_stride;
		boost::shared_array<GLfloat> d_ctrl_pts;
		GLint d_order;
		GLenum d_curve_type;
	};


	/**
	 * A drawable using a GLU nurbs renderer.
	 */
	class GLUNurbsRendererDrawable :
			public GLDrawable
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLUNurbsRendererDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLUNurbsRendererDrawable> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLUNurbsRendererDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLUNurbsRendererDrawable> non_null_ptr_to_const_type;

		/**
		 * Creates a @a GLUNurbsRendererDrawable object.
		 */
		static
		non_null_ptr_type
		create(
				const GLUNurbsRenderer::glu_nurbs_obj_type &glu_nurbs_obj,
				const boost::shared_ptr<GLUNurbsGeometry> &glu_nurbs_geometry,
				const GLUNurbsRenderer::Parameters &glu_nurbs_params,
				const GPlatesGui::Colour &colour)
		{
			return non_null_ptr_type(
					new GLUNurbsRendererDrawable(
							glu_nurbs_obj, glu_nurbs_geometry, glu_nurbs_params, colour));
		}


		virtual
		void
		bind() const;


		virtual
		void
		draw() const;

	private:
		GLUNurbsRenderer::glu_nurbs_obj_type d_glu_nurbs_obj;
		boost::shared_ptr<GLUNurbsGeometry> d_glu_nurbs_geometry;
		GLUNurbsRenderer::Parameters d_glu_nurbs_params;
		GPlatesGui::Colour d_colour;


		//! Constructor.
		GLUNurbsRendererDrawable(
				const GLUNurbsRenderer::glu_nurbs_obj_type &glu_nurbs_obj,
				const boost::shared_ptr<GLUNurbsGeometry> &glu_nurbs_geometry,
				const GLUNurbsRenderer::Parameters &glu_nurbs_params,
				const GPlatesGui::Colour &colour) :
			d_glu_nurbs_obj(glu_nurbs_obj),
			d_glu_nurbs_geometry(glu_nurbs_geometry),
			d_glu_nurbs_params(glu_nurbs_params),
			d_colour(colour)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLUNURBSRENDERERDRAWABLE_H
