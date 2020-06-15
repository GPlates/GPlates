/* $Id$ */

/**
 * @file 
 * Contains the main function of GPlates.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2009, 2010, 2011 The University of Sydney, Australia
 * Copyright (C) 2011 Geological Survey of Norway
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

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <QtGlobal>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

#include "app-logic/ApplicationState.h"
#include "app-logic/GPlatesQtMsgHandler.h"

#include "api/PythonInterpreterLocker.h"
#include "api/Sleeper.h"

#include "cli/CliCommandDispatcher.h"

#include "global/NotYetImplementedException.h"
#include "global/python.h"
#include "global/Version.h"

#include "gui/DrawStyleManager.h"
#include "gui/FileIOFeedback.h"
#include "gui/GPlatesQApplication.h"
#include "gui/PythonManager.h"

#include "maths/MathsUtils.h"

#include "presentation/Application.h"

#include "qt-widgets/PythonInitFailedDialog.h"
#include "qt-widgets/ViewportWindow.h"

#include "utils/CommandLineParser.h"
#include "utils/ComponentManager.h"
#include "utils/Profile.h"


namespace
{
	/**
	 * Option name to print usage of a specific GPlates command (non-GUI).
	 */
	const char *HELP_COMMAND_OPTION_NAME = "help-command";

	/**
	 * The option name used to extract the first positional command-line argument which
	 * is the GPlates command that the user wishes to execute (for non-GUI GPlates).
	 * Each command has its own set of options.
	 * This allows the user to select different functionality using a single
	 * command-line GPlates executable.
	 */
	const char *COMMAND_OPTION_NAME = "command";


	/**
	 * The results of parsing the GUI command-line options.
	 *
	 * Any command-line options specific to a particular non-GUI command are handled by
	 * GPlatesCli::CommandDispatcher (when GPlates is *not* used as the familiar GUI application).
	 */
	class GuiCommandLineOptions
	{
	public:
		GuiCommandLineOptions() :
			debug_gui(false),
			enable_python(true), // Enabled by default.
			enable_external_syncing(false),
			enable_data_mining(true),//Enable data mining by default
			enable_symbol_table(false),
			enable_hellinger_three_plate(false) // Disable three-plate fitting by default
		{  }

		boost::optional<QString> project_filename;
		QStringList feature_collection_filenames;
		bool debug_gui;
		bool enable_python;
		bool enable_external_syncing;
		bool enable_data_mining;
		bool enable_symbol_table;
		bool enable_hellinger_three_plate;
	};
	
	//! Option name associated with positional arguments (project files or feature collection files).
	const char *POSITIONAL_FILENAMES_OPTION_NAME = "positional";
	
	//! Option name for loading a project file.
	const char *PROJECT_FILENAME_OPTION_NAME = "project";
	//! Option name for loading a project file with short version.
	const char *PROJECT_FILENAME_OPTION_NAME_WITH_SHORT_OPTION = "project,p";
	
	//! Option name for loading feature collection file(s).
	const char *FEATURE_COLLECTION_FILENAMES_OPTION_NAME = "file";
	//! Option name for loading feature collection file(s) with short version.
	const char *FEATURE_COLLECTION_FILENAMES_OPTION_NAME_WITH_SHORT_OPTION = "file,f";

	//! Enable the debug GUI menu.
	const char *DEBUG_GUI_OPTION_NAME = "debug-gui";

	//! Enable data-mining feature by secret command line option.
	const char *DATA_MINING_OPTION_NAME = "data-mining";

	//! Enable symbol-table feature by secret command line option.
	const char *SYMBOL_TABLE_OPTION_NAME = "symbol-table";

	//! Enable python by secret command line option.
	const char *NO_PYTHON_OPTION_NAME = "no-python";

	//! Enable communication with external programs
	const char *ENABLE_EXTERNAL_SYNCING_OPTION_NAME = "enable-external-syncing";

	//! Enable hellinger fitting tool
	const char *ENABLE_HELLINGER_THREE_PLATE_OPTION_NAME = "enable-hellinger-3";


	/**
	 * Prints program usage to @a os.
	 */
	void
	print_usage(
			std::ostream &os,
			const GPlatesUtils::CommandLineParser::InputOptions &input_options)
	{
		typedef std::vector<GPlatesCli::CommandDispatcher::command_name_and_description_type>
				command_name_and_desc_seq_type;

		// Get the list of commands.
		GPlatesCli::CommandDispatcher command_dispatcher;
		const command_name_and_desc_seq_type command_names_and_descriptions =
				command_dispatcher.get_command_names_and_descriptions();

		// Print an basic introduction about how to use the command-line interface.
		os
				<< std::endl
				<< "Using GPlates to process a command (no graphical user interface):"
				<< std::endl
				<< "----------------------------------------------------------------"
				<< std::endl
				<< std::endl
				<< "gplates [<command> <command options ...>]"
				<< std::endl
				<< "            where <command> includes:"
				<< std::endl
				<< std::endl;

		// Print the list of commands.
		command_name_and_desc_seq_type::const_iterator iter = command_names_and_descriptions.begin();
		command_name_and_desc_seq_type::const_iterator end = command_names_and_descriptions.end();
		for ( ; iter != end; ++iter)
		{
			const std::string &command_name = iter->first;
			const std::string &command_desc = iter->second;
			os << command_name.c_str() << " - " << command_desc.c_str() << std::endl;
		}

		os
				<< std::endl
				<< "Use --help-command <command> to see the command-specific options."
				<< std::endl;

		// Print the GUI (visible) options.
		os
				<< std::endl
				<< std::endl
				<< "Starting the GPlates graphical user interface application:"
				<< std::endl
				<< "---------------------------------------------------------"
				<< std::endl
				<< std::endl
				<< "gplates [<options>] "
					"[<project-filename> | <feature-collection-filename> [<feature-collection-filename> ...]]"
				<< std::endl
				<< std::endl
				<< GPlatesUtils::CommandLineParser::get_visible_options(input_options)
				<< std::endl;
	}

	/**
	 * Adds the help command option (non-GUI).
	 */
	void
	add_help_command_option(
			GPlatesUtils::CommandLineParser::InputOptions &input_options)
	{
		input_options.generic_options.add_options()
			(HELP_COMMAND_OPTION_NAME,
			boost::program_options::value<std::string>(),
			"print options available for the specified command");
	}

	/**
	 * Prints usage for to @a os.
	 */
	void
	print_command_usage(
			std::ostream &os,
			const GPlatesUtils::CommandLineParser::InputOptions &input_options,
			const std::string &command)
	{
		GPlatesCli::CommandDispatcher command_dispatcher;

		// Add options for the command specified so that they become visible
		// when we print out the usage for the command.
		if (!command_dispatcher.is_recognised_command(command))
		{
			// The command was not a recognised command.
			qWarning()
					<< "Command-line argument '"
					<< command.c_str()
					<< "' is not a recognised command.";
			exit(1);
		}

		// Get the command's options.
		GPlatesUtils::CommandLineParser::InputOptions command_options;
		command_dispatcher.add_options_for_command(
				command,
				command_options.generic_options,
				command_options.config_options,
				command_options.hidden_options,
				command_options.positional_options);

		// Just print the options belonging to the command and nothing else.
		std::cout << GPlatesUtils::CommandLineParser::get_visible_options(command_options) << std::endl;
	}

	GuiCommandLineOptions
	parse_gui_command_line_options(
			int argc, 
			char *argv[])
	{
		GPlatesUtils::CommandLineParser::InputOptions input_options;

		// Add generic, visible options more specific to GPlates use.
		input_options.generic_options.add_options()
			(POSITIONAL_FILENAMES_OPTION_NAME,
			boost::program_options::value< std::vector<std::string> >(),
			"specify a single project file to load or one or more feature collections to load")
			;
		input_options.generic_options.add_options()
			(PROJECT_FILENAME_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value<std::string>(),
			"specify a single project file to load")
			;
		input_options.generic_options.add_options()
			(FEATURE_COLLECTION_FILENAMES_OPTION_NAME_WITH_SHORT_OPTION,
			boost::program_options::value< std::vector<std::string> >(),
			"specify feature collections to load (rotation/geometry/topology/etc)")
			;

		// Add simple help, version, etc.
		input_options.add_simple_options();

		// Add the help command option even though it's not a GUI option because all non-command
		// options go through here.
		add_help_command_option(input_options);

		// Filenames to load can be specified as positional arguments, or as '-f' / '--file' options
		// for feature collections and '-p' / '--project' options for projects, or a combination.
		//
		// NOTE: Each positional option must have an associated normal option.
		// That's just how boost positional options work.
		input_options.positional_options.add(POSITIONAL_FILENAMES_OPTION_NAME, -1);

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
			(NO_PYTHON_OPTION_NAME, "Disable python");

		// Add enable-external-syncing options
		input_options.hidden_options.add_options()
			(ENABLE_EXTERNAL_SYNCING_OPTION_NAME, "Enable external syncing.");

		// Add secret hellinger option
		input_options.hidden_options.add_options()
			(ENABLE_HELLINGER_THREE_PLATE_OPTION_NAME, "Enable three-plate hellinger fitting.");

		boost::program_options::variables_map vm;

		try
		{
			GPlatesUtils::CommandLineParser::parse_command_line_options(
				vm, argc, argv, input_options);
		}
		catch (std::exception &exc)
		{
			qWarning() << "Error parsing command-line arguments: " << exc.what();
			exit(1);
		}
		catch(...)
		{
			qWarning() << "Error parsing command-line arguments: Unknown error";
			exit(1);
		}

		// Print GPlates version if requested.
		if (GPlatesUtils::CommandLineParser::is_version_requested(vm))
		{
			// Specify the major.minor version.
			std::cout << GPlatesGlobal::Version::get_GPlates_version().toLatin1().constData() << std::endl;

			// Specify the build revision (using the subversion info).
			QString subversion_version_number = GPlatesGlobal::Version::get_working_copy_version_number();
			if (!subversion_version_number.isEmpty())
			{
				QString subversion_info = "Build: " + subversion_version_number;
				QString subversion_branch_name = GPlatesGlobal::Version::get_working_copy_branch_name();
				if (!subversion_branch_name.isEmpty())
				{
					if (subversion_branch_name == "trunk")
					{
						subversion_info.append(" (trunk)");
					}
					else
					{
						subversion_info.append(" (").append(subversion_branch_name).append(" branch)");
					}
				}
				std::cout << subversion_info.toLatin1().constData() << std::endl;
			}

			exit(0);
		}

		// Print GPlates program usage if requested.
		if (GPlatesUtils::CommandLineParser::is_help_requested(vm))
		{	
			print_usage(std::cout, input_options);
			exit(0);
		}

		// Print the usage for a specific command (non-GUI).
		// Add the help for the non-GUI commands even though it's not related to GUI command-line
		// options - this is because the help printout should still mention it.
		if (vm.count(HELP_COMMAND_OPTION_NAME))
		{
			const std::string command = vm[HELP_COMMAND_OPTION_NAME].as<std::string>();
			print_command_usage(std::cout, input_options, command);
			exit(0);
		}

		// Create our return structure.
		GuiCommandLineOptions command_line_options;

		if (vm.count(POSITIONAL_FILENAMES_OPTION_NAME))
		{
			std::vector<std::string> filenames =
					vm[POSITIONAL_FILENAMES_OPTION_NAME].as<std::vector<std::string> >();
			for (unsigned int i=0; i<filenames.size(); i++)
			{
				const QString filename = QString(filenames[i].c_str());

				// If the filename does not belong to a project file then consider it a feature collection.
				if (filename.endsWith(GPlatesGui::FileIOFeedback::PROJECT_FILENAME_EXTENSION, Qt::CaseInsensitive))
				{
					if (command_line_options.project_filename)
					{
						qWarning() << "More than one project file specified on command-line.";
						exit(1);
					}

					if (!command_line_options.feature_collection_filenames.empty())
					{
						qWarning() << "Cannot specify a project file and feature collection files on command-line.";
						exit(1);
					}

					command_line_options.project_filename = filename;
				}
				else
				{
					if (command_line_options.project_filename)
					{
						qWarning() << "Cannot specify a project file and feature collection files on command-line.";
						exit(1);
					}

					command_line_options.feature_collection_filenames.push_back(filename);
				}
			}
		}

		if (vm.count(FEATURE_COLLECTION_FILENAMES_OPTION_NAME))
		{
			if (command_line_options.project_filename)
			{
				qWarning() << "Cannot specify a project file and feature collection files on command-line.";
				exit(1);
			}

			std::vector<std::string> feature_collection_filenames =
					vm[FEATURE_COLLECTION_FILENAMES_OPTION_NAME].as<std::vector<std::string> >();
			for (unsigned int i=0; i<feature_collection_filenames.size(); i++)
			{
				command_line_options.feature_collection_filenames.push_back(
						feature_collection_filenames[i].c_str());
			}
		}

		if (vm.count(PROJECT_FILENAME_OPTION_NAME))
		{
			const QString project_filename = vm[PROJECT_FILENAME_OPTION_NAME].as<std::string>().c_str();

			if (!project_filename.endsWith(
					GPlatesGui::FileIOFeedback::PROJECT_FILENAME_EXTENSION,
					Qt::CaseInsensitive))
			{
#if defined(Q_OS_MAC)
				// Mac OS X sometimes (when invoking from Finder or 'open' command) adds the
				// '-psn...' command-line argument to the applications argument list
				// (for example '-psn_0_548998').
				// Note that we end up ignoring the '-psn...' option.
				// Also note that it doesn't actually appear in 'argv[]' for some reason.
				if (!project_filename.startsWith("sn_", Qt::CaseInsensitive))
#endif
				{
					qWarning()
							<< "Specified project file does not have a '."
							<< GPlatesGui::FileIOFeedback::PROJECT_FILENAME_EXTENSION
							<< "' filename extension.";
					exit(1);
				}
			}
			else if (!command_line_options.feature_collection_filenames.empty())
			{
				qWarning() << "Cannot specify a project file and feature collection files on command-line.";
				exit(1);
			}
			else if (command_line_options.project_filename)
			{
				qWarning() << "More than one project file specified on command-line.";
				exit(1);
			}
			else
			{
				command_line_options.project_filename = project_filename;
			}
		}

		if (vm.count(DEBUG_GUI_OPTION_NAME))
		{
			command_line_options.debug_gui = true;
		}

		if (vm.count(DATA_MINING_OPTION_NAME))
		{
			command_line_options.enable_data_mining = true;
		}

		//enable symbol-table feature by command line option.
		if (vm.count(SYMBOL_TABLE_OPTION_NAME))
		{
			command_line_options.enable_symbol_table = true;
		}

		if (vm.count(ENABLE_EXTERNAL_SYNCING_OPTION_NAME))
		{
			command_line_options.enable_external_syncing = true;
		}

		if(vm.count(ENABLE_HELLINGER_THREE_PLATE_OPTION_NAME))
		{
			command_line_options.enable_hellinger_three_plate = true;
		}

		// Disable python if command line option specified.
		if (vm.count(NO_PYTHON_OPTION_NAME))
		{
			command_line_options.enable_python = false;
		}

		return command_line_options;
	}


	/**
	 * Parses command-line assuming first argument is a recognised command and executes command.
	 */
	void
	parse_and_run_command(
			const std::string &command,
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
		// GPlatesQApplication is a QApplication that also handles uncaught exceptions in the Qt event thread.
		// NOTE: This enables the console (command-line) version of GPlates to pop up error message
		// dialogs such as QMessageBox (which happens in some file I/O code, but really shouldn't).
		GPlatesGui::GPlatesQApplication qapplication(argc, argv);

		// Add some simple options.
		GPlatesUtils::CommandLineParser::InputOptions input_options;
		input_options.add_simple_options();
		add_help_command_option(input_options);

		// Since we have parsed a command we know that the user must specify a single
		// positional option (ie, not a regular option like "--command <cmd>" or
		// "-c <cmd>" but simply "<cmd>") to select which functionality they desire (and
		// each command has its own group of options used to configure it - these get added later).
		// This is really just letting the boost parser know that it should expect a
		// positional argument so that it parses correctly (we won't retrieve the argument's
		// value from the boost parsers though).
		// NOTE: each positional option must have an associated normal option (that's just
		// how boost positional options work) - it doesn't mean the user can use
		// "--command <cmd>" though (they must still use "<cmd>").
		input_options.positional_options.add(COMMAND_OPTION_NAME, 1);
		input_options.hidden_options.add_options()(COMMAND_OPTION_NAME, "GPlates command");

		//
		// Get the GPlates command dispatcher to add its options depending on the GPlates command.
		//
		command_dispatcher.add_options_for_command(
				command,
				input_options.generic_options,
				input_options.config_options,
				input_options.hidden_options,
				input_options.positional_options);

		boost::program_options::variables_map vm;

		try
		{
			// Parse the command-line options.
			GPlatesUtils::CommandLineParser::parse_command_line_options(
					vm, argc, argv, input_options);
		}
		catch (std::exception &exc)
		{
			qWarning() << "Error parsing command-line arguments: " << exc.what();
			exit(1);
		}
		catch(...)
		{
			qWarning() << "Error parsing command-line arguments: Unknown error";
			exit(1);
		}

		//
		// Get the GPlates command dispatcher to look at the parsed options and run
		// whatever tasks it decides to based on them.
		//
		command_dispatcher.run(command, vm);
	}

	/**
	 * Classifies the type of the first command-line argument.
	 */
	enum FirstCommandLineArgumentType
	{
		FIRST_ARG_IS_COMMAND,
		FIRST_ARG_IS_UNRECOGNISED_COMMAND,
		FIRST_ARG_IS_OPTION,
		FIRST_ARG_IS_FILENAME,
		FIRST_ARG_IS_NONEXISTENT
	};

	/**
	 * Parses the command-line to determine the command specified by the user but
	 * doesn't parse any options specific to that command since we don't yet know the command.
	 */
	FirstCommandLineArgumentType
	get_command(
			std::string &command,
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
		if (argc < 2)
		{
			command.clear();
			// Is there a command-line argument to test even ?
			return FIRST_ARG_IS_NONEXISTENT;
		}

		const std::string first_arg = argv[1];
		command = first_arg;

		// See if the first command-line argument is a recognised command.
		if (!command_dispatcher.is_recognised_command(first_arg))
		{
			if (!first_arg.empty())
			{
				// See if the first argument looks like an option.
				if (first_arg[0] == '-')
				{
					// It looks like an option since it starts with the '-' character.
					return FIRST_ARG_IS_OPTION;
				}

				// See if the first argument is the filename of an existing file.
				if (QFileInfo(QString(first_arg.c_str())).exists())
				{
					return FIRST_ARG_IS_FILENAME;
				}
			}

			// It doesn't look like an option so it's an unrecognised command.
			return FIRST_ARG_IS_UNRECOGNISED_COMMAND;
		}

		return FIRST_ARG_IS_COMMAND;
	}

	/**
	 * Parses command-line options and either:
	 *  1) processes a non-GUI command (with its own options), or
	 *  2) parses GUI command-line options.
	 *
	 * Returns boost::none for case (1) to indicate that the GUI version of GPlates should not
	 * be started (because GPlates is being used only to process a command and then exit).
	 */
	boost::optional<GuiCommandLineOptions>
	process_command_line_options(
			int argc, 
			char *argv[])
	{
		/*
		 * This object handles all interpretation of command-line options for different
		 * commands and executes a specified command.
		 *
		 * We create only one instance of this object because it creates all possible commands
		 * in its constructor and we might as well only do that once.
		 */
		GPlatesCli::CommandDispatcher command_dispatcher;

		// Get the user-specified command (this is the first positional argument on the
		// command-line).
		std::string command;
		const FirstCommandLineArgumentType first_arg_type =
				get_command(command, command_dispatcher, argc, argv);

		switch (first_arg_type)
		{
		case FIRST_ARG_IS_NONEXISTENT:
		case FIRST_ARG_IS_OPTION:
		case FIRST_ARG_IS_FILENAME:
			// First command-line argument was not a recognised command and it didn't
			// look like a command so parse the command-line to see if any
			// GUI options (or simple options such as help and version) were specified.
			//
			// NOTE: This is the only case where GPlates runs as the familiar GUI application.
			return parse_gui_command_line_options(argc, argv);

		case FIRST_ARG_IS_UNRECOGNISED_COMMAND:
			// The first command-line argument was not a recognised command or existing filename
			// but it did not look like an option.
			qWarning()
					<< "First command-line argument '"
					<< command.c_str()
					<< "' does not look like an existing filename, an option or a command.";
			exit(1);
			break; // ...in case compiler complains.

		case FIRST_ARG_IS_COMMAND:
			// Process the specified command.
			parse_and_run_command(command, command_dispatcher, argc, argv);
			// Notify the caller that the GPlates GUI should *not* be started since the user
			// has requested GPlates process a command instead.
			return boost::none;

		default:
			break;
		}

		// We only get here if the above switch statement doesn't catch all cases.
		throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
	}

	void
	initialise_python(
			GPlatesPresentation::Application *app,
			char* argv[])
	{
		using namespace GPlatesGui;
		PythonManager* mgr = PythonManager::instance();
		try
		{
			mgr->initialize(argv,app);
		}
		catch(const PythonInitFailed& ex)
		{
			std::stringstream ss;
			ex.write(ss);
			qWarning() << ss.str().c_str();
			
			if(mgr->show_init_fail_dlg())
			{
				using namespace GPlatesQtWidgets;
				boost::scoped_ptr<PythonInitFailedDialog> python_fail_dlg(
					new PythonInitFailedDialog);

				python_fail_dlg->exec();
				mgr->set_show_init_fail_dlg(python_fail_dlg->show_again());
			}

			GPlatesUtils::ComponentManager::instance().disable(
				GPlatesUtils::ComponentManager::Component::python());
		}
	}

	void
	clean_up()
	{
		// FIXME: If we can merge multiple singletons into a single singleton that would be better
		// from a management/organization point-of-view and also when destructor of single singleton
		// is called then contained objects are destroyed in correct order.
		// Also we should be careful about excessive use of singletons because they are essentially global data.

		if(GPlatesUtils::ComponentManager::instance().is_enabled(
				GPlatesUtils::ComponentManager::Component::python()))
		{
			GPlatesApi::PythonInterpreterLocker lock;
			delete GPlatesGui::DrawStyleManager::instance(); //delete draw style manager singleton.
		}
		delete GPlatesGui::PythonManager::instance();
	}
}

int
internal_main(int argc, char* argv[])
{
	// Initialize Qt resources that exist in the static 'qt-resources' library.
	// NOTE: This is done here so that both the GUI and command-line-only paths have initialized resources.
	//
	// NOTE: According to the QtResources documentation calls to Q_INIT_RESOURCE are not needed if
	// the resources are compiled into a shared library (further, if resources only accessed from
	// within shared library then there's also no issue with the shared library not being loaded yet).
	// So Q_INIT_RESOURCE is not called for the python (API) shared library (since this source
	// file is not included in it) but that's no problem since the python shared library is used
	// externally (ie, used by an external python interpreter not the GPlates embedded interpreter)
	// and so the resources are only accessed internally by the shared library.
	//
	Q_INIT_RESOURCE(opengl);
	Q_INIT_RESOURCE(python);
	Q_INIT_RESOURCE(gpgim);
	Q_INIT_RESOURCE(qt_widgets);

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

	// Process the command-line options.
	// NOTE: We do this before setting up anything GUI-related such as QApplication in case
	// GPlates is being used *only* for command-line processing and then exiting.
	// This is because GPlates now doubles as the familiar GPlates GUI application *and*
	// what was previously a separate GPlates command-line application.
	// GPlates can be used either way depending on the command-line options.
	boost::optional<GuiCommandLineOptions> gui_command_line_options = process_command_line_options(argc, argv);
	if (!gui_command_line_options)
	{
		// Note that a return value of zero (from 'main') means success.
		// If there's an error then an exception would have been thrown and caught in 'main()' via
		// 'GPlatesGui::GPlatesQApplication::call_main()' which logs the error and calls qFatal()
		// which calls exit(1) where the '1' indicates error (since it's non-zero).
		return 0;
	}

	// Enable data mining if specified on the command-line.
	if (gui_command_line_options->enable_data_mining)
	{
		GPlatesUtils::ComponentManager::instance().enable(
				GPlatesUtils::ComponentManager::Component::data_mining());
	}

	// Enable temporary symbol table if specified on the command-line.
	if (gui_command_line_options->enable_symbol_table)
	{
		GPlatesUtils::ComponentManager::instance().enable(
				GPlatesUtils::ComponentManager::Component::symbology());
	}

	// Enable or disable python as specified on command-line.
	if (gui_command_line_options->enable_python)
	{
		GPlatesUtils::ComponentManager::instance().enable(
			GPlatesUtils::ComponentManager::Component::python());
	}
	else
	{
		GPlatesUtils::ComponentManager::instance().disable(
			GPlatesUtils::ComponentManager::Component::python());
	}

	// Enable or disable hellinger tool.
	if (gui_command_line_options->enable_hellinger_three_plate)
	{
		GPlatesUtils::ComponentManager::instance().enable(
			GPlatesUtils::ComponentManager::Component::hellinger_three_plate());
	}
	else
	{
		GPlatesUtils::ComponentManager::instance().disable(
			GPlatesUtils::ComponentManager::Component::hellinger_three_plate());
	}

	// This will only install handler if any of the following conditions are satisfied:
	//   1) GPLATES_PUBLIC_RELEASE is defined in 'global/config.h' (automatically handled by CMake build system), or
	//   2) GPLATES_OVERRIDE_QT_MESSAGE_HANDLER environment variable is set to case-insensitive
	//      "true", "1", "yes" or "on".
	// Note: Installing handler overrides default Qt message handler.
	//       And does not log messages to the console.
	GPlatesAppLogic::GPlatesQtMsgHandler::install_qt_message_handler();

	// Enable high DPI pixmaps (for high DPI displays like Apple Retina).
	//
	// For example this enables a QImage with a device pixel ratio of 2
	// (and twice the dimensions of associated QIcon) to have the QIcon displayed
	// as high DPI, when QImage is converted to QPixmap and then set on QIcon.
	// Enabling this attribute allows this in our globe/map colouring previews
	// (though we still have to manually render a QImage twice the icon dimensions
	// and set the image's device pixel ratio to 2).
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
	// Enable high DPI scaling in Qt on supported platforms (X11 and Windows).
	// Note that MacOS has its own native scaling (eg, for Retina), so this
	// attribute does not affect MacOS.
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

	// GPlatesQApplication is a QApplication that also handles uncaught exceptions in the Qt event thread.
	GPlatesGui::GPlatesQApplication qapplication(argc, argv);

	// GPlatesPresentation::Application is a singleton which is normally only accessed via 'Application::instance()'.
	// However we also need to control its lifetime and ensure it gets destroyed before QApplication
	// otherwise Qt will crash since the QApplication object will already have gone out of scope and
	// afterwards we would be trying to access destructors of QWidgets when the 'Application' is
	// finally destroyed after returning from 'main()'.
	//
	// It can still be accessed via 'Application::instance()' provided 'application' is in scope.
	//
	// An exception will be thrown if 'Application::instance()' is called before here or if it
	// is called after 'application' goes out of scope.
	//
	// Note that python references 'Application' so this should be instantiated before python is initialised.
	GPlatesPresentation::Application application;

	// Initialise python if it's enabled.
	if(GPlatesUtils::ComponentManager::instance().is_enabled(
			GPlatesUtils::ComponentManager::Component::python()))
	{
		initialise_python(&application,argv);
	}

	// Also load a project file or any feature collection files specified on the command-line.
	if (gui_command_line_options->project_filename)
	{
		application.get_main_window().load_project(gui_command_line_options->project_filename.get());
	}
	else if (!gui_command_line_options->feature_collection_filenames.empty())
	{
		application.get_main_window().load_feature_collections(
				gui_command_line_options->feature_collection_filenames);
	}

	// Install an extra menu for developers to help debug GUI problems.
	if (gui_command_line_options->debug_gui)
	{
		application.get_main_window().install_gui_debug_menu();
	}
	// Enable external program syncing with GPlates.
	if (gui_command_line_options->enable_external_syncing)
	{
		application.enable_syncing_with_external_applications();
	}

// 	if (!GPlatesUtils::ComponentManager::instance().is_enabled(
// 		GPlatesUtils::ComponentManager::Component::symbology()))
// 	{
// 		application.get_main_window().hide_symbol_menu();
// 	}

	// Display the main window.
	// This calls QMainWindow::show() and then performs extra actions that depend on the
	// main window being visible.
	application.get_main_window().display();

	// Start the application event loop.
	const int ret = qapplication.exec();

	clean_up();

	return ret;

	// Note: Because we are using Boost.Python, Py_Finalize() should not be called.
}

int
main(int argc, char* argv[])
{
	// The first of two reasons to wrap 'main()' around 'internal_main()' is to
	// handle any uncaught exceptions that occur in main() but outside the Qt event thread.
	// Any uncaught exceptions occurring in the Qt event thread will get caught by the
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

