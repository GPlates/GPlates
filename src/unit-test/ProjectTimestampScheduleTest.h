/*
 * Copyright (C) 2026 CaliTarheel
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#ifndef GPLATES_UNIT_TEST_PROJECTTIMESTAMPSCHEDULETEST_H
#define GPLATES_UNIT_TEST_PROJECTTIMESTAMPSCHEDULETEST_H

#include <boost/test/unit_test.hpp>

#include "unit-test/GPlatesTestSuite.h"


namespace GPlatesUnitTest
{
	class ProjectTimestampScheduleTest
	{
	public:
		void test_divisible_schedule();
		void test_non_divisible_schedule();
		void test_single_timestamp();
		void test_invalid_schedule();
		void test_project_document_schedule();
		void test_schedule_survives_other_bad_fields();
	};

	class ProjectTimestampScheduleTestSuite :
			public GPlatesTestSuite
	{
	public:
		ProjectTimestampScheduleTestSuite(unsigned depth);

	protected:
		void construct_maps();
	};
}

#endif
