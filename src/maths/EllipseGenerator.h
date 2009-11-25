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

#ifndef GPLATES_MATHS_ELLIPSEGENERATOR_H
#define GPLATES_MATHS_ELLIPSEGENERATOR_H

#include <boost/noncopyable.hpp>

#include "maths/Rotation.h"
#include "model/types.h"

namespace GPlatesMaths
{
	class GreatCircle;
	class PointOnSphere;

	/**
	 * This class can be used to obtain unit vector representations of points on 
	 * an ellipse as a function of the angle from the semi-major axis.
	 *
	 * Usage: 
	 * 1. construct an EllipseGenerator, providing the desired centre, semi-major and
	 * semi-minor axes, and orientation. The semi-minor axis of the ellipse will lie along
	 * the great circle @a axis.
	 * 2. Call the function get_point_on_ellipse(double angle_in_radians) to obtain the 
	 * unit vector of the point at angle @a angle_from_semi_major_axis from the semi-major axis.
	 */
	class EllipseGenerator:
		public boost::noncopyable
	{

	public:
		EllipseGenerator(
			const PointOnSphere &centre,
			const Real &semi_major_axis_radians,
			const Real &semi_minor_axis_radians,
			const GreatCircle &axis);

		UnitVector3D 
		get_point_on_ellipse(
			double angle_from_semi_major_axis);

	private:
		/** 
		 * The rotation required to transform a point on the ellipse, defined in 
		 * a tangent plane to the north pole, to the desired location and orientation
		 * on the sphere.
		 */
		Rotation d_rotation;
		
		//! Semi major axis of the ellipse as defined in the tangent plane to the north pole.
		double d_semi_major_axis;
		
		//! Semi minor axis of the ellipse as defined in the tangent plane to the north pole.
		double d_semi_minor_axis;
	};

}

#endif //GPLATES_MATHS_ELLIPSEGENERATOR_H 

