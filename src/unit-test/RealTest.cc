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

#include "unit-test/RealTest.h"
#include "maths/MathsUtils.h"

using GPlatesMaths::are_almost_exactly_equal;

GPlatesUnitTest::RealTestSuite::RealTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"RealTestSuite")
{
	init(level);
}


void
GPlatesUnitTest::RealTestSuite::construct_maps()
{
	boost::shared_ptr<RealTest> instance(
		new RealTest());

	ADD_TESTCASE(RealTest,test_positive_infinity);
	ADD_TESTCASE(RealTest,test_negative_infinity);
	ADD_TESTCASE(RealTest,test_nan);
	ADD_TESTCASE(RealTest,test_zero);
}

void
GPlatesUnitTest::RealTest::test_positive_infinity()
{
	BOOST_CHECK(GPlatesMaths::is_infinity(GPlatesMaths::positive_infinity<double>()));
	BOOST_CHECK(GPlatesMaths::is_positive_infinity(GPlatesMaths::positive_infinity<double>()));
	BOOST_CHECK(!GPlatesMaths::is_negative_infinity(GPlatesMaths::positive_infinity<double>()));
	BOOST_CHECK(!GPlatesMaths::is_nan(GPlatesMaths::positive_infinity<double>()));
}

void
GPlatesUnitTest::RealTest::test_negative_infinity()
{
	BOOST_CHECK(GPlatesMaths::is_infinity(GPlatesMaths::negative_infinity<double>()));
	BOOST_CHECK(!GPlatesMaths::is_positive_infinity(GPlatesMaths::negative_infinity<double>()));
	BOOST_CHECK(GPlatesMaths::is_negative_infinity(GPlatesMaths::negative_infinity<double>()));
	BOOST_CHECK(!GPlatesMaths::is_nan(GPlatesMaths::negative_infinity<double>()));
}

void 
GPlatesUnitTest::RealTest::test_nan()
{
	BOOST_CHECK(!GPlatesMaths::is_infinity(GPlatesMaths::quiet_nan<double>()));
	BOOST_CHECK(!GPlatesMaths::is_positive_infinity(GPlatesMaths::quiet_nan<double>()));
	BOOST_CHECK(!GPlatesMaths::is_negative_infinity(GPlatesMaths::quiet_nan<double>()));
	BOOST_CHECK(GPlatesMaths::is_nan(GPlatesMaths::quiet_nan<double>()));
}

void
GPlatesUnitTest::RealTest::test_zero()
{
	BOOST_CHECK(!GPlatesMaths::is_infinity(0.0));
	BOOST_CHECK(!GPlatesMaths::is_positive_infinity(0.0));
	BOOST_CHECK(!GPlatesMaths::is_negative_infinity(0.0));
	BOOST_CHECK(!GPlatesMaths::is_nan(0.0));
}

