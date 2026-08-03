/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 */

#include <boost/test/unit_test.hpp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "ProjectMetadataTest.h"

#include "app-logic/PlanetaryParameters.h"
#include "app-logic/ProjectDocumentRegistry.h"
#include "app-logic/ProjectMetadata.h"

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"

#include "utils/Earth.h"


namespace
{
	void
	write_utf8_file(
			const QString &file_path,
			const QString &contents)
	{
		QFile file(file_path);
		BOOST_REQUIRE(file.open(QIODevice::WriteOnly));
		BOOST_REQUIRE(file.write(contents.toUtf8()) == contents.toUtf8().size());
	}
}


GPlatesUnitTest::ProjectMetadataTestSuite::ProjectMetadataTestSuite(
		unsigned depth) :
	GPlatesTestSuite("ProjectMetadataTestSuite")
{
	init(depth);
}


void
GPlatesUnitTest::ProjectMetadataTestSuite::construct_maps()
{
	boost::shared_ptr<ProjectMetadataTest> instance(new ProjectMetadataTest());
	ADD_TESTCASE(ProjectMetadataTest, test_front_matter_parsing);
	ADD_TESTCASE(ProjectMetadataTest, test_invalid_front_matter);
	ADD_TESTCASE(ProjectMetadataTest, test_document_registry_and_planetary_parameters);
	ADD_TESTCASE(ProjectMetadataTest, test_document_registry_restoration);
	ADD_TESTCASE(ProjectMetadataTest, test_invalid_metadata_fallback);
	ADD_TESTCASE(ProjectMetadataTest, test_radius_scaling);
}


void
GPlatesUnitTest::ProjectMetadataTest::test_front_matter_parsing()
{
	const GPlatesAppLogic::ProjectMetadata no_front_matter =
			GPlatesAppLogic::ProjectMetadataParser::parse("# Notes\n");
	BOOST_CHECK(!no_front_matter.has_front_matter);
	BOOST_CHECK(no_front_matter.is_valid);
	BOOST_CHECK(!no_front_matter.planet_radius_metres);

	const GPlatesAppLogic::ProjectMetadata delimiter_not_on_first_line =
			GPlatesAppLogic::ProjectMetadataParser::parse("# Notes\n---\ngplates:\n  planet:\n    radius_m: 1\n---\n");
	BOOST_CHECK(!delimiter_not_on_first_line.has_front_matter);

	const GPlatesAppLogic::ProjectMetadata integer_radius =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  planet:\n"
					"    radius_m: 6900000 # metres\n"
					"  unknown_key: true\n"
					"---\n");
	BOOST_REQUIRE(integer_radius.planet_radius_metres);
	BOOST_CHECK(integer_radius.is_valid);
	BOOST_CHECK_EQUAL(integer_radius.planet_radius_metres.get(), 6900000.0);

	const GPlatesAppLogic::ProjectMetadata floating_radius =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6900000.25\n"
					"---\n");
	BOOST_REQUIRE(floating_radius.planet_radius_metres);
	BOOST_CHECK_CLOSE(floating_radius.planet_radius_metres.get(), 6900000.25, 1e-10);
}


void
GPlatesUnitTest::ProjectMetadataTest::test_invalid_front_matter()
{
	const QStringList invalid_documents = QStringList()
			<< "---\ngplates:\n  planet:\n    radius_m: 0\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: -1\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: not-a-number\n---\n"
			<< "---\ngplates:\n  schema_version: 2\n  planet:\n    radius_m: 1\n---\n"
			<< "---\ngplates:\n planet:\n    radius_m: 1\n---\n"
			<< "---\ngplates: { planet: { radius_m: 1 } }\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: [1]\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: .inf\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: nan\n---\n"
			<< "---\nradius: 6900000\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 1\n    radius_m: 2\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 1\n";

	for (int index = 0; index < invalid_documents.size(); ++index)
	{
		const GPlatesAppLogic::ProjectMetadata metadata =
				GPlatesAppLogic::ProjectMetadataParser::parse(invalid_documents[index]);
		BOOST_CHECK(metadata.has_front_matter);
		BOOST_CHECK(!metadata.is_valid);
		BOOST_CHECK(!metadata.diagnostic.isEmpty());
	}
}


void
GPlatesUnitTest::ProjectMetadataTest::test_document_registry_restoration()
{
	QTemporaryDir temporary_directory;
	BOOST_REQUIRE(temporary_directory.isValid());
	const QString project_path = QDir(temporary_directory.path()).filePath("project.md");
	const QString notes_path = QDir(temporary_directory.path()).filePath("notes.md");
	const QString missing_path = QDir(temporary_directory.path()).filePath("missing.md");
	write_utf8_file(project_path, "# Project\n");
	write_utf8_file(notes_path, "# Notes\n");

	GPlatesAppLogic::ProjectDocumentRegistry source_registry;
	BOOST_CHECK_EQUAL(source_registry.add_document(project_path, "Project overview"), 0);
	BOOST_CHECK_EQUAL(source_registry.add_document(notes_path), 1);
	BOOST_CHECK_EQUAL(source_registry.add_document(missing_path), 2);
	// Adding an existing path returns its existing record instead of creating
	// an ambiguous duplicate/second primary document.
	BOOST_CHECK_EQUAL(source_registry.add_document(project_path), 0);
	BOOST_CHECK_EQUAL(source_registry.document_count(), 3);
	BOOST_REQUIRE(source_registry.set_primary_document(1));
	BOOST_REQUIRE(source_registry.move_document(2, 0));
	BOOST_REQUIRE(source_registry.primary_document_index());
	BOOST_CHECK_EQUAL(source_registry.primary_document_index().get(), 2);

	GPlatesAppLogic::ProjectDocumentRegistry restored_registry;
	restored_registry.restore_documents(
			source_registry.file_paths(),
			source_registry.display_names(),
			source_registry.primary_document_index());
	BOOST_CHECK_EQUAL(restored_registry.document_count(), 3);
	BOOST_CHECK(restored_registry.document_at(0).file_path == QDir::cleanPath(QFileInfo(missing_path).absoluteFilePath()));
	BOOST_CHECK(!QFileInfo::exists(restored_registry.document_at(0).file_path));
	BOOST_CHECK(restored_registry.document_at(1).display_name == QString("Project overview"));
	BOOST_REQUIRE(restored_registry.primary_document_index());
	BOOST_CHECK_EQUAL(restored_registry.primary_document_index().get(), 2);

	BOOST_REQUIRE(restored_registry.remove_document(1));
	BOOST_REQUIRE(restored_registry.primary_document_index());
	BOOST_CHECK_EQUAL(restored_registry.primary_document_index().get(), 1);
	BOOST_REQUIRE(restored_registry.remove_document(1));
	BOOST_CHECK(!restored_registry.primary_document_index());
}


void
GPlatesUnitTest::ProjectMetadataTest::test_invalid_metadata_fallback()
{
	QTemporaryDir temporary_directory;
	BOOST_REQUIRE(temporary_directory.isValid());
	const QString invalid_path = QDir(temporary_directory.path()).filePath("invalid.md");
	const QString valid_path = QDir(temporary_directory.path()).filePath("valid.md");
	write_utf8_file(invalid_path, "---\ngplates:\n  planet:\n    radius_m: -1\n---\n");
	write_utf8_file(valid_path, "---\ngplates:\n  planet:\n    radius_m: 8000000\n---\n");

	GPlatesAppLogic::ProjectDocumentRegistry registry;
	GPlatesAppLogic::PlanetaryParameters parameters(registry);
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::EARTH_DEFAULT);
	BOOST_CHECK(parameters.radius_diagnostic().isEmpty());
	BOOST_CHECK_EQUAL(parameters.effective_radius_metres(), GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1000.0);

	const int invalid_index = registry.add_document(invalid_path);
	const int valid_index = registry.add_document(valid_path);
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT);
	BOOST_CHECK(!parameters.radius_diagnostic().isEmpty());
	BOOST_CHECK_EQUAL(parameters.effective_radius_metres(), GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1000.0);

	BOOST_REQUIRE(registry.set_primary_document(valid_index));
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::PROJECT_MARKDOWN);
	BOOST_CHECK_EQUAL(parameters.effective_radius_metres(), 8000000.0);
	BOOST_REQUIRE(registry.set_primary_document(invalid_index));
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT);
}


void
GPlatesUnitTest::ProjectMetadataTest::test_document_registry_and_planetary_parameters()
{
	QTemporaryDir temporary_directory;
	BOOST_REQUIRE(temporary_directory.isValid());
	const QString first_path = QDir(temporary_directory.path()).filePath("project.md");
	const QString second_path = QDir(temporary_directory.path()).filePath("notes.md");
	write_utf8_file(first_path, "---\ngplates:\n  planet:\n    radius_m: 7000000\n---\n");
	write_utf8_file(second_path, "# Notes\n");

	GPlatesAppLogic::ProjectDocumentRegistry registry;
	GPlatesAppLogic::PlanetaryParameters parameters(registry);
	const int first_index = registry.add_document(first_path);
	const int second_index = registry.add_document(second_path);
	BOOST_CHECK_EQUAL(registry.document_count(), 2);
	BOOST_REQUIRE(registry.primary_document_index());
	BOOST_CHECK_EQUAL(registry.primary_document_index().get(), first_index);
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::PROJECT_MARKDOWN);
	BOOST_CHECK_EQUAL(parameters.effective_radius_metres(), 7000000.0);

	BOOST_REQUIRE(registry.load_document(first_index));
	registry.set_document_text(first_index, "---\ngplates:\n  planet:\n    radius_m: 7100000\n---\n");
	BOOST_CHECK(registry.has_dirty_documents());
	BOOST_REQUIRE(registry.save_document(first_index));
	BOOST_CHECK(!registry.has_dirty_documents());
	BOOST_CHECK_EQUAL(parameters.effective_radius_metres(), 7100000.0);

	registry.set_primary_document(second_index);
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::EARTH_DEFAULT);
	registry.remove_document(second_index);
	BOOST_CHECK(!registry.primary_document_index());
	BOOST_CHECK_CLOSE(
			parameters.effective_radius_metres(),
			GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1000.0,
			1e-10);
}


void
GPlatesUnitTest::ProjectMetadataTest::test_radius_scaling()
{
	const GPlatesMaths::PointOnSphere point_a =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));
	const GPlatesMaths::PointOnSphere point_b =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 90));
	const double distance_at_radius_1 =
			GPlatesMaths::calculate_distance_on_surface_of_sphere(point_a, point_b, 1.0).dval();
	const double distance_at_radius_2 =
			GPlatesMaths::calculate_distance_on_surface_of_sphere(point_a, point_b, 2.0).dval();
	BOOST_CHECK_CLOSE(distance_at_radius_2 / distance_at_radius_1, 2.0, 1e-10);

	const GPlatesMaths::FiniteRotation stage_rotation = GPlatesMaths::FiniteRotation::create(
			GPlatesMaths::PointOnSphere::north_pole,
			GPlatesMaths::convert_deg_to_rad(1.0));
	const double velocity_at_radius_1 =
			GPlatesMaths::calculate_velocity_vector(point_a, stage_rotation, 1.0, 1000.0).magnitude().dval();
	const double velocity_at_radius_2 =
			GPlatesMaths::calculate_velocity_vector(point_a, stage_rotation, 1.0, 2000.0).magnitude().dval();
	BOOST_CHECK_CLOSE(velocity_at_radius_2 / velocity_at_radius_1, 2.0, 1e-10);

	const double angular_area = 0.25;
	BOOST_CHECK_CLOSE((angular_area * 2.0 * 2.0) / (angular_area * 1.0 * 1.0), 4.0, 1e-10);
}
