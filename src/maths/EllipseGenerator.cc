/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PointOnSphere.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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
 
 #include "EllipseGenerator.h"
 #include "GreatCircle.h"
 #include "PointOnSphere.h"
 #include "Rotation.h"
 #include "UnitVector3D.h"
 
 #include "global/types.h"
 
 
 namespace
 {
 #if 0
	void
	draw_point(
		const GPlatesMaths::PointOnSphere &point)
	{
		glBegin(GL_POINTS);
		glVertex3d(point.position_vector().x().dval(),
					point.position_vector().y().dval(),
					point.position_vector().z().dval());
		glEnd();
	}
#endif
	GPlatesMaths::Real
	get_rotation_angle(
		const GPlatesMaths::PointOnSphere &u1,
		const GPlatesMaths::PointOnSphere &u2,
		const GPlatesMaths::PointOnSphere &pivot)
	{
		GPlatesMaths::GreatCircle c1(pivot,u1);
		GPlatesMaths::GreatCircle c2(pivot,u2);

		GPlatesMaths::Real angle = GPlatesMaths::acos(GPlatesMaths::dot(c1.normal(),c2.normal()));

		GPlatesMaths::UnitVector3D norm_cross_product = 
			GPlatesMaths::cross(c1.normal(),c2.normal()).get_normalisation();
	
		if (GPlatesMaths::dot(norm_cross_product,pivot.position_vector()).is_precisely_less_than(0.))
		{
			angle = - angle;
		}			

		return angle;
	}	
 }
 
GPlatesMaths::EllipseGenerator::EllipseGenerator(
	const PointOnSphere &centre, 
	const Real &semi_major_axis_radians,
	const Real &semi_minor_axis_radians, 
	const GreatCircle &axis):
d_rotation(Rotation::create(UnitVector3D(1,0,0),0))
{
	// The following maths can probably be simplified greatly. 
	
	// Consider a tangent plane touching the earth's north pole, and an ellipse on this plane, 
	// with its semi-major axis along our x-axis. 
	// d_semi_major_axis is the distance from the centre to the intersection of
	// the ellipse and the semi-major axis. 
	// d_semi_minor_axis is the distance from the centre to the intersection of 
	// the ellipse and the semi-minor axis.
	d_semi_major_axis = std::tan(semi_major_axis_radians.dval());
	d_semi_minor_axis = std::tan(semi_minor_axis_radians.dval());

	// r1 is a rotation around our x-axis.
	Rotation r1 = Rotation::create(UnitVector3D(0,1,0),semi_major_axis_radians);
	// Rotate a point at the north pole so that it lies at the end of the ellipse's semi-minor axis.
	PointOnSphere p1= r1*PointOnSphere::north_pole;
	// r2 is a rotation from the north pole to the ellipse's desired centre.
	Rotation r2 = Rotation::create(PointOnSphere::north_pole,centre);
	// Rotate p1 to see where it ends up after applying r2. We'll use this later to apply a correction
	// to the rotation matrix.
	p1 = r2*p1;

	// Consider the ellipse at its desired location and orientation.
	// r3 represents a rotation from the ellipse's centre to the end of its semi-minor axis.
	Rotation r3 = Rotation::create(axis.axis_vector(),semi_major_axis_radians);
	// p2 is a point at the end of the ellipse's semi-minor axis. 
	PointOnSphere p2 = r3*centre;		
	
	// Determine the angle required to rotate p1 to p2, with rotation around the ellipse centre.	
	Real angle = get_rotation_angle(p1,p2,centre);

	Rotation r4 = Rotation::create(centre.position_vector(),angle);

	// r2 is a rotation from the north pole to the ellipse's desired centre.
	// r4 is a rotation about the ellipse's centre which corrects the orientation of the ellipse. 
	d_rotation = r4*r2;	
}

GPlatesMaths::UnitVector3D
GPlatesMaths::EllipseGenerator::get_point_on_ellipse(
	double angle_from_semi_major_axis)
{
	double x = d_semi_major_axis*std::cos(angle_from_semi_major_axis);
	double y = d_semi_minor_axis*std::sin(angle_from_semi_major_axis);
	Vector3D v = Vector3D(x,y,1.);
	// Vector v represents a point on the ellipse in the tangent plane; normalise it to get it on the sphere.
	UnitVector3D uv = v.get_normalisation();
	// Rotate the point to conform to the ellipse's desired location and orientation.
	return d_rotation*uv;
}

 
