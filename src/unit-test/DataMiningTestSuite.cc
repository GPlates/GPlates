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

#include "unit-test/AppLogicTestSuite.h"
#include "unit-test/ApplicationStateTest.h"
#include "unit-test/TestSuiteFilter.h"
#include "unit-test/DataMiningTestSuite.h"
#include "unit-test/DataAssociationDataTableTest.h"
#include "unit-test/DataAssociationTest.h"
#include "unit-test/DataSelectorTest.h"
#include "unit-test/MultiThreadTest.h"
#include "unit-test/FilterTest.h"
#include "unit-test/CoregTest.h"


GPlatesUnitTest::DataMiningTestSuite::DataMiningTestSuite(
		unsigned level) : 
	GPlatesUnitTest::GPlatesTestSuite(
			"DataMiningTestSuite")
{
	init(level);
}

void 
GPlatesUnitTest::DataMiningTestSuite::construct_maps()
{
	ADD_TESTSUITE(Coreg);
	ADD_TESTSUITE(DataAssociationDataTable);
	ADD_TESTSUITE(DataAssociation);
	ADD_TESTSUITE(DataSelector);
	ADD_TESTSUITE(MultiThread);
	ADD_TESTSUITE(Filter);
}

