/*
 * Copyright (C) 2026 CaliTarheel
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#include "unit-test/ProjectTimestampScheduleTest.h"

#include <stdexcept>
#include <vector>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "app-logic/ProjectDocumentRegistry.h"
#include "app-logic/ProjectTimestampSchedule.h"


GPlatesUnitTest::ProjectTimestampScheduleTestSuite::ProjectTimestampScheduleTestSuite(
		unsigned depth) :
	GPlatesTestSuite("ProjectTimestampScheduleTestSuite")
{
	init(depth);
}


void
GPlatesUnitTest::ProjectTimestampScheduleTestSuite::construct_maps()
{
	boost::shared_ptr<ProjectTimestampScheduleTest> instance(
			new ProjectTimestampScheduleTest());
	ADD_TESTCASE(ProjectTimestampScheduleTest, test_divisible_schedule);
	ADD_TESTCASE(ProjectTimestampScheduleTest, test_non_divisible_schedule);
	ADD_TESTCASE(ProjectTimestampScheduleTest, test_single_timestamp);
	ADD_TESTCASE(ProjectTimestampScheduleTest, test_invalid_schedule);
	ADD_TESTCASE(ProjectTimestampScheduleTest, test_project_document_schedule);
	ADD_TESTCASE(ProjectTimestampScheduleTest, test_schedule_survives_other_bad_fields);
}


void
GPlatesUnitTest::ProjectTimestampScheduleTest::test_divisible_schedule()
{
	const std::vector<double> schedule =
			GPlatesAppLogic::ProjectTimestampSchedule::build(0.0, 100.0, 10.0);
	BOOST_REQUIRE_EQUAL(schedule.size(), 11);
	BOOST_CHECK_EQUAL(schedule.front(), 0.0);
	BOOST_CHECK_EQUAL(schedule.back(), 100.0);
}


void
GPlatesUnitTest::ProjectTimestampScheduleTest::test_non_divisible_schedule()
{
	const std::vector<double> schedule =
			GPlatesAppLogic::ProjectTimestampSchedule::build(0.0, 95.0, 10.0);
	BOOST_REQUIRE_EQUAL(schedule.size(), 11);
	BOOST_CHECK_EQUAL(schedule[schedule.size() - 2], 90.0);
	BOOST_CHECK_EQUAL(schedule.back(), 95.0);
}


void
GPlatesUnitTest::ProjectTimestampScheduleTest::test_single_timestamp()
{
	const std::vector<double> schedule =
			GPlatesAppLogic::ProjectTimestampSchedule::build(50.0, 50.0, 10.0);
	BOOST_REQUIRE_EQUAL(schedule.size(), 1);
	BOOST_CHECK_EQUAL(schedule.front(), 50.0);
}


void
GPlatesUnitTest::ProjectTimestampScheduleTest::test_invalid_schedule()
{
	BOOST_CHECK_THROW(
			GPlatesAppLogic::ProjectTimestampSchedule::build(100.0, 0.0, 10.0),
			std::invalid_argument);
	BOOST_CHECK_THROW(
			GPlatesAppLogic::ProjectTimestampSchedule::build(0.0, 100.0, 0.0),
			std::invalid_argument);
	BOOST_CHECK_THROW(
			GPlatesAppLogic::ProjectTimestampSchedule::build(0.0, 100.0, 1.0, 10),
			std::length_error);
}


void
GPlatesUnitTest::ProjectTimestampScheduleTest::test_project_document_schedule()
{
	QTemporaryDir temporary_directory;
	BOOST_REQUIRE(temporary_directory.isValid());
	const QString document_path = QDir(temporary_directory.path()).filePath("PROJECT.md");
	QFile document(document_path);
	BOOST_REQUIRE(document.open(QIODevice::WriteOnly));
	// A whole, well-formed document. test_schedule_survives_other_bad_fields() covers the schedule
	// arriving intact when the rest of the document is missing or wrong.
	const QByteArray markdown(
			"---\n"
			"gplates:\n"
			"  planet:\n"
			"    radius_m: 6371000\n"
			"  reconstruction:\n"
			"    required_timestamps_ma: \"1000, 950, 900, 850, 800, 750, 700, 650, 600, 560, 520, 480, 440, 400, 370, 340, 310, 280, 250, 225, 200, 180, 160, 140, 120, 100, 80, 60, 40, 20, 10, 0\"\n"
			"---\n");
	BOOST_REQUIRE_EQUAL(document.write(markdown), markdown.size());
	document.close();

	GPlatesAppLogic::ProjectDocumentRegistry registry;
	GPlatesAppLogic::ProjectTimestampSchedule schedule(registry);
	BOOST_CHECK_EQUAL(
			schedule.source(),
			GPlatesAppLogic::ProjectTimestampSchedule::NO_PROJECT_TIMESTAMPS);
	registry.add_document(document_path);
	BOOST_CHECK_EQUAL(
			schedule.source(),
			GPlatesAppLogic::ProjectTimestampSchedule::PROJECT_MARKDOWN);
	BOOST_REQUIRE_EQUAL(schedule.timestamps_older_to_younger().size(), 32);
	BOOST_REQUIRE(schedule.next_older_timestamp(850.0));
	BOOST_CHECK_EQUAL(schedule.next_older_timestamp(850.0).get(), 900.0);
	BOOST_REQUIRE(schedule.next_younger_timestamp(850.0));
	BOOST_CHECK_EQUAL(schedule.next_younger_timestamp(850.0).get(), 800.0);
	BOOST_REQUIRE(schedule.next_older_timestamp(875.0));
	BOOST_CHECK_EQUAL(schedule.next_older_timestamp(875.0).get(), 900.0);
	BOOST_REQUIRE(schedule.next_younger_timestamp(875.0));
	BOOST_CHECK_EQUAL(schedule.next_younger_timestamp(875.0).get(), 850.0);
	BOOST_CHECK(!schedule.next_older_timestamp(1000.0));
	BOOST_CHECK(!schedule.next_younger_timestamp(0.0));
}


void
GPlatesUnitTest::ProjectTimestampScheduleTest::test_schedule_survives_other_bad_fields()
{
	// A schedule does not depend on anything else in the document being present or usable. This
	// document has no planet section at all, and a second has a radius that is nonsense; both
	// still navigate. The schedule is the user's data and is not collateral damage from a mistake
	// in an unrelated field.
	QTemporaryDir temporary_directory;
	BOOST_REQUIRE(temporary_directory.isValid());

	const QString no_planet_path = QDir(temporary_directory.path()).filePath("PROJECT.md");
	QFile no_planet(no_planet_path);
	BOOST_REQUIRE(no_planet.open(QIODevice::WriteOnly));
	const QByteArray no_planet_markdown(
			"---\n"
			"gplates:\n"
			"  reconstruction:\n"
			"    required_timestamps_ma: \"1000, 500, 0\"\n"
			"---\n");
	BOOST_REQUIRE_EQUAL(no_planet.write(no_planet_markdown), no_planet_markdown.size());
	no_planet.close();

	GPlatesAppLogic::ProjectDocumentRegistry registry;
	GPlatesAppLogic::ProjectTimestampSchedule schedule(registry);
	registry.add_document(no_planet_path);
	BOOST_CHECK_EQUAL(
			schedule.source(),
			GPlatesAppLogic::ProjectTimestampSchedule::PROJECT_MARKDOWN);
	BOOST_REQUIRE_EQUAL(schedule.timestamps_older_to_younger().size(), 3);
	BOOST_REQUIRE(schedule.next_older_timestamp(500.0));
	BOOST_CHECK_EQUAL(schedule.next_older_timestamp(500.0).get(), 1000.0);

	const QString bad_radius_path = QDir(temporary_directory.path()).filePath("BAD-RADIUS.md");
	QFile bad_radius(bad_radius_path);
	BOOST_REQUIRE(bad_radius.open(QIODevice::WriteOnly));
	const QByteArray bad_radius_markdown(
			"---\n"
			"gplates:\n"
			"  planet:\n"
			"    radius_m: 0\n"
			"  reconstruction:\n"
			"    required_timestamps_ma: \"800, 400, 0\"\n"
			"---\n");
	BOOST_REQUIRE_EQUAL(bad_radius.write(bad_radius_markdown), bad_radius_markdown.size());
	bad_radius.close();

	const int bad_radius_index = registry.add_document(bad_radius_path);
	BOOST_REQUIRE(registry.set_primary_document(bad_radius_index));
	BOOST_CHECK_EQUAL(
			schedule.source(),
			GPlatesAppLogic::ProjectTimestampSchedule::PROJECT_MARKDOWN);
	BOOST_REQUIRE_EQUAL(schedule.timestamps_older_to_younger().size(), 3);
	BOOST_REQUIRE(schedule.next_younger_timestamp(400.0));
	BOOST_CHECK_EQUAL(schedule.next_younger_timestamp(400.0).get(), 0.0);

	// A schedule that is itself unreadable is still reported, which AnimationController shows in
	// the status bar before falling back to the ordinary frame step, so Alt doing nothing is
	// explained rather than silent.
	const QString bad_schedule_path = QDir(temporary_directory.path()).filePath("BAD-SCHEDULE.md");
	QFile bad_schedule(bad_schedule_path);
	BOOST_REQUIRE(bad_schedule.open(QIODevice::WriteOnly));
	const QByteArray bad_schedule_markdown(
			"---\n"
			"gplates:\n"
			"  reconstruction:\n"
			"    required_timestamps_ma: \"100, 50, 50, 0\"\n"
			"---\n");
	BOOST_REQUIRE_EQUAL(bad_schedule.write(bad_schedule_markdown), bad_schedule_markdown.size());
	bad_schedule.close();

	const int bad_schedule_index = registry.add_document(bad_schedule_path);
	BOOST_REQUIRE(registry.set_primary_document(bad_schedule_index));
	BOOST_CHECK_EQUAL(
			schedule.source(),
			GPlatesAppLogic::ProjectTimestampSchedule::INVALID_PROJECT_METADATA);
	BOOST_CHECK(!schedule.diagnostic().isEmpty());
	BOOST_CHECK(schedule.timestamps_older_to_younger().empty());
}
