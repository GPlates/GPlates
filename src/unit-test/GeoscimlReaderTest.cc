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

#include <iterator>
#include <vector>
#include <boost/optional.hpp>
#include <QString>
#include <gtest/gtest.h>

#include "feature-visitors/GeometryFinder.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "maths/GeometryOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"


// The GeoSciML (".gsml") reader, driven the way GPlates drives it: through the file-format
// registry. Nothing tested this reader before it was ported off QtXmlPatterns, so these pin
// the translation it does today, not the GeoSciML specification.
namespace
{
	class GeoscimlReaderTest :
			public ::testing::Test
	{
	protected:

		typedef std::vector<GPlatesModel::FeatureHandle::weak_ref> feature_seq_type;

		feature_seq_type
		read(
				const char *fixture)
		{
			const GPlatesFileIO::FileInfo file_info(
					QString(GPLATES_UNIT_TEST_DATA_DIR "/gsml/") + fixture);
			GPlatesFileIO::File::non_null_ptr_type file =
					GPlatesFileIO::File::create_file(file_info);
			d_registry.read_feature_collection(file->get_reference(), d_read_errors);
			// Keep the file, and so its feature collection, alive for the test.
			d_files.push_back(file);

			feature_seq_type features;
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
					file->get_reference().get_feature_collection();
			for (GPlatesModel::FeatureCollectionHandle::iterator iter = feature_collection->begin();
				iter != feature_collection->end();
				++iter)
			{
				features.push_back((*iter)->reference());
			}
			return features;
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry d_registry;
		GPlatesFileIO::ReadErrorAccumulation d_read_errors;
		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_files;
	};


	GPlatesModel::FeatureType
	gpml_type(
			const char *name)
	{
		return GPlatesModel::FeatureType::create_gpml(name);
	}

	GPlatesModel::PropertyName
	gpml_property(
			const char *name)
	{
		return GPlatesModel::PropertyName::create_gpml(name);
	}

	QString
	string_property(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			const GPlatesModel::PropertyName &property_name)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> value =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
						feature, property_name);
		return value ? value.get()->get_value().get().qstring() : QString();
	}

	boost::optional<double>
	double_property(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			const GPlatesModel::PropertyName &property_name)
	{
		boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type> value =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsDouble>(
						feature, property_name);
		if (!value)
		{
			return boost::none;
		}
		return value.get()->get_value();
	}

	QString
	name(
			const GPlatesModel::FeatureHandle::weak_ref &feature)
	{
		return string_property(feature, GPlatesModel::PropertyName::create_gml("name"));
	}

	/**
	 * The feature's one geometry.
	 *
	 * The caller must keep the returned pointer alive for as long as it uses the geometry:
	 * a gml:Point is stored as a coordinate pair and synthesises its PointGeometryOnSphere
	 * on demand, so - unlike a gml:LineString or a gml:Polygon - the feature itself holds
	 * no reference to it.
	 */
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
	only_geometry(
			const GPlatesModel::FeatureHandle::weak_ref &feature)
	{
		GPlatesFeatureVisitors::GeometryFinder finder;
		finder.visit_feature(feature);
		EXPECT_EQ(1, std::distance(finder.found_geometries_begin(), finder.found_geometries_end()));
		return *finder.found_geometries_begin();
	}

	void
	expect_lat_lon(
			double latitude,
			double longitude,
			const GPlatesMaths::PointOnSphere &point)
	{
		const GPlatesMaths::LatLonPoint lat_lon = GPlatesMaths::make_lat_lon_point(point);
		EXPECT_NEAR(latitude, lat_lon.latitude(), 1e-9);
		EXPECT_NEAR(longitude, lat_lon.longitude(), 1e-9);
	}
}


TEST_F(GeoscimlReaderTest, translates_every_supported_member_and_skips_the_rest)
{
	const feature_seq_type features = read("wfs_features.gsml");

	// Seven members, of which the gsml:MappedFeature is not translated - and the members
	// after it still are.
	ASSERT_EQ(6u, features.size());
	EXPECT_EQ(gpml_type("UnclassifiedFeature"), features[0]->feature_type());
	EXPECT_EQ(gpml_type("UnclassifiedFeature"), features[1]->feature_type());
	EXPECT_EQ(gpml_type("UnclassifiedFeature"), features[2]->feature_type());
	EXPECT_EQ(gpml_type("RockUnit_siliciclastic"), features[3]->feature_type());
	EXPECT_EQ(gpml_type("FossilCollection_large"), features[4]->feature_type());
	EXPECT_EQ(gpml_type("UnclassifiedFeature"), features[5]->feature_type());

	EXPECT_EQ("Line feature", name(features[0]));
	EXPECT_EQ("Point feature", name(features[1]));
	EXPECT_EQ("Polygon feature", name(features[2]));
	EXPECT_EQ("Rock unit", name(features[3]));
	EXPECT_EQ("Fossil collection", name(features[4]));
	EXPECT_EQ("Three-dimensional line", name(features[5]));
}


TEST_F(GeoscimlReaderTest, line_string_swaps_pos_list_to_lat_lon)
{
	const feature_seq_type features = read("wfs_features.gsml");
	ASSERT_LE(1u, features.size());
	const GPlatesModel::FeatureHandle::weak_ref &line = features[0];

	EXPECT_EQ("Three vertices along a diagonal",
			string_property(line, GPlatesModel::PropertyName::create_gml("description")));

	// gml:posList is longitude latitude; GPlates stores latitude longitude.
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type line_geometry =
			only_geometry(line);
	const GPlatesMaths::PolylineOnSphere *polyline =
			dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(line_geometry.get());
	ASSERT_TRUE(polyline);
	ASSERT_EQ(3u, polyline->number_of_vertices());
	expect_lat_lon(20, 10, *polyline->vertex_begin());
	expect_lat_lon(30, 20, *--polyline->vertex_end());

	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					line, GPlatesModel::PropertyName::create_gml("validTime"));
	ASSERT_TRUE(valid_time);
	EXPECT_EQ(100, valid_time.get()->begin()->get_time_position().value());
	EXPECT_EQ(0, valid_time.get()->end()->get_time_position().value());
}


TEST_F(GeoscimlReaderTest, point_is_read_in_gpml_order)
{
	const feature_seq_type features = read("wfs_features.gsml");
	ASSERT_LE(2u, features.size());

	// Unlike a gml:posList, a gml:pos is not swapped: it is read as GPML's latitude longitude.
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
			only_geometry(features[1]);
	const GPlatesMaths::PointGeometryOnSphere *point =
			dynamic_cast<const GPlatesMaths::PointGeometryOnSphere *>(geometry.get());
	ASSERT_TRUE(point);
	expect_lat_lon(45, -30, point->position());
}


TEST_F(GeoscimlReaderTest, polygon_and_repeated_properties)
{
	const feature_seq_type features = read("wfs_features.gsml");
	ASSERT_LE(3u, features.size());
	const GPlatesModel::FeatureHandle::weak_ref &polygon_feature = features[2];

	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type polygon_geometry =
			only_geometry(polygon_feature);
	const GPlatesMaths::PolygonOnSphere *polygon =
			dynamic_cast<const GPlatesMaths::PolygonOnSphere *>(polygon_geometry.get());
	ASSERT_TRUE(polygon);
	EXPECT_EQ(4, std::distance(
			polygon->exterior_ring_vertex_begin(), polygon->exterior_ring_vertex_end()));
	expect_lat_lon(0, 10, *++polygon->exterior_ring_vertex_begin());

	// A feature with two gml:name elements gets two gml:name properties.
	const std::vector<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> names =
			GPlatesFeatureVisitors::get_property_values<GPlatesPropertyValues::XsString>(
					polygon_feature, GPlatesModel::PropertyName::create_gml("name"));
	ASSERT_EQ(2u, names.size());
	EXPECT_EQ("Polygon feature", names[0]->get_value().get().qstring());
	EXPECT_EQ("Polygon alias", names[1]->get_value().get().qstring());
}


TEST_F(GeoscimlReaderTest, macrostrat_properties)
{
	const feature_seq_type features = read("wfs_features.gsml");
	ASSERT_LE(5u, features.size());
	const GPlatesModel::FeatureHandle::weak_ref &rock_unit = features[3];
	const GPlatesModel::FeatureHandle::weak_ref &fossils = features[4];

	EXPECT_EQ("sandstone", string_property(rock_unit, gpml_property("rock_type")));
	// The two thicknesses land in their own properties.
	EXPECT_EQ(12.5, double_property(rock_unit, gpml_property("rock_max_thick")).get_value_or(-1));
	EXPECT_EQ(3.0, double_property(rock_unit, gpml_property("rock_min_thick")).get_value_or(-1));

	EXPECT_EQ(7.0, double_property(fossils, gpml_property("fossil_diversity")).get_value_or(-1));
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
			only_geometry(fossils);
	const GPlatesMaths::PointGeometryOnSphere *point =
			dynamic_cast<const GPlatesMaths::PointGeometryOnSphere *>(geometry.get());
	ASSERT_TRUE(point);
	expect_lat_lon(-33, 151, point->position());
}


TEST_F(GeoscimlReaderTest, three_dimensional_pos_list_in_another_srs)
{
	const feature_seq_type features = read("wfs_features.gsml");
	ASSERT_EQ(6u, features.size());

	// srsDimension="3" is read off the bare gml:posList start tag, so the coordinates are
	// consumed in triples and the height dropped. (The SRS itself is not transformed - that
	// code has been disabled since GDAL 3 - so the values pass through, and are swapped.)
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
			only_geometry(features[5]);
	const GPlatesMaths::PolylineOnSphere *polyline =
			dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(geometry.get());
	ASSERT_TRUE(polyline);
	ASSERT_EQ(3u, polyline->number_of_vertices());
	expect_lat_lon(20, 10, *polyline->vertex_begin());
	expect_lat_lon(30, 20, *--polyline->vertex_end());
}


TEST_F(GeoscimlReaderTest, feature_without_a_feature_member_wrapper)
{
	const feature_seq_type features = read("unwrapped_feature.gsml");

	ASSERT_EQ(1u, features.size());
	EXPECT_EQ("Unwrapped feature", name(features[0]));

	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
			only_geometry(features[0]);
	const GPlatesMaths::PolylineOnSphere *polyline =
			dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(geometry.get());
	ASSERT_TRUE(polyline);
	EXPECT_EQ(2u, polyline->number_of_vertices());
	expect_lat_lon(-10, 100, *polyline->vertex_begin());
}
