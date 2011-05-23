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
 
#ifndef GPLATES_MATHS_CENTROID_H
#define GPLATES_MATHS_CENTROID_H

#include <boost/optional.hpp>

#include "MultiPointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "SphericalArea.h"
#include "UnitVector3D.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	namespace Centroid
	{
		/**
		 * Returns the sum of the sequence of @a PointOnSphere objects.
		 *
		 * NOTE: Returned vector could be zero length if points average to zero.
		 */
		template <typename PointForwardIter>
		Vector3D
		calculate_sum_points(
				PointForwardIter begin,
				PointForwardIter end);

		/**
		 * Returns the sum of the sequence of @a UnitVector3D objects.
		 *
		 * NOTE: Returned vector could be zero length if points average to zero.
		 */
		template <typename UnitVector3DForwardIter>
		Vector3D
		calculate_sum_vertices(
				UnitVector3DForwardIter begin,
				UnitVector3DForwardIter end);


		/**
		 * Calculates the centroid of the sequence of @a PointOnSphere objects.
		 *
		 * Returns false if centroid cannot be determined because points sum to the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename PointForwardIter>
		GPlatesMaths::UnitVector3D
		calculate_points_centroid(
				PointForwardIter begin,
				PointForwardIter end);

		/**
		 * Calculates the centroid of the sequence of @a UnitVector3D objects.
		 *
		 * Returns false if centroid cannot be determined because points sum to the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename UnitVector3DForwardIter>
		GPlatesMaths::UnitVector3D
		calculate_vertices_centroid(
				UnitVector3DForwardIter begin,
				UnitVector3DForwardIter end);


		/**
		 * Calculates the centroid of points on @a polygon.
		 *
		 * Returns false if centroid cannot be determined because points sum to the zero vector.
		 */
		inline
		GPlatesMaths::UnitVector3D
		calculate_points_centroid(
				const PolygonOnSphere &polygon)
		{
			return calculate_points_centroid(polygon.vertex_begin(), polygon.vertex_end());
		}


		/**
		 * Calculates the centroid of points on @a polyline.
		 *
		 * Returns false if centroid cannot be determined because points sum to the zero vector.
		 */
		inline
		GPlatesMaths::UnitVector3D
		calculate_points_centroid(
				const PolylineOnSphere &polyline)
		{
			return calculate_points_centroid(polyline.vertex_begin(), polyline.vertex_end());
		}


		/**
		 * Calculates the centroid of points on @a multi_point.
		 *
		 * Returns false if centroid cannot be determined because points sum to the zero vector.
		 */
		inline
		GPlatesMaths::UnitVector3D
		calculate_points_centroid(
				const MultiPointOnSphere &multi_point)
		{
			return calculate_points_centroid(multi_point.begin(), multi_point.end());
		}


		/**
		 * Calculates the centroid of a sequence of @a GreatCircleArc objects using
		 * spherical area weighting to get a more accurate result.
		 *
		 * Returns false if centroid cannot be determined because
		 * area weighted triangle centroids sum to the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename EdgeForwardIter>
		boost::optional<GPlatesMaths::UnitVector3D>
		calculate_spherical_centroid(
				EdgeForwardIter begin,
				EdgeForwardIter end);


		/**
		 * Calculates the centroid of @a polygon using spherical area weighting to get a more accurate result.
		 *
		 * Returns false if centroid cannot be determined because
		 * area weighted triangle centroids sum to the zero vector.
		 */
		inline
		boost::optional<GPlatesMaths::UnitVector3D>
		calculate_spherical_centroid(
				const PolygonOnSphere &polygon)
		{
			return calculate_spherical_centroid(polygon.begin(), polygon.end());
		}


		/**
		 * Calculates the centroid of @a polygon.
		 *
		 * Attempts to use the faster point centroid calculation otherwise uses
		 * more expensive spherical centroid calculation if the average of the sum
		 * of the polygon vertices has a magnitude less than @a min_average_point_magnitude.
		 *
		 * The default value of @a min_average_point_magnitude of zero means the faster point
		 * centroid calculation is used except when the average point vector is the zero vector.
		 *
		 * Returns false if centroid cannot be determined because both the point centroid and
		 * spherical centroid calculations generate the zero vector.
		 */
		boost::optional<GPlatesMaths::UnitVector3D>
		calculate_centroid(
				const PolygonOnSphere &polygon,
				const double &min_average_point_magnitude = 0.0);


		//
		// Implementation
		//


		template <typename PointForwardIter>
		Vector3D
		calculate_sum_points(
				PointForwardIter points_begin,
				PointForwardIter points_end)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					points_begin != points_end,
					GPLATES_ASSERTION_SOURCE);

			// Iterate through the points and calculate the sum of vertex positions.
			GPlatesMaths::Vector3D summed_points_position(0,0,0);

			int num_vertices = 0;
			for (PointForwardIter points_iter = points_begin;
				points_iter != points_end;
				++points_iter, ++num_vertices)
			{
				const GPlatesMaths::PointOnSphere &point = *points_iter;

				const GPlatesMaths::Vector3D vertex(point.position_vector());
				summed_points_position = summed_points_position + vertex;
			}

			return summed_points_position;
		}

		template <typename UnitVector3DForwardIter>
		Vector3D
		calculate_sum_vertices(
				UnitVector3DForwardIter points_begin,
				UnitVector3DForwardIter points_end)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					points_begin != points_end,
					GPLATES_ASSERTION_SOURCE);

			// Iterate through the points and calculate the sum of vertex positions.
			GPlatesMaths::Vector3D summed_points_position(0,0,0);

			int num_vertices = 0;
			for (UnitVector3DForwardIter points_iter = points_begin;
				points_iter != points_end;
				++points_iter, ++num_vertices)
			{
				const GPlatesMaths::UnitVector3D &point = *points_iter;

				const GPlatesMaths::Vector3D vertex(point);
				summed_points_position = summed_points_position + vertex;
			}

			return summed_points_position;
		}


		template <typename PointForwardIter>
		GPlatesMaths::UnitVector3D
		calculate_points_centroid(
				PointForwardIter points_begin,
				PointForwardIter points_end)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					points_begin != points_end,
					GPLATES_ASSERTION_SOURCE);

			const Vector3D summed_points_position = calculate_sum_points(points_begin, points_end);

			// If the magnitude of the summed vertex position is zero then all the points averaged
			// to zero and hence we cannot get a centroid point.
			// This most likely happens when the vertices roughly form a great circle arc.
			if (summed_points_position.magSqrd() <= 0.0)
			{
				// Just return the first point - this is obviously not very good but it
				// alleviates the caller from having to check an error code or catch an exception.
				// Also it's extremely unlikely to happen.
				// And even when 'summed_points_position.magSqrd()' is *very* close to zero
				// but passes the test then the returned centroid is essentially random.
				// TODO: Implement a more robust alternative for those clients that require
				// an accurate centroid all the time - for most uses in GPlates the worst
				// that happens is a small circle bounding some geometry (with centroid used
				// as small circle centre) becomes larger than it would normally be resulting
				// in less efficient intersection tests.
				return points_begin->position_vector();
			}

			return summed_points_position.get_normalisation();
		}

		template <typename UnitVector3DForwardIter>
		GPlatesMaths::UnitVector3D
		calculate_vertices_centroid(
				UnitVector3DForwardIter points_begin,
				UnitVector3DForwardIter points_end)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					points_begin != points_end,
					GPLATES_ASSERTION_SOURCE);

			const Vector3D summed_points_position = calculate_sum_points(points_begin, points_end);

			// If the magnitude of the summed vertex position is zero then all the points averaged
			// to zero and hence we cannot get a centroid point.
			// This most likely happens when the vertices roughly form a great circle arc.
			if (summed_points_position.magSqrd() <= 0.0)
			{
				// Just return the first point - this is obviously not very good but it
				// alleviates the caller from having to check an error code or catch an exception.
				// Also it's extremely unlikely to happen.
				// And even when 'summed_points_position.magSqrd()' is *very* close to zero
				// but passes the test then the returned centroid is essentially random.
				// TODO: Implement a more robust alternative for those clients that require
				// an accurate centroid all the time - for most uses in GPlates the worst
				// that happens is a small circle bounding some geometry (with centroid used
				// as small circle centre) becomes larger than it would normally be resulting
				// in less efficient intersection tests.
				return *points_begin;
			}

			return summed_points_position.get_normalisation();
		}


		template <typename EdgeForwardIter>
		boost::optional<UnitVector3D>
		calculate_spherical_centroid(
				EdgeForwardIter edges_begin,
				EdgeForwardIter edges_end)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					edges_begin != edges_end,
					GPLATES_ASSERTION_SOURCE);

			//
			// First iterate through the points and calculate the sum of vertex positions.
			// This gives us a rough centroid from which to base our triangle area calculations.
			//

			GPlatesMaths::Vector3D summed_point_position(0,0,0);

			// Add the first point on the first edge.
			summed_point_position = summed_point_position +
					GPlatesMaths::Vector3D(edges_begin->start_point().position_vector());

			EdgeForwardIter edges_iter = edges_begin;
			for ( ; edges_iter != edges_end; ++edges_iter)
			{
				const GPlatesMaths::PointOnSphere &point = edges_iter->end_point();

				const GPlatesMaths::Vector3D vertex(point.position_vector());
				summed_point_position = summed_point_position + vertex;
			}

			const PointOnSphere points_centroid =
					// If all the points average to zero (the origin) then just pick an
					// arbitrary point to base our triangle area calculations from.
					(summed_point_position.magSqrd() <= 0.0)
					? PointOnSphere(UnitVector3D(0, 0, 1)) // Arbitrarily choose the north pole
					: PointOnSphere(summed_point_position.get_normalisation());

			//
			// Next iterate through the edges and calculate the area and centroid of each
			// triangle formed by the edge and the points centroid.
			// Iterate through the points and calculate the sum of vertex positions.
			Vector3D area_weighted_centroid(0,0,0);
			real_t total_area = 0;

			edges_iter = edges_begin;
			for ( ; edges_iter != edges_end; ++edges_iter)
			{
				const GreatCircleArc &edge = *edges_iter;

				// FIXME: Detect and handle case where arc endpoints are antipodal.
				const GreatCircleArc start_edge =
						GreatCircleArc::create(points_centroid, edge.start_point());
				const GreatCircleArc end_edge =
						GreatCircleArc::create(edge.end_point(), points_centroid);

				const real_t triangle_area = SphericalArea::calculate_spherical_triangle_signed_area(
						start_edge, edge, end_edge);

				const Vector3D triangle_centroid =
								Vector3D(points_centroid.position_vector()) +
								Vector3D(edge.start_point().position_vector()) +
								Vector3D(edge.end_point().position_vector());
				if (triangle_centroid.magSqrd() > 0)
				{
					// Note that 'triangle_area' can be negative which means the triangle centroid
					// is subtracted instead of added.
					area_weighted_centroid = area_weighted_centroid +
							triangle_area * triangle_centroid.get_normalisation();
					total_area += triangle_area;
				}
			}

			if (area_weighted_centroid.magSqrd() <= 0)
			{
				// The area weighted triangle centroids summed to the zero vector.
				return boost::none;
			}

			return area_weighted_centroid.get_normalisation();
		}
	}
}

#endif // GPLATES_MATHS_CENTROID_H
