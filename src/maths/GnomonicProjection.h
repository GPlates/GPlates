/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GNOMONICPROJECTION_H
#define GPLATES_MATHS_GNOMONICPROJECTION_H

#include <boost/optional.hpp>
#include <QPointF>

#include "AngularDistance.h"
#include "LatLonPoint.h"
#include "PointOnSphere.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * A projection from 3D points on unit sphere to a 2D tangent plane.
	 *
	 * A gnomonic projection maps great circle arcs to 2D 'straight' lines in the tangent plane.
	 */
	class GnomonicProjection
	{
	public:

		/**
		 * The tangent plane touches the unit sphere at position @a tangent_point
		 * (which is also the direction of the tangent plane normal).
		 *
		 * The tangent plane 2D axes are arbitrary (but orthogonal to each other and the tangent plane normal).
		 *
		 * If the any 3D points (to be projected) are further than @a maximum_projection_angle radians
		 * from @a tangent_point then they will fail to project to the tangent plane.
		 *
		 * @throws PreconditionViolationError exception if @a maximum_projection_angle is
		 * greater than *or equal* to PI/2 radians - note that this *excludes* PI/2 radians (ie, 90 degrees)
		 * since 3D points on the equator (with respect to the tangent point) project to infinity.
		 */
		explicit
		GnomonicProjection(
				const PointOnSphere &tangent_point,
				const AngularDistance &maximum_projection_angle);

		/**
		 * The tangent plane touches the unit sphere at position PointOnSphere(tangent_plane_normal).
		 *
		 * The tangent plane normal and its 2D plane x/y axes are explicitly specified here.
		 * Note that they should be orthogonal to each other and form a right-handed coordinate system
		 * (ie, cross(tangent_plane_x_axis, tangent_plane_y_axis) == Vector3D(tangent_plane_normal)).
		 * @throws PreconditionViolationError exception if this is not the case.
		 *
		 * If the any 3D points (to be projected) are further than @a maximum_projection_angle radians
		 * from @a tangent_point then they will fail to project to the tangent plane.
		 *
		 * @throws PreconditionViolationError exception if @a maximum_projection_angle is
		 * greater than *or equal* to PI/2 radians - note that this *excludes* PI/2 radians (ie, 90 degrees)
		 * since 3D points on the equator (with respect to the tangent point) project to infinity.
		 */
		GnomonicProjection(
				const UnitVector3D &tangent_plane_normal,
				const UnitVector3D &tangent_plane_x_axis,
				const UnitVector3D &tangent_plane_y_axis,
				const AngularDistance &maximum_projection_angle);


		/**
		 * Returns the point where the tangent plane touches the unit sphere.
		 */
		PointOnSphere
		get_tangent_point() const
		{
			return PointOnSphere(d_tangent_plate_frame.z_axis);
		}

		/**
		 * Returns tangent plane normal.
		 */
		const UnitVector3D &
		get_tangent_plane_normal() const
		{
			return d_tangent_plate_frame.z_axis;
		}

		/**
		 * Returns tangent plane x-axis (one of the two 2D reference frame axes of the tangent plane).
		 */
		const UnitVector3D &
		get_tangent_plane_x_axis() const
		{
			return d_tangent_plate_frame.x_axis;
		}

		/**
		 * Returns tangent plane y-axis (one of the two 2D reference frame axes of the tangent plane).
		 */
		const UnitVector3D &
		get_tangent_plane_y_axis() const
		{
			return d_tangent_plate_frame.y_axis;
		}


		/**
		 * Project a point in Cartesian (x,y,z) space to the tangent plane (x,y) space.
		 *
		 * Returns none if the angle between @a point and the tangent point exceeds
		 * @a maximum_projection_angle passed into constructor.
		 */
		boost::optional<QPointF>
		project_from_point_on_sphere(
				const PointOnSphere &point) const;

		/**
		 * Convenient overload to return a template 2D point type.
		 */
		template <class Point2Type>
		boost::optional<Point2Type>
		project_from_point_on_sphere(
				const PointOnSphere &point) const
		{
			const boost::optional<QPointF> new_point = project_from_point_on_sphere(point);
			if (!new_point)
			{
				return boost::none;
			}
			return Point2Type(new_point->x(), new_point->y());
		}


		/**
		 * Project a 3D point in Spherical (lon,lat) space to the tangent plane (x,y) space.
		 *
		 * Returns none if the angle between @a point and the tangent point exceeds
		 * @a maximum_projection_angle passed into constructor.
		 */
		boost::optional<QPointF>
		project_from_lat_lon(
				const LatLonPoint &point) const
		{
			return project_from_point_on_sphere(make_point_on_sphere(point));
		}

		/**
		 * Convenient overload to return a template 2D point type.
		 */
		template <class Point2Type>
		boost::optional<Point2Type>
		project_from_lat_lon(
				const LatLonPoint &point) const
		{
			const boost::optional<QPointF> new_point = project_from_lat_lon(point);
			if (!new_point)
			{
				return boost::none;
			}
			return Point2Type(new_point->x(), new_point->y());
		}


		/**
		 * Project a point in the tangent plane (x,y) space to Cartesian (x,y,z) space.
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


		/**
		 * Project a point in the tangent plane (x,y) space to Spherical (lon,lat) space.
		 */
		LatLonPoint
		unproject_to_lat_lon(
				const QPointF &point) const
		{
			return make_lat_lon_point(unproject_to_point_on_sphere(point));
		}

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

	private:

		/**
		 * The axes of the tangent plane.
		 */
		struct TangentPlaneFrame
		{
			TangentPlaneFrame(
					const UnitVector3D &x_axis_,
					const UnitVector3D &y_axis_,
					const UnitVector3D &z_axis_) :
				x_axis(x_axis_),
				y_axis(y_axis_),
				z_axis(z_axis_)
			{  }

			UnitVector3D x_axis;
			UnitVector3D y_axis;
			UnitVector3D z_axis;
		};


		/**
		 * Calculate a tangent plane frame given only a tangent plane normal.
		 */
		static
		TangentPlaneFrame
		get_tangent_plane_frame(
				const UnitVector3D &tangent_plane_normal);


		TangentPlaneFrame d_tangent_plate_frame;
		double d_minimum_projection_cosine;
	};
}

#endif // GPLATES_MATHS_GNOMONICPROJECTION_H
