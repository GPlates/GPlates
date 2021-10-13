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

#include "unit-test/DataAssociationTest.h"

//copy the following code into directory level test suite file
//for example, if the test class is in data-mining directory, the following code will
//be copied to DataMiningTestSuite.cc
//#include "unit-test/DataAssociationTest.h"
//ADD_TESTSUITE(DataAssociation);


GPlatesUnitTest::DataAssociationTestSuite::DataAssociationTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"DataAssociationTestSuite")
{
	init(level);
} 

void 
GPlatesUnitTest::DataAssociationTest::test_case_1()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataAssociationTest::test_case_2()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataAssociationTest::test_case_3()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataAssociationTest::test_case_4()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataAssociationTest::test_case_5()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataAssociationTest::test_case_6()
{
	//Add you test code here
	return;
}

void 
GPlatesUnitTest::DataAssociationTest::test_case_7()
{
	//Add you test code here
	return;
}

void
GPlatesUnitTest::DataAssociationTestSuite::construct_maps()
{
	boost::shared_ptr<DataAssociationTest> instance(
		new DataAssociationTest());

	ADD_TESTCASE(DataAssociationTest,test_case_1);
	ADD_TESTCASE(DataAssociationTest,test_case_2);
	ADD_TESTCASE(DataAssociationTest,test_case_3);
	ADD_TESTCASE(DataAssociationTest,test_case_4);
	ADD_TESTCASE(DataAssociationTest,test_case_5);
	ADD_TESTCASE(DataAssociationTest,test_case_6);
	ADD_TESTCASE(DataAssociationTest,test_case_7);
}

