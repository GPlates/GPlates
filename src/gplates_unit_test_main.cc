/* $Id$ */

/**
* \file 
* $Revision$
* $Date$
* 
* Copyright (C) 2009 The University of Sydney, Australia
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
#include <boost/test/detail/unit_test_parameters.hpp>
#include <boost/version.hpp>

#include <QDebug>

#include "unit-test/GPlatesGlobalFixture.h"
#include "unit-test/MainTestSuite.h"
#include "unit-test/TestSuiteFilter.h"
#include "utils/CommandLineParser.h"
#include "gui/GPlatesQtMsgHandler.h"
#include "global/Constants.h"

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
			std::cout << GPlatesGlobal::VersionString << std::endl;
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

boost::unit_test::test_suite*
init_unit_test_suite(
		int argc, char* argv[])
{
	GPlatesGui::GPlatesQtMsgHandler::install_qt_message_handler("GPlates_unit_test_QT.log");

	BOOST_GLOBAL_FIXTURE( GPlatesGlobalFixture );
	boost::unit_test::framework::master_test_suite().p_name.value = 
		"GPlates main test suite";

	GPlatesUnitTest::TestSuiteFilter::instance().set_filter_string(
			get_test_to_run_option(argc, argv));
	
	new GPlatesUnitTest::MainTestSuite();
	
	return 0;
}



