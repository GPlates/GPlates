/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLUNURBSRENDERER_H
#define GPLATES_OPENGL_GLUNURBSRENDERER_H

#include <boost/noncopyable.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLDrawable.h"

#include "gui/Colour.h"

#include "maths/types.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	class GreatCircleArc;
	class LatLonPoint;
	class PointOnSphere;
	class UnitVector3D;
}

namespace GPlatesOpenGL
{
	/**
	 * A wrapper around the GLU nurbs renderer type.
	 */
	class GLUNurbsRenderer :
			public GPlatesUtils::ReferenceCount<GLUNurbsRenderer>
	{
		public:
			//! A convenience typedef for a shared pointer to a non-const @a GLUNurbsRenderer.
			typedef GPlatesUtils::non_null_intrusive_ptr<GLUNurbsRenderer> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a GLUNurbsRenderer.
			typedef GPlatesUtils::non_null_intrusive_ptr<const GLUNurbsRenderer> non_null_ptr_to_const_type;

			//! Typedef for a shared pointer to a 'GLUnurbsObj'.
			typedef boost::shared_ptr<GLUnurbsObj> glu_nurbs_obj_type;


			/**
			 * Creates a @a GLUNurbsRenderer object.
			 */
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new GLUNurbsRenderer());
			}


			/**
			 * Parameters that determine the appearance of a quadric.
			 */
			struct Parameters
			{
				//! Constructor sets parameters to GLU defaults.
				Parameters();

				GLfloat sampling_method;
				GLfloat sampling_tolerance;
			};


			/**
			 * Draw a general NURBS curve.
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
			GLDrawable::non_null_ptr_to_const_type
			draw_curve(
					GLint num_knots,
					const boost::shared_array<GLfloat> &knots,
					GLint stride,
					const boost::shared_array<GLfloat> &ctrl_pts,
					GLint order,
					GLenum curve_type,
					const GPlatesGui::Colour &colour);

			/**
			 * Draw a great circle arc on a sphere of radius one.
			 *
			 * The angle spanned by the endpoints of the GreatCircleArc
			 * must be strictly less than PI.
			 */
			GLDrawable::non_null_ptr_to_const_type
			draw_great_circle_arc(
					const GPlatesMaths::GreatCircleArc &arc,
					const GPlatesGui::Colour &colour);


			/**
			 * Draw a great circle arc on a sphere of radius one.
			 *
			 * The angle spanned by points @a start and @a end
			 * must be strictly less than PI.
			 */
			GLDrawable::non_null_ptr_to_const_type
			draw_great_circle_arc(
					const GPlatesMaths::PointOnSphere &start,
					const GPlatesMaths::PointOnSphere &end,
					const GPlatesGui::Colour &colour);
					
					
			/**
			 *  Draw a small circle centred at @a centre with radius @a radius_in_degrees 
			 *  degrees of arc.                                                                     
			 */
			GLDrawable::non_null_ptr_to_const_type
			draw_small_circle(
					const GPlatesMaths::PointOnSphere &centre,
					const GPlatesMaths::Real &radius_in_radians,
					const GPlatesGui::Colour &colour);
					
			/**
			*  Draw a small circle centred determined by @a axis and with 
			*  radius determined by @a cos_colatitude .
			*/
			GLDrawable::non_null_ptr_to_const_type
			draw_small_circle(
					const GPlatesMaths::UnitVector3D &axis,
					const GPlatesMaths::Real &cos_colatitude,
					const GPlatesGui::Colour &colour);					
				
			/**
			 *  Draw a small circle arc with
			 *		@a centre the centre of the small circle.
			 *      @a first_point_on_circle the start point of the arc, and 
			 *		@a degrees_of_arc the length of the arc in degrees.
			 *	The arc will be drawn anti-clockwise around the centre of the small circle when 
			 *	looking down onto the surface of globe.                                             
			 */	
			GLDrawable::non_null_ptr_to_const_type
			draw_small_circle_arc(
					const GPlatesMaths::PointOnSphere &centre,
					const GPlatesMaths::PointOnSphere &first_point_on_circle,
					const GPlatesMaths::Real &arc_length_in_radians,
					const GPlatesGui::Colour &colour);
					
			

		private:
			/**
			 * GLU nurbs renderer object
			 */
			glu_nurbs_obj_type d_nurbs;

			Parameters d_current_parameters;


			//! Constructor.
			GLUNurbsRenderer();


			/**
			 * Creates our 'GLUnurbsObj' in 'd_nurbs'.
			 *
			 * Creation is delayed until something is drawn because when something
			 * is drawn we know the OpenGL context is current.
			 */
			void
			create_nurbs_obj();


			GLDrawable::non_null_ptr_to_const_type
			draw_great_circle_arc(
					const GPlatesMaths::UnitVector3D &start_pt,
					const GPlatesMaths::UnitVector3D &end_pt,
					const GPlatesMaths::real_t &dot_of_endpoints,
					const GPlatesGui::Colour &colour);

			GLDrawable::non_null_ptr_to_const_type
			draw_great_circle_arc_smaller_than_ninety_degrees(
					const GPlatesMaths::UnitVector3D &start_pt,
					const GPlatesMaths::UnitVector3D &end_pt,
					const GPlatesGui::Colour &colour);
					
			GLDrawable::non_null_ptr_to_const_type
			draw_small_circle_arc_smaller_than_or_equal_to_ninety_degrees(
					const GPlatesMaths::UnitVector3D &centre_pt,
					const GPlatesMaths::UnitVector3D &start_pt, 
					const GPlatesMaths::Real &arc_length_in_radians,
					const GPlatesGui::Colour &colour);
	};
}

#endif  // GPLATES_OPENGL_GLUNURBSRENDERER_H
