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

#include "unit-test/FilterTest.h"
#include "data-mining/DataTable.h"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/FilterTest.h"
//ADD_TESTSUITE(Filter);


GPlatesUnitTest::FilterTestSuite::FilterTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"FilterTestSuite")
{
	init(level);
} 

#if 0
bool
dummy(int)
{
	std::cout<<"my dummy function running..."<<std::endl;
	return true;
}

typedef  std::vector<int>::const_iterator InputIter;
typedef std::vector<int>::iterator OutputIter;

class MyGFilterImpl 
{
public:
    
	template<class OutputType, class OutputMode>
    int
    operator()(
			std::vector<int>::const_iterator input_begin,
			std::vector<int>::const_iterator input_end,
			GPlatesUtils::FilterMapOutputHandler<OutputType, OutputMode> &result )
    {
        std::cout<<"MyGFilterImpl running...."<<std::endl;
        return std::distance(input_begin, input_end);
    }
};

struct MyPred
{
		bool
		operator()(
				int)
		{
			std::cout<<"MyPred running...."<<std::endl;
			return true;
		}
};
#endif

void 
GPlatesUnitTest::FilterTest::test_case_1()
{
#if 0
	using namespace GPlatesUtils;
	typedef bool(*Fun)(int);

	typedef PredicateFilter<
			std::vector<int>::const_iterator,
			std::vector<int>::iterator
			> MyFilter;

	typedef GenericFilter<
			std::vector<int>::const_iterator,
			std::vector<int>::iterator,
			MyGFilterImpl> GFilter;

	std::vector<int> input(7,99), output(7,0);
	boost::shared_ptr<MyFilter> filter(new  MyFilter(dummy));
	
	boost::shared_ptr<GFilter> filter1(new  GFilter(MyGFilterImpl()));


	typedef PredicateFilter<
			std::vector<int>::const_iterator,
			std::vector<int>::iterator,
			MyPred> MyFilter1;
	boost::shared_ptr<MyFilter1> filter2(new  MyFilter1(MyPred()));

	std::vector< boost::any > filters;
	filters.push_back(filter);
	filters.push_back(filter1);
	filters.push_back(filter1);
	filters.push_back(filter2);
	GPlatesDataMining::OpaqueData ret;
	typedef  boost::mpl::vector<MyFilter, GFilter, GFilter, MyFilter1>::type TypeList;

	bool flag = 
		FilterMapReduceWorkFlow<
				TypeList,
				std::vector<int>::const_iterator, 
				GPlatesDataMining::OpaqueData>
				::exec(
						filters,
						input.begin(),
						input.end());
	if(flag)
		std::cout << flag <<std::endl;
	std::cin.get();
#endif
	return;
}

void 
GPlatesUnitTest::FilterTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FilterTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FilterTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FilterTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FilterTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::FilterTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::FilterTestSuite::construct_maps()
{
	boost::shared_ptr<FilterTest> instance(
		new FilterTest());

	ADD_TESTCASE(FilterTest,test_case_1);
	ADD_TESTCASE(FilterTest,test_case_2);
	ADD_TESTCASE(FilterTest,test_case_3);
	ADD_TESTCASE(FilterTest,test_case_4);
	ADD_TESTCASE(FilterTest,test_case_5);
	ADD_TESTCASE(FilterTest,test_case_6);
	ADD_TESTCASE(FilterTest,test_case_7);
}

