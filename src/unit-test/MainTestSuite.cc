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

#include "unit-test/MainTestSuite.h"
#include "unit-test/AppLogicTestSuite.h"
#include "unit-test/UnitTestTestSuite.h"
#include "unit-test/TestSuiteFilter.h"
#include "unit-test/ModelTestSuite.h"
#include "unit-test/CanvasToolsTestSuite.h"
#include "unit-test/PresentationTestSuite.h"
#include "unit-test/FeatureVisitorsTestSuite.h"
#include "unit-test/PropertyValuesTestSuite.h"
#include "unit-test/FileIoTestSuite.h"
#include "unit-test/GeometryVisitorsTestSuite.h"
#include "unit-test/UtilsTestSuite.h"
#include "unit-test/GlobalTestSuite.h"
#include "unit-test/ViewOperationsTestSuite.h"
#include "unit-test/GuiTestSuite.h"
#include "unit-test/MathsTestSuite.h"
#include "unit-test/DataMiningTestSuite.h"



void GPlatesUnitTest::MainTestSuite::construct_maps()
{
	ADD_TESTSUITE(AppLogic);
	ADD_TESTSUITE(UnitTest);
	ADD_TESTSUITE(Model);
	ADD_TESTSUITE(CanvasTools);
	ADD_TESTSUITE(Presentation);
	ADD_TESTSUITE(FeatureVisitors);
	ADD_TESTSUITE(PropertyValues);
	ADD_TESTSUITE(FileIo);
	ADD_TESTSUITE(GeometryVisitors);
	ADD_TESTSUITE(Utils);
	ADD_TESTSUITE(Global);
	ADD_TESTSUITE(ViewOperations);
	ADD_TESTSUITE(Gui);
	ADD_TESTSUITE(Maths);
	ADD_TESTSUITE(DataMining);
}

void GPlatesUnitTest::MainTestSuite::add_test_suites()
{	
	for(TestSuiteMap::iterator it=d_test_suites_map.begin(); 
		it!=d_test_suites_map.end();
		it++ )
	{
		if(GPlatesUnitTest::TestSuiteFilter::instance().pass((*it).first, d_level))
		{
			qDebug() << "adding " << (*it).first.c_str();
			boost::unit_test::framework::master_test_suite().add((*it).second);
		}
	}
}
void GPlatesUnitTest::MainTestSuite::add_test_cases()
{
//TODO
}

