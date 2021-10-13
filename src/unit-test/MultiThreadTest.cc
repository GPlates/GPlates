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

#include "unit-test/MultiThreadTest.h"

#include "utils/Profile.h"
#include "boost/detail/atomic_count.hpp"
#include "boost/thread/mutex.hpp"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/MultiThreadTest.h"
//ADD_TESTSUITE(MultiThread);

GPlatesUnitTest::MultiThreadTestSuite::MultiThreadTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"MultiThreadTestSuite")
{
	init(level);
} 
//#pragma optimize("", on) 

void 
GPlatesUnitTest::MultiThreadTest::test_case_1()
{
#if 0
	//PROFILE_REPORT_TO_FILE("profile.txt");
	PROFILE_FUNC();
	volatile long counter = 0;
	long tmp = 0;
	for(long i=0; i<100000000; ++i)
	{
		++counter;
		tmp ^=counter;
	}
	std::cout << "test the performance of atomic counter." << std::endl;
	std::cout << tmp;	
	return;
#endif
}

void 
GPlatesUnitTest::MultiThreadTest::test_case_2()
{
#if 0
	//Add you test code here
	PROFILE_FUNC();
	boost::detail::atomic_count acounter(0);
	long tmp = 0;
	for(long i=0; i<100000000; ++i)
	{
		++acounter;
		tmp ^=acounter;
	}
	std::cout << "test the performance of atomic counter." << std::endl;
	std::cout << tmp;	
	return;
#endif
}

void 
GPlatesUnitTest::MultiThreadTest::test_case_3()
{
#if 0
	//Add you test code here
	PROFILE_FUNC();
	boost::mutex mtx;
	long counter = 0;
	long tmp = 0;
	for(long i=0; i<100000000; ++i)
	{
		boost::mutex::scoped_lock lc(mtx);
		++counter;
		tmp ^=counter;
	}
	std::cout << "test the performance of atomic counter." << std::endl;
	std::cout << tmp;	
	return;
#endif
}

void 
GPlatesUnitTest::MultiThreadTest::test_case_4()
{
#if 0
	//Add you test code here
	PROFILE_FUNC();
	long counter = 0;
	long tmp = 0;
	for(long i=0; i<100000000; ++i)
	{
		tmp ^=counter;
	}
	std::cout << "test the performance of atomic counter." << std::endl;
	std::cout << tmp;	
 	return;
#endif
}

void 
GPlatesUnitTest::MultiThreadTest::test_case_5()
{
	//Add you test code here
	// 	PROFILE_FUNC();
// 	long counter = 0;
// 	for(long i=0; i<100000000; ++i)
// 	{
// 		BOOST_INTERLOCKED_INCREMENT(&counter);
// 	}
// 	std::cout << "test the performance of atomic counter." << std::endl;
// 	std::cout << counter;	
	return;
}

void 
GPlatesUnitTest::MultiThreadTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::MultiThreadTest::test_case_7()
{
	//Add you test code here
	PROFILE_REPORT_TO_FILE("profile.txt");
	return;
}

void
GPlatesUnitTest::MultiThreadTestSuite::construct_maps()
{
	boost::shared_ptr<MultiThreadTest> instance(
		new MultiThreadTest());

	ADD_TESTCASE(MultiThreadTest,test_case_1);
	ADD_TESTCASE(MultiThreadTest,test_case_2);
	ADD_TESTCASE(MultiThreadTest,test_case_3);
	ADD_TESTCASE(MultiThreadTest,test_case_4);
	ADD_TESTCASE(MultiThreadTest,test_case_5);
	ADD_TESTCASE(MultiThreadTest,test_case_6);
	ADD_TESTCASE(MultiThreadTest,test_case_7);
}

