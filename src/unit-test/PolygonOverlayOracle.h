/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_UNITTEST_POLYGONOVERLAYORACLE_H
#define GPLATES_UNITTEST_POLYGONOVERLAYORACLE_H

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "maths/GeometryDistance.h"
#include "maths/GeometryIntersect.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/PolygonOrientation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


/**
 * The output oracle of the polygon overlay tests (phase4b-design.md section 11.3): for points well
 * away from both inputs' boundaries, a point is in an operation's output, read by parity over its
 * rings, exactly when f(in subject, in clip). No exact reference is needed, since point-in-polygon
 * was audited separately; the samples keep more than M = max(10h, twice the widest record cluster)
 * from both outlines, so that rounding and the band cannot decide them.
 */
namespace PolygonOverlayOracle
{
	inline
	void
	add_ring_points(
			const GPlatesMaths::PolygonOnSphere &polygon,
			std::vector<GPlatesMaths::PointOnSphere> &points)
	{
		points.insert(
				points.end(),
				polygon.exterior_ring_vertex_begin(),
				polygon.exterior_ring_vertex_end());
		for (unsigned int i = 0; i < polygon.number_of_interior_rings(); ++i)
		{
			points.insert(
					points.end(),
					polygon.interior_ring_vertex_begin(i),
					polygon.interior_ring_vertex_end(i));
		}
	}


	inline
	double
	angle_between(
			const GPlatesMaths::UnitVector3D &a,
			const GPlatesMaths::UnitVector3D &b)
	{
		return std::atan2(cross(a, b).magnitude().dval(), dot(a, b).dval());
	}


	/**
	 * Sample points: uniform in a disc over both inputs, and in annuli (radius M to 30 M) round
	 * every overlay vertex, where the hard regions are; those within M of either outline dropped.
	 */
	inline
	std::vector<GPlatesMaths::PointOnSphere>
	get_samples(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip,
			const GPlatesMaths::PolygonOverlay::Overlay &overlay,
			unsigned int num_global = 300,
			unsigned int num_per_vertex = 12)
	{
		using namespace GPlatesMaths;
		const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();

		std::vector<PointOnSphere> vertices;
		add_ring_points(subject, vertices);
		add_ring_points(clip, vertices);
		Vector3D sum(0, 0, 0);
		for (const PointOnSphere &v : vertices)
		{
			sum = sum + Vector3D(v.position_vector());
		}
		const UnitVector3D centre = sum.get_normalisation();
		double radius = 0;
		for (const PointOnSphere &v : vertices)
		{
			radius = (std::max)(radius, angle_between(centre, v.position_vector()));
		}
		radius = 1.3 * radius + 1e-4;

		// The widest record cluster (the records of one overlay vertex).
		double cluster_width = 0;
		const auto &records = overlay.get_graph().unordered_intersections;
		for (const auto &vertex : overlay.get_vertices())
		{
			const std::vector<unsigned int> vertex_records = overlay.get_vertex_records(vertex);
			for (unsigned int a : vertex_records)
			{
				for (unsigned int b : vertex_records)
				{
					cluster_width = (std::max)(cluster_width, angle_between(
							records[a].position.position_vector(),
							records[b].position.position_vector()));
				}
			}
		}
		const double margin = (std::max)(10 * h, 2 * cluster_width);

		std::mt19937 random(12345);
		std::uniform_real_distribution<double> unit(0.0, 1.0);
		const auto in_frame = [&](const UnitVector3D &c, double r, double a)
		{
			const UnitVector3D east = generate_perpendicular(c);
			const UnitVector3D north = cross(c, east).get_normalisation();
			const double t = std::tan(r);
			const Vector3D offset =
					t * std::cos(a) * Vector3D(east) + t * std::sin(a) * Vector3D(north);
			return PointOnSphere((Vector3D(c) + offset).get_normalisation());
		};

		std::vector<PointOnSphere> candidates;
		for (unsigned int i = 0; i < num_global; ++i)
		{
			const double r = radius * std::sqrt(unit(random));
			candidates.push_back(in_frame(centre, r, 2 * PI * unit(random)));
		}
		for (const auto &vertex : overlay.get_vertices())
		{
			for (unsigned int i = 0; i < num_per_vertex; ++i)
			{
				candidates.push_back(in_frame(
						vertex.position.position_vector(),
						margin * (1 + 29 * unit(random)),
						2 * PI * unit(random)));
			}
		}

		std::vector<PointOnSphere> samples;
		for (const PointOnSphere &p : candidates)
		{
			const boost::optional<AngularDistance> to_subject = minimum_distance(p, subject);
			const boost::optional<AngularDistance> to_clip = minimum_distance(p, clip);
			if (to_subject && to_subject->calculate_angle().dval() > margin &&
				to_clip && to_clip->calculate_angle().dval() > margin)
			{
				samples.push_back(p);
			}
		}
		return samples;
	}


	/**
	 * The number of samples whose membership of the output (parity over @a rings) is not
	 * f(in subject, in clip).
	 */
	inline
	unsigned int
	count_failures(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip,
			GPlatesMaths::PolygonOverlay::Operation operation,
			const std::vector<GPlatesMaths::PolygonOverlay::OutputRing> &rings,
			const std::vector<GPlatesMaths::PointOnSphere> &samples)
	{
		using namespace GPlatesMaths;

		std::vector<PolygonOnSphere::non_null_ptr_to_const_type> ring_polygons;
		for (const PolygonOverlay::OutputRing &ring : rings)
		{
			ring_polygons.push_back(PolygonOnSphere::create(ring.points));
		}

		unsigned int num_failures = 0;
		for (const PointOnSphere &p : samples)
		{
			const bool in_subject =
					subject.is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false);
			const bool in_clip = clip.is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false);
			bool in_output = false;
			for (const auto &ring_polygon : ring_polygons)
			{
				const bool in_ring =
						ring_polygon->is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false);
				in_output = in_output != in_ring;
			}
			if (in_output != PolygonOverlay::is_in_output(operation, in_subject, in_clip))
			{
				++num_failures;
			}
		}
		return num_failures;
	}


	/**
	 * The number of samples whose membership of the output, read as @a polygons (each by parity
	 * over its exterior and holes, as point-in-polygon reads a polygon, and the union over the
	 * polygons), is not f(in subject, in clip). This checks the grouping as well as the rings.
	 */
	inline
	unsigned int
	count_failures(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip,
			GPlatesMaths::PolygonOverlay::Operation operation,
			const std::vector<GPlatesMaths::PolygonOverlay::OutputPolygon> &polygons,
			const std::vector<GPlatesMaths::PointOnSphere> &samples)
	{
		using namespace GPlatesMaths;

		std::vector<PolygonOnSphere::non_null_ptr_to_const_type> output_polygons;
		for (const PolygonOverlay::OutputPolygon &polygon : polygons)
		{
			std::vector<std::vector<PointOnSphere>> interiors;
			for (const PolygonOverlay::OutputRing &interior : polygon.interiors)
			{
				interiors.push_back(interior.points);
			}
			output_polygons.push_back(PolygonOnSphere::create(polygon.exterior.points, interiors));
		}

		unsigned int num_failures = 0;
		for (const PointOnSphere &p : samples)
		{
			const bool in_subject =
					subject.is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false);
			const bool in_clip = clip.is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false);
			unsigned int num_containing = 0;
			for (const auto &output_polygon : output_polygons)
			{
				if (output_polygon->is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false))
				{
					++num_containing;
				}
			}
			// The output polygons are disjoint, so no point is in two of them.
			const bool in_output = PolygonOverlay::is_in_output(operation, in_subject, in_clip);
			if (num_containing > 1 || (num_containing == 1) != in_output)
			{
				++num_failures;
			}
		}
		return num_failures;
	}


	/**
	 * The number of rings of @a polygons not oriented with the output on their left: an exterior
	 * not anticlockwise, or a hole not clockwise. (With the grouping checked by the oracle, the
	 * orientation says which side is the output.)
	 */
	inline
	unsigned int
	count_misoriented_rings(
			const std::vector<GPlatesMaths::PolygonOverlay::OutputPolygon> &polygons)
	{
		using namespace GPlatesMaths;

		const auto anticlockwise = [](const PolygonOverlay::OutputRing &ring)
		{
			return PolygonOrientation::calculate_polygon_orientation(
					*PolygonOnSphere::create(ring.points)) == PolygonOrientation::COUNTERCLOCKWISE;
		};
		unsigned int num_misoriented = 0;
		for (const PolygonOverlay::OutputPolygon &polygon : polygons)
		{
			num_misoriented += anticlockwise(polygon.exterior) ? 0 : 1;
			for (const PolygonOverlay::OutputRing &interior : polygon.interiors)
			{
				num_misoriented += anticlockwise(interior) ? 1 : 0;
			}
		}
		return num_misoriented;
	}
}

#endif // GPLATES_UNITTEST_POLYGONOVERLAYORACLE_H
