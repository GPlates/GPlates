/* $Id$ */

/**
 * @file 
 * Contains the main function of GPlates.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2009, 2010, 2011 The University of Sydney, Australia
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

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <algorithm>
#include <vector>
#include <QStringList>
#include <QTextStream>

#include "app-logic/ApplicationState.h"
#include "api/PythonInterpreterLocker.h"
#include "api/Sleeper.h"

#include "global/Constants.h"
#include "global/python.h"

#include "gui/GPlatesQApplication.h"
#include "gui/GPlatesQtMsgHandler.h"

#include "maths/MathsUtils.h"

#include "presentation/Application.h"

#include "qt-widgets/ViewportWindow.h"

#include "utils/Profile.h"
#include "utils/CommandLineParser.h"
#include "utils/ComponentManager.h"
#include "gui/PythonManager.h"

namespace
{
	/**
	 * The results of parsing the command-line options.
	 */
	class CommandLineOptions
	{
	public:
		CommandLineOptions() :
			debug_gui(false)
		{ }

		QStringList line_format_filenames;
		QStringList rotation_format_filenames;
		bool debug_gui;
	};
	
	const char *ROTATION_FILE_OPTION_NAME_WITH_SHORT_OPTION = "rotation-file,r";
	const char *LINE_FILE_OPTION_NAME_WITH_SHORT_OPTION = "line-file,l";

	const char *ROTATION_FILE_OPTION_NAME = "rotation-file";
	const char *LINE_FILE_OPTION_NAME = "line-file";
	const char *DEBUG_GUI_OPTION_NAME = "debug-gui";
	//enable data-mining feature by secret command line option.
	const char *DATA_MINING_OPTION_NAME = "data-mining";
	//enable symbol-table feature by secret command line option.
	const char *SYMBOL_TABLE_OPTION_NAME = "symbol-table";
	//enable python by secret command line option.
	const char *PYTHON_OPTION_NAME = "python";

	void
	print_usage(
			std::ostream &os,
			const GPlatesUtils::CommandLineParser::InputOptions &input_options)
	{

		// Print the visible options.
		os
			<< GPlatesUtils::CommandLineParser::get_visible_options(input_options)
			<< std::endl;

		// Let the user know that the line format filenames are positional arguments
		// and hence the '-l' is optional for them.
		os
			<< "NOTE: The line files do not need to be prefixed with '"
			<< LINE_FILE_OPTION_NAME_WITH_SHORT_OPTION
			<< "'"
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

	CommandLineOptions
	process_command_line_options(
			int argc, 
			char *argv[])
	{
		GPlatesUtils::CommandLineParser::InputOptions input_options;
		
		// Add simple help, version, etc.
		input_options.add_simple_options();
		
		// Add generic, visible options more specific to GPlates use.
		input_options.generic_options.add_options()
			(ROTATION_FILE_OPTION_NAME_WITH_SHORT_OPTION, boost::program_options::value< std::vector<std::string> >(),
			"specify rotation files")
			(LINE_FILE_OPTION_NAME_WITH_SHORT_OPTION, boost::program_options::value< std::vector<std::string> >(),
			"specify line files")
			;
		
		input_options.positional_options.add(LINE_FILE_OPTION_NAME, -1);
		
		// Add secret developer options.
		input_options.hidden_options.add_options()
			(DEBUG_GUI_OPTION_NAME, "Enable GUI debugging menu")
			;

		//Add secret data-mining options.
		input_options.hidden_options.add_options()
			(DATA_MINING_OPTION_NAME, "Enable data mining feature");

		//Add secret symbol-table options.
		input_options.hidden_options.add_options()
			(SYMBOL_TABLE_OPTION_NAME, "Enable symbol feature");

		//Add secret python options.
		input_options.hidden_options.add_options()
			(PYTHON_OPTION_NAME, "Enable python");

		boost::program_options::variables_map vm;

		try
		{
			parse_command_line_options(
				vm, argc, argv, input_options);
		}
		catch(std::exception& exc)
		{
			//TODO: process the exception instead of printing out error message
			std::cout<<"Error processing command-line: "<<exc.what()<<std::endl;
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

		// Create our return structure.
		CommandLineOptions command_line_options;

		if(vm.count(ROTATION_FILE_OPTION_NAME))
		{
			std::vector<std::string> rotation_files =
				vm[ROTATION_FILE_OPTION_NAME].as<std::vector<std::string> >();
			for(unsigned int i=0; i<rotation_files.size(); i++)
			{
				command_line_options.rotation_format_filenames.push_back(rotation_files[i].c_str());
			}

		}
		if(vm.count(LINE_FILE_OPTION_NAME))
		{
			std::vector<std::string> line_files =
				vm[LINE_FILE_OPTION_NAME].as<std::vector<std::string> >();
			for(unsigned int i=0; i<line_files.size(); i++)
			{
				command_line_options.line_format_filenames.push_back(line_files[i].c_str());
			}
		}
		if(vm.count(DEBUG_GUI_OPTION_NAME))
		{
			command_line_options.debug_gui = true;
		}

		using namespace GPlatesUtils;
		if(vm.count(DATA_MINING_OPTION_NAME))
		{
			ComponentManager::instance().enable(
				ComponentManager::Component::data_mining());
		}

		//enable symbol-table feature by command line option.
		if(vm.count(SYMBOL_TABLE_OPTION_NAME))
		{
			ComponentManager::instance().enable(
				ComponentManager::Component::symbology());
		}

		//enable python by command line option.
		if(vm.count(PYTHON_OPTION_NAME))
		{
			ComponentManager::instance().enable(
				ComponentManager::Component::python());
		}

		return command_line_options;
	}
}

int internal_main(int argc, char* argv[])
{
	//on Ubuntu Natty, we need to set this env variable to avoid the funny looking of spherical grid.
	#if defined(linux) || defined(__linux__) || defined(__linux)
	{
	    char v[]= "MESA_NO_SSE=1"; 
	    putenv(v);
	}
	#endif
	// Sanity check: Proceed only if we have access to infinity and NaN.
	// This should pass on all systems that we support.
	GPlatesMaths::assert_has_infinity_and_nan();

	// This will only install handler if any of the following conditions are satisfied:
	//   1) GPLATES_PUBLIC_RELEASE is defined (automatically handled by CMake build system), or
	//   2) GPLATES_OVERRIDE_QT_MESSAGE_HANDLER environment variable is set to case-insensitive
	//      "true", "1", "yes" or "on".
	// Note: Installing handler overrides default Qt message handler.
	//       And does not log messages to the console.
	GPlatesGui::GPlatesQtMsgHandler::install_qt_message_handler();

	// GPlatesQApplication is a QApplication that also handles uncaught exceptions
	// in the Qt event thread.
	GPlatesGui::GPlatesQApplication qapplication(argc, argv);

	Q_INIT_RESOURCE(qt_widgets);

	CommandLineOptions command_line_options = process_command_line_options(
			qapplication.argc(), qapplication.argv());

	
	GPlatesPresentation::Application* app = GPlatesPresentation::Application::instance();
	GPlatesQtWidgets::ViewportWindow &main_window_widget = app->get_viewport_window();
	
	// Set up the main window widget.
	main_window_widget.load_files(
			command_line_options.line_format_filenames +
			command_line_options.rotation_format_filenames);

	// Install an extra menu for developers to help debug GUI problems.
	if (command_line_options.debug_gui)
	{
		main_window_widget.install_gui_debug_menu();
	}

	using namespace GPlatesGui;
	using namespace GPlatesUtils;
	if(!ComponentManager::instance().is_enabled(ComponentManager::Component::symbology()))
	{
		main_window_widget.hide_symbol_menu();
	}
    	
	main_window_widget.show();
	
	if(ComponentManager::instance().is_enabled(ComponentManager::Component::python()))
	{
#ifdef GPLATES_NO_PYTHON
		app->get_application_state().get_python_manager().initialize(*app);
#else
		try{
			app->get_application_state().get_python_manager().initialize();
		}
		catch(const PythonInitFailed& ex)
		{
			std::stringstream ss;
			ex.write(ss);
			qWarning() << ss.str().c_str();
			main_window_widget.hide_python_menu();
		}
#endif
	}
	else
	{
		main_window_widget.hide_python_menu();
	}

	return qapplication.exec();

	// Note: Because we are using Boost.Python, Py_Finalize() should not be called.
}


int main(int argc, char* argv[])
{
	// The first of two reasons to wrap 'main()' around 'internal_main()' is to
	// handle any uncaught exceptions that occur in main() but outside the Qt event thread.
	// Any uncaught exceptions occurring in the Qt event thread will caught by the
	// local variable of type "GPlatesGui::GPlatesQApplication" inside 'internal_main()'.
	const int return_code = GPlatesGui::GPlatesQApplication::call_main(internal_main, argc, argv);

	// The second of two reasons to wrap 'main' around 'internal_main' is because
	// we want all profiles to have completed before we do profile reporting
	// and we only want to do profile reporting if no exceptions have made their
	// way back to 'main' (in other words we won't get here if 'internal_main()' threw
	// an exception).
	// NOTE: This statement is a no-op unless the current build type is "ProfileGplates"
	// in Visual Studio or you used the "-DCMAKE_BUILD_TYPE:STRING=profilegplates"
	// command-line option in "cmake" on Linux or Mac.
	PROFILE_REPORT_TO_FILE("profile.txt");

	return return_code;
}

