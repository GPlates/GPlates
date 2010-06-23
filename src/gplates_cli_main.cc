/* $Id$ */

/**
 * \file Command-line interface version of GPlates.
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

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>

#include "cli/CliCommandDispatcher.h"

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "global/Constants.h"
#include "global/GPlatesException.h"
#include "global/NotYetImplementedException.h"
#include "global/SubversionInfo.h"

#include "utils/Profile.h"
#include "utils/CommandLineParser.h"


namespace
{
	/**
	 * Option name to print usage of a specific GPlates command.
	 */
	const char *HELP_COMMAND_OPTION_NAME = "help-command";

	/**
	 * The option name used to extract the first positional command-line argument which
	 * is the GPlates command that the user wishes to execute.
	 * Each command has its own set of options.
	 * This allows the user to select different functionality using a single
	 * command-line GPlates executable.
	 */
	const char *COMMAND_OPTION_NAME = "command";


	/**
	 * Adds the help command option.
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
	 * Prints program usage to @a os.
	 */
	void
	print_usage(
			std::ostream &os,
			const GPlatesCli::CommandDispatcher &command_dispatcher)
	{
		typedef std::vector<GPlatesCli::CommandDispatcher::command_name_and_description_type>
				command_name_and_desc_seq_type;

		// Add the simple options (such as help and version).
		GPlatesUtils::CommandLineParser::InputOptions input_options;
		input_options.add_simple_options();
		add_help_command_option(input_options);


		// Get the list of commands.
		const command_name_and_desc_seq_type command_names_and_descriptions =
				command_dispatcher.get_command_names_and_descriptions();

		// Print an basic introduction about how to use the command-line interface.
		os
				<< "gplates-cli <command> <command options ...>"
				<< std::endl
				<< "            where <command> includes:"
				<< std::endl;

		// Print the list of commands.
		command_name_and_desc_seq_type::const_iterator iter = command_names_and_descriptions.begin();
		command_name_and_desc_seq_type::const_iterator end = command_names_and_descriptions.end();
		for ( ; iter != end; ++iter)
		{
			const std::string &command_name = iter->first;
			const std::string &command_desc = iter->second;
			os << " * " << command_name.c_str() << " - " << command_desc.c_str() << std::endl;
		}

		// Print the visible options.
		os
				<< std::endl
				<< GPlatesUtils::CommandLineParser::get_visible_options(input_options)
				<< std::endl;
	}


	

	/**
	 * Parses the command-line without expecting a command.
	 *
	 * This means only the simple options such as help and version are searched for.
	 */
	void
	parse_simple_options_only(
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
		// Add some simple options.
		GPlatesUtils::CommandLineParser::InputOptions input_options;
		input_options.add_simple_options();
		add_help_command_option(input_options);

		boost::program_options::variables_map vm;

		try
		{
			// Parse the command-line options.
			GPlatesUtils::CommandLineParser::parse_command_line_options(
				vm, argc, argv, input_options);
		}
		catch(std::exception& exc)
		{
			std::cerr<<"Error processing command-line: "<<exc.what()<<std::endl;
		}

		// Print usage if 'help' option is specified.
		if (GPlatesUtils::CommandLineParser::is_help_requested(vm))
		{
			print_usage(std::cout, command_dispatcher);
			return;
		}

		// Print GPlates version if requested.
		if (GPlatesUtils::CommandLineParser::is_version_requested(vm))
		{
			std::cout << GPlatesGlobal::VersionString << std::endl;
			return;
		}

		// Print the usage for a specific command.
		if (vm.count(HELP_COMMAND_OPTION_NAME))
		{
			// Add options for the command specified so that they become visible
			// when we print out the usage for the command.
			const std::string command = vm[HELP_COMMAND_OPTION_NAME].as<std::string>();
			if (!command_dispatcher.is_recognised_command(command))
			{
				// The command was not a recognised command.
				std::cerr
						<< "Command '" << command.c_str() << "' is not a recognised command."
						<< std::endl;
				return;
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
			return;
		}

		// No simple options were encountered so just print the help usage.
		print_usage(std::cout, command_dispatcher);
	}


	/**
	 * Classifies the type of the first command-line argument.
	 */
	enum FirstArgumentType
	{
		FIRST_ARG_IS_COMMAND,
		FIRST_ARG_IS_UNRECOGNISED_COMMAND,
		FIRST_ARG_IS_OPTION,
		FIRST_ARG_IS_NONEXISTENT
	};


	/**
	 * Parses command-line assuming first argument is a recognised command and executes command.
	 */
	int
	parse_and_run_command(
			const std::string &command,
			GPlatesCli::CommandDispatcher &command_dispatcher,
			int argc,
			char* argv[])
	{
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
			parse_command_line_options(
				vm, argc, argv, input_options);
		}
		catch(std::exception& exc)
		{
			std::cerr<<"Error processing command-line: "<<exc.what()<<std::endl;
		}

		//
		// Get the GPlates command dispatcher to look at the parsed options and run
		// whatever tasks it decides to based on them.
		//
		return command_dispatcher.run(command, vm);
	}


	/**
	 * Parses the command-line to determine the command specified by the user but
	 * doesn't parse any options specific to that command since we don't yet know the command.
	 */
	FirstArgumentType
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
			// See if the first argument looks like an option.
			if (!first_arg.empty() && first_arg[0] == '-')
			{
				// It looks like an option since it starts with the '-' character.
				return FIRST_ARG_IS_OPTION;
			}

			// It doesn't look like an option so it's an unrecognised command.
			return FIRST_ARG_IS_UNRECOGNISED_COMMAND;
		}

		return FIRST_ARG_IS_COMMAND;
	}


	/**
	 * The main function minus any exception handling.
	 */
	int
	internal_main(
			int argc,
			char* argv[])
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
		const FirstArgumentType first_arg_type = get_command(
				command, command_dispatcher, argc, argv);
		switch (first_arg_type)
		{
		case FIRST_ARG_IS_NONEXISTENT:
		case FIRST_ARG_IS_OPTION:
			// First command-line argument was not a recognised command and it didn't
			// look like a command so parse the command-line to see if any
			// simple options such as help and version were specified.
			parse_simple_options_only(command_dispatcher, argc, argv);
			return 1;

		case FIRST_ARG_IS_UNRECOGNISED_COMMAND:
			// The first command-line argument was not a recognised command but it did
			// look like a command (rather than an option).
			std::cerr
					<< "Command '" << command.c_str() << "' is not a recognised command."
					<< std::endl;
			return 1;

		case FIRST_ARG_IS_COMMAND:
			return parse_and_run_command(command, command_dispatcher, argc, argv);

		default:
			break;
		}

		// We only get here if the above switch statement doesn't catch all cases.
		throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
	}


	/**
	 * Handles exceptions thrown by @a internal_main.
	 */
	int
	try_catch_internal_main(
			int argc,
			char *argv[])
	{
		std::string error_message_std;
		std::string call_stack_trace_std;

		try
		{
			return internal_main(argc, argv);
		}
		catch (GPlatesGlobal::Exception &exc)
		{
			// Get exception to write its message.
			std::ostringstream ostr_stream;
			ostr_stream << exc;
			error_message_std = ostr_stream.str();

			// Extract the call stack trace to the location where the exception was thrown.
			exc.get_call_stack_trace_string(call_stack_trace_std);
		}
		catch(std::exception& exc)
		{
			error_message_std = exc.what();
		}
		catch (...)
		{
			error_message_std = "unknown exception";
		}

		//
		// If we get here then we caught an exception
		//

		QStringList error_message_stream;
		error_message_stream
				<< QObject::tr("Error: GPlates has caught an unhandled exception: ")
				<< QString::fromStdString(error_message_std);

		QString error_message = error_message_stream.join("");

		// If we have an installed message handler then this will output to a log file.
		qWarning() << error_message;

		// Output the call stack trace if we have one.
		if (!call_stack_trace_std.empty())
		{
			// If we have an installed message handler then this will output to a log file.
			// Also write out the SVN revision number so we know which source code to look
			// at when users send us back a log file.
			qWarning()
					<< QString::fromStdString(call_stack_trace_std)
					<< endl
					<< GPlatesGlobal::SubversionInfo::get_working_copy_version_number();
		}

		// If we have an installed message handler then this will output to a log file.
		// This is where the core dump or debugger trigger happens on debug builds.
		// On release builds this exits the application with a return value of 1.
		qFatal("Exiting due to exception caught");

		// Shouldn't get past qFatal - this just keeps compiler happy.
		return -1;
	}
}


int
main(
		int argc,
		char* argv[])
{
	// The first of two reasons to wrap 'main()' around 'internal_main()' is to
	// handle any uncaught exceptions that occur in main().
	const int return_code = try_catch_internal_main(argc, argv);

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
