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

#ifndef GPLATES_GUI_NURBSRENDERER_H
#define GPLATES_GUI_NURBSRENDERER_H

#include <boost/noncopyable.hpp>

#include "OpenGL.h"

#include "maths/types.h"


namespace GPlatesMaths
{
	class GreatCircleArc;
	class LatLonPoint;
	class PointOnSphere;
	class UnitVector3D;
}

namespace GPlatesGui
{
	/**
	 *  We need different sampling tolerances for great circles and small circles.
	 *  Great circles will always appear quite big in GPlates, because we can't zoom
	 *  the globe out beyond 100%, so a sampling tolerance of 25 is OK; the curves will
	 *  still appear smooth.
	 *
	 *  For small circles this can make the circle appear to have jagged edges.
	 *  (Small circles are, after all, small).
	 *  So we use a lower sampling tolerance to ensure a smooth appearance.
	 */
	static const double GREAT_CIRCLE_SAMPLING_TOLERANCE = 25.;
	static const double SMALL_CIRCLE_SAMPLING_TOLERANCE = 5.;

	/**
	 * A wrapper around the GLU nurbs renderer type.
	 *
	 * Performs resource management and provides a nice class interface.
	 */
	class NurbsRenderer :
			// Noncopyable because there doesn't seem to be a way to duplicate a GLUnurbsObj resource.
			private boost::noncopyable
	{
		public:
			NurbsRenderer();

			~NurbsRenderer()
			{
				gluDeleteNurbsRenderer(d_nurbs_ptr);
			}

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
			void
			draw_curve(
					GLint num_knots,
					GLfloat *knots,
					GLint stride,
					GLfloat *ctrl_pts,
					GLint order,
					GLenum curve_type)
			{
				gluBeginCurve(d_nurbs_ptr);
				gluNurbsCurve(d_nurbs_ptr, num_knots, knots, stride,
						ctrl_pts, order, curve_type);
				gluEndCurve(d_nurbs_ptr);
			}


			/**
			 * Draw a great circle arc on a sphere of radius one.
			 *
			 * The angle spanned by the endpoints of the GreatCircleArc
			 * must be strictly less than PI.
			 */
			void
			draw_great_circle_arc(
					const GPlatesMaths::GreatCircleArc &arc);


			/**
			 * Draw a great circle arc on a sphere of radius one.
			 *
			 * The angle spanned by points @a start and @a end
			 * must be strictly less than PI.
			 */
			void
			draw_great_circle_arc(
					const GPlatesMaths::PointOnSphere &start,
					const GPlatesMaths::PointOnSphere &end);
					
					
			/**
			 *  Draw a small circle centred at @a centre with radius @a radius_in_degrees 
			 *  degrees of arc.                                                                     
			 */
			void
			draw_small_circle(
					const GPlatesMaths::PointOnSphere &centre,
					const GPlatesMaths::Real &radius_in_radians);
					
			/**
			*  Draw a small circle centred determined by @a axis and with 
			*  radius determined by @a cos_colatitude .
			*/
			void
			draw_small_circle(
					const GPlatesMaths::UnitVector3D &axis,
					const GPlatesMaths::Real &cos_colatitude);					
				
			/**
			 *  Draw a small circle arc with
			 *		@a centre the centre of the small circle.
			 *      @a first_point_on_circle the start point of the arc, and 
			 *		@a degrees_of_arc the length of the arc in degrees.
			 *	The arc will be drawn anti-clockwise around the centre of the small circle when 
			 *	looking down onto the surface of globe.                                             
			 */	
			void
			draw_small_circle_arc(
					const GPlatesMaths::PointOnSphere &centre,
					const GPlatesMaths::PointOnSphere &first_point_on_circle,
					const GPlatesMaths::Real &arc_length_in_radians);
					
			

		private:
			/**
			 * GLU nurbs renderer object
			 */
			GLUnurbsObj *d_nurbs_ptr;


			void
			draw_great_circle_arc(
					const GPlatesMaths::UnitVector3D &start_pt,
					const GPlatesMaths::UnitVector3D &end_pt,
					const GPlatesMaths::real_t &dot_of_endpoints);

			void
			draw_great_circle_arc_smaller_than_ninety_degrees(
					const GPlatesMaths::UnitVector3D &start_pt,
					const GPlatesMaths::UnitVector3D &end_pt);
					
			void
			draw_small_circle_arc_smaller_than_or_equal_to_ninety_degrees(
					const GPlatesMaths::UnitVector3D &centre_pt,
					const GPlatesMaths::UnitVector3D &start_pt, 
					const GPlatesMaths::Real &arc_length_in_radians);
	};
}

#endif  // GPLATES_GUI_NURBSRENDERER_H
