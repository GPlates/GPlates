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
		 * If @a include_interior_rings is true then includes points in the interior rings (if any).
		 *
		 * Note that a better centroid calculation for polygons is @a calculate_outline_centroid or
		 * @a calculate_interior_centroid.
		 *
		 * NOTE: Returns the first point of the first arc in the exterior ring if the
		 * calculated centroid is the zero vector.
		 */
		UnitVector3D
		calculate_points_centroid(
				const PolygonOnSphere &polygon,
				bool include_interior_rings = true);


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
		 * If @a use_interior_rings is true then includes arc edges in the interior rings (if any).
		 *
		 * This calculates a centroid that is more suitable for a bounding small circle than
		 * @a calculate_interior_centroid - in other words the bounding small circle will generally
		 * be a tighter fit.
		 *
		 * NOTE: Returns the first point of the exterior ring if the weighted average centroid is the zero vector.
		 */
		UnitVector3D
		calculate_outline_centroid(
				const PolygonOnSphere &polygon,
				bool use_interior_rings = true);

		/**
		 * Calculates the centroid of @a polygon using spherical area weighting.
		 *
		 * This centroid can be considered a centre-of-mass type of centroid since the calculated
		 * centroid is weighted according to the area coverage of the interior region.
		 * For example a bottom-heavy pear-shaped polygon will have an interior centroid closer
		 * to the bottom whereas the outline centroid (see @a calculate_outline_centroid) will
		 * be closer to the middle of the pear.
		 *
		 * If @a use_interior_rings is true then the interior rings (if any) are used in the
		 * calculation (ie, the interior ring holes are excluded from the spherical weighting).
		 *
		 * The interior rings change the spherical area weighting because they are holes in the polygon
		 * and are meant to cutout the internal area.
		 * Note that the orientation of the interior rings can be arbitrary (ie, the interior orientations
		 * are not forced to have the opposite orientation to the exterior ring like some software does)
		 * and they will still correctly affect the spherical weighting.
		 *
		 * NOTE: Returns the first point of the exterior ring if centroid cannot be determined because
		 * area weighted triangle centroids sum to the zero vector.
		 */
		UnitVector3D
		calculate_interior_centroid(
				const PolygonOnSphere &polygon,
				bool use_interior_rings = true);
	}
}


//
// Implementation
//


namespace GPlatesMaths
{
	namespace Centroid
	{
		namespace Implementation
		{
			UnitVector3D
			get_normalised_centroid_or_placeholder_centroid(
					const Vector3D &centroid,
					const UnitVector3D &placeholder_centroid);


			/**
			 * Calculate the sum of centroids of a sequence of @a GreatCircleArc objects using
			 * an approximate arc-length weighting of the arc centroids.
			 */
			template <typename EdgeForwardIter>
			Vector3D
			calculate_sum_arc_length_weighted_centroids(
					EdgeForwardIter edges_begin,
					EdgeForwardIter edges_end)
			{
				Vector3D arc_length_weighted_centroid(0,0,0);

				// Iterate over the arc edges and accumulate approximate arc-length-weighted edge centroids.
				for (EdgeForwardIter edges_iter = edges_begin; edges_iter != edges_end; ++edges_iter)
				{
					const GreatCircleArc &edge = *edges_iter;

					// Note: We use GPlatesMaths::acos instead of std::acos since it's possible the
					// dot product is just outside the range [-1,1] which would result in NaN.
					const double arc_length = acos(edge.dot_of_endpoints()).dval();

					const Vector3D edge_centroid = 0.5 * (
							Vector3D(edge.start_point().position_vector()) +
								Vector3D(edge.end_point().position_vector()));
					// For edge arcs subtending a small enough angle we don't need to normalise the
					// average of the edge end points (saving us an inverse square root calculation).
					// At 45 degrees the length of the average of the edge end points is
					// approx 1 (it's 0.989) which is already almost normalised.
					const double edge_weight = (arc_length > 0.5 * HALF_PI)
							? arc_length / edge_centroid.magnitude().dval()
							: arc_length;

					// Our approximation to the (poly) line integral.
					// It should be independent of the tessellation of the edges but it's not.
					// In other words you should be able to keep the same edges but just divide them up
					// more finely and still get the same centroid.
					// It's still better than just summing the endpoints.
					arc_length_weighted_centroid = arc_length_weighted_centroid + edge_weight * edge_centroid;
				}

				return arc_length_weighted_centroid;
			}
		}


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

			for (PointForwardIter points_iter = points_begin; points_iter != points_end; ++points_iter)
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

			for (UnitVector3DForwardIter points_iter = points_begin; points_iter != points_end; ++points_iter)
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

			const Vector3D summed_points_position = calculate_sum_vertices(points_begin, points_end);

			// If the magnitude of the summed vertex position is zero then all the points averaged
			// to zero and hence we cannot get a centroid point.
			// This most likely happens when the vertices roughly form a great circle arc.
			// If this happens then just return the first point.
			return Implementation::get_normalised_centroid_or_placeholder_centroid(
					summed_points_position,
					*points_begin);
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
			// If this happens then just return the first point.
			return Implementation::get_normalised_centroid_or_placeholder_centroid(
					summed_points_position,
					points_begin->position_vector());
		}

		template <typename EdgeForwardIter>
		UnitVector3D
		calculate_outline_centroid(
				EdgeForwardIter edges_begin,
				EdgeForwardIter edges_end)
		{
			//PROFILE_FUNC();

			const Vector3D arc_length_weighted_centroid =
					Implementation::calculate_sum_arc_length_weighted_centroids(edges_begin, edges_end);

			// If the magnitude is zero then just return the first point of the first arc.
			return Implementation::get_normalised_centroid_or_placeholder_centroid(
					arc_length_weighted_centroid,
					edges_begin->start_point().position_vector());
		}
	}
}

#endif // GPLATES_MATHS_CENTROID_H
