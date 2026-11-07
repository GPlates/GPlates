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

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <QDir>
#include <QString>
#include <QStringList>
#include <gtest/gtest.h>

#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "maths/GeometryIntersect.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/SmallCircleBounds.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"

#include "utils/UnicodeStringUtils.h"

#include "unit-test/PolygonOverlayOracle.h"


// Real data (phase4b-design.md sections 10.3 and 11.4), not committed: set
// GPLATES_POLYGON_OVERLAY_REAL_DATA to a plate model's directory, laid out as the plate model
// manager lays it out (Rotations/, Topologies/ and Coastlines/), and optionally
// GPLATES_POLYGON_OVERLAY_REAL_DATA_TIMES to the reconstruction times ("0 100 200" by default).
//
// At each time the coastlines' polygons are reconstructed, then cut through the cascade of the
// topologies resolved at that time as Assign Plate IDs cuts them (GeometryCookieCutter's order, the
// partitioner's bounding-circle shortcut and result rule), but by the overlay itself, so that each
// cut's diagnostics and outputs can be read. Expected: no InconsistentOverlayException, no repair,
// the bundle cap never hit, and every cut's outputs right by the oracle (section 11.3). Reported:
// those counts, and, for the sliver filter's decision (section 4.5), a histogram of the mean widths
// of the output rings of every polygon that was cut, with the counts under h, 10 h and 100 h, the
// thinnest rings, and the output edges shorter than h.
namespace
{
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	typedef std::vector<PointOnSphere> ring_type;

	const double H =
			GPlatesMaths::GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();


	//! The files in @a directory matching @a filters, read as feature collections.
	std::vector<GPlatesFileIO::File::non_null_ptr_type>
	read_files(
			GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			const QString &directory,
			const QStringList &filters)
	{
		std::vector<GPlatesFileIO::File::non_null_ptr_type> files;
		const QDir dir(directory);
		for (const QString &name : dir.entryList(filters, QDir::Files, QDir::Name))
		{
			GPlatesFileIO::File::non_null_ptr_type file =
					GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(dir.filePath(name)));
			GPlatesFileIO::ReadErrorAccumulation read_errors;
			registry.read_feature_collection(file->get_reference(), read_errors);
			files.push_back(file);
		}
		return files;
	}

	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
	collections_of(
			const std::vector<GPlatesFileIO::File::non_null_ptr_type> &files)
	{
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> collections;
		for (const GPlatesFileIO::File::non_null_ptr_type &file : files)
		{
			collections.push_back(file->get_reference().get_feature_collection());
		}
		return collections;
	}

	//! As PolygonPartitioner's shortcut (phase4b-design.md section 4.1 as built).
	bool
	is_outside_by_bounds(
			const PolygonOnSphere &subject,
			const PolygonOnSphere &clip)
	{
		const GPlatesMaths::BoundingSmallCircle &subject_bounds =
				subject.get_bounding_small_circle();
		const GPlatesMaths::BoundingSmallCircle &clip_bounds = clip.get_bounding_small_circle();
		if (GPlatesMaths::intersect(subject_bounds, clip_bounds))
		{
			return false;
		}
		return clip.is_outside_bounding_small_circle_outside_polygon(clip_bounds) &&
				subject.is_outside_bounding_small_circle_outside_polygon(subject_bounds);
	}

	PolygonOnSphere::non_null_ptr_to_const_type
	create_polygon(
			const Overlay::OutputPolygon &output)
	{
		std::vector<ring_type> interiors;
		for (const Overlay::OutputRing &interior : output.interiors)
		{
			interiors.push_back(interior.points);
		}
		return PolygonOnSphere::create(output.exterior.points, interiors.begin(), interiors.end());
	}

	std::vector<ring_type>
	rings_of(
			const PolygonOnSphere &polygon)
	{
		std::vector<ring_type> rings{ ring_type(
				polygon.exterior_ring_vertex_begin(),
				polygon.exterior_ring_vertex_end()) };
		for (unsigned int i = 0; i < polygon.number_of_interior_rings(); ++i)
		{
			rings.push_back(ring_type(
					polygon.interior_ring_vertex_begin(i),
					polygon.interior_ring_vertex_end(i)));
		}
		return rings;
	}


	//! What the run counts, per time and in total.
	class Counts
	{
	public:
		unsigned int polygons = 0;
		unsigned int polygons_cut = 0;
		unsigned int pieces = 0;          // of the polygons cut
		unsigned int overlays = 0;        // cuts past the bounding-circle shortcut
		unsigned int intersecting = 0;
		unsigned int exceptions = 0;
		unsigned int oracle_failures = 0; // cuts with any sample, or the area sum, wrong
		unsigned int not_checked_as_polygons = 0;  // cuts with an input whose rings cross

		// From the overlays' diagnostics.
		unsigned int vertex_tests = 0;
		unsigned int midpoint_tests = 0;
		unsigned int check_failures_before_repair = 0;
		unsigned int repairs = 0;
		unsigned int bundle_vertices = 0;
		unsigned int bundle_capped = 0;
		unsigned int bundle_ambiguous = 0;
		unsigned int pairing_failures = 0;
		unsigned int grouping_failures = 0;
		unsigned int hemisphere_check_failures = 0;
		unsigned int collapsed_rings_dropped = 0;
		unsigned int max_ends = 0;

		void
		add(
				const Overlay::Diagnostics &d)
		{
			vertex_tests += d.vertex_tests;
			midpoint_tests += d.midpoint_tests;
			check_failures_before_repair += d.check_failures_before_repair;
			repairs += d.repairs;
			bundle_vertices += d.bundle_vertices;
			bundle_capped += d.bundle_capped;
			bundle_ambiguous += d.bundle_ambiguous;
			pairing_failures += d.pairing_failures;
			grouping_failures += d.grouping_failures;
			hemisphere_check_failures += d.hemisphere_check_failures;
			collapsed_rings_dropped += d.collapsed_rings_dropped;
			max_ends = (std::max)(max_ends, d.max_ends);
		}

		void
		add(
				const Counts &c)
		{
			polygons += c.polygons;
			polygons_cut += c.polygons_cut;
			pieces += c.pieces;
			overlays += c.overlays;
			intersecting += c.intersecting;
			exceptions += c.exceptions;
			oracle_failures += c.oracle_failures;
			not_checked_as_polygons += c.not_checked_as_polygons;
			vertex_tests += c.vertex_tests;
			midpoint_tests += c.midpoint_tests;
			check_failures_before_repair += c.check_failures_before_repair;
			repairs += c.repairs;
			bundle_vertices += c.bundle_vertices;
			bundle_capped += c.bundle_capped;
			bundle_ambiguous += c.bundle_ambiguous;
			pairing_failures += c.pairing_failures;
			grouping_failures += c.grouping_failures;
			hemisphere_check_failures += c.hemisphere_check_failures;
			collapsed_rings_dropped += c.collapsed_rings_dropped;
			max_ends = (std::max)(max_ends, c.max_ends);
		}

		std::string
		report() const
		{
			std::stringstream out;
			out << polygons << " polygons, " << polygons_cut << " cut into " << pieces
					<< " pieces; " << overlays << " overlays past the bounding circles, "
					<< intersecting << " intersecting; " << exceptions << " exceptions, "
					<< oracle_failures << " oracle failures (" << not_checked_as_polygons
					<< " cuts with crossing input rings checked as rings only); " << vertex_tests
					<< " vertex tests, " << midpoint_tests << " midpoint tests, "
					<< check_failures_before_repair << " check failures before repair, " << repairs
					<< " repairs; " << bundle_vertices << " bundle vertices, " << bundle_capped
					<< " capped, " << bundle_ambiguous << " ambiguous; most ends at a vertex "
					<< max_ends << "; " << pairing_failures << " pairing failures, "
					<< grouping_failures << " grouping failures, " << hemisphere_check_failures
					<< " hemisphere check failures, " << collapsed_rings_dropped
					<< " collapsed rings dropped";
			return out.str();
		}
	};


	//! An output ring of a polygon that was cut, for the width histogram.
	class Ring
	{
	public:
		double width;
		double area;
		unsigned int num_points;
		double time;
		std::string feature_id;
		bool exterior;
	};


	//! Cut @a subject by @a clip with the overlay, as PolygonPartitioner does; check and count.
	void
	cut(
			const PolygonOnSphere::non_null_ptr_to_const_type &subject,
			const PolygonOnSphere &clip,
			bool input_rings_cross,
			std::vector<PolygonOnSphere::non_null_ptr_to_const_type> &inside,
			std::vector<PolygonOnSphere::non_null_ptr_to_const_type> &outside,
			bool &was_cut,
			Counts &counts)
	{
		was_cut = false;
		if (is_outside_by_bounds(*subject, clip))
		{
			outside.push_back(subject);
			return;
		}

		++counts.overlays;
		Overlay::Overlay overlay(*subject, clip);
		std::vector<Overlay::OutputPolygon> intersection, difference;
		Overlay::Result result;
		try
		{
			result = overlay.classify();
			if (result == Overlay::RESULT_INTERSECTING)
			{
				overlay.extract(Overlay::INTERSECTION, intersection);
				overlay.extract(Overlay::DIFFERENCE, difference);
			}
		}
		catch (const Overlay::InconsistentOverlayException &exception)
		{
			++counts.exceptions;
			counts.add(exception.get_diagnostics());
			exception.write(std::cout);
			std::cout << std::endl;
			outside.push_back(subject);
			return;
		}
		counts.add(overlay.get_diagnostics());
		if (result != Overlay::RESULT_INTERSECTING)
		{
			(result == Overlay::RESULT_INSIDE ? inside : outside).push_back(subject);
			return;
		}
		++counts.intersecting;

		// The oracle (section 11.3): samples away from both outlines, read by the outputs as
		// polygons, or as rings where an input's rings cross; and the area sum where they don't.
		const std::vector<PointOnSphere> samples =
				PolygonOverlayOracle::get_samples(*subject, clip, overlay);
		unsigned int failures = 0;
		for (Overlay::Operation operation : { Overlay::INTERSECTION, Overlay::DIFFERENCE })
		{
			if (input_rings_cross)
			{
				std::vector<Overlay::OutputRing> rings;
				overlay.extract_rings(operation, rings);
				failures += PolygonOverlayOracle::count_failures(
						*subject, clip, operation, rings, samples);
			}
			else
			{
				failures += PolygonOverlayOracle::count_failures(
						*subject,
						clip,
						operation,
						operation == Overlay::INTERSECTION ? intersection : difference,
						samples);
			}
		}
		if (input_rings_cross)
		{
			++counts.not_checked_as_polygons;
		}
		else if (PolygonOverlayOracle::area_sum_error(*subject, overlay) >
				PolygonOverlayOracle::area_sum_tolerance(*subject, clip))
		{
			++failures;
		}
		if (failures != 0)
		{
			++counts.oracle_failures;
			std::cout << "oracle: " << failures << " failures of " << samples.size() << " samples"
					<< std::endl;
		}

		// The result after clean-up (section 5.3).
		if (intersection.empty())
		{
			outside.push_back(subject);
			return;
		}
		if (difference.empty())
		{
			inside.push_back(subject);
			return;
		}
		was_cut = true;
		for (const Overlay::OutputPolygon &output : intersection)
		{
			inside.push_back(create_polygon(output));
		}
		for (const Overlay::OutputPolygon &output : difference)
		{
			outside.push_back(create_polygon(output));
		}
	}
}


TEST(PolygonOverlayRealDataTest, Coastlines)
{
	const char *const directory = std::getenv("GPLATES_POLYGON_OVERLAY_REAL_DATA");
	if (!directory)
	{
		GTEST_SKIP() << "GPLATES_POLYGON_OVERLAY_REAL_DATA is not set";
	}
	std::vector<double> times{ 0, 100, 200 };
	if (const char *const times_string = std::getenv("GPLATES_POLYGON_OVERLAY_REAL_DATA_TIMES"))
	{
		times.clear();
		std::istringstream in(times_string);
		for (double time; in >> time; )
		{
			times.push_back(time);
		}
	}

	GPlatesFileIO::FeatureCollectionFileFormat::Registry registry;
	const QString model(directory);
	const auto rotation_files =
			read_files(registry, model + "/Rotations", QStringList{ "*.rot" });
	const auto topology_files =
			read_files(registry, model + "/Topologies", QStringList{ "*.gpml" });
	const auto coastline_files =
			read_files(registry, model + "/Coastlines", QStringList{ "*.gpml", "*.gpmlz" });
	ASSERT_FALSE(rotation_files.empty());
	ASSERT_FALSE(topology_files.empty());
	ASSERT_FALSE(coastline_files.empty());

	const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
			GPlatesAppLogic::create_cached_reconstruction_tree_creator(
					collections_of(rotation_files));
	const GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;

	Counts total;
	std::vector<Ring> rings;
	double area_cut = 0.0;  // of the polygons cut

	// Edge lengths of the cut polygons (input) and of their pieces (output): under 1e-9 radians
	// (rounding: two distinct vertices within a crossing's rounding, PLAN.md 2026-11-04), under
	// h / 10, and under h.
	const double edge_thresholds[] = { 1e-9, H / 10, H };
	unsigned int num_edges[2] = { 0, 0 };
	unsigned int num_short_edges[2][3] = { { 0, 0, 0 }, { 0, 0, 0 } };
	const auto count_edges = [&](const ring_type &ring, unsigned int output)
	{
		for (unsigned int i = 0; i < ring.size(); ++i)
		{
			const double length = PolygonOverlayOracle::angle_between(
					ring[i].position_vector(), ring[(i + 1) % ring.size()].position_vector());
			++num_edges[output];
			for (unsigned int t = 0; t < 3; ++t)
			{
				num_short_edges[output][t] += (length < edge_thresholds[t]) ? 1 : 0;
			}
		}
	};
	for (double time : times)
	{
		// As pyGPlates' PlatePartitioner builds it: networks, then boundaries, each by plate ID.
		const GPlatesAppLogic::GeometryCookieCutter cookie_cutter(
				time,
				reconstruct_method_registry,
				collections_of(topology_files),
				reconstruction_tree_creator);
		const std::vector<PolygonOnSphere::non_null_ptr_to_const_type> clips =
				cookie_cutter.get_partitioning_polygons();
		std::map<const PolygonOnSphere *, bool> clip_rings_cross;

		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed;
		GPlatesAppLogic::ReconstructUtils::reconstruct(
				reconstructed,
				time,
				reconstruct_method_registry,
				collections_of(coastline_files),
				reconstruction_tree_creator);

		Counts counts;
		for (const auto &rfg : reconstructed)
		{
			const boost::optional<PolygonOnSphere::non_null_ptr_to_const_type> polygon =
					GPlatesAppLogic::GeometryUtils::get_polygon_on_sphere(
							*rfg->reconstructed_geometry());
			if (!polygon)
			{
				continue;
			}
			++counts.polygons;
			const bool subject_rings_cross = PolygonOverlayOracle::rings_cross(**polygon);

			bool any_cut = false;
			std::vector<PolygonOnSphere::non_null_ptr_to_const_type> pieces;
			std::vector<PolygonOnSphere::non_null_ptr_to_const_type> remaining{ *polygon };
			for (const PolygonOnSphere::non_null_ptr_to_const_type &clip : clips)
			{
				if (remaining.empty())
				{
					break;
				}
				const auto crosses = clip_rings_cross.find(clip.get());
				const bool clip_crosses = (crosses != clip_rings_cross.end())
						? crosses->second
						: (clip_rings_cross[clip.get()] = PolygonOverlayOracle::rings_cross(*clip));
				std::vector<PolygonOnSphere::non_null_ptr_to_const_type> next_remaining;
				for (const PolygonOnSphere::non_null_ptr_to_const_type &piece : remaining)
				{
					bool was_cut = false;
					cut(
							piece,
							*clip,
							subject_rings_cross || clip_crosses,
							pieces,
							next_remaining,
							was_cut,
							counts);
					any_cut = any_cut || was_cut;
				}
				remaining.swap(next_remaining);
			}
			pieces.insert(pieces.end(), remaining.begin(), remaining.end());
			if (!any_cut)
			{
				continue;
			}

			++counts.polygons_cut;
			counts.pieces += pieces.size();
			for (const ring_type &ring : rings_of(**polygon))
			{
				count_edges(ring, 0);
			}
			// The polygon's area: its exterior ring's less its holes'.
			const std::vector<ring_type> polygon_rings = rings_of(**polygon);
			for (unsigned int r = 0; r < polygon_rings.size(); ++r)
			{
				double ring_area = 0.0;
				Overlay::get_mean_width(polygon_rings[r], &ring_area);
				area_cut += (r == 0) ? ring_area : -ring_area;
			}
			const GPlatesModel::FeatureHandle::weak_ref feature = rfg->get_feature_ref();
			const std::string feature_id = feature.is_valid()
					? GPlatesUtils::make_qstring_from_icu_string(feature->feature_id().get())
							.toStdString()
					: std::string();
			for (const PolygonOnSphere::non_null_ptr_to_const_type &piece : pieces)
			{
				const std::vector<ring_type> piece_rings = rings_of(*piece);
				for (unsigned int r = 0; r < piece_rings.size(); ++r)
				{
					double area = 0.0;
					const double width = Overlay::get_mean_width(piece_rings[r], &area);
					rings.push_back(Ring{
							width,
							area,
							unsigned(piece_rings[r].size()),
							time,
							feature_id,
							r == 0 });
					count_edges(piece_rings[r], 1);
				}
			}
		}
		std::cout << "time " << time << ": " << clips.size() << " partitioning polygons; "
				<< counts.report() << std::endl;
		total.add(counts);
	}
	std::cout << "total: " << total.report() << std::endl;

	// The sliver measurement (section 4.5): mean widths in thicknesses, by half decade.
	const double earth_radius = 6371.0e3;
	std::sort(
			rings.begin(),
			rings.end(),
			[](const Ring &a, const Ring &b) { return a.width < b.width; });
	const std::vector<double> bin_edges{
			0, 0.1, 0.316, 1, 3.16, 10, 31.6, 100, 316, 1000, 3160, 1e4, 3.16e4, 1e5, 1e300 };
	std::cout << rings.size() << " output rings of the polygons cut; mean width in h (h = " << H
			<< " radians, about " << H * earth_radius << " m):" << std::endl;
	unsigned int ring_index = 0;
	for (unsigned int b = 0; b + 1 < bin_edges.size(); ++b)
	{
		unsigned int count = 0;
		while (ring_index < rings.size() && rings[ring_index].width < bin_edges[b + 1] * H)
		{
			++count;
			++ring_index;
		}
		std::cout << "  [" << bin_edges[b] << ", " << bin_edges[b + 1] << "): " << count
				<< std::endl;
	}
	for (const double candidate : { 1.0, 10.0, 100.0 })
	{
		unsigned int count = 0;
		double area = 0.0;
		for (const Ring &ring : rings)
		{
			if (ring.width < candidate * H)
			{
				++count;
				area += ring.area;
			}
		}
		std::cout << "under " << candidate << " h: " << count << " rings, "
				<< area * earth_radius * earth_radius << " square metres (" << area / area_cut
				<< " of the polygons cut)" << std::endl;
	}
	std::cout << "the thinnest rings (width in h, length in km, area in square metres, points, "
			"time, feature, kind):" << std::endl;
	for (unsigned int i = 0; i < (std::min)(rings.size(), std::size_t(30)); ++i)
	{
		const Ring &ring = rings[i];
		const double length = ring.area / ring.width;  // half the perimeter
		std::cout << "  " << std::setprecision(4) << ring.width / H << "  "
				<< length * earth_radius / 1e3 << "  " << ring.area * earth_radius * earth_radius
				<< "  " << ring.num_points << "  " << ring.time << "  " << ring.feature_id << "  "
				<< (ring.exterior ? "exterior" : "hole") << std::endl;
	}
	for (unsigned int output = 0; output < 2; ++output)
	{
		std::cout << (output ? "output: " : "input (the polygons cut): ") << num_edges[output]
				<< " edges, " << num_short_edges[output][0] << " under 1e-9 radians, "
				<< num_short_edges[output][1] << " under h / 10, " << num_short_edges[output][2]
				<< " under h" << std::endl;
	}

	EXPECT_EQ(total.exceptions, 0u);
	EXPECT_EQ(total.repairs, 0u);
	EXPECT_EQ(total.bundle_capped, 0u);
	EXPECT_EQ(total.oracle_failures, 0u);
}
