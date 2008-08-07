/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <iostream>
#include <utility>

#include "NurbsRenderer.h"
#include "OpenGLBadAllocException.h"

#include "maths/GreatCircleArc.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"
#include "maths/GenericVectorOps3D.h"


using namespace GPlatesGui;

namespace
{
	// Handle GLU NURBS errors
	GLvoid
	NurbsError()
	{
		// XXX I'm not sure if glGetError actually returns the GLU 
		// error codes as well as the GL codes.
		std::cerr
		 << "NURBS Error: "
		 << gluErrorString(glGetError())
		 << std::endl;

		exit(1);  // FIXME: should this be an exception instead?
	}
}


GPlatesGui::NurbsRenderer::NurbsRenderer()
{
	d_nurbs_ptr = gluNewNurbsRenderer();
	if (d_nurbs_ptr == 0) {

		// not enough memory to allocate object
		throw OpenGLBadAllocException(
		 "Not enough memory for OpenGL to create new NURBS renderer.");
	}
	// Previously, the type-parameter of the cast was 'void (*)()'.
	// On Mac OS X, the compiler complained, so it was changed to this.
	// Update: Fixed the prototype of the NurbsError callback function 
	// and removed the varargs ellipsis from the cast type.
	gluNurbsCallback(d_nurbs_ptr, GLU_ERROR,
#if defined(__APPLE__)
			// Assume compilation on OS X.
			reinterpret_cast< GLvoid (__CONVENTION__ *)(...) >(&NurbsError));
#else
			reinterpret_cast< GLvoid (__CONVENTION__ *)() >(&NurbsError));
#endif

	// Increase the resolution so we get smoother curves:
	//  -  Even though it's the default, we force GLU_SAMPLING_METHOD
	//    to be GLU_PATH_LENGTH.
	gluNurbsProperty(d_nurbs_ptr, GLU_SAMPLING_METHOD, GLU_PATH_LENGTH);
	//  -  The default GLU_SAMPLING_TOLERANCE is 50.0 pixels.  The OpenGL
	//    API notes that this is rather conservative, so we halve it.
	gluNurbsProperty(d_nurbs_ptr, GLU_SAMPLING_TOLERANCE, 25.0);
}

namespace
{
	/**
	 * Return the mid-point of the arc.
	 */
	GPlatesMaths::UnitVector3D
	mid_point_of(
			const GPlatesMaths::GreatCircleArc &arc)
	{
		GPlatesMaths::Vector3D start_pt = GPlatesMaths::Vector3D(arc.start_point().position_vector());
		GPlatesMaths::Vector3D end_pt = GPlatesMaths::Vector3D(arc.end_point().position_vector());

		return GPlatesMaths::Vector3D(start_pt + 0.5*(end_pt - start_pt)).get_normalisation();
	}


	typedef std::pair< GPlatesMaths::Vector3D, GLfloat > ControlPointData;


	/**
	 * Returns a Vector3D corresponding to the middle control point of the arc.
	 *
	 * The actual return values are determined thus:
	 *
	 *  - Draw a (straight) line from start_pt to end_pt.  This will be the base of a triangle.
	 *  - Using this line, create an isoceles triangle with both base angles equal to half of 
	 *    the angular extent of the arc traced out by start_pt and end_pt (i.e. the base angle
	 *    is half of  acos( dot( start_pt, end_pt ) ).)
	 *  - The top of this triangle is the location of the control point we want.
	 *  - The weight of the control point is  cos( base_angle ).
	 */
	ControlPointData
	calc_ctrl_point_and_weight(
			const GPlatesMaths::Vector3D &start_pt,
			const GPlatesMaths::Vector3D &end_pt)
	{
		const GPlatesMaths::Vector3D arc_direction = end_pt - start_pt;
		GPlatesMaths::Vector3D triangle_base_mid = start_pt + 0.5*arc_direction;

		double triangle_base_angle = 0.5 * std::acos(GPlatesMaths::GenericVectorOps3D::dot(start_pt, end_pt).dval());
		double triangle_height = 0.5 * arc_direction.magnitude().dval() * std::tan(triangle_base_angle);

		GLfloat weight = static_cast<GLfloat>(std::cos(triangle_base_angle));

		GPlatesMaths::Vector3D triangle_tip = triangle_base_mid + triangle_height * triangle_base_mid.get_normalisation();
		return std::make_pair(triangle_tip, weight);
	}
}


void
GPlatesGui::NurbsRenderer::draw_great_circle_arc(
		const GPlatesMaths::GreatCircleArc &arc)
{
	const GPlatesMaths::UnitVector3D &start_pt = arc.start_point().position_vector();
	const GPlatesMaths::UnitVector3D &end_pt = arc.end_point().position_vector();

	if (arc.dot_of_endpoints() < 0.0) {
		// arc is bigger than 90 degrees.
		GPlatesMaths::UnitVector3D mid_pt = mid_point_of(arc);

		// A great circle arc is always less than 180 degress, so if we split it
		// into two pieces, we definitely get two arcs of less than 90 degress.
		draw_great_circle_arc_smaller_than_ninety_degrees(start_pt, mid_pt);
		draw_great_circle_arc_smaller_than_ninety_degrees(mid_pt, end_pt);
	} else {
		draw_great_circle_arc_smaller_than_ninety_degrees(start_pt, end_pt);
	}
}


void
GPlatesGui::NurbsRenderer::draw_great_circle_arc_smaller_than_ninety_degrees(
		const GPlatesMaths::UnitVector3D &start_pt,
		const GPlatesMaths::UnitVector3D &end_pt)
{
	static const GLint STRIDE = 4;
	static const GLint ORDER = 3;
	static const GLsizei NUM_CONTROL_POINTS = 3;
	static const GLsizei KNOT_SIZE = 6;
	static GLfloat KNOTS[KNOT_SIZE] = 
	{
		0.0, 0.0, 0.0, 
		1.0, 1.0, 1.0
	};

	ControlPointData mid_ctrl_pt_data = calc_ctrl_point_and_weight(
			GPlatesMaths::Vector3D(start_pt), 
			GPlatesMaths::Vector3D(end_pt));

	const GPlatesMaths::Vector3D &mid_ctrl_pt = mid_ctrl_pt_data.first;
	const GLfloat &weight = mid_ctrl_pt_data.second;

	GLfloat ctrl_points[NUM_CONTROL_POINTS][STRIDE] = {
		{
			static_cast<GLfloat>(start_pt.x().dval()),
				static_cast<GLfloat>(start_pt.y().dval()),        
					static_cast<GLfloat>(start_pt.z().dval()), 
						1.0
		}, 
		{
			static_cast<GLfloat>(weight*mid_ctrl_pt.x().dval()),
				static_cast<GLfloat>(weight*mid_ctrl_pt.y().dval()),
					static_cast<GLfloat>(weight*mid_ctrl_pt.z().dval()), 
						static_cast<GLfloat>(weight)
		},
		{
			static_cast<GLfloat>(end_pt.x().dval()),
				static_cast<GLfloat>(end_pt.y().dval()),
					static_cast<GLfloat>(end_pt.z().dval()),      
						1.0
		}
	};

	draw_curve(KNOT_SIZE, &KNOTS[0], STRIDE,
			&ctrl_points[0][0], ORDER, GL_MAP1_VERTEX_4);
}
