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

#include "unit-test/ApplicationStateTest.h"

GPlatesUnitTest::ApplicationStateTestSuite::ApplicationStateTestSuite(
		unsigned level) :
	GPlatesUnitTest::GPlatesTestSuite(
			"ApplicationStateTestSuite")
{
	init(level);
} 

void 
GPlatesUnitTest::ApplicationStateTest::test_get_model_interface()
{
	BOOST_TEST_MESSAGE( "Testing get_model_interface()...." );
	//BOOST_CHECK(false==true);
	BOOST_TEST_MESSAGE( "End of testing get_model_interface()!" );
	return;
}

void
GPlatesUnitTest::ApplicationStateTestSuite::construct_maps()
{
	boost::shared_ptr<ApplicationStateTest> instance(
		new ApplicationStateTest());

	ADD_TESTCASE(ApplicationStateTest,test_get_model_interface);
}

