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

#include <boost/operators.hpp>

#include <QDebug>

#include "maths/PointOnSphere.h"
#include "maths/SphericalDistance.h"

#include "unit-test/SphericalDistanceTest.h"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/SphericalDistanceTest.h"
//ADD_TESTSUITE(SphericalDistance);


GPlatesUnitTest::SphericalDistanceTestSuite::SphericalDistanceTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"SphericalDistanceTestSuite")
{
	init(level);
} 


void 
GPlatesUnitTest::SphericalDistanceTest::test_case_1()
{
#if 0
	using namespace GPlatesMaths;
	PointOnSphere p1,p2;
	std::cout<< Spherical::min_dot_product_distance(p1,p2).dval() << std::endl;
	std::cout<< Spherical::is_min_dot_product_distance_below_threshold(p1,p2,1) << std::endl;
	if(Spherical::min_dot_product_distance_below_threshold(p1,p2,1))
		std::cout<< Spherical::min_dot_product_distance_below_threshold(p1,p2,1).get().dval() << std::endl;
	//Spherical::min_dot_product_distance<int,int>(1,2);
	return;
#endif
}

void 
GPlatesUnitTest::SphericalDistanceTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::SphericalDistanceTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::SphericalDistanceTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::SphericalDistanceTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::SphericalDistanceTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::SphericalDistanceTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::SphericalDistanceTestSuite::construct_maps()
{
	boost::shared_ptr<SphericalDistanceTest> instance(
		new SphericalDistanceTest());

	ADD_TESTCASE(SphericalDistanceTest,test_case_1);
	ADD_TESTCASE(SphericalDistanceTest,test_case_2);
	ADD_TESTCASE(SphericalDistanceTest,test_case_3);
	ADD_TESTCASE(SphericalDistanceTest,test_case_4);
	ADD_TESTCASE(SphericalDistanceTest,test_case_5);
	ADD_TESTCASE(SphericalDistanceTest,test_case_6);
	ADD_TESTCASE(SphericalDistanceTest,test_case_7);
}

