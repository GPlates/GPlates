/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_AZIMUTHALEQUALAREAPROJECTION_H
#define GPLATES_MATHS_AZIMUTHALEQUALAREAPROJECTION_H

#include <QPointF>

#include "LatLonPoint.h"
#include "PointOnSphere.h"


namespace GPlatesMaths
{
	//
	// Lambert Equal Area Projection
	//
	// http://mathworld.wolfram.com/LambertAzimuthalEqual-AreaProjection.html
	//
	class AzimuthalEqualAreaProjection
	{
	public:

		/**
		 * @a projection_scale is a scale factor for the projected coordinates.
		 * When unprojected back onto the globe the scale factor is reversed/undone.
		 */
		explicit
		AzimuthalEqualAreaProjection(
				const LatLonPoint &center_of_projection,
				const double &projection_scale = 1.0);

		/**
		 * @a projection_scale is a scale factor for the projected coordinates.
		 * When unprojected back onto the globe the scale factor is reversed/undone.
		 */
		explicit
		AzimuthalEqualAreaProjection(
				const PointOnSphere &center_of_projection,
				const double &projection_scale = 1.0);


		/**
		 * Returns centre of projection passed into constructor.
		 */
		const LatLonPoint &
		get_center_of_projection() const
		{
			return d_center_of_projection;
		}


		/**
		 * Project a point in Spherical (lon,lat) space to Azimuthal Equal Area (x,y) space.
		 */
		QPointF
		project_from_lat_lon(
				const LatLonPoint &point) const;

		/**
		 * Convenient overload to return a template 2D point type.
		 */
		template <class Point2Type>
		Point2Type
		project_from_lat_lon(
				const LatLonPoint &point) const
		{
			const QPointF new_point = project_from_lat_lon(point);
			return Point2Type(new_point.x(), new_point.y());
		}


		/**
		 * Project a point in Cartesian (x,y,z) space to Azimuthal Equal Area (x,y) space.
		 */
		QPointF
		project_from_point_on_sphere(
				const PointOnSphere &point) const;

		/**
		 * Convenient overload to return a template 2D point type.
		 */
		template <class Point2Type>
		Point2Type
		project_from_point_on_sphere(
				const PointOnSphere &point) const
		{
			const QPointF new_point = project_from_point_on_sphere(point);
			return Point2Type(new_point.x(), new_point.y());
		}


		/**
		 * Project a point in Azimuthal Equal Area (x,y) space to Spherical (lon,lat) space.
		 */
		LatLonPoint
		unproject_to_lat_lon(
				const QPointF &point) const;

		/**
		 * Convenient overload to accept a template 2D point type.
		 */
		template <class Point2Type>
		LatLonPoint
		unproject_to_lat_lon(
				const Point2Type &point) const
		{
			return unproject_to_lat_lon(QPointF(point.x(), point.y()));
		}


		/**
		 * Project a point in Azimuthal Equal Area (x,y) space to Cartesian (x,y,z) space.
		 */
		PointOnSphere
		unproject_to_point_on_sphere(
				const QPointF &point) const;

		/**
		 * Convenient overload to accept a template 2D point type.
		 */
		template <class Point2Type>
		PointOnSphere
		unproject_to_point_on_sphere(
				const Point2Type &point) const
		{
			return unproject_to_point_on_sphere(QPointF(point.x(), point.y()));
		}

	private:

		const LatLonPoint d_center_of_projection;
		const double d_sin_center_of_projection_latitude;
		const double d_cos_center_of_projection_latitude;

		const double d_projection_scale;
	};
}

#endif // GPLATES_MATHS_AZIMUTHALEQUALAREAPROJECTION_H
