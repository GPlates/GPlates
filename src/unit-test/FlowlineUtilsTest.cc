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
#include <cmath>
#include <vector>
#include <boost/optional.hpp>
#include <QString>
#include <gtest/gtest.h>

#include "app-logic/FlowlineGeometryPopulator.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/StructuralType.h"


// The Create Feature dialog can place a new flowline so that the end of its left or right half,
// rather than its seed point, is where the user clicked (FlowlineUtils::correct_end_point_to_centre).
// That must find the seed point at any reconstruction time, including younger than the flowline's
// youngest time, where the halves don't start at the seed point, and wherever the flowline's spreading
// starts (its geometry import time, or its youngest time if it has none). Only the GUI reaches it.
namespace
{
	const GPlatesModel::integer_plate_id_type LEFT_PLATE = 201;
	const GPlatesModel::integer_plate_id_type RIGHT_PLATE = 801;

	// About 10 metres (acos loses precision near zero angles).
	const double MAX_ANGLE_IN_DEGREES = 1e-4;


	double
	angle_in_degrees(
			const GPlatesMaths::PointOnSphere &point1,
			const GPlatesMaths::PointOnSphere &point2)
	{
		const double cos_angle = GPlatesMaths::dot(
				point1.position_vector(), point2.position_vector()).dval();
		return GPlatesMaths::convert_rad_to_deg(std::acos(std::max(-1.0, std::min(1.0, cos_angle))));
	}


	class FlowlineUtilsTest :
			public ::testing::Test
	{
	protected:
		void
		SetUp() override
		{
			// The flowline's times are [10, 20, 30, 40], so it doesn't start at present day, and
			// plates 201 and 801 move differently before 10 Ma. It has no geometry import time.
			d_rotation_file = read(GPLATES_UNIT_TEST_DATA_DIR "/flowline/rotations.rot");
			d_flowline_file = read(GPLATES_UNIT_TEST_DATA_DIR "/flowline/flowline.gpml");

			d_reconstruction_tree_creator =
					GPlatesAppLogic::create_cached_reconstruction_tree_creator(
							std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
									1, d_rotation_file.get()->get_reference().get_feature_collection()));

			const GPlatesModel::FeatureCollectionHandle::weak_ref flowlines =
					d_flowline_file.get()->get_reference().get_feature_collection();
			ASSERT_EQ(flowlines->size(), 1u);
			d_flowline = (*flowlines->begin())->reference();
		}

		GPlatesFileIO::File::non_null_ptr_type
		read(
				const char *filename)
		{
			GPlatesFileIO::File::non_null_ptr_type file =
					GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(QString(filename)));
			GPlatesFileIO::ReadErrorAccumulation read_errors;
			d_registry.read_feature_collection(file->get_reference(), read_errors);
			return file;
		}

		/**
		 * Gives the flowline a geometry import time (the time its spreading starts from).
		 */
		void
		set_geometry_import_time(
				const double &geometry_import_time)
		{
			GPlatesModel::ModelUtils::set_property(
					d_flowline,
					GPlatesModel::PropertyName::create_gpml("geometryImportTime"),
					GPlatesModel::ModelUtils::create_gml_time_instant(
							GPlatesPropertyValues::GeoTimeInstant(geometry_import_time)));
		}

		/**
		 * Replaces the flowline's times (youngest first), as editing them does.
		 */
		void
		set_times(
				const std::vector<double> &times)
		{
			std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> time_periods;
			for (unsigned int n = 1; n < times.size(); ++n)
			{
				time_periods.push_back(
						GPlatesPropertyValues::GmlTimePeriod::create(
								GPlatesModel::ModelUtils::create_gml_time_instant(
										GPlatesPropertyValues::GeoTimeInstant(times[n])),
								GPlatesModel::ModelUtils::create_gml_time_instant(
										GPlatesPropertyValues::GeoTimeInstant(times[n - 1]))));
			}

			GPlatesModel::ModelUtils::set_property(
					d_flowline,
					GPlatesModel::PropertyName::create_gpml("times"),
					GPlatesPropertyValues::GpmlArray::create(
							time_periods,
							GPlatesPropertyValues::StructuralType::create_gml("TimePeriod")));
		}

		/**
		 * The flowline reconstructed to @a reconstruction_time.
		 */
		GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_to_const_type
		reconstruct(
				const double &reconstruction_time)
		{
			std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> geometries;
			GPlatesAppLogic::FlowlineGeometryPopulator populator(
					geometries,
					d_reconstruction_tree_creator.get(),
					reconstruction_time);
			populator.visit_feature(d_flowline);

			EXPECT_EQ(geometries.size(), 1u);
			const GPlatesAppLogic::ReconstructedFlowline *flowline =
					dynamic_cast<const GPlatesAppLogic::ReconstructedFlowline *>(geometries.front().get());
			EXPECT_TRUE(flowline);
			return GPlatesUtils::get_non_null_pointer(flowline);
		}

		/**
		 * The seed point that correct_end_point_to_centre finds for @a end_point, the end of the
		 * half of the flowline on @a plate_1.
		 */
		GPlatesMaths::PointOnSphere
		correct_end_point_to_centre(
				const GPlatesMaths::PointOnSphere &end_point,
				GPlatesModel::integer_plate_id_type plate_1,
				GPlatesModel::integer_plate_id_type plate_2,
				const double &reconstruction_time)
		{
			GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder(reconstruction_time);
			finder.visit_feature(d_flowline);

			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_point =
					GPlatesAppLogic::FlowlineUtils::correct_end_point_to_centre(
							end_point.get_geometry_on_sphere(),
							plate_1,
							plate_2,
							finder.get_times(),
							finder.get_spreading_start_time(),
							d_reconstruction_tree_creator.get(),
							reconstruction_time);

			const boost::optional<const GPlatesMaths::PointOnSphere &> seed_point_on_sphere =
					GPlatesAppLogic::GeometryUtils::get_point_on_sphere(*seed_point);
			EXPECT_TRUE(seed_point_on_sphere);
			return seed_point_on_sphere.get();
		}

		/**
		 * Checks that the end of each half of the flowline corrects to its seed point: younger than the
		 * youngest time (where the halves don't start at the seed point), at it, and between the times.
		 */
		void
		check_end_points_correct_to_the_seed_point()
		{
			for (const double reconstruction_time : { 0.0, 5.0, 10.0, 15.0, 25.0 })
			{
				SCOPED_TRACE(reconstruction_time);
				const GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_to_const_type flowline =
						reconstruct(reconstruction_time);
				const GPlatesMaths::PointOnSphere &seed_point = flowline->reconstructed_seed_point();

				EXPECT_LT(
						angle_in_degrees(
								correct_end_point_to_centre(
										flowline->left_flowline_points()->end_point(),
										LEFT_PLATE,
										RIGHT_PLATE,
										reconstruction_time),
								seed_point),
						MAX_ANGLE_IN_DEGREES);
				EXPECT_LT(
						angle_in_degrees(
								correct_end_point_to_centre(
										flowline->right_flowline_points()->end_point(),
										RIGHT_PLATE,
										LEFT_PLATE,
										reconstruction_time),
								seed_point),
						MAX_ANGLE_IN_DEGREES);
			}
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry d_registry;
		boost::optional<GPlatesFileIO::File::non_null_ptr_type> d_rotation_file;
		boost::optional<GPlatesFileIO::File::non_null_ptr_type> d_flowline_file;
		boost::optional<GPlatesAppLogic::ReconstructionTreeCreator> d_reconstruction_tree_creator;
		GPlatesModel::FeatureHandle::weak_ref d_flowline;
	};


	TEST_F(FlowlineUtilsTest, end_points_correct_to_the_seed_point)
	{
		// Spreading from the youngest time (no geometry import time).
		check_end_points_correct_to_the_seed_point();
	}


	TEST_F(FlowlineUtilsTest, end_points_correct_to_the_seed_point_with_geometry_import_time)
	{
		// Between two of the flowline's times, so it splits a half-stage step.
		set_geometry_import_time(15.0);
		check_end_points_correct_to_the_seed_point();

		// Older than the flowline's oldest time.
		set_geometry_import_time(50.0);
		check_end_points_correct_to_the_seed_point();
	}


	TEST_F(FlowlineUtilsTest, default_geometry_import_time_is_the_youngest_time)
	{
		// Without a geometry import time the flowline spreads from its youngest time...
		GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder;
		finder.visit_feature(d_flowline);
		EXPECT_FALSE(finder.get_geometry_import_time());
		EXPECT_EQ(finder.get_spreading_start_time(), 10.0);
		EXPECT_TRUE(GPlatesAppLogic::FlowlineUtils::get_default_geometry_import_time(d_flowline) ==
				boost::optional<double>(10.0));

		// ...so giving it one there doesn't shift it.
		const GPlatesMaths::PointOnSphere seed_point_before = reconstruct(25.0)->reconstructed_seed_point();
		EXPECT_TRUE(GPlatesAppLogic::FlowlineUtils::set_default_geometry_import_time(d_flowline));
		EXPECT_LT(angle_in_degrees(reconstruct(25.0)->reconstructed_seed_point(), seed_point_before),
				MAX_ANGLE_IN_DEGREES);

		GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder_after;
		finder_after.visit_feature(d_flowline);
		EXPECT_TRUE(finder_after.get_geometry_import_time() == boost::optional<double>(10.0));

		// It already has one now.
		EXPECT_FALSE(GPlatesAppLogic::FlowlineUtils::get_default_geometry_import_time(d_flowline));
		EXPECT_FALSE(GPlatesAppLogic::FlowlineUtils::set_default_geometry_import_time(d_flowline));
	}


	TEST_F(FlowlineUtilsTest, editing_times_does_not_shift_a_flowline)
	{
		const GPlatesModel::PropertyName times_property_name =
				GPlatesModel::PropertyName::create_gpml("times");
		const GPlatesModel::PropertyName name_property_name =
				GPlatesModel::PropertyName::create_gml("name");

		const auto get_geometry_import_time = [this]()
		{
			GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder;
			finder.visit_feature(d_flowline);
			return finder.get_geometry_import_time();
		};

		// An edit of another property, or one that doesn't modify the times, adds nothing.
		EXPECT_TRUE(GPlatesAppLogic::FlowlineUtils::edit_property_without_shifting_flowline(
				d_flowline, name_property_name, []() { return true; }));
		EXPECT_FALSE(GPlatesAppLogic::FlowlineUtils::edit_property_without_shifting_flowline(
				d_flowline, times_property_name, []() { return false; }));
		EXPECT_FALSE(get_geometry_import_time());

		// Removing the youngest time (10 Ma) would make the flowline spread from 20 Ma...
		const GPlatesMaths::PointOnSphere seed_point_before = reconstruct(25.0)->reconstructed_seed_point();
		EXPECT_TRUE(GPlatesAppLogic::FlowlineUtils::edit_property_without_shifting_flowline(
				d_flowline,
				times_property_name,
				[this]()
				{
					set_times({ 20.0, 30.0, 40.0 });
					return true;
				}));

		// ...but it still spreads from 10 Ma, so it isn't shifted.
		EXPECT_TRUE(get_geometry_import_time() == boost::optional<double>(10.0));
		EXPECT_LT(angle_in_degrees(reconstruct(25.0)->reconstructed_seed_point(), seed_point_before),
				MAX_ANGLE_IN_DEGREES);
	}
}
