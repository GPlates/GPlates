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
#include "maths/PolylineOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


/**
 * The output oracle of the polygon overlay tests (spherical-geometry.md section 6): for points well
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
	 * Output points of a ring that have no subject location though an edge beside them runs along
	 * the subject's outline: its midpoint within the thickness h of the subject's outline and
	 * farther than 2h from the clip's, so that it is along the subject and not a cut (an edge
	 * within 2h of both is not judged). Only for an overlay with no subject table, where every
	 * point on the subject has its location (a split ring's vertex once lost it).
	 */
	inline
	unsigned int
	count_missing_subject_locations(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip,
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			const GPlatesMaths::PolygonOverlay::provenance_seq_type &provenances)
	{
		using namespace GPlatesMaths;
		const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
		const AngularExtent near = AngularExtent::create_from_angle(h);
		const AngularExtent far = AngularExtent::create_from_angle(2 * h);
		const unsigned int n = points.size();
		std::vector<bool> along_subject(n, false);  // edge i, from point i to point i + 1
		for (unsigned int i = 0; i < n; ++i)
		{
			const Vector3D sum = Vector3D(points[i].position_vector()) +
					Vector3D(points[(i + 1) % n].position_vector());
			if (!(sum.magSqrd().dval() > 0))
			{
				continue;
			}
			const PointOnSphere midpoint(sum.get_normalisation());
			along_subject[i] = minimum_distance(midpoint, subject, false, near) &&
					!minimum_distance(midpoint, clip, false, far);
		}
		unsigned int num_missing = 0;
		for (unsigned int i = 0; i < n && i < provenances.size(); ++i)
		{
			if (!provenances[i].subject && (along_subject[i] || along_subject[(i + n - 1) % n]))
			{
				++num_missing;
			}
		}
		return num_missing;
	}


	//! Whether two points are the same stored position, coordinate for coordinate.
	inline
	bool
	same_position(
			const GPlatesMaths::PointOnSphere &a,
			const GPlatesMaths::PointOnSphere &b)
	{
		return GPlatesMaths::are_exactly_equal(
					a.position_vector().x().dval(), b.position_vector().x().dval()) &&
				GPlatesMaths::are_exactly_equal(
					a.position_vector().y().dval(), b.position_vector().y().dval()) &&
				GPlatesMaths::are_exactly_equal(
					a.position_vector().z().dval(), b.position_vector().z().dval());
	}


	//! The widest record cluster: the largest distance between two records of one overlay vertex.
	inline
	double
	get_cluster_width(
			const GPlatesMaths::PolygonOverlay::Overlay &overlay)
	{
		using namespace GPlatesMaths;
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
		return cluster_width;
	}


	//! The oracle's margin M: max(10h, twice the widest record cluster, the records of a vertex).
	inline
	double
	get_margin(
			const GPlatesMaths::PolygonOverlay::Overlay &overlay)
	{
		using namespace GPlatesMaths;
		const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
		return (std::max)(10 * h, 2 * get_cluster_width(overlay));
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

		const double margin = get_margin(overlay);

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
	 *
	 * A ring whose mean width is at rounding level (under a hundredth of the thickness) has no
	 * orientation, and is not counted: an input that overlaps itself along a stretch (the 'shared'
	 * hunt's dense vertices) bounds rings of three or four points on one great circle, which enclose
	 * no area (35 of 119,198 cases), and which a partitioner's sliver filter drops.
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
		const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
		const auto has_orientation = [h](const PolygonOverlay::OutputRing &ring)
		{
			return PolygonOverlay::get_mean_width(ring.points) >= 0.01 * h;
		};
		unsigned int num_misoriented = 0;
		for (const PolygonOverlay::OutputPolygon &polygon : polygons)
		{
			if (has_orientation(polygon.exterior))
			{
				num_misoriented += anticlockwise(polygon.exterior) ? 0 : 1;
			}
			for (const PolygonOverlay::OutputRing &interior : polygon.interiors)
			{
				if (has_orientation(interior))
				{
					num_misoriented += anticlockwise(interior) ? 1 : 0;
				}
			}
		}
		return num_misoriented;
	}


	/**
	 * The oracle for a polyline subject (section 6): points sampled along it
	 * (spherically), farther than M from the clip's outline, must each lie on an output polyline,
	 * and every output they lie on must have the point's class against the clip (threshold off);
	 * on exactly one, unless the subject passes the point more than once (an opened ring of the
	 * hunt can double back on itself), then on at most as many as it passes. Returns the number
	 * that do not, of @a num_samples drawn; @a num_tested is set to how many were beyond M.
	 */
	inline
	unsigned int
	count_polyline_failures(
			const GPlatesMaths::PolylineOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip,
			const GPlatesMaths::PolygonOverlay::Overlay &overlay,
			const std::vector<GPlatesMaths::PolygonOverlay::OutputPolyline> &polylines,
			unsigned int &num_tested,
			unsigned int num_samples = 300)
	{
		using namespace GPlatesMaths;

		const double margin = get_margin(overlay);
		// An output polyline runs through its vertices' output positions, up to h off the subject
		// (a clip vertex touching it, say), so a sample of the subject lies within h of the output
		// that covers it; and a sample beyond M is farther than 9h from every record, so from any
		// other output.
		const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
		const AngularExtent on_polyline = AngularExtent::create_from_angle(2 * h);
		const AngularExtent margin_extent = AngularExtent::create_from_angle(margin);

		std::vector<PolylineOnSphere::non_null_ptr_to_const_type> outputs;
		for (const PolygonOverlay::OutputPolyline &polyline : polylines)
		{
			if (polyline.points.size() >= 2)
			{
				outputs.push_back(PolylineOnSphere::create(polyline.points));
			}
			else
			{
				outputs.push_back(PolylineOnSphere::create(std::vector<PointOnSphere>{
						polyline.points.front(), polyline.points.front() }));
			}
		}

		std::vector<double> lengths;
		double total = 0;
		for (auto arc = subject.begin(); arc != subject.end(); ++arc)
		{
			lengths.push_back(angle_between(
					arc->start_point().position_vector(), arc->end_point().position_vector()));
			total += lengths.back();
		}

		std::mt19937 random(54321);
		std::uniform_real_distribution<double> unit(0.0, 1.0);
		unsigned int num_failures = 0;
		num_tested = 0;
		for (unsigned int i = 0; i < num_samples; ++i)
		{
			double x = total * unit(random);
			unsigned int k = 0;
			while (k + 1 < lengths.size() && x > lengths[k])
			{
				x -= lengths[k];
				++k;
			}
			const GreatCircleArc &arc = *std::next(subject.begin(), k);
			const double angle = lengths[k];
			if (!(angle > 0))
			{
				continue;
			}
			const double t = (std::min)(1.0, x / angle);
			const Vector3D start(arc.start_point().position_vector());
			const Vector3D end(arc.end_point().position_vector());
			const Vector3D v = (std::sin((1 - t) * angle) / std::sin(angle)) * start +
					(std::sin(t * angle) / std::sin(angle)) * end;
			const PointOnSphere point(v.get_normalisation());
			if (minimum_distance(point, clip, false, margin_extent))
			{
				continue;  // within M of the clip's outline
			}
			++num_tested;
			const bool in_clip = clip.is_point_in_polygon(point, PolygonOnSphere::ADAPTIVE, false);
			unsigned int num_covering = 0;
			bool classes_right = true;
			for (unsigned int r = 0; r < outputs.size(); ++r)
			{
				if (minimum_distance(point, *outputs[r], on_polyline))
				{
					++num_covering;
					classes_right = classes_right && polylines[r].inside == in_clip;
				}
			}
			// The subject's passes near the point: runs of consecutive segments within 2h of it.
			unsigned int num_passes = 0;
			bool previous_near = false;
			for (auto segment = subject.begin(); segment != subject.end(); ++segment)
			{
				const bool near = bool(minimum_distance(
						point, *PolylineOnSphere::create(std::vector<PointOnSphere>{
								segment->start_point(), segment->end_point() }),
						on_polyline));
				num_passes += (near && !previous_near) ? 1 : 0;
				previous_near = near;
			}
			if (num_covering == 0 || num_covering > (std::max)(1u, num_passes) || !classes_right)
			{
				++num_failures;
			}
		}
		return num_failures;
	}


	/**
	 * The point a Location names on a geometry given as its rings' points (one ring for a
	 * polyline, whose last segment does not wrap): a vertex, or the point at the angle fraction
	 * along a segment (GreatCircleArc::point_on_arc's).
	 */
	inline
	GPlatesMaths::PointOnSphere
	resolve_location(
			const std::vector<std::vector<GPlatesMaths::PointOnSphere>> &rings,
			const GPlatesMaths::PolygonOverlay::Location &location)
	{
		using namespace GPlatesMaths;
		const std::vector<PointOnSphere> &ring = rings.at(location.ring);
		if (!location.ratio)
		{
			return ring.at(location.index);
		}
		const PointOnSphere &a = ring.at(location.index);
		const PointOnSphere &b = ring.at((location.index + 1) % ring.size());
		if (same_position(a, b))
		{
			return a;
		}
		return GreatCircleArc::create(a, b).point_on_arc(*location.ratio);
	}


	/**
	 * Output points whose provenance (section 5.4) does not resolve to them: a
	 * subject or clip vertex must be the point exactly when @a exact_vertices (a vertex is output
	 * at its own position), and within @a tolerance otherwise; a segment location within
	 * @a tolerance (an output vertex is at its identity group's position, which can be off the
	 * segment by the group's extent); a point with no subject location (a cut) must have a clip
	 * location. @a clips are the clips by Provenance::clip_index.
	 */
	inline
	unsigned int
	count_provenance_failures(
			const std::vector<std::vector<GPlatesMaths::PointOnSphere>> &subject,
			const std::vector<std::vector<std::vector<GPlatesMaths::PointOnSphere>>> &clips,
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			const GPlatesMaths::PolygonOverlay::provenance_seq_type &provenances,
			double tolerance,
			bool exact_vertices = true)
	{
		using namespace GPlatesMaths;
		if (provenances.size() != points.size())
		{
			return points.size();
		}
		unsigned int num_failures = 0;
		for (unsigned int i = 0; i < points.size(); ++i)
		{
			const PolygonOverlay::Provenance &provenance = provenances[i];
			bool right = provenance.subject || provenance.clip;
			for (const auto &located : {
					std::make_pair(&subject, provenance.subject),
					std::make_pair(
							provenance.clip_index < clips.size()
									? &clips[provenance.clip_index]
									: nullptr,
							provenance.clip) })
			{
				if (!right || !located.second)
				{
					continue;
				}
				if (!located.first)
				{
					right = false;
					continue;
				}
				const PointOnSphere resolved = resolve_location(*located.first, *located.second);
				if (!located.second->ratio && exact_vertices && located.first == &subject)
				{
					right = same_position(resolved, points[i]);
				}
				else
				{
					right = angle_between(
							resolved.position_vector(), points[i].position_vector()) <= tolerance;
				}
			}
			num_failures += right ? 0 : 1;
		}
		return num_failures;
	}


	/**
	 * Whether two segments of a polygon's rings cross (clearly: each segment's ends on opposite
	 * sides of the other's great circle by more than rounding, so that segments meeting at a
	 * vertex or touching do not count). The catalogue's twists cross themselves, and the hunt's
	 * generators move vertices onto the other polygon and place holes near their ring's vertex
	 * centroid, which can reach outside it. Grouping and orientation assume rings that do not
	 * cross (section 5.2), and so does a polygon's area.
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
				// The ring's vertex or edge midpoint farthest from the other ring (rings can touch,
				// and a vertex near an edge shorter than about 1e-6 radians is not tested exactly).
				// Not the vertices alone: a hole can touch its exterior at every one of its
				// vertices (a triangle with a corner at each of three of the exterior's), where
				// each vertex is on the other ring and its test decides nothing.
				std::vector<PointOnSphere> candidates(rings[r]);
				for (unsigned int i = 0; i < rings[r].size(); ++i)
				{
					const Vector3D sum = Vector3D(rings[r][i].position_vector()) +
							Vector3D(rings[r][(i + 1) % rings[r].size()].position_vector());
					if (sum.magSqrd().dval() > 1e-12)
					{
						candidates.push_back(PointOnSphere(sum.get_normalisation()));
					}
				}
				const PointOnSphere *farthest = nullptr;
				double farthest_distance = 0;
				for (const PointOnSphere &point : candidates)
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
	 * The oracle's area item (section 6): the intersection and the
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


	/**
	 * The other three area identities beside @a area_sum_error's, each as its absolute error: the
	 * intersection and the reverse difference make up the clip, the union and the intersection
	 * both inputs, and the symmetric difference is the two differences. Every point of a twisted
	 * or wrongly grouped ring can read right by parity, so that the sampled points pass, and only
	 * an area shows it; the piece comparison cannot see it either (where the overlay and the
	 * prototype both cut wrong, or where the prototype's pieces are band pieces). For inputs whose
	 * rings do not cross (@a is_area_sum_applicable); compare with @a area_identity_tolerance.
	 */
	inline
	std::vector<double>
	area_identity_errors(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &clip,
			const GPlatesMaths::PolygonOverlay::Overlay &overlay)
	{
		using namespace GPlatesMaths;

		const auto rings_of = [](const PolygonOnSphere &polygon)
		{
			std::vector<std::vector<PointOnSphere>> rings{ std::vector<PointOnSphere>(
					polygon.exterior_ring_vertex_begin(), polygon.exterior_ring_vertex_end()) };
			for (unsigned int i = 0; i < polygon.number_of_interior_rings(); ++i)
			{
				rings.push_back(std::vector<PointOnSphere>(
						polygon.interior_ring_vertex_begin(i),
						polygon.interior_ring_vertex_end(i)));
			}
			return rings;
		};
		const auto output_area = [&overlay](PolygonOverlay::Operation operation)
		{
			std::vector<PolygonOverlay::OutputPolygon> polygons;
			overlay.extract(operation, polygons);
			double area = 0;
			for (const PolygonOverlay::OutputPolygon &polygon : polygons)
			{
				std::vector<std::vector<PointOnSphere>> rings{ polygon.exterior.points };
				for (const PolygonOverlay::OutputRing &interior : polygon.interiors)
				{
					rings.push_back(interior.points);
				}
				area += parity_area(rings);
			}
			return area;
		};

		const double subject_area = parity_area(rings_of(subject));
		const double clip_area = parity_area(rings_of(clip));
		const double intersection = output_area(PolygonOverlay::INTERSECTION);
		const double difference = output_area(PolygonOverlay::DIFFERENCE);
		const double reverse_difference = output_area(PolygonOverlay::REVERSE_DIFFERENCE);
		return std::vector<double>{
				std::fabs(intersection + reverse_difference - clip_area),
				std::fabs(
						output_area(PolygonOverlay::UNION) + intersection -
						subject_area - clip_area),
				std::fabs(
						output_area(PolygonOverlay::SYMMETRIC_DIFFERENCE) -
						difference - reverse_difference) };
	}


	/**
	 * The absolute error @a area_identity_errors allows: four thicknesses times both inputs'
	 * perimeters (each output's boundary can be a thickness off the inputs' along them, and an
	 * identity adds two or three outputs), as PolygonOverlayTest's area checks allow.
	 */
	inline
	double
	area_identity_tolerance(
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
		return 4 * h * perimeter;
	}
}

#endif // GPLATES_UNITTEST_POLYGONOVERLAYORACLE_H
