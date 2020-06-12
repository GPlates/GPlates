/* $Id$ */

/**
* \file 
* $Revision$
* $Date$
* 
* Copyright (C) 2009, 2010 The University of Sydney, Australia
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
#include <boost/test/unit_test.hpp>
#include <boost/version.hpp>

#include <QDebug>
#include <QDir>

#include "app-logic/GPlatesQtMsgHandler.h"

#include "global/Version.h"

#include "maths/MathsUtils.h"

#include "unit-test/GPlatesGlobalFixture.h"
#include "unit-test/MainTestSuite.h"
#include "unit-test/TestSuiteFilter.h"

#include "utils/CommandLineParser.h"


namespace
{
	const char *TEST_TO_RUN_OPTION_NAME = "G_test_to_run";

	void
	print_usage(
			std::ostream &os,
			const GPlatesUtils::CommandLineParser::InputOptions &input_options) 
	{

		// Print the visible options.
		os
			<< GPlatesUtils::CommandLineParser::get_visible_options(input_options)
			<< std::endl;
	}

	void
	print_usage_and_exit(
			std::ostream &os,
			const GPlatesUtils::CommandLineParser::InputOptions &input_options) 
	{

		print_usage(os, input_options);
		exit(1);
	}

	std::string 
	get_test_to_run_option(
			int argc, 
			char* argv[])
	{
		std::string test_to_run_option;

		GPlatesUtils::CommandLineParser::InputOptions input_options;
		input_options.add_simple_options();

		input_options.generic_options.add_options()
			(TEST_TO_RUN_OPTION_NAME,
			boost::program_options::value<std::string>()->default_value(""),
			"specify the test names to run");

		boost::program_options::variables_map vm;

		try
		{
			parse_command_line_options(
				vm, argc, argv, input_options);
		}
		catch(std::exception& exc)
		{
			//TODO: process the exception instead of printing out error message
			qWarning() << "Error processing command-line: " << exc.what();
		}
		
		if (GPlatesUtils::CommandLineParser::is_help_requested(vm))
		{	
			print_usage_and_exit(std::cout, input_options);
		}

		// Print GPlates version if requested.
		if (GPlatesUtils::CommandLineParser::is_version_requested(vm))
		{
			std::cout << GPlatesGlobal::Version::get_GPlates_version().toLatin1().constData() << std::endl;
			exit(1);
		}
		
		if (vm.count(TEST_TO_RUN_OPTION_NAME))
		{
			test_to_run_option =
					vm[TEST_TO_RUN_OPTION_NAME].as<std::string>();
		}
		return test_to_run_option;
	}
}

bool init_unit_test()
{
	// Initialise Qt resources that exist in the static 'qt-resources' library.
	Q_INIT_RESOURCE(opengl);
	Q_INIT_RESOURCE(python);
	Q_INIT_RESOURCE(gpgim);
	Q_INIT_RESOURCE(qt_widgets);

	// Sanity check: Proceed only if we have access to infinity and NaN.
	// This should pass on all systems that we support.
	GPlatesMaths::assert_has_infinity_and_nan();

	GPlatesAppLogic::GPlatesQtMsgHandler::install_qt_message_handler("GPlates_unit_test_QT.log");

	BOOST_GLOBAL_FIXTURE( GPlatesGlobalFixture );
	boost::unit_test::framework::master_test_suite().p_name.value =
		"GPlates main test suite";

	GPlatesUnitTest::TestSuiteFilter::instance().set_filter_string(
			get_test_to_run_option(
					boost::unit_test::framework::master_test_suite().argc,
					boost::unit_test::framework::master_test_suite().argv));
	
	/*
	"DO NOT REMOVE THE NEW OPERATOR!
	*/
	//I know it looks strange, feel like a memory leak.
	//But the memory is managed by boost unit test framework.
	//We need to keep the object alive. The boost framework will release memory.
	//I print out a message in the destructor of MainTestSuite to testify it.
	new GPlatesUnitTest::MainTestSuite();
	/*
	" DO NOT REMOVE THE NEW OPERATOR!
	*/

	return true;
}

//
// We're using the dynamically-linked version of Boost unit test library (rather than statically linked)
// because we use dynamic linking for other Boost libraries (such as Boost python) and it is error prone
// to change the CMake variable "Boost_USE_STATIC_LIBS" from "OFF" (eg, for Boost python) to "ON"
// (for Boost unit test). So we just set "Boost_USE_STATIC_LIBS" to "OFF" for all Boost libraries.
//
// We also defined "BOOST_TEST_DYN_LINK" in CMake to avoid having to define it before each <boost/test/unit_test.hpp> include.
//
// Apparently when dynamic linking we cannot use the obsolete initialisation function "init_unit_test_suite()"
// that we used previously:
//   https://www.boost.org/doc/libs/1_60_0/libs/test/doc/html/boost_test/adv_scenarios/obsolete_init_func.html
//
// Instead (for dynamic linking) we must use the alternative initialization function name and signature "bool init_unit_test();".
// We're following the advice from here:
//   https://www.boost.org/doc/libs/1_60_0/libs/test/doc/html/boost_test/adv_scenarios/shared_lib_customizations/init_func.html
//
//
// To list the unit tests you can run 'gplates-unit-test --list_content'.
// To run specific unit tests you can run 'gplates-unit-test --run_test=ScribeTestSuite,*/*/RealTest__test_zero', for example, to run
// all tests in 'ScribeTestSuite' and also the test 'RealTest__test_zero' in the 'MathsTestSuite/RealTestSuite/' level of the test tree.
//
int main(int argc, char* argv[])
{
	return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
