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

#include <boost/optional.hpp>

#include "Centroid.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "MathsUtils.h"

#include "utils/Profile.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Visits a @a GeometryOnSphere and calculates its centroid.
		 */
		class CentroidGeometryOnSphere :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			/**
			 * Returns the centroid of the visiting @a GeometryOnSphere.
			 */
			const boost::optional<UnitVector3D> &
			get_centroid() const
			{
				return d_centroid;
			}


			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_centroid = Centroid::calculate_centroid(*multi_point_on_sphere);
			}

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_centroid = Centroid::calculate_centroid(*point_on_sphere);
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_centroid = Centroid::calculate_centroid(*polygon_on_sphere);
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_centroid = Centroid::calculate_centroid(*polyline_on_sphere);
			}

		private:
			boost::optional<UnitVector3D> d_centroid;
		};
	}
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::calculate_centroid(
		const GeometryOnSphere &geometry_on_sphere)
{
	CentroidGeometryOnSphere visitor;

	geometry_on_sphere.accept_visitor(visitor);

	const boost::optional<UnitVector3D> &centroid = visitor.get_centroid();

	// We should have all geometry types covered.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			centroid,
			GPLATES_ASSERTION_SOURCE);

	return centroid.get();
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::calculate_centroid(
		const PolylineOnSphere &polyline,
		const double &min_average_point_magnitude)
{
	//PROFILE_FUNC();

	if (!are_almost_exactly_equal(min_average_point_magnitude, 0))
	{
		const Vector3D summed_points_position =
				calculate_sum_points(polyline.vertex_begin(), polyline.vertex_end());

		// When the magnitude gets too small divert to a more expensive but more accurate test.
		const double min_summed_point_magnitude = polyline.number_of_vertices() * min_average_point_magnitude;
		if (summed_points_position.magSqrd() < min_summed_point_magnitude * min_summed_point_magnitude)
		{
			return calculate_spherical_centroid(polyline);
		}
	}

	return calculate_edges_centroid(polyline);
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::calculate_centroid(
		const PolygonOnSphere &polygon,
		const double &min_average_point_magnitude)
{
	//PROFILE_FUNC();

	if (!are_almost_exactly_equal(min_average_point_magnitude, 0))
	{
		const Vector3D summed_points_position =
				calculate_sum_points(polygon.vertex_begin(), polygon.vertex_end());

		// When the magnitude gets too small divert to a more expensive but more accurate test.
		const double min_summed_point_magnitude = polygon.number_of_vertices() * min_average_point_magnitude;
		if (summed_points_position.magSqrd() < min_summed_point_magnitude * min_summed_point_magnitude)
		{
			return calculate_spherical_centroid(polygon);
		}
	}

	return calculate_edges_centroid(polygon);
}
