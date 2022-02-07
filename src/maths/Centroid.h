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

#include <cmath>

#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "Rotation.h"
#include "SphericalArea.h"
#include "UnitVector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


namespace GPlatesMaths
{
	namespace Centroid
	{
		/**
		 * Returns the sum of the sequence of @a PointOnSphere objects.
		 *
		 * NOTE: Returned vector could be zero length if points sum to the zero vector.
		 */
		template <typename PointForwardIter>
		Vector3D
		calculate_sum_points(
				PointForwardIter begin,
				PointForwardIter end);

		/**
		 * Returns the sum of the sequence of @a UnitVector3D objects.
		 *
		 * NOTE: Returned vector could be zero length if points sum to the zero vector.
		 */
		template <typename UnitVector3DForwardIter>
		Vector3D
		calculate_sum_vertices(
				UnitVector3DForwardIter begin,
				UnitVector3DForwardIter end);


		/**
		 * Calculates the centroid of the sequence of @a UnitVector3D objects.
		 *
		 * NOTE: Returns the first point if the points sum to the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename UnitVector3DForwardIter>
		UnitVector3D
		calculate_vertices_centroid(
				UnitVector3DForwardIter begin,
				UnitVector3DForwardIter end);


		/**
		 * Calculates the centroid of the sequence of @a PointOnSphere objects.
		 *
		 * NOTE: Returns the first point if the points sum to the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename PointForwardIter>
		UnitVector3D
		calculate_points_centroid(
				PointForwardIter begin,
				PointForwardIter end);

		/**
		 * Calculates the centroid of @a point - which is just @a point.
		 *
		 * This is here only for completeness for generic (template) clients.
		 */
		inline
		UnitVector3D
		calculate_points_centroid(
				const PointOnSphere &point)
		{
			return point.position_vector();
		}

		/**
		 * Calculates the centroid of the points in @a multi_point.
		 *
		 * NOTE: Returns first point if centroid cannot be determined because points sum to the zero vector.
		 */
		inline
		UnitVector3D
		calculate_points_centroid(
				const MultiPointOnSphere &multi_point)
		{
			return calculate_points_centroid(multi_point.begin(), multi_point.end());
		}

		/**
		 * Calculates the centroid of the points in @a polyline.
		 *
		 * Note that a better centroid calculation for polylines is @a calculate_outline_centroid.
		 *
		 * NOTE: Returns the first point of the first arc if the calculated centroid is the zero vector.
		 */
		inline
		UnitVector3D
		calculate_points_centroid(
				const PolylineOnSphere &polyline)
		{
			return calculate_points_centroid(polyline.vertex_begin(), polyline.vertex_end());
		}

		/**
		 * Calculates the centroid of the points in @a polygon.
		 *
		 * Note that a better centroid calculation for polygons is @a calculate_outline_centroid or
		 * @a calculate_interior_centroid.
		 *
		 * NOTE: Returns the first point of the first arc if the calculated centroid is the zero vector.
		 */
		inline
		UnitVector3D
		calculate_points_centroid(
				const PolygonOnSphere &polygon)
		{
			return calculate_points_centroid(polygon.vertex_begin(), polygon.vertex_end());
		}


		/**
		 * Calculates the centroid of a sequence of @a GreatCircleArc objects using
		 * an approximate arc-length weighted average of the arc centroids.
		 *
		 * This generally produces a better centroid for bounding polylines and polygons
		 * (with a bounding small circle) than @a calculate_points_centroid.
		 *
		 * NOTE: Returns the first point of the first arc if the weighted average centroid is the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename EdgeForwardIter>
		UnitVector3D
		calculate_outline_centroid(
				EdgeForwardIter begin,
				EdgeForwardIter end);

		/**
		 * Calculates the centroid of the great circle arc edges in @a polyline.
		 *
		 * NOTE: Returns the first point of the first arc if the weighted average centroid is the zero vector.
		 */
		inline
		UnitVector3D
		calculate_outline_centroid(
				const PolylineOnSphere &polyline)
		{
			return calculate_outline_centroid(polyline.begin(), polyline.end());
		}

		/**
		 * Calculates the centroid of the great circle arc edges in @a polygon.
		 *
		 * This calculates a centroid that is more suitable for a bounding small circle than
		 * @a calculate_interior_centroid - in other words the bounding small circle will generally
		 * be a tighter fit.
		 *
		 * NOTE: Returns the first point of the first arc if the weighted average centroid is the zero vector.
		 */
		inline
		UnitVector3D
		calculate_outline_centroid(
				const PolygonOnSphere &polygon)
		{
			return calculate_outline_centroid(polygon.begin(), polygon.end());
		}


		/**
		 * Calculates the centroid of a sequence of @a GreatCircleArc objects using
		 * spherical area weighting.
		 *
		 * This centroid can be considered a centre-of-mass type of centroid since it considers
		 * the sequence of edges as a contiguous boundary with an interior region and the calculated
		 * centroid is weighted according to the area coverage of the interior region.
		 * For example a bottom-heavy pear-shaped polygon will have a spherical centroid closer
		 * to the bottom whereas a the outline centroid (see @a calculate_outline_centroid) will
		 * be closer to the middle of the pear.
		 *
		 * NOTE: Returns the first point of the first arc if the area weighted triangle centroids
		 * sum to the zero vector.
		 *
		 * @throws @a PreconditionViolationError if @a begin equals @a end.
		 */
		template <typename EdgeForwardIter>
		UnitVector3D
		calculate_interior_centroid(
				EdgeForwardIter begin,
				EdgeForwardIter end);

		/**
		 * Calculates the centroid of @a polygon using spherical area weighting.
		 *
		 * NOTE: Returns the first point of the first arc if centroid cannot be determined because
		 * area weighted triangle centroids sum to the zero vector.
		 */
		inline
		UnitVector3D
		calculate_interior_centroid(
				const PolygonOnSphere &polygon)
		{
			return calculate_interior_centroid(polygon.begin(), polygon.end());
		}
	}
}


//
// Implementation
//


namespace GPlatesMaths
{
	namespace Centroid
	{
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
			Vector3D summed_points_position(0,0,0);

			int num_vertices = 0;
			for (PointForwardIter points_iter = points_begin;
				points_iter != points_end;
				++points_iter, ++num_vertices)
			{
				const PointOnSphere &point = *points_iter;

				const Vector3D vertex(point.position_vector());
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
			Vector3D summed_points_position(0,0,0);

			int num_vertices = 0;
			for (UnitVector3DForwardIter points_iter = points_begin;
				points_iter != points_end;
				++points_iter, ++num_vertices)
			{
				const UnitVector3D &point = *points_iter;

				const Vector3D vertex(point);
				summed_points_position = summed_points_position + vertex;
			}

			return summed_points_position;
		}

		template <typename UnitVector3DForwardIter>
		UnitVector3D
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

		template <typename PointForwardIter>
		UnitVector3D
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

		template <typename EdgeForwardIter>
		UnitVector3D
		calculate_outline_centroid(
				EdgeForwardIter edges_begin,
				EdgeForwardIter edges_end)
		{
			//PROFILE_FUNC();

			Vector3D arc_length_weighted_centroid(0,0,0);

			// Iterate over the arc edges and accumulate approximate arc-length-weighted edge centroids.
			for (EdgeForwardIter edges_iter = edges_begin; edges_iter != edges_end; ++edges_iter)
			{
				const GreatCircleArc &edge = *edges_iter;

				const double &cosine_arc_length = edge.dot_of_endpoints().dval();
				// This is a faster approximation of "acos" (inverse cosine).
				// We don't really need too much accuracy here because we're not doing a proper line integral.
				// So the faster version of 'acos' is fine.
				const double approx_arc_length = std::acos(cosine_arc_length);

				const Vector3D edge_centroid = 0.5 * (
						Vector3D(edge.start_point().position_vector()) +
							Vector3D(edge.end_point().position_vector()));
				// For edge arcs subtending a small enough angle we don't need to normalise the
				// average of the edge end points (saving us an inverse square root calculation).
				// At 45 degrees the length of the average of the edge end points is
				// approx 1 (it's 0.989) which is already almost normalised.
				const double edge_weight = (approx_arc_length > 0.5 * HALF_PI)
						? approx_arc_length / edge_centroid.magnitude().dval()
						: approx_arc_length;

				// Our approximation to the (poly) line integral.
				// It should be independent of the tessellation of the edges but it's not.
				// In other words you should be able to keep the same edges but just divide them up
				// more finely and still get the same centroid.
				// It's still better than just summing the endpoints.
				arc_length_weighted_centroid = arc_length_weighted_centroid +
						edge_weight * edge_centroid;
			}

			if (arc_length_weighted_centroid.magSqrd() <= 0)
			{
				// Just return the first point of the first arc - this is obviously not very good but it
				// alleviates the caller from having to check an error code or catch an exception.
				// Also it's extremely unlikely to happen.
				// And even when 'arc_length_weighted_centroid.magSqrd()' is *very* close to zero
				// but passes the test then the returned centroid is essentially random.
				// TODO: Implement a more robust alternative for those clients that require
				// an accurate centroid all the time - for most uses in GPlates the worst
				// that happens is a small circle bounding some geometry (with centroid used
				// as small circle centre) becomes larger than it would normally be resulting
				// in less efficient intersection tests.
				return edges_begin->start_point().position_vector();
			}

			return arc_length_weighted_centroid.get_normalisation();
		}

		template <typename EdgeForwardIter>
		UnitVector3D
		calculate_interior_centroid(
				EdgeForwardIter edges_begin,
				EdgeForwardIter edges_end)
		{
			//PROFILE_FUNC();

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					edges_begin != edges_end,
					GPLATES_ASSERTION_SOURCE);

			//
			// First iterate through the points and calculate the sum of vertex positions.
			// This gives us a *rough* centroid from which to base our triangle area calculations.
			// We use the fastest centroid calculation which is over points instead of edges.
			//

			Vector3D summed_point_position(0,0,0);

			// Add the first point on the first edge.
			summed_point_position = summed_point_position +
					Vector3D(edges_begin->start_point().position_vector());

			EdgeForwardIter edges_iter = edges_begin;
			for ( ; edges_iter != edges_end; ++edges_iter)
			{
				const PointOnSphere &point = edges_iter->end_point();

				const Vector3D vertex(point.position_vector());
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

				real_t triangle_area;

				const GreatCircleArc::ConstructionParameterValidity centroid_to_start_point_validity =
						GreatCircleArc::evaluate_construction_parameter_validity(
								points_centroid,
								edge.start_point());
				const GreatCircleArc::ConstructionParameterValidity end_point_to_centroid_validity =
						GreatCircleArc::evaluate_construction_parameter_validity(
								edge.end_point(),
								points_centroid);

				// Detect and handle case where an arc end point is antipodal with respect to the centroid.
				if (centroid_to_start_point_validity == GreatCircleArc::VALID &&
					end_point_to_centroid_validity == GreatCircleArc::VALID)
				{
					const GreatCircleArc start_edge =
							GreatCircleArc::create(
									points_centroid,
									edge.start_point(),
									// No need to check construction parameter validity - we've already done so...
									false/*check_validity*/);
					const GreatCircleArc end_edge =
							GreatCircleArc::create(
									edge.end_point(),
									points_centroid,
									// No need to check construction parameter validity - we've already done so...
									false/*check_validity*/);

					// Returns zero area if any triangles edges are zero length...
					triangle_area = SphericalArea::calculate_spherical_triangle_signed_area(
							start_edge, edge, end_edge);
				}
				else // One (or both) edges (to centroid) is invalid (antipodal)...
				{
					// If polygon edge is zero length then both edge end points are antipodal to the
					// centroid, but the triangle area will be zero anyway so just skip it.
					if (edge.is_zero_length())
					{
						continue;
					}

					// Rotate the polygon centroid slightly so that's it's no longer antipodal.
					// This will introduce a small error to the final polygon area though.
					// An angle of 1e-4 radians equates to a dot product (cosine) deviation of 5e-9 which is
					// less than the 1e-12 epsilon used in dot product to determine if two points are antipodal.
					// So this should be enough to prevent the same thing happening again.
					// Note that it's still possible that the *other* edge end point is now antipodal
					// to the rotated centroid - to avoid this we rotate the centroid *away* from the
					// antipodal edge such that it cannot lie on the antipodal arc.
					const Rotation centroid_rotation =
							Rotation::create(
									edge.rotation_axis(),
									(centroid_to_start_point_validity == GreatCircleArc::VALID) ? 1e-4 : -1e-4);
					const PointOnSphere rotated_points_centroid(centroid_rotation * points_centroid);

					const GreatCircleArc start_edge =
							GreatCircleArc::create(rotated_points_centroid, edge.start_point());

					const GreatCircleArc end_edge =
							GreatCircleArc::create(edge.end_point(), rotated_points_centroid);

					// Returns zero area if any triangles edges are zero length...
					triangle_area = SphericalArea::calculate_spherical_triangle_signed_area(
							start_edge, edge, end_edge);
				}

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
				// Just return the first point of the first arc - this is obviously not very good but it
				// alleviates the caller from having to check an error code or catch an exception.
				// Also it's extremely unlikely to happen.
				// And even when 'area_weighted_centroid.magSqrd()' is *very* close to zero
				// but passes the test then the returned centroid is essentially random.
				// TODO: Implement a more robust alternative for those clients that require
				// an accurate centroid all the time - for most uses in GPlates the worst
				// that happens is a small circle bounding some geometry (with centroid used
				// as small circle centre) becomes larger than it would normally be resulting
				// in less efficient intersection tests.
				return edges_begin->start_point().position_vector();
			}

			// A negative polygon total area means the polygon orientation is clockwise which
			// means the triangle-area-weighted centroid will be on the opposite side of the globe
			// from the polygon, so we negate it to bring it onto the same side.
			if (total_area.is_precisely_less_than(0))
			{
				area_weighted_centroid = -area_weighted_centroid;
			}

			return area_weighted_centroid.get_normalisation();
		}
	}
}

#endif // GPLATES_MATHS_CENTROID_H
