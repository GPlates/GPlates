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
#include "maths/GreatCircleArc.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
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
	 * orientation says which side is the output.) By the sign of the ring's vector area (the sum
	 * of its edges' cross products) along its vertices' mean direction, which is not how the
	 * overlay orients (PolygonOrientation), and is right for a ring within a hemisphere of that
	 * direction, as every test's ring is.
	 */
	inline
	unsigned int
	count_misoriented_rings(
			const std::vector<GPlatesMaths::PolygonOverlay::OutputPolygon> &polygons)
	{
		using namespace GPlatesMaths;

		const auto anticlockwise = [](const PolygonOverlay::OutputRing &ring)
		{
			Vector3D mean(0, 0, 0);
			Vector3D vector_area(0, 0, 0);
			for (unsigned int i = 0; i < ring.points.size(); ++i)
			{
				const UnitVector3D &a = ring.points[i].position_vector();
				const UnitVector3D &b = ring.points[(i + 1) % ring.points.size()].position_vector();
				mean = mean + Vector3D(a);
				vector_area = vector_area + Vector3D(cross(a, b));
			}
			return dot(vector_area, mean).dval() > 0;
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


	//! Whether two points are the same stored position, coordinate for coordinate.
	inline
	bool
	same_position(
			const GPlatesMaths::PointOnSphere &a,
			const GPlatesMaths::PointOnSphere &b)
	{
		return a.position_vector().x().dval() == b.position_vector().x().dval() &&
				a.position_vector().y().dval() == b.position_vector().y().dval() &&
				a.position_vector().z().dval() == b.position_vector().z().dval();
	}


	/**
	 * Whether two segments of a polygon's rings cross (clearly: each segment's ends on opposite
	 * sides of the other's great circle by more than rounding, so that segments meeting at a
	 * vertex or touching do not count). The catalogue's twists cross themselves, and the hunt's
	 * generators move vertices onto the other polygon and place holes near their ring's vertex
	 * centroid, which can reach outside it. Grouping and orientation assume rings that do not
	 * cross (phase4b-design.md sections 4.2 to 4.4), and so does a polygon's area.
	 */
	inline
	bool
	rings_cross(
			const GPlatesMaths::PolygonOnSphere &polygon)
	{
		using namespace GPlatesMaths;

		const std::vector<GreatCircleArc> arcs(polygon.begin(), polygon.end());
		const auto midpoint = [](const GreatCircleArc &arc)
		{
			return Vector3D(arc.start_point().position_vector()) +
					Vector3D(arc.end_point().position_vector());
		};
		// The side of an arc's great circle a point is on, by the arc's own plane normalised in
		// doubles (GreatCircleArc has no rotation axis for an arc under about 1e-6 radians, and the
		// 'chain' hunt's segments are 0.3h to 1.9h long).
		const auto side = [](const GreatCircleArc &arc, const PointOnSphere &point)
		{
			const Vector3D n = cross(
					arc.start_point().position_vector(), arc.end_point().position_vector());
			const double length = std::sqrt(n.magSqrd().dval());
			return length > 0 ? dot(n, Vector3D(point.position_vector())).dval() / length : 0.0;
		};
		const auto opposite = [](double x, double y)
		{
			return (x > 1e-14 && y < -1e-14) || (x < -1e-14 && y > 1e-14);
		};
		for (unsigned int i = 0; i < arcs.size(); ++i)
		{
			for (unsigned int j = i + 1; j < arcs.size(); ++j)
			{
				const GreatCircleArc &a = arcs[i];
				const GreatCircleArc &b = arcs[j];
				if (same_position(a.start_point(), b.start_point()) ||
					same_position(a.start_point(), b.end_point()) ||
					same_position(a.end_point(), b.start_point()) ||
					same_position(a.end_point(), b.end_point()))
				{
					continue;  // two segments meeting at a vertex
				}
				if (opposite(side(a, b.start_point()), side(a, b.end_point())) &&
					opposite(side(b, a.start_point()), side(b, a.end_point())) &&
					// The same crossing of the two great circles, not its antipode.
					dot(midpoint(a), midpoint(b)).dval() > 0)
				{
					return true;
				}
			}
		}
		return false;
	}


	/**
	 * The area of the smaller side of a ring that does not cross itself: a fan of triangles from
	 * the vertices' mean direction, each by Van Oosterom and Strackee's formula, which is accurate
	 * for any short edge. (Not SphericalArea, whose fan drops the triangle of every edge that is
	 * zero length to GreatCircleArc, under about 1e-6 radians.) For a ring within a hemisphere of
	 * that direction, as every test's ring is.
	 */
	inline
	double
	ring_area(
			const std::vector<GPlatesMaths::PointOnSphere> &ring)
	{
		using namespace GPlatesMaths;

		Vector3D sum(0, 0, 0);
		for (const PointOnSphere &point : ring)
		{
			sum = sum + Vector3D(point.position_vector());
		}
		const UnitVector3D c = sum.get_normalisation();
		double area = 0;
		for (unsigned int i = 0; i < ring.size(); ++i)
		{
			const UnitVector3D &a = ring[i].position_vector();
			const UnitVector3D &b = ring[(i + 1) % ring.size()].position_vector();
			area += 2 * std::atan2(
					dot(c, cross(a, b)).dval(),
					1 + dot(c, a).dval() + dot(a, b).dval() + dot(b, c).dval());
		}
		return std::fabs(area);
	}


	/**
	 * The area of a region read by parity over @a rings (which do not cross): each ring's area,
	 * added at an even depth among the rings and subtracted at an odd one, as point-in-polygon
	 * reads the rings (an island in a hole adds again).
	 */
	inline
	double
	parity_area(
			const std::vector<std::vector<GPlatesMaths::PointOnSphere>> &rings)
	{
		using namespace GPlatesMaths;

		std::vector<PolygonOnSphere::non_null_ptr_to_const_type> ring_polygons;
		for (const std::vector<PointOnSphere> &ring : rings)
		{
			ring_polygons.push_back(PolygonOnSphere::create(ring));
		}
		double area = 0;
		for (unsigned int r = 0; r < rings.size(); ++r)
		{
			unsigned int depth = 0;
			for (unsigned int other = 0; other < rings.size(); ++other)
			{
				if (other == r)
				{
					continue;
				}
				// The ring's vertex farthest from the other ring (rings can touch, and a vertex
				// near an edge shorter than about 1e-6 radians is not tested exactly).
				const PointOnSphere *farthest = nullptr;
				double farthest_distance = 0;
				for (const PointOnSphere &point : rings[r])
				{
					const double distance = minimum_distance(point, *ring_polygons[other])
							->calculate_angle().dval();
					if (!farthest || distance > farthest_distance)
					{
						farthest = &point;
						farthest_distance = distance;
					}
				}
				depth += ring_polygons[other]->is_point_in_polygon(
						*farthest, PolygonOnSphere::ADAPTIVE, false) ? 1 : 0;
			}
			area += (depth % 2 == 0 ? 1 : -1) * ring_area(rings[r]);
		}
		return area;
	}


	/**
	 * The oracle's area item (phase4b-design.md section 11.3): the intersection and the
	 * difference partition the subject (every subject piece is kept by exactly one of them), so
	 * their areas sum to the subject's. Returns the relative error, to compare with
	 * @a area_sum_tolerance. For inputs whose rings do not cross.
	 */
	inline
	double
	area_sum_error(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOverlay::Overlay &overlay)
	{
		using namespace GPlatesMaths;

		double area = 0;
		for (PolygonOverlay::Operation operation :
				{ PolygonOverlay::INTERSECTION, PolygonOverlay::DIFFERENCE })
		{
			std::vector<PolygonOverlay::OutputPolygon> polygons;
			overlay.extract(operation, polygons);
			for (const PolygonOverlay::OutputPolygon &polygon : polygons)
			{
				std::vector<std::vector<PointOnSphere>> rings{ polygon.exterior.points };
				for (const PolygonOverlay::OutputRing &interior : polygon.interiors)
				{
					rings.push_back(interior.points);
				}
				area += parity_area(rings);
			}
		}

		std::vector<std::vector<PointOnSphere>> subject_rings{ std::vector<PointOnSphere>(
				subject.exterior_ring_vertex_begin(), subject.exterior_ring_vertex_end()) };
		for (unsigned int i = 0; i < subject.number_of_interior_rings(); ++i)
		{
			subject_rings.push_back(std::vector<PointOnSphere>(
					subject.interior_ring_vertex_begin(i), subject.interior_ring_vertex_end(i)));
		}
		const double subject_area = parity_area(subject_rings);
		return std::fabs(area - subject_area) / subject_area;
	}


	/**
	 * Whether the area item applies to these inputs: their rings do not cross (rings_cross), and
	 * none is degenerate, with an isoperimetric ratio 4 pi A / L^2 under 1e-3: the 'chain' hunt
	 * makes rings whose two lobes, wound opposite ways, meet at a crossing at the rounding level,
	 * which no sign test can tell from a touch, and whose area by any fan then cancels.
	 */
	inline
	bool
	is_area_sum_applicable(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip)
	{
		using namespace GPlatesMaths;

		for (const PolygonOnSphere *polygon : { &subject, &clip })
		{
			if (rings_cross(*polygon))
			{
				return false;
			}
			std::vector<std::vector<PointOnSphere>> rings{ std::vector<PointOnSphere>(
					polygon->exterior_ring_vertex_begin(), polygon->exterior_ring_vertex_end()) };
			for (unsigned int i = 0; i < polygon->number_of_interior_rings(); ++i)
			{
				rings.push_back(std::vector<PointOnSphere>(
						polygon->interior_ring_vertex_begin(i),
						polygon->interior_ring_vertex_end(i)));
			}
			for (const std::vector<PointOnSphere> &ring : rings)
			{
				double length = 0;
				for (unsigned int i = 0; i < ring.size(); ++i)
				{
					length += angle_between(
							ring[i].position_vector(),
							ring[(i + 1) % ring.size()].position_vector());
				}
				if (4 * PI * ring_area(ring) < 1e-3 * length * length)
				{
					return false;
				}
			}
		}
		return true;
	}


	/**
	 * The relative error @a area_sum_error allows: an output ring runs through its vertices'
	 * output positions, up to the thickness h off an input's edge (a record's position, or the
	 * other input's vertex), so the outputs' boundary can be off the subject's by h along it:
	 * 2h times both inputs' perimeters, over the subject's area. (A ring or a hole grouped wrong
	 * is off by its whole area.)
	 */
	inline
	double
	area_sum_tolerance(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip)
	{
		using namespace GPlatesMaths;

		const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
		double perimeter = 0;
		for (const PolygonOnSphere *polygon : { &subject, &clip })
		{
			for (auto arc = polygon->begin(); arc != polygon->end(); ++arc)
			{
				perimeter += angle_between(
						arc->start_point().position_vector(), arc->end_point().position_vector());
			}
		}
		std::vector<std::vector<PointOnSphere>> subject_rings{ std::vector<PointOnSphere>(
				subject.exterior_ring_vertex_begin(), subject.exterior_ring_vertex_end()) };
		for (unsigned int i = 0; i < subject.number_of_interior_rings(); ++i)
		{
			subject_rings.push_back(std::vector<PointOnSphere>(
					subject.interior_ring_vertex_begin(i), subject.interior_ring_vertex_end(i)));
		}
		return 2 * h * perimeter / parity_area(subject_rings);
	}
}

#endif // GPLATES_UNITTEST_POLYGONOVERLAYORACLE_H
