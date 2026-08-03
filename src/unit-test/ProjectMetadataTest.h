/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 */

#ifndef GPLATES_UNIT_TEST_PROJECTMETADATATEST_H
#define GPLATES_UNIT_TEST_PROJECTMETADATATEST_H

#include "unit-test/GPlatesTestSuite.h"


namespace GPlatesUnitTest
{
	class ProjectMetadataTest
	{
	public:
		void test_front_matter_parsing();
		void test_invalid_front_matter();
		void test_document_registry_and_planetary_parameters();
		void test_document_registry_restoration();
		void test_invalid_metadata_fallback();
		void test_radius_scaling();
	};


	class ProjectMetadataTestSuite :
			public GPlatesTestSuite
	{
	public:
		explicit
		ProjectMetadataTestSuite(
				unsigned depth);

	protected:
		void construct_maps();
	};
}

#endif // GPLATES_UNIT_TEST_PROJECTMETADATATEST_H
