/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <iostream>

#include <QDebug>

#include "unit-test/TestSuiteFilterTest.h"

GPlatesUnitTest::TestSuiteFilterTestSuite::TestSuiteFilterTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"TestSuiteFilterTestSuite")
{
	init(level);
}

void 
GPlatesUnitTest::TestSuiteFilterTest::test_is_empty()
{
	GPlatesUnitTest::TestSuiteFilter::set_filter_string(
		"leve0-1/leve1-1,level1-2,level1-3*,*-4/*//");

	BOOST_CHECK(d_test_suite_filter.is_empty(0) == false);
	BOOST_CHECK(d_test_suite_filter.is_empty(1) == false);
	BOOST_CHECK(d_test_suite_filter.is_empty(2) == false);
	BOOST_CHECK(d_test_suite_filter.is_empty(3) == true);
	BOOST_CHECK(d_test_suite_filter.is_empty(4) == true);

	return;
}

void 
GPlatesUnitTest::TestSuiteFilterTest::test_is_match()
{
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "test_string") == true);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "string") == false);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "test*") == true);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "*string") == true);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "*") == true);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "string*") == false);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "*test") == false);
	BOOST_CHECK(d_test_suite_filter.is_match("", "test_string") == false);
	BOOST_CHECK(d_test_suite_filter.is_match("test_string", "") == true);
}

void 
GPlatesUnitTest::TestSuiteFilterTest::test_pass()
{
	GPlatesUnitTest::TestSuiteFilter::set_filter_string(
		"level0-1/leve1-1,level1-2,level1-3*,*-4/*//");
	
	BOOST_CHECK(d_test_suite_filter.pass("level0-1", 0) == true);
	BOOST_CHECK(d_test_suite_filter.pass("level1-2", 1) == true);
	BOOST_CHECK(d_test_suite_filter.pass("level1-3234", 1) == true);
	BOOST_CHECK(d_test_suite_filter.pass("dfwaegfd-4", 1) == true);
	BOOST_CHECK(d_test_suite_filter.pass("level2dswedw", 2) == true);
	BOOST_CHECK(d_test_suite_filter.pass("level3", 3) == true);
	BOOST_CHECK(d_test_suite_filter.pass("level0-2", 0) == false);
	BOOST_CHECK(d_test_suite_filter.pass("level1", 1) == false);
	BOOST_CHECK(d_test_suite_filter.pass("fde-41", 1) == false);
	BOOST_CHECK(d_test_suite_filter.pass("dfef-34", 1) == false);
}

void 
GPlatesUnitTest::TestSuiteFilterTest::test_set_filter_string()
{
	GPlatesUnitTest::TestSuiteFilter::set_filter_string(
		"level0-1/leve1-1,level1-2,level1-3*,*-4/*//");
	
	FilterData d_filter = d_test_suite_filter.get_filter();

	BOOST_CHECK(d_filter[0][0] == "level0-1");
	BOOST_CHECK(d_filter[1][0] == "leve1-1");
	BOOST_CHECK(d_filter[1][1] == "level1-2");
	BOOST_CHECK(d_filter[1][2] == "level1-3*");
	BOOST_CHECK(d_filter[1][3] == "*-4");
	BOOST_CHECK(d_filter[2][0] == "*");
}

void
GPlatesUnitTest::TestSuiteFilterTestSuite::construct_maps()
{
	boost::shared_ptr<TestSuiteFilterTest> instance(
		new TestSuiteFilterTest());

	ADD_TESTCASE(TestSuiteFilterTest,test_is_empty);
	ADD_TESTCASE(TestSuiteFilterTest,test_is_match);
	ADD_TESTCASE(TestSuiteFilterTest,test_pass);
	ADD_TESTCASE(TestSuiteFilterTest,test_set_filter_string);
}



