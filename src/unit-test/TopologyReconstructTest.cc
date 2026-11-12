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
#include <vector>
#include <boost/optional.hpp>
#include <gtest/gtest.h>

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/ScalarCoverageTimeSpan.h"
#include "app-logic/TimeSpanUtils.h"
#include "app-logic/TopologyReconstruct.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"

#include "model/FeatureCollectionHandle.h"

#include "property-values/ValueObjectType.h"


//
// Tests of how TopologyReconstruct samples a polygon's rings when it imports a geometry: every
// ring is sampled (exterior first, then each interior ring), so that a scalar coverage, which has
// a scalar for every vertex of every ring, can be interpolated onto the samples.
//
// There are no topologies and no rotations, so the geometry stays where it is; only the sampling
// is under test.
//

namespace
{
	using GPlatesAppLogic::TopologyReconstruct;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;


	PointOnSphere
	point(
			double lat,
			double lon)
	{
		return GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat, lon));
	}


	//! A polygon with a four-vertex exterior ring and a three-vertex interior ring.
	PolygonOnSphere::non_null_ptr_to_const_type
	create_polygon_with_hole()
	{
		const std::vector<PointOnSphere> exterior_ring = {
			point(0, 0), point(0, 40), point(40, 40), point(40, 0) };
		const std::vector<std::vector<PointOnSphere>> interior_rings = {
			{ point(10, 10), point(20, 10), point(10, 20) } };

		return PolygonOnSphere::create(exterior_ring, interior_rings);
	}


	TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type
	create_geometry_time_span(
			const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
			boost::optional<double> max_poly_segment_angular_extent_radians)
	{
		const GPlatesAppLogic::TimeSpanUtils::TimeRange time_range(
				10.0/*begin_time*/, 0.0/*end_time*/, 1.0/*time_increment*/,
				GPlatesAppLogic::TimeSpanUtils::TimeRange::ADJUST_TIME_INCREMENT);

		// No rotation features, so every plate has the identity rotation.
		const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::create_cached_reconstruction_tree_creator(
						std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>());

		const TopologyReconstruct::non_null_ptr_type topology_reconstruct =
				TopologyReconstruct::create(
						time_range,
						TopologyReconstruct::resolved_boundary_time_span_type::create(time_range),
						TopologyReconstruct::resolved_network_time_span_type::create(time_range),
						reconstruction_tree_creator);

		return topology_reconstruct->create_geometry_time_span(
				polygon,
				0/*reconstruction_plate_id*/,
				0.0/*geometry_import_time*/,
				boost::none/*deactivate_points*/,
				max_poly_segment_angular_extent_radians);
	}


	TEST(TopologyReconstructTest, untessellated_polygon_samples_every_ring)
	{
		const PolygonOnSphere::non_null_ptr_to_const_type polygon = create_polygon_with_hole();
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				create_geometry_time_span(polygon, boost::none);

		// Each sample is an original vertex, in the order of the exterior ring then the interior ring.
		const TopologyReconstruct::GeometryTimeSpan::interpolate_original_points_seq_type &
				interpolate_original_points = geometry_time_span->get_interpolate_original_points();
		ASSERT_EQ(interpolate_original_points.size(), 7u);
		for (unsigned int n = 0; n < interpolate_original_points.size(); ++n)
		{
			SCOPED_TRACE(n);
			EXPECT_EQ(interpolate_original_points[n].first_point_index, n);
			EXPECT_EQ(interpolate_original_points[n].second_point_index, n);
			EXPECT_EQ(interpolate_original_points[n].interpolate_ratio, 0.0);
		}

		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry =
				geometry_time_span->get_geometry(0.0);
		ASSERT_TRUE(geometry);
		std::vector<PointOnSphere> sampled_points;
		GPlatesAppLogic::GeometryUtils::get_geometry_points(*geometry.get(), sampled_points);
		std::vector<PointOnSphere> polygon_points;
		GPlatesAppLogic::GeometryUtils::get_geometry_points(*polygon, polygon_points);
		ASSERT_EQ(sampled_points.size(), polygon_points.size());
		for (unsigned int n = 0; n < sampled_points.size(); ++n)
		{
			SCOPED_TRACE(n);
			EXPECT_TRUE(sampled_points[n] == polygon_points[n]);
		}
	}


	TEST(TopologyReconstructTest, tessellated_polygon_interpolates_within_each_ring)
	{
		const PolygonOnSphere::non_null_ptr_to_const_type polygon = create_polygon_with_hole();
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				create_geometry_time_span(polygon, GPlatesMaths::convert_deg_to_rad(2.0));

		const TopologyReconstruct::GeometryTimeSpan::interpolate_original_points_seq_type &
				interpolate_original_points = geometry_time_span->get_interpolate_original_points();
		ASSERT_GT(interpolate_original_points.size(), 7u);

		// Exterior ring vertices are 0 to 3, the interior ring's are 4 to 6. Each sample interpolates
		// along an arc of one ring, from a vertex to the next one in that ring (the last arc of a
		// ring ends at the ring's own first vertex), and each original vertex is sampled exactly.
		std::vector<unsigned int> num_exact_samples(7, 0);
		for (unsigned int n = 0; n < interpolate_original_points.size(); ++n)
		{
			SCOPED_TRACE(n);
			const TopologyReconstruct::GeometryTimeSpan::InterpolateOriginalPoints &
					interpolate_original_point = interpolate_original_points[n];

			const unsigned int first = interpolate_original_point.first_point_index;
			const unsigned int second = interpolate_original_point.second_point_index;
			ASSERT_LT(first, 7u);
			const unsigned int ring_begin = (first < 4) ? 0 : 4;
			const unsigned int ring_end = (first < 4) ? 4 : 7;
			const unsigned int expected_second = (first + 1 == ring_end) ? ring_begin : first + 1;
			EXPECT_EQ(second, expected_second);

			EXPECT_GE(interpolate_original_point.interpolate_ratio, 0.0);
			EXPECT_LT(interpolate_original_point.interpolate_ratio, 1.0);
			if (GPlatesMaths::are_exactly_equal(interpolate_original_point.interpolate_ratio, 0.0))
			{
				++num_exact_samples[first];
			}
		}
		for (unsigned int v = 0; v < num_exact_samples.size(); ++v)
		{
			SCOPED_TRACE(v);
			EXPECT_EQ(num_exact_samples[v], 1u);
		}

		// A coverage with a scalar for every vertex of every ring interpolates onto the samples:
		// each sample's scalar lies between those of its arc's two vertices.
		const GPlatesPropertyValues::ValueObjectType scalar_type =
				GPlatesPropertyValues::ValueObjectType::create_gpml("TestScalar");
		GPlatesAppLogic::ScalarCoverageTimeSpan::initial_scalar_coverage_type initial_scalar_coverage;
		initial_scalar_coverage[scalar_type] = { 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0 };
		const GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
				GPlatesAppLogic::ScalarCoverageTimeSpan::create(initial_scalar_coverage, geometry_time_span);

		std::vector<double> scalar_values;
		ASSERT_TRUE(scalar_coverage_time_span->get_scalar_values(scalar_type, 0.0, scalar_values));
		ASSERT_EQ(scalar_values.size(), interpolate_original_points.size());
		for (unsigned int n = 0; n < scalar_values.size(); ++n)
		{
			SCOPED_TRACE(n);
			const double first_scalar =
					initial_scalar_coverage[scalar_type][interpolate_original_points[n].first_point_index];
			const double second_scalar =
					initial_scalar_coverage[scalar_type][interpolate_original_points[n].second_point_index];
			EXPECT_GE(scalar_values[n], std::min(first_scalar, second_scalar) - 1e-9);
			EXPECT_LE(scalar_values[n], std::max(first_scalar, second_scalar) + 1e-9);
		}
	}
}
