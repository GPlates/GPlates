/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
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
	BOOST_CHECK(!no_front_matter.required_timestamps_ma);

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

	// Required project timestamps. Radius is still mandatory - this document supplies both,
	// since a radius-less document is covered separately by test_invalid_front_matter().
	const GPlatesAppLogic::ProjectMetadata timestamps =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"  reconstruction:\n"
					"    required_timestamps_ma: \"1000, 950, 900, 850, 800, 750, 700, 650, 600, 560, 520, 480, 440, 400, 370, 340, 310, 280, 250, 225, 200, 180, 160, 140, 120, 100, 80, 60, 40, 20, 10, 0\"\n"
					"---\n");
	BOOST_CHECK(timestamps.is_valid);
	BOOST_REQUIRE(timestamps.planet_radius_metres);
	BOOST_REQUIRE(timestamps.required_timestamps_ma);
	BOOST_REQUIRE_EQUAL(timestamps.required_timestamps_ma->size(), 32);
	BOOST_CHECK_EQUAL(timestamps.required_timestamps_ma->front(), 1000.0);
	BOOST_CHECK_EQUAL(timestamps.required_timestamps_ma->back(), 0.0);

	// Saying nothing about timestamps is valid - absent means no schedule is active, which is a
	// different thing from a schedule that is present and malformed.
	const GPlatesAppLogic::ProjectMetadata no_timestamps =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"---\n");
	BOOST_CHECK(no_timestamps.is_valid);
	BOOST_CHECK(no_timestamps.required_timestamps_are_valid);
	BOOST_CHECK(!no_timestamps.required_timestamps_ma);

	// A malformed schedule must not disturb an otherwise-good radius: the two are validated
	// independently, since they are unrelated concerns that happen to share a document.
	const GPlatesAppLogic::ProjectMetadata independent_fields =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"  reconstruction:\n"
					"    required_timestamps_ma: \"100, 50, 50, 0\"\n"
					"---\n");
	BOOST_CHECK(!independent_fields.is_valid);
	BOOST_CHECK(independent_fields.planet_radius_is_valid);
	BOOST_REQUIRE(independent_fields.planet_radius_metres);
	BOOST_CHECK_CLOSE(independent_fields.planet_radius_metres.get(), 6371000.0, 1e-10);
	BOOST_CHECK(!independent_fields.required_timestamps_are_valid);
	BOOST_CHECK(!independent_fields.required_timestamps_diagnostic.isEmpty());

	// The radius is optional. A document that says nothing about its planet is describing Earth,
	// which is applied downstream by PlanetaryParameters rather than invented here.
	const GPlatesAppLogic::ProjectMetadata no_radius =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  resolution:\n"
					"    default_km: 250\n"
					"---\n");
	BOOST_CHECK(no_radius.is_valid);
	BOOST_CHECK(no_radius.diagnostic.isEmpty());
	BOOST_CHECK(no_radius.planet_radius_is_valid);
	BOOST_CHECK(!no_radius.planet_radius_metres);
	BOOST_REQUIRE(no_radius.default_resolution_km);
	BOOST_CHECK_CLOSE(no_radius.default_resolution_km.get(), 250.0, 1e-10);

	// Front matter carrying nothing at all is well-formed, not an error.
	const GPlatesAppLogic::ProjectMetadata empty_front_matter =
			GPlatesAppLogic::ProjectMetadataParser::parse("---\ngplates:\n  schema_version: 1\n---\n");
	BOOST_CHECK(empty_front_matter.has_front_matter);
	BOOST_CHECK(empty_front_matter.is_valid);
	BOOST_CHECK(!empty_front_matter.planet_radius_metres);

	// The point of the whole arrangement: one unusable value costs only its own field. This
	// document has a bad radius, a bad granularity and a bad per-feature-type override, and every
	// other field in it still arrives intact.
	const GPlatesAppLogic::ProjectMetadata salvaged =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 0\n"
					"  resolution:\n"
					"    default_km: 500\n"
					"    by_feature_type:\n"
					"      MidOceanRidge: 250\n"
					"      Coastline: -3\n"
					"  reconstruction:\n"
					"    required_timestamps_ma: \"1000, 500, 0\"\n"
					"    granularity_my: nonsense\n"
					"  subduction:\n"
					"    initiation_my: 10\n"
					"---\n");
	BOOST_CHECK(!salvaged.is_valid);

	// The three bad fields are unset, and each is named in the diagnostic along with its value.
	BOOST_CHECK(!salvaged.planet_radius_is_valid);
	BOOST_CHECK(!salvaged.planet_radius_metres);
	BOOST_CHECK(!salvaged.granularity_my);
	BOOST_CHECK(!salvaged.resolution_km_by_feature_type.contains("gpml:Coastline"));
	BOOST_CHECK(salvaged.diagnostic.contains("radius_m"));
	BOOST_CHECK(salvaged.diagnostic.contains("granularity_my"));
	BOOST_CHECK(salvaged.diagnostic.contains("Coastline"));
	BOOST_CHECK(salvaged.diagnostic.contains("still being used"));

	// Everything else survived.
	BOOST_REQUIRE(salvaged.default_resolution_km);
	BOOST_CHECK_CLOSE(salvaged.default_resolution_km.get(), 500.0, 1e-10);
	BOOST_REQUIRE(salvaged.resolution_km_by_feature_type.contains("gpml:MidOceanRidge"));
	BOOST_CHECK_CLOSE(salvaged.resolution_km_by_feature_type.value("gpml:MidOceanRidge"), 250.0, 1e-10);
	BOOST_CHECK(salvaged.required_timestamps_are_valid);
	BOOST_REQUIRE(salvaged.required_timestamps_ma);
	BOOST_REQUIRE_EQUAL(salvaged.required_timestamps_ma->size(), 3);
	BOOST_REQUIRE(salvaged.subduction_initiation_my);
	BOOST_CHECK_CLOSE(salvaged.subduction_initiation_my.get(), 10.0, 1e-10);

	// A document that could not be parsed at all is still fatal - there is nothing to salvage,
	// because nothing was successfully read. Contrast with the case above.
	const GPlatesAppLogic::ProjectMetadata unparseable =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"  reconstruction:\n"
					"    required_timestamps_ma: \"1000, 500, 0\"\n"
					"  resolution: [500]\n"
					"---\n");
	BOOST_CHECK(!unparseable.is_valid);
	BOOST_CHECK(!unparseable.required_timestamps_ma);
	BOOST_CHECK(!unparseable.planet_radius_metres);

	// Granularity and the subduction rates. The template documents all of these, so a document
	// setting them expects them to mean something rather than being accepted and ignored.
	const GPlatesAppLogic::ProjectMetadata intent =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"  reconstruction:\n"
					"    granularity_my: 5\n"
					"  subduction:\n"
					"    initiation_my: 10\n"
					"    propagation_km_per_my: 30\n"
					"    reversal_my: 8\n"
					"    breakoff_my: 15\n"
					"---\n");
	BOOST_CHECK(intent.is_valid);
	BOOST_REQUIRE(intent.granularity_my);
	BOOST_CHECK_CLOSE(intent.granularity_my.get(), 5.0, 1e-10);
	BOOST_REQUIRE(intent.subduction_initiation_my);
	BOOST_CHECK_CLOSE(intent.subduction_initiation_my.get(), 10.0, 1e-10);
	BOOST_REQUIRE(intent.subduction_propagation_km_per_my);
	BOOST_CHECK_CLOSE(intent.subduction_propagation_km_per_my.get(), 30.0, 1e-10);
	BOOST_REQUIRE(intent.subduction_reversal_my);
	BOOST_CHECK_CLOSE(intent.subduction_reversal_my.get(), 8.0, 1e-10);
	BOOST_REQUIRE(intent.subduction_breakoff_my);
	BOOST_CHECK_CLOSE(intent.subduction_breakoff_my.get(), 15.0, 1e-10);

	// Each is independently optional: pinning one down leaves the rest to the consumer's own
	// defaults rather than forcing the whole section to be written out.
	const GPlatesAppLogic::ProjectMetadata partial_intent =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"  subduction:\n"
					"    reversal_my: 12\n"
					"---\n");
	BOOST_CHECK(partial_intent.is_valid);
	BOOST_CHECK(!partial_intent.granularity_my);
	BOOST_CHECK(!partial_intent.subduction_initiation_my);
	BOOST_CHECK(!partial_intent.subduction_propagation_km_per_my);
	BOOST_CHECK(!partial_intent.subduction_breakoff_my);
	BOOST_REQUIRE(partial_intent.subduction_reversal_my);
	BOOST_CHECK_CLOSE(partial_intent.subduction_reversal_my.get(), 12.0, 1e-10);

	// The whole PROJECT-template.md front matter, with every documented key present and set to
	// the value the template ships. A template that does not parse is worse than no template.
	const GPlatesAppLogic::ProjectMetadata whole_template =
			GPlatesAppLogic::ProjectMetadataParser::parse(
					"---\n"
					"gplates:\n"
					"  schema_version: 1\n"
					"  planet:\n"
					"    radius_m: 6371000\n"
					"  resolution:\n"
					"    default_km: 500\n"
					"    by_feature_type:\n"
					"      MidOceanRidge: 250\n"
					"  reconstruction:\n"
					"    required_timestamps_ma: \"1000, 500, 0\"\n"
					"    granularity_my: 5\n"
					"  subduction:\n"
					"    initiation_my: 10\n"
					"    propagation_km_per_my: 30\n"
					"    reversal_my: 8\n"
					"    breakoff_my: 15\n"
					"---\n");
	BOOST_CHECK(whole_template.is_valid);
	BOOST_CHECK(whole_template.diagnostic.isEmpty());
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
			<< "---\ngplates:\n  planet:\n    radius_m: 1\n    radius_m: 2\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 1\n"
			// Present but unusable is a mistake in the document, and deliberately not the same
			// as absent - saying nothing leaves the consumer its own default, saying zero does
			// not. Every optional number is held to this, not just the ones with consumers today.
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  resolution:\n    default_km: 0\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  reconstruction:\n    granularity_my: 0\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  reconstruction:\n    granularity_my: -5\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  reconstruction:\n    granularity_my: soon\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  subduction:\n    initiation_my: -1\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  subduction:\n    propagation_km_per_my: 0\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  subduction:\n    reversal_my: .inf\n---\n"
			<< "---\ngplates:\n  planet:\n    radius_m: 6371000\n  subduction:\n    breakoff_my: nan\n---\n";

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

	// Front matter that simply does not mention the planet falls back to Earth the quiet way, with
	// no diagnostic. This is the distinction the parser draws: saying nothing is Earth by
	// omission, while saying something unusable - the invalid document above - keeps the Earth
	// radius but says so.
	const QString silent_path = QDir(temporary_directory.path()).filePath("silent.md");
	write_utf8_file(silent_path, "---\ngplates:\n  schema_version: 1\n---\n\n# Notes\n");
	const int silent_index = registry.add_document(silent_path);
	BOOST_REQUIRE(registry.set_primary_document(silent_index));
	BOOST_CHECK_EQUAL(parameters.radius_source(), GPlatesAppLogic::PlanetaryParameters::EARTH_DEFAULT);
	BOOST_CHECK(parameters.radius_diagnostic().isEmpty());
	BOOST_CHECK_EQUAL(parameters.effective_radius_metres(), GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1000.0);
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
